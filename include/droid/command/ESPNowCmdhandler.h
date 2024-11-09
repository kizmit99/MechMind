#pragma once
#include <Arduino.h>
#include "CmdHandler.h"
#include "droid/services/System.h"

namespace droid::command {
    class ESPNowCmdHandler : public CmdHandler {
    public:
        ESPNowCmdHandler(const char* name, droid::services::System* system);
        bool process(const char* device, const char* command);

    private:
        const char* name;
        droid::services::Logger* logger;
    };
}