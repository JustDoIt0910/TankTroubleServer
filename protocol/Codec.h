//
// Created by zr on 23-3-6.
//

#ifndef TANK_TROUBLE_SERVER_CODEC_H
#define TANK_TROUBLE_SERVER_CODEC_H
#include <unordered_map>
#include <functional>
#include "Messages.h"
#include "ev/net/TcpConnection.h"

#define MSG_LOGIN               0x10
#define MSG_LOGIN_RESP          0x11
#define MSG_UDP_HANDSHAKE       0x12

#define MSG_NEW_ROOM            0x20
#define MSG_ROOM_INFO           0x21
#define MSG_JOIN_ROOM           0x22
#define MSG_JOIN_ROOM_RESP      0x23
#define MSG_QUIT_ROOM           0x24

#define MSG_GAME_ON             0x30
#define MSG_UPDATE_BLOCKS       0x31
#define MSG_UPDATE_OBJECTS      0x32
#define MSG_UPDATE_SCORES       0x33
#define MSG_GAME_OFF            0x34

#define MSG_CONTROL             0x40


namespace TankTrouble
{
    class Codec
    {
    public:
        enum StatusCode
        {
            // join room
            JOIN_ROOM_SUCCESS = 1,
            ERR_IS_IN_ROOM,
            ERR_ROOM_NOT_EXIST,
            ERR_ROOM_FULL
        };

        typedef std::function<void(const ev::net::TcpConnectionPtr& conn,
                Message,
                ev::Timestamp receiveTime)> MessageHandler;
        Codec();
        void handleMessage(const ev::net::TcpConnectionPtr& conn,
                           ev::net::Buffer* buf,
                           ev::Timestamp receiveTime);
        static ev::net::Buffer packMessage(int messageType, const Message& message);
        static void sendMessage(const ev::net::TcpConnectionPtr& conn, int messageType, const Message& message);
        Message getEmptyMessage(int messageType);
        void registerHandler(int messageType, MessageHandler handler);

    private:
        std::unordered_map<int, MessageTemplate> messages_;
        std::unordered_map<int, MessageHandler> handlers_;
    };
}

#endif //TANK_TROUBLE_SERVER_CODEC_H
