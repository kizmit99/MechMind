/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

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