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
        server(server),
        managerLoop(nullptr),
        started(false),
        nextRoomId(1) {}

    Manager::~Manager()
    {
        if(started)
        {
            managerLoop->quit();
            managerThread.join();
        }
    }

    /************************************ Interfaces for server **************************************/

    void Manager::createRoom(const std::string& name, int cap)
    {managerLoop->queueInLoop([this, name, cap] () { manageCreateRoom(name, cap);});}

    void Manager::joinRoom(const std::string& connId, uint8_t roomId)
    {managerLoop->queueInLoop([this, connId, roomId] () { manageJoinRoom(connId, roomId);});}

    void Manager::quitRoom(const std::string &connId)
    {managerLoop->queueInLoop([this, connId] () { manageQuitRoom(connId);});}

    void Manager::control(const std::string& connId, int action, bool enable)
    {
        managerLoop->queueInLoop([this, connId, action, enable] () {
            manageControl(connId, action, enable);
        });
    }

    void Manager::start()
    {
        assert(started == false);
        managerThread = std::thread([this] () {manage();});
        {
            std::unique_lock<std::mutex> lk(mu);
            cond.wait(lk, [this](){return started;});
        }
    }

    /***************************************** Internal **********************************************/

    void Manager::manageCreateRoom(const std::string &name, int cap)
    {
        managerLoop->assertInLoopThread();
        auto newRoom = std::make_unique<GameRoom>(nextRoomId++, name, cap);
        rooms[newRoom->info().roomId_] = std::move(newRoom);
        updateRoomsInfo();
    }

    void Manager::manageJoinRoom(const std::string& connId, uint8_t roomId)
    {
        managerLoop->assertInLoopThread();
        if(playersInfo.find(connId) != playersInfo.end())
        {
            server->joinRoomRespond(connId, roomId, Codec::ERR_IS_IN_ROOM);
            return;
        }
        if(rooms.find(roomId) == rooms.end())
        {
            server->joinRoomRespond(connId, roomId, Codec::ERR_ROOM_NOT_EXIST);
            return;
        }
        if(rooms[roomId]->info().roomStatus_ == GameRoom::Playing ||
            rooms[roomId]->info().playerNum_ == rooms[roomId]->info().roomCap_)
        {
            server->joinRoomRespond(connId, roomId, Codec::ERR_ROOM_FULL);
            return;
        }
        uint8_t playerId = rooms[roomId]->newPlayer();
        playersInfo[connId] = PlayerInfo(roomId, playerId);
        connIdsInRoom[roomId].insert(connId);
        assert(rooms[roomId]->info().playerNum_ == connIdsInRoom[roomId].size());
        server->joinRoomRespond(connId, roomId, Codec::JOIN_ROOM_SUCCESS);
        updateRoomsInfo();
    }

    void Manager::manageQuitRoom(const std::string &connId)
    {
        if(playersInfo.find(connId) == playersInfo.end())
            return;
        PlayerInfo player = playersInfo[connId];
        if(rooms.find(player.roomId_) == rooms.end())
            return;
        rooms[player.roomId_]->playerQuit(player.playerId_);
        connIdsInRoom[player.roomId_].erase(connId);
        assert(rooms[player.roomId_]->info().playerNum_ == connIdsInRoom[player.roomId_].size());
        playersInfo.erase(connId);
        updateRoomsInfo();
    }

    void Manager::manageControl(const std::string& connId, int action, bool enable)
    {
        if(playersInfo.find(connId) == playersInfo.end())
            return;
        PlayerInfo player = playersInfo[connId];
        if(rooms.find(player.roomId_) == rooms.end())
            return;
        rooms[player.roomId_]->control(player.playerId_, action, enable);
    }

    void Manager::manage()
    {
        muduo::net::EventLoop loop;
        {
            std::unique_lock<std::mutex> lk(mu);
            managerLoop = &loop;
            started = true;
            cond.notify_all();
        }
        loop.runEvery(0.01, [this] () {manageGames();});
        loop.loop();
        {
            std::unique_lock<std::mutex> lk(mu);
            managerLoop = nullptr;
        }
    }

    void Manager::updateRoomsInfo()
    {
        RoomInfoList infos;
        for(const auto& room: rooms)
            infos.push_back(room.second->info());
        server->roomsInfoBroadcast(infos);
    }

    void Manager::restartRoom(int roomId)
    {
        managerLoop->runAfter(2.0, [this, roomId] () {
            rooms[roomId]->init();
            ServerBlockDataList blocksData = rooms[roomId]->getBlocksData();
            server->blocksDataBroadcast(connIdsInRoom[roomId], std::move(blocksData));
        });
    }

    void Manager::manageGames()
    {
        for(auto& entry: rooms)
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
                server->notifyGameOn(players);
                room->init();
                ServerBlockDataList blocksData = room->getBlocksData();
                server->blocksDataBroadcast(connIdsInRoom[info.roomId_], std::move(blocksData));
            }
            else if(info.roomStatus_ == GameRoom::Playing)
            {
                if(info.playerNum_ < info.roomCap_)
                {
                    // TODO notify game off
                    continue;
                }
                room->moveAll();
                ServerObjectsData objectsData = room->getObjectsData();
                server->objectsDataBroadcast(connIdsInRoom[info.roomId_], std::move(objectsData));
                if(room->needRestart())
                {
                    restartRoom(room->info().roomId_);
                    auto scores = room->getPlayersScore();
                    server->playersScoreBroadcast(connIdsInRoom[info.roomId_], std::move(scores));
                }
            }
        }
    }
}