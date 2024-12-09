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
    class LocalCmdHandler : public droid::command::CmdHandler {
    public:
        LocalCmdHandler(const char* name, droid::core::System* system, Brain* brain, Stream* console);
        bool process(const char* device, const char* command);

    private:
        Brain* brain;
        Stream* console;

        void printHelp();
        void printCmdHelp(const char* cmdName, const char* cmdDescription);
        void printParmHelp(const char* parmName, const char* parmDescription);
    };
}