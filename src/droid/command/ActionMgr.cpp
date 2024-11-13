#include "droid/command/ActionMgr.h"
#include <map>

namespace droid::command {
    ActionMgr::ActionMgr(const char* name, droid::services::System* system, droid::controller::Controller* controller) :
        name(name),
        logger(system->getLogger()),
        config(system->getConfig()),
        controller(controller) {}

    void ActionMgr::init() {
        //init cmdMap with defaults then load overrides from config
        cmdMap.clear();
        #include "droid/command/Trigger.map"
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

    void ActionMgr::addCmdHandler(droid::command::CmdHandler* cmdHandler) {
        if (cmdHandler) {
            cmdHandlers.push_back(cmdHandler);
        }
    }

    void ActionMgr::factoryReset() {
        //init cmdMap with defaults then store default into config
        cmdMap.clear();
        #include "droid/command/Trigger.map"
        // Iterate through the map clearing all overrides
        for (const auto& mapEntry : cmdMap) {
            const char* trigger = mapEntry.first.c_str();
            const char* cmd = mapEntry.second.c_str();
            config->putString(name, trigger, cmd);
        }
    }

    void ActionMgr::overrideCmdMap(const char* trigger, const char* cmd) {
        if (trigger) {
            if (cmd == NULL) {  //Revert to default
                std::map<String, String> cmdMap;
                //Load defaults into a temp map
                #include "droid/command/Trigger.map"
                //override master map with the default value just loaded
                this->cmdMap[trigger] = cmdMap[trigger];
                //save the default back to config storage
                config->putString(name, trigger, this->cmdMap[trigger].c_str());
            } else {
                //New cmd is specified, use it
                cmdMap[trigger] = cmd;
                config->putString(name, trigger, cmd);
            }
        }
    }

    void ActionMgr::logConfig() {
        // Iterate through the cmdMap for keys to log
        for (const auto& mapEntry : cmdMap) {
            const char* trigger = mapEntry.first.c_str();
            logger->log(name, INFO, "Config %s = %s\n", trigger, config->getString(name, trigger, "").c_str());
        }
    }

    void ActionMgr::fireTrigger(const char* trigger) {
        if (cmdMap.count(trigger) > 0) {
            parseCommands(cmdMap[trigger].c_str());
        } else {
            logger->log(name, DEBUG, "Trigger (%s) not recognized, trying to parse as a command\n", trigger);
            parseCommands(trigger);
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
                logger->log(name, DEBUG, "Trigger: %s, Cmd: %s\n", trigger, cmdMap[trigger].c_str());
                fireTrigger(trigger.c_str());
            }
        }
        executeCommands();
    }

    // Parse the command sequence and store commands with their execute times
    void ActionMgr::parseCommands(const char* sequence) {
        // instructionList.dump("parseBegin", logger);
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
                queueCommand(currentDevice, token, currentTime + cumulativeDelay);
                logger->log(name, DEBUG, "parsed device: %s, cmd: %s\n",currentDevice, token);
            }
            token = strtok(NULL, ";");
        }
        // instructionList.dump("parseEnd", logger);
    }

    void ActionMgr::queueCommand(const char* device, const char* command, unsigned long executeTime) {
        Instruction* newInstruction = instructionList.addInstruction();
        strncpy(newInstruction->device, device, MAX_DEVICE_LEN);
        strncpy(newInstruction->command, command, MAX_COMMAND_LEN);
        newInstruction->executeTime = executeTime;
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

                bool processed = false;
                for (droid::command::CmdHandler* cmdHandler : cmdHandlers) {
                    processed |= cmdHandler->process(instruction->device, instruction->command);
                }
                if (!processed) {
                    logger->log(name, WARN, "Command was not handled.  Device: %s, cmd: %s\n", instruction->device, instruction->command);
                }

                instruction = instructionList.deleteInstruction(instruction);
            } else {
                instruction = instruction->next;
            }
        }
    }

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
        for (index = 0; index < sizeof(list); index++) {
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