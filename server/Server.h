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
        void onLogin(const muduo::net::TcpConnectionPtr& conn, Message message, muduo::Timestamp);
        void onCreateRoom(const muduo::net::TcpConnectionPtr& conn, Message message, muduo::Timestamp);
        void onJoinRoom(const muduo::net::TcpConnectionPtr& conn, Message message, muduo::Timestamp);
        void onControlMessage(const muduo::net::TcpConnectionPtr& conn, Message message, muduo::Timestamp);

        void roomsInfoBroadcast(Manager::RoomInfoList newInfoList);
        void joinRoomRespond(const std::string& connId, uint8_t roomId, Codec::StatusCode code);
        void notifyGameOn(std::vector<std::pair<std::string, uint8_t>> playersInfo);
        void blocksDataBroadcast(const std::unordered_set<std::string>& connIds, ServerBlockDataList data);
        void objectsDataBroadcast(const std::unordered_set<std::string>& connIds, ServerObjectsData data);
        void sendRoomsInfo(const std::string& connId = std::string());
        void handleDisconnection(const muduo::net::TcpConnectionPtr& conn);

        muduo::net::EventLoop loop_;
        muduo::net::TcpServer server_;
        friend class Manager;
        Manager manager_;
        int maxRoomNum_;
        std::unique_ptr<orm::DB> db_;
        Codec codec_;
        std::unordered_map<std::string, OnlineUser> onlineUsers_;
        Manager::RoomInfoList roomInfos_;
    };
}

#endif //TANK_TROUBLE_SERVER_SERVER_H
