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

服务器使用8080(tcp) 和8081(udp)端口，可以在main.cc里修改(udp端口使用tcp端口 + 1)。

Server.cc中的数据库相关变量值要修改成自己的，如果数据库不存在的话启动会报错的。tinyorm会根据model里的类自动建表，不需要手动建表。

编译过后可执行文件目录下会有一个rules.json文件，这个是tinyorm的一些规则，编译时自动复制到可执行文件目录下的，运行时会读取rules.json，所以要保证它和可执行文件在同一目录。