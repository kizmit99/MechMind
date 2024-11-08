//#include "../test/DRV8871Driver.test"
//#include "../test/CmdParser.test"

#include <Arduino.h>
#include "droid/brain/Brain.h"

droid::brain::Brain brain("R2D2");

void setup() {
    Serial.begin(115200);
    brain.init();
    brain.logConfig();
}

void loop() {
    brain.task();
}
