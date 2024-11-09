#pragma once
#include <Arduino.h>
#include "CmdHandler.h"
#include "droid/services/System.h"

namespace droid::command {
    class StreamCmdHandler : public CmdHandler {
    public:
        StreamCmdHandler(const char* name, droid::services::System* system, Stream* stream);
        bool process(const char* device, const char* command);

    private:
        const char* name;
        Stream* stream;
    };
}