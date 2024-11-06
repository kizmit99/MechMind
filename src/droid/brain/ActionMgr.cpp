#include "droid/brain/ActionMgr.h"

namespace droid::brain {
    ActionMgr::ActionMgr(const char* name, droid::services::System* system, droid::controller::Controller* controller) :
        name(name),
        logger(system->getLogger()),
        controller(controller) {}

    void ActionMgr::init() {
        //TODO
    }

    void ActionMgr::task() {
        String trigger = controller->getTrigger();
        unsigned long now = millis();
        if ((trigger == lastTrigger) &&
            (now < (lastTriggerTime + 1000))) {
            //Skip it
         return;
        }
        lastTriggerTime = now;
        lastTrigger = trigger;
        if (trigger != "") {
            logger->log(name, DEBUG, "Trigger: %s\n", trigger);
        }
    }
}