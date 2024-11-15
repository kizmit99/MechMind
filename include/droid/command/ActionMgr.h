#pragma once
#include <Arduino.h>
#include "droid/services/System.h"
#include "droid/controller/Controller.h"
#include "droid/command/CmdHandler.h"
#include "droid/util/InstructionList.h"
#include <map>
#include <vector>

#define ACTION_MAX_SEQUENCE_LEN 200

namespace droid::command {
    class ActionMgr {
    public:
        ActionMgr(const char* name, droid::services::System* system, droid::controller::Controller*);
        void init();
        void task();
        void factoryReset();
        void logConfig();
        void addCmdHandler(droid::command::CmdHandler*);
        void fireTrigger(const char* trigger);
        void queueCommand(const char* device, const char* command, unsigned long executeTime);
        void overrideCmdMap(const char* trigger, const char* cmd);

    private:
        const char* name;
        droid::controller::Controller* controller;
        droid::services::Logger* logger;
        droid::services::Config* config;
        std::map<String, String> cmdMap;
        unsigned long lastTriggerTime = 0;
        String lastTrigger;
        droid::util::InstructionList instructionList;
        std::vector<droid::command::CmdHandler*> cmdHandlers;

        void parseCommands(const char* command);
        void executeCommands();
    };
}