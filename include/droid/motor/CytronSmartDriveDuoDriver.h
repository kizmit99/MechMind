#pragma once

#include "droid/motor/MotorDriver.h"
#include <motor/CytronSmartDriveDuoDriver.h>
#include "droid/services/System.h"

namespace droid::motor {
    class CytronSmartDriveDuoDriver : public MotorDriver {
    public:

        CytronSmartDriveDuoDriver(const char* name, droid::services::System* sys, byte address, Stream& port, uint8_t initialByte = 0x80);

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
        droid::services::Logger* logger;
        droid::services::Config* config;

        ::CytronSmartDriveDuoDriver wrapped;

        int8_t motorSpeed[2] = {0};
        uint8_t deadband = 0;
        uint16_t timeoutMs = 0;
        ulong lastCommandMs = 0;
        ulong lastUpdateMs = 0;
    };

    class CytronSmartDriveDuoMDDS10Driver : public CytronSmartDriveDuoDriver
    {
    public:
        CytronSmartDriveDuoMDDS10Driver(const char* name, droid::services::System* sys, byte address, Stream& port) :
            CytronSmartDriveDuoDriver(name, sys, address, port, 0x55) {}
    };

    class CytronSmartDriveDuoMDDS30Driver : public CytronSmartDriveDuoDriver
    {
    public:
        CytronSmartDriveDuoMDDS30Driver(const char* name, droid::services::System* sys, byte address, Stream& port) :
            CytronSmartDriveDuoDriver(name, sys, address, port, 0x80) {}
    };

    class CytronSmartDriveDuoMDDS60Driver : public CytronSmartDriveDuoDriver
    {
    public:
        CytronSmartDriveDuoMDDS60Driver(const char* name, droid::services::System* sys, byte address, Stream& port) :
            CytronSmartDriveDuoDriver(name, sys, address, port, 0x55) {}
    };
}