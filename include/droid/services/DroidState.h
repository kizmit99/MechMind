/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/MechMind
 */

#pragma once

namespace droid::services {
    struct DroidState {
        bool stickEnable = 1;
        bool turboSpeed = 0;
        bool autoDomeEnable = 1;
        bool domePanelsOpen = 0;
        bool bodyPanelsOpen = 0;
        bool holosActive = 0;
        bool holoLightsActive = 0;
        bool musingEnabled = 0;
        bool gestureMode = 0;
    };
}