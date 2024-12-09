/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#pragma once

#include "droid/motor/MotorDriver.h"
#include <motor/CytronSmartDriveDuoDriver.h>
#include "droid/core/System.h"

namespace droid::motor {
    class CytronSmartDriveDuoDriver : public MotorDriver {
    public:

        CytronSmartDriveDuoDriver(const char* name, droid::core::System* system, byte address, Stream& port, uint8_t initialByte = 0x80);

        //Override virtual methods from MotorDriver/BaseComponent
        void init() override;
        void factoryReset() override;
        void task() override;
        void logConfig() override;
        void failsafe() override;

        //Motor speed should be specified in a range from -100 to +100
        bool setMotorSpeed(uint8_t motor, int8_t speed);
        //Joystick Positions should be a value between -100 and +100 for each axis
        bool arcadeDrive(int8_t joystickX, int8_t joystickY);
        void stop();

    private:
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
        CytronSmartDriveDuoMDDS10Driver(const char* name, droid::core::System* system, byte address, Stream& port) :
            CytronSmartDriveDuoDriver(name, system, address, port, 0x55) {}
    };

    class CytronSmartDriveDuoMDDS30Driver : public CytronSmartDriveDuoDriver
    {
    public:
        CytronSmartDriveDuoMDDS30Driver(const char* name, droid::core::System* system, byte address, Stream& port) :
            CytronSmartDriveDuoDriver(name, system, address, port, 0x80) {}
    };

    class CytronSmartDriveDuoMDDS60Driver : public CytronSmartDriveDuoDriver
    {
    public:
        CytronSmartDriveDuoMDDS60Driver(const char* name, droid::core::System* system, byte address, Stream& port) :
            CytronSmartDriveDuoDriver(name, system, address, port, 0x55) {}
    };
}