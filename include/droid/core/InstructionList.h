/*
 * Droid Brain Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#pragma once
#include "droid/core/System.h"

#define INSTRUCTIONLIST_DEVICE_LEN  20
#define INSTRUCTIONLIST_COMMAND_LEN 50
#define INSTRUCTIONLIST_QUEUE_SIZE  20

namespace droid::core {

    struct Instruction {
        char device[INSTRUCTIONLIST_DEVICE_LEN] = {0};
        char command[INSTRUCTIONLIST_COMMAND_LEN] = {0};
        unsigned long executeTime = 0; // Time when the instruction should be executed
        bool isActive = 0;
        Instruction* next = NULL;
        Instruction* prev = NULL;
    };

    class InstructionList {
    public:
        Instruction* addInstruction();
        Instruction* deleteInstruction(Instruction*);  //Note, this method returns the NEXT Instruction* in the list
        Instruction* initLoop();
        Instruction* getNext(Instruction*);
        void dump(const char *name, Logger* logger, LogLevel level);

    private:
        Instruction list[INSTRUCTIONLIST_QUEUE_SIZE];
        Instruction* head = NULL;
        Instruction* tail = NULL;
    };
}
