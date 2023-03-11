//
// Created by zr on 23-3-8.
//

#include "Manager.h"
#include "Server.h"
#include "GameRoom.h"
#include "muduo/net/EventLoop.h"
#include <cassert>

namespace TankTrouble
{
    Manager::Manager(Server* server):
        server_(server),
        managerLoop_(nullptr),
        started_(false),
        nextRoomId_(1) {}

    Manager::~Manager()
    {
        if(started_)
        {
            managerLoop_->quit();
            managerThread_.join();
        }
    }

    /************************************ Interfaces for server **************************************/

    void Manager::createRoom(const std::string& name, int cap)
    {managerLoop_->queueInLoop([this, name, cap] () { manageCreateRoom(name, cap);});}

    void Manager::joinRoom(const std::string& connId, uint8_t roomId)
    {managerLoop_->queueInLoop([this, connId, roomId] () { manageJoinRoom(connId, roomId);});}

    void Manager::quitRoom(const std::string &connId)
    {managerLoop_->queueInLoop([this, connId] () { manageQuitRoom(connId);});}

    void Manager::start()
    {
        assert(started_ == false);
        managerThread_ = std::thread([this] () {manage();});
        {
            std::unique_lock<std::mutex> lk(mu_);
            cond_.wait(lk, [this](){return started_;});
        }
    }

    /***************************************** Internal **********************************************/

    void Manager::manageCreateRoom(const std::string &name, int cap)
    {
        managerLoop_->assertInLoopThread();
        auto newRoom = std::make_unique<GameRoom>(nextRoomId_++, name, cap);
        rooms_[newRoom->info().roomId_] = std::move(newRoom);
        updateRoomsInfo();
    }

    void Manager::manageJoinRoom(const std::string& connId, uint8_t roomId)
    {
        managerLoop_->assertInLoopThread();
        if(playersInfo.find(connId) != playersInfo.end())
        {
            server_->joinRoomRespond(connId, roomId, Codec::ERR_IS_IN_ROOM);
            return;
        }
        if(rooms_.find(roomId) == rooms_.end())
        {
            server_->joinRoomRespond(connId, roomId, Codec::ERR_ROOM_NOT_EXIST);
            return;
        }
        if(rooms_[roomId]->info().roomStatus_ == GameRoom::Playing ||
            rooms_[roomId]->info().playerNum_ == rooms_[roomId]->info().roomCap_)
        {
            server_->joinRoomRespond(connId, roomId, Codec::ERR_ROOM_FULL);
            return;
        }
        uint8_t playerId = rooms_[roomId]->newPlayer();
        playersInfo[connId] = PlayerInfo(roomId, playerId, 0);
        connIdsInRoom[roomId].insert(connId);
        assert(rooms_[roomId]->info().playerNum_ == connIdsInRoom[roomId].size());
        server_->joinRoomRespond(connId, roomId, Codec::JOIN_ROOM_SUCCESS);
        updateRoomsInfo();
    }

    void Manager::manageQuitRoom(const std::string &connId)
    {
        if(playersInfo.find(connId) == playersInfo.end())
            return;
        PlayerInfo player = playersInfo[connId];
        if(rooms_.find(player.roomId_) == rooms_.end())
            return;
        rooms_[player.roomId_]->playerQuit(player.playerId_);
        connIdsInRoom[player.roomId_].erase(connId);
        assert(rooms_[player.roomId_]->info().playerNum_ == connIdsInRoom[player.roomId_].size());
        playersInfo.erase(connId);
        updateRoomsInfo();
    }

    void Manager::manage()
    {
        muduo::net::EventLoop loop;
        {
            std::unique_lock<std::mutex> lk(mu_);
            managerLoop_ = &loop;
            started_ = true;
            cond_.notify_all();
        }
        loop.runEvery(0.01, [this] () {manageGames();});
        loop.loop();
        {
            std::unique_lock<std::mutex> lk(mu_);
            managerLoop_ = nullptr;
        }
    }

    void Manager::updateRoomsInfo()
    {
        RoomInfoList infos;
        for(const auto& room: rooms_)
            infos.push_back(room.second->info());
        server_->roomsInfoBroadcast(infos);
    }

    void Manager::manageGames()
    {
        for(auto& entry: rooms_)
        {
            GameRoomPtr& room = entry.second;
            GameRoom::RoomInfo info = room->info();
            if(info.roomStatus_ == GameRoom::Waiting && info.playerNum_ == info.roomCap_)
            {
                room->setStatus(GameRoom::Playing);
                std::vector<std::pair<std::string, uint8_t>> players;
                for(const std::string& connId: connIdsInRoom[info.roomId_])
                {
                    uint8_t playerId = playersInfo[connId].playerId_;
                    players.emplace_back(connId, playerId);
                }
                server_->notifyGameOn(players);
                room->init();
                ServerBlockDataList blocksData = room->getBlocksData();
                server_->blocksDataBroadcast(connIdsInRoom[info.roomId_], std::move(blocksData));
            }
            else if(info.roomStatus_ == GameRoom::Playing)
            {
                if(info.playerNum_ < info.roomCap_)
                {
                    // TODO notify game off
                    continue;
                }
                ServerObjectsData objectsData = room->getObjectsData();
                server_->objectsDataBroadcast(connIdsInRoom[info.roomId_], std::move(objectsData));
            }
        }
    }
}