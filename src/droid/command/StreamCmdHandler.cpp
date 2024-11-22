#include "droid/command/StreamCmdHandler.h"

namespace droid::command {
    StreamCmdHandler::StreamCmdHandler(const char* name, droid::core::System* system, Stream* stream) :
        CmdHandler(name, system),
        stream(stream) {}

    bool StreamCmdHandler::process(const char* device, const char* command) {
        if ((strcmp(name, device) == 0) &&
            (command != NULL)) {
            if (stream != NULL) {
                stream->printf("%s\r", command);
            }
            return true;
        } else {
            return false;
        }
    }
}