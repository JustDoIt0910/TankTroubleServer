//
// Created by zr on 23-3-5.
//

#ifndef TANK_TROUBLE_SERVER_GAME_ROOM_H
#define TANK_TROUBLE_SERVER_GAME_ROOM_H
#include <cstdint>
#include <string>

namespace TankTrouble
{
    class GameRoom
    {
    public:
        enum RoomStatus {Waiting, Playing};
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
        RoomInfo info();

    private:
        RoomInfo roomInfo_;
    };
}

#endif //TANK_TROUBLE_SERVER_GAME_ROOM_H
