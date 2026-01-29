/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/MechMind
 */

#include "droid/brain/LocalCmdHandler.h"
#include "droid/services/DroidState.h"
#include "droid/command/ActionMgr.h"
#include "settings/hardware.config.h"

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
            if (strcasecmp(cmd, "StickEnable") == 0) {
                droidState->stickEnable = true;

            } else if (strcasecmp(cmd, "StickDisable") == 0) {
                droidState->stickEnable = false;

            } else if (strcasecmp(cmd, "StickToggle") == 0) {
                droidState->stickEnable = !droidState->stickEnable;

            } else if (strcasecmp(cmd, "SpeedChange") == 0) {
                droidState->turboSpeed = !droidState->turboSpeed;

            } else if (strcasecmp(cmd, "DomeAutoOn") == 0) {
                droidState->autoDomeEnable = true;

            } else if (strcasecmp(cmd, "DomeAutoOff") == 0) {
                droidState->autoDomeEnable = false;

            } else if (strcasecmp(cmd, "DomeAutoToggle") == 0) {
                droidState->autoDomeEnable = !droidState->autoDomeEnable;

            } else if (strcasecmp(cmd, "DomePAllToggle") == 0) {
                if (droidState->domePanelsOpen) {
                    brain->fireAction("DomePAllClose");
                } else {
                    brain->fireAction("DomePAllOpen");
                }
                droidState->domePanelsOpen = !droidState->domePanelsOpen;

            } else if (strcasecmp(cmd, "HoloAutoToggle") == 0) {
                if (droidState->holosActive) {
                    brain->fireAction("HolosAutoOff");
                } else {
                    brain->fireAction("HolosAutoOn");
                }
                droidState->holosActive = !droidState->holosActive;

            } else if (strcasecmp(cmd, "HoloLightsTogl") == 0) {
                if (droidState->holoLightsActive) {
                    brain->fireAction("HoloLightsOff");
                } else {
                    brain->fireAction("HoloLightsOn");
                }
                droidState->holoLightsActive = !droidState->holoLightsActive;

            } else if (strcasecmp(cmd, "BodyPAllToggle") == 0) {
                if (droidState->bodyPanelsOpen) {
                    brain->fireAction("BodyPAllClose");
                } else {
                    brain->fireAction("BodyPAllOpen");
                }
                droidState->bodyPanelsOpen = !droidState->bodyPanelsOpen;

            } else if (strcasecmp(cmd, "MusingsToggle") == 0) {
                if (droidState->musingEnabled) {
                    brain->fireAction("MusingsOff");
                } else {
                    brain->fireAction("MusingsOn");
                }
                droidState->musingEnabled = !droidState->musingEnabled;

            } else if (strcasecmp(cmd, "Gesture") == 0) {
                //TODO

            } else {
                logger->log(name, WARN, "LocalCmdHandler asked to process an undefined command: %s\n", command);
            }
            return true;
        } else {
            return false;
        }
    }
}