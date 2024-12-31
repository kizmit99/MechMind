/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/MechMind
 */

#include <Arduino.h>
#include "droid/core/hardware.h"
#include "droid/controller/PS3BtController.h"
#include <string>
#include <stdexcept>

#define CONFIG_KEY_PS3_MAC                 "MAC"
#define CONFIG_KEY_PS3_ALT_MAC             "AltMAC"
#define CONFIG_KEY_PS3_DEADBAND_X          "DeadbandX"
#define CONFIG_KEY_PS3_DEADBAND_Y          "DeadbandY"
#define CONFIG_KEY_PS3_ACTIVE_TIMEOUT      "activeTimeout"
#define CONFIG_KEY_PS3_INACTIVE_TIMEOUT    "inactiveTimeout"
#define CONFIG_KEY_PS3_BAD_DATA_WINDOW     "badDataWindow"
#define CONFIG_DEFAULT_PS3_ACTIVE_TIMEOUT   200
#define CONFIG_DEFAULT_PS3_INACTIVE_TIMEOUT 10000
#define CONFIG_DEFAULT_PS3_BAD_DATA_WINDOW  50
#define CONFIG_DEFAULT_PS3_DEADBAND         20

namespace droid::controller {
    PS3BtController::PS3BtController(const char* name, droid::core::System* system) :
        Controller(name, system),
        Usb(),
        Btd(&Usb),
        PS3(&Btd) {

        if (PS3BtController::instance != NULL) {
            logger->log(name, FATAL, "Constructor for PS3Controller called more than once!\n");
            while (1);  //TODO Better way to handle this???
        }
        PS3BtController::instance = this;

        PS3.ps3BT.attachOnInit(PS3BtController::onInitPS3Wrapper);
    }

    void PS3BtController::factoryReset() {
        config->clear(name);
        config->putString(name, CONFIG_KEY_PS3_MAC, CONFIG_DEFAULT_PS3_MAC);
        config->putString(name, CONFIG_KEY_PS3_ALT_MAC, CONFIG_DEFAULT_PS3_ALT_MAC);
        config->putInt(name, CONFIG_KEY_PS3_ACTIVE_TIMEOUT, CONFIG_DEFAULT_PS3_ACTIVE_TIMEOUT);
        config->putInt(name, CONFIG_KEY_PS3_INACTIVE_TIMEOUT, CONFIG_DEFAULT_PS3_INACTIVE_TIMEOUT);
        config->putInt(name, CONFIG_KEY_PS3_BAD_DATA_WINDOW, CONFIG_DEFAULT_PS3_BAD_DATA_WINDOW);
        config->putInt(name, CONFIG_KEY_PS3_DEADBAND_X, CONFIG_DEFAULT_PS3_DEADBAND);
        config->putInt(name, CONFIG_KEY_PS3_DEADBAND_Y, CONFIG_DEFAULT_PS3_DEADBAND);

        //init triggerMap with defaults then store default into config
        triggerMap.clear();
        #include "droid/controller/PS3Trigger.map"
        // Iterate through the map clearing all overrides
        for (const auto& mapEntry : triggerMap) {
            const char* trigger = mapEntry.first.c_str();
            const char* action = mapEntry.second.c_str();
            config->putString(name, trigger, action);
        }
    }

