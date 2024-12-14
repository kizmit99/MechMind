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
    class DriveMgr : public droid::core::BaseComponent {
    public:
        DriveMgr(const char* name, droid::core::System* system, droid::controller::Controller*, droid::motor::MotorDriver*);

        //Override virtual methods from BaseComponent
        void init() override;
        void factoryReset() override;
        void task() override;
        void logConfig() override;
        void failsafe() override;

    private:
        droid::controller::Controller* controller = nullptr;
        droid::motor::MotorDriver* driveMotor = nullptr;

        int8_t normalSpeed = 0;
        int8_t turboSpeed = 0;
        int8_t turnSpeed = 0;
        int8_t deadband = 0;
    };
}