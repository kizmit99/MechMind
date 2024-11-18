#pragma once
#include <Arduino.h>
#include "droid/services/ActiveComponent.h"
#include "droid/controller/Controller.h"
#include "droid/command/CmdHandler.h"
#include "droid/util/InstructionList.h"
#include <map>
#include <vector>

#define ACTION_MAX_SEQUENCE_LEN 200

namespace droid::command {
    class ActionMgr : public droid::services::ActiveComponent {
    public:
        ActionMgr(const char* name, droid::services::System* system, droid::controller::Controller*);

        //Override virtual methods from ActiveComponent
        void init() override;
        void factoryReset() override;
        void task() override;
        void logConfig() override;
        void failsafe() override;

        void addCmdHandler(droid::command::CmdHandler*);
        void fireTrigger(const char* trigger);
        void queueCommand(const char* device, const char* command, unsigned long executeTime);
        void overrideCmdMap(const char* trigger, const char* cmd);

    private:
        droid::controller::Controller* controller;
        std::map<String, String> cmdMap;
        unsigned long lastTriggerTime = 0;
        String lastTrigger;
        droid::util::InstructionList instructionList;
        std::vector<droid::command::CmdHandler*> cmdHandlers;

        void parseCommands(const char* command);
        void executeCommands();
    };
}