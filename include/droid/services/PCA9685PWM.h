#pragma once
#include "PWMService.h"
#include <Adafruit_PWMServoDriver.h>

#define NUMBER_OF_PWM_OUTPUTS 16

namespace droid::services {
    class PCA9685PWM : public PWMService {
    public:
        PCA9685PWM(const uint8_t I2CAddress, uint8_t outputEnablePin = 0);
        PCA9685PWM(const uint8_t I2CAddress, TwoWire &i2c, uint8_t outputEnablePin = 0);
        void init();
        void task();
        void failsafe();
        void setPWMuS(uint8_t outNum, uint16_t pulseMicroseconds, uint16_t durationMilliseconds = 0);
        void setPWMpercent(uint8_t outNum, uint8_t percent, uint16_t durationMilliseconds = 0);
        void setOscFreq(uint32_t freq);

    private:
        Adafruit_PWMServoDriver pca9685Driver;
        bool initialized = false;
        uint8_t outputEnablePin;
        struct {
            bool isActive;
            uint32_t disableAt;
        } outDetails[NUMBER_OF_PWM_OUTPUTS];
    };
}