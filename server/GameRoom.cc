//
// Created by zr on 23-3-5.
//

#include "GameRoom.h"
#include "game/Tank.h"
#include "game/util/Math.h"
#include "game/Shell.h"
#include <cassert>

namespace TankTrouble
{
    static util::Vec topBorderCenter(static_cast<double>(GAME_VIEW_WIDTH) / 2, 2.0);
    static util::Vec leftBorderCenter(2.0, static_cast<double>(GAME_VIEW_HEIGHT) / 2);
    static util::Vec bottomBorderCenter(static_cast<double>(GAME_VIEW_WIDTH) / 2,
                                        static_cast<double>(GAME_VIEW_HEIGHT) - 2 - 1);
    static util::Vec rightBorderCenter(static_cast<double>(GAME_VIEW_WIDTH) - 2 - 1,
                                       static_cast<double>(GAME_VIEW_HEIGHT) / 2);

    GameRoom::GameRoom(int id, const std::string& name, int cap):
            roomInfo(id, name, cap, 0, New),
            survivors(0),
            hasWinner(false),
            restartNeeded(false) {}

    /******************************* interfaces for manager ***********************************/

    GameRoom::RoomInfo GameRoom::info() const {return roomInfo;}

    ServerBlockDataList GameRoom::getBlocksData() const
    {
        ServerBlockDataList data;
        for(const auto& entry: blocks)
        {
            const Block& block = entry.second;
            data.emplace_back(block.isHorizon(), block.center());
        }
        return std::move(data);
    }

    ServerObjectsData GameRoom::getObjectsData() const
    {
        ServerObjectsData data;
        for(const auto& entry: objects)
        {
            const auto& obj = entry.second;
            Object::PosInfo pos = obj->getCurrentPosition();
            if(obj->type() == OBJ_SHELL)
                data.shells_.emplace_back(pos.pos.x(), pos.pos.y());
            else
                data.tanks_.emplace_back(entry.first, pos);
        }
        return std::move(data);
    }

    std::unordered_map<uint8_t, uint32_t> GameRoom::getPlayersScore() const {return players;}

    void GameRoom::control(int playerId, int action, bool enable)
    {
        if(objects.find(playerId) == objects.end() || objects[playerId]->type() != OBJ_TANK)
            return;
        Tank* playerTank = dynamic_cast<Tank*>(objects[playerId].get());
        switch (action)
        {
            case Forward: playerTank->forward(enable); break;
            case Backward: playerTank->backward(enable); break;
            case RotateCW: playerTank->rotateCW(enable); break;
            case RotateCCW: playerTank->rotateCCW(enable); break;
            case Fire: fire(playerTank); break;
            default: break;
        }
    }

    void GameRoom::setStatus(GameRoom::RoomStatus newStatus) {roomInfo.roomStatus_ = newStatus;}

    uint8_t GameRoom::newPlayer()
    {
        assert(roomInfo.roomStatus_ != Playing && roomInfo.playerNum_ < roomInfo.roomCap_);
        uint8_t playerId = idManager.getTankId();
        players[playerId] = 0;
        roomInfo.playerNum_++;
        if(roomInfo.roomStatus_ == New)
            roomInfo.roomStatus_ = Waiting;
        return playerId;
    }

    void GameRoom::playerQuit(uint8_t playerId)
    {
        players.erase(playerId);
        idManager.returnTankId(playerId);
        roomInfo.playerNum_--;
    }

    void GameRoom::init()
    {
        idManager.reset();
        objects.clear();
        blocks.clear();
        deletedObjs.clear();
        for(int i = 0; i < HORIZON_GRID_NUMBER; i++)
            for(int j = 0; j < VERTICAL_GRID_NUMBER; j++)
            {
                tankPossibleCollisionBlocks[i][j].clear();
                for(int k = 0; k < 8; k++)
                    shellPossibleCollisionBlocks[i][j][k].clear();
            }
        initBlocks();
        std::vector<Object::PosInfo> playerPositions = getRandomPositions(
                static_cast<int>(players.size()));
        int i = 0;
        for(const auto& player: players)
        {
            std::unique_ptr<Object> tank(
                    new Tank(player.first, playerPositions[i].pos, playerPositions[i].angle));
            objects[tank->id()] = std::move(tank);
            i++;
        }
        survivors = static_cast<int>(players.size());
        hasWinner = false;
    }