    void PS3BtController::init() {
        logger->log(name, INFO, "init - called\n");
        strncpy(PS3.MAC, config->getString(name, CONFIG_KEY_PS3_MAC, CONFIG_DEFAULT_PS3_MAC).c_str(), sizeof(PS3.MAC));
        strncpy(PS3.MACBackup, config->getString(name, CONFIG_KEY_PS3_ALT_MAC, CONFIG_DEFAULT_PS3_ALT_MAC).c_str(), sizeof(PS3.MACBackup));
        activeTimeout = config->getInt(name, CONFIG_KEY_PS3_ACTIVE_TIMEOUT, CONFIG_DEFAULT_PS3_ACTIVE_TIMEOUT);
        inactiveTimeout = config->getInt(name, CONFIG_KEY_PS3_INACTIVE_TIMEOUT, CONFIG_DEFAULT_PS3_INACTIVE_TIMEOUT);
        badDataWindow = config->getInt(name, CONFIG_KEY_PS3_BAD_DATA_WINDOW, CONFIG_DEFAULT_PS3_BAD_DATA_WINDOW);
        deadbandX = config->getInt(name, CONFIG_KEY_PS3_DEADBAND_X, CONFIG_DEFAULT_PS3_DEADBAND);
        deadbandY = config->getInt(name, CONFIG_KEY_PS3_DEADBAND_Y, CONFIG_DEFAULT_PS3_DEADBAND);

        //init triggerMap with defaults then load overrides from config
        triggerMap.clear();
        #include "droid/controller/PS3Trigger.map"
        // Iterate through the map looking for overrides
        for (const auto& mapEntry : triggerMap) {
            const char* trigger = mapEntry.first.c_str();
            const char* action = mapEntry.second.c_str();
            String override = config->getString(name, trigger, action);
            if (override != action) {
                triggerMap[trigger] = override;
            }
        }

        if (Usb.Init() != 0) {
            logger->log(name, FATAL, "Unable to init() the USB stack");
        }
    }

    void PS3BtController::task() {
        //logger->log(name, DEBUG, "task - called\n");
        Usb.Task();
        faultCheck(&PS3);
    }

