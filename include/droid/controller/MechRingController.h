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
#include "droid/controller/Controller.h"
#include "droid/network/MechNetNode.h"
#include <map>

namespace droid::controller {
    class MechRingController : public Controller {
    public:
        MechRingController(const char* name, droid::core::System* system, 
                          droid::network::MechNetNode* mechNetNode);

        // BaseComponent lifecycle
        void init() override;
        void factoryReset() override;
        void task() override;
        void logConfig() override;
        void failsafe() override;

        // Controller interface
        void setCritical(bool isCritical) override;
        int8_t getJoystickPosition(Joystick, Axis) override;
        String getAction() override;
        ControllerType getType() override { return MECHRING; }

        // Message handling (called by Brain::routeMessageToHandler)
        void handleRingMessage(const String& sender, const String& message);

    private:
        struct RingState {
            char uniqueName[32];           // "DriveRing-A3F2"
            volatile int8_t joystick_x;    // -128 to +127
            volatile int8_t joystick_y;    // -128 to +127
            volatile uint32_t buttonState; // Bitmask
            volatile uint32_t lastMsgTime; // millis()
            volatile uint16_t batteryVoltage; // mV
            volatile bool isConnected;
        };
        
        RingState driveRing;  // RIGHT joystick
        RingState domeRing;   // LEFT joystick
        
        droid::network::MechNetNode* mechNetNode;  // Reference (not owned)
        std::map<String, String> triggerMap;
        static MechRingController* instance;
        
        bool isCritical = false;
        uint32_t activeTimeout = 0;
        uint32_t inactiveTimeout = 0;
        int8_t deadbandX = 0;
        int8_t deadbandY = 0;
        bool faultState = false;
        
        void parseRingMessage(const String& msg, RingState& ring);
        void updateRingConnectionStatus();
        void faultCheck();
        String getTrigger();
    };
}
