//#include "../test/DRV8871Driver.test"
//#include "../test/CmdParser.test"

#include <Arduino.h>
#include "droid/brain/Brain.h"
#include "droid/brain/hardware.h"

droid::brain::Brain brain("R2D2");

void setup() {
    LOGGER_STREAM_SETUP;
    DOME_STREAM_SETUP;
    BODY_STREAM_SETUP;
    HCR_STREAM_SETUP;
    brain.init();
    brain.logConfig();
}

void loop() {
    brain.task();
}
