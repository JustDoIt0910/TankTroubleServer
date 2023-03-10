//
// Created by zr on 23-2-18.
//

#include "IdManager.h"
#include "game/defs.h"
#include <cassert>

namespace TankTrouble::util
{
    const int IdManager::initialBlockId = 11;
    const int IdManager::initialShellId = MAX_BLOCKS_NUM + 11;

    IdManager::IdManager() {reset();}

    int IdManager::getTankId()
    {
        for(int i = MIN_TANK_ID; i <= MAX_TANK_ID; i++)
            if(usedTankId.find(i) == usedTankId.end())
            {
                usedTankId.insert(i);
                return i;
            }
        return 0;
    }

    void IdManager::returnTankId(int id)
    {
        assert(id >= MIN_TANK_ID && id <= MAX_TANK_ID);
        usedTankId.erase(id);
    }

    int IdManager::getBlockId() {return globalBlockId++;}

    int IdManager::getShellId() {return globalShellId++;}

    void IdManager::reset()
    {
        globalBlockId = initialBlockId;
        globalShellId = initialShellId;
    }
}