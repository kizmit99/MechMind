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
#include "droid/motor/MotorDriver.h"

namespace droid::motor {
    class StubMotorDriver : public MotorDriver {
    public:
        StubMotorDriver(const char* name, droid::core::System* system) :
            MotorDriver(name, system) {}

        void init() {}
        void factoryReset() {}
        void task() {}
        void logConfig() {}
        void failsafe() {}

        bool setMotorSpeed(uint8_t motor, int8_t speed) {return true;}
        bool arcadeDrive(int8_t joystickX, int8_t joystickY) {return true;}
        void stop() {}
    };
}
