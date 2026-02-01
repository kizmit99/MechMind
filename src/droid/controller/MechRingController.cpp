/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/MechMind
 */

#include "droid/controller/MechRingController.h"

#define CONFIG_KEY_MECHRING_ACTIVE_TIMEOUT      "activeTimeout"
#define CONFIG_KEY_MECHRING_INACTIVE_TIMEOUT    "inactiveTimeout"
#define CONFIG_KEY_MECHRING_DEADBAND_X          "DeadbandX"
#define CONFIG_KEY_MECHRING_DEADBAND_Y          "DeadbandY"

#define CONFIG_DEFAULT_MECHRING_ACTIVE_TIMEOUT   200
#define CONFIG_DEFAULT_MECHRING_INACTIVE_TIMEOUT 10000
#define CONFIG_DEFAULT_MECHRING_DEADBAND         16

namespace {
    // Button bitmask constants
    const uint8_t BTN_JOY_PRESS   = 0x01;  // Bit 0
    const uint8_t BTN_TRIGGER     = 0x02;  // Bit 1
    const uint8_t BTN_PUSH1       = 0x04;  // Bit 2
    const uint8_t BTN_PUSH2       = 0x08;  // Bit 3
    const uint8_t BTN_PUSH3       = 0x10;  // Bit 4
    const uint8_t BTN_PUSH4       = 0x20;  // Bit 5
    const uint8_t BTN_RECESSED    = 0x40;  // Bit 6
}

namespace droid::controller {
    MechRingController* MechRingController::instance = nullptr;

    MechRingController::MechRingController(const char* name, droid::core::System* system,
                                           droid::network::MechNetNode* mechNetNode) :
        Controller(name, system),
        mechNetNode(mechNetNode) {
        
        if (MechRingController::instance != nullptr) {
            logger->log(name, FATAL, "Constructor called more than once!\n");
            while (1);  // Halt (singleton constraint)
        }
        MechRingController::instance = this;
        
        // Initialize RingState structs
        memset(&driveRing, 0, sizeof(RingState));
        memset(&domeRing, 0, sizeof(RingState));
    }

    void MechRingController::init() {
        logger->log(name, INFO, "init - called\n");
        
        // Load configuration
        activeTimeout = config->getInt(name, CONFIG_KEY_MECHRING_ACTIVE_TIMEOUT, 
                                       CONFIG_DEFAULT_MECHRING_ACTIVE_TIMEOUT);
        inactiveTimeout = config->getInt(name, CONFIG_KEY_MECHRING_INACTIVE_TIMEOUT, 
                                         CONFIG_DEFAULT_MECHRING_INACTIVE_TIMEOUT);
        deadbandX = config->getInt(name, CONFIG_KEY_MECHRING_DEADBAND_X, 
                                   CONFIG_DEFAULT_MECHRING_DEADBAND);
        deadbandY = config->getInt(name, CONFIG_KEY_MECHRING_DEADBAND_Y, 
                                   CONFIG_DEFAULT_MECHRING_DEADBAND);
        
        // Init triggerMap with defaults from MechRingTrigger.map
        triggerMap.clear();
        #include "settings/MechRingTrigger.map"
        
        // Load config overrides for trigger mappings
        for (const auto& mapEntry : triggerMap) {
            const char* trigger = mapEntry.first.c_str();
            const char* action = mapEntry.second.c_str();
            String override = config->getString(name, trigger, action);
            if (override != action) {
                triggerMap[trigger] = override;
            }
        }
        
        // Initialize ring states
        memset(&driveRing, 0, sizeof(RingState));
        memset(&domeRing, 0, sizeof(RingState));
        driveRing.lastMsgTime = millis();
        domeRing.lastMsgTime = millis();
        
        faultState = false;
        isCritical = false;
    }

    void MechRingController::factoryReset() {
        config->clear(name);
        config->putInt(name, CONFIG_KEY_MECHRING_ACTIVE_TIMEOUT, 
                       CONFIG_DEFAULT_MECHRING_ACTIVE_TIMEOUT);
        config->putInt(name, CONFIG_KEY_MECHRING_INACTIVE_TIMEOUT, 
                       CONFIG_DEFAULT_MECHRING_INACTIVE_TIMEOUT);
        config->putInt(name, CONFIG_KEY_MECHRING_DEADBAND_X, 
                       CONFIG_DEFAULT_MECHRING_DEADBAND);
        config->putInt(name, CONFIG_KEY_MECHRING_DEADBAND_Y, 
                       CONFIG_DEFAULT_MECHRING_DEADBAND);
        
        // Save default trigger mappings
        triggerMap.clear();
        #include "settings/MechRingTrigger.map"
        for (const auto& mapEntry : triggerMap) {
            const char* trigger = mapEntry.first.c_str();
            const char* action = mapEntry.second.c_str();
            config->putString(name, trigger, action);
        }
    }

    void MechRingController::task() {
        // Update connection status from MechNet
        updateRingConnectionStatus();
        
        // Check for timeouts
        faultCheck();
    }

