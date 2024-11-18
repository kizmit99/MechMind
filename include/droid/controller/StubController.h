#pragma once
#include "droid/controller/Controller.h"

namespace droid::controller {
    class StubController : public Controller {
    public:
        StubController(const char* name, droid::services::System* system) :
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
