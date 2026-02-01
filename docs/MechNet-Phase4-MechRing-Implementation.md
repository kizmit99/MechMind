# MechNet Phase 4: MechRingController Implementation Design

**Version:** 1.0  
**Date:** January 31, 2026  
**Status:** Design Ready for Implementation  
**Dependencies:** Phase 2 (Brain as MechNet Master) - COMPLETE, Phase 3 (DataPort) - COMPLETE

---

## 1. Overview

### 1.1 Purpose

Implement MechRingController - a wireless controller using two MechRing remote nodes (DriveRing and DomeRing) communicating with MechMind Brain via MechNet. This phase creates a fully-functional wireless controller option for droid operation.

### 1.2 Goals

- ✅ Implement MechRingController class (implements Controller interface)
- ✅ Parse incoming state messages from ring nodes (joystick positions, button states)
- ✅ Map button combinations to actions via trigger map
- ✅ Leverage existing `setCritical()` timeout mechanism (200ms/10s adaptive)
- ✅ Integrate into Brain component lifecycle and controller selection
- ✅ Support role-based node naming (DriveRing → RIGHT, DomeRing → LEFT)
- ✅ Implement fault detection using MechNet link monitoring

### 1.3 Non-Goals (Future Enhancements)

- ❌ MechRing firmware implementation (separate project)
- ❌ Brain → Ring commands (LED control, config updates)
- ❌ Ring-side haptic feedback
- ❌ Multi-mode ring support (drive/panel/manipulator modes)
- ❌ Advanced trigger combos (long-press, double-click, gestures)

---

## 2. Architecture Overview

### 2.1 Component Hierarchy

```
Brain (BaseComponent)
  ├─ MechNetNode (BaseComponent, MechNet Master)
  ├─ MechRingController (Controller, BaseComponent)
  │   ├─ Receives messages via Brain::routeMessageToHandler()
  │   ├─ Parses DriveRing/DomeRing state updates
  │   ├─ Maps buttons to actions via triggerMap
  │   └─ Uses MechNet for connection monitoring
  ├─ DomeMgr (calls controller->setCritical())
  ├─ DriveMgr (calls controller->setCritical())
  └─ ActionMgr (executes actions from controller->getAction())
```

### 2.2 Data Flow

**Ring → Brain State Updates**:
```
DriveRing firmware
    ↓ MechNet (50 Hz / 1 Hz adaptive)
MechNetNode::task() receives message
    ↓
Brain::processInboundMechNetMessages()
    ↓
Brain::routeMessageToHandler("DriveRing-A3F2", "S:64,-30,02")
    ↓
MechRingController::handleRingMessage()
    ↓
parseRingMessage() updates RingState struct
    ↓
DomeMgr/DriveMgr call getJoystickPosition()
    ↓
Return cached values from RingState
```

**Button Trigger → Action**:
```
MechRingController::getAction()
    ↓
getTrigger() checks button bitmasks
    ↓
triggerMap lookup ("TRIG+BTN1" → "Scream")
    ↓
ActionMgr::fireAction("Scream")
```

**Fault Detection**:
```
MechRingController::task()
    ↓
faultCheck() checks lastMsgTime vs. timeout
    ↓
If timeout: set faultState, disable stickEnable, log error
    ↓
Brain::failsafe() called on ERROR log level
```

---

## 3. File Structure

### 3.1 Files to Create

#### Header Files

**`include/droid/controller/MechRingController.h`**
```cpp
/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
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
```

#### Implementation Files

**`src/droid/controller/MechRingController.cpp`**

Structure:
- Constructor
- init() - Load config, setup triggerMap
- factoryReset() - Clear config, write defaults
- task() - Update connection status, fault check
- logConfig() - Log all config values
- failsafe() - Stop/disable (NOOP, handled by Brain)
- setCritical() - Store isCritical flag
- getJoystickPosition() - Return cached values with deadband/scaling
- getAction() - Check triggers, return action string
- handleRingMessage() - Entry point from Brain
- parseRingMessage() - Parse protocol, update RingState
- updateRingConnectionStatus() - Query MechNet for node status
- faultCheck() - Check timeouts, trigger emergency stop
- getTrigger() - Map button bitmask to trigger string

