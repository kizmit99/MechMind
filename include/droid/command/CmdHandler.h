#pragma once
#include <Arduino.h>
#include "droid/services/PassiveComponent.h"

namespace droid::command {
    class CmdHandler : public droid::services::PassiveComponent {
    public:
        CmdHandler(const char* name, droid::services::System* system) :
            PassiveComponent(name, system) {}

        virtual bool process(const char* device, const char* command) = 0;
    };
}