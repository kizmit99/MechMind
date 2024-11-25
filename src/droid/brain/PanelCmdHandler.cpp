/*
 * Droid Brain Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#include "droid/brain/PanelCmdHandler.h"
#include "droid/services/PWMService.h"

#define CONFIG_KEY_PANEL_OPEN_MICROSECONDS        "Panel%dOpen"
#define CONFIG_KEY_PANEL_CLOSE_MICROSECONDS       "Panel%dClose"
#define CONFIG_KEY_PANEL_TIME_MILLISECONDS        "Panel%dTime"
#define CONFIG_KEY_PANEL_PWMOUT                   "Panel%dPWMOut"
#define CONFIG_DEFAULT_PANEL_OPEN_MICROSECONDS    1800
#define CONFIG_DEFAULT_PANEL_CLOSE_MICROSECONDS   800
#define CONFIG_DEFAULT_PANEL_TIME_MILLISECONDS    20

namespace droid::brain {
    PanelCmdHandler::PanelCmdHandler(const char* name, droid::core::System* system) :
        CmdHandler(name, system) {}

    bool PanelCmdHandler::process(const char* device, const char* command) {
        droid::services::PWMService* pwmService = system->getPWMService();
        if ((strcmp(name, device)!= 0) ||
            (command == NULL) ||
            (pwmService == NULL) ||
            ((strlen(command) < 4) || (strlen(command) > 6))) {
            return false;
        }
        //Parse the command
        if (command[0] != ':') {return false;}
        int panel = atoi(&command[3]);
        if (panel > LOCAL_PANEL_COUNT) {return false;}
        uint8_t startIndex, endIndex = panel - 1;
        if (panel == 0) {
            startIndex = 0;
            endIndex = LOCAL_PANEL_COUNT - 1;
        }
        if (((command[1] == 'O') || (command[1] == 'o')) && 
            ((command[2] == 'P') || (command[1] == 'p'))) {
            for (uint8_t index = startIndex; index <= endIndex; index++) {
                pwmService->setPWMuS(panelDetails[index].pwmOutput, 
                                        panelDetails[index].openMicroSeconds, 
                                        panelDetails[index].timeMilliSeconds);
                panelDetails[index].isOpen = true;
            }
            return true;
        }
        if (((command[1] == 'C') || (command[1] == 'c')) && 
            ((command[2] == 'L') || (command[1] == 'l'))) {
            for (uint8_t index = startIndex; index <= endIndex; index++) {
                pwmService->setPWMuS(panelDetails[index].pwmOutput, 
                                        panelDetails[index].closeMicroSeconds, 
                                        panelDetails[index].timeMilliSeconds);
                panelDetails[index].isOpen = false;
            }
            return true;
        }
        return false;
    }

    void PanelCmdHandler::init() {
        char keyOpen[16];
        char keyClose[16];
        char keyTime[16];
        char keyPWM[16];
        for (int i = 0; i < LOCAL_PANEL_COUNT; i++) {
            snprintf(keyOpen, sizeof(keyOpen), CONFIG_KEY_PANEL_OPEN_MICROSECONDS, i+1);
            snprintf(keyClose, sizeof(keyClose), CONFIG_KEY_PANEL_CLOSE_MICROSECONDS, i+1);
            snprintf(keyTime, sizeof(keyTime), CONFIG_KEY_PANEL_TIME_MILLISECONDS, i+1);
            snprintf(keyPWM, sizeof(keyPWM), CONFIG_KEY_PANEL_PWMOUT, i+1);
            panelDetails[i].openMicroSeconds = config->getInt(name, keyOpen, CONFIG_DEFAULT_PANEL_OPEN_MICROSECONDS);
            panelDetails[i].closeMicroSeconds = config->getInt(name, keyClose, CONFIG_DEFAULT_PANEL_CLOSE_MICROSECONDS);
            panelDetails[i].timeMilliSeconds = config->getInt(name, keyTime, CONFIG_DEFAULT_PANEL_TIME_MILLISECONDS);
            panelDetails[i].pwmOutput = config->getInt(name, keyPWM, PWMSERVICE_PANEL_FIRST_OUT + i);
            panelDetails[i].isOpen = false;
        }
    }

    void PanelCmdHandler::factoryReset() {
        config->clear(name);
        char keyOpen[16];
        char keyClose[16];
        char keyTime[16];
        char keyPWM[16];
        for (int i = 0; i < LOCAL_PANEL_COUNT; i++) {
            snprintf(keyOpen, sizeof(keyOpen), CONFIG_KEY_PANEL_OPEN_MICROSECONDS, i+1);
            snprintf(keyClose, sizeof(keyClose), CONFIG_KEY_PANEL_CLOSE_MICROSECONDS, i+1);
            snprintf(keyTime, sizeof(keyTime), CONFIG_KEY_PANEL_TIME_MILLISECONDS, i+1);
            snprintf(keyTime, sizeof(keyPWM), CONFIG_KEY_PANEL_PWMOUT, i+1);
            config->putInt(name, keyOpen, CONFIG_DEFAULT_PANEL_OPEN_MICROSECONDS);
            config->putInt(name, keyClose, CONFIG_DEFAULT_PANEL_CLOSE_MICROSECONDS);
            config->putInt(name, keyTime, CONFIG_DEFAULT_PANEL_TIME_MILLISECONDS);
            config->putInt(name, keyPWM, PWMSERVICE_PANEL_FIRST_OUT + i);
        }
    }

    void PanelCmdHandler::task() {
        //TODO probably noop?  If so, remove from .h and here
        //Maybe flutter?
    }

    void PanelCmdHandler::logConfig() {
        char keyOpen[16];
        char keyClose[16];
        char keyTime[16];
        char keyPWM[16];
        for (int i = 0; i < LOCAL_PANEL_COUNT; i++) {
            snprintf(keyOpen, sizeof(keyOpen), CONFIG_KEY_PANEL_OPEN_MICROSECONDS, i+1);
            snprintf(keyClose, sizeof(keyClose), CONFIG_KEY_PANEL_CLOSE_MICROSECONDS, i+1);
            snprintf(keyTime, sizeof(keyTime), CONFIG_KEY_PANEL_TIME_MILLISECONDS, i+1);
            snprintf(keyPWM, sizeof(keyPWM), CONFIG_KEY_PANEL_PWMOUT, i+1);
            logger->log(name, INFO, "Config %s = %s\n", keyOpen, config->getString(name, keyOpen, "").c_str());
            logger->log(name, INFO, "Config %s = %s\n", keyClose, config->getString(name, keyClose, "").c_str());
            logger->log(name, INFO, "Config %s = %s\n", keyTime, config->getString(name, keyTime, "").c_str());
            logger->log(name, INFO, "Config %s = %s\n", keyPWM, config->getString(name, keyPWM, "").c_str());
        }
    }

    void PanelCmdHandler::failsafe() {
        droid::services::PWMService* pwmService = system->getPWMService();
        if (pwmService) {
            for (int i = 0; i < LOCAL_PANEL_COUNT; i++) {
                pwmService->setPWMpercent(panelDetails[i].pwmOutput, 0);
            }
        }
    }
}