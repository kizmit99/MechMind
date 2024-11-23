/*
 * Droid Brain Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#include "droid/command/CmdLogger.h"

namespace droid::command {
    CmdLogger::CmdLogger(const char* name, droid::core::System* system) :
        CmdHandler(name, system) {}

    bool CmdLogger::process(const char* device, const char* command) {
        logger->log(name, INFO, "Device: %s, Command: %s\n", device, command);
        return false;
    }
}
