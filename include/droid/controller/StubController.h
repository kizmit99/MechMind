/*
 * Droid Brain Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#pragma once
#include "droid/controller/Controller.h"

namespace droid::controller {
    class StubController : public Controller {
    public:
        StubController(const char* name, droid::core::System* system) :
            Controller(name, system) {}

        void init() override {}
        void factoryReset() override {}
        void task() override {}
        void logConfig() override {}
        void failsafe() override {}

        void setCritical(bool isCritical) {}
        void setDeadband(int8_t deadband) {}
        int8_t getJoystickPosition(Joystick, Axis) {return 0;}
        String getTrigger() {return "";}
    };
}
