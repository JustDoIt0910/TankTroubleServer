//
// Created by zr on 23-3-5.
//

#include "GameRoom.h"
#include "game/Tank.h"
#include "game/util/Math.h"
#include <cassert>

namespace TankTrouble
{
    GameRoom::GameRoom(int id, const std::string& name, int cap):
            roomInfo_(id, name, cap, 0, New) {}

    GameRoom::RoomInfo GameRoom::info() const {return roomInfo_;}

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

    void GameRoom::setStatus(GameRoom::RoomStatus newStatus) {roomInfo_.roomStatus_ = newStatus;}

    uint8_t GameRoom::newPlayer()
    {
        assert(roomInfo_.roomStatus_ != Playing && roomInfo_.playerNum_ < roomInfo_.roomCap_);
        uint8_t playerId = idManager.getTankId();
        playerIds.insert(playerId);
        roomInfo_.playerNum_++;
        if(roomInfo_.roomStatus_ == New)
            roomInfo_.roomStatus_ = Waiting;
        return playerId;
    }

    void GameRoom::playerQuit(uint8_t playerId)
    {
        playerIds.erase(playerId);
        idManager.returnTankId(playerId);
        roomInfo_.playerNum_--;
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
                static_cast<int>(playerIds.size()));
        int i = 0;
        for(int playerId: playerIds)
        {
            std::unique_ptr<Object> tank(
                    new Tank(playerId, playerPositions[i].pos, playerPositions[i].angle));
            objects[tank->id()] = std::move(tank);
            i++;
        }
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
}