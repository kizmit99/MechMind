/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#include "droid/command/ESPNowCmdHandler.h"

namespace droid::command {
    ESPNowCmdHandler::ESPNowCmdHandler(const char* name, droid::core::System* system) :
        CmdHandler(name, system) {}

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