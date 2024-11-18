#pragma once
#include "droid/services/System.h"

namespace droid::services {
    class PassiveComponent {
    public:
        PassiveComponent(const char* name, System* system) :
            name(name),
            system(system),
            logger(system->getLogger()),
            config(system->getConfig()),
            droidState(system->getDroidState()) {}

    protected:
        const char* name;
        System* system;
        Logger* logger;
        Config* config;
        DroidState* droidState;
    };
}