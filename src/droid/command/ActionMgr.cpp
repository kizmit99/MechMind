/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/MechMind
 */

#include "droid/command/ActionMgr.h"

namespace droid::command {
    ActionMgr::ActionMgr(const char* name, droid::core::System* system, droid::controller::Controller* controller) :
        BaseComponent(name, system),
        controller(controller) {}

    void ActionMgr::init() {
        //init cmdMap with defaults then load overrides from config
        cmdMap.clear();
        #include "settings/Action.map"
        // Iterate through the map looking for overrides
        for (const auto& mapEntry : cmdMap) {
            const char* action = mapEntry.first.c_str();
            const char* cmd = mapEntry.second.c_str();
            String override = config->getString(name, action, cmd);
            if (override != cmd) {
                cmdMap[action] = override;
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
        #include "settings/Action.map"
        // Iterate through the map clearing all overrides
        for (const auto& mapEntry : cmdMap) {
            const char* action = mapEntry.first.c_str();
            const char* cmd = mapEntry.second.c_str();
            config->putString(name, action, cmd);
        }
    }

    void ActionMgr::failsafe() {
        //NOOP
    }

    void ActionMgr::overrideCmdMap(const char* action, const char* cmd) {
        if (action) {
            if (cmd == NULL) {  //Revert to default
                std::map<String, String> cmdMap;
                //Load defaults into a temp map
                #include "settings/Action.map"
                //override master map with the default value just loaded
                this->cmdMap[action] = cmdMap[action];
                //save the default back to config storage
                config->putString(name, action, this->cmdMap[action].c_str());
            } else {
                //New cmd is specified, use it
                cmdMap[action] = cmd;
                config->putString(name, action, cmd);
            }
        }
    }

    void ActionMgr::logConfig() {
        // Iterate through the cmdMap for keys to log
        for (const auto& mapEntry : cmdMap) {
            const char* action = mapEntry.first.c_str();
            logger->log(name, INFO, "Config %s = %s\n", action, config->getString(name, action, "").c_str());
        }
    }

    void ActionMgr::fireAction(const char* action) {
        if (cmdMap.count(action) > 0) {
            parseCommands(cmdMap[action].c_str());
        } else {
            logger->log(name, DEBUG, "Action (%s) not recognized, trying to parse as a command\n", action);
            parseCommands(action);
        }
    }

    void ActionMgr::task() {
        String action = controller->getAction();
        unsigned long now = millis();
        if ((action == lastAction) &&
            (now < (lastActionTime + 1000))) {
            //Skip it
        } else {
            lastActionTime = now;
            lastAction = action;
            if (action != "") {
                logger->log(name, DEBUG, "Trigger: %s, Cmd: %s\n", action.c_str(), cmdMap[action].c_str());
                fireAction(action.c_str());
            }
        }
        executeCommands();
    }

    // Parse the command sequence and store commands with their execute times
    void ActionMgr::parseCommands(const char* sequence) {
        char buf[ACTION_MAX_SEQUENCE_LEN];
        strncpy(buf, sequence, sizeof(buf));  // Create a copy of the sequence to avoid modifying the original
        char* token = strtok(buf, ";");
        char currentDevice[INSTRUCTIONLIST_DEVICE_LEN] = "";
        unsigned long currentTime = millis();
        unsigned long cumulativeDelay = 0;

        while (token != NULL) {
            char* greaterPos = strchr(token, '>');
            if (greaterPos != NULL) {
                // It's a device token
                *greaterPos = '\0';  // Null-terminate the device name
                strncpy(currentDevice, token, sizeof(currentDevice));
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
    }

    void ActionMgr::queueCommand(const char* device, const char* command, unsigned long executeTime) {
        droid::core::Instruction* newInstruction = instructionList.addInstruction();
        if (newInstruction == NULL) {
            logger->log(name, WARN, "Command Queue is FULL, dropping command: %s\n", command);
            instructionList.dump(name, logger, WARN);
            return;
        }
        strncpy(newInstruction->device, device, INSTRUCTIONLIST_DEVICE_LEN);
        strncpy(newInstruction->command, command, INSTRUCTIONLIST_COMMAND_LEN);
        newInstruction->executeTime = executeTime;
    }

    // Execute commands at the proper times
    void ActionMgr::executeCommands() {
        unsigned long currentTime = millis();
        droid::core::Instruction* instruction = instructionList.initLoop();
        while (instruction != NULL) {
            if (currentTime >= instruction->executeTime) {

                logger->log(name, DEBUG, "Sending command to %s: %s at time: %d\n", instruction->device, instruction->command, currentTime);

                bool consumed = false;
                for (droid::command::CmdHandler* cmdHandler : cmdHandlers) {
                    consumed |= cmdHandler->process(instruction->device, instruction->command);
                    if (consumed) {
                        break;
                    }
                }
                if (!consumed) {
                    logger->log(name, WARN, "Command was not handled.  Device: %s, cmd: %s\n", instruction->device, instruction->command);
                }

                instruction = instructionList.deleteInstruction(instruction);
            } else {
                instruction = instruction->next;
            }
        }
    }
}