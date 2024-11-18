#pragma once
#include "droid/services/PWMService.h"
#include <Adafruit_PWMServoDriver.h>

#define NUMBER_OF_PWM_OUTPUTS 16

namespace droid::services {
    class PCA9685PWM : public PWMService {
    public:
        PCA9685PWM(const char* name, droid::core::System* system, const uint8_t I2CAddress, uint8_t outputEnablePin = 0);
        PCA9685PWM(const char* name, droid::core::System* system, const uint8_t I2CAddress, TwoWire &i2c, uint8_t outputEnablePin = 0);

        //Override virtual methods from PWMService/ActiveComponent
        void init() override;
        void factoryReset() override;
        void task() override;
        void logConfig() override;
        void failsafe() override;

        void setPWMuS(uint8_t outNum, uint16_t pulseMicroseconds, uint16_t durationMilliseconds = 0) override;
        void setPWMpercent(uint8_t outNum, uint8_t percent, uint16_t durationMilliseconds = 0) override;
        void setOscFreq(uint32_t freq) override;

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