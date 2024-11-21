#include "droid/brain/LocalCmdHandler.h"
#include "droid/services/DroidState.h"
#include "droid/command/ActionMgr.h"

namespace {
    const char* trimSpaces(const char* str) {
        while (isspace(*str)) {
            str++;
        }
        return str;
    }

    void parseCmd(const char* input, char* cmdBuf, int cmdBufLen, char* parm1Buf, int parm1BufLen, char* parm2Buf, int parm2BufLen) {
        char temp[256];
        strncpy(temp, input, sizeof(temp));
        temp[sizeof(temp) - 1] = '\0';

        char* token = strtok(temp, " ");
        if (token) {
            strncpy(cmdBuf, token, cmdBufLen);
            cmdBuf[cmdBufLen - 1] = '\0';
        }

        token = strtok(NULL, " ");
        if (token) {
            strncpy(parm1Buf, token, parm1BufLen);
            parm1Buf[parm1BufLen - 1] = '\0';
        } else {
            parm1Buf[0] = '\0';
            parm2Buf[0] = '\0';
            return;
        }

        token = strtok(NULL, "");
        if (token) {
            strncpy(parm2Buf, trimSpaces(token), parm2BufLen);
            parm2Buf[parm2BufLen - 1] = '\0';
        } else {
            parm2Buf[0] = '\0';
        }
    }

    void split(const char* input, char* part1Buf, int part1BufLen, char* part2Buf, int part2BufLen) {
        const char* firstSpace = strnstr(input, " ", part1BufLen);
        if (!firstSpace) { // No space found, part1 is the whole string and part2 is empty
            strncpy(part1Buf, input, part1BufLen);
            part1Buf[part1BufLen - 1] = '\0';
            part2Buf[0] = '\0';
            return;
        }

        // Copy part1
        size_t len1 = firstSpace - input;
        if (len1 > part1BufLen) {
            len1 = part1BufLen - 1;
        }
        strncpy(part1Buf, input, len1);
        part1Buf[len1] = '\0';

        // Find the start of part2
        const char* remaining = trimSpaces(firstSpace);
        strncpy(part2Buf, remaining, part2BufLen);
        part2Buf[part2BufLen - 1] = '\0';
    }   
}

namespace droid::brain {
    LocalCmdHandler::LocalCmdHandler(const char* name, droid::core::System* system, Brain* brain) :
        CmdHandler(name, system),
        brain(brain) {}

    bool LocalCmdHandler::process(const char* device, const char* command) {
        char cmd[ACTION_MAX_SEQUENCE_LEN] = {0};
        char parm1[ACTION_MAX_SEQUENCE_LEN] = {0};
        char parm2[ACTION_MAX_SEQUENCE_LEN] = {0};
        char parm2a[ACTION_MAX_SEQUENCE_LEN] = {0};
        char parm3[ACTION_MAX_SEQUENCE_LEN] = {0};

        if ((strcasecmp(name, device) == 0) &&
            (command != NULL)) {
            logger->log(name, DEBUG, "LocalCmdHandler asked to processcommand: %s\n", command);
            parseCmd(command, cmd, sizeof(cmd), parm1, sizeof(parm1), parm2, sizeof(parm2));
            if (strcasecmp(cmd, "StickOn") == 0) {
                droidState->stickEnable = true;

            } else if (strcasecmp(cmd, "StickOff") == 0) {
                droidState->stickEnable = false;

            } else if (strcasecmp(cmd, "StickToggle") == 0) {
                droidState->stickEnable = !droidState->stickEnable;

            } else if (strcasecmp(cmd, "SpeedToggle") == 0) {
                droidState->turboSpeed = !droidState->turboSpeed;

            } else if (strcasecmp(cmd, "AutoDomeOn") == 0) {
                droidState->autoDomeEnable = true;

            } else if (strcasecmp(cmd, "AutoDomeOff") == 0) {
                droidState->autoDomeEnable = false;

            } else if (strcasecmp(cmd, "AutoDomeToggle") == 0) {
                droidState->autoDomeEnable = !droidState->autoDomeEnable;

            } else if (strcasecmp(cmd, "Restart") == 0) {
                //Restart the controller.
                brain->reboot();

            } else if (strcasecmp(cmd, "FactoryReset") == 0) {
                //Delete all preferences, reset button triggers to sketch defaults, unpair controllers.
                logger->log(name, WARN, "Initiating factory reset...\n");
                brain->factoryReset();
                brain->reboot();

            } else if (strcasecmp(cmd, "ListConfig") == 0) {
                //List all configuration data
                brain->logConfig();

            } else if (strcasecmp(cmd, "SetTrigger") == 0) {
                //Set the button trigger (parm1) to the specified action (parm2).
                brain->overrideCmdMap(parm1, parm2);

            } else if (strcasecmp(cmd, "Play") == 0) {
                //Play any action associated with the specified trigger or cmd string
                //Note that you cannot directly play cmd strings that contain spaces!
                brain->trigger(parm1);

            } else if (strcasecmp(cmd, "ResetTrigger") == 0) {
                //Reset command for specified trigger to default.
                brain->overrideCmdMap(parm1, NULL);

            } else if (strcasecmp(cmd, "SetConfig") == 0) {
                //This has 3 parms, so have to reparse parm2
                split(parm2, parm2a, sizeof(parm2a), parm3, sizeof(parm3));
                if (strlen(parm1) > 15) {
                    logger->log(name, WARN, "Invalid config-name (%s) is longer than 15 characters\n", parm1);
                } else if (strlen(parm1) == 0) {
                    logger->log(name, WARN, "Invalid config-name must be specified\n");
                } else if (strlen(parm2a) > 15) {
                    logger->log(name, WARN, "Invalid config-key (%s) is longer than 15 characters\n", parm2a);
                } else if (strlen(parm2a) == 0) {
                    logger->log(name, WARN, "Invalid config-key must be specified\n");
                } else {
                    logger->log(name, DEBUG, "SetConfig Name: '%s', Key: '%s', Value: '%s'\n",parm1, parm2a, parm3);
                    config->putString((const char*) &parm1, (const char*) &parm2a, (const char*) &parm3);
                }

            } else if ((strcasecmp(cmd, "Help") == 0) ||
                       (strcasecmp(cmd, "?") == 0)) {
                //Provide help on using Local Commands
                printHelp();

            } else {
                logger->log(name, WARN, "LocalCmdHandler asked to process an undefined command: %s\n", command);
            }
            return true;
        } else {
            return false;
        }
    }