    void GameRoom::moveAll()
    {
        deletedObjs.clear();
        for(auto& entry: objects)
        {
            std::unique_ptr<Object>& obj = entry.second;
            Object::PosInfo next = obj->getNextPosition(0, 0);
            Object::PosInfo cur = obj->getCurrentPosition();
            bool countdown = false;
            if(obj->type() == OBJ_SHELL)
            {
                auto* shell = dynamic_cast<Shell*>(obj.get());
                int id = checkShellCollision(cur, next);
                if(id < 0 || id > MAX_TANK_ID)
                {
                    obj->resetNextPosition(getBouncedPosition(cur, next, id));
                    countdown = true;
                }
                else if(id)
                {
                    if((id != shell->tankId() || shell->ttl() < Shell::INITIAL_TTL))
                    {
                        deletedObjs.insert(id);
                        deletedObjs.insert(shell->id());
                        survivors--;
                    }
                }
                if(countdown && shell->countDown() <= 0)
                {
                    deletedObjs.insert(shell->id());
                    if(objects.find(shell->tankId()) != objects.end())
                        dynamic_cast<Tank*>(objects[shell->tankId()].get())->getRemainShell();
                }
            }
            else
            {
                auto* tank = dynamic_cast<Tank*>(obj.get());
                int id = checkTankBlockCollision(cur, next);
                if(id)
                    obj->resetNextPosition(cur);
                if(survivors == 1 && !hasWinner && deletedObjs.find(tank->id()) == deletedObjs.end())
                {
                    hasWinner = true;
                    restartNeeded = true;
                    players[tank->id()]++;
                }
            }
            obj->moveToNextPosition();
        }
        for(int id: deletedObjs)
            objects.erase(id);
    }

    bool GameRoom::needRestart()
    {
        bool val = restartNeeded;
        restartNeeded = false;
        return val;
    }

    /************************************* internal ***************************************/

    void GameRoom::initBlocks()
    {
        maze.generate();
        auto blockPositions = maze.getBlockPositions();
        for(const auto& b: blockPositions)
        {
            Block block(idManager.getBlockId(), b.first, b.second);
            assert(blocks.find(block.id()) == blocks.end());
            blocks[block.id()] = block;
            //将block添加到其相邻六个格子对应的碰撞可能列表中
            int gx = (block.start().x() - GRID_SIZE / 2 > 0) ?
                     MAP_REAL_X_TO_GRID_X(block.start().x() - GRID_SIZE / 2) : -1;
            int gy = (block.start().y() - GRID_SIZE / 2 > 0) ?
                     MAP_REAL_Y_TO_GRID_Y(block.start().y() - GRID_SIZE / 2) : -1;;
            if(block.isHorizon())
            {
                if(gx >= 0)
                {
                    //左上
                    shellPossibleCollisionBlocks[gx][gy][DOWNWARDS].push_back(block.id());
                    shellPossibleCollisionBlocks[gx][gy][DOWNWARDS_RIGHT].push_back(block.id());
                    shellPossibleCollisionBlocks[gx][gy][RIGHT].push_back(block.id());
                    tankPossibleCollisionBlocks[gx][gy].push_back(block.id());
                    //左下
                    gy += 1;
                    shellPossibleCollisionBlocks[gx][gy][UPWARDS].push_back(block.id());
                    shellPossibleCollisionBlocks[gx][gy][UPWARDS_RIGHT].push_back(block.id());
                    shellPossibleCollisionBlocks[gx][gy][RIGHT].push_back(block.id());
                    tankPossibleCollisionBlocks[gx][gy].push_back(block.id());
                    gy -= 1;
                }
                //上中
                gx += 1;
                shellPossibleCollisionBlocks[gx][gy][DOWNWARDS].push_back(block.id());
                shellPossibleCollisionBlocks[gx][gy][DOWNWARDS_RIGHT].push_back(block.id());
                shellPossibleCollisionBlocks[gx][gy][DOWNWARDS_LEFT].push_back(block.id());
                tankPossibleCollisionBlocks[gx][gy].push_back(block.id());
                //下中
                gy += 1;
                shellPossibleCollisionBlocks[gx][gy][UPWARDS].push_back(block.id());
                shellPossibleCollisionBlocks[gx][gy][UPWARDS_RIGHT].push_back(block.id());
                shellPossibleCollisionBlocks[gx][gy][UPWARDS_LEFT].push_back(block.id());
                tankPossibleCollisionBlocks[gx][gy].push_back(block.id());
                gy -= 1; gx += 1;
                if(gx < HORIZON_GRID_NUMBER)
                {
                    //右上
                    shellPossibleCollisionBlocks[gx][gy][DOWNWARDS].push_back(block.id());
                    shellPossibleCollisionBlocks[gx][gy][DOWNWARDS_LEFT].push_back(block.id());
                    shellPossibleCollisionBlocks[gx][gy][LEFT].push_back(block.id());
                    tankPossibleCollisionBlocks[gx][gy].push_back(block.id());
                    //右下
                    gy += 1;
                    shellPossibleCollisionBlocks[gx][gy][UPWARDS].push_back(block.id());
                    shellPossibleCollisionBlocks[gx][gy][UPWARDS_LEFT].push_back(block.id());
                    shellPossibleCollisionBlocks[gx][gy][LEFT].push_back(block.id());
                    tankPossibleCollisionBlocks[gx][gy].push_back(block.id());
                }
            }
            else
            {
                if(gy >= 0)
                {
                    //左上
                    shellPossibleCollisionBlocks[gx][gy][DOWNWARDS].push_back(block.id());
                    shellPossibleCollisionBlocks[gx][gy][DOWNWARDS_RIGHT].push_back(block.id());
                    shellPossibleCollisionBlocks[gx][gy][RIGHT].push_back(block.id());
                    tankPossibleCollisionBlocks[gx][gy].push_back(block.id());
                    //右上
                    gx += 1;
                    shellPossibleCollisionBlocks[gx][gy][DOWNWARDS].push_back(block.id());
                    shellPossibleCollisionBlocks[gx][gy][DOWNWARDS_LEFT].push_back(block.id());
                    shellPossibleCollisionBlocks[gx][gy][LEFT].push_back(block.id());
                    tankPossibleCollisionBlocks[gx][gy].push_back(block.id());
                    gx -= 1;
                }
                //左中
                gy += 1;
                shellPossibleCollisionBlocks[gx][gy][RIGHT].push_back(block.id());
                shellPossibleCollisionBlocks[gx][gy][UPWARDS_RIGHT].push_back(block.id());
                shellPossibleCollisionBlocks[gx][gy][DOWNWARDS_RIGHT].push_back(block.id());
                tankPossibleCollisionBlocks[gx][gy].push_back(block.id());
                //右中
                gx += 1;
                shellPossibleCollisionBlocks[gx][gy][LEFT].push_back(block.id());
                shellPossibleCollisionBlocks[gx][gy][UPWARDS_LEFT].push_back(block.id());
                shellPossibleCollisionBlocks[gx][gy][DOWNWARDS_LEFT].push_back(block.id());
                tankPossibleCollisionBlocks[gx][gy].push_back(block.id());
                gx -= 1; gy += 1;
                if(gy < VERTICAL_GRID_NUMBER)
                {
                    //左下
                    shellPossibleCollisionBlocks[gx][gy][UPWARDS].push_back(block.id());
                    shellPossibleCollisionBlocks[gx][gy][RIGHT].push_back(block.id());
                    shellPossibleCollisionBlocks[gx][gy][UPWARDS_RIGHT].push_back(block.id());
                    tankPossibleCollisionBlocks[gx][gy].push_back(block.id());
                    //右下
                    gx += 1;
                    shellPossibleCollisionBlocks[gx][gy][UPWARDS].push_back(block.id());
                    shellPossibleCollisionBlocks[gx][gy][LEFT].push_back(block.id());
                    shellPossibleCollisionBlocks[gx][gy][UPWARDS_LEFT].push_back(block.id());
                    tankPossibleCollisionBlocks[gx][gy].push_back(block.id());
                }
            }
        }
    }

