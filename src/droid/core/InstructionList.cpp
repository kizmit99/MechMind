/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#include "droid/core/InstructionList.h"

namespace droid::core {

    void InstructionList::dump(const char* name, Logger* logger, LogLevel level) {
        int i = 0;
        Instruction* instruction = head;
        while (instruction != NULL) {
            logger->log(name, level, "InstructionList[%d] device: %s, cmd: %s\n", i, instruction->device, instruction->command);
            i++;
            instruction = instruction->next;
        }
    }

    Instruction* InstructionList::addInstruction() {
        Instruction* freeRec = NULL;
        uint8_t index;
        for (index = 0; index < INSTRUCTIONLIST_QUEUE_SIZE; index++) {
            if (!list[index].isActive) {
                freeRec = &list[index];
                break;
            }
        }
        if (freeRec == NULL) {  //List is full
            return NULL;
        }
        //link prev record to this one
        if (tail != NULL) {
            tail->next = freeRec;
        }
        //Prepare record for reuse
        freeRec->isActive = true;
        freeRec->prev = tail;       //Always add to end of list
        freeRec->next = NULL;
        tail = freeRec;
        if (head == NULL) {   //List was empty
            head = freeRec;
        }
        return freeRec;
    }

    Instruction* InstructionList::deleteInstruction(Instruction* entry) {
        if (entry == NULL) {
            return NULL;
        }
        if (head == NULL) {
            return NULL;
        }
        if (entry->prev == NULL) {   //Was first entry in list
            head = entry->next;
        } else {
            entry->prev->next = entry->next;
        }
        if (tail == entry) {    //Was last entry in list
            tail = entry->prev;
        }
        Instruction* next = entry->next;
        if (next != NULL) {
            next->prev = entry->prev;
        }

        //Clear record for reuse
        entry->command[0] = 0;
        entry->device[0] = 0;
        entry->executeTime = 0;
        entry->isActive = false;
        entry->next = NULL;
        entry->prev = NULL;

        return next;
    }

    Instruction* InstructionList::initLoop() {
        return head;
    }

    Instruction* InstructionList::getNext(Instruction* entry) {
        if (entry == NULL) {
            return NULL;
        }
        return entry->next;
    }
}
