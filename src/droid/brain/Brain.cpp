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

namespace droid::brain {
    Brain::Brain(const char* name) : 
        name(name),
        system(name, &Serial, DEBUG),
        controller("DualSony", &system),
//        controller("StubCtrl", &system),
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

        if (logger->getMaxLevel() >= ERROR) {
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