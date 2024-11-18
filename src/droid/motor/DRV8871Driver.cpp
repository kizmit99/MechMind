#include "droid/motor/DRV8871Driver.h"
#include "droid/services/PWMService.h"

#define CONFIG_KEY_DRV8871_TIMEOUT          "Timeout"
#define CONFIG_KEY_DRV8871_DEADBAND         "Deadband"
#define CONFIG_KEY_DRV8871_RAMP             "RampPowerPerMs"
#define CONFIG_DEFAULT_DRV8871_TIMEOUT      100
#define CONFIG_DEFAULT_DRV8871_DEADBAND     3
#define CONFIG_DEFAULT_DRV8871_RAMP         1.0

namespace droid::motor {
    DRV8871Driver::DRV8871Driver(const char* name, droid::core::System* system, uint8_t out1, uint8_t out2) :
        MotorDriver(name, system),
        out1(out1),
        out2(out2) {

        requestedDutyCycle = 0;
    }

    void DRV8871Driver::init() {
        timeoutMs = config->getInt(name, CONFIG_KEY_DRV8871_TIMEOUT, CONFIG_DEFAULT_DRV8871_TIMEOUT);
        deadband = config->getInt(name, CONFIG_KEY_DRV8871_DEADBAND, CONFIG_DEFAULT_DRV8871_DEADBAND);
        rampPowerPerMs = config->getFloat(name, CONFIG_KEY_DRV8871_RAMP, CONFIG_DEFAULT_DRV8871_RAMP);

        requestedDutyCycle = 0;
        setMotorSpeed(0);
    }

    void DRV8871Driver::factoryReset() {
        config->clear(name);
        config->putInt(name, CONFIG_KEY_DRV8871_TIMEOUT, CONFIG_DEFAULT_DRV8871_TIMEOUT);
        config->putInt(name, CONFIG_KEY_DRV8871_DEADBAND, CONFIG_DEFAULT_DRV8871_DEADBAND);
        config->putFloat(name, CONFIG_KEY_DRV8871_RAMP, CONFIG_DEFAULT_DRV8871_RAMP);
    }

    void DRV8871Driver::logConfig() {
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_DRV8871_TIMEOUT, config->getString(name, CONFIG_KEY_DRV8871_TIMEOUT, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_DRV8871_DEADBAND, config->getString(name, CONFIG_KEY_DRV8871_DEADBAND, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_DRV8871_RAMP, config->getString(name, CONFIG_KEY_DRV8871_RAMP, "").c_str());
    }

    void DRV8871Driver::failsafe() {
        setMotorSpeed(0);
        requestedDutyCycle = 0;
    }

    bool DRV8871Driver::setMotorSpeed(uint8_t motor, int8_t speed) {
        if (motor != 0) {return false;}
        if (abs(speed) <= deadband) {
            speed = 0;
        }

        lastCommandMs = millis();
        if (speed == 0) {
            requestedDutyCycle = 0;
        } else {
            requestedDutyCycle = map(speed, -128, 127, -100, 100);
        }
        task();
        return true;
    }

    //TODO not actually supported on this driver!
    bool DRV8871Driver::arcadeDrive(int8_t joystickX, int8_t joystickY) {
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
    
    void DRV8871Driver::stop() {
        setMotorSpeed(0, 0);
    }

    void DRV8871Driver::task() {
        ulong now = millis();
        if ((requestedDutyCycle != 0) && (now > (lastCommandMs + timeoutMs))) {
            logger->log(name, WARN, "task - timeout happened now=%d, lastCmd=%d, timeout=%d\n", now, lastCommandMs, timeoutMs);
            requestedDutyCycle = 0;
            lastCommandMs = now;
        }
        if (lastUpdateMs > now) {
            lastUpdateMs = now;
        }
        if (requestedDutyCycle != currentDutyCycle) {
            logger->log(name, DEBUG, "task - requestedDutyCycle=%d, currentDutyCycle=%d\n", requestedDutyCycle, currentDutyCycle);
            int16_t delta = abs(currentDutyCycle - requestedDutyCycle);
            int16_t maxDelta = (int16_t) ((now - lastUpdateMs) * rampPowerPerMs);
            if (delta > maxDelta) {
                delta = maxDelta;
            }
            logger->log(name, DEBUG, "task - delta=%d, maxDelta=%d\n", delta, maxDelta);
            if (delta > 0) {
                if (currentDutyCycle > requestedDutyCycle) {
                    currentDutyCycle = max(-100, currentDutyCycle - delta);
                } else {
                    currentDutyCycle = min(100, currentDutyCycle + delta);
                }
                setMotorSpeed(currentDutyCycle);
            }
        }
        lastUpdateMs = now;
    }

    void DRV8871Driver::setMotorSpeed(int16_t dutyCycle) {
        logger->log(name, DEBUG, "setMotorSpeed %d\n", dutyCycle);
        droid::services::PWMService* pwmService = system->getPWMService();
        if (pwmService) {
            if (dutyCycle == 0) {
                //Braking
                pwmService->setPWMpercent(out1, 100);
                pwmService->setPWMpercent(out2, 100);
            } else if (dutyCycle > 0) {
                pwmService->setPWMpercent(out2, 0);
                pwmService->setPWMpercent(out1, abs(dutyCycle), timeoutMs);
            } else {
                pwmService->setPWMpercent(out1, 0);
                pwmService->setPWMpercent(out2, abs(dutyCycle), timeoutMs);
            }
        }
    }
}