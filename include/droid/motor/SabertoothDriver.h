#pragma once

#include "droid/motor/MotorDriver.h"
#include <motor/SabertoothDriver.h>
#include "droid/services/System.h"

namespace droid::motor {
    class SabertoothDriver : public MotorDriver {
    public:
        SabertoothDriver(const char* name, droid::services::System* sys, byte address, Stream& port);

        void init();
        void factoryReset();
        void task();
        void logConfig();
        void failsafe();

        bool setMotorSpeed(uint8_t motor, int8_t speed);
        bool arcadeDrive(int8_t joystickX, int8_t joystickY);
        void stop();

    private:
        const char* name;
        droid::services::Config* config;
        droid::services::Logger* logger;
        ::SabertoothDriver wrapped;
    };
}