/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/MechMind
 */

#include "droid/services/PCA9685PWM.h"
#include "settings/hardware.config.h"

namespace droid::services {
    PCA9685PWM::PCA9685PWM(const char* name, droid::core::System* system, const uint8_t I2CAddress, uint8_t outputEnablePin) :
        PWMService(name, system),
        outputEnablePin(outputEnablePin),
        pca9685Driver(I2CAddress) {}

    PCA9685PWM::PCA9685PWM(const char* name, droid::core::System* system, const uint8_t I2CAddress, TwoWire &i2c, uint8_t outputEnablePin) :
        PWMService(name, system),
        outputEnablePin(outputEnablePin),
        pca9685Driver(I2CAddress, i2c) {}

    void PCA9685PWM::init() {
        if (pca9685Driver.begin()) {
            logger->log(name, DEBUG, "begin method successful!\n");
            pca9685Driver.setOscillatorFrequency(PCA9685_OSC_FREQUENCY);
            pca9685Driver.setPWMFreq(PCA9685_PWM_FREQ_HZ);
            pca9685Driver.setOutputMode(true);
            if (outputEnablePin != 0) {
                pinMode(outputEnablePin, OUTPUT);
                digitalWrite(outputEnablePin, LOW);
            }
            initialized = true;
        } else {
            logger->log(name, DEBUG, "begin method failed!");
            initialized = false;
        }
    }

    void PCA9685PWM::factoryReset() {
        //NOOP
    }

    void PCA9685PWM::task() {
        if (!initialized) return;
        uint32_t now = millis();
        for (int i = 0; i < NUMBER_OF_PWM_OUTPUTS; i++) {
            if (outDetails[i].isActive &&
                (outDetails[i].disableAt != 0) &&
                (now >= outDetails[i].disableAt)) {
                logger->log(name, DEBUG, "disabling PWM output %d after timeout\n", i);
                pca9685Driver.setPin(i, 0);
                outDetails[i].disableAt = 0;
                outDetails[i].isActive = false;
            }
        }
    }

    void PCA9685PWM::failsafe() {
        if (!initialized) return;
        for (int i = 0; i < NUMBER_OF_PWM_OUTPUTS; i++) {
            pca9685Driver.setPin(i, 0);
            outDetails[i].disableAt = 0;
            outDetails[i].isActive = false;
        }
        if (outputEnablePin != 0) {
            digitalWrite(outputEnablePin, HIGH);
        }
    }

    void PCA9685PWM::logConfig() {
        //NOOP
    }

    void PCA9685PWM::setPWMuS(uint8_t outNum, uint16_t pulseMicroseconds, uint16_t durationMilliseconds) {
        logger->log(name, DEBUG, "setPWMuS output %d, pulse: %d, duration: %d\n", outNum, pulseMicroseconds, durationMilliseconds);
        if (!initialized) return;
        if (outNum >= NUMBER_OF_PWM_OUTPUTS) {
            return;
        }
        logger->log(name, DEBUG, "getting ready to write to pca9685\n");
        if (pulseMicroseconds == 0) {
            pca9685Driver.setPin(outNum, 0);
            outDetails[outNum].isActive = false;
            outDetails[outNum].disableAt = 0;
        } else {
            logger->log(name, DEBUG, "oscFeq=%d, prescale=%d\n",pca9685Driver.getOscillatorFrequency(),pca9685Driver.readPrescale());
            pca9685Driver.writeMicroseconds(outNum, pulseMicroseconds);
            outDetails[outNum].isActive = true;
            if (durationMilliseconds > 0) {
                outDetails[outNum].disableAt = millis() + durationMilliseconds;
            } else {
                outDetails[outNum].disableAt = 0;
            }
        }
    }

    void PCA9685PWM::setPWMpercent(uint8_t outNum, uint8_t percent, uint16_t durationMilliseconds) {
        if (!initialized) return;
        if (outNum >= NUMBER_OF_PWM_OUTPUTS) {
            return;
        }
        if (percent > 100) {
            percent = 100;
        }
        uint16_t onTicks = map(percent, 0, 100, 0, 4095);
        if (percent == 0) {
            onTicks = 0;
        }
        if (percent == 100) {
            onTicks = 4095;
        }
//        logger->log(name, DEBUG, "setPWMpercent output %d, percent: %d, duration: %d\n", outNum, percent, durationMilliseconds);
        pca9685Driver.setPin(outNum, onTicks);
        outDetails[outNum].isActive = (onTicks > 0);
        if ((durationMilliseconds > 0) && (onTicks > 0)) {
            outDetails[outNum].disableAt = millis() + durationMilliseconds;
        } else {
            outDetails[outNum].disableAt = 0;
        }
    }
}