    int GameRoom::checkShellCollision(const Object::PosInfo& curPos, const Object::PosInfo& nextPos)
    {
        int collisionBlock = checkShellBlockCollision(curPos, nextPos);
        if(collisionBlock) return collisionBlock;
        return checkShellTankCollision(curPos, nextPos);
    }

    int GameRoom::checkShellBlockCollision(const Object::PosInfo& curPos, const Object::PosInfo& nextPos)
    {
        if(nextPos.pos.x() < Shell::RADIUS + Block::BLOCK_WIDTH || nextPos.pos.x() > GAME_VIEW_WIDTH - 1 - Block::BLOCK_WIDTH)
            return VERTICAL_BORDER_ID;
        if(nextPos.pos.y() < Shell::RADIUS + Block::BLOCK_WIDTH || nextPos.pos.y() > GAME_VIEW_HEIGHT - 1 - Block::BLOCK_WIDTH)
            return HORIZON_BORDER_ID;

        util::Vec grid(MAP_REAL_TO_GRID(curPos.pos.x(), curPos.pos.y()));
        static double degreeRange[] = {0.0, 90.0, 180.0, 270.0, 360.0};
        static int directions[] = {RIGHT, UPWARDS_RIGHT, UPWARDS, UPWARDS_LEFT,
                                   LEFT, DOWNWARDS_LEFT, DOWNWARDS, DOWNWARDS_RIGHT};
        int dir = 0;
        for(int i = 0; i < 4; i++)
        {
            if(curPos.angle == degreeRange[i])
            {
                dir = directions[2 * i];
                break;
            }
            else if(curPos.angle > degreeRange[i] && curPos.angle < degreeRange[i + 1])
            {
                dir = directions[2 * i + 1];
                break;
            }
        }
        for(int blockId: shellPossibleCollisionBlocks[static_cast<int>(grid.x())][static_cast<int>(grid.y())][dir])
        {
            Block block = blocks[blockId];
            util::Vec v1 = block.isHorizon() ? util::Vec(1, 0) : util::Vec(0, 1);
            util::Vec v2 = block.isHorizon() ? util::Vec(0, 1) : util::Vec(1, 0);
            if(util::checkRectCircleCollision(v1, v2, block.center(), nextPos.pos,
                                              block.width(), block.height(), Shell::RADIUS))
                return blockId;
        }
        return 0;
    }

