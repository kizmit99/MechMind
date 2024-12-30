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
#include <PS3BT.h>

#include "droid/controller/Controller.h"
#include "droid/core/System.h"
#include <map>

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
        DualSonyMoveController(const char* name, droid::core::System* system);

        //Override virtual methods from BaseComponent
        void init() override;
        void factoryReset() override;
        void task() override;
        void logConfig() override;
        void failsafe() override;

        void setCritical(bool isCritical);
        //Joystick Position should be returned as a value between -100 and +100 for each axis
        int8_t getJoystickPosition(Joystick, Axis);
        String getAction();
        ControllerType getType() {return DUAL_SONY;}

    private:
        USB Usb;
        BTD Btd;
        ControllerDetails PS3Right;
        ControllerDetails PS3Left;
        static DualSonyMoveController* instance;
        void (*statusChangeCallback)(Controller*) = nullptr;
        bool isCritical = false;
        uint32_t activeTimeout = 0;
        uint32_t inactiveTimeout = 0;
        uint32_t badDataWindow = 0;
        int8_t deadbandX = 0;
        int8_t deadbandY = 0;
        std::map<String, String> triggerMap;

        void onInitPS3(Joystick which);
        void faultCheck(ControllerDetails* controller);
        void disconnect(ControllerDetails* controller);
        String getTrigger();

        static void onInitPS3RightWrapper() {
            instance->onInitPS3(RIGHT);
        }
        static void onInitPS3LeftWrapper() {
            instance->onInitPS3(LEFT);
        }
    };
}
