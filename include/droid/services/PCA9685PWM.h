/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/MechMind
 */

#pragma once
#include "droid/services/PWMService.h"
#include <Adafruit_PWMServoDriver.h>

#define NUMBER_OF_PWM_OUTPUTS 16

namespace droid::services {
    class PCA9685PWM : public PWMService {
    public:
        PCA9685PWM(const char* name, droid::core::System* system, const uint8_t I2CAddress, uint8_t outputEnablePin = 0);
        PCA9685PWM(const char* name, droid::core::System* system, const uint8_t I2CAddress, TwoWire &i2c, uint8_t outputEnablePin = 0);

        //Override virtual methods from PWMService/BaseComponent
        void init() override;
        void factoryReset() override;
        void task() override;
        void logConfig() override;
        void failsafe() override;

        void setPWMuS(uint8_t outNum, uint16_t pulseMicroseconds, uint16_t durationMilliseconds = 0) override;
        void setPWMpercent(uint8_t outNum, uint8_t percent, uint16_t durationMilliseconds = 0) override;

    private:
        Adafruit_PWMServoDriver pca9685Driver;
        bool initialized = false;
        uint8_t outputEnablePin = 0;
        struct {
            bool isActive = false;
            uint32_t disableAt = 0;
        } outDetails[NUMBER_OF_PWM_OUTPUTS];
    };
}