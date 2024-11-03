#include "droid/motor/DRV8871Driver.h"

namespace droid::motor {
    DRV8871Driver::DRV8871Driver(const char* name, droid::services::System* sys, uint8_t pwm1, uint8_t pwm2) :
        name(name),
        pwmPin1(pwm1),
        pwmPin2(pwm2) {

        logger = sys->getLogger();
        pinMode(pwmPin1, OUTPUT);
        pinMode(pwmPin2, OUTPUT);
        stop();
    }

    void DRV8871Driver::factoryReset() {
        //TODO No config values yet
    }

    void DRV8871Driver::drive(int8_t speed) {
        logger->log(name, DEBUG, "drive - requested speed = %d\n", speed);
        lastCommandMs = millis();
        if (abs(speed) <= deadband) {
            speed = 0;
            requestedPWM = 0;
        } else {
            requestedPWM = (int16_t) (((float) speed) * 2.0);
        }
        logger->log(name, DEBUG, "drive - requestedPWM = %d\n", requestedPWM);
        task();
    }
    
    void DRV8871Driver::turn(int8_t speed) {
        //Single motor driver
        stop();
    }
    
    void DRV8871Driver::stop() {
        drive(0);
    }

    void DRV8871Driver::setTimeout(uint16_t milliSeconds) {
        timeoutMs = milliSeconds;
    }

    void DRV8871Driver::setDeadband(uint8_t deadband) {
        this->deadband = min((int) deadband, 127);
    }

    void DRV8871Driver::setPowerRamp(float_t rampPowerPerMs) {
        this->rampPowerPerMs = abs(rampPowerPerMs);
    }

    void DRV8871Driver::task() {
        ulong now = millis();
        if ((requestedPWM != 0) && (now > (lastCommandMs + timeoutMs))) {
            logger->log(name, WARN, "task - timeout happened now=%d, lastCmd=%d, timeout=%d\n", now, lastCommandMs, timeoutMs);
            requestedPWM = 0;
            lastCommandMs = now;
        }
        if (lastUpdateMs > now) {
            lastUpdateMs = now;
        }
        if (requestedPWM != currentPWM) {
            logger->log(name, DEBUG, "task - requestPWM=%d, currentPWM=%d\n", requestedPWM, currentPWM);
            int16_t delta = abs(currentPWM - requestedPWM);
            int16_t maxDelta = (int16_t) ((now - lastUpdateMs) * rampPowerPerMs);
            if (delta > maxDelta) {
                delta = maxDelta;
            }
            logger->log(name, DEBUG, "task - delta=%d\n", delta);
            if (delta > 0) {
                if (currentPWM > requestedPWM) {
                    currentPWM = max(-255, currentPWM - delta);
                } else {
                    currentPWM = min(255, currentPWM + delta);
                }
                setMotorSpeed(currentPWM);
            }
        }
        lastUpdateMs = now;
    }

    void DRV8871Driver::setMotorSpeed(int16_t speed) {
        logger->log(name, DEBUG, "setMotorSpeed %d\n", speed);
        if (speed == 0) {
            analogWrite(pwmPin1, 0);
            analogWrite(pwmPin2, 0);
        } else if (speed > 0) {
            analogWrite(pwmPin2, 0);
            analogWrite(pwmPin1, (abs(speed)));
        } else {
            analogWrite(pwmPin1, 0);
            analogWrite(pwmPin2, (abs(speed)));
        }
    }
}