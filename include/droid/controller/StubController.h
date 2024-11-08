#pragma once
#include <Arduino.h>

#include "droid/controller/Controller.h"
#include "droid/services/System.h"

namespace droid::controller {
    class StubController : public Controller {
    public:
        StubController(const char* name, droid::services::System* system) :
            name(name),
            logger(system->getLogger()),
            config(system->getConfig()) {}

        void init() {}
        void factoryReset() {}
        void task() {}
        void logConfig() {}
        void failsafe() {}
        void setCritical(bool isCritical) {}
        void setDeadband(int8_t deadband) {}
        int8_t getJoystickPosition(Joystick, Axis) {return 0;}
        String getTrigger() {return "";}

    private:
        //instance name
        const char* name;
        droid::services::Logger* logger;
        droid::services::Config* config;
    };
}
