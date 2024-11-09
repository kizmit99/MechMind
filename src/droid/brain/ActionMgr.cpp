#include "droid/brain/ActionMgr.h"
#include <map>

namespace droid::brain {
    ActionMgr::ActionMgr(const char* name, droid::services::System* system, droid::controller::Controller* controller) :
        name(name),
        logger(system->getLogger()),
        config(system->getConfig()),
        controller(controller) {}

    void ActionMgr::init() {
        //init cmdMap with defaults then load overrides from config
        cmdMap.clear();
        #include "droid/brain/Trigger.map"
        // Iterate through the map looking for overrides
        for (const auto& mapEntry : cmdMap) {
            const char* trigger = mapEntry.first.c_str();
            const char* cmd = mapEntry.second.c_str();
            String override = config->getString(name, trigger, cmd);
            if (override != cmd) {
                cmdMap[trigger] = override;
            }
        }
    }

    void ActionMgr::factoryReset() {
        //init cmdMap with defaults then store default into config
        cmdMap.clear();
        #include "droid/brain/Trigger.map"
        // Iterate through the map clearing all overrides
        for (const auto& mapEntry : cmdMap) {
            const char* trigger = mapEntry.first.c_str();
            const char* cmd = mapEntry.second.c_str();
            config->putString(name, trigger, cmd);
        }
    }

    void ActionMgr::logConfig() {
        // Iterate through the cmdMap for keys to log
        for (const auto& mapEntry : cmdMap) {
            const char* trigger = mapEntry.first.c_str();
            logger->log(name, INFO, "Config %s = %s\n", trigger, config->getString(name, trigger, "").c_str());
        }
    }

    void ActionMgr::task() {
        String trigger = controller->getTrigger();
        unsigned long now = millis();
        if ((trigger == lastTrigger) &&
            (now < (lastTriggerTime + 1000))) {
            //Skip it
        } else {
            lastTriggerTime = now;
            lastTrigger = trigger;
            if (trigger != "") {
                logger->log(name, DEBUG, "Trigger: %s, Cmd: %s\n", trigger, cmdMap[trigger]);
                parseCommands(cmdMap[trigger].c_str());
            }
        }
        executeCommands();
    }

    // Parse the command sequence and store commands with their execute times
    void ActionMgr::parseCommands(const char* sequence) {
        char buf[MAX_SEQUENCE_LEN];
        strncpy(buf, sequence, sizeof(buf));  // Create a copy of the sequence to avoid modifying the original
        char* token = strtok(buf, ";");
        char currentDevice[MAX_DEVICE_LEN] = "";
        unsigned long currentTime = millis();
        unsigned long cumulativeDelay = 0;

        while (token != NULL) {
            char* greaterPos = strchr(token, '>');
            if (greaterPos != NULL) {
                // It's a device token
                *greaterPos = '\0';  // Null-terminate the device name
                strncpy(currentDevice, token, sizeof(currentDevice));
                cumulativeDelay = 0;
                token = greaterPos + 1;
                continue;
            }
            if (token[0] == '#') {
                // It's a delay
                unsigned long delay = strtoul(token + 1, NULL, 10);
                cumulativeDelay += delay;
            } else {
                // It's a simple or panel instruction
                Instruction* newInstruction = instructionList.addInstruction();
                strncpy(newInstruction->device, currentDevice, MAX_DEVICE_LEN);
                strncpy(newInstruction->command, token, MAX_COMMAND_LEN);
                newInstruction->executeTime = currentTime + cumulativeDelay;
            }
            token = strtok(NULL, ";");
        }
    }

    // Execute commands at the proper times
    void ActionMgr::executeCommands() {
        unsigned long currentTime = millis();
        Instruction* instruction = instructionList.initLoop();
        while (instruction != NULL) {
            if (currentTime >= instruction->executeTime) {
                Serial.print("Sending command to ");
                Serial.print(instruction->device);
                Serial.print(": ");
                Serial.print(instruction->command);
                Serial.print(" at time: ");
                Serial.println(currentTime);

                //TODO Replace this with actual command sending logic
                instructionList.deleteInstruction(instruction);
            }
            instruction = instruction->next;
        }
    }

    Instruction* InstructionList::addInstruction() {
        Instruction* freeRec = NULL;
        uint8_t index = 0;
        for (index = 0; index < sizeof(list); index++) {
            if (!list[index].isActive) {
                freeRec = &list[index];
                break;
            }
        }
        if (freeRec == NULL) {  //List is full
            return NULL;
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

    void InstructionList::deleteInstruction(Instruction* entry) {
        if (entry == NULL) {
            return;
        }
        if (head == NULL) {
            return;
        }
        if (entry->prev == NULL) {   //Was first entry in list
            head = entry->next;
        } else {
            entry->prev->next = entry->next;
        }
        if (tail == entry) {    //Was last entry in list
            tail = entry->prev;
        }
        //Clear record for reuse
        entry->command[0] = 0;
        entry->device[0] = 0;
        entry->executeTime = 0;
        entry->isActive = false;
        entry->next = NULL;
        entry->prev = NULL;
    }

    Instruction* InstructionList::initLoop() {
        return head;
    }

    bool InstructionList::hasNext(Instruction* entry) {
        if (entry == NULL) {
            return false;
        }
        return (entry->next != NULL);
    }

    Instruction* InstructionList::getNext(Instruction* entry) {
        if (entry == NULL) {
            return NULL;
        }
        return entry->next;
    }



// // =======================================================================================
// // This function handles the processing of custom MarcDuino panel routines
// // =======================================================================================
// inline void custMarcDuinoPanel()
// {
//     if (!sRunningCustRoutine)
//         return;
//     sRunningCustRoutine = false;
//     for (int i = 0; i < SizeOfArray(sPanelStatus); i++)
//     {
//         PanelStatus &panel = sPanelStatus[i];
//         if (panel.fStatus == 1)
//         {
//             if (panel.fStartTime + panel.fStartDelay * 1000 < millis())
//             {
//                 char cmd[10];
//                 snprintf(cmd, sizeof(cmd), ":OP%02d", i+1);
//                 sendMarcCommand(cmd);
//                 panel.fStatus = 2;
//             }
//         }
//         else if (panel.fStatus == 2)
//         {
//             if (panel.fStartTime + (panel.fStartDelay + panel.fDuration) * 1000 < millis())
//             {
//                 char cmd[10];
//                 snprintf(cmd, sizeof(cmd), ":CL%02d", i+1);
//                 sendMarcCommand(cmd);
//                 panel.fStatus = 0;
//             }
//         }
//         if (panel.fStatus != 0)
//             sRunningCustRoutine = true;
//     }
// }

}