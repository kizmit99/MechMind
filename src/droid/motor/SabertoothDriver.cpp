/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#include "droid/motor/SabertoothDriver.h"

//Kill the motors is an updated command not received within this many milliSeconds
#define CONFIG_KEY_SABERTOOTH_TIMEOUT          "Timeout"

//abs(Motor speed) < deadband equivalent to speed=0
#define CONFIG_KEY_SABERTOOTH_DEADBAND         "Deadband"

// See the Sabertooth 2x60 documentation for information on ramping values.
// There are three ranges: 1-10 (Fast), 11-20 (Slow), and 21-80 (Intermediate).
// The ramping value 14 used here sets a ramp time of 4 seconds for full
// forward-to-full reverse.
//
// 0 turns off ramping. Turning off ramping requires a power cycle.
#define CONFIG_KEY_SABERTOOTH_RAMP             "Ramping"

//MinVoltage calculation varies between specific controllers
//Sabertooth 2x25 MinVoltage = (Desired Min - 6)/5
#define CONFIG_KEY_SABERTOOTH_MIN_VOLTAGE      "MinVoltage"

//MaxVoltage calculation varies between specific controllers
//Sabertooth2x25 MaxVoltage = Desired Max * 5.12
#define CONFIG_KEY_SABERTOOTH_MAX_VOLTAGE      "MaxVoltage"

#define CONFIG_DEFAULT_SABERTOOTH_TIMEOUT      100
#define CONFIG_DEFAULT_SABERTOOTH_DEADBAND     3
#define CONFIG_DEFAULT_SABERTOOTH_RAMP         14
#define CONFIG_DEFAULT_SABERTOOTH_MIN_VOLTAGE  3
#define CONFIG_DEFAULT_SABERTOOTH_MAX_VOLTAGE  144

#define SABERTOOTH_UPDATE_THROTTLE 20

namespace droid::motor {
    SabertoothDriver::SabertoothDriver(const char* name, droid::core::System* system, byte address, Stream* port) :
        MotorDriver(name, system),
        port(port),
        wrapped(address, *port) {}

    void SabertoothDriver::init() {
        uint32_t timeoutMs = config->getInt(name, CONFIG_KEY_SABERTOOTH_TIMEOUT, CONFIG_DEFAULT_SABERTOOTH_TIMEOUT);
        uint8_t deadband = config->getInt(name, CONFIG_KEY_SABERTOOTH_DEADBAND, CONFIG_DEFAULT_SABERTOOTH_DEADBAND);
        uint8_t ramping = config->getInt(name, CONFIG_KEY_SABERTOOTH_RAMP, CONFIG_DEFAULT_SABERTOOTH_RAMP);
        uint8_t minVoltage = config->getInt(name, CONFIG_KEY_SABERTOOTH_MIN_VOLTAGE, CONFIG_DEFAULT_SABERTOOTH_MIN_VOLTAGE);
        uint8_t maxVoltage = config->getInt(name, CONFIG_KEY_SABERTOOTH_MAX_VOLTAGE, CONFIG_DEFAULT_SABERTOOTH_MAX_VOLTAGE);

        stop();
        wrapped.setTimeout(timeoutMs);
        wrapped.setDeadband(deadband);
        wrapped.setRamping(ramping);
        wrapped.setMinVoltage(minVoltage);
        wrapped.setMaxVoltage(maxVoltage);
    }

    void SabertoothDriver::factoryReset() {
        config->clear(name);
        config->putInt(name, CONFIG_KEY_SABERTOOTH_TIMEOUT, CONFIG_DEFAULT_SABERTOOTH_TIMEOUT);
        config->putInt(name, CONFIG_KEY_SABERTOOTH_DEADBAND, CONFIG_DEFAULT_SABERTOOTH_DEADBAND);
        config->putInt(name, CONFIG_KEY_SABERTOOTH_RAMP, CONFIG_DEFAULT_SABERTOOTH_RAMP);
        config->putInt(name, CONFIG_KEY_SABERTOOTH_MIN_VOLTAGE, CONFIG_DEFAULT_SABERTOOTH_MIN_VOLTAGE);
        config->putInt(name, CONFIG_KEY_SABERTOOTH_MAX_VOLTAGE, CONFIG_DEFAULT_SABERTOOTH_MAX_VOLTAGE);
    }
    
    void SabertoothDriver::task() {
        //NOOP
    }
    
    void SabertoothDriver::logConfig() {
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_SABERTOOTH_TIMEOUT, config->getString(name, CONFIG_KEY_SABERTOOTH_TIMEOUT, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_SABERTOOTH_DEADBAND, config->getString(name, CONFIG_KEY_SABERTOOTH_DEADBAND, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_SABERTOOTH_RAMP, config->getString(name, CONFIG_KEY_SABERTOOTH_RAMP, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_SABERTOOTH_MIN_VOLTAGE, config->getString(name, CONFIG_KEY_SABERTOOTH_MIN_VOLTAGE, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_SABERTOOTH_MAX_VOLTAGE, config->getString(name, CONFIG_KEY_SABERTOOTH_MAX_VOLTAGE, "").c_str());
    }
    
    void SabertoothDriver::failsafe() {
        stop();
    }
    

    //Note that the motor param for this method refers to motor index (0 or 1), not (1 or 2)
    // And speed should be specified in the normalized range from -100 to +100
    bool SabertoothDriver::setMotorSpeed(uint8_t motor, int8_t speed) {
        if (motor > 1) {return false;}
        if (speed < -100) {speed = -100;}
        if (speed > 100) {speed = 100;}

        //Throttle motor updates when resending the same speed as last time
        if ((speed != lastMotorSpeed[motor]) ||
            (millis() > (lastMotorUpdate[motor] + SABERTOOTH_UPDATE_THROTTLE))) {
            lastMotorUpdate[motor] = millis();
            int8_t nativeSpeed = map(speed, -100, 100, -128, 127);
            if (speed == 0) {
                nativeSpeed = 0;
            }
            wrapped.motor(motor + 1, nativeSpeed);
        }
        if (speed != lastMotorSpeed[motor]) {
            logger->log(name, DEBUG, "setMotorSpeed(%d, %d)\n", motor, speed);
        }
        lastMotorSpeed[motor] = speed;
        return true;
    }
    
    //Joystick positions specified as normalized values between -100 and +100
    bool SabertoothDriver::arcadeDrive(int8_t joystickX, int8_t joystickY) {
        if (joystickX < -100) {joystickX = -100;}
        if (joystickX > 100) {joystickX = 100;}
        if (joystickY < -100) {joystickY = -100;}
        if (joystickY > 100) {joystickY = 100;}
        // Scale joystick inputs to motor output ranges 
        int forward = joystickY; // Forward/backward control 
        int turn = joystickX; // Left/right control 
        
        // Calculate the motor speeds based on joystick input 
        int leftMotorSpeed = forward + turn; 
        int rightMotorSpeed = forward - turn; 
        
        // Ensure the motor speeds are within the valid range (-100 to 100) 
        leftMotorSpeed = std::max(-100, std::min(100, leftMotorSpeed)); 
        rightMotorSpeed = std::max(-100, std::min(100, rightMotorSpeed));

        bool supported = true;
        supported &= setMotorSpeed(0, leftMotorSpeed);
        supported &= setMotorSpeed(1, rightMotorSpeed);
        return supported;
    }
    
    void SabertoothDriver::stop() {
        setMotorSpeed(0, 0);
        setMotorSpeed(1, 0);
    }
}