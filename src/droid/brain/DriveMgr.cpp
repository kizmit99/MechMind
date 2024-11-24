/*
 * Droid Brain Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#include "droid/brain/DriveMgr.h"

#define CONFIG_KEY_DRIVEMGR_NORMALSPEED     "NormalSpeed"
#define CONFIG_KEY_DRIVEMGR_TURBOSPEED      "TurboSpeed"
#define CONFIG_KEY_DRIVEMGR_TURNSPEED       "TurnSpeed"
#define CONFIG_KEY_DRIVEMGR_DEADBAND        "Deadband"

#define CONFIG_DEFAULT_DRIVEMGR_NORMALSPEED  70
#define CONFIG_DEFAULT_DRIVEMGR_TURBOSPEED   100
#define CONFIG_DEFAULT_DRIVEMGR_TURNSPEED    50
#define CONFIG_DEFAULT_DRIVEMGR_DEADBAND     16


namespace {
    int8_t scale(int8_t in, int8_t min, int8_t max) {
        if (in == 0) {
            return 0;   //Maintain zero
        }
        return map(in, -127, 128, min, max);
    }
}

namespace droid::brain {
    DriveMgr::DriveMgr(const char* name, droid::core::System* system, droid::controller::Controller* controller, droid::motor::MotorDriver* driveMotor) : 
        BaseComponent(name, system),
        controller(controller),
        driveMotor(driveMotor) {}

    void DriveMgr::init() {
        normalSpeed = config->getInt(name, CONFIG_KEY_DRIVEMGR_NORMALSPEED, CONFIG_DEFAULT_DRIVEMGR_NORMALSPEED);
        turboSpeed = config->getInt(name, CONFIG_KEY_DRIVEMGR_TURBOSPEED, CONFIG_DEFAULT_DRIVEMGR_TURBOSPEED);
        turnSpeed = config->getInt(name, CONFIG_KEY_DRIVEMGR_TURNSPEED, CONFIG_DEFAULT_DRIVEMGR_TURNSPEED);
        deadband = config->getInt(name, CONFIG_KEY_DRIVEMGR_DEADBAND, CONFIG_DEFAULT_DRIVEMGR_DEADBAND);
    }

    void DriveMgr::factoryReset() {
        config->clear(name);
        config->putInt(name, CONFIG_KEY_DRIVEMGR_NORMALSPEED, CONFIG_DEFAULT_DRIVEMGR_NORMALSPEED);
        config->putInt(name, CONFIG_KEY_DRIVEMGR_TURBOSPEED, CONFIG_DEFAULT_DRIVEMGR_TURBOSPEED);
        config->putInt(name, CONFIG_KEY_DRIVEMGR_TURNSPEED, CONFIG_DEFAULT_DRIVEMGR_TURNSPEED);
        config->putInt(name, CONFIG_KEY_DRIVEMGR_DEADBAND, CONFIG_DEFAULT_DRIVEMGR_DEADBAND);
    }

    void DriveMgr::logConfig() {
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_DRIVEMGR_NORMALSPEED, config->getString(name, CONFIG_KEY_DRIVEMGR_NORMALSPEED, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_DRIVEMGR_TURBOSPEED, config->getString(name, CONFIG_KEY_DRIVEMGR_TURBOSPEED, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_DRIVEMGR_TURNSPEED, config->getString(name, CONFIG_KEY_DRIVEMGR_TURNSPEED, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_DRIVEMGR_DEADBAND, config->getString(name, CONFIG_KEY_DRIVEMGR_DEADBAND, "").c_str());
    }

    void DriveMgr::task() {
        int8_t joyX = controller->getJoystickPosition(droid::controller::Controller::Joystick::RIGHT, droid::controller::Controller::Axis::X);
        int8_t joyY = controller->getJoystickPosition(droid::controller::Controller::Joystick::RIGHT, droid::controller::Controller::Axis::Y);
        if (!droidState->stickEnable) {
            joyX = 0;
            joyY = 0;
        }
        if (abs(joyX) <= deadband) {
            joyX = 0;
        }
        if (abs(joyY) <= deadband) {
            joyY = 0;
        }
        if (droidState->turboSpeed) {
            joyX = scale(joyX, -turboSpeed, turboSpeed);
        } else {
            joyX = scale(joyX, -normalSpeed, normalSpeed);
        }
        joyY = scale(joyY, -turnSpeed, turnSpeed);
        controller->setCritical((abs(joyX) > 0) || (abs(joyY) > 0));
        driveMotor->arcadeDrive(joyX, joyY);
    }

    void DriveMgr::failsafe() {
        //NOOP - other BaseComponents will be notified directly
    }
}
