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
#include "droid/core/PassiveComponent.h"

namespace droid::command {
    class CmdHandler : public droid::core::PassiveComponent {
    public:
        CmdHandler(const char* name, droid::core::System* system) :
            PassiveComponent(name, system) {}

        virtual bool process(const char* device, const char* command) = 0;
    };
}