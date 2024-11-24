/*
 * Droid Brain Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#include "droid/brain/DomeMgr.h"

#define CONFIG_KEY_DOMEMGR_SPEED        "Speed"
#define CONFIG_KEY_DOMEMGR_DEADBAND     "Deadband"

#define CONFIG_DEFAULT_DOMEMGR_SPEED    127
#define CONFIG_DEFAULT_DOMEMGR_DEADBAND 16

namespace {
    int8_t scale(int8_t in, int8_t min, int8_t max) {
        if (in == 0) {
            return 0;   //Maintain zero
        }
        return map(in, -127, 128, min, max);
    }
}

namespace droid::brain {
    DomeMgr::DomeMgr(const char* name, droid::core::System* system, droid::controller::Controller* controller, droid::motor::MotorDriver* domeMotor) : 
        BaseComponent(name, system),
        controller(controller),
        domeMotor(domeMotor) {}

    void DomeMgr::init() {
        speed = config->getInt(name, CONFIG_KEY_DOMEMGR_SPEED, CONFIG_DEFAULT_DOMEMGR_SPEED);
        deadband = config->getInt(name, CONFIG_KEY_DOMEMGR_DEADBAND, CONFIG_DEFAULT_DOMEMGR_DEADBAND);
    }

    void DomeMgr::factoryReset() {
        config->clear(name);
        config->putInt(name, CONFIG_KEY_DOMEMGR_SPEED, CONFIG_DEFAULT_DOMEMGR_SPEED);
        config->putInt(name, CONFIG_KEY_DOMEMGR_DEADBAND, CONFIG_DEFAULT_DOMEMGR_DEADBAND);
    }

    void DomeMgr::logConfig() {
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_DOMEMGR_SPEED, config->getString(name, CONFIG_KEY_DOMEMGR_SPEED, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_DOMEMGR_DEADBAND, config->getString(name, CONFIG_KEY_DOMEMGR_DEADBAND, "").c_str());
    }

    void DomeMgr::task() {
        int8_t joyX = controller->getJoystickPosition(droid::controller::Controller::Joystick::LEFT, droid::controller::Controller::Axis::X);
        if (abs(joyX) <= deadband) {
            joyX = 0;
        }
        joyX = scale(joyX, -speed, speed);
        controller->setCritical(abs(joyX) > 0);
        domeMotor->setMotorSpeed(0, joyX);
    }

    void DomeMgr::failsafe() {
        //NOOP - other BaseComponents will be notified directly
    }
}