#### Configuration Files

**`settings/MechRingTrigger.map`**

Button combination → action mappings (similar to DualSonyTrigger.map):

```cpp
// MechRing Trigger Mappings
// Format: triggerMap["<button_combo>"] = "<action>";

// Single buttons (DriveRing - RIGHT joystick)
triggerMap["R_TRIG"] = "VolumeUp";
triggerMap["R_BTN1"] = "Happy";
triggerMap["R_BTN2"] = "Sad";
triggerMap["R_BTN3"] = "DomePAllToggle";
triggerMap["R_BTN4"] = "BodyPAllToggle";
triggerMap["R_RECESS"] = "QuietMode";

// Single buttons (DomeRing - LEFT joystick)
triggerMap["L_TRIG"] = "VolumeDown";
triggerMap["L_BTN1"] = "Scream";
triggerMap["L_BTN2"] = "Wave";
triggerMap["L_BTN3"] = "HoloLightsTogl";
triggerMap["L_BTN4"] = "MusingsToggle";
triggerMap["L_RECESS"] = "FullAwake";

// Combinations (both rings)
triggerMap["R_TRIG+R_BTN1"] = "CantinaDance";
triggerMap["L_TRIG+L_BTN1"] = "Disco";
triggerMap["R_TRIG+L_TRIG"] = "LeiaFullMsg";
triggerMap["R_BTN1+L_BTN1"] = "MarchingAnts";
triggerMap["R_RECESS+L_RECESS"] = "FactoryReset";

// Joystick buttons
triggerMap["R_JOY"] = "StickToggle";
triggerMap["L_JOY"] = "DomeAutoToggle";
triggerMap["R_JOY+L_JOY"] = "SpeedChange";
```

### 3.2 Files to Modify

#### **`include/droid/controller/Controller.h`**

Add `MECHRING` to ControllerType enum:

```cpp
enum ControllerType {
    STUB, DUAL_SONY, DUAL_RING, PS3_BT, PS3_USB, MECHRING};
```

#### **`settings/hardware.config.h`**

Add configuration options:

```cpp
#define CONTROLLER_OPTION_MECHRING      "MechRing"

// Add to CONFIG_DEFAULT_CONTROLLER options comment
// CONFIG_DEFAULT_CONTROLLER can be:
//   CONTROLLER_OPTION_DUALRING, CONTROLLER_OPTION_SONYNAV,
//   CONTROLLER_OPTION_PS3BT, CONTROLLER_OPTION_PS3USB,
//   CONTROLLER_OPTION_MECHRING, or CONTROLLER_OPTION_STUB
```

#### **`src/droid/brain/Brain.cpp`**

**1. Add include**:
```cpp
#include "droid/controller/MechRingController.h"
```

**2. Modify constructor** (add MechRing instantiation):

```cpp
// In Brain::Brain() constructor, add to controller selection:
String whichService = config->getString(name, CONFIG_KEY_BRAIN_CONTROLLER, CONFIG_DEFAULT_CONTROLLER);
logger->log(name, DEBUG, "Requested Controller: %s\n", whichService);
if (whichService == CONTROLLER_OPTION_MECHRING) {
    logger->log(name, DEBUG, "Initializing MechRing\n");
    controller = new droid::controller::MechRingController(CONTROLLER_OPTION_MECHRING, system, mechNetMasterNode);
} else if (whichService == CONTROLLER_OPTION_DUALRING) {
    // ... existing code
```

**3. Modify routeMessageToHandler()** (add ring message routing):

