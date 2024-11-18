#pragma once
#include "droid/core/PassiveComponent.h"

namespace droid::core {
    class ActiveComponent : public PassiveComponent {
    public:
        ActiveComponent(const char* name, System* system) :
            PassiveComponent(name, system) {}

        virtual void init() = 0;
        virtual void factoryReset() = 0;
        virtual void task() = 0;
        virtual void logConfig() = 0;
        virtual void failsafe() = 0;
    };
}