//
// Created by zr on 23-2-18.
//
#include "Object.h"

namespace TankTrouble
{
    Object::Object(const util::Vec& pos, double angle, int id):
            posInfo(pos, angle),
            movingStatus(MOVING_STATIONARY),
            _id(id){}

    void Object::resetNextPosition(const PosInfo& next) {nextPos = next;}

    Object::PosInfo Object::getCurrentPosition() {return posInfo;}

    int Object::id() const {return _id;}

    int Object::getMovingStatus() {return movingStatus;}
}