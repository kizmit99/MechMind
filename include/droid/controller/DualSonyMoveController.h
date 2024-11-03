#pragma once

#include <PS3BT.h>

#include "droid/controller/Controller.h"
#include "droid/services/System.h"

namespace droid::controller {
    struct ControllerDetails {
        PS3BT ps3BT;
        char MAC[20] = "";
        char MACBackup[20] = "";
        int8_t badDataCount = 0;
        unsigned long lastBadDataTime = 0;
        bool waitingForReconnect = false;
        bool active = false;

        //Struct Constructor
        ControllerDetails(BTD* param) : ps3BT(param) {};
    };

    class DualSonyMoveController : public Controller {
    public:
        DualSonyMoveController(const char* name, droid::services::System* system);
        void init();
        void factoryReset();
        void task();
        int8_t getJoystickPosition(Joystick, Axis);
        String getTrigger();
        void setActive(Joystick, bool active);

    private:
        //instance name
        const char* name;
        droid::services::Logger* logger;
        droid::services::Config* config;

        USB Usb;
        BTD Btd;
        ControllerDetails PS3Right;
        ControllerDetails PS3Left;
        static DualSonyMoveController* instance;
        void (*statusChangeCallback)(Controller*);

        void onInitPS3(Joystick which);
        void faultCheck(ControllerDetails* controller);

        static void onInitPS3RightWrapper() {
            instance->onInitPS3(RIGHT);
        }
        static void onInitPS3LeftWrapper() {
            instance->onInitPS3(LEFT);
        }
    };

    DualSonyMoveController* DualSonyMoveController::instance = NULL;
}
