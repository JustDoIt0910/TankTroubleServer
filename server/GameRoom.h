//
// Created by zr on 23-3-5.
//

#ifndef TANK_TROUBLE_SERVER_GAME_ROOM_H
#define TANK_TROUBLE_SERVER_GAME_ROOM_H
#include <cstdint>
#include <string>
#include <memory>
#include <unordered_map>
#include "game/util/IdManager.h"
#include "game/Object.h"
#include "game/Block.h"
#include "game/defs.h"
#include "game/Maze.h"
#include "Data.h"

#define UPWARDS             0
#define UPWARDS_LEFT        1
#define LEFT                2
#define DOWNWARDS_LEFT      3
#define DOWNWARDS           4
#define DOWNWARDS_RIGHT     5
#define RIGHT               6
#define UPWARDS_RIGHT       7

namespace TankTrouble
{
    class Tank;

    class GameRoom
    {
    public:
        enum Action{Forward, Backward, RotateCW, RotateCCW, Fire};

        enum RoomStatus {New, Waiting, Playing};
        struct RoomInfo
        {
            uint8_t roomId_;
            std::string roomName_;
            uint8_t roomCap_;
            uint8_t playerNum_;
            RoomStatus roomStatus_;

            RoomInfo(uint8_t id, const std::string& name,
                     uint8_t cap, uint8_t playerNum,
                     RoomStatus status):
                    roomId_(id), roomName_(name), roomCap_(cap), playerNum_(playerNum),
                    roomStatus_(status) {}
        };

        GameRoom(int id, const std::string& name, int cap);
        uint8_t newPlayer();
        void playerQuit(uint8_t playerId);

        void init();
        RoomInfo info() const;
        ServerBlockDataList getBlocksData() const;
        ServerObjectsData getObjectsData() const;
        void control(int playerId, int action, bool enable);
        void setStatus(GameRoom::RoomStatus newStatus);
        void moveAll();

    private:
        util::IdManager idManager;
        RoomInfo roomInfo_;
        std::unordered_set<uint8_t> playerIds;

        // for game logics
        typedef std::unique_ptr<Object> ObjectPtr;
        typedef std::unordered_map<int, ObjectPtr> ObjectList;
        typedef std::unordered_map<int, Block> BlockList;

        void initBlocks();
        struct PairHash
        {
            template<typename T1, typename T2>
            size_t operator()(const std::pair<T1, T2>& p) const
            {
                return std::hash<T1>()(p.first) ^ std::hash<T2>()(p.second);
            }
        };
        static std::vector<Object::PosInfo> getRandomPositions(int num);
        void fire(Tank* tank);
        int checkShellTankCollision(const Object::PosInfo& curPos, const Object::PosInfo& nextPos);
        int checkShellBlockCollision(const Object::PosInfo& curPos, const Object::PosInfo& nextPos);
        int checkShellCollision(const Object::PosInfo& curPos, const Object::PosInfo& nextPos);
        int checkTankBlockCollision(const Object::PosInfo& curPos, const Object::PosInfo& nextPos);
        Object::PosInfo getBouncedPosition(const Object::PosInfo& cur, const Object::PosInfo& next, int blockId);

        Maze maze;
        ObjectList objects;
        BlockList blocks;
        std::vector<int> deletedObjs;
        std::vector<int> shellPossibleCollisionBlocks[HORIZON_GRID_NUMBER][VERTICAL_GRID_NUMBER][8];
        std::vector<int> tankPossibleCollisionBlocks[HORIZON_GRID_NUMBER][VERTICAL_GRID_NUMBER];
    };
}

#endif //TANK_TROUBLE_SERVER_GAME_ROOM_H
