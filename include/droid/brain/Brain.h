/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#pragma once
#include "droid/core/BaseComponent.h"
#include "droid/services/NoPWMService.h"
#include "droid/services/PCA9685PWM.h"
#include "droid/controller/DualSonyMoveController.h"
#include "droid/controller/DualRingController.h"
#include "droid/controller/StubController.h"
#include "droid/motor/DRV8871Driver.h"
#include "droid/motor/SabertoothDriver.h"
#include "droid/brain/DomeMgr.h"
#include "droid/command/ActionMgr.h"
#include "droid/audio/AudioMgr.h"
#include "droid/audio/HCRDriver.h"
#include "droid/brain/DriveMgr.h"
#include "droid/brain/PanelCmdHandler.h"

namespace droid::brain {
    class Brain : droid::core::BaseComponent {
    public:
        Brain(const char* name, droid::core::System* system);

        //Override virtual methods from BaseComponent
        void init() override;
        void factoryReset() override;
        void task() override;
        void logConfig() override;
        void failsafe() override;
        
        void reboot();
        void overrideCmdMap(const char* trigger, const char* cmd);
        void trigger(const char* trigger);

    private:
        droid::core::System* system;
#ifdef BUILD_FOR_DEBUGGER
        droid::services::NoPWMService pwmService;
#else
        droid::services::PCA9685PWM pwmService;
#endif
#ifdef BUILD_FOR_DEBUGGER
        droid::controller::StubController controller;
#else
//        droid::controller::DualSonyMoveController controller;
        droid::controller::DualRingController controller;
#endif
        droid::motor::DRV8871Driver domeMotorDriver;
//        droid::motor::SabertoothDriver driveMotorDriver;
        droid::motor::DRV8871Driver driveMotorDriver;
        DomeMgr domeMgr;
        droid::command::ActionMgr actionMgr;
        droid::audio::HCRDriver audioDriver;
        droid::audio::AudioMgr audioMgr;
        droid::brain::DriveMgr driveMgr;

        std::vector<droid::core::BaseComponent*> componentList;

        char inputBuf[100];
        uint8_t bufIndex = 0;

        void processConsoleInput(Stream* cmdStream);
    };
}