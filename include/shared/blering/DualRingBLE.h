/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>
#include "Ring.h"
#include "shared/common/Config.h"

/**
 * This class encapsulates access to two physical Magicsee R1 remote controllers.
 * 
 */
namespace blering {
    class DualRingBLE {
    public:
        enum Modifier {
            A, B, L2};

        enum Button {
            Up, Down, Left, Right};

        enum Clicker {
            C, D, L1};

        enum Controller {
            Drive, Dome};

        enum Axis {
            X, Y};

        DualRingBLE() {}

        void init(const char* name, Logger* logger, Config* config);
        void task();
        void factoryReset();
        void logConfig();

        bool isConnected();
        bool hasFault();
        void printState();

        bool isModifierPressed(Controller controller, Modifier button);
        bool isButtonPressed(Controller controller, Button button);
        bool isButtonClicked(Controller controller, Clicker button);
        //Joystick return value ranges from -128 to 127 (native joystick coordinates)
        int8_t getJoystick(Controller controller, Axis direction);

        Ring* getRing(Controller controller);
        void clearMACMap();

        //helper method for embedded global functions without direct access to Logger
        void log(LogLevel level, const char *format, ...) __attribute__ ((format (printf, 3, 4)));

    private:
        const char* name;
        Config* config;
        Logger* logger;
        Ring driveRing;
        Ring domeRing;
    };
}

// Reference to the single instance of this class
extern blering::DualRingBLE rings;
