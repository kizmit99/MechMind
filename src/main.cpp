/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/MechMind
 */

#include "droid/brain/Brain.h"
#include "droid/core/System.h"
#include "droid/core/hardware.h"

droid::core::System* sys;
droid::brain::Brain* brain;

void setup() {

    CONSOLE_STREAM_SETUP; 
    LOGGER_STREAM_SETUP;
    DOME_STREAM_SETUP;
    BODY_STREAM_SETUP;
    HCR_STREAM_SETUP;
    SABERTOOTH_STREAM_SETUP;
    
    delay(500);
        
    sys = new droid::core::System(LOGGER_STREAM, INFO);
    brain = new droid::brain::Brain("R2D2", sys);

    brain->init();
    brain->logConfig();
}

void loop() {
    brain->task();
}
