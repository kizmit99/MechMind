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
#include <PS3USB.h>

#include "droid/controller/Controller.h"
#include "droid/core/System.h"
#include <map>

namespace droid::controller {
    class PS3UsbController : public Controller {
    public:
        PS3UsbController(const char* name, droid::core::System* system);

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
        ControllerType getType() {return ControllerType::PS3_USB;}

    private:
        struct ControllerDetails {
            PS3USB ps3USB;
            char MAC[20] = "";
            char MACBackup[20] = "";
            volatile int8_t badDataCount = 0;
            volatile unsigned long lastBadDataTime = 0;
            volatile uint32_t lastMsgTime = 0;
            volatile bool waitingForReconnect = false;
            volatile bool isConnected = false;

            //Struct Constructor
            ControllerDetails(USB* param) : ps3USB(param) {};
        };

        USB Usb;
        ControllerDetails PS3;
        static PS3UsbController* instance;
        void (*statusChangeCallback)(Controller*) = nullptr;
        bool isCritical = false;
        uint32_t activeTimeout = 0;
        uint32_t inactiveTimeout = 0;
        uint32_t badDataWindow = 0;
        int8_t deadbandX = 0;
        int8_t deadbandY = 0;
        std::map<String, String> triggerMap;

        void onInitPS3();
        void faultCheck(ControllerDetails* controller);
        void disconnect(ControllerDetails* controller);
        String getTrigger();

        static void onInitPS3Wrapper() {
            instance->onInitPS3();
        }
    };
}
