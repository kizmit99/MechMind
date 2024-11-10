#include "droid/services/PCA9685PWM.h"
#include <Arduino.h>

namespace droid::services {
    PCA9685PWM::PCA9685PWM(const uint8_t I2CAddress, uint8_t outputEnablePin) :
        outputEnablePin(outputEnablePin),
        pca9685Driver(I2CAddress) {}

    PCA9685PWM::PCA9685PWM(const uint8_t I2CAddress, TwoWire &i2c, uint8_t outputEnablePin) :
        outputEnablePin(outputEnablePin),
        pca9685Driver(I2CAddress, i2c) {}

    void PCA9685PWM::init() {
        pca9685Driver.begin();
        pca9685Driver.setOutputMode(true);
        if (outputEnablePin != 0) {
            pinMode(outputEnablePin, OUTPUT);
            digitalWrite(outputEnablePin, LOW);
        }
    }

    void PCA9685PWM::task() {
        uint32_t now = millis();
        for (int i = 0; i < NUMBER_OF_PWM_OUTPUTS; i++) {
            if (outDetails[i].isActive &&
                (outDetails[i].disableAt != 0) &&
                (now >= outDetails[i].disableAt)) {
                pca9685Driver.setPin(i, 0);
                outDetails[i].disableAt = 0;
                outDetails[i].isActive = false;
            }
        }
    }

    void PCA9685PWM::failsafe() {
        for (int i = 0; i < NUMBER_OF_PWM_OUTPUTS; i++) {
            pca9685Driver.setPin(i, 0);
            outDetails[i].disableAt = 0;
            outDetails[i].isActive = false;
        }
        if (outputEnablePin != 0) {
            digitalWrite(outputEnablePin, HIGH);
        }
    }

    void PCA9685PWM::setOscFreq(uint32_t freq) {
        pca9685Driver.setOscillatorFrequency(freq);
    }

    void PCA9685PWM::setPWMuS(uint8_t outNum, uint16_t pulseMicroseconds, uint16_t durationMilliseconds) {
        if (outNum >= NUMBER_OF_PWM_OUTPUTS) {
            return;
        }
        if (pulseMicroseconds == 0) {
            pca9685Driver.setPin(outNum, 0);
            outDetails[outNum].isActive = false;
            outDetails[outNum].disableAt = 0;
        } else {
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
        pca9685Driver.setPin(outNum, onTicks);
        outDetails[outNum].isActive = (onTicks > 0);
        if ((durationMilliseconds > 0) && (onTicks > 0)) {
            outDetails[outNum].disableAt = millis() + durationMilliseconds;
        } else {
            outDetails[outNum].disableAt = 0;
        }
    }
}