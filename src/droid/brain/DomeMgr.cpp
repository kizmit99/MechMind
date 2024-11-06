#include <Arduino.h>
#include "droid/brain/DomeMgr.h"
#include "droid/services/System.h"
#include "droid/controller/Controller.h"
#include "droid/motor/MotorDriver.h"

namespace droid::brain {
    DomeMgr::DomeMgr(const char* name, droid::services::System* system, droid::controller::Controller* controller, droid::motor::MotorDriver* domeMotor) : 
        name(name),
        config(system->getConfig()),
        logger(system->getLogger()),
        controller(controller),
        domeMotor(domeMotor) {
        }

    void DomeMgr::init() {
        domeMotor->setPowerRamp(10.0);
    }

    void DomeMgr::task() {
        int8_t pos = controller->getJoystickPosition(droid::controller::Controller::Joystick::LEFT, droid::controller::Controller::Axis::X);
        // logger->log(name, DEBUG, "X: %d    Y: %d\n", pos, controller.getJoystickPosition(droid::controller::Controller::Joystick::RIGHT, droid::controller::Controller::Axis::Y));
        controller->setCritical(abs(pos) > 0);
        domeMotor->drive(pos);
    }
}