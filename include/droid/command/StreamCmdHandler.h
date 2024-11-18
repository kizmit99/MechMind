#pragma once
#include <Arduino.h>
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