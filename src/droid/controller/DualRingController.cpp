#include "droid/controller/DualRingController.h"
#include "droid/core/System.h"
#include "shared/blering/DualRingBLE.h"

// #define PREFERENCE_PS3_FOOT_MAC        "ps3footmac"
// #define PREFERENCE_PS3_DOME_MAC        "ps3domemac"
// #define PS3_CONTROLLER_DEFAULT_MAC     "XX:XX:XX:XX:XX:XX"
// #define PS3_CONTROLLER_FOOT_MAC        PS3_CONTROLLER_DEFAULT_MAC
// #define PS3_CONTROLLER_DOME_MAC        PS3_CONTROLLER_DEFAULT_MAC
// #define PS3_CONTROLLER_BACKUP_FOOT_MAC PS3_CONTROLLER_DEFAULT_MAC
// #define PS3_CONTROLLER_BACKUP_DOME_MAC PS3_CONTROLLER_DEFAULT_MAC

constexpr blering::DualRingBLE::Controller DualRingBLE_Drive = blering::DualRingBLE::Controller::Drive; 
constexpr blering::DualRingBLE::Controller DualRingBLE_Dome = blering::DualRingBLE::Controller::Dome; 
constexpr blering::DualRingBLE::Modifier DualRingBLE_A = blering::DualRingBLE::Modifier::A; 
constexpr blering::DualRingBLE::Modifier DualRingBLE_B = blering::DualRingBLE::Modifier::B; 
constexpr blering::DualRingBLE::Modifier DualRingBLE_L2 = blering::DualRingBLE::Modifier::L2; 
constexpr blering::DualRingBLE::Clicker DualRingBLE_C = blering::DualRingBLE::Clicker::C; 
constexpr blering::DualRingBLE::Clicker DualRingBLE_D = blering::DualRingBLE::Clicker::D; 
constexpr blering::DualRingBLE::Clicker DualRingBLE_L1 = blering::DualRingBLE::Clicker::L1; 
constexpr blering::DualRingBLE::Button DualRingBLE_Up = blering::DualRingBLE::Button::Up; 
constexpr blering::DualRingBLE::Button DualRingBLE_Down = blering::DualRingBLE::Button::Down; 
constexpr blering::DualRingBLE::Button DualRingBLE_Left = blering::DualRingBLE::Button::Left; 
constexpr blering::DualRingBLE::Button DualRingBLE_Right = blering::DualRingBLE::Button::Right; 
constexpr blering::DualRingBLE::Axis DualRingBLE_X = blering::DualRingBLE::Axis::X; 
constexpr blering::DualRingBLE::Axis DualRingBLE_Y = blering::DualRingBLE::Axis::Y; 

namespace droid::controller {
    DualRingController::DualRingController(const char* name, droid::core::System* system) :
        Controller(name, system) {
        if (DualRingController::instance != NULL) {
            logger->log(name, ERROR, "\nFATAL Problem - constructor for DualRingController called more than once!\r\n");
            while (1);
        }
        DualRingController::instance = this;
    }

    void DualRingController::init() {
        rings.init("DualRingBLE", logger, config);
    }

    void DualRingController::factoryReset() {
        rings.factoryReset();
    }

    void DualRingController::logConfig() {
        rings.logConfig();
    }

    void DualRingController::task() {
        rings.task();
    }

    void DualRingController::failsafe() {
        //TODO
    }

    void DualRingController::setCritical(bool isCritical) {
        //TODO
        if (isCritical) {
            timeoutWindow = 10000;
        } else {
            timeoutWindow = 100;
        }
    }

    void DualRingController::setDeadband(int8_t deadband) {
        //TODO
    }

    int8_t DualRingController::getJoystickPosition(Controller::Joystick controller, Controller::Axis axis) {
        //Note that this method returns an int, not a uint!  Range is -128 to 127, not 0 to 255!

        blering::DualRingBLE::Controller mappedController;
        if (controller == Controller::RIGHT) {
            mappedController = DualRingBLE_Drive;
        } else {
            mappedController = DualRingBLE_Dome;
        }
        blering::DualRingBLE::Axis mappedAxis;
        if (axis == Controller::X) {
            mappedAxis = DualRingBLE_Y;
        } else {
            mappedAxis = DualRingBLE_X;
        }
        return rings.getJoystick(mappedController, mappedAxis);
    }

    String DualRingController::getTrigger() {
        if (rings.isButtonClicked(DualRingBLE_Drive, DualRingBLE_L1)) return "FTbtnDown_MD";

        if (rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2) &&
            rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Up)) return "btnUP_CROSS_MD";

        if (rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2) &&
            rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Down)) return "btnDown_CROSS_MD";

        if (rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2) &&
            rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Left)) return "FTbtnDown_CROSS_MD";

        if (rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2) &&
            rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Right)) return "FTbtnUP_CROSS_MD";

        if (!rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2) &&
            rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Up)) return "FTbtnUP_L1_MD";

        if (!rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2) &&
            rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Down)) return "FTbtnDown_L1_MD";

        if (!rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2) &&
            rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Left)) return "btnDown_L1_MD";

        if (!rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2) &&
            rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Right)) return "";

        if (!rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2) &&
            rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Up)) return "btnUP_MD";

        if (!rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2) &&
            rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Down)) return "btnDown_MD";

        if (!rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2) &&
            rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Left)) return "btnLeft_MD";

        if (!rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2) &&
            rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Right)) return "btnRight_MD";

        if (!rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2) &&
            rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Up)) return "FTbtnUP_MD";

        if (!rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2) &&
            rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Down)) return "btnUP_L1_MD";

        if (!rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2) &&
            rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Left)) return "btnDown_L1_MD";

        if (!rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2) &&
            rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Right)) return "btnUP_CIRCLE_MD";

        if (!rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2) &&
            rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Up)) return "btnDown_CIRCLE_MD";

        if (!rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2) &&
            rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Down)) return "btnRight_CIRCLE_MD";

        if (!rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2) &&
            rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Left)) return "btnLeft_CIRCLE_MD";

        if (!rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2) &&
            rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Right)) return "btnLeft_L1_MD";

        if (rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Up)) return "FTbtnUP_L1_MD";

        if (rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Down)) return "FTbtnDown_L1_MD";

        if (rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Left)) return "FTbtnLeft_L1_MD";

        if (rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Right)) return "FTbtnRight_L1_MD";

        if (rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Up)) return "FTbtnUP_PS_MD";

        if (rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Down)) return "FTbtnDown_PS_MD";

        if (rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Left)) return "FTbtnLeft_PS_MD";

        if (rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Right)) return "FTbtnRight_PS_MD";

        return "";
    }
}

droid::controller::DualRingController* droid::controller::DualRingController::instance = NULL;
blering::DualRingBLE rings;
