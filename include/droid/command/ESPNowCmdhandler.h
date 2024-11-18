#pragma once
#include <Arduino.h>
#include "CmdHandler.h"

namespace droid::command {
    class ESPNowCmdHandler : public CmdHandler {
    public:
        ESPNowCmdHandler(const char* name, droid::services::System* system);
        bool process(const char* device, const char* command);
    };
}