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
            server_(&loop_, muduo::net::InetAddress(port), "TankTroubleServer"),
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
        codec_.registerHandler(MSG_QUIT_ROOM,
                               std::bind(&Server::onQuitRoom, this, _1, _2, _3));
        codec_.registerHandler(MSG_CONTROL,
                               std::bind(&Server::onControlMessage, this, _1, _2, _3));
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
            db_->model<UserInfo>().Select("id").Where("nickname = ?", nickname).First(user);
        }
        
        // 用户重复登录，这可能是因为某一个客户端退出的时候，由于网络中断等原因，fin没有被服务器收到，
        // 所以服务器中还残留着上次连接的信息，而客户端再次连接的时候，ip:port 组合很有可能和上一次不同，
        // 这时服务器要清理掉上次的连接，并且重建 ip:port -> 用户id 的映射关系，才能保证正确性
        if(onlineUsers_.find(user.id) != onlineUsers_.end())
        {
            std::string old = onlineUsers_[user.id].conn_->peerAddress().toIpPort();
            connIdToUserId_.erase(old);
        }
        OnlineUser onlineUser(conn, user.nickname, user.score);
        onlineUsers_[user.id] = onlineUser;
        std::string connId = conn->peerAddress().toIpPort();
        assert(connIdToUserId_.find(connId) == connIdToUserId_.end());
        connIdToUserId_[connId] = user.id;

        Message resp = codec_.getEmptyMessage(MSG_LOGIN_RESP);
        resp.setField<Field<std::string>>("nickname", onlineUser.nickname_);
        resp.setField<Field<uint32_t>>("score", onlineUser.score_);
        Codec::sendMessage(conn, MSG_LOGIN_RESP, resp);
        sendRoomsInfo(user.id);
    }

    void Server::onCreateRoom(const muduo::net::TcpConnectionPtr& conn, Message message, muduo::Timestamp)
    {
        std::string roomName = message.getField<Field<std::string>>("room_name").get();
        uint8_t roomCap = message.getField<Field<uint8_t>>("player_num").get();
        manager_.createRoom(roomName, roomCap);
    }

    void Server::onJoinRoom(const muduo::net::TcpConnectionPtr& conn, Message message, muduo::Timestamp)
    {
        std::string connId = conn->peerAddress().toIpPort();
        if(connIdToUserId_.find(connId) == connIdToUserId_.end())
            return;
        uint8_t roomId = message.getField<Field<uint8_t>>("join_room_id").get();
        manager_.joinRoom(connIdToUserId_[connId], roomId);
    }

    void Server::onQuitRoom(const muduo::net::TcpConnectionPtr& conn, Message message, muduo::Timestamp)
    {
        auto msg = message.getField<Field<std::string>>("msg").get();
        if(msg != "quit")
            return;
        int userId = connIdToUserId_[conn->peerAddress().toIpPort()];
        manager_.quitRoom(userId);
    }

    void Server::onControlMessage(const muduo::net::TcpConnectionPtr& conn, Message message, muduo::Timestamp)
    {
        std::string connId = conn->peerAddress().toIpPort();
        if(connIdToUserId_.find(connId) == connIdToUserId_.end())
            return;
        auto action = message.getField<Field<uint8_t>>("action").get();
        bool enable = message.getField<Field<uint8_t>>("enable").get() == 1;
        manager_.control(connIdToUserId_[connId], action, enable);
    }

    /*********************************** Callbacks for manager *****************************************/

    void Server::roomsInfoBroadcast(Manager::RoomInfoList newInfoList)
    {
        loop_.queueInLoop([this,  l = std::move(newInfoList)] () mutable {
            roomInfos_ = std::move(l);
            sendRoomsInfo();
        });
    }

    void Server::joinRoomRespond(int userId, uint8_t roomId, Codec::StatusCode code)
    {
        loop_.queueInLoop([this, userId, roomId, code] () {
            Message message = codec_.getEmptyMessage(MSG_JOIN_ROOM_RESP);
            message.setField<Field<uint8_t>>("join_room_id", roomId);
            message.setField<Field<uint8_t>>("operation_status", code);
            if(onlineUsers_.find(userId) != onlineUsers_.end() && !onlineUsers_[userId].disconnecting_)
                Codec::sendMessage(onlineUsers_[userId].conn_, MSG_JOIN_ROOM_RESP, message);
        });
    }

    void Server::notifyGameOn(std::vector<std::pair<int, uint8_t>> playersInfo)
    {
        loop_.queueInLoop([this, playersInfo = std::move(playersInfo)] () mutable {
            Message gameOn = codec_.getEmptyMessage(MSG_GAME_ON);
            for(const auto& info: playersInfo)
            {
                int userId = info.first;
                if(onlineUsers_.find(userId) == onlineUsers_.end())
                    continue;
                uint8_t playerId = info.second;
                std::string nickname = onlineUsers_[userId].nickname_;
                StructField<uint8_t, std::string> elem("", {"player_id", "player_nickname"});
                elem.set<uint8_t>("player_id", playerId);
                elem.set<std::string>("player_nickname", nickname);
                gameOn.addArrayElement("players_info", elem);
            }
            for(const auto& info: playersInfo)
            {
                int userId = info.first;
                if(onlineUsers_.find(userId) == onlineUsers_.end() || onlineUsers_[userId].disconnecting_)
                    continue;
                Codec::sendMessage(onlineUsers_[userId].conn_, MSG_GAME_ON, gameOn);
            }
        });
    }

    void Server::notifyGameOff(const std::unordered_set<int>& userIds)
    {
        loop_.queueInLoop([this, userIds] () {
           Message gameOff = codec_.getEmptyMessage(MSG_GAME_OFF);
           gameOff.setField<Field<std::string>>("msg", "off");
            for(int userId: userIds)
            {
                if(onlineUsers_.find(userId) == onlineUsers_.end() || onlineUsers_[userId].disconnecting_)
                    continue;
                Codec::sendMessage(onlineUsers_[userId].conn_,
                                   MSG_GAME_OFF, gameOff);
            }
        });
    }

    void Server::blocksDataBroadcast(const std::unordered_set<int>& userIds,
                                     ServerBlockDataList data)
    {
        loop_.queueInLoop([this, userIds, data = std::move(data)] () mutable {
            Message blockUpdate = codec_.getEmptyMessage(MSG_UPDATE_BLOCKS);
            for(const ServerBlockData& blockData: data)
            {
                StructField<uint8_t, uint64_t, uint64_t> elem("", {
                    "is_horizon", "center_x", "center_y"
                });
                elem.set<uint8_t>("is_horizon", blockData.horizon_ ? 1 : 0);
                elem.set<uint64_t>("center_x", blockData.centerX_);
                elem.set<uint64_t>("center_y", blockData.centerY_);
                blockUpdate.addArrayElement("blocks", elem);
            }
            for(int userId: userIds)
            {
                if(onlineUsers_.find(userId) == onlineUsers_.end() || onlineUsers_[userId].disconnecting_)
                    continue;
                Codec::sendMessage(onlineUsers_[userId].conn_,
                                   MSG_UPDATE_BLOCKS, blockUpdate);
            }
        });
    }

    void Server::objectsDataBroadcast(const std::unordered_set<int>& userIds, ServerObjectsData data)
    {
        loop_.queueInLoop([this, userIds, objs(std::move(data))] () mutable {
            Message objectsUpdate = codec_.getEmptyMessage(MSG_UPDATE_OBJECTS);
            for(const ServerTankData& tank: objs.tanks_)
            {
                StructField<uint8_t, uint64_t, uint64_t, uint64_t> elem("", {
                    "id", "center_x", "center_y", "angle"
                });
                elem.set<uint8_t>("id", tank.id_);
                elem.set<uint64_t>("center_x", tank.centerX_);
                elem.set<uint64_t>("center_y", tank.centerY_);
                elem.set<uint64_t>("angle", tank.angle_);
                objectsUpdate.addArrayElement("tanks", elem);
            }
            for(const ServerShellData& shell: objs.shells_)
            {
                StructField<uint64_t, uint64_t> elem("", {
                        "x", "y"
                });
                elem.set<uint64_t>("x", shell.x_);
                elem.set<uint64_t>("y", shell.y_);
                objectsUpdate.addArrayElement("shells", elem);
            }
            for(int userId: userIds)
            {
                if(onlineUsers_.find(userId) == onlineUsers_.end() || onlineUsers_[userId].disconnecting_)
                    continue;
                Codec::sendMessage(onlineUsers_[userId].conn_,
                                   MSG_UPDATE_OBJECTS, objectsUpdate);
            }
        });
    }

    void Server::playersScoreBroadcast(const std::unordered_set<int>& userIds,
                               std::unordered_map<uint8_t, uint32_t> scores)
    {
        loop_.queueInLoop([this, userIds, scores = std::move(scores)] {
            Message scoresUpdate = codec_.getEmptyMessage(MSG_UPDATE_SCORES);
            for(const auto& entry: scores)
            {
                StructField<uint8_t, uint32_t> elem("", {"player_id", "score"});
                elem.set<uint8_t>("player_id", entry.first);
                elem.set<uint32_t>("score", entry.second);
                scoresUpdate.addArrayElement("scores", elem);
            }
            for(int userId: userIds)
            {
                if(onlineUsers_.find(userId) == onlineUsers_.end() || onlineUsers_[userId].disconnecting_)
                    continue;
                Codec::sendMessage(onlineUsers_[userId].conn_,
                                   MSG_UPDATE_SCORES, scoresUpdate);
            }
        });
    }

    void Server::saveOnlineUserInfo(int userId, uint32_t gameScore)
    {
        loop_.queueInLoop([this, userId, gameScore] () {
            if(onlineUsers_.find(userId) == onlineUsers_.end())
                return;
            onlineUsers_[userId].score_ += gameScore;
            db_->model<UserInfo>().Where("nickname = ?", onlineUsers_[userId].nickname_)
                .Update("score", static_cast<int>(onlineUsers_[userId].score_));
            if(onlineUsers_[userId].disconnecting_)
            {
                std::string connId = onlineUsers_[userId].conn_->peerAddress().toIpPort();
                onlineUsers_.erase(userId);
                assert(connIdToUserId_.find(connId) != connIdToUserId_.end());
                connIdToUserId_.erase(connId);
            }
        });
    }

    /************************************** Server internal *********************************************/

    void Server::sendRoomsInfo(int userId)
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
        if(userId != 0)
        {
            if(onlineUsers_.find(userId) != onlineUsers_.end() && !onlineUsers_[userId].disconnecting_)
                Codec::sendMessage(onlineUsers_[userId].conn_, MSG_ROOM_INFO, message);
            return;
        }
        for(const auto& entry: onlineUsers_)
            if(!entry.second.disconnecting_)
                Codec::sendMessage(entry.second.conn_, MSG_ROOM_INFO, message);
    }

    void Server::handleDisconnection(const muduo::net::TcpConnectionPtr& conn)
    {
        if(conn->disconnected())
        {
            std::string connId = conn->peerAddress().toIpPort();
            int userId = connIdToUserId_[connId];
            if(onlineUsers_.find(userId) == onlineUsers_.end())
                return;
            onlineUsers_[userId].disconnecting_ = true;
            manager_.quitRoom(userId);
        }
        else
            conn->setTcpNoDelay(true);
    }

    void Server::serve()
    {
        manager_.start();
        server_.start();
        loop_.loop();
    }

    Server::~Server() = default;
}