//
// Created by zr on 23-3-8.
//

#ifndef TANK_TROUBLE_SERVER_DATA_H
#define TANK_TROUBLE_SERVER_DATA_H
#include <string>
#include "muduo/net/TcpConnection.h"

namespace TankTrouble
{
    struct OnlineUser
    {
        muduo::net::TcpConnectionPtr conn_;
        std::string nickname_;
        uint32_t score_;

        OnlineUser(): score_(0) {};
        OnlineUser(const muduo::net::TcpConnectionPtr& conn, const std::string& nickname, int score):
            conn_(conn), nickname_(nickname), score_(score) {}
    };

    struct PlayerInfo
    {
        uint8_t roomId_;
        uint8_t playerId_;
        uint32_t scoreInGame_;

        PlayerInfo(): roomId_(0), playerId_(0), scoreInGame_(0) {}
        PlayerInfo(uint8_t roomId, uint8_t playerId, uint32_t scoreInGame):
            roomId_(roomId), playerId_(playerId), scoreInGame_(scoreInGame) {}
    };
}

#endif //TANK_TROUBLE_SERVER_DATA_H