    int GameRoom::checkShellTankCollision(const Object::PosInfo& curPos, const Object::PosInfo& nextPos)
    {
        for(int id = MIN_TANK_ID; id <= MAX_TANK_ID; id++)
        {
            if(objects.find(id) == objects.end())
                continue;
            Tank* tank = dynamic_cast<Tank*>(objects[id].get());
            auto axis = util::getUnitVectors(tank->getCurrentPosition().angle);
            if(util::checkRectCircleCollision(axis.first, axis.second, tank->getCurrentPosition().pos, nextPos.pos,
                                              Tank::TANK_WIDTH - 2, Tank::TANK_HEIGHT - 2, Shell::RADIUS))
                return id;
        }
        return 0;
    }

    int GameRoom::checkTankBlockCollision(const Object::PosInfo& curPos, const Object::PosInfo& nextPos)
    {
        util::Vec grid(MAP_REAL_TO_GRID(curPos.pos.x(), curPos.pos.y()));
        for(int blockId: tankPossibleCollisionBlocks[static_cast<int>(grid.x())][static_cast<int>(grid.y())])
        {
            Block block = blocks[blockId];
            double blockAngle = block.isHorizon() ? 0.0 : 90.0;
            if(util::checkRectRectCollision(blockAngle, block.center(), block.width(), block.height(),
                                            nextPos.angle, nextPos.pos, Tank::TANK_WIDTH, Tank::TANK_HEIGHT))
                return blockId;
        }
        if(util::checkRectRectCollision(90.0, leftBorderCenter, 4.0, GAME_VIEW_HEIGHT, nextPos.angle, nextPos.pos, Tank::TANK_WIDTH, Tank::TANK_HEIGHT) ||
           util::checkRectRectCollision(90.0, rightBorderCenter, 4.0, GAME_VIEW_HEIGHT, nextPos.angle, nextPos.pos, Tank::TANK_WIDTH, Tank::TANK_HEIGHT))
            return VERTICAL_BORDER_ID;
        if(util::checkRectRectCollision(0.0, topBorderCenter, 4.0, GAME_VIEW_WIDTH, nextPos.angle, nextPos.pos, Tank::TANK_WIDTH, Tank::TANK_HEIGHT) ||
           util::checkRectRectCollision(0.0, bottomBorderCenter, 4.0, GAME_VIEW_WIDTH, nextPos.angle, nextPos.pos, Tank::TANK_WIDTH, Tank::TANK_HEIGHT))
            return VERTICAL_BORDER_ID;
        return 0;
    }

    Object::PosInfo GameRoom::getBouncedPosition(const Object::PosInfo& cur, const Object::PosInfo& next, int blockId)
    {
        Object::PosInfo bounced = next;
        if(blockId < 0)
        {
            bounced.angle = (blockId == VERTICAL_BORDER_ID) ? util::angleFlipY(next.angle) : util::angleFlipX(next.angle);
            return bounced;
        }
        Block block = blocks[blockId];
        for(int i = 0; i < 4; i++)
        {
            std::pair<util::Vec, util::Vec> b = block.border(i);
            if(!util::intersectionOfSegments(cur.pos, next.pos, b.first, b.second, &bounced.pos))
                continue;
            if(i < 2)
                bounced.angle = util::angleFlipX(cur.angle);
            else
                bounced.angle = util::angleFlipY(cur.angle);
        }
        return bounced;
    }

    std::vector<Object::PosInfo> GameRoom::getRandomPositions(int num)
    {
        std::vector<Object::PosInfo> pos;
        std::unordered_set<std::pair<int, int>, PairHash> s;
        while(s.size() < num)
        {
            int x = util::getRandomNumber(0, HORIZON_GRID_NUMBER - 1);
            int y = util::getRandomNumber(0, VERTICAL_GRID_NUMBER - 1);
            s.insert(std::make_pair(x, y));
        }
        for(const auto& p: s)
        {
            int i = util::getRandomNumber(0, 3);
            pos.emplace_back(util::Vec(MAP_GRID_TO_REAL(p.first, p.second)), i * 90.0);
        }
        return pos;
    }

    void GameRoom::fire(Tank* tank)
    {
        if(tank->remainShells() == 0)
            return;
        std::unique_ptr<Object> shell(tank->makeShell(idManager.getShellId()));
        objects[shell->id()] = std::move(shell);
    }
}