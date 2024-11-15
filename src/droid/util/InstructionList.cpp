#include "droid/util/InstructionList.h"

namespace droid::util {

    void InstructionList::dump(const char* name, droid::services::Logger* logger) {
        int i = 0;
        Instruction* instruction = head;
        while (instruction != NULL) {
            logger->log(name, INFO, "InstructionList[%d] device: %s, cmd: %s\n", i, instruction->device, instruction->command);
            i++;
            instruction = instruction->next;
        }
    }

    Instruction* InstructionList::addInstruction() {
        Instruction* freeRec = NULL;
        uint8_t index = 0;
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