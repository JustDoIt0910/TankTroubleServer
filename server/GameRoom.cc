//
// Created by zr on 23-3-5.
//

#include "GameRoom.h"

namespace TankTrouble
{
    GameRoom::GameRoom(int id, const std::string& name, int cap):
            roomInfo_(id, name, cap, 0, Waiting) {}

    GameRoom::RoomInfo GameRoom::info() {return roomInfo_;}
}