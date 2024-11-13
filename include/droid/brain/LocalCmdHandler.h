#pragma once
#include <Arduino.h>
#include "droid/command/CmdHandler.h"
#include "droid/services/System.h"
#include "droid/brain/Brain.h"

namespace droid::brain {
    class LocalCmdHandler : public droid::command::CmdHandler {
    public:
        LocalCmdHandler(const char* name, droid::services::System* system, Brain* brain);
        bool process(const char* device, const char* command);

    private:
        const char* name;
        droid::services::Logger* logger;
        droid::services::Config* config;
        droid::services::DroidState* droidState;
        Brain* brain;
    };
}