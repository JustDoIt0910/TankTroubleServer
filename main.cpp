#include "server/Server.h"

using namespace TankTrouble;

int main() {

    TankTrouble::Server server(8080);
    server.serve();
}
