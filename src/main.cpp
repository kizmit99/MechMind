/*
 * Droid Brain Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#include "droid/brain/Brain.h"
#include "droid/core/System.h"
#include "droid/core/hardware.h"

droid::core::System* sys;
droid::brain::Brain* brain;

void setup() {

    CONSOLE_STREAM_SETUP; 
    LOGGER_STREAM_SETUP;

    delay(500);
Serial.println("Debug1");

    DOME_STREAM_SETUP;
Serial.println("Debug2");
    BODY_STREAM_SETUP;
Serial.println("Debug3");
    HCR_STREAM_SETUP;
Serial.println("Debug4");
    SABERTOOTH_STREAM_SETUP;
Serial.println("Debug5");
    
    delay(500);
        
Serial.println("Debug6");
    sys = new droid::core::System(LOGGER_STREAM, INFO);
Serial.println("Debug7");
    brain = new droid::brain::Brain("R2D2", sys);
Serial.println("Debug8");

    brain->init();
Serial.println("Debug9");
    brain->logConfig();
}

void loop() {
    brain->task();
}
