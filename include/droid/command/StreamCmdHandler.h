/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#pragma once
#include "droid/command/CmdHandler.h"

namespace droid::command {
    class StreamCmdHandler : public CmdHandler {
    public:
        StreamCmdHandler(const char* name, droid::core::System* system, Stream* stream);
        bool process(const char* device, const char* command);

    private:
        Stream* stream;
    };
}