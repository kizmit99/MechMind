#pragma once
#include <Arduino.h>
#include "droid/command/CmdHandler.h"
#include "droid/services/PassiveComponent.h"
#include "droid/brain/Brain.h"

namespace droid::brain {
    class LocalCmdHandler : public droid::command::CmdHandler {
    public:
        LocalCmdHandler(const char* name, droid::services::System* system, Brain* brain);
        bool process(const char* device, const char* command);

    private:
        Brain* brain;
    };
}