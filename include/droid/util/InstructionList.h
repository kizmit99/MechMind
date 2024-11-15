#pragma once
#include <Arduino.h>
#include "droid/services/System.h"

#define INSTRUCTIONLIST_DEVICE_LEN  20
#define INSTRUCTIONLIST_COMMAND_LEN 50
#define INSTRUCTIONLIST_QUEUE_SIZE  20

namespace droid::util {

    struct Instruction {
        char device[INSTRUCTIONLIST_DEVICE_LEN] = {0};
        char command[INSTRUCTIONLIST_COMMAND_LEN] = {0};
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
        Instruction* getNext(Instruction*);
        void dump(const char *name, droid::services::Logger* logger);

    private:
        Instruction list[INSTRUCTIONLIST_QUEUE_SIZE];
        Instruction* head = 0;
        Instruction* tail = 0;
    };
}
