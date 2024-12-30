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
        //init triggerMap with defaults then load overrides from config
        triggerMap.clear();
        #include "droid/controller/DualRingTrigger.map"
        // Iterate through the map looking for overrides
        for (const auto& mapEntry : triggerMap) {
            const char* trigger = mapEntry.first.c_str();
            const char* action = mapEntry.second.c_str();
            String override = config->getString(name, trigger, action);
            if (override != action) {
                triggerMap[trigger] = override;
            }
        }

        rings.init("DualRingBLE", logger, config);
    }

    void DualRingController::factoryReset() {
        //init triggerMap with defaults then store default into config
        triggerMap.clear();
        #include "droid/controller/DualRingTrigger.map"
        // Iterate through the map clearing all overrides
        for (const auto& mapEntry : triggerMap) {
            const char* trigger = mapEntry.first.c_str();
            const char* action = mapEntry.second.c_str();
            config->putString(name, trigger, action);
        }

        rings.factoryReset();
    }

    void DualRingController::logConfig() {
        // Iterate through the triggerMap for keys to log
        for (const auto& mapEntry : triggerMap) {
            const char* trigger = mapEntry.first.c_str();
            logger->log(name, INFO, "Config %s = %s\n", trigger, config->getString(name, trigger, "").c_str());
        }

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

    String DualRingController::getAction() {
        //Button definitions for Dual Ring Triggers to mimic PenumbraShadowMD
        String trigger = getTrigger();
        if (trigger == "") {    //No trigger detected
            return "";
        }
        return triggerMap[trigger];
    }

    String DualRingController::getTrigger() {
        //Click triggers
        if (rings.isButtonClicked(DualRingBLE_Dome, DualRingBLE_C)) return "LC";
        if (rings.isButtonClicked(DualRingBLE_Dome, DualRingBLE_D)) return "LD";
        if (rings.isButtonClicked(DualRingBLE_Dome, DualRingBLE_L1)) return "LL1";
        if (rings.isButtonClicked(DualRingBLE_Drive, DualRingBLE_C)) return "RC";
        if (rings.isButtonClicked(DualRingBLE_Drive, DualRingBLE_D)) return "RD";
        if (rings.isButtonClicked(DualRingBLE_Drive, DualRingBLE_L1)) return "RL1";

        // Dome Joystick + Drive-A
        if (rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2)) {
            if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Up)) return "RA_Lup";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Down)) return "RA_Ldown";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Left)) return "RA_Lleft";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Right)) return "RA_Lright";
        }

        //Dome Joystick + Drive-B
        if (rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2)) {
            if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Up)) return "RB_Lup";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Down)) return "RB_Ldown";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Left)) return "RB_Lleft";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Right)) return "RB_Lright";
        }

        //Dome Joystick + Dome-A (Requires two hands, not stealthy)
        if (!rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2)) {
            if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Up)) return "LA_Lup";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Down)) return "LA_Ldown";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Left)) return "LA_Lleft";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Right)) return "LA_Lright";
        }

        //Dome Joystick + Dome-B (Requires two hands, not stealthy)
        if (!rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2)) {
            if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Up)) return "LB_Lup";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Down)) return "LB_Ldown";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Left)) return "LB_Lleft";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Right)) return "LB_Lright";
        }

        //Drive Joystick + Dome-A
        if (!rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2) &&
            rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A)) {
            if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Up)) return "LA_Rup";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Down)) return "LA_Rdown";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Left)) return "LA_Rleft";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Right)) return "LA_Rright";
        }

        //Drive Joystick + Dome-B
        if (!rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2) &&
            rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B)) {
            if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Up)) return "LB_Rup";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Down)) return "LB_Rdown";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Left)) return "LB_Rleft";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Right)) return "LB_Rright";
        }

        //Drive Joystick + Drive-A (Requires two hands, not stealthy)
        if (rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B)) {
            if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Up)) return "RA_Rup";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Down)) return "RA_Rdown";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Left)) return "RA_Rleft";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Right)) return "RA_Rright";
        }

        //Drive Joystick + Drive-B (Requires two hands, not stealthy)
        if (rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B)) {
            if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Up)) return "RB_Rup";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Down)) return "RB_Rdown";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Left)) return "RB_Rleft";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Right)) return "RB_Rright";
        }

        //Dome Joystick + Dome-A + Drive-A (Requries two hands, awkward to use)
        if (rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2) &&
            rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A)) {
            if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Up)) return "LA_RA_Lup";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Down)) return "LA_RA_Ldown";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Left)) return "LA_RA_Lleft";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Right)) return "LA_RA_Lright";
        }

        //Dome Joystick + Dome-A + Drive-B (Requries two hands, awkward to use)
        if (rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2) &&
            rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B)) {
            if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Up)) return "LA_RB_Lup";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Down)) return "LA_RB_Ldown";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Left)) return "LA_RB_Lleft";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Right)) return "LA_RB_Lright";
        }

        //Dome Joystick + Dome-B + Drive-A (Requries two hands, awkward to use)
        if (rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2) &&
            rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A)) {
            if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Up)) return "LB_RA_Lup";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Down)) return "LB_RA_Ldown";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Left)) return "LB_RA_Lleft";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Right)) return "LB_RA_Lright";
        }

        //Dome Joystick + Dome-B + Drive-B (Requries two hands, awkward to use)
        if (rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2) &&
            rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B)) {
            if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Up)) return "LB_RB_Lup";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Down)) return "LB_RB_Ldown";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Left)) return "LB_RB_Lleft";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Right)) return "LB_RB_Lright";
        }


        //Drive Joystick + Drive-A + Dome-A (Requries two hands, awkward to use)
        if (rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2)) {
            if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Up)) return "RA_LA_Rup";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Down)) return "RA_LA_Rdown";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Left)) return "RA_LA_Rleft";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Right)) return "RA_LA_Rright";
        }

        //Drive Joystick + Drive-A + Dome-B (Requries two hands, awkward to use)
        if (rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2)) {
            if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Up)) return "RA_LB_Rup";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Down)) return "RA_LB_Rdown";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Left)) return "RA_LB_Rleft";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Right)) return "RA_LB_Rright";
        }

        //Drive Joystick + Drive-B + Dome-A (Requries two hands, awkward to use)
        if (rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2)) {
            if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Up)) return "RB_LA_Rup";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Down)) return "RB_LA_Rdown";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Left)) return "RB_LA_Rleft";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Right)) return "RB_LA_Rright";
        }

        //Drive Joystick + Drive-B + Dome-B (Requries two hands, awkward to use)
        if (rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2)) {
            if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Up)) return "RB_LB_Rup";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Down)) return "RB_LB_Rdown";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Left)) return "RB_LB_Rleft";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Right)) return "RB_LB_Rright";
        }


        //Dome Joystick + no modifiers (Avoid, easily triggered accidentally)
        if (!rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_L2)) {
            if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Up)) return "Lup";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Down)) return "Ldown";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Left)) return "Lleft";
            else if (rings.isButtonPressed(DualRingBLE_Dome, DualRingBLE_Right)) return "Lright";
        }

        //Drive Joystick + no modifiers (Avoid, easily triggered accidentally)
        if (!rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_B) &&
            !rings.isModifierPressed(DualRingBLE_Dome, DualRingBLE_L2) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_A) &&
            !rings.isModifierPressed(DualRingBLE_Drive, DualRingBLE_B)) {
            if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Up)) return "Rup";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Down)) return "Rdown";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Left)) return "Rleft";
            else if (rings.isButtonPressed(DualRingBLE_Drive, DualRingBLE_Right)) return "Rright";
        }

        return "";
    }
}

droid::controller::DualRingController* droid::controller::DualRingController::instance = NULL;
blering::DualRingBLE rings;
