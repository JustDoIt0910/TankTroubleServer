//
// Created by zr on 23-3-8.
//

#ifndef TANK_TROUBLE_SERVER_DATA_H
#define TANK_TROUBLE_SERVER_DATA_H
#include <string>
#include "game/util/Vec.h"
#include "game/Object.h"
#include "muduo/net/TcpConnection.h"

namespace TankTrouble
{
    struct OnlineUser
    {
        muduo::net::TcpConnectionPtr conn_;
        struct sockaddr udpAddr_;
        std::string nickname_;
        uint32_t score_;
        bool disconnecting_;

        OnlineUser(): score_(0), disconnecting_(false) {};
        OnlineUser(const muduo::net::TcpConnectionPtr& conn,
                   const std::string& nickname, int score):
            conn_(conn), nickname_(nickname),
            score_(score),disconnecting_(false) {}
    };

    struct PlayerInfo
    {
        uint8_t roomId_;
        uint8_t playerId_;

        PlayerInfo(): roomId_(0), playerId_(0) {}
        PlayerInfo(uint8_t roomId, uint8_t playerId):
            roomId_(roomId), playerId_(playerId) {}
    };

    struct ServerBlockData
    {
        bool horizon_;
        uint64_t centerX_;
        uint64_t centerY_;

        ServerBlockData(bool horizon, util::Vec center):
            horizon_(horizon), centerX_(0), centerY_(0)
        {
            double x = center.x();
            double y = center.y();
            memcpy(&centerX_, &x, sizeof(double));
            memcpy(&centerY_, &y, sizeof(double));
        }
    };

    typedef std::vector<ServerBlockData>ServerBlockDataList;

    struct ServerTankData
    {
        uint8_t id_;
        uint64_t centerX_;
        uint64_t centerY_;
        uint64_t angle_;

        ServerTankData(uint8_t id, Object::PosInfo pos):
            id_(id), centerX_(0), centerY_(0), angle_(0)
        {
            double x = pos.pos.x();
            double y = pos.pos.y();
            double angle = pos.angle;
            memcpy(&centerX_, &x, sizeof(double));
            memcpy(&centerY_, &y, sizeof(double));
            memcpy(&angle_, &angle, sizeof(double));
        }
    };

    struct ServerShellData
    {
        uint64_t x_;
        uint64_t y_;

        ServerShellData(double x, double y): x_(0), y_(0)
        {
            memcpy(&x_, &x, sizeof(double));
            memcpy(&y_, &y, sizeof(double));
        }
    };

    struct ServerObjectsData
    {
        std::vector<ServerTankData> tanks_;
        std::vector<ServerShellData> shells_;

        ServerObjectsData() = default;

        ServerObjectsData(const ServerObjectsData&) = default;

        ServerObjectsData(ServerObjectsData&& data) noexcept :
            tanks_(std::move(data.tanks_)), shells_(std::move(data.shells_)) {}
    };
}

#endif //TANK_TROUBLE_SERVER_DATA_H
