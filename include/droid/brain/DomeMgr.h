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
#include "droid/controller/Controller.h"
#include "droid/motor/MotorDriver.h"

namespace droid::brain {
    class DomeMgr : public droid::core::BaseComponent {
    public:
        DomeMgr(const char* name, droid::core::System* system, droid::controller::Controller*, droid::motor::MotorDriver*);

        //Override virtual methods from BaseComponent
        void init() override;
        void factoryReset() override;
        void task() override;
        void logConfig() override;
        void failsafe() override;

    private:
        bool doAutoDome();

        droid::controller::Controller* controller = nullptr;
        droid::motor::MotorDriver* domeMotor = nullptr;

        int8_t speed = 0;
        int16_t rotationTimeMs = 1000;     //How long it takes for the dome to turn 360 degrees at max speed
        int8_t deadband = 0;

        //AutoDome config
        bool autoEnabled = false;
        int8_t autoMinSpeed = 0;
        int8_t autoMaxSpeed = 0;
        int32_t autoMinDelayMs = 0;
        int32_t autoMaxDelayMs = 0;
        // bool autoAudio = false;
        // bool autoLights = false;

        //AutoDome state
        bool autoDomeActive = false;
        bool autoDomeMoving = false;
        int16_t autoDomeAngle = 0;
        int8_t autoDomeSpeed = 0;
        unsigned long autoDomeNextStop = 0;
        unsigned long autoDomeNextMove = 0;
    };
}