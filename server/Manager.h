//
// Created by zr on 23-3-8.
//

#ifndef TANK_TROUBLE_SERVER_MANAGER_H
#define TANK_TROUBLE_SERVER_MANAGER_H
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <vector>
#include "GameRoom.h"
#include "Data.h"

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
        void quitRoom(const std::string& connId);

    private:
        void manage();
        void manageCreateRoom(const std::string& name, int cap);
        void manageJoinRoom(const std::string& connId, uint8_t roomId);
        void manageQuitRoom(const std::string& connId);
        void manageGames();
        void updateRoomsInfo();

        Server* server_;
        muduo::net::EventLoop* managerLoop_;
        std::thread managerThread_;
        std::mutex mu_;
        std::condition_variable cond_;
        bool started_;

        typedef std::unique_ptr<GameRoom> GameRoomPtr;
        typedef std::unordered_map<uint8_t, GameRoomPtr> GameRoomList;
        GameRoomList rooms_;
        int nextRoomId_;
        std::unordered_map<std::string, PlayerInfo> playersInfo;
        std::unordered_map<uint8_t, std::unordered_set<std::string>> connIdsInRoom;
    };
}

#endif //TANK_TROUBLE_SERVER_MANAGER_H
