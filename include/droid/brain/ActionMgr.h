#pragma once
#include <Arduino.h>
#include "droid/services/System.h"
#include "droid/controller/Controller.h"

namespace droid::brain {
    class ActionMgr {
    public:
        ActionMgr(const char* name, droid::services::System* system, droid::controller::Controller*);
        void init();
        void task();

    private:
        const char* name;
        droid::controller::Controller* controller;
        droid::services::Logger* logger;
        droid::services::Config* config;
        unsigned long lastTriggerTime = 0;
        String lastTrigger;
    };
}