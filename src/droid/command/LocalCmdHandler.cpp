#include "droid/command/LocalCmdHandler.h"

namespace droid::command {
    LocalCmdHandler::LocalCmdHandler(const char* name, droid::services::System* system) :
        name(name),
        logger(system->getLogger()) {}

    bool LocalCmdHandler::process(const char* device, const char* command) {
        if ((strcmp(name, device) == 0) &&
            (command != NULL)) {
            //TODO Implement
            logger->log(name, WARN, "LocalCmdHandler not implemented!\n");
            return true;
        } else {
            return false;
        }
    }
}