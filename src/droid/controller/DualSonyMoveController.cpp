#include "droid/controller/DualSonyMoveController.h"

#define CONFIG_KEY_SONY_RIGHT_MAC           "RightMAC"
#define CONFIG_KEY_SONY_ALT_RIGHT_MAC       "RightAltMAC"
#define CONFIG_KEY_SONY_LEFT_MAC            "LeftMAC"
#define CONFIG_KEY_SONY_ALT_LEFT_MAC        "LeftAltMAC"
#define CONFIG_DEFAULT_MAC                  "XX:XX:XX:XX:XX:XX"
#define CONFIG_DEFAULT_SONY_RIGHT_MAC       CONFIG_DEFAULT_MAC
#define CONFIG_DEFAULT_SONY_ALT_RIGHT_MAC   CONFIG_DEFAULT_MAC
#define CONFIG_DEFAULT_SONY_LEFT_MAC        CONFIG_DEFAULT_MAC
#define CONFIG_DEFAULT_SONY_ALT_LEFT_MAC    CONFIG_DEFAULT_MAC

namespace droid::controller {
    DualSonyMoveController::DualSonyMoveController(const char* name, droid::services::System* sys) :
        name(name),
        Usb(),
        Btd(&Usb),
        PS3Right(&Btd),
        PS3Left(&Btd) {

        config = sys->getConfig();
        logger = sys->getLogger();
        if (DualSonyMoveController::instance != NULL) {
            logger->log(name, droid::services::Logger::Level::FATAL, "Constructor for DualSonyMoveController called more than once!\n");
            while (1);  //TODO Better eay to handle this???
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
    }

    void DualSonyMoveController::init() {
        logger->log(name, INFO, "init - called\n");
        if (Usb.Init() != 0) {
            logger->log(name, FATAL, "Unable to init() the USB stack");
        }
    }

    void DualSonyMoveController::task() {
        logger->log(name, INFO, "task - called\n");
        Usb.Task();
        faultCheck(&PS3Right);
        faultCheck(&PS3Left);
    }

    void DualSonyMoveController::setActive(Joystick which, bool active) {
        switch (which) {
            case RIGHT:
                PS3Right.active = active;
                break;

            case LEFT:
                PS3Left.active = active;
                break;
        }
    }

    void DualSonyMoveController::faultCheck(ControllerDetails* controller) {
        //TODO Check for faults!
        //  Check PS3BT.getLastMessageTime()s and PS3BT.PS3Connected (or PS3NavigationConnected?)
        //  to determine if we've lost the connection.
        //  Do WHAT if we have?  Log a Fault?
        //  Original code had two different timeout periods, 10Seconds and 200mS
        //  it also called getStatus and when it got 'bad' results 10 times it called disconnect()

        if (controller->ps3BT.PS3Connected || controller->ps3BT.PS3NavigationConnected) {
            unsigned long now = millis();
            uint32_t lastMsgTime = controller->ps3BT.getLastMessageTime();
            uint32_t msgLagTime = now - lastMsgTime;

            if (controller->waitingForReconnect) {
                if (msgLagTime < 200) {
                    controller->waitingForReconnect = false;
                }
                lastMsgTime = now;
            }

            if (now > lastMsgTime) {
                msgLagTime = now - lastMsgTime;
            } else {
                msgLagTime = 0;
            }

            if (controller->active && 
                (msgLagTime > 200)) {        //TODO make config
                logger->log(name, ERROR, "Timeout while controller active");
            }

            if (msgLagTime > 10000) {       //TODO make config
                controller->ps3BT.disconnect();
                controller->waitingForReconnect = true;
                logger->log(name, ERROR, "Timeout while controller inactive");
                return;
            }

            if (!controller->ps3BT.getStatus(Plugged) && !controller->ps3BT.getStatus(Unplugged)) {
                if (now > (controller->lastBadDataTime + 50)) {     //TODO Make Config
                    controller->badDataCount++;
                    controller->lastBadDataTime = now;
                }
                if (controller->badDataCount > 10) {
                    controller->ps3BT.disconnect();
                    controller->waitingForReconnect = true;
                    logger->log(name, ERROR, "Too much bad data from Controller");
                }
            } else {
                if (controller->badDataCount > 0) {
                    controller->badDataCount = 0;
                }
            }
        } else {
            if (controller->active) {
                controller->waitingForReconnect = true;
                logger->log(name, ERROR, "Lost connection to Controller while Active");
            }
        }
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

        //If requested controller is present, use it
        if (request->ps3BT.PS3NavigationConnected) {
            if (request->ps3BT.getButtonPress(L1) || request->ps3BT.getButtonPress(L2)) {
                return 0;
            }
            switch (axis) {
                case X:
                    return request->ps3BT.getAnalogHat(LeftHatX) - 128;

                case Y:
                    return request->ps3BT.getAnalogHat(LeftHatY) - 128;

                default:
                    return 0;
            }
        }

        //Use the other controller if requested one isn't connected
        if (other->ps3BT.PS3NavigationConnected) {
            if (!other->ps3BT.getButtonPress(L2)) {
                return 0;
            }
            switch (axis) {
                case X:
                    return other->ps3BT.getAnalogHat(LeftHatX) - 128;

                case Y:
                    return other->ps3BT.getAnalogHat(LeftHatY) - 128;

                default:
                    return 0;
            }
        }

        //No controllers are connected
        return 0;
    }
    
    String DualSonyMoveController::getTrigger() {
        //TODO
        return "";
    }

    void DualSonyMoveController::onInitPS3(Joystick which) {
        ControllerDetails* controller;
        const char* whichStr;
        const char* configKey;
        const char* altConfigKey;

        switch (which) {
            case RIGHT:
                controller = &PS3Right;
                whichStr = "RIGHT";
                configKey = CONFIG_KEY_SONY_RIGHT_MAC;
                altConfigKey = CONFIG_KEY_SONY_ALT_RIGHT_MAC;
                break;

            case LEFT:
                controller = &PS3Left;
                whichStr = "LEFT";
                configKey = CONFIG_KEY_SONY_LEFT_MAC;
                altConfigKey = CONFIG_KEY_SONY_ALT_LEFT_MAC;
                break;
        }

        char btAddr[20];
        uint8_t* addr = Btd.disc_bdaddr;
        snprintf(btAddr, sizeof(btAddr), "%02X:%02X:%02X:%02X:%02X:%02X",
            addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

        controller->ps3BT.setLedOn(LED1);

        logger->log(name, INFO, "Address of Last connected Device: %s\n", btAddr);
        
        if ((strncmp(btAddr, controller->MAC, sizeof(btAddr)) == 0) || 
            (strncmp(btAddr, controller->MACBackup, sizeof(btAddr)) == 0)) {
            logger->log(name, INFO, "We have our %s controller connected.\n", whichStr);
        }
        else if (controller->MAC[0] == 'X')
        {
            logger->log(name, INFO, "Assigning %s as %s controller.\n", btAddr, whichStr);
            
            config->putString(name, CONFIG_KEY_SONY_RIGHT_MAC, btAddr);
            strncpy(controller->MAC, btAddr, sizeof(controller->MAC));
        }
        else
        {
            // Prevent connection from anything but the MAIN controllers          
            logger->log(name, WARN, "We have an invalid controller trying to connect as the %s controller, it will be dropped.\n", whichStr);

            controller->ps3BT.setLedOff(LED1);
            controller->ps3BT.disconnect();
        } 
    }
}
