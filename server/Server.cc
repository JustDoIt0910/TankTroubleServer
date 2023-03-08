//
// Created by zr on 23-3-5.
//

#include "Server.h"
#include "tinyorm/db.h"
#include "tinyorm/model.h"
#include "model/User.h"

namespace TankTrouble
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;

    const int Server::DefaultMaxRoomNumber = 10;
    const std::string Server::DBHost = "localhost";
    const int Server::DBPort = 3306;
    const std::string Server::DBUserName = "root";
    const std::string Server::DBPassword = "20010910cheng";
    const std::string Server::DBName = "tank_trouble";

    Server::Server(uint16_t port, int maxRoomNumber):
            server_(&loop_, muduo::net::InetAddress(port, true), "TankTroubleServer"),
            manager_(this),
            maxRoomNum_(DefaultMaxRoomNumber),
            db_(new orm::DB(DBHost, DBPort, DBUserName, DBPassword, DBName))
    {
        db_->AutoMigrate(UserInfo());

        server_.setMessageCallback(std::bind(&Codec::handleMessage, &codec_, _1, _2, _3));
        codec_.registerHandler(MSG_LOGIN, std::bind(&Server::onLogin, this, _1, _2, _3));
        codec_.registerHandler(MSG_NEW_ROOM, std::bind(&Server::onCreateRoom, this, _1, _2, _3));
    }

    void Server::onLogin(const muduo::net::TcpConnectionPtr& conn, Message message, muduo::Timestamp receiveTime)
    {
        std::string nickname = message.getField<Field<std::string>>("nickname").get();
        UserInfo user;
        if(db_->model<UserInfo>().Where("nickname = ?", nickname).First(user) < 1)
        {
            user.nickname = nickname;
            user.score = 0;
            user.createTime = Timestamp::now();
            db_->model<UserInfo>().Create(user);
        }
        OnlineUser onlineUser(conn, user.nickname, user.score);
        onlineUsers_[conn->peerAddress().toIpPort()] = onlineUser;
    }

    void Server::onCreateRoom(const muduo::net::TcpConnectionPtr& conn, Message message, muduo::Timestamp receiveTime)
    {
        std::string roomName = message.getField<Field<std::string>>("room_name").get();
        uint8_t roomCap = message.getField<Field<uint8_t>>("player_num").get();
        manager_.createRoom(roomName, roomCap);
    }

    void Server::updateRoomInfos(Manager::RoomInfoList newInfoList)
    {
        loop_.queueInLoop([this,  l = std::move(newInfoList)] () mutable {
            roomInfos_ = std::move(l);
            sendRoomInfos();
        });
    }

    void Server::sendRoomInfos(const std::string& user)
    {
        Message message = codec_.getEmptyMessage(MSG_ROOM_INFO);
        for(const GameRoom::RoomInfo& info: roomInfos_)
        {
            StructField<uint8_t, std::string, uint8_t, uint8_t> elem("", {
                "room_id", "room_name", "room_cap", "room_players"
            });
            elem.set<uint8_t>("room_id", info.roomId_);
            elem.set<std::string>("room_name", info.roomName_);
            elem.set<uint8_t>("room_cap", info.roomCap_);
            elem.set<uint8_t>("room_players", info.playerNum_);
            message.addArrayElement("room_infos", elem);
        }
        if(!user.empty())
        {
            if(onlineUsers_.find(user) != onlineUsers_.end())
                Codec::sendMessage(onlineUsers_[user].conn_, MSG_ROOM_INFO, message);
            return;
        }
        for(const auto& entry: onlineUsers_)
            Codec::sendMessage(entry.second.conn_, MSG_ROOM_INFO, message);
    }

    void Server::serve()
    {
        manager_.start();
        server_.start();
        loop_.loop();
    }

    Server::~Server() {delete db_;}
}