//
// Created by zr on 23-3-5.
//

#ifndef TANK_TROUBLE_SERVER_SERVER_H
#define TANK_TROUBLE_SERVER_SERVER_H
#include "muduo/net/TcpServer.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/Channel.h"
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
        void onQuitRoom(const muduo::net::TcpConnectionPtr& conn, Message message, muduo::Timestamp);
        void onControlMessage(const muduo::net::TcpConnectionPtr& conn, Message message, muduo::Timestamp);

        void roomsInfoBroadcast(Manager::RoomInfoList newInfoList);
        void joinRoomRespond(int userId, uint8_t roomId, Codec::StatusCode code);
        void notifyGameOn(std::vector<std::pair<int, uint8_t>> playersInfo);
        void notifyGameOff(const std::unordered_set<int>& userIds);
        void blocksDataBroadcast(const std::unordered_set<int>& userIds, ServerBlockDataList data);
        void objectsDataBroadcast(const std::unordered_set<int>& userIds, ServerObjectsData data);
        void playersScoreBroadcast(const std::unordered_set<int>& userIds,
                                   std::unordered_map<uint8_t, uint32_t> scores);
        void saveOnlineUserInfo(int userId, uint32_t gameScore);
        void sendRoomsInfo(int userId = 0);
        void handleDisconnection(const muduo::net::TcpConnectionPtr& conn);

        void onUdpHandshake();

        muduo::net::EventLoop loop_;
        muduo::net::TcpServer server_;
        friend class Manager;
        Manager manager_;
        int maxRoomNum_;
        std::unique_ptr<orm::DB> db_;
        Codec codec_;
        int udpSocket_;
        std::unique_ptr<muduo::net::Channel> udpChannel_;
        std::unordered_map<int, OnlineUser> onlineUsers_;
        std::unordered_map<std::string, int> connIdToUserId_;
        Manager::RoomInfoList roomInfos_;
    };
}

#endif //TANK_TROUBLE_SERVER_SERVER_H