```cpp
void Brain::routeMessageToHandler(const String& sender, const String& message) {
    // Route to MechRingController if sender is a ring
    if (sender.startsWith("DriveRing") || sender.startsWith("DomeRing")) {
        if (controller && controller->getType() == droid::controller::Controller::MECHRING) {
            droid::controller::MechRingController* ringController = 
                static_cast<droid::controller::MechRingController*>(controller);
            ringController->handleRingMessage(sender, message);
            return;
        }
        // If MechRing controller not active, log unhandled
        logger->log(name, DEBUG, "Unhandled ring message (no MechRing controller): %s\n", sender.c_str());
        return;
    }
    
    // Try each registered message handler
    for (droid::message::MsgHandler* handler : inboundMsgHandlers) {
        if (handler->handleMessage(sender, message)) {
            return;  // Handler processed the message
        }
    }
    
    // No handler claimed the message
    logger->log(name, DEBUG, "Unhandled MechNet RX [%s]: %s\n", 
               sender.c_str(), message.c_str());
}
```

**4. Modify factoryReset()** (add MechRing reset):

```cpp
void Brain::factoryReset() {
    // ... existing controller resets
    
    if (controller->getType() == droid::controller::Controller::MECHRING) {
        controller->factoryReset();
    } else {
        droid::controller::MechRingController* mechRingController = 
            new droid::controller::MechRingController(CONTROLLER_OPTION_MECHRING, system, mechNetMasterNode);
        mechRingController->init();
        mechRingController->factoryReset();
    }
    
    // ... rest of existing code
}
```

#### **`settings/LoggerLevels.config.h`**

Add MechRing log level:

```cpp
LOGGER->setLogLevel("MechRing", DEBUG);
```

---

## 4. Detailed Component Design

### 4.1 MechRingController Class

#### Member Variables

```cpp
// Ring state structures
RingState driveRing;  // Maps to RIGHT joystick (body/drive control)
RingState domeRing;   // Maps to LEFT joystick (dome/head control)

// References to system components
droid::network::MechNetNode* mechNetNode;  // For connection status queries

// Configuration (loaded from NVS)
std::map<String, String> triggerMap;      // Button combo → action mappings
uint32_t activeTimeout;                    // 200ms default (emergency stop)
uint32_t inactiveTimeout;                  // 10000ms default (idle disconnect)
int8_t deadbandX;                          // Joystick X deadband (16 default)
int8_t deadbandY;                          // Joystick Y deadband (16 default)

// Runtime state
bool isCritical;                           // Set by setCritical()
bool faultState;                           // Emergency stop triggered
static MechRingController* instance;       // Singleton pattern (like other controllers)
```

#### RingState Structure

```cpp
struct RingState {
    char uniqueName[32];           // "DriveRing-A3F2" (MechNet unique name)
    volatile int8_t joystick_x;    // -128 to +127 (from ring message)
    volatile int8_t joystick_y;    // -128 to +127 (from ring message)
    volatile uint32_t buttonState; // 8-bit bitmask (stored as uint32 for alignment)
    volatile uint32_t lastMsgTime; // millis() when last message received
    volatile uint16_t batteryVoltage; // mV (e.g., 3750 = 3.75V)
    volatile bool isConnected;     // true if node in MechNet connectedNodes list
};
```

**Why volatile**: Values updated in message handler, read in controller methods (potential threading concerns, though ESP32 Arduino is single-threaded in practice).

#### Button Bitmask Constants

```cpp
// In MechRingController.cpp (private constants)
namespace {
    const uint8_t BTN_JOY_PRESS   = 0x01;  // Bit 0
    const uint8_t BTN_TRIGGER     = 0x02;  // Bit 1
    const uint8_t BTN_PUSH1       = 0x04;  // Bit 2
    const uint8_t BTN_PUSH2       = 0x08;  // Bit 3
    const uint8_t BTN_PUSH3       = 0x10;  // Bit 4
    const uint8_t BTN_PUSH4       = 0x20;  // Bit 5
    const uint8_t BTN_RECESSED    = 0x40;  // Bit 6
}
```

### 4.2 Configuration Keys

