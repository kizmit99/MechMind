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
#include "droid/core/BaseComponent.h"
#include "droid/brain/DomeMgr.h"
#include "droid/brain/DriveMgr.h"
#include "droid/command/ActionMgr.h"
#include "droid/audio/AudioMgr.h"
#include "droid/services/PWMService.h"
#include "droid/controller/Controller.h"
#include "droid/motor/MotorDriver.h"
#include "droid/audio/AudioDriver.h"

namespace droid::brain {
    class ConsoleHandler;  // Forward declaration
    
    class Brain : droid::core::BaseComponent {
    public:
        Brain(const char* name, droid::core::System* system);

        //Override virtual methods from BaseComponent
        void init() override;
        void factoryReset() override;
        void task() override;
        void logConfig() override;
        void failsafe() override;
        
        // Public API for ConsoleHandler
        void reboot();
        void overrideCmdMap(const char* action, const char* cmd);
        void fireAction(const char* action);
        droid::core::System* getSystem() { return system; }

    private:
        droid::brain::DomeMgr* domeMgr;
        droid::brain::DriveMgr* driveMgr;
        droid::command::ActionMgr* actionMgr;
        droid::audio::AudioMgr* audioMgr;

        droid::services::PWMService* pwmService;
        droid::controller::Controller* controller;
        droid::motor::MotorDriver* driveMotorDriver;
        droid::motor::MotorDriver* domeMotorDriver;
        droid::audio::AudioDriver* audioDriver;
        droid::brain::ConsoleHandler* consoleHandler;

        std::vector<droid::core::BaseComponent*> componentList;
    };
}