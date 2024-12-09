/*
 * MechMind Program
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

    //Note that the motor param for this method refers to motor index (0 or 1), not (1 or 2)
    // And speed should be specified in the normalized range from -100 to +100
    bool CytronSmartDriveDuoDriver::setMotorSpeed(uint8_t motor, int8_t speed) {
        if (motor > 1) {return false;}
        if (speed < -100) {speed = -100;}
        if (speed > 100) {speed = 100;}
        if (abs(speed) <= deadband) {
            speed = 0;
        }
        lastCommandMs = millis();

        int8_t nativeSpeed = map(speed, -100, 100, -128, 127);
        if (speed == 0) {
            nativeSpeed = 0;
        }
        motorSpeed[motor] = nativeSpeed;
        
        wrapped.motor(motorSpeed[0], motorSpeed[1]);
        return true;
    }

    //Joystick positions specified as normalized values between -100 and +100
    bool CytronSmartDriveDuoDriver::arcadeDrive(int8_t joystickX, int8_t joystickY) {
        if (joystickX < -100) {joystickX = -100;}
        if (joystickX > 100) {joystickX = 100;}
        if (joystickY < -100) {joystickY = -100;}
        if (joystickY > 100) {joystickY = 100;}
        lastCommandMs = millis();
        // Scale joystick inputs to motor output ranges 
        int forward = joystickY; // Forward/backward control 
        int turn = joystickX; // Left/right control 
        
        // Calculate the motor speeds based on joystick input 
        int leftMotorSpeed = forward + turn; 
        int rightMotorSpeed = forward - turn; 
        
        // Ensure the motor speeds are within the valid range (-127 to 128) 
        leftMotorSpeed = std::max(-100, std::min(100, leftMotorSpeed)); 
        rightMotorSpeed = std::max(-100, std::min(100, rightMotorSpeed));

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
