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
#include "CmdHandler.h"

namespace droid::command {
    class CmdLogger : public CmdHandler {
    public:
        CmdLogger(const char* name, droid::core::System* sys);
        bool process(const char* device, const char* command);

    private:
        Stream* stream = nullptr;
    };
}