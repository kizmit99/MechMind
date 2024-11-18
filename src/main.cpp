//#include "../test/DRV8871Driver.test"
//#include "../test/CmdParser.test"

#include <Arduino.h>
#include "droid/brain/Brain.h"
#include "droid/services/System.h"
#include "droid/brain/hardware.h"

droid::services::System sys(&LOGGER_STREAM, DEBUG);
droid::brain::Brain brain("R2D2", &sys);

void setup() {
    LOGGER_STREAM_SETUP;
    DOME_STREAM_SETUP;
    BODY_STREAM_SETUP;
    HCR_STREAM_SETUP;
    SABERTOOTH_STREAM_SETUP;
    delay(500);
    brain.init();
    brain.logConfig();
}

void loop() {
    brain.task();
}
