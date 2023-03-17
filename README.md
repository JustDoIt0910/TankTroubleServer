# TankTroubleServer

这个是 [TankTrouble](https://github.com/JustDoIt0910/TankTrouble) 在线模式下的服务器。依赖另外两个个人仓库 ev(网络库) 和 tinyorm(一个简单的ORM框架)。ev，tinyorm，和Server本身都只是学习之作，水平有限，肯定无法达到生产环境要求，所以我并没有让Server 一直跑在云服务器上。如果想要测试在线模式的话，可以编译以后扔到云服务器上试一下。

```bash
git clone https://github.com/JustDoIt0910/TankTroubleServer.git
cd TankTroubleServer
git submodule update --init --recursive
mkdir build && cd build
cmake ..
make
```

服务器使用8080(tcp) 和8081(udp)端口，可以在main.cc里修改(udp端口使用tcp端口 + 1)。测试的话要打开防火墙里的对应端口，PC端 TankTrouble中 Window.cc 里的服务器地址也要修改成自己云服务器的地址。

Server.cc中的数据库相关变量值要修改成自己的，如果数据库不存在的话启动会报错的。tinyorm会根据model里的类自动建表，不需要手动建表。

编译过后可执行文件目录下会有一个rules.json文件，这个是tinyorm的一些规则，编译时自动复制到可执行文件目录下的，运行时会读取rules.json，所以要保证它和可执行文件在同一目录。



### 项目结构

- game ------------------- 游戏对象的定义等等，和 TankTrouble 单机版差不多，在 TankTrouble 的文档里有写。
- model ------------------ 数据库的表结构，现在只需要保存一下用户的昵称，分数之类的，就一个表。
- protocol --------------- 通信协议部分，包括消息设计和编解码器，和 TankTrouble 中完全一样，在TankTrouble文档里有说明
- server ------------------ 主要部分
  - Server.h Server.cc ---------------------------- 服务器，负责消息收发，与manager交互
  - Manager.h Manager.cc --------------------- manager 管理所有游戏房间
  - GameRoom.h GameRoom.cc ------------ 控制一个游戏房间里的游戏进行
  - Data.h -------------------------------------------- 一些数据结构定义

![structure](https://github.com/JustDoIt0910/MarkDownPictures/blob/main/TankTrouble/p8.png)

图中是线程模型和各部分责任划分，Server运行在io线程中，只负责io，所有对数据层面的操作移交给Manager执行，对数据库的操作移交给线程池。Manager运行在 Manager线程，也就是游戏线程，它管理房间的创建，用户加入与退出，驱动每个GameRoom运行。Server和Manager的数据相互隔离，它们间任务与数据的传递依靠reactor的任务队列。

