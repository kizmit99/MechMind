/*
 * Droid Brain Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#pragma once
#include "droid/core/ActiveComponent.h"
#include "droid/controller/Controller.h"
#include "droid/motor/MotorDriver.h"

namespace droid::brain {
    class DriveMgr : public droid::core::ActiveComponent {
    public:
        DriveMgr(const char* name, droid::core::System* system, droid::controller::Controller*, droid::motor::MotorDriver*);

        //Override virtual methods from ActiveComponent
        void init() override;
        void factoryReset() override;
        void task() override;
        void logConfig() override;
        void failsafe() override;

    private:
        droid::controller::Controller* controller;
        droid::motor::MotorDriver* driveMotor;

        int8_t normalSpeed;
        int8_t turboSpeed;
        int8_t turnSpeed;
        int8_t deadband;
    };
}