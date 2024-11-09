#include "droid/command/ESPNowCmdHandler.h"

namespace droid::command {
    ESPNowCmdHandler::ESPNowCmdHandler(const char* name, droid::services::System* system) :
        name(name),
        logger(system->getLogger()) {}

    bool ESPNowCmdHandler::process(const char* device, const char* command) {
        if ((strcmp(name, device) == 0) &&
            (command != NULL)) {
            //TODO Implement
            logger->log(name, WARN, "ESPNowCmdHandler not implemented!\n");
            return true;
        } else {
            return false;
        }
    }
}