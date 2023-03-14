//
// Created by zr on 23-3-8.
//

#include "Manager.h"
#include "Server.h"
#include "GameRoom.h"
#include "ev/reactor/EventLoop.h"
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

    void Manager::joinRoom(int userId, uint8_t roomId)
    {managerLoop->queueInLoop([this, userId, roomId] () { manageJoinRoom(userId, roomId);});}

    void Manager::quitRoom(int userId)
    {managerLoop->queueInLoop([this, userId] () { manageQuitRoom(userId);});}

    void Manager::control(int userId, int action, bool enable)
    {
        managerLoop->queueInLoop([this, userId, action, enable] () {
            manageControl(userId, action, enable);
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

    void Manager::manageJoinRoom(int userId, uint8_t roomId)
    {
        managerLoop->assertInLoopThread();
        if(playersInfo.find(userId) != playersInfo.end())
        {
            server->joinRoomRespond(userId, roomId, Codec::ERR_IS_IN_ROOM);
            return;
        }
        if(rooms.find(roomId) == rooms.end())
        {
            server->joinRoomRespond(userId, roomId, Codec::ERR_ROOM_NOT_EXIST);
            return;
        }
        if(rooms[roomId]->info().roomStatus_ == GameRoom::Playing ||
            rooms[roomId]->info().playerNum_ == rooms[roomId]->info().roomCap_)
        {
            server->joinRoomRespond(userId, roomId, Codec::ERR_ROOM_FULL);
            return;
        }
        uint8_t playerId = rooms[roomId]->newPlayer();
        playersInfo[userId] = PlayerInfo(roomId, playerId);
        userIdsInRoom[roomId].insert(userId);
        assert(rooms[roomId]->info().playerNum_ == userIdsInRoom[roomId].size());
        server->joinRoomRespond(userId, roomId, Codec::JOIN_ROOM_SUCCESS);
        updateRoomsInfo();
    }

    void Manager::manageQuitRoom(int userId)
    {
        if(playersInfo.find(userId) == playersInfo.end())
            return;
        PlayerInfo player = playersInfo[userId];
        if(rooms.find(player.roomId_) == rooms.end())
            return;
        uint32_t gameScore = rooms[player.roomId_]->playerQuit(player.playerId_);
        userIdsInRoom[player.roomId_].erase(userId);
        assert(rooms[player.roomId_]->info().playerNum_ == userIdsInRoom[player.roomId_].size());
        playersInfo.erase(userId);
        server->saveOnlineUserInfo(userId, gameScore);
        updateRoomsInfo();
    }

    void Manager::manageControl(int userId, int action, bool enable)
    {
        if(playersInfo.find(userId) == playersInfo.end())
            return;
        PlayerInfo player = playersInfo[userId];
        if(rooms.find(player.roomId_) == rooms.end())
            return;
        rooms[player.roomId_]->control(player.playerId_, action, enable);
    }

    void Manager::manage()
    {
        ev::reactor::EventLoop loop;
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
            server->blocksDataBroadcast(userIdsInRoom[roomId], std::move(blocksData));
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
                std::vector<std::pair<int, uint8_t>> players;
                for(int userId: userIdsInRoom[info.roomId_])
                {
                    uint8_t playerId = playersInfo[userId].playerId_;
                    players.emplace_back(userId, playerId);
                }
                server->notifyGameOn(players);
                room->init();
                ServerBlockDataList blocksData = room->getBlocksData();
                server->blocksDataBroadcast(userIdsInRoom[info.roomId_], std::move(blocksData));
            }
            else if(info.roomStatus_ == GameRoom::Playing)
            {
                if(info.playerNum_ < info.roomCap_)
                {
                    room->setStatus(GameRoom::Waiting);
                    server->notifyGameOff(userIdsInRoom[info.roomId_]);
                    continue;
                }
                room->moveAll();
                ServerObjectsData objectsData = room->getObjectsData();
                server->objectsDataBroadcast(userIdsInRoom[info.roomId_], std::move(objectsData));
                if(room->needRestart())
                {
                    restartRoom(room->info().roomId_);
                    auto scores = room->getPlayersScore();
                    server->playersScoreBroadcast(userIdsInRoom[info.roomId_], std::move(scores));
                }
            }
        }
    }
}