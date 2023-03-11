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
        void control(const std::string& connId, int action, bool enable);

    private:
        void manage();
        void manageCreateRoom(const std::string& name, int cap);
        void manageJoinRoom(const std::string& connId, uint8_t roomId);
        void manageQuitRoom(const std::string& connId);
        void manageControl(const std::string& connId, int action, bool enable);
        void manageGames();
        void updateRoomsInfo();
        void restartRoom(int roomId);

        Server* server;
        muduo::net::EventLoop* managerLoop;
        std::thread managerThread;
        std::mutex mu;
        std::condition_variable cond;
        bool started;

        typedef std::unique_ptr<GameRoom> GameRoomPtr;
        typedef std::unordered_map<uint8_t, GameRoomPtr> GameRoomList;
        GameRoomList rooms;
        int nextRoomId;
        std::unordered_map<std::string, PlayerInfo> playersInfo;
        std::unordered_map<uint8_t, std::unordered_set<std::string>> connIdsInRoom;
    };
}

#endif //TANK_TROUBLE_SERVER_MANAGER_H
