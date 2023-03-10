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
            maxRoomNum_(maxRoomNumber),
            db_(new orm::DB(DBHost, DBPort, DBUserName, DBPassword, DBName))
    {
        db_->AutoMigrate(UserInfo());

        server_.setConnectionCallback(std::bind(&Server::handleDisconnection, this, _1));
        server_.setMessageCallback(std::bind(&Codec::handleMessage, &codec_, _1, _2, _3));
        codec_.registerHandler(MSG_LOGIN,
                               std::bind(&Server::onLogin, this, _1, _2, _3));
        codec_.registerHandler(MSG_NEW_ROOM,
                               std::bind(&Server::onCreateRoom, this, _1, _2, _3));
        codec_.registerHandler(MSG_JOIN_ROOM,
                               std::bind(&Server::onJoinRoom, this, _1, _2, _3));
    }

    /*************************************** Message handlers ****************************************/

    void Server::onLogin(const muduo::net::TcpConnectionPtr& conn, Message message, muduo::Timestamp)
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
        Message resp = codec_.getEmptyMessage(MSG_LOGIN_RESP);
        resp.setField<Field<std::string>>("nickname", onlineUser.nickname_);
        resp.setField<Field<uint32_t>>("score", onlineUser.score_);
        Codec::sendMessage(conn, MSG_LOGIN_RESP, resp);
        sendRoomsInfo(conn->peerAddress().toIpPort());
    }

    void Server::onCreateRoom(const muduo::net::TcpConnectionPtr& conn, Message message, muduo::Timestamp)
    {
        std::string roomName = message.getField<Field<std::string>>("room_name").get();
        uint8_t roomCap = message.getField<Field<uint8_t>>("player_num").get();
        manager_.createRoom(roomName, roomCap);
    }

    void Server::onJoinRoom(const muduo::net::TcpConnectionPtr& conn, Message message, muduo::Timestamp)
    {
        uint8_t roomId = message.getField<Field<uint8_t>>("join_room_id").get();
        std::string connId = conn->peerAddress().toIpPort();
        if(onlineUsers_.find(connId) == onlineUsers_.end())
            return;
        manager_.joinRoom(connId, roomId);
    }

    /*********************************** Callbacks for manager *****************************************/

    void Server::roomsInfoBroadcast(Manager::RoomInfoList newInfoList)
    {
        loop_.queueInLoop([this,  l = std::move(newInfoList)] () mutable {
            roomInfos_ = std::move(l);
            sendRoomsInfo();
        });
    }

    void Server::joinRoomRespond(const std::string& connId, uint8_t roomId, Codec::StatusCode code)
    {
        loop_.queueInLoop([this, connId, roomId, code] () {
            Message message = codec_.getEmptyMessage(MSG_JOIN_ROOM_RESP);
            message.setField<Field<uint8_t>>("join_room_id", roomId);
            message.setField<Field<uint8_t>>("operation_status", code);
            if(onlineUsers_.find(connId) != onlineUsers_.end())
                Codec::sendMessage(onlineUsers_[connId].conn_, MSG_JOIN_ROOM_RESP, message);
        });
    }

    void Server::notifyGameOn(std::vector<std::pair<std::string, uint8_t>> playersInfo)
    {
        loop_.queueInLoop([this, playersInfo = std::move(playersInfo)] () mutable {
            Message gameOn = codec_.getEmptyMessage(MSG_GAME_ON);
            for(const auto& info: playersInfo)
            {
                std::string connId = info.first;
                if(onlineUsers_.find(connId) == onlineUsers_.end())
                    continue;
                uint8_t playerId = info.second;
                std::string nickname = onlineUsers_[connId].nickname_;
                StructField<uint8_t, std::string> elem("", {"player_id", "player_nickname"});
                elem.set<uint8_t>("player_id", playerId);
                elem.set<std::string>("player_nickname", nickname);
                gameOn.addArrayElement("players_info", elem);
            }
            for(const auto& info: playersInfo)
            {
                std::string connId = info.first;
                if(onlineUsers_.find(connId) == onlineUsers_.end())
                    continue;
                Codec::sendMessage(onlineUsers_[connId].conn_, MSG_GAME_ON, gameOn);
            }
        });
    }

    /************************************** Server internal *********************************************/

    void Server::sendRoomsInfo(const std::string& connId)
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
        if(!connId.empty())
        {
            if(onlineUsers_.find(connId) != onlineUsers_.end())
                Codec::sendMessage(onlineUsers_[connId].conn_, MSG_ROOM_INFO, message);
            return;
        }
        for(const auto& entry: onlineUsers_)
            Codec::sendMessage(entry.second.conn_, MSG_ROOM_INFO, message);
    }

    void Server::handleDisconnection(const muduo::net::TcpConnectionPtr& conn)
    {
        if(conn->disconnected())
        {
            std::string connId = conn->peerAddress().toIpPort();
            if(onlineUsers_.find(connId) == onlineUsers_.end())
                return;
            onlineUsers_.erase(connId);
            manager_.quitRoom(connId);
        }
    }

    void Server::serve()
    {
        manager_.start();
        server_.start();
        loop_.loop();
    }

    Server::~Server() = default;
}