```cpp
// In MechRingController.cpp
#define CONFIG_KEY_MECHRING_ACTIVE_TIMEOUT      "activeTimeout"
#define CONFIG_KEY_MECHRING_INACTIVE_TIMEOUT    "inactiveTimeout"
#define CONFIG_KEY_MECHRING_DEADBAND_X          "DeadbandX"
#define CONFIG_KEY_MECHRING_DEADBAND_Y          "DeadbandY"

#define CONFIG_DEFAULT_MECHRING_ACTIVE_TIMEOUT   200
#define CONFIG_DEFAULT_MECHRING_INACTIVE_TIMEOUT 10000
#define CONFIG_DEFAULT_MECHRING_DEADBAND         16
```

### 4.3 Key Method Implementations

#### **Constructor**

```cpp
MechRingController::MechRingController(const char* name, droid::core::System* system,
                                       droid::network::MechNetNode* mechNetNode) :
    Controller(name, system),
    mechNetNode(mechNetNode) {
    
    if (MechRingController::instance != NULL) {
        logger->log(name, FATAL, "Constructor called more than once!\n");
        while (1);  // Halt (singleton constraint)
    }
    MechRingController::instance = this;
    
    // Initialize RingState structs
    memset(&driveRing, 0, sizeof(RingState));
    memset(&domeRing, 0, sizeof(RingState));
}
```

#### **init()**

```cpp
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
```

#### **factoryReset()**

```cpp
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
```

#### **task()**

```cpp
void MechRingController::task() {
    // Update connection status from MechNet
    updateRingConnectionStatus();
    
    // Check for timeouts
    faultCheck();
}
```

#### **logConfig()**

```cpp
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
```

#### **failsafe()**

```cpp
void MechRingController::failsafe() {
    // NOOP - Brain handles emergency stop via droidState->stickEnable = false
}
```

#### **setCritical()**

```cpp
void MechRingController::setCritical(bool isCritical) {
    this->isCritical = isCritical;
}
```

#### **getJoystickPosition()**

```cpp
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
```

#### **getAction()**

```cpp
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
```

#### **handleRingMessage()** (called by Brain)

```cpp
void MechRingController::handleRingMessage(const String& sender, const String& message) {
    // Route to correct ring based on sender prefix
    if (sender.startsWith("DriveRing")) {
        parseRingMessage(message, driveRing);
        strncpy(driveRing.uniqueName, sender.c_str(), sizeof(driveRing.uniqueName) - 1);
    } else if (sender.startsWith("DomeRing")) {
        parseRingMessage(message, domeRing);
        strncpy(domeRing.uniqueName, sender.c_str(), sizeof(domeRing.uniqueName) - 1);
    } else {
        logger->log(name, WARN, "Unknown ring sender: %s\n", sender.c_str());
    }
}
```

#### **parseRingMessage()**

```cpp
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
```

#### **updateRingConnectionStatus()**

```cpp
void MechRingController::updateRingConnectionStatus() {
    // Query MechNet for connected nodes
    driveRing.isConnected = false;
    domeRing.isConnected = false;
    
    if (!mechNetNode || !mechNetNode->isInitialized()) {
        return;
    }
    
    // Check if ring nodes are in connected list
    // Note: MechNetNode tracks connection via link monitoring
    driveRing.isConnected = mechNetNode->nodeConnected("DriveRing");
    domeRing.isConnected = mechNetNode->nodeConnected("DomeRing");
}
```

**Note**: Assumes `MechNetNode::nodeConnected(prefix)` exists. If not, use:

```cpp
void MechRingController::updateRingConnectionStatus() {
    driveRing.isConnected = (strlen(driveRing.uniqueName) > 0) && 
                            mechNetNode->nodeConnected(driveRing.uniqueName);
    domeRing.isConnected = (strlen(domeRing.uniqueName) > 0) && 
                           mechNetNode->nodeConnected(domeRing.uniqueName);
}
```

#### **faultCheck()**

```cpp
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
```

#### **getTrigger()**

