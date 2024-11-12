#pragma once
#include <Arduino.h>
#include "droid/brain/hardware.h"
#include "droid/services/System.h"
#include "droid/services/NoPWMService.h"
#include "droid/services/PCA9685PWM.h"
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
        void reboot();
        void overrideCmdMap(const char* trigger, const char* cmd);
        void trigger(const char* trigger);

    private:
        const char* name;
#ifdef BUILD_FOR_DEBUGGER
        droid::services::NoPWMService pwmService;
#else
        droid::services::PCA9685PWM pwmService;
#endif
        droid::services::System system;
        droid::services::Config* config;
        droid::services::Logger* logger;
#ifdef BUILD_FOR_DEBUGGER
        droid::controller::StubController controller;
#else
        droid::controller::DualSonyMoveController controller;
#endif
        droid::motor::DRV8871Driver motorDriver;
        DomeMgr domeMgr;
        ActionMgr actionMgr;

        char inputBuf[100];
        uint8_t bufIndex = 0;
    };
}