#pragma once
#include "droid/command/CmdHandler.h"
#include "droid/core/PassiveComponent.h"
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