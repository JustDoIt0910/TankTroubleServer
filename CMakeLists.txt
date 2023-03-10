cmake_minimum_required(VERSION 3.8)
project(TankTroubleServer)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_STANDARD 17)

set(TankTroubleServer_SRC main.cpp
        server/Server.cc
        server/Manager.cc
        server/GameRoom.cc
        protocol/Codec.cc
        game/util/IdManager.cc
        game/util/Math.cc
        game/util/Vec.cc
        game/Object.cc
        game/Tank.cc
        game/Shell.cc
        game/Block.cc
        game/Maze.cc)


set(SPDLOGD ${CMAKE_SOURCE_DIR}/third_party/tinyorm/mysql4cpp/spdlog/lib)
set(MUDUO ${CMAKE_SOURCE_DIR}/third_party/muduo/lib)

set(MUDUO_LIB muduo_net muduo_base pthread)
set(TINYORM_LIB spdlog mysql4cpp mysqlclient nlohmann_json::nlohmann_json)

add_subdirectory(third_party/tinyorm)

link_directories(${SPDLOGD} ${MUDUO})

add_executable(TankTroubleServer ${TankTroubleServer_SRC})

target_link_libraries(TankTroubleServer ${MUDUO_LIB} ${TINYORM_LIB})

target_include_directories(TankTroubleServer PUBLIC
     ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/third_party ${CMAKE_SOURCE_DIR}/third_party/muduo/include
    )