    void MechRingController::logConfig() {
        logger->log(name, INFO, "Config %s = %s\n", 
                    CONFIG_KEY_MECHRING_ACTIVE_TIMEOUT, 
                    config->getString(name, CONFIG_KEY_MECHRING_ACTIVE_TIMEOUT, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", 
                    CONFIG_KEY_MECHRING_INACTIVE_TIMEOUT,
                    config->getString(name, CONFIG_KEY_MECHRING_INACTIVE_TIMEOUT, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", 
                    CONFIG_KEY_MECHRING_DEADBAND_X,
                    config->getString(name, CONFIG_KEY_MECHRING_DEADBAND_X, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", 
                    CONFIG_KEY_MECHRING_DEADBAND_Y,
                    config->getString(name, CONFIG_KEY_MECHRING_DEADBAND_Y, "").c_str());
        
        // Log trigger mappings
        for (const auto& mapEntry : triggerMap) {
            const char* trigger = mapEntry.first.c_str();
            logger->log(name, INFO, "Config %s = %s\n", 
                        trigger, config->getString(name, trigger, "").c_str());
        }
    }

    void MechRingController::failsafe() {
        // NOOP - Brain handles emergency stop via droidState->stickEnable = false
    }

    void MechRingController::setCritical(bool isCritical) {
        this->isCritical = isCritical;
    }

    int8_t MechRingController::getJoystickPosition(Joystick stick, Axis axis) {
        // Select ring based on joystick
        RingState& ring = (stick == RIGHT) ? driveRing : domeRing;
        
        // Get raw value
        int8_t rawValue = (axis == X) ? ring.joystick_x : ring.joystick_y;
        
        // Apply deadband
        int8_t deadband = (axis == X) ? deadbandX : deadbandY;
        if (abs(rawValue) < deadband) {
            return 0;
        }
        
        // Convert -128/+127 → -100/+100
        int16_t scaled = (rawValue * 100) / 127;
        return constrain(scaled, -100, 100);
    }

    String MechRingController::getAction() {
        String trigger = getTrigger();
        if (trigger.length() == 0) {
            return "";
        }
        
        // Lookup in triggerMap
        auto it = triggerMap.find(trigger);
        if (it != triggerMap.end()) {
            logger->log(name, DEBUG, "Trigger: %s -> Action: %s\n", 
                        trigger.c_str(), it->second.c_str());
            return it->second;
        }
        
        return "";
    }

    void MechRingController::handleRingMessage(const String& sender, const String& message) {
        // Route to correct ring based on sender prefix
        if (sender.startsWith("DriveRing")) {
            parseRingMessage(message, driveRing);
            strncpy(driveRing.uniqueName, sender.c_str(), sizeof(driveRing.uniqueName) - 1);
            driveRing.uniqueName[sizeof(driveRing.uniqueName) - 1] = '\0';
        } else if (sender.startsWith("DomeRing")) {
            parseRingMessage(message, domeRing);
            strncpy(domeRing.uniqueName, sender.c_str(), sizeof(domeRing.uniqueName) - 1);
            domeRing.uniqueName[sizeof(domeRing.uniqueName) - 1] = '\0';
        } else {
            logger->log(name, WARN, "Unknown ring sender: %s\n", sender.c_str());
        }
    }

    void MechRingController::parseRingMessage(const String& msg, RingState& ring) {
        if (!msg.startsWith("S:")) {
            logger->log(name, DEBUG, "Invalid message format: %s\n", msg.c_str());
            return;
        }
        
        int x, y;
        unsigned int buttons;
        int voltage = 0;
        
        // Parse: S:<x>,<y>,<hex>[,<voltage>]
        int parsed = sscanf(msg.c_str() + 2, "%d,%d,%x,%d", &x, &y, &buttons, &voltage);
        
        if (parsed >= 3) {
            ring.joystick_x = constrain(x, -128, 127);
            ring.joystick_y = constrain(y, -128, 127);
            ring.buttonState = buttons & 0xFF;
            ring.lastMsgTime = millis();
            
            if (parsed == 4 && voltage > 0) {
                ring.batteryVoltage = voltage;
                
                // Log battery warnings
                if (voltage < 3300) {  // Below 3.3V
                    logger->log(name, WARN, "%s battery low: %umV\n", 
                                ring.uniqueName, voltage);
                }
            }
            
            logger->log(name, DEBUG, "Parsed %s: X=%d, Y=%d, BTN=0x%02X\n",
                        ring.uniqueName, ring.joystick_x, ring.joystick_y, 
                        (uint8_t)ring.buttonState);
        } else {
            logger->log(name, WARN, "Failed to parse message: %s\n", msg.c_str());
        }
    }

    void MechRingController::updateRingConnectionStatus() {
        if (!mechNetNode || !mechNetNode->isInitialized()) {
            driveRing.isConnected = false;
            domeRing.isConnected = false;
            return;
        }
        
        // Check if ring nodes are connected via MechNet
        driveRing.isConnected = (strlen(driveRing.uniqueName) > 0) && 
                                mechNetNode->isNodeConnected(driveRing.uniqueName);
        domeRing.isConnected = (strlen(domeRing.uniqueName) > 0) && 
                               mechNetNode->isNodeConnected(domeRing.uniqueName);
    }

    void MechRingController::faultCheck() {
        uint32_t timeout = isCritical ? activeTimeout : inactiveTimeout;
        unsigned long now = millis();
        
        // Check DriveRing timeout
        if ((now - driveRing.lastMsgTime) > timeout) {
            if (!faultState) {
                faultState = true;
                droidState->stickEnable = false;
                logger->log(name, ERROR, "EMERGENCY STOP: DriveRing timeout (%ums)\n", timeout);
            }
            return;  // Don't check DomeRing if DriveRing timed out
        }
        
        // Check DomeRing timeout
        if ((now - domeRing.lastMsgTime) > timeout) {
            if (!faultState) {
                faultState = true;
                droidState->stickEnable = false;
                logger->log(name, ERROR, "EMERGENCY STOP: DomeRing timeout (%ums)\n", timeout);
            }
            return;
        }
        
        // Both rings OK - clear fault state if recovering
        if (faultState && driveRing.isConnected && domeRing.isConnected) {
            faultState = false;
            logger->log(name, INFO, "Ring controllers reconnected\n");
            // User must manually re-enable stick control for safety
        }
    }

    String MechRingController::getTrigger() {
        uint8_t driveButtons = driveRing.buttonState & 0xFF;
        uint8_t domeButtons = domeRing.buttonState & 0xFF;
        
        // Priority order: combinations first, then single buttons
        
        // Check for both-ring combinations first
        if ((driveButtons & BTN_TRIGGER) && (domeButtons & BTN_TRIGGER)) {
            return "R_TRIG+L_TRIG";
        }
        if ((driveButtons & BTN_PUSH1) && (domeButtons & BTN_PUSH1)) {
            return "R_BTN1+L_BTN1";
        }
        if ((driveButtons & BTN_PUSH2) && (domeButtons & BTN_PUSH2)) {
            return "R_BTN2+L_BTN2";
        }
        if ((driveButtons & BTN_PUSH3) && (domeButtons & BTN_PUSH3)) {
            return "R_BTN3+L_BTN3";
        }
        if ((driveButtons & BTN_JOY_PRESS) && (domeButtons & BTN_JOY_PRESS)) {
            return "R_JOY+L_JOY";
        }
        if ((driveButtons & BTN_RECESSED) && (domeButtons & BTN_RECESSED)) {
            return "R_RECESS+L_RECESS";
        }
        
        // Check DriveRing (RIGHT) combinations
        if ((driveButtons & BTN_TRIGGER) && (driveButtons & BTN_PUSH1)) {
            return "R_TRIG+R_BTN1";
        }
        if ((driveButtons & BTN_TRIGGER) && (driveButtons & BTN_PUSH2)) {
            return "R_TRIG+R_BTN2";
        }
        if ((driveButtons & BTN_TRIGGER) && (driveButtons & BTN_PUSH3)) {
            return "R_TRIG+R_BTN3";
        }
        if ((driveButtons & BTN_TRIGGER) && (driveButtons & BTN_PUSH4)) {
            return "R_TRIG+R_BTN4";
        }
        
        // Check DomeRing (LEFT) combinations
        if ((domeButtons & BTN_TRIGGER) && (domeButtons & BTN_PUSH1)) {
            return "L_TRIG+L_BTN1";
        }
        if ((domeButtons & BTN_TRIGGER) && (domeButtons & BTN_PUSH2)) {
            return "L_TRIG+L_BTN2";
        }
        if ((domeButtons & BTN_TRIGGER) && (domeButtons & BTN_PUSH3)) {
            return "L_TRIG+L_BTN3";
        }
        if ((domeButtons & BTN_TRIGGER) && (domeButtons & BTN_PUSH4)) {
            return "L_TRIG+L_BTN4";
        }
        
        // Single button presses (DriveRing)
        if (driveButtons & BTN_TRIGGER) return "R_TRIG";
        if (driveButtons & BTN_PUSH1) return "R_BTN1";
        if (driveButtons & BTN_PUSH2) return "R_BTN2";
        if (driveButtons & BTN_PUSH3) return "R_BTN3";
        if (driveButtons & BTN_PUSH4) return "R_BTN4";
        if (driveButtons & BTN_JOY_PRESS) return "R_JOY";
        if (driveButtons & BTN_RECESSED) return "R_RECESS";
        
        // Single button presses (DomeRing)
        if (domeButtons & BTN_TRIGGER) return "L_TRIG";
        if (domeButtons & BTN_PUSH1) return "L_BTN1";
        if (domeButtons & BTN_PUSH2) return "L_BTN2";
        if (domeButtons & BTN_PUSH3) return "L_BTN3";
        if (domeButtons & BTN_PUSH4) return "L_BTN4";
        if (domeButtons & BTN_JOY_PRESS) return "L_JOY";
        if (domeButtons & BTN_RECESSED) return "L_RECESS";
        
        return "";  // No buttons pressed
    }
}
