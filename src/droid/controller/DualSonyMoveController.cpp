#include "droid/controller/DualSonyMoveController.h"
#include "droid/brain/hardware.h"
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
    DualSonyMoveController::DualSonyMoveController(const char* name, droid::services::System* system) :
        Controller(name, system),
        Usb(),
        Btd(&Usb),
        PS3Right(&Btd),
        PS3Left(&Btd) {

        if (DualSonyMoveController::instance != NULL) {
            logger->log(name, droid::services::Logger::Level::FATAL, "Constructor for DualSonyMoveController called more than once!\n");
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
                logger->log(name, INFO, "msgLag: %d, inactiveTimeout: %d, lastMsg: %d, origLastMsgTime: %d, deviceLastmsgTime: %d\n", msgLagTime, inactiveTimeout, holdLastMsgTime, origLastMsgTime, reportedLastMsgTime);
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
        //Logic for Dual Sony Triggers pulled almost directly from PenumbraShadowMD
        //------------------------------------ 
        // Send triggers for the base buttons 
        //------------------------------------
        if (PS3Right.ps3BT.getButtonPress(ButtonEnum::UP) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::CROSS) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::CIRCLE) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::L1) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::PS))
        {
            if (PS3Left.ps3BT.PS3NavigationConnected && (PS3Left.ps3BT.getButtonPress(ButtonEnum::CROSS) || PS3Left.ps3BT.getButtonPress(ButtonEnum::CIRCLE) || PS3Left.ps3BT.getButtonPress(ButtonEnum::PS)))
            {
                // Skip this section
            }
            else
            {
                return "FullAwake";
            }
        }
    
        if (PS3Right.ps3BT.getButtonPress(ButtonEnum::DOWN) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::CROSS) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::CIRCLE) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::L1) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::PS))
        {
            if (PS3Left.ps3BT.PS3NavigationConnected && (PS3Left.ps3BT.getButtonPress(ButtonEnum::CROSS) || PS3Left.ps3BT.getButtonPress(ButtonEnum::CIRCLE) || PS3Left.ps3BT.getButtonPress(ButtonEnum::PS)))
            {
                // Skip this section
            }
            else
            {     
                return "QuietMode";
            }
        }
    
        if (PS3Right.ps3BT.getButtonPress(ButtonEnum::LEFT) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::CROSS) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::CIRCLE) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::L1) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::PS))
        {
            if (PS3Left.ps3BT.PS3NavigationConnected && (PS3Left.ps3BT.getButtonPress(ButtonEnum::CROSS) || PS3Left.ps3BT.getButtonPress(ButtonEnum::CIRCLE) || PS3Left.ps3BT.getButtonPress(ButtonEnum::PS)))
            {
                // Skip this section
            }
            else
            {           
                return "MidAwake";
            }
        }

        if (PS3Right.ps3BT.getButtonPress(ButtonEnum::RIGHT) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::CROSS) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::CIRCLE) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::L1) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::PS))
        {
            if (PS3Left.ps3BT.PS3NavigationConnected && (PS3Left.ps3BT.getButtonPress(ButtonEnum::CROSS) || PS3Left.ps3BT.getButtonPress(ButtonEnum::CIRCLE) || PS3Left.ps3BT.getButtonPress(ButtonEnum::PS)))
            {
                // Skip this section
            }
            else
            {     
                return "FullAware";
            }
        }
    
        //------------------------------------ 
        // Send triggers for the CROSS + base buttons 
        //------------------------------------
        if (((!PS3Left.ps3BT.PS3NavigationConnected && PS3Right.ps3BT.getButtonPress(ButtonEnum::UP) && PS3Right.ps3BT.getButtonPress(ButtonEnum::CROSS)) || (PS3Left.ps3BT.PS3NavigationConnected && PS3Right.ps3BT.getButtonPress(ButtonEnum::UP) && PS3Left.ps3BT.getButtonPress(ButtonEnum::CROSS))))
        {
            return "VolumeUp";
        }
        
        if (((!PS3Left.ps3BT.PS3NavigationConnected && PS3Right.ps3BT.getButtonPress(ButtonEnum::DOWN) && PS3Right.ps3BT.getButtonPress(ButtonEnum::CROSS)) || (PS3Left.ps3BT.PS3NavigationConnected && PS3Right.ps3BT.getButtonPress(ButtonEnum::DOWN) && PS3Left.ps3BT.getButtonPress(ButtonEnum::CROSS))))
        {      
            return "VolumeDown";
        }
        
        if (((!PS3Left.ps3BT.PS3NavigationConnected && PS3Right.ps3BT.getButtonPress(ButtonEnum::LEFT) && PS3Right.ps3BT.getButtonPress(ButtonEnum::CROSS)) || (PS3Left.ps3BT.PS3NavigationConnected && PS3Right.ps3BT.getButtonPress(ButtonEnum::LEFT) && PS3Left.ps3BT.getButtonPress(ButtonEnum::CROSS))))
        {
            return "HolosOn";
        }

        if (((!PS3Left.ps3BT.PS3NavigationConnected && PS3Right.ps3BT.getButtonPress(ButtonEnum::RIGHT) && PS3Right.ps3BT.getButtonPress(ButtonEnum::CROSS)) || (PS3Left.ps3BT.PS3NavigationConnected && PS3Right.ps3BT.getButtonPress(ButtonEnum::RIGHT) && PS3Left.ps3BT.getButtonPress(ButtonEnum::CROSS))))
        {
            return "HolosOff";
        }

        //------------------------------------ 
        // Send triggers for the CIRCLE + base buttons 
        //------------------------------------
        if (((!PS3Left.ps3BT.PS3NavigationConnected && PS3Right.ps3BT.getButtonPress(ButtonEnum::UP) && PS3Right.ps3BT.getButtonPress(ButtonEnum::CIRCLE)) || (PS3Left.ps3BT.PS3NavigationConnected && PS3Right.ps3BT.getButtonPress(ButtonEnum::UP) && PS3Left.ps3BT.getButtonPress(ButtonEnum::CIRCLE))))
        {
            return "Scream";
        }
        
        if (((!PS3Left.ps3BT.PS3NavigationConnected && PS3Right.ps3BT.getButtonPress(ButtonEnum::DOWN) && PS3Right.ps3BT.getButtonPress(ButtonEnum::CIRCLE)) || (PS3Left.ps3BT.PS3NavigationConnected && PS3Right.ps3BT.getButtonPress(ButtonEnum::DOWN) && PS3Left.ps3BT.getButtonPress(ButtonEnum::CIRCLE))))
        {
            return "Disco";
        }
        
        if (((!PS3Left.ps3BT.PS3NavigationConnected && PS3Right.ps3BT.getButtonPress(ButtonEnum::LEFT) && PS3Right.ps3BT.getButtonPress(ButtonEnum::CIRCLE)) || (PS3Left.ps3BT.PS3NavigationConnected && PS3Right.ps3BT.getButtonPress(ButtonEnum::LEFT) && PS3Left.ps3BT.getButtonPress(ButtonEnum::CIRCLE))))
        {
            return "FastSmirk";
        }

        if (((!PS3Left.ps3BT.PS3NavigationConnected && PS3Right.ps3BT.getButtonPress(ButtonEnum::RIGHT) && PS3Right.ps3BT.getButtonPress(ButtonEnum::CIRCLE)) || (PS3Left.ps3BT.PS3NavigationConnected && PS3Right.ps3BT.getButtonPress(ButtonEnum::RIGHT) && PS3Left.ps3BT.getButtonPress(ButtonEnum::CIRCLE))))
        {
            return "ShortCircuit";
        }
    
        //------------------------------------ 
        // Send triggers for the L1 + base buttons 
        //------------------------------------
        if (PS3Right.ps3BT.getButtonPress(ButtonEnum::UP) && PS3Right.ps3BT.getButtonPress(ButtonEnum::L1))
        {
            return "CantinaDance";
        }
        
        if (PS3Right.ps3BT.getButtonPress(ButtonEnum::DOWN) && PS3Right.ps3BT.getButtonPress(ButtonEnum::L1))
        {
            return "LeiaMessage";
        }
        
        if (PS3Right.ps3BT.getButtonPress(ButtonEnum::LEFT) && PS3Right.ps3BT.getButtonPress(ButtonEnum::L1))
        {
            return "Wave";
        }

        if (PS3Right.ps3BT.getButtonPress(ButtonEnum::RIGHT) && PS3Right.ps3BT.getButtonPress(ButtonEnum::L1))
        {
            return "Wave2";
        }
    
        //------------------------------------ 
        // Send triggers for the PS + base buttons 
        //------------------------------------
        if (((!PS3Left.ps3BT.PS3NavigationConnected && PS3Right.ps3BT.getButtonPress(ButtonEnum::UP) && PS3Right.ps3BT.getButtonPress(ButtonEnum::PS)) || (PS3Left.ps3BT.PS3NavigationConnected && PS3Right.ps3BT.getButtonPress(ButtonEnum::UP) && PS3Left.ps3BT.getButtonPress(ButtonEnum::PS))))
        {
            return "Custom1";
        }
        
        if (((!PS3Left.ps3BT.PS3NavigationConnected && PS3Right.ps3BT.getButtonPress(ButtonEnum::DOWN) && PS3Right.ps3BT.getButtonPress(ButtonEnum::PS)) || (PS3Left.ps3BT.PS3NavigationConnected && PS3Right.ps3BT.getButtonPress(ButtonEnum::DOWN) && PS3Left.ps3BT.getButtonPress(ButtonEnum::PS))))
        {
            return "Custom2";
        }
        
        if (((!PS3Left.ps3BT.PS3NavigationConnected && PS3Right.ps3BT.getButtonPress(ButtonEnum::LEFT) && PS3Right.ps3BT.getButtonPress(ButtonEnum::PS)) || (PS3Left.ps3BT.PS3NavigationConnected && PS3Right.ps3BT.getButtonPress(ButtonEnum::LEFT) && PS3Left.ps3BT.getButtonPress(ButtonEnum::PS))))
        {
            return "Custom3";
        }

        if (((!PS3Left.ps3BT.PS3NavigationConnected && PS3Right.ps3BT.getButtonPress(ButtonEnum::RIGHT) && PS3Right.ps3BT.getButtonPress(ButtonEnum::PS)) || (PS3Left.ps3BT.PS3NavigationConnected && PS3Right.ps3BT.getButtonPress(ButtonEnum::RIGHT) && PS3Left.ps3BT.getButtonPress(ButtonEnum::PS))))
        {
            return "Custom4";
        }

        //------------------------------------ 
        // Send triggers for the base buttons 
        //------------------------------------
        if (PS3Left.ps3BT.getButtonPress(ButtonEnum::UP) && !PS3Left.ps3BT.getButtonPress(ButtonEnum::CROSS) && !PS3Left.ps3BT.getButtonPress(ButtonEnum::CIRCLE) && !PS3Left.ps3BT.getButtonPress(ButtonEnum::L1) && !PS3Left.ps3BT.getButtonPress(ButtonEnum::PS) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::CROSS) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::CIRCLE) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::PS))
        {
            return "BeepCantina";
        }
        
        if (PS3Left.ps3BT.getButtonPress(ButtonEnum::DOWN) && !PS3Left.ps3BT.getButtonPress(ButtonEnum::CROSS) && !PS3Left.ps3BT.getButtonPress(ButtonEnum::CIRCLE) && !PS3Left.ps3BT.getButtonPress(ButtonEnum::L1) && !PS3Left.ps3BT.getButtonPress(ButtonEnum::PS) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::CROSS) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::CIRCLE) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::PS))
        {
            return "MarchingAnts";
        }
        
        if (PS3Left.ps3BT.getButtonPress(ButtonEnum::LEFT) && !PS3Left.ps3BT.getButtonPress(ButtonEnum::CROSS) && !PS3Left.ps3BT.getButtonPress(ButtonEnum::CIRCLE) && !PS3Left.ps3BT.getButtonPress(ButtonEnum::L1) && !PS3Left.ps3BT.getButtonPress(ButtonEnum::PS) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::CROSS) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::CIRCLE) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::PS))
        {
            return "OpenBodyP1";
        }

        if (PS3Left.ps3BT.getButtonPress(ButtonEnum::RIGHT) && !PS3Left.ps3BT.getButtonPress(ButtonEnum::CROSS) && !PS3Left.ps3BT.getButtonPress(ButtonEnum::CIRCLE) && !PS3Left.ps3BT.getButtonPress(ButtonEnum::L1) && !PS3Left.ps3BT.getButtonPress(ButtonEnum::PS) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::CROSS) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::CIRCLE) && !PS3Right.ps3BT.getButtonPress(ButtonEnum::PS))
        {
            return "CloseBodyP1";
        }
    
        //------------------------------------ 
        // Send triggers for the CROSS + base buttons 
        //------------------------------------
        if (PS3Left.ps3BT.getButtonPress(ButtonEnum::UP) && PS3Right.ps3BT.getButtonPress(ButtonEnum::CROSS))
        {
            return "VolumeMax";
        }
        
        if (PS3Left.ps3BT.getButtonPress(ButtonEnum::DOWN) && PS3Right.ps3BT.getButtonPress(ButtonEnum::CROSS))
        {
            return "VolumeMid";
        }
        
        if (PS3Left.ps3BT.getButtonPress(ButtonEnum::LEFT) && PS3Right.ps3BT.getButtonPress(ButtonEnum::CROSS))
        {
            return "CloseDomeAll";
        }

        if (PS3Left.ps3BT.getButtonPress(ButtonEnum::RIGHT) && PS3Right.ps3BT.getButtonPress(ButtonEnum::CROSS))
        {
            return "OpenDomeAll";
        }

        //------------------------------------ 
        // Send triggers for the CIRCLE + base buttons 
        //------------------------------------
        if (PS3Left.ps3BT.getButtonPress(ButtonEnum::UP) && PS3Right.ps3BT.getButtonPress(ButtonEnum::CIRCLE))
        {
            return "HoloMoveOn";
        }
        
        if (PS3Left.ps3BT.getButtonPress(ButtonEnum::DOWN) && PS3Right.ps3BT.getButtonPress(ButtonEnum::CIRCLE))
        {
            return "HoloReset";
        }
        
        if (PS3Left.ps3BT.getButtonPress(ButtonEnum::LEFT) && PS3Right.ps3BT.getButtonPress(ButtonEnum::CIRCLE))
        {
            return "HoloLightsOn";
        }

        if (PS3Left.ps3BT.getButtonPress(ButtonEnum::RIGHT) && PS3Right.ps3BT.getButtonPress(ButtonEnum::CIRCLE))
        {
            return "HoloLightsOff";
        }
    
        //------------------------------------ 
        // Send triggers for the L1 + base buttons 
        //------------------------------------
        if (PS3Left.ps3BT.getButtonPress(ButtonEnum::UP) && PS3Left.ps3BT.getButtonPress(ButtonEnum::L1))
        {
            return "OpenDomeP1";
        }
        
        if (PS3Left.ps3BT.getButtonPress(ButtonEnum::DOWN) && PS3Left.ps3BT.getButtonPress(ButtonEnum::L1))
        {
            return "CloseDomeP1";
        }
        
        if (PS3Left.ps3BT.getButtonPress(ButtonEnum::LEFT) && PS3Left.ps3BT.getButtonPress(ButtonEnum::L1))
        {
            return "OpenDomeP2";
        }

        if (PS3Left.ps3BT.getButtonPress(ButtonEnum::RIGHT) && PS3Left.ps3BT.getButtonPress(ButtonEnum::L1))
        {
            return "CloseDomeP2";
        }
    
        //------------------------------------ 
        // Send triggers for the PS + base buttons 
        //------------------------------------
        if (PS3Left.ps3BT.getButtonPress(ButtonEnum::UP) && PS3Right.ps3BT.getButtonPress(ButtonEnum::PS))
        {
            return "OpenDomeP3";
        }
        
        if (PS3Left.ps3BT.getButtonPress(ButtonEnum::DOWN) && PS3Right.ps3BT.getButtonPress(ButtonEnum::PS))
        {
            return "CloseDomeP3";
        }
        
        if (PS3Left.ps3BT.getButtonPress(ButtonEnum::LEFT) && PS3Right.ps3BT.getButtonPress(ButtonEnum::PS))
        {
            return "OpenDomeP3";
        }

        if (PS3Left.ps3BT.getButtonPress(ButtonEnum::RIGHT) && PS3Right.ps3BT.getButtonPress(ButtonEnum::PS))
        {
            return "CloseDomeP3";
        }

        //------------------------------------
        // Send triggers for command toggles
        //------------------------------------
        // enable / disable drive stick
        if (PS3Right.ps3BT.getButtonPress(ButtonEnum::PS) && PS3Right.ps3BT.getButtonClick(ButtonEnum::CROSS))
        {
            return "StickDisable";
        }
        
        if(PS3Right.ps3BT.getButtonPress(ButtonEnum::PS) && PS3Right.ps3BT.getButtonClick(ButtonEnum::CIRCLE))
        {
            return "StickEnable";
        }
        
        // Enable and Disable Overspeed
        if (PS3Right.ps3BT.getButtonPress(L3) && PS3Right.ps3BT.getButtonPress(ButtonEnum::L1))
        {
            return "ToggleSpeed";
        }
    
        // Enable Disable Dome Automation
        if(PS3Right.ps3BT.getButtonPress(ButtonEnum::L2) && PS3Right.ps3BT.getButtonClick(ButtonEnum::CROSS))
        {
            return "AutoDomeOff";
        } 

        if(PS3Right.ps3BT.getButtonPress(ButtonEnum::L2) && PS3Right.ps3BT.getButtonClick(ButtonEnum::CIRCLE))
        {
            return "AutoDomeOn";
        } 

        return String("");
    }

    DualSonyMoveController* DualSonyMoveController::instance = NULL;
}
