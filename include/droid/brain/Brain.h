#pragma once
#include <Arduino.h>
#include "droid/services/System.h"
#include "droid/controller/DualSonyMoveController.h"
#include "droid/controller/StubController.h"
#include "droid/motor/DRV8871Driver.h"
#include "DomeMgr.h"
#include "ActionMgr.h"

namespace droid::brain {
    class Brain {
    public:
        Brain(const char* name);
        void init();
        void factoryReset();
        void task();
        void logConfig();

    private:
        const char* name;
        droid::services::System system;
        droid::services::Config* config;
        droid::services::Logger* logger;
        droid::controller::DualSonyMoveController controller;
//        droid::controller::StubController controller;
        droid::motor::DRV8871Driver motorDriver;
        DomeMgr domeMgr;
        ActionMgr actionMgr;
    };
}