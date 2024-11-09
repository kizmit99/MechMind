#include "droid/command/CmdLogger.h"

namespace droid::command {
    CmdLogger::CmdLogger(const char* name, droid::services::System* system) :
        name(name),
        logger(system->getLogger()) {}

    bool CmdLogger::process(const char* device, const char* command) {
        logger->log(name, INFO, "Device: %s, Command: %s\n", device, command);
        return false;
    }
}
