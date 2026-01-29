/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/MechMind
 */

#pragma once
#include "droid/command/CmdHandler.h"
#include "droid/brain/Brain.h"

namespace droid::brain {
    /**
     * Handles Category B commands (runtime state) triggered programmatically
     * via ActionMgr from controller events or automation.
     * 
     * Category A commands (admin/provisioning) are handled by ConsoleHandler.
     */
    class LocalCmdHandler : public droid::command::CmdHandler {
    public:
        LocalCmdHandler(const char* name, droid::core::System* system, Brain* brain);
        bool process(const char* device, const char* command);

    private:
        Brain* brain = nullptr;
    };
}