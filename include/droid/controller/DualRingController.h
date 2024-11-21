#include "droid/controller/Controller.h"
#include "droid/core/System.h"
#include "shared/blering/DualRingBLE.h"

namespace droid::controller {
    class DualRingController : public Controller {
    public:

        DualRingController(const char* name, droid::core::System* system);

        //Override virtual methods from ActiveComponent
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