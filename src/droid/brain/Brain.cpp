#include <Arduino.h>
#include "droid/brain/Brain.h"
#include "droid/services/System.h"
#include "droid/brain/DomeMgr.h"
#include "droid/controller/DualSonyMoveController.h"
#include "droid/motor/DRV8871Driver.h"
#include "droid/command/CmdHandler.h"
#include "droid/command/StreamCmdHandler.h"
#include "droid/command/CmdLogger.h"
#include "droid/command/LocalCmdHandler.h"

#define CONFIG_KEY_BRAIN_INITIALIZED    "initialized"

#define PCA9685_I2C_ADDRESS 0x40
#define PCA9685_OUTPUT_ENABLE_PIN 15

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
        system(name, &Serial, DEBUG, &pwmService),
        motorDriver("DRV8871", &system, 14, 13),
        domeMgr("DomeMgr", &system, &controller, &motorDriver),
        actionMgr("ActionMgr", &system, &controller) {
            config = system.getConfig();
            logger = system.getLogger();
            actionMgr.addCmdHandler(new droid::command::CmdLogger("CmdLogger", &system));
            //TODO Implement configurable Serial ports
            actionMgr.addCmdHandler(new droid::command::StreamCmdHandler("Dome", &system, &Serial));
            actionMgr.addCmdHandler(new droid::command::StreamCmdHandler("Body", &system, &Serial));
            actionMgr.addCmdHandler(new droid::command::StreamCmdHandler("HCR", &system, &Serial));
            actionMgr.addCmdHandler(new droid::command::LocalCmdHandler("Brain", &system));
        }

#define deadband 20

    void Brain::init() {
        bool initialized = config->getBool(name, CONFIG_KEY_BRAIN_INITIALIZED, false);
        if (!initialized) {
            factoryReset();
        }
        pwmService.init();
        pwmService.setOscFreq(27000000);
        controller.init();
        controller.setDeadband(deadband);
        motorDriver.init();
        domeMgr.init();
        actionMgr.init();
    }

    void Brain::factoryReset() {
        config->putBool(name, CONFIG_KEY_BRAIN_INITIALIZED, true);
        controller.factoryReset();
        motorDriver.factoryReset();
        actionMgr.factoryReset();
    }

    void Brain::task() {
        controller.task();
        domeMgr.task();
        actionMgr.task();
        motorDriver.task();

#ifdef BUILD_FOR_DEBUGGER
        if (Serial.available()) {
            Serial.read();
            logger->log(name, ERROR, "Generating a fake ERROR\n");
        }
#endif

        if (logger->getMaxLevel() >= ERROR) {
            pwmService.failsafe();
            controller.failsafe();
            motorDriver.failsafe();
            logger->clear();
        }
    }

    void Brain::logConfig() {
        logger->log(name, INFO, "Config %s = %d\n", CONFIG_KEY_BRAIN_INITIALIZED, config->getBool(name, CONFIG_KEY_BRAIN_INITIALIZED, false));
        controller.logConfig();
        motorDriver.logConfig();
        actionMgr.logConfig();
    }
}