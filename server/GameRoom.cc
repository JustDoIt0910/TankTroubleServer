//
// Created by zr on 23-3-5.
//

#include "GameRoom.h"
#include <cassert>

namespace TankTrouble
{
    GameRoom::GameRoom(int id, const std::string& name, int cap):
            roomInfo_(id, name, cap, 0, New) {}

    GameRoom::RoomInfo GameRoom::info() {return roomInfo_;}

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
}