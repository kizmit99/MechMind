/*
 * Droid Brain Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#include <Arduino.h>
#include "droid/core/hardware.h"
#include "droid/controller/DualSonyMoveController.h"
#include <string>
#include <stdexcept>

#define CONFIG_KEY_SONY_RIGHT_MAC           "RightMAC"
#define CONFIG_KEY_SONY_ALT_RIGHT_MAC       "RightAltMAC"
#define CONFIG_KEY_SONY_LEFT_MAC            "LeftMAC"
#define CONFIG_KEY_SONY_ALT_LEFT_MAC        "LeftAltMAC"
#define CONFIG_KEY_SONY_ACTIVE_TIMEOUT      "activeTimeout"
#define CONFIG_KEY_SONY_INACTIVE_TIMEOUT    "inactiveTimeout"
#define CONFIG_KEY_SONY_BAD_DATA_WINDOW     "badDataWindow"
#define CONFIG_DEFAULT_SONY_ACTIVE_TIMEOUT   200
#define CONFIG_DEFAULT_SONY_INACTIVE_TIMEOUT 10000
#define CONFIG_DEFAULT_SONY_BAD_DATA_WINDOW  50

namespace droid::controller {
    DualSonyMoveController::DualSonyMoveController(const char* name, droid::core::System* system) :
        Controller(name, system),
        Usb(),
        Btd(&Usb),
        PS3Right(&Btd),
        PS3Left(&Btd) {

        if (DualSonyMoveController::instance != NULL) {
            logger->log(name, FATAL, "Constructor for DualSonyMoveController called more than once!\n");
            while (1);  //TODO Better way to handle this???
        }
        DualSonyMoveController::instance = this;

        PS3Right.ps3BT.attachOnInit(DualSonyMoveController::onInitPS3RightWrapper);
        PS3Left.ps3BT.attachOnInit(DualSonyMoveController::onInitPS3LeftWrapper);
    }

    void DualSonyMoveController::factoryReset() {
        config->clear(name);
        config->putString(name, CONFIG_KEY_SONY_RIGHT_MAC, CONFIG_DEFAULT_SONY_RIGHT_MAC);
        config->putString(name, CONFIG_KEY_SONY_ALT_RIGHT_MAC, CONFIG_DEFAULT_SONY_ALT_RIGHT_MAC);
        config->putString(name, CONFIG_KEY_SONY_LEFT_MAC, CONFIG_DEFAULT_SONY_LEFT_MAC);
        config->putString(name, CONFIG_KEY_SONY_ALT_LEFT_MAC, CONFIG_DEFAULT_SONY_ALT_LEFT_MAC);
        config->putInt(name, CONFIG_KEY_SONY_ACTIVE_TIMEOUT, CONFIG_DEFAULT_SONY_ACTIVE_TIMEOUT);
        config->putInt(name, CONFIG_KEY_SONY_INACTIVE_TIMEOUT, CONFIG_DEFAULT_SONY_INACTIVE_TIMEOUT);
        config->putInt(name, CONFIG_KEY_SONY_BAD_DATA_WINDOW, CONFIG_DEFAULT_SONY_BAD_DATA_WINDOW);
    }

    void DualSonyMoveController::init() {
        logger->log(name, INFO, "init - called\n");
        strncpy(PS3Right.MAC, config->getString(name, CONFIG_KEY_SONY_RIGHT_MAC, CONFIG_DEFAULT_SONY_RIGHT_MAC).c_str(), sizeof(PS3Right.MAC));
        strncpy(PS3Right.MACBackup, config->getString(name, CONFIG_KEY_SONY_ALT_RIGHT_MAC, CONFIG_DEFAULT_SONY_ALT_RIGHT_MAC).c_str(), sizeof(PS3Right.MACBackup));
        strncpy(PS3Left.MAC, config->getString(name, CONFIG_KEY_SONY_LEFT_MAC, CONFIG_DEFAULT_SONY_LEFT_MAC).c_str(), sizeof(PS3Left.MAC));
        strncpy(PS3Left.MACBackup, config->getString(name, CONFIG_KEY_SONY_ALT_LEFT_MAC, CONFIG_DEFAULT_SONY_ALT_LEFT_MAC).c_str(), sizeof(PS3Left.MACBackup));
        activeTimeout = config->getInt(name, CONFIG_KEY_SONY_ACTIVE_TIMEOUT, CONFIG_DEFAULT_SONY_ACTIVE_TIMEOUT);
        inactiveTimeout = config->getInt(name, CONFIG_KEY_SONY_INACTIVE_TIMEOUT, CONFIG_DEFAULT_SONY_INACTIVE_TIMEOUT);
        badDataWindow = config->getInt(name, CONFIG_KEY_SONY_BAD_DATA_WINDOW, CONFIG_DEFAULT_SONY_BAD_DATA_WINDOW);
        if (Usb.Init() != 0) {
            logger->log(name, FATAL, "Unable to init() the USB stack");
        }
    }

    void DualSonyMoveController::task() {
        //logger->log(name, DEBUG, "task - called\n");
        Usb.Task();
        faultCheck(&PS3Right);
        faultCheck(&PS3Left);
    }

    void DualSonyMoveController::logConfig() {
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_SONY_RIGHT_MAC, config->getString(name, CONFIG_KEY_SONY_RIGHT_MAC, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_SONY_ALT_RIGHT_MAC, config->getString(name, CONFIG_KEY_SONY_ALT_RIGHT_MAC, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_SONY_LEFT_MAC, config->getString(name, CONFIG_KEY_SONY_LEFT_MAC, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_SONY_ALT_LEFT_MAC, config->getString(name, CONFIG_KEY_SONY_ALT_LEFT_MAC, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_SONY_ACTIVE_TIMEOUT, config->getString(name, CONFIG_KEY_SONY_ACTIVE_TIMEOUT, ""));
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_SONY_INACTIVE_TIMEOUT, config->getString(name, CONFIG_KEY_SONY_INACTIVE_TIMEOUT, ""));
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_SONY_BAD_DATA_WINDOW, config->getString(name, CONFIG_KEY_SONY_BAD_DATA_WINDOW, ""));
    }

    void DualSonyMoveController::failsafe() {
        //Noop
    }

    void DualSonyMoveController::setCritical(bool isCritical) {
        this->isCritical = isCritical;
    }

    void DualSonyMoveController::setDeadband(int8_t deadband) {
        this->deadband = abs(deadband);
    }

    void DualSonyMoveController::faultCheck(ControllerDetails* controller) {
        if (!controller->isConnected) {
            return;
        }

        if (controller->ps3BT.PS3Connected || 
            controller->ps3BT.PS3NavigationConnected  || 
            controller->ps3BT.PS3MoveConnected) {
            unsigned long now = millis();
            uint32_t origLastMsgTime = controller->lastMsgTime;
            uint32_t reportedLastMsgTime = controller->ps3BT.getLastMessageTime();
            if (reportedLastMsgTime > controller->lastMsgTime) {
                controller->lastMsgTime = controller->ps3BT.getLastMessageTime();
            }
            uint32_t msgLagTime = now - controller->lastMsgTime;

            if (controller->waitingForReconnect) {
                if (msgLagTime < activeTimeout) {
                    controller->waitingForReconnect = false;
                }
                controller->lastMsgTime = now;
            }

            if (now > controller->lastMsgTime) {
                msgLagTime = now - controller->lastMsgTime;
            } else {
                msgLagTime = 0;
            }

            if (isCritical && 
                (msgLagTime > activeTimeout)) {
                logger->log(name, ERROR, "Timeout while controller active\n");
            }

            if (msgLagTime > inactiveTimeout) {
                uint32_t holdLastMsgTime = controller->lastMsgTime;
                disconnect(controller);
                controller->waitingForReconnect = true;
                logger->log(name, ERROR, "Timeout while controller inactive\n");
                logger->log(name, ERROR, "msgLag: %d, inactiveTimeout: %d, lastMsg: %d, origLastMsgTime: %d, deviceLastmsgTime: %d\n", msgLagTime, inactiveTimeout, holdLastMsgTime, origLastMsgTime, reportedLastMsgTime);
                return;
            }

            if (!controller->ps3BT.getStatus(Plugged) && !controller->ps3BT.getStatus(Unplugged)) {
                if (now > (controller->lastBadDataTime + badDataWindow)) {
                    controller->badDataCount++;
                    controller->lastBadDataTime = now;
                }
                if (controller->badDataCount > 10) {
                    disconnect(controller);
                    controller->waitingForReconnect = true;
                    logger->log(name, ERROR, "Too much bad data from Controller\n");
                }
            } else {
                if (controller->badDataCount > 0) {
                    controller->badDataCount = 0;
                }
            }
        } else {
            controller->waitingForReconnect = true;
            controller->isConnected = false;
            logger->log(name, ERROR, "Lost connection to Controller while Active\n");
        }
    }

    void DualSonyMoveController::disconnect(ControllerDetails* controller) {
        controller->ps3BT.disconnect();
        controller->badDataCount = 0;
        controller->lastBadDataTime = 0;
        controller->lastMsgTime = 0;
        controller->waitingForReconnect = false;
        controller->isConnected = false;
    }

    int8_t DualSonyMoveController::getJoystickPosition(Joystick joystick, Axis axis) {
        //This class supports operating with either one or two controllers.
        //The general case is two controllers connected and this method returns the
        //position of that joystick as long as neither L1 nor L2 is pressed.
        //When only one controller is present, asking for the joystick position of
        //the connected controller works just as if there were two controllers.
        //However, when asking for the joystick position of the non-connected controller
        //The connected controller's joystick position will be returned if L2 is pressed.

        ControllerDetails* request;
        ControllerDetails* other;

        if (joystick == RIGHT) {
            request = &PS3Right;
            other = &PS3Left;
        } else {
            request = &PS3Left;
            other = &PS3Right;
        }

        int8_t rawPosition = 0;

        //If requested controller is present, use it
        if (request->isConnected) {
            if (!request->ps3BT.getButtonPress(ButtonEnum::L1) && !request->ps3BT.getButtonPress(ButtonEnum::L2)) {
                switch (axis) {
                    case X:
                        rawPosition = (127 - request->ps3BT.getAnalogHat(LeftHatY));
                        break;

                    case Y:
                        rawPosition = (request->ps3BT.getAnalogHat(LeftHatX) - 128);
                        break;

                    default:
                        //Shouldn't happen
                        rawPosition = 0;
                }
            }
        } else {
            //Use the other controller if requested one isn't connected
            if (other->isConnected) {
                if (other->ps3BT.getButtonPress(ButtonEnum::L2)) {
                    switch (axis) {
                        case X:
                            rawPosition = (127 - other->ps3BT.getAnalogHat(LeftHatY));
                            break;

                        case Y:
                            rawPosition = (other->ps3BT.getAnalogHat(LeftHatX) - 128);
                            break;

                        default:
                            //Shouldn't happen
                            rawPosition = 0;
                    }
                }
            }
        }

        if (abs(rawPosition) <= deadband) {
            return 0;
        }
        return rawPosition;
    }

    void DualSonyMoveController::onInitPS3(Joystick which) {
        ControllerDetails* controller;
        const char* whichStr;
        const char* configKey;

        switch (which) {
            case RIGHT:
                controller = &PS3Right;
                whichStr = "RIGHT";
                configKey = CONFIG_KEY_SONY_RIGHT_MAC;
                break;

            case LEFT:
                controller = &PS3Left;
                whichStr = "LEFT";
                configKey = CONFIG_KEY_SONY_LEFT_MAC;
                break;
        }

        char btAddr[20];
        uint8_t* addr = Btd.disc_bdaddr;
        snprintf(btAddr, sizeof(btAddr), "%02X:%02X:%02X:%02X:%02X:%02X",
            addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

        controller->ps3BT.setLedOn(LED1);
        controller->lastMsgTime = millis();
        controller->isConnected = true;

        logger->log(name, INFO, "Address of Last connected Device: %s\n", btAddr);
        
        if ((strncmp(btAddr, controller->MAC, sizeof(btAddr)) == 0) || 
            (strncmp(btAddr, controller->MACBackup, sizeof(btAddr)) == 0)) {
            logger->log(name, INFO, "We have our %s controller connected.\n", whichStr);
        } else if (controller->MAC[0] == 'X') {
            logger->log(name, INFO, "Assigning %s as %s controller.\n", btAddr, whichStr);
            
            config->putString(name, configKey, btAddr);
            strncpy(controller->MAC, btAddr, sizeof(controller->MAC));
        } else {
            // Prevent connection from anything but the MAIN controllers          
            logger->log(name, WARN, "We have an invalid controller trying to connect as the %s controller, it will be dropped.\n", whichStr);

            controller->ps3BT.setLedOff(LED1);
            disconnect(controller);
        } 
    }

    String DualSonyMoveController::getTrigger() {
        //Button definitions for Dual Sony Triggers to mimic PenumbraShadowMD

        // Helper function to check for individual button presses
        auto isButtonPressed = [this](PS3BT* thisController, ButtonEnum button) {
            return thisController->getButtonPress(button) &&
                !thisController->getButtonPress(ButtonEnum::CROSS) &&
                !thisController->getButtonPress(ButtonEnum::CIRCLE) &&
                !thisController->getButtonPress(ButtonEnum::L1) &&
                !thisController->getButtonPress(ButtonEnum::PS);
        };

        // Helper function to check for ANY modifier button press on 'other' controller
        auto isOtherModifierPressed = [this](PS3BT* otherController) {
            return otherController->PS3NavigationConnected &&
                (otherController->getButtonPress(ButtonEnum::CROSS) ||
                    otherController->getButtonPress(ButtonEnum::CIRCLE) ||
                    otherController->getButtonPress(ButtonEnum::PS));
        };

        // Helper function to check for a button + modifier combo
        // Note that if two controllers are active the modifier must be pressed on the 'other' controller
        auto checkCombo = [this](PS3BT* thisController, PS3BT* otherController, ButtonEnum button, ButtonEnum modifier) {
            return ((!otherController->PS3NavigationConnected && thisController->getButtonPress(button) && thisController->getButtonPress(modifier)) ||
                    (otherController->PS3NavigationConnected && thisController->getButtonPress(button) && otherController->getButtonPress(modifier)));
        };

        // Helper function to check for a button + L1 combo
        // Note that this is only looking for L1 on 'this' controller (not the 'other' one)
        auto checkL1Combo = [this](PS3BT* thisController, ButtonEnum button) {
            return (thisController->getButtonPress(button) && thisController->getButtonPress(ButtonEnum::L1));
        };

        // Base button on Right controller
        if (isButtonPressed(&PS3Right.ps3BT, ButtonEnum::UP) && !isOtherModifierPressed(&PS3Left.ps3BT)) {
            return "FullAwake";
        }
        if (isButtonPressed(&PS3Right.ps3BT, ButtonEnum::DOWN) && !isOtherModifierPressed(&PS3Left.ps3BT)) {
            return "QuietMode";
        }
        if (isButtonPressed(&PS3Right.ps3BT, ButtonEnum::LEFT) && !isOtherModifierPressed(&PS3Left.ps3BT)) {
            return "MidAwake";
        }
        if (isButtonPressed(&PS3Right.ps3BT, ButtonEnum::RIGHT) && !isOtherModifierPressed(&PS3Left.ps3BT)) {
            return "FullAware";
        }

        // CROSS + base buttons on Right controller
        if (checkCombo(&PS3Right.ps3BT, &PS3Left.ps3BT, ButtonEnum::UP, ButtonEnum::CROSS)) {
            return "VolumeUp";
        }
        if (checkCombo(&PS3Right.ps3BT, &PS3Left.ps3BT, ButtonEnum::DOWN, ButtonEnum::CROSS)) {
            return "VolumeDown";
        }
        if (checkCombo(&PS3Right.ps3BT, &PS3Left.ps3BT, ButtonEnum::LEFT, ButtonEnum::CROSS)) {
            return "HolosOn";
        }
        if (checkCombo(&PS3Right.ps3BT, &PS3Left.ps3BT, ButtonEnum::RIGHT, ButtonEnum::CROSS)) {
            return "HolosOff";
        }

        // CIRCLE + base buttons on Right controller
        if (checkCombo(&PS3Right.ps3BT, &PS3Left.ps3BT, ButtonEnum::UP, ButtonEnum::CIRCLE)) {
            return "Scream";
        }
        if (checkCombo(&PS3Right.ps3BT, &PS3Left.ps3BT, ButtonEnum::DOWN, ButtonEnum::CIRCLE)) {
            return "Disco";
        }
        if (checkCombo(&PS3Right.ps3BT, &PS3Left.ps3BT, ButtonEnum::LEFT, ButtonEnum::CIRCLE)) {
            return "FastSmirk";
        }
        if (checkCombo(&PS3Right.ps3BT, &PS3Left.ps3BT, ButtonEnum::RIGHT, ButtonEnum::CIRCLE)) {
            return "ShortCircuit";
        }

        // L1 + base buttons on Right controller
        if (checkL1Combo(&PS3Right.ps3BT, ButtonEnum::UP)) {
            return "CantinaDance";
        }
        if (checkL1Combo(&PS3Right.ps3BT, ButtonEnum::DOWN)) {
            return "LeiaMessage";
        }
        if (checkL1Combo(&PS3Right.ps3BT, ButtonEnum::LEFT)) {
            return "Wave";
        }
        if (checkL1Combo(&PS3Right.ps3BT, ButtonEnum::RIGHT)) {
            return "Wave2";
        }

        // PS + base buttons on Right controller
        if (checkCombo(&PS3Right.ps3BT, &PS3Left.ps3BT, ButtonEnum::UP, ButtonEnum::PS)) {
            return "Custom1";
        }
        if (checkCombo(&PS3Right.ps3BT, &PS3Left.ps3BT, ButtonEnum::DOWN, ButtonEnum::PS)) {
            return "Custom2";
        }
        if (checkCombo(&PS3Right.ps3BT, &PS3Left.ps3BT, ButtonEnum::LEFT, ButtonEnum::PS)) {
            return "Custom3";
        }
        if (checkCombo(&PS3Right.ps3BT, &PS3Left.ps3BT, ButtonEnum::RIGHT, ButtonEnum::PS)) {
            return "Custom4";
        }

        // Base button on Left controller
        if (isButtonPressed(&PS3Left.ps3BT, ButtonEnum::UP) && !isOtherModifierPressed(&PS3Right.ps3BT)) {
            return "BeepCantina";
        }
        if (isButtonPressed(&PS3Left.ps3BT, ButtonEnum::DOWN) && !isOtherModifierPressed(&PS3Right.ps3BT)) {
            return "MarchingAnts";
        }
        if (isButtonPressed(&PS3Left.ps3BT, ButtonEnum::LEFT) && !isOtherModifierPressed(&PS3Right.ps3BT)) {
            return "OpenBodyP1";
        }
        if (isButtonPressed(&PS3Left.ps3BT, ButtonEnum::RIGHT) && !isOtherModifierPressed(&PS3Right.ps3BT)) {
            return "CloseBodyP1";
        }

        // CROSS + base buttons on Left controller
        if (checkCombo(&PS3Left.ps3BT, &PS3Right.ps3BT, ButtonEnum::UP, ButtonEnum::CROSS)) {
            return "VolumeMax";
        }
        if (checkCombo(&PS3Left.ps3BT, &PS3Right.ps3BT, ButtonEnum::DOWN, ButtonEnum::CROSS)) {
            return "VolumeMid";
        }
        if (checkCombo(&PS3Left.ps3BT, &PS3Right.ps3BT, ButtonEnum::LEFT, ButtonEnum::CROSS)) {
            return "CloseDomeAll";
        }
        if (checkCombo(&PS3Left.ps3BT, &PS3Right.ps3BT, ButtonEnum::RIGHT, ButtonEnum::CROSS)) {
            return "OpenDomeAll";
        }

        // CIRCLE + base buttons on Left controller
        if (checkCombo(&PS3Left.ps3BT, &PS3Right.ps3BT, ButtonEnum::UP, ButtonEnum::CIRCLE)) {
            return "HoloMoveOn";
        }
        if (checkCombo(&PS3Left.ps3BT, &PS3Right.ps3BT, ButtonEnum::DOWN, ButtonEnum::CIRCLE)) {
            return "HoloReset";
        }
        if (checkCombo(&PS3Left.ps3BT, &PS3Right.ps3BT, ButtonEnum::LEFT, ButtonEnum::CIRCLE)) {
            return "HoloLightsOn";
        }
        if (checkCombo(&PS3Left.ps3BT, &PS3Right.ps3BT, ButtonEnum::RIGHT, ButtonEnum::CIRCLE)) {
            return "HoloLightsOff";
        }

        // L1 + base buttons on Left controller
        if (checkL1Combo(&PS3Left.ps3BT, ButtonEnum::UP)) {
            return "OpenDomeP1";
        }
        if (checkL1Combo(&PS3Left.ps3BT, ButtonEnum::DOWN)) {
            return "CloseDomeP1";
        }
        if (checkL1Combo(&PS3Left.ps3BT, ButtonEnum::LEFT)) {
            return "OpenDomeP2";
        }
        if (checkL1Combo(&PS3Left.ps3BT, ButtonEnum::RIGHT)) {
            return "CloseDomeP2";
        }

        // PS + base buttons on Left controller
        if (checkCombo(&PS3Left.ps3BT, &PS3Right.ps3BT, ButtonEnum::UP, ButtonEnum::PS)) {
            return "OpenDomeP3";
        }
        if (checkCombo(&PS3Left.ps3BT, &PS3Right.ps3BT, ButtonEnum::DOWN, ButtonEnum::PS)) {
            return "CloseDomeP3";
        }
        if (checkCombo(&PS3Left.ps3BT, &PS3Right.ps3BT, ButtonEnum::LEFT, ButtonEnum::PS)) {
            return "OpenDomeP4";
        }
        if (checkCombo(&PS3Left.ps3BT, &PS3Right.ps3BT, ButtonEnum::RIGHT, ButtonEnum::PS)) {
            return "CloseDomeP4";
        }

        // Triggers for command toggles

        // enable / disable drive stick
        if (PS3Right.ps3BT.getButtonPress(ButtonEnum::PS) && PS3Right.ps3BT.getButtonClick(ButtonEnum::CROSS)) {
            return "StickDisable";
        }
        
        if(PS3Right.ps3BT.getButtonPress(ButtonEnum::PS) && PS3Right.ps3BT.getButtonClick(ButtonEnum::CIRCLE)){
            return "StickEnable";
        }
        
        // Enable and Disable Overspeed
        if (PS3Right.ps3BT.getButtonPress(L3) && PS3Right.ps3BT.getButtonPress(ButtonEnum::L1)) {
            return "ToggleSpeed";
        }
    
        // Enable Disable Dome Automation
        if(PS3Right.ps3BT.getButtonPress(ButtonEnum::L2) && PS3Right.ps3BT.getButtonClick(ButtonEnum::CROSS)) {
            return "AutoDomeOff";
        } 

        if(PS3Right.ps3BT.getButtonPress(ButtonEnum::L2) && PS3Right.ps3BT.getButtonClick(ButtonEnum::CIRCLE)) {
            return "AutoDomeOn";
        } 

        return String("");
    }

    DualSonyMoveController* DualSonyMoveController::instance = NULL;
}
