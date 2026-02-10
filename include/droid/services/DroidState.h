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

        // Updated each loop by DomeMgr/DriveMgr to indicate motion is being commanded.
        // Brain uses these to set Controller::setCritical() once per loop.
        bool driveMotion = 0;
        bool domeMotion = 0;

        bool domePanelsOpen = 0;
        bool bodyPanelsOpen = 0;
        bool holosActive = 0;
        bool holoLightsActive = 0;
        bool musingEnabled = 0;
        bool gestureMode = 0;
    };
}