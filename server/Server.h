//
// Created by zr on 23-3-5.
//

#ifndef TANK_TROUBLE_SERVER_SERVER_H
#define TANK_TROUBLE_SERVER_SERVER_H
#include "muduo/net/TcpServer.h"
#include "muduo/net/EventLoop.h"
#include "protocol/Codec.h"
#include "Data.h"
#include "Manager.h"
#include <vector>

namespace orm {class DB;}

namespace TankTrouble
{
    class Server
    {
    public:
        const static int DefaultMaxRoomNumber;
        const static std::string DBHost;
        const static int DBPort;
        const static std::string DBUserName;
        const static std::string DBPassword;
        const static std::string DBName;

        explicit Server(uint16_t port, int maxRoomNumber = DefaultMaxRoomNumber);
        void serve();
        ~Server();

    private:
        void onLogin(const muduo::net::TcpConnectionPtr& conn, Message message, muduo::Timestamp receiveTime);
        void onCreateRoom(const muduo::net::TcpConnectionPtr& conn, Message message, muduo::Timestamp receiveTime);

        void updateRoomInfos(Manager::RoomInfoList newInfoList);
        void sendRoomInfos(const std::string& user = std::string());

        muduo::net::EventLoop loop_;
        muduo::net::TcpServer server_;
        friend class Manager;
        Manager manager_;
        int maxRoomNum_;
        orm::DB* db_;
        Codec codec_;
        std::unordered_map<std::string, OnlineUser> onlineUsers_;
        Manager::RoomInfoList roomInfos_;
    };
}

#endif //TANK_TROUBLE_SERVER_SERVER_H
