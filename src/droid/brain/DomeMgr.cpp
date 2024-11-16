#include <Arduino.h>
#include "droid/brain/DomeMgr.h"
#include "droid/services/System.h"
#include "droid/controller/Controller.h"
#include "droid/motor/MotorDriver.h"
#include "droid/brain/hardware.h"

namespace droid::brain {
    DomeMgr::DomeMgr(const char* name, droid::services::System* system, droid::controller::Controller* controller, droid::motor::MotorDriver* domeMotor) : 
        name(name),
        config(system->getConfig()),
        logger(system->getLogger()),
        droidState(system->getDroidState()),
        controller(controller),
        domeMotor(domeMotor) {
        }

    void DomeMgr::init() {
        //NOOP
    }

    void DomeMgr::task() {
        int8_t pos = controller->getJoystickPosition(droid::controller::Controller::Joystick::LEFT, droid::controller::Controller::Axis::X);
        controller->setCritical(abs(pos) > 0);
        domeMotor->setMotorSpeed(0, pos);
    }
}