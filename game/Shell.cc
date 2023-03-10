//
// Created by zr on 23-2-16.
//

#include "Shell.h"
#include "util/Math.h"
#include "util/IdManager.h"

namespace TankTrouble
{
    const double Shell::RADIUS = 2.5;
    Shell::Shell(int id, const util::Vec& p, double angle, int tankId):
        Object(p, angle, id),
        _tankId(tankId),
        _ttl(INITIAL_TTL)
    {movingStatus = MOVING_FORWARD;}

    Object::PosInfo Shell::getNextPosition(int movingStep, int rotationStep)
    {
        Object::PosInfo next = getNextPosition(posInfo, movingStep, rotationStep);
        nextPos = next;
        return next;
    }

    Object::PosInfo Shell::getNextPosition(const Object::PosInfo& cur, int movingStep, int rotationStep)
    {
        if(movingStep == 0)
            movingStep = SHELL_MOVING_STEP;
        Object::PosInfo next = cur;
        next.pos = util::polar2Cart(cur.angle, movingStep, cur.pos);
        return next;
    }

    void Shell::moveToNextPosition() {posInfo = nextPos;}

    ObjType Shell::type() {return OBJ_SHELL;}

    int Shell::countDown() {return _ttl--;}

    int Shell::tankId() const {return _tankId;}

    int Shell::ttl() const {return _ttl;}
}