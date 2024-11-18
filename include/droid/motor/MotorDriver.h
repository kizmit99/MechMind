#pragma once
#include "droid/services/ActiveComponent.h"

namespace droid::motor {
    class MotorDriver : public droid::services::ActiveComponent {
    public:
        MotorDriver(const char* name, droid::services::System* system) :
            ActiveComponent(name, system) {}
            
        //Virtual methods from ActiveComponent redeclared here for clarity
        virtual void init() = 0;
        virtual void factoryReset() = 0;
        virtual void task() = 0;
        virtual void logConfig() = 0;
        virtual void failsafe() = 0;

        virtual bool setMotorSpeed(uint8_t motor, int8_t speed) = 0;
        virtual bool arcadeDrive(int8_t joystickX, int8_t joystickY) = 0;
        virtual void stop() = 0;
    };
}