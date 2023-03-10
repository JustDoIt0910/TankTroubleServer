//
// Created by zr on 23-3-8.
//

#ifndef TANK_TROUBLE_SERVER_MANAGER_H
#define TANK_TROUBLE_SERVER_MANAGER_H
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include "GameRoom.h"

namespace muduo::net {class EventLoop;}

namespace TankTrouble
{
    class Server;

    class Manager
    {
    public:
        typedef std::vector<GameRoom::RoomInfo> RoomInfoList;

        explicit Manager(Server* server);
        ~Manager();
        void start();
        void createRoom(const std::string& name, int cap);
        void joinRoom(const std::string& connId, uint8_t roomId);

    private:
        void manage();
        void managerCreateRoom(const std::string& name, int cap);
        void managerJoinRoom(const std::string& connId, uint8_t roomId);

        Server* server_;
        muduo::net::EventLoop* managerLoop_;
        std::thread managerThread_;
        std::mutex mu_;
        std::condition_variable cond_;
        bool started_;

        typedef std::unique_ptr<GameRoom> GameRoomPtr;
        typedef std::vector<GameRoomPtr> GameRoomList;
        GameRoomList rooms_;
        int nextRoomId_;
    };
}

#endif //TANK_TROUBLE_SERVER_MANAGER_H
