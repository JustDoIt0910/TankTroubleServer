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

    void Manager::start()
    {
        assert(started_ == false);
        managerThread_ = std::thread([this] () {manage();});
        {
            std::unique_lock<std::mutex> lk(mu_);
            cond_.wait(lk, [this](){return started_;});
        }
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
        loop.loop();
        {
            std::unique_lock<std::mutex> lk(mu_);
            managerLoop_ = nullptr;
        }
    }

    void Manager::createRoom(const std::string& name, int cap)
    {managerLoop_->queueInLoop([this, name, cap] () { managerCreateRoom(name, cap);});}

    void Manager::managerCreateRoom(const std::string &name, int cap)
    {
        rooms_.push_back(std::make_unique<GameRoom>(nextRoomId_++, name, cap));
        RoomInfoList infos;
        for(const GameRoomPtr& room: rooms_)
            infos.push_back(room->info());
        server_->updateRoomInfos(infos);
    }
}