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
#include "droid/core/BaseComponent.h"
#include "droid/controller/Controller.h"
#include "droid/command/CmdHandler.h"
#include "droid/core/InstructionList.h"
#include <map>
#include <vector>

#define ACTION_MAX_SEQUENCE_LEN 200

namespace droid::command {
    class ActionMgr : public droid::core::BaseComponent {
    public:
        ActionMgr(const char* name, droid::core::System* system, droid::controller::Controller*);

        //Override virtual methods from BaseComponent
        void init() override;
        void factoryReset() override;
        void task() override;
        void logConfig() override;
        void failsafe() override;

        void addCmdHandler(droid::command::CmdHandler*);
        void fireAction(const char* action);
        void queueCommand(const char* device, const char* command, unsigned long executeTime);
        void overrideCmdMap(const char* action, const char* cmd);

    private:
        droid::controller::Controller* controller = nullptr;
        std::map<String, String> cmdMap;
        unsigned long lastActionTime = 0;
        String lastAction = "";
        droid::core::InstructionList instructionList;
        std::vector<droid::command::CmdHandler*> cmdHandlers;

        void parseCommands(const char* command);
        void executeCommands();
    };
}