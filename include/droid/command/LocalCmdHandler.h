#pragma once
#include <Arduino.h>
#include "CmdHandler.h"
#include "droid/services/System.h"
#include "droid/brain/Brain.h"

namespace droid::command {
    class LocalCmdHandler : public CmdHandler {
    public:
        LocalCmdHandler(const char* name, droid::services::System* system, droid::brain::Brain* brain);
        bool process(const char* device, const char* command);

    private:
        const char* name;
        droid::services::Logger* logger;
        droid::services::Config* config;
        droid::services::DroidState* droidState;
        droid::brain::Brain* brain;
    };
}