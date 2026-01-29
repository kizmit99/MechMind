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
#include "droid/brain/ConsoleStream.h"
#include "droid/core/System.h"
#include "settings/hardware.config.h"

#define LOGNAME "Main"

droid::core::System* sys;
droid::brain::Brain* brain;
droid::brain::ConsoleStream consoleStream;
EspSoftwareSerial::UART Serial3;

void setup() {

    CONSOLE_STREAM_SETUP; 
    DOME_STREAM_SETUP;
    BODY_STREAM_SETUP;
    AUDIO_STREAM_SETUP;
    SABERTOOTH_STREAM_SETUP;
    CYTRON_STREAM_SETUP;
    
    delay(500);

    sys = new droid::core::System(&consoleStream, DEBUG);
    brain = new droid::brain::Brain("R2D2", sys);

    brain->init();
    brain->logConfig();
    
    sys->getLogger()->log(LOGNAME, INFO, "Free Memory: %d\n", ESP.getFreeHeap());
}

#define ONE_MINUTE 60000
ulong next = millis() + ONE_MINUTE;

void loop() {
    brain->task();

    if (millis() >= next) {
        sys->getLogger()->log(LOGNAME, INFO, "Free Memory: %d\n", ESP.getFreeHeap());
        next = next + ONE_MINUTE;
    }
}
