#pragma once
#include <Arduino.h>
#include "droid/services/System.h"
#include "droid/controller/Controller.h"
#include <map>

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
        void deleteInstruction(Instruction*);
        Instruction* initLoop();
        bool hasNext(Instruction*);
        Instruction* getNext(Instruction*);

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

    private:
        const char* name;
        droid::controller::Controller* controller;
        droid::services::Logger* logger;
        droid::services::Config* config;
        std::map<String, String> cmdMap;
        unsigned long lastTriggerTime = 0;
        String lastTrigger;
        InstructionList instructionList;

        void parseCommands(const char* command);
        void executeCommands();
    };
}