```cpp
String MechRingController::getTrigger() {
    uint8_t driveButtons = driveRing.buttonState & 0xFF;
    uint8_t domeButtons = domeRing.buttonState & 0xFF;
    
    // Priority order: combinations, then single buttons
    
    // Check for both-ring combinations first
    if ((driveButtons & BTN_TRIGGER) && (domeButtons & BTN_TRIGGER)) {
        return "R_TRIG+L_TRIG";
    }
    if ((driveButtons & BTN_PUSH1) && (domeButtons & BTN_PUSH1)) {
        return "R_BTN1+L_BTN1";
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
    // Add more DriveRing combos as needed...
    
    // Check DomeRing (LEFT) combinations
    if ((domeButtons & BTN_TRIGGER) && (domeButtons & BTN_PUSH1)) {
        return "L_TRIG+L_BTN1";
    }
    // Add more DomeRing combos as needed...
    
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
```

**Design Note**: Priority order ensures combos are detected before single buttons. Combos must check both buttons simultaneously (bitwise AND), not just presence.

---

## 5. Integration Points

### 5.1 Brain Lifecycle Integration

**Initialization Order** (in `Brain::Brain()` constructor):
1. MechNetNode created first
2. Controller instantiated (MechRingController receives MechNetNode reference)
3. componentList registration (MechRingController added)
4. Brain::init() calls controller->init()

**Task Loop** (`Brain::task()`):
1. Process inbound MechNet messages → routes to MechRingController
2. Call component->task() for all components (includes MechRingController)
3. DomeMgr/DriveMgr call controller->getJoystickPosition()
4. ActionMgr calls controller->getAction()

**Factory Reset**:
- Brain::factoryReset() must handle all controller types
- Create temporary MechRingController if not active, call factoryReset()

### 5.2 MechNetNode Dependency

MechRingController requires MechNetNode for:
- Connection status queries (`nodeConnected()`)
- Link monitoring (automatic via MechNet)
- No direct message sending (one-way: ring → brain)

**Null Safety**: Always check `mechNetNode != nullptr` and `mechNetNode->isInitialized()` before use.

### 5.3 DroidState Integration

MechRingController modifies `droidState`:
- `droidState->stickEnable = false` on timeout (emergency stop)
- Reads `droidState->stickEnable` for connection recovery decisions

---

## 6. Testing Strategy

### 6.1 Unit Testing (Without Hardware)

**Test harness**: Create mock ring message sender

```cpp
// In test sketch or console command
void sendMockRingMessage(const char* sender, const char* message) {
    brain->routeMessageToHandler(sender, message);
}

// Test cases
sendMockRingMessage("DriveRing-TEST", "S:0,0,00");      // Centered
sendMockRingMessage("DriveRing-TEST", "S:64,-30,02");   // Moved + trigger
sendMockRingMessage("DomeRing-TEST", "S:-50,100,01");   // Left + joystick press
```

**Verify**:
- Message parsing (check `getJoystickPosition()` returns correct values)
- Deadband application (values < threshold → 0)
- Scaling (-128/+127 → -100/+100)
- Button trigger mapping (verify `getAction()` returns expected action)

### 6.2 Integration Testing (With MechNet Simulation)

**Requirements**:
- Second ESP32 flashed with MechNet Remote node
- Simple sketch sending state updates at 50 Hz
- MechMind Brain with MechRingController selected

**Test Scenarios**:
1. **Connection establishment**: Ring announces, Brain accepts
2. **Message reception**: Brain logs incoming messages
3. **Joystick control**: Motor commands respond to joystick input
4. **Button triggers**: Actions fire on button press
5. **Timeout detection**: Unplug ring, verify 200ms emergency stop
6. **Reconnection**: Plug ring back in, verify recovery message

### 6.3 Hardware Validation (With Actual MechRings)

**Full system test**:
1. Provision two MechRings (DriveRing, DomeRing)
2. Power on Brain, wait for connection
3. Test joystick control (drive, dome rotation)
4. Test button actions (panels, sounds, lights)
5. Test emergency stop (power off ring during motion)
6. Test battery reporting (check logs for voltage updates)
7. Test idle mode (leave rings untouched, verify 1 Hz heartbeat)
8. Test range (30+ feet typical for ESP-NOW)

