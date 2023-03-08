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
        int score_;

        OnlineUser() = default;
        OnlineUser(const muduo::net::TcpConnectionPtr& conn, const std::string& nickname, int score):
            conn_(conn), nickname_(nickname), score_(score) {}
    };
}

#endif //TANK_TROUBLE_SERVER_DATA_H
