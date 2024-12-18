/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/MechMind
 */

#include "droid/controller/DualRingController.h"
#include "droid/core/System.h"
#include "shared/blering/DualRingBLE.h"

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
        faultCheck();
    }

    void DualRingController::failsafe() {
        //NOOP
    }

    void DualRingController::setCritical(bool isCritical) {
        //NOOP
    }

    void DualRingController::faultCheck() {
        if ((!faultState) &&
            (!rings.isConnected())) {
            faultState = true;
            logger->log(name, ERROR, "Controller has lost connection to one of the Rings\n");
        }
        if ((faultState) &&
            (rings.isConnected())) {
            faultState = false;
        }
    }

    //This method return normalized joystick position (range -100 to +100)
    int8_t DualRingController::getJoystickPosition(Controller::Joystick controller, Controller::Axis axis) {
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

        int8_t nativeValue = rings.getJoystick(mappedController, mappedAxis);
        int8_t normalizedValue = map(nativeValue, -128, 127, -100, 100);
        if (nativeValue == 0) {
            normalizedValue = 0;
        }
        return normalizedValue;
    }

    String DualRingController::getTrigger() {
        //Click triggers
        if (rings.isButtonClicked(DualRingBLE_Dome, DualRingBLE_C)) return "AllDomePanels";
        if (rings.isButtonClicked(DualRingBLE_Dome, DualRingBLE_D)) return "ToggleHolos";
        if (rings.isButtonClicked(DualRingBLE_Dome, DualRingBLE_L1)) return "AutoDome";
        if (rings.isButtonClicked(DualRingBLE_Drive, DualRingBLE_C)) return "AllBodyPanels";
        if (rings.isButtonClicked(DualRingBLE_Drive, DualRingBLE_D)) return "ToggleMusings";
        if (rings.isButtonClicked(DualRingBLE_Drive, DualRingBLE_L1)) return "Gesture";

        // Dome Joystick + Drive-A
        if (rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2)) {
            if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Up)) return "Happy";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Down)) return "Sad";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Left)) return "Fear";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Right)) return "Anger";
        }

        //Dome Joystick + Drive-B
        if (rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2)) {
            if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Up)) return "VolumeUp";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Down)) return "VolumeDown";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Left)) return "VolumeMid";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Right)) return "VolumeMax";
        }

        //Dome Joystick + Dome-A (Requires two hands, not stealthy)
        if (!rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2)) {
            if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Up)) return "OpenDomeP1";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Down)) return "CloseDomeP1";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Left)) return "LeiaMessage";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Right)) return "MarchingAnts";
        }

        //Dome Joystick + Dome-B (Requires two hands, not stealthy)
        if (!rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2)) {
            if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Up)) return "FullAwake";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Down)) return "QuietMode";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Left)) return "MidAwake";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Right)) return "FullAware";
        }

        //Drive Joystick + Dome-A
        if (!rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2) &&
            rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A)) {
            if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Up)) return "BeepCantina";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Down)) return "CantinaDance";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Left)) return "LeiaMessage";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Right)) return "Scream";
        }

        //Drive Joystick + Dome-B
        if (!rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2) &&
            rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B)) {
            if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Up)) return "Disco";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Down)) return "ShortCircuit";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Left)) return "FastSmirk";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Right)) return "Wave";
        }

        //Drive Joystick + Drive-A (Requires two hands, not stealthy)
        if (rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B)) {
            if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Up)) return "OpenDomeP1";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Down)) return "CloseDomeP1";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Left)) return "OpenDomeP2";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Right)) return "CloseDomeP2";
        }

        //Drive Joystick + Drive-B (Requires two hands, not stealthy)
        if (rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B)) {
            if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Up)) return "OpenDomeP3";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Down)) return "CloseDomeP3";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Left)) return "OpenDomeP4";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Right)) return "CloseDomeP4";
        }

        //Dome Joystick + Dome-A + Drive-A (Requries two hands, awkward to use)
        if (rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2) &&
            rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A)) {
            if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Up)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Down)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Left)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Right)) return "";
        }

        //Dome Joystick + Dome-A + Drive-B (Requries two hands, awkward to use)
        if (rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2) &&
            rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B)) {
            if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Up)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Down)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Left)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Right)) return "";
        }

        //Dome Joystick + Dome-B + Drive-A (Requries two hands, awkward to use)
        if (rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2) &&
            rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A)) {
            if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Up)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Down)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Left)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Right)) return "";
        }

        //Dome Joystick + Dome-B + Drive-B (Requries two hands, awkward to use)
        if (rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2) &&
            rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B)) {
            if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Up)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Down)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Left)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Right)) return "";
        }


        //Drive Joystick + Drive-A + Dome-A (Requries two hands, awkward to use)
        if (rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2)) {
            if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Up)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Down)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Left)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Right)) return "";
        }

        //Drive Joystick + Drive-A + Dome-B (Requries two hands, awkward to use)
        if (rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2)) {
            if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Up)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Down)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Left)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Right)) return "";
        }

        //Drive Joystick + Drive-B + Dome-A (Requries two hands, awkward to use)
        if (rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2)) {
            if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Up)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Down)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Left)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Right)) return "";
        }

        //Drive Joystick + Drive-B + Dome-B (Requries two hands, awkward to use)
        if (rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2)) {
            if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Up)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Down)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Left)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Right)) return "";
        }


        //Dome Joystick + no modifiers (Avoid, easily triggered accidentally)
        if (!rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2)) {
            if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Up)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Down)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Left)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Right)) return "";
        }

        //Drive Joystick + no modifiers (Avoid, easily triggered accidentally)
        if (!rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B)) {
            if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Up)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Down)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Left)) return "";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Right)) return "";
        }

        return "";
    }
}

droid::controller::DualRingController* droid::controller::DualRingController::instance = NULL;
blering::DualRingBLE rings;
