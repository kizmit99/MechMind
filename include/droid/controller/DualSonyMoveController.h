#pragma once
#include <Arduino.h>
#include <PS3BT.h>

#include "droid/controller/Controller.h"
#include "droid/services/System.h"

namespace droid::controller {
    struct ControllerDetails {
        PS3BT ps3BT;
        char MAC[20] = "";
        char MACBackup[20] = "";
        volatile int8_t badDataCount = 0;
        volatile unsigned long lastBadDataTime = 0;
        volatile uint32_t lastMsgTime = 0;
        volatile bool waitingForReconnect = false;
        volatile bool isConnected = false;

        //Struct Constructor
        ControllerDetails(BTD* param) : ps3BT(param) {};
    };

    class DualSonyMoveController : public Controller {
    public:
        DualSonyMoveController(const char* name, droid::services::System* system);

        //Override virtual methods from ActiveComponent
        void init() override;
        void factoryReset() override;
        void task() override;
        void logConfig() override;
        void failsafe() override;

        void setCritical(bool isCritical);
        void setDeadband(int8_t deadband);
        int8_t getJoystickPosition(Joystick, Axis);
        String getTrigger();

    private:
        USB Usb;
        BTD Btd;
        ControllerDetails PS3Right;
        ControllerDetails PS3Left;
        static DualSonyMoveController* instance;
        void (*statusChangeCallback)(Controller*);
        bool isCritical;
        uint32_t activeTimeout;
        uint32_t inactiveTimeout;
        uint32_t badDataWindow;
        int8_t deadband;

        void onInitPS3(Joystick which);
        void faultCheck(ControllerDetails* controller);
        void disconnect(ControllerDetails* controller);

        static void onInitPS3RightWrapper() {
            instance->onInitPS3(RIGHT);
        }
        static void onInitPS3LeftWrapper() {
            instance->onInitPS3(LEFT);
        }
    };
}
