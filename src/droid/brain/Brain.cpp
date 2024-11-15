#include <Arduino.h>
#include "droid/brain/Brain.h"
#include "droid/command/StreamCmdHandler.h"
#include "droid/command/CmdLogger.h"
#include "droid/brain/LocalCmdHandler.h"
#include "droid/audio/AudioCmdHandler.h"

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
        actionMgr("ActionMgr", &system, &controller),
        audioDriver(&HCR_STREAM),
        audioMgr("AudioMgr", &system, &audioDriver) {

        config = system.getConfig();
        logger = system.getLogger();
        actionMgr.addCmdHandler(new droid::command::CmdLogger("CmdLogger", &system));
        //TODO Implement configurable Serial ports
        actionMgr.addCmdHandler(new droid::command::StreamCmdHandler("Dome", &system, &DOME_STREAM));
        actionMgr.addCmdHandler(new droid::command::StreamCmdHandler("Body", &system, &BODY_STREAM));
        actionMgr.addCmdHandler(new droid::audio::AudioCmdHandler("HCR", &system, &audioMgr));
        actionMgr.addCmdHandler(new droid::brain::LocalCmdHandler("Brain", &system, this));
    }

    void Brain::init() {
        bool initialized = config->getBool(name, CONFIG_KEY_BRAIN_INITIALIZED, false);
        if (!initialized) {
            logger->log(name, INFO, "Brain has not been initialized, performing a factory reset.");
            factoryReset();
        }
        pwmService.init();
        pwmService.setOscFreq(PCA9685_OSC_FREQUENCY);
        controller.init();
        controller.setDeadband(CONTROLLER_DEADBAND);
        motorDriver.init();
        domeMgr.init();
        actionMgr.init();
        audioMgr.init();
    }

    void Brain::factoryReset() {
        config->putBool(name, CONFIG_KEY_BRAIN_INITIALIZED, true);
        controller.factoryReset();
        motorDriver.factoryReset();
        actionMgr.factoryReset();
        audioMgr.factoryReset();
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

    void Brain::processCmdInput(Stream* cmdStream) {
        //Check for incoming serial commands
        while (cmdStream->available()) {
            char in = cmdStream->read();
            if (in == '\b') {    //backspace
                cmdStream->print("\b \b");
                if (bufIndex > 0) {
                    bufIndex--;
                }
            } else {
                cmdStream->print(in);
                if (in == '\r') {
                    cmdStream->print('\n');
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
                    actionMgr.queueCommand("Brain", inputBuf, millis());
                }
                bufIndex = 0;
            }
        }
    }

    void Brain::task() {
        processCmdInput(&LOGGER_STREAM);
        controller.task();
        domeMgr.task();
        actionMgr.task();
        motorDriver.task();
        audioMgr.task();

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
        audioMgr.logConfig();
        actionMgr.logConfig();
    }
}