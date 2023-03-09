//
// Created by zr on 23-3-6.
//

#ifndef TANK_TROUBLE_SERVER_CODEC_H
#define TANK_TROUBLE_SERVER_CODEC_H
#include <unordered_map>
#include <functional>
#include "Messages.h"
#include "muduo/net/TcpConnection.h"

#define MSG_LOGIN               0x10
#define MSG_LOGIN_RESP          0x11

#define MSG_NEW_ROOM            0x20
#define MSG_GET_ROOM            0x21
#define MSG_ROOM_INFO           0x22
#define MSG_JOIN_ROOM           0x23


namespace TankTrouble
{
    class Codec
    {
    public:
        typedef std::function<void(const muduo::net::TcpConnectionPtr& conn,
                Message,
                muduo::Timestamp receiveTime)> MessageHandler;
        Codec();
        void handleMessage(const muduo::net::TcpConnectionPtr& conn,
                           muduo::net::Buffer* buf,
                           muduo::Timestamp receiveTime);
        static void sendMessage(const muduo::net::TcpConnectionPtr& conn, int messageType, const Message& message);
        Message getEmptyMessage(int messageType);
        void registerHandler(int messageType, MessageHandler handler);

    private:
        std::unordered_map<int, MessageTemplate> messages_;
        std::unordered_map<int, MessageHandler> handlers_;
    };
}

#endif //TANK_TROUBLE_SERVER_CODEC_H