---

## 7. Implementation Sequence

### Phase A: Core Structure (30 minutes)

1. Create header file (`MechRingController.h`)
2. Create implementation skeleton (`MechRingController.cpp`)
3. Add `MECHRING` to `Controller.h` enum
4. Add `CONTROLLER_OPTION_MECHRING` to `hardware.config.h`
5. Verify compilation (no linkage yet)

### Phase B: Basic Parsing (45 minutes)

1. Implement constructor, destructor
2. Implement `parseRingMessage()` with protocol parsing
3. Implement `handleRingMessage()` routing logic
4. Add Brain integration (include, instantiation, routing)
5. Test with mock messages (serial console)

### Phase C: Joystick Interface (30 minutes)

1. Implement `getJoystickPosition()` with deadband and scaling
2. Implement `setCritical()`
3. Test joystick values in DomeMgr/DriveMgr
4. Verify motor response to ring input

### Phase D: Button Triggers (45 minutes)

1. Create `MechRingTrigger.map`
2. Implement `getTrigger()` button mapping logic
3. Implement `getAction()` trigger lookup
4. Test action execution (verify ActionMgr receives actions)

### Phase E: Fault Detection (30 minutes)

1. Implement `updateRingConnectionStatus()`
2. Implement `faultCheck()` with timeout logic
3. Test emergency stop (delay message sending)
4. Test reconnection recovery

### Phase F: Configuration & Lifecycle (30 minutes)

1. Implement `init()` config loading
2. Implement `factoryReset()` default writing
3. Implement `logConfig()` logging
4. Implement `task()` housekeeping
5. Add Brain factory reset integration

### Phase G: Polish & Testing (60 minutes)

1. Add logging at appropriate levels
2. Test all trigger combinations
3. Validate battery voltage reporting
4. Performance profiling (verify <1ms loop impact)
5. Documentation review

**Total Estimated Time**: 4-5 hours

---

## 8. Success Criteria

### 8.1 Functional Requirements

- ✅ MechRingController compiles and links
- ✅ Brain instantiates MechRingController when configured
- ✅ Incoming ring messages parsed correctly
- ✅ Joystick values mapped to motor commands
- ✅ Button combinations trigger actions
- ✅ Emergency stop activates within 200ms of ring disconnect
- ✅ Connection recovery logged and handled
- ✅ Battery voltage reported in logs
- ✅ Configuration persists across reboots
- ✅ Factory reset clears all MechRing config

### 8.2 Performance Requirements

- ✅ Message parsing < 100µs per message
- ✅ Controller task() < 1ms per cycle
- ✅ No heap fragmentation (use stack for parsing)
- ✅ Supports 50 Hz sustained message rate

### 8.3 Safety Requirements

- ✅ Emergency stop within 200ms when `isCritical == true`
- ✅ No automatic stick re-enable after timeout (user must confirm)
- ✅ Fault state persists until both rings reconnected
- ✅ Timeout detection works when active or idle

---

## 9. Known Limitations & Future Enhancements

### 9.1 Current Limitations

- **One-way communication**: Brain cannot send commands to rings (LED, vibration)
- **No ring configuration**: Update rate, deadzone hardcoded in ring firmware
- **Basic trigger mapping**: No long-press, double-click, or gesture support
- **Fixed roles**: DriveRing/DomeRing assignment in provisioning, not runtime
- **No battery alerts**: Brain only logs voltage, doesn't trigger actions

### 9.2 Future Enhancements (Out of Scope)

**Phase 5: Brain → Ring Commands**
- LED control for user feedback
- Configuration updates (rate, deadzone)
- Haptic feedback (if hardware added)

**Phase 6: Advanced Triggers**
- Long-press detection (e.g., hold button 2s for different action)
- Double-click/triple-click sequences
- Gesture recognition (joystick swipes)

**Phase 7: Multi-Mode Support**
- Switch ring roles dynamically (drive mode, panel mode, manipulator mode)
- Mode indicator via LED color
- Per-mode trigger maps

