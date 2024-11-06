#include <Arduino.h>
#include "droid/brain/Brain.h"
#include "droid/services/System.h"
#include "droid/brain/DomeMgr.h"
#include "droid/controller/DualSonyMoveController.h"
#include "droid/motor/DRV8871Driver.h"

#define CONFIG_KEY_BRAIN_INITIALIZED    "initialized"

namespace droid::brain {
    Brain::Brain(const char* name) : 
        name(name),
        system(name, &Serial, DEBUG),
        controller("DualSony", &system),
        motorDriver("DRV8871", &system, 14, 13),
        domeMgr("DomeMgr", &system, &controller, &motorDriver),
        actionMgr("ActionMgr", &system, &controller) {
            config = system.getConfig();
            logger = system.getLogger();
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
    }
}