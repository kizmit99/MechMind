/*
 * Droid Brain Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#include "droid/controller/Controller.h"
#include "droid/core/System.h"
#include "shared/blering/DualRingBLE.h"

namespace droid::controller {
    class DualRingController : public Controller {
    public:

        DualRingController(const char* name, droid::core::System* system);

        //Override virtual methods from BaseComponent
        void init() override;
        void factoryReset() override;
        void task() override;
        void logConfig() override;
        void failsafe() override;

        void setCritical(bool isCritical);
        void setDeadband(int8_t deadband);
        int8_t getJoystickPosition(Joystick, Axis);
        String getTrigger();

    private:
        static DualRingController* instance;

        int8_t deadband;
        bool faultState = true;

        void faultCheck();
    };
}