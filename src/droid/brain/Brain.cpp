#include <Arduino.h>
#include "droid/brain/Brain.h"
#include "droid/command/StreamCmdHandler.h"
#include "droid/command/CmdLogger.h"
#include "droid/command/LocalCmdHandler.h"

#define CONFIG_KEY_BRAIN_INITIALIZED    "initialized"

namespace droid::brain {
    Brain::Brain(const char* name) : 
        name(name),
#ifdef BUILD_FOR_DEBUGGER
        pwmService(),
        controller("StubCtrl", &system),
#else
        pwmService(PCA9685_I2C_ADDRESS, PCA9685_OUTPUT_ENABLE_PIN),
        controller("DualSony", &system),
#endif
        system(name, &LOGGER_STREAM, DEBUG, &pwmService),
        motorDriver("DRV8871", &system, PWMSERVICE_DOME_MOTOR_OUT1, PWMSERVICE_DOME_MOTOR_OUT2),
        domeMgr("DomeMgr", &system, &controller, &motorDriver),
        actionMgr("ActionMgr", &system, &controller) {
            config = system.getConfig();
            logger = system.getLogger();
            actionMgr.addCmdHandler(new droid::command::CmdLogger("CmdLogger", &system));
            //TODO Implement configurable Serial ports
            actionMgr.addCmdHandler(new droid::command::StreamCmdHandler("Dome", &system, &DOME_STREAM));
            actionMgr.addCmdHandler(new droid::command::StreamCmdHandler("Body", &system, &BODY_STREAM));
            actionMgr.addCmdHandler(new droid::command::StreamCmdHandler("HCR", &system, &HCR_STREAM));
            actionMgr.addCmdHandler(new droid::command::LocalCmdHandler("Brain", &system, this));
        }

    void Brain::init() {
        bool initialized = config->getString(name, CONFIG_KEY_BRAIN_INITIALIZED, "0") == "1";
        if (!initialized) {
            factoryReset();
        }
        pwmService.init();
        pwmService.setOscFreq(PCA9685_OSC_FREQUENCY);
        controller.init();
        controller.setDeadband(CONTROLLER_DEADBAND);
        motorDriver.init();
        domeMgr.init();
        actionMgr.init();
    }

    void Brain::factoryReset() {
        config->putString(name, CONFIG_KEY_BRAIN_INITIALIZED, "1");
        controller.factoryReset();
        motorDriver.factoryReset();
        actionMgr.factoryReset();
    }

    void Brain::reboot() {
        logger->log(name, WARN, "System Restarting...\n");
        sleep(2);
        ESP.restart();
    }

    void Brain::overrideCmdMap(const char* trigger, const char* cmd) {
        actionMgr.overrideCmdMap(trigger, cmd);
    }

    void Brain::trigger(const char* trigger) {
        actionMgr.fireTrigger(trigger);
    }

    void Brain::task() {
        //Check for incoming serial commands
        if (Serial.available()) {       //if or while?  while gives precedence to typing, if gives precedence to the rest of the droid?
            char in = Serial.read();
            if (in == '\b') {    //backspace
                Serial.print("\b \b");
                if (bufIndex > 0) {
                    bufIndex--;
                }
            } else {
                Serial.print(in);
                if (in == '\r') {
                    Serial.print('\n');
                }
                if (bufIndex < sizeof(inputBuf)) {
                    inputBuf[bufIndex] = in;
                    bufIndex++;
                } else {
                    //Buffer overrun, don't store incoming char
                }
            }
            if ((in == '\r') || (in == '\n')) {
                //end of input
                if (bufIndex > 1) {     //Skip empty commands
                    inputBuf[bufIndex - 1] = 0;
                    logger->log(name, DEBUG, "inputBuf: %s, len=%d\n", inputBuf, strlen(inputBuf));
                    actionMgr.queueCommand("Brain", inputBuf, millis());
                }
                bufIndex = 0;
            }
        }

        controller.task();
        domeMgr.task();
        actionMgr.task();
        motorDriver.task();

        if (logger->getMaxLevel() >= ERROR) {
            pwmService.failsafe();
            controller.failsafe();
            motorDriver.failsafe();
            logger->clear();
        }
    }

    void Brain::logConfig() {
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_BRAIN_INITIALIZED, config->getString(name, CONFIG_KEY_BRAIN_INITIALIZED, "0"));
        controller.logConfig();
        motorDriver.logConfig();
        actionMgr.logConfig();
    }
}