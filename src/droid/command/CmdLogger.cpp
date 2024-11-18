#include "droid/command/CmdLogger.h"

namespace droid::command {
    CmdLogger::CmdLogger(const char* name, droid::core::System* system) :
        CmdHandler(name, system) {}

    bool CmdLogger::process(const char* device, const char* command) {
        logger->log(name, INFO, "Device: %s, Command: %s\n", device, command);
        return false;
    }
}