---

## 10. Dependencies & Prerequisites

### 10.1 Required Components

- **MechNet library** v0.2.0+ (Phase 1 & 2 security complete)
- **MechNetNode** integrated in Brain (Phase 2 complete)
- **MsgHandler architecture** (Phase 3 complete)
- **Brain message routing** (Phase 3 complete)

### 10.2 Configuration Requirements

**MechNet provisioning** (via console):
- Network name configured
- PSK provisioned
- WiFi channel set (must match rings)
- MechNet master initialized

**Controller selection**:
- `CONFIG_DEFAULT_CONTROLLER` or runtime config set to `CONTROLLER_OPTION_MECHRING`

### 10.3 Hardware Requirements (for testing)

- MechMind ESP32 board (Brain)
- 2x MechRing ESP32 boards (DriveRing, DomeRing) with firmware
- USB cables for provisioning
- Motor drivers for physical droid testing

---

## 11. Risk Mitigation

### 11.1 Technical Risks

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Message parsing errors | Medium | High | Extensive input validation, unit tests |
| Timeout false positives | Low | High | Proven pattern from DualSonyNavController |
| MechNet link issues | Low | Medium | Redundant timeout layers (app + MechNet) |
| Button mapping conflicts | Medium | Low | Clear trigger priority in getTrigger() |
| Performance degradation | Low | Medium | Profiling, optimize parseRingMessage() |

### 11.2 Integration Risks

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Brain routing breaks existing controllers | Low | High | Test all controller types after Brain changes |
| MechNetNode API incompatibility | Low | Medium | Verify nodeConnected() method exists |
| Factory reset regression | Medium | Low | Test factory reset with all controller types |
| Singleton constraint violation | Low | High | Assert in constructor (existing pattern) |

---

## 12. References

- **Protocol Spec**: [MechRing-Protocol-Specification.md](MechRing-Protocol-Specification.md)
- **MechNet Library**: [MechNet README](../../MechNet/README.md)
- **Phase 3 Design**: [MechNet-Phase3-DataPort-Implementation.md](MechNet-Phase3-DataPort-Implementation.md)
- **DualSonyNavController**: Reference implementation for timeout pattern
- **Controller Interface**: [Controller.h](../include/droid/controller/Controller.h)

---

## 13. Appendix: Example Console Session

```
> help controller
Available controllers:
  - StubController (testing only)
  - DualSonyNavController (Bluetooth PS3 Nav)
  - DualRingController (BLE ring controllers)
  - PS3BtController (Bluetooth PS3 gamepad)
  - PS3UsbController (USB PS3 gamepad)
  - MechRingController (MechNet wireless rings)

> setconfig Brain Controller MechRing
✓ Configuration updated

> restart
Restarting...

[Brain R2D2] Requested Controller: MechRing
[Brain R2D2] Initializing MechRing
[MechRing] init - called
[MechRing] Config activeTimeout = 200
[MechRing] Config inactiveTimeout = 10000
[MechRing] Config DeadbandX = 16
[MechRing] Config DeadbandY = 16
[MechRing] Config R_TRIG = VolumeUp
[MechRing] Config L_TRIG = VolumeDown
... (all trigger mappings logged)

[MechNet] Node connected: DriveRing-A3F2
[MechNet] Node connected: DomeRing-B81D
[MechRing] Ring controllers reconnected

[MechRing] Parsed DriveRing-A3F2: X=64, Y=-30, BTN=0x02
[DriveMgr] Joystick: X=50, Y=-23
[MechRing] Trigger: R_TRIG -> Action: VolumeUp
[ActionMgr] Executing: VolumeUp
[Audio] Volume: 7/10

... (droid operation)

[MechRing] ERROR: EMERGENCY STOP: DriveRing timeout (200ms)
[Brain R2D2] Failsafe triggered - motors stopped
```

---

**Status**: Design complete, ready for implementation. Estimated 4-5 hours development time.
