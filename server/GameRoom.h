//
// Created by zr on 23-3-5.
//

#ifndef TANK_TROUBLE_SERVER_GAME_ROOM_H
#define TANK_TROUBLE_SERVER_GAME_ROOM_H
#include <cstdint>
#include <string>
#include "game/util/IdManager.h"

namespace TankTrouble
{
    class GameRoom
    {
    public:
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
        RoomInfo info();
        void setStatus(GameRoom::RoomStatus newStatus);

    private:
        util::IdManager idManager;
        RoomInfo roomInfo_;
        std::unordered_set<uint8_t> playerIds;
    };
}

#endif //TANK_TROUBLE_SERVER_GAME_ROOM_H
