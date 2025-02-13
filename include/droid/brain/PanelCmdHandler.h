/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/MechMind
 */

#pragma once
#include "droid/command/CmdHandler.h"
#include "settings/hardware.config.h"

namespace droid::brain {
    class PanelCmdHandler : public droid::command::CmdHandler {
    public:
        PanelCmdHandler(const char* name, droid::core::System* system);
        bool process(const char* device, const char* command);

        //Override default methods from CmdHandler
        void init() override;
        void factoryReset() override;
        void task() override;
        void logConfig() override;
        void failsafe() override;

    private:
        struct {
            uint16_t openMicroSeconds = 0;
            uint16_t closeMicroSeconds = 0;
            uint16_t timeMilliSeconds = 0;
            uint8_t pwmOutput = 0;
            bool isOpen = false;
        } panelDetails[LOCAL_PANEL_COUNT];
    };
}