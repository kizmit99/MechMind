#pragma once
#include <Arduino.h>
#include "droid/services/System.h"
#include "droid/services/Logger.h"
#include "droid/controller/Controller.h"
#include "droid/motor/MotorDriver.h"

namespace droid::brain {
    class DomeMgr {
    public:
        DomeMgr(const char* name, droid::services::System* system, droid::controller::Controller*, droid::motor::MotorDriver*);
        void init();
        void task();

    private:
        const char* name;
        droid::controller::Controller* controller;
        droid::motor::MotorDriver* domeMotor;
        droid::services::Logger* logger;
        droid::services::Config* config;
        droid::services::DroidState* droidState;
    };
}