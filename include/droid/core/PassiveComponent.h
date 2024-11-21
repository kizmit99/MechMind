#pragma once
#include "droid/core/System.h"

namespace droid::core {
    class PassiveComponent {
    public:
        PassiveComponent(const char* name, droid::core::System* system) :
            name(name),
            system(system),
            logger(system->getLogger()),
            config(system->getConfig()),
            droidState(system->getDroidState()) {}

        const char* name;
    protected:
        droid::core::System* system;
        Logger* logger;
        Config* config;
        droid::services::DroidState* droidState;
    };
}