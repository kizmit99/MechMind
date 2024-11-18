#pragma once
#include "droid/core/ActiveComponent.h"
#include "droid/controller/Controller.h"
#include "droid/motor/MotorDriver.h"

namespace droid::brain {
    class DomeMgr : public droid::core::ActiveComponent {
    public:
        DomeMgr(const char* name, droid::core::System* system, droid::controller::Controller*, droid::motor::MotorDriver*);

        //Override virtual methods from ActiveComponent
        void init() override;
        void factoryReset() override;
        void task() override;
        void logConfig() override;
        void failsafe() override;

    private:
        droid::controller::Controller* controller;
        droid::motor::MotorDriver* domeMotor;

        int8_t speed;
        int8_t deadband;
    };
}