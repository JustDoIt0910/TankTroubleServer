//
// Created by zr on 23-2-18.
//

#ifndef TANK_TROUBLE_ID_H
#define TANK_TROUBLE_ID_H
#include <unordered_set>

#define VERTICAL_BORDER_ID      -1
#define HORIZON_BORDER_ID       -2
#define MIN_TANK_ID             1
#define MAX_TANK_ID             10

namespace TankTrouble::util
{
    class IdManager
    {
    public:
        IdManager();

        void reset();

        int getTankId();

        void returnTankId(int id);

        int getBlockId();

        int getShellId();

    private:
        int globalBlockId;
        int globalShellId;
        std::unordered_set<int> usedTankId;

        const static int initialBlockId;
        const static int initialShellId;
    };
}

#endif //TANK_TROUBLE_ID_H
