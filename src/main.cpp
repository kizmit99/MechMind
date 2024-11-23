//#include "../test/DRV8871Driver.test"
//#include "../test/CmdParser.test"

#include "droid/brain/Brain.h"
#include "droid/core/System.h"
#include <HardwareSerial.h>
#include "droid/core/hardware.h"

droid::core::System* sys;
droid::brain::Brain* brain;

void setup() {

    LOGGER_STREAM_SETUP;
    CONSOLE_STREAM_SETUP; 
    DOME_STREAM_SETUP;
    BODY_STREAM_SETUP;
    HCR_STREAM_SETUP;
    SABERTOOTH_STREAM_SETUP;
    
    delay(500);
        
    sys = new droid::core::System(LOGGER_STREAM, DEBUG);
    brain = new droid::brain::Brain("R2D2", sys);

    brain->init();
    brain->logConfig();
}

void loop() {
    brain->task();
}
