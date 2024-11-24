/*
 * Droid Brain Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#pragma once
#include "droid/core/BaseComponent.h"

namespace droid::command {
    class CmdHandler : public droid::core::BaseComponent {
    public:
        CmdHandler(const char* name, droid::core::System* system) :
            BaseComponent(name, system) {}

        //Virtual methods required by BaseComponent declared here as NOOPs for concrete sub-classes
        void init() {}
        void factoryReset() {}
        void task() {}
        void logConfig() {}
        void failsafe() {}

        virtual bool process(const char* device, const char* command) = 0;
    };
}