#pragma once
#include "droid/services/ActiveComponent.h"
#include "droid/controller/Controller.h"
#include "droid/motor/MotorDriver.h"

namespace droid::brain {
    class DriveMgr : public droid::services::ActiveComponent {
    public:
        DriveMgr(const char* name, droid::services::System* system, droid::controller::Controller*, droid::motor::MotorDriver*);

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