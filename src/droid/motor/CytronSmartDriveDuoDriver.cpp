/*
 * Droid Brain Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#include "droid/motor/CytronSmartDriveDuoDriver.h"

#define CONFIG_KEY_CYTRON_TIMEOUT          "Timeout"
#define CONFIG_KEY_CYTRON_DEADBAND         "Deadband"
#define CONFIG_DEFAULT_CYTRON_TIMEOUT      100
#define CONFIG_DEFAULT_CYTRON_DEADBAND     3

namespace droid::motor {

    CytronSmartDriveDuoDriver::CytronSmartDriveDuoDriver(const char* name, droid::core::System* system, byte address, Stream& port, uint8_t initialByte) : 
        MotorDriver(name, system),
        wrapped(address, port, initialByte) {}

    void CytronSmartDriveDuoDriver::init() {
        timeoutMs = config->getInt(name, CONFIG_KEY_CYTRON_TIMEOUT, CONFIG_DEFAULT_CYTRON_TIMEOUT);
        deadband = config->getInt(name, CONFIG_KEY_CYTRON_DEADBAND, CONFIG_DEFAULT_CYTRON_DEADBAND);
        stop();
    }

    void CytronSmartDriveDuoDriver::factoryReset() {
        config->clear(name);
        config->putInt(name, CONFIG_KEY_CYTRON_TIMEOUT, CONFIG_DEFAULT_CYTRON_TIMEOUT);
        config->putInt(name, CONFIG_KEY_CYTRON_DEADBAND, CONFIG_DEFAULT_CYTRON_DEADBAND);
    }

    void CytronSmartDriveDuoDriver::task() {
        ulong now = millis();
        if (((motorSpeed[0] != 0) || (motorSpeed[1] != 0)) && 
            (now > (lastCommandMs + timeoutMs))) {
            logger->log(name, WARN, "task - timeout happened now=%d, lastCmd=%d, timeout=%d\n", now, lastCommandMs, timeoutMs);
            stop();
        }
    }

    void CytronSmartDriveDuoDriver::logConfig() {
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_CYTRON_TIMEOUT, config->getString(name, CONFIG_KEY_CYTRON_TIMEOUT, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_CYTRON_DEADBAND, config->getString(name, CONFIG_KEY_CYTRON_DEADBAND, "").c_str());
    }

    void CytronSmartDriveDuoDriver::failsafe() {
        stop();
    }

    bool CytronSmartDriveDuoDriver::setMotorSpeed(uint8_t motor, int8_t speed) {
        if (motor > 1) {return false;}
        if (abs(speed) <= deadband) {
            speed = 0;
        }
        lastCommandMs = millis();
        motorSpeed[motor] = speed;
        wrapped.motor(motorSpeed[0], motorSpeed[1]);
        return true;
    }

    bool CytronSmartDriveDuoDriver::arcadeDrive(int8_t joystickX, int8_t joystickY) {
        lastCommandMs = millis();
        // Scale joystick inputs to motor output ranges 
        int forward = joystickX; // Forward/backward control 
        int turn = joystickY; // Left/right control 
        
        // Calculate the motor speeds based on joystick input 
        int leftMotorSpeed = forward + turn; 
        int rightMotorSpeed = forward - turn; 
        
        // Ensure the motor speeds are within the valid range (-127 to 128) 
        leftMotorSpeed = std::max(-127, std::min(128, leftMotorSpeed)); 
        rightMotorSpeed = std::max(-127, std::min(128, rightMotorSpeed));

        bool supported = true;
        supported &= setMotorSpeed(0, leftMotorSpeed);
        supported &= setMotorSpeed(1, rightMotorSpeed);
        return supported;
    }

    void CytronSmartDriveDuoDriver::stop() {
        wrapped.stop();
        motorSpeed[0] = 0;
        motorSpeed[1] = 0;
        lastCommandMs = millis();
    }
}
