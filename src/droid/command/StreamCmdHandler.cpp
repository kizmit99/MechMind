#include "droid/command/StreamCmdHandler.h"

namespace droid::command {
    StreamCmdHandler::StreamCmdHandler(const char* name, droid::services::System* system, Stream* stream) :
        name(name),
        stream(stream) {}

    bool StreamCmdHandler::process(const char* device, const char* command) {
        if ((strcmp(name, device) == 0) &&
            (command != NULL)) {
            stream->printf("%s\r", command);
            return true;
        } else {
            return false;
        }
    }
}