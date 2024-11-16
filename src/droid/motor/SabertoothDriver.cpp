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

namespace droid::motor {
    SabertoothDriver::SabertoothDriver(const char* name, droid::services::System* sys, byte address, Stream& port) :
        name(name),
        logger(sys->getLogger()),
        config(sys->getConfig()),
        wrapped(address, port) {}

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
    

    //Note that the motor param for this method refers to motor index (0 or 1), not (1 or 2)!
    bool SabertoothDriver::setMotorSpeed(uint8_t motor, int8_t speed) {
        if (motor > 1) {return false;}
        if (speed > 127) {speed = 127;}

        wrapped.motor(motor + 1, speed);
        return true;
    }
    
    bool SabertoothDriver::arcadeDrive(int8_t joystickX, int8_t joystickY) {
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
    
    void SabertoothDriver::stop() {
        setMotorSpeed(0, 0);
        setMotorSpeed(1, 0);
    }
}