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
#include "droid/network/MechNetNode.h"

namespace droid::command {
    class MechNetCmdHandler : public CmdHandler {
    public:
        MechNetCmdHandler(const char* name, droid::core::System* system, 
                               droid::network::MechNetNode* masterNode);

        bool process(const char* device, const char* command) override;

    private:
        droid::network::MechNetNode* mechNetMasterNode;
    };
}