    void LocalCmdHandler::printHelp() {
        logger->printf(name, FORCE, "\n");
        logger->printf(name, FORCE, "Commands:");
        printCmdHelp("Help or ?", "Print this list of commands");
        printCmdHelp("StickOn", "Enable the Drive joystick");
        printCmdHelp("StickOff", "Disable the Drive joystick");
        printCmdHelp("StickToggle", "Toggle the enabled state of the Drive joystick");
        printCmdHelp("AutoDomeOn", "Enable the Auto Dome functionality");
        printCmdHelp("AutoDomeOff", "Disable the Auto Dome functionality");
        printCmdHelp("AutoDomeToggle", "Toggle the enabled state of the Auto Dome Functionality");
        printCmdHelp("Restart", "Perform a complete system restart");
        printCmdHelp("FactoryReset", "Restore all configuration parameters to defaults and restart the system");
        printCmdHelp("ListConfig", "Print out all of the configuration parameters");
        printCmdHelp("SetTrigger <trigger> <action>", "Configure the Action associated with the specified Trigger");
        printParmHelp("trigger", "The Trigger to override");
        printParmHelp("action", "The new Action (list of instructions) to associate with the Trigger");
        printCmdHelp("Play <command>", "Execute the specified Trigger or Action");
        printParmHelp("command", "This can be a Trigger, or a list of instructions (see ListConfig for examples)");
        printCmdHelp("ResetTrigger <trigger>", "Restore the default Action associated with the specified Trigger");
        printParmHelp("trigger", "The Trigger to override");
        printCmdHelp("SetConfig <anmespace> <key> <newValue>", "Update the specified configuration value");
        printParmHelp("namespace", "The namespace of the configuration entry to update (see ListConfig for valid options)");
        printParmHelp("key", "The key name of the configuration entry to update (see ListConfig for valid options)");
        printParmHelp("newValue", "The new value for the specified configuration namespace/key");
        logger->printf(name, FORCE, "\n");
    }

    void LocalCmdHandler::printCmdHelp(const char* cmdName, const char* cmdDescription) {
        logger->printf(name, FORCE, "\n");
        logger->printf(name, FORCE, "  %s\n", cmdName);
        logger->printf(name, FORCE, "    %s\n", cmdDescription);
    }

    void LocalCmdHandler::printParmHelp(const char* parmName, const char* parmDescription) {
        logger->printf(name, FORCE, "    -- %s: %s\n", parmName, parmDescription);
    }
}