    void PS3BtController::logConfig() {
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_PS3_MAC, config->getString(name, CONFIG_KEY_PS3_MAC, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_PS3_ALT_MAC, config->getString(name, CONFIG_KEY_PS3_ALT_MAC, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_PS3_ACTIVE_TIMEOUT, config->getString(name, CONFIG_KEY_PS3_ACTIVE_TIMEOUT, ""));
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_PS3_INACTIVE_TIMEOUT, config->getString(name, CONFIG_KEY_PS3_INACTIVE_TIMEOUT, ""));
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_PS3_BAD_DATA_WINDOW, config->getString(name, CONFIG_KEY_PS3_BAD_DATA_WINDOW, ""));
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_PS3_DEADBAND_X, config->getString(name, CONFIG_KEY_PS3_DEADBAND_X, ""));
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_PS3_DEADBAND_Y, config->getString(name, CONFIG_KEY_PS3_DEADBAND_Y, ""));

        // Iterate through the triggerMap for keys to log
        for (const auto& mapEntry : triggerMap) {
            const char* trigger = mapEntry.first.c_str();
            logger->log(name, INFO, "Config %s = %s\n", trigger, config->getString(name, trigger, "").c_str());
        }
    }

    void PS3BtController::failsafe() {
        //Noop
    }

    void PS3BtController::setCritical(bool isCritical) {
        this->isCritical = isCritical;
    }

    void PS3BtController::faultCheck(ControllerDetails* controller) {
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

    void PS3BtController::disconnect(ControllerDetails* controller) {
        controller->ps3BT.disconnect();
        controller->badDataCount = 0;
        controller->lastBadDataTime = 0;
        controller->lastMsgTime = 0;
        controller->waitingForReconnect = false;
        controller->isConnected = false;
    }

    // Note this method return normalized Joystick positions in the range of -100 to +100
    int8_t PS3BtController::getJoystickPosition(Joystick joystick, Axis axis) {
        int8_t rawPosition = 0;
        int8_t deadband = 0;

        if (PS3.isConnected) {
            if (!PS3.ps3BT.getButtonPress(ButtonEnum::L1) && !PS3.ps3BT.getButtonPress(ButtonEnum::L2)) {
                switch (axis) {
                    case X:
                        if (joystick == RIGHT) {
                            rawPosition = (127 - PS3.ps3BT.getAnalogHat(RightHatY));
                        } else {
                            rawPosition = (127 - PS3.ps3BT.getAnalogHat(LeftHatY));
                        }
                        deadband = deadbandX;
                        break;

                    case Y:
                        if (joystick == RIGHT) {
                            rawPosition = (PS3.ps3BT.getAnalogHat(RightHatX) - 128);
                        } else {
                            rawPosition = (PS3.ps3BT.getAnalogHat(LeftHatX) - 128);
                        }
                        deadband = deadbandY;
                        break;

                    default:
                        //Shouldn't happen
                        rawPosition = 0;
                }
            }
        }

        if (abs(rawPosition) <= deadband) {
            rawPosition = 0;
        }
        int8_t normalizedPosition = map(rawPosition, -128, 127, -100, 100);
        if (rawPosition == 0) {
            normalizedPosition = 0;
        }
        return normalizedPosition;
    }

    void PS3BtController::onInitPS3() {
        logger->log(name, INFO, "PS3Controller::onInitPS3 called.\n");

        char btAddr[20];
        uint8_t* addr = Btd.disc_bdaddr;
        snprintf(btAddr, sizeof(btAddr), "%02X:%02X:%02X:%02X:%02X:%02X",
            addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

        PS3.ps3BT.setLedOn(LED1);
        PS3.lastMsgTime = millis();
        PS3.isConnected = true;

        logger->log(name, INFO, "Address of Last connected Device: %s\n", btAddr);
        
        if ((strncmp(btAddr, PS3.MAC, sizeof(btAddr)) == 0) || 
            (strncmp(btAddr, PS3.MACBackup, sizeof(btAddr)) == 0)) {
            logger->log(name, INFO, "We have our controller connected.\n");
        } else if (PS3.MAC[0] == 'X') {
            logger->log(name, INFO, "Assigning %s as controller.\n", btAddr);
            
            config->putString(name, CONFIG_KEY_PS3_MAC, btAddr);
            strncpy(PS3.MAC, btAddr, sizeof(PS3.MAC));
        } else {
            // Prevent connection from anything but the MAIN controllers          
            logger->log(name, WARN, "We have an invalid controller trying to connect, it will be dropped.\n");

            PS3.ps3BT.setLedOff(LED1);
            disconnect(&PS3);
        } 
    }

    String PS3BtController::getAction() {
        //Button definitions for Dual Sony Triggers to mimic PenumbraShadowMD
        String trigger = getTrigger();
        if (trigger == "") {    //No trigger detected
            return "";
        }
        return triggerMap[trigger];
    }

    String PS3BtController::getTrigger() {
        // Helper function to check for L1 modifier button press
        auto isL1Pressed = [this]() {
            return PS3.isConnected &&
                PS3.ps3BT.getButtonPress(ButtonEnum::L1) &&
                !PS3.ps3BT.getButtonPress(ButtonEnum::R1) &&
                !PS3.ps3BT.getButtonPress(ButtonEnum::L2) &&
                !PS3.ps3BT.getButtonPress(ButtonEnum::R2);
        };

        // Helper function to check for R1 modifier button press
        auto isR1Pressed = [this]() {
            return PS3.isConnected &&
                !PS3.ps3BT.getButtonPress(ButtonEnum::L1) &&
                PS3.ps3BT.getButtonPress(ButtonEnum::R1) &&
                !PS3.ps3BT.getButtonPress(ButtonEnum::L2) &&
                !PS3.ps3BT.getButtonPress(ButtonEnum::R2);
        };

        // Helper function to check for L2 modifier button press
        auto isL2Pressed = [this]() {
            return PS3.isConnected &&
                !PS3.ps3BT.getButtonPress(ButtonEnum::L1) &&
                !PS3.ps3BT.getButtonPress(ButtonEnum::R1) &&
                PS3.ps3BT.getButtonPress(ButtonEnum::L2) &&
                !PS3.ps3BT.getButtonPress(ButtonEnum::R2);
        };

        // Helper function to check for R2 modifier button press
        auto isR2Pressed = [this]() {
            return PS3.isConnected &&
                !PS3.ps3BT.getButtonPress(ButtonEnum::L1) &&
                !PS3.ps3BT.getButtonPress(ButtonEnum::R1) &&
                !PS3.ps3BT.getButtonPress(ButtonEnum::L2) &&
                PS3.ps3BT.getButtonPress(ButtonEnum::R2);
        };

        // Helper function to check for no modifier button presses
        auto noModifiersPressed = [this]() {
            return PS3.isConnected &&
                !PS3.ps3BT.getButtonPress(ButtonEnum::L1) &&
                !PS3.ps3BT.getButtonPress(ButtonEnum::R1) &&
                !PS3.ps3BT.getButtonPress(ButtonEnum::L2) &&
                !PS3.ps3BT.getButtonPress(ButtonEnum::R2);
        };

        // Check for special buttons first
        if (PS3.ps3BT.getButtonClick(ButtonEnum::START)) {return "Start";}
        if (PS3.ps3BT.getButtonClick(ButtonEnum::SELECT) && isR2Pressed()) {return "Select_R2";}
        if (PS3.ps3BT.getButtonClick(ButtonEnum::SELECT)) {return "Select";}
        if (PS3.ps3BT.getButtonClick(ButtonEnum::PS)) {return "P3";}
        if (PS3.ps3BT.getButtonClick(ButtonEnum::L3)) {return "L3";}
        if (PS3.ps3BT.getButtonClick(ButtonEnum::R3)) {return "R3";}

        // Check for unmodified button presses
        if (noModifiersPressed()) {
            if (PS3.ps3BT.getButtonClick(ButtonEnum::CROSS)) {return "Cross";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::CIRCLE)) {return "Circle";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::SQUARE)) {return "Square";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::TRIANGLE)) {return "Triangle";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::UP)) {return "Up";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::DOWN)) {return "Down";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::LEFT)) {return "Left";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::RIGHT)) {return "Right";}
        }

        // Check L1 + button presses
        if (isL1Pressed()) {
            if (PS3.ps3BT.getButtonClick(ButtonEnum::CROSS)) {return "L1_Cross";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::CIRCLE)) {return "L1_Circle";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::SQUARE)) {return "L1_Square";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::TRIANGLE)) {return "L1_Triangle";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::UP)) {return "L1_Up";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::DOWN)) {return "L1_Down";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::LEFT)) {return "L1_Left";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::RIGHT)) {return "L1_Right";}
        }

        // Check R1 + button presses
        if (isR1Pressed()) {
            if (PS3.ps3BT.getButtonClick(ButtonEnum::CROSS)) {return "R1_Cross";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::CIRCLE)) {return "R1_Circle";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::SQUARE)) {return "R1_Square";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::TRIANGLE)) {return "R1_Triangle";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::UP)) {return "R1_Up";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::DOWN)) {return "R1_Down";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::LEFT)) {return "R1_Left";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::RIGHT)) {return "R1_Right";}
        }

        // Check L2 + button presses
        if (isL2Pressed()) {
            if (PS3.ps3BT.getButtonClick(ButtonEnum::CROSS)) {return "L2_Cross";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::CIRCLE)) {return "L2_Circle";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::SQUARE)) {return "L2_Square";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::TRIANGLE)) {return "L2_Triangle";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::UP)) {return "L2_Up";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::DOWN)) {return "L2_Down";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::LEFT)) {return "L2_Left";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::RIGHT)) {return "L2_Right";}
        }

        // Check R2 + button presses
        if (isR2Pressed()) {
            if (PS3.ps3BT.getButtonClick(ButtonEnum::CROSS)) {return "R2_Cross";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::CIRCLE)) {return "R2_Circle";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::SQUARE)) {return "R2_Square";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::TRIANGLE)) {return "R2_Triangle";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::UP)) {return "R2_Up";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::DOWN)) {return "R2_Down";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::LEFT)) {return "R2_Left";}
            if (PS3.ps3BT.getButtonClick(ButtonEnum::RIGHT)) {return "R2_Right";}
        }

        return String("");
    }

    PS3BtController* PS3BtController::instance = NULL;
}
