#include "droid/motor/DRV8871Driver.h"

namespace droid::motor {
    DRV8871Driver::DRV8871Driver(const char* name, droid::services::System* sys, uint8_t out1, uint8_t out2) :
        name(name),
        out1(out1),
        out2(out2) {

        logger = sys->getLogger();
        pwmService = sys->getPWMService();
        stop();
    }

    void DRV8871Driver::init() {
        pwmService->setPWMpercent(out1, 0);
        pwmService->setPWMpercent(out2, 0);
    }

    void DRV8871Driver::factoryReset() {
        //TODO No config values yet
    }

    void DRV8871Driver::logConfig() {
        //No config defined yet
    }

    void DRV8871Driver::failsafe() {
        stop();
    }

    void DRV8871Driver::drive(int8_t speed) {
        lastCommandMs = millis();
        if (speed == 0) {
            requestedDutyCycle = 0;
        } else {
            requestedDutyCycle = map(speed, -128, 127, -100, 100);
        }
        //logger->log(name, DEBUG, "drive - speed = %d, requestedDutyCycle = %d\n", speed, requestedDutyCycle);
        task();
    }
    
    void DRV8871Driver::turn(int8_t speed) {
        //Single motor driver
        //Noop
    }
    
    void DRV8871Driver::stop() {
        drive(0);
    }

    void DRV8871Driver::setTimeout(uint16_t milliSeconds) {
        timeoutMs = milliSeconds;
    }

    void DRV8871Driver::setPowerRamp(float_t rampPowerPerMs) {
        this->rampPowerPerMs = abs(rampPowerPerMs);
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
        if (dutyCycle == 0) {
            //Braking
            pwmService->setPWMpercent(out1, 100);
            pwmService->setPWMpercent(out2, 100);
        } else if (dutyCycle > 0) {
            analogWrite(out2, 0);
            analogWrite(out1, (abs(dutyCycle)));
        } else {
            analogWrite(out1, 0);
            analogWrite(out2, (abs(dutyCycle)));
        }
    }
}