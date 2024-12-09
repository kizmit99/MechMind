/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#include "droid/motor/DRV8871Driver.h"
#include "droid/services/PWMService.h"

#define CONFIG_KEY_DRV8871_TIMEOUT          "Timeout"
#define CONFIG_KEY_DRV8871_DEADBAND         "Deadband"
#define CONFIG_KEY_DRV8871_RAMP             "RampPowerPerMs"
#define CONFIG_DEFAULT_DRV8871_TIMEOUT      100
#define CONFIG_DEFAULT_DRV8871_DEADBAND     3
#define CONFIG_DEFAULT_DRV8871_RAMP         1.0

namespace droid::motor {
    DRV8871Driver::DRV8871Driver(const char* name, droid::core::System* system, int8_t M0_out1, int8_t M0_out2, int8_t M1_out1, int8_t M1_out2) :
        MotorDriver(name, system) {

        motorDetails[0].out1 = M0_out1;
        motorDetails[0].out2 = M0_out2;
        motorDetails[0].requestedDutyCycle = 0;

        motorDetails[1].out1 = M1_out1;
        motorDetails[1].out2 = M1_out2;
        motorDetails[1].requestedDutyCycle = 0;
    }

    void DRV8871Driver::init() {
        motorDetails[0].timeoutMs = config->getInt(name, CONFIG_KEY_DRV8871_TIMEOUT, CONFIG_DEFAULT_DRV8871_TIMEOUT);
        motorDetails[0].deadband = config->getInt(name, CONFIG_KEY_DRV8871_DEADBAND, CONFIG_DEFAULT_DRV8871_DEADBAND);
        motorDetails[0].rampPowerPerMs = config->getFloat(name, CONFIG_KEY_DRV8871_RAMP, CONFIG_DEFAULT_DRV8871_RAMP);
        motorDetails[0].requestedDutyCycle = 0;
        setDutyCycle(0, 0);

        motorDetails[1].timeoutMs = motorDetails[0].timeoutMs;
        motorDetails[1].deadband = motorDetails[0].deadband;
        motorDetails[1].rampPowerPerMs = motorDetails[0].rampPowerPerMs;
        motorDetails[1].requestedDutyCycle = 0;
        setDutyCycle(1, 0);
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
        setDutyCycle(0, 0);
        motorDetails[0].requestedDutyCycle = 0;

        setDutyCycle(1, 0);
        motorDetails[1].requestedDutyCycle = 0;
    }

    //Speed should be specified in the range -100 to +100
    bool DRV8871Driver::setMotorSpeed(uint8_t motor, int8_t speed) {
        if (motor > 1) {return false;}
        if (speed < -100) {speed = -100;}
        if (speed > 100) {speed = 100;}
        if (abs(speed) <= motorDetails[motor].deadband) {
            speed = 0;
        }

        motorDetails[motor].lastCommandMs = millis();
        motorDetails[motor].requestedDutyCycle = speed;
        task();
        return true;
    }

    //Joystick positions should be passed as normalized (-100 to +100)
    bool DRV8871Driver::arcadeDrive(int8_t joystickX, int8_t joystickY) {
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
    
    void DRV8871Driver::stop() {
        setMotorSpeed(0, 0);
        setMotorSpeed(1, 0);
    }

    void DRV8871Driver::task() {
        ulong now = millis();
        for (uint8_t motor = 0; motor < 2; motor++) {
            if ((motorDetails[motor].requestedDutyCycle != 0) && (now > (motorDetails[motor].lastCommandMs + motorDetails[motor].timeoutMs))) {
                logger->log(name, WARN, "task - timeout happened now=%d, lastCmd=%d, timeout=%d\n", now, motorDetails[motor].lastCommandMs, motorDetails[motor].timeoutMs);
                motorDetails[motor].requestedDutyCycle = 0;
                motorDetails[motor].lastCommandMs = now;
            }
            if (motorDetails[motor].lastUpdateMs > now) {
                motorDetails[motor].lastUpdateMs = now;
            }
            if (motorDetails[motor].requestedDutyCycle != motorDetails[motor].currentDutyCycle) {
                logger->log(name, DEBUG, "task - motor=%d, requestedDutyCycle=%d, currentDutyCycle=%d\n", motor, motorDetails[motor].requestedDutyCycle, motorDetails[motor].currentDutyCycle);
                int16_t delta = abs(motorDetails[motor].currentDutyCycle - motorDetails[motor].requestedDutyCycle);
                int16_t maxDelta = (int16_t) ((now - motorDetails[motor].lastUpdateMs) * motorDetails[motor].rampPowerPerMs);
                if (delta > maxDelta) {
                    delta = maxDelta;
                }
                logger->log(name, DEBUG, "task - delta=%d, maxDelta=%d\n", delta, maxDelta);
                if (delta > 0) {
                    if (motorDetails[motor].currentDutyCycle > motorDetails[motor].requestedDutyCycle) {
                        motorDetails[motor].currentDutyCycle = max(-100, motorDetails[motor].currentDutyCycle - delta);
                    } else {
                        motorDetails[motor].currentDutyCycle = min(100, motorDetails[motor].currentDutyCycle + delta);
                    }
                    setDutyCycle(motor, motorDetails[motor].currentDutyCycle);
                    motorDetails[motor].lastUpdateMs = now;
                }
            } else {
                if (now > (motorDetails[motor].lastUpdateMs + (motorDetails[motor].timeoutMs / 2))) {
                    setDutyCycle(motor, motorDetails[motor].currentDutyCycle);
                    motorDetails[motor].lastUpdateMs = now;
                }
            }
        }
    }

    void DRV8871Driver::setDutyCycle(uint8_t motor, int8_t dutyCycle) {
//        logger->log(name, DEBUG, "setDutyCycle motor: %d, dutyCycle: %d\n", motor, dutyCycle);
        if ((motorDetails[motor].out1 < 0) || (motorDetails[motor].out2 < 0)) {
            return;
        }
        droid::services::PWMService* pwmService = system->getPWMService();
        if (pwmService) {
            if (dutyCycle == 0) {
                pwmService->setPWMpercent(motorDetails[motor].out1, 0);
                pwmService->setPWMpercent(motorDetails[motor].out2, 0);
            } else if (dutyCycle > 0) {
                pwmService->setPWMpercent(motorDetails[motor].out1, abs(dutyCycle), motorDetails[motor].timeoutMs);
                pwmService->setPWMpercent(motorDetails[motor].out2, 0);
            } else {
                pwmService->setPWMpercent(motorDetails[motor].out1, 0);
                pwmService->setPWMpercent(motorDetails[motor].out2, abs(dutyCycle), motorDetails[motor].timeoutMs);
            }
        }
    }
}