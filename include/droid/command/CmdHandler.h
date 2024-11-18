#pragma once
#include "droid/core/PassiveComponent.h"

namespace droid::command {
    class CmdHandler : public droid::core::PassiveComponent {
    public:
        CmdHandler(const char* name, droid::core::System* system) :
            PassiveComponent(name, system) {}

        virtual bool process(const char* device, const char* command) = 0;
    };
}