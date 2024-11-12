#pragma once
#include <Arduino.h>
#include "droid/services/System.h"
#include "droid/controller/Controller.h"
#include "droid/command/CmdHandler.h"
#include <map>
#include <vector>

#define MAX_DEVICE_LEN 20
#define MAX_COMMAND_LEN 50
#define MAX_SEQUENCE_LEN 200
#define INSTRUCTION_QUEUE_SIZE 20

namespace droid::brain {
    struct Instruction {
        char device[MAX_DEVICE_LEN];
        char command[MAX_COMMAND_LEN];
        unsigned long executeTime; // Time when the instruction should be executed
        bool isActive;
        Instruction* next;
        Instruction* prev;
    };

    class InstructionList {
    public:
        Instruction* addInstruction();
        Instruction* deleteInstruction(Instruction*);  //Note, this method returns the NEXT Instruction* in the list
        Instruction* initLoop();
        // Instruction* getNext(Instruction*);
        // void dump(const char *name, droid::services::Logger* logger);

    private:
        Instruction list[INSTRUCTION_QUEUE_SIZE];
        Instruction* head = 0;
        Instruction* tail = 0;
    };

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
        InstructionList instructionList;
        std::vector<droid::command::CmdHandler*> cmdHandlers;

        void parseCommands(const char* command);
        void executeCommands();
    };
}