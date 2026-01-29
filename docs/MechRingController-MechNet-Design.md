# MechMind MechNet Integration Design

**Status**: Active Design  
**Date**: January 28, 2026  
**Purpose**: Define phased integration of MechNet library into MechMind for ESP-NOW communication with multiple remote node types

---

## 1. Overview

This document outlines the **multi-phase integration** of MechNet into MechMind, transforming the Brain into a **MechNet Master node** capable of communicating with diverse ESP-NOW remote devices. This is a comprehensive architectural enhancement, not limited to a single controller type.

### Scope

MechNet integration enables MechMind to communicate with:
- **MechRing Controllers**: Wireless joystick input devices (Phase 4)
- **DataPort Nodes**: Remote LED display boards (Phase 3)
- **Future Remote Nodes**: Panel controllers, sensor arrays, external audio boards, etc.

Each remote node type implements its own message protocol; the Brain (MechNet Master) routes messages to appropriate handlers based on node identity.

### Key Design Principles

- **MechNet as Core Capability**: Brain becomes a general-purpose MechNet Master, not controller-specific
- **Protocol Agnostic Master**: Brain routes messages without understanding node-specific protocols
- **Phased Implementation**: Incremental rollout minimizes risk and validates architecture at each step
- **Provisioning Console**: Administrative interface for secure network configuration
- **Extensible Architecture**: New remote node types integrate via standard patterns (CmdHandler for outbound, message parsing for inbound)

---

## 2. Implementation Phases

### Phase 1: Provisioning Console
**Objective**: Provide administrative interface for MechNet configuration  
**Dependencies**: None  
**Deliverables**:
- Console command handler for MechNet config (network name, channel, PSK, etc.)
- NVS storage for provisioned parameters
- PSK generation and secure storage
- Status display for network configuration

### Phase 2: Brain as MechNet Master
**Objective**: Integrate MechNetMaster into Brain component lifecycle  
**Dependencies**: Phase 1 (provisioning)  
**Deliverables**:
- MechNetMaster instance owned by Brain
- Initialization from provisioned config
- Task loop integration (`brain->task()` calls `mechNetMaster->task()`)
- Factory reset support for network config
- Basic message routing infrastructure

### Phase 3: DataPort Remote Node Support
**Objective**: Support existing remote node type with command routing  
**Dependencies**: Phase 2 (Master node operational)  
**Deliverables**:
- DataPort command handler (routes commands to DataPort via MechNet)
- Message protocol implementation for DataPort LED commands
- Integration with ActionMgr command routing
- Example: `DataPort>LED:R255,G0,B0` routes to DataPort node

### Phase 4: MechRingController Implementation
**Objective**: Wireless controller using two MechRing remote nodes  
**Dependencies**: Phase 2 (Master node operational)  
**Deliverables**:
- MechRingController class (implements Controller interface)
- Message parsing for joystick/button data from rings
- Trigger mapping (button combinations → actions)
- Fault detection via MechNet link monitoring

### Phase 5: Generalized Remote Command Routing
**Objective**: Allow any remote node to receive commands via ActionMgr  
**Dependencies**: Phase 3 (DataPort pattern established)  
**Deliverables**:
- MechNetCmdHandler base class for remote node command handlers
- Auto-discovery of remote node capabilities
- Dynamic command routing based on connected nodes
- Example: `Panel>:OP01` routes to MechNet panel controller if connected, else falls back to serial

---

## 3. Network Architecture

### 3.1 Topology

```
                                    ┌──────────────────┐
                              ┌────→│   MechRing       │ (Remote - Input)
                              │     │  "DriveRing"     │
                              │     └──────────────────┘
                              │
┌──────────────┐              │     ┌──────────────────┐
│   MechMind   │              ├────→│   MechRing       │ (Remote - Input)
│    Brain     │ ESP-NOW ─────┤     │  "DomeRing"      │
│(Master Node) │              │     └──────────────────┘
└──────────────┘              │
                              │     ┌──────────────────┐
                              ├────→│   DataPort       │ (Remote - Display)
                              │     │  "DataPort-A1B2" │
                              │     └──────────────────┘
                              │
                              │     ┌──────────────────┐
                              └────→│   Future Node    │ (Remote - Various)
                                    │  "PanelCtrl"     │
                                    └──────────────────┘
```

- **Brain (MechMind)**: Single MechNetMaster node (1 per network)
- **Remote Nodes**: Up to 6 simultaneous connections (MechNet limit)
- **Protocol Diversity**: Each remote node type uses its own message format
- **Network Name**: Single provisioned name for all nodes (e.g., "R2D2Net")
- **Security**: HMAC-SHA256 authentication + AES-128-CCM encryption (MechNet Phase 1 & 2)

### 3.2 Node Identification

MechNet automatically generates unique node names by appending MAC address bytes:

- **Base Name** (provisioned on remote): `"DriveRing"`, `"DataPort"`, `"PanelCtrl"`
- **Unique Name** (auto-generated): `"DriveRing-A3F2"`, `"DataPort-B7E1"`

Brain uses **node name prefixes** to route messages to appropriate handlers:
- `DriveRing-*` / `DomeRing-*` → MechRingController
- `DataPort-*` → DataPortCmdHandler
- `PanelCtrl-*` → PanelCmdHandler (future)

---

## 4. Brain Architecture Changes (Phase 2)

### 4.1 New Brain Member

**Addition to Brain.h**:

```cpp
class Brain : droid::core::BaseComponent {
private:
    MechNet::MechNetMaster* mechNetMaster;
    
    // ... existing components (domeMgr, driveMgr, actionMgr, etc.)
    
    void processInboundMechNetMessages();
    void routeMessageToHandler(const String& sender, const String& message);
};
```

### 4.2 Initialization Sequence (Brain.cpp)

**Constructor** (before controller instantiation):
```cpp
Brain::Brain(const char* name, droid::core::System* system) : 
    BaseComponent(name, system) {
    
    // Initialize MechNetMaster FIRST (before controllers/handlers need it)
    mechNetMaster = new MechNet::MechNetMaster("NetworkName", LOGGER_STREAM);
    
    // ... existing component construction
}
```

**init()** method:
```cpp
void Brain::init() {
    // Load provisioned MechNet config (Phase 1 output)
    bool mechNetProvisioned = config->getBool(name, CONFIG_KEY_MECHNET_INITIALIZED, false);
    
    if (mechNetProvisioned) {
        String networkName = config->getString(name, CONFIG_KEY_MECHNET_NETWORK_NAME, 
                                               CONFIG_DEFAULT_MECHNET_NETWORK_NAME);
        uint8_t channel = config->getInt(name, CONFIG_KEY_MECHNET_CHANNEL, 
                                          CONFIG_DEFAULT_MECHNET_CHANNEL);
        
        // Load PSK from binary blob storage
        uint8_t psk[32];
        size_t pskLen = config->getBytes(name, CONFIG_KEY_MECHNET_PSK, psk, sizeof(psk));
        
        if (pskLen == 32) {
            // Initialize MechNetMaster
            MechNet::NetworkConfig netCfg;
            netCfg.psk = psk;
            netCfg.pskLen = 32;
            netCfg.channel = channel;
            
            if (!mechNetMaster->begin(&netCfg)) {
                logger->log(name, ERROR, "MechNet initialization failed\n");
            } else {
                logger->log(name, INFO, "MechNet Master initialized: %s (ch %d)\n", 
                           networkName.c_str(), channel);
            }
        } else {
            logger->log(name, ERROR, "Invalid PSK length in config\n");
        }
    } else {
        logger->log(name, WARN, "MechNet not provisioned - network features disabled\n");
    }
    
    // ... existing component initialization
}
```

### 4.3 Task Loop Integration

**Brain::task()** addition:
```cpp
void Brain::task() {
    unsigned long begin = millis();
    
    // MechNet housekeeping (retries, heartbeats, link monitoring)
    if (mechNetMaster) {
        mechNetMaster->task();
    }
    
    // Process inbound MechNet messages
    processInboundMechNetMessages();
    
    // Console input processing
    if (CONSOLE_STREAM != NULL) {
        processConsoleInput(CONSOLE_STREAM);
    }
    
    // Existing component tasks
    for (droid::core::BaseComponent* component : componentList) {
        component->task();
    }
    
    // ... fault checking, etc.
}
```

### 4.4 Message Routing (Phase 2 Implementation)

**Basic routing infrastructure**:
```cpp
void Brain::processInboundMechNetMessages() {
    if (!mechNetMaster) return;
    
    while (mechNetMaster->messageAvailable()) {
        String message = mechNetMaster->nextMessage();
        String sender = mechNetMaster->lastSender();
        
        routeMessageToHandler(sender, message);
    }
}

void Brain::routeMessageToHandler(const String& sender, const String& message) {
    // Phase 4: Route to MechRingController if sender is a ring
    if (sender.startsWith("DriveRing") || sender.startsWith("DomeRing")) {
        if (controller && controller->getType() == Controller::MECHRING) {
            // MechRingController handles its own message parsing
            // (will be implemented in Phase 4)
            return;
        }
    }
    
    // Phase 3: Route to DataPort handler
    if (sender.startsWith("DataPort")) {
        // DataPort sends status/diagnostic messages to Brain
        logger->log("MechNet", INFO, "DataPort [%s]: %s\n", sender.c_str(), message.c_str());
        return;
    }
    
    // Unknown node type
    logger->log("MechNet", DEBUG, "Unhandled message from %s: %s\n", 
               sender.c_str(), message.c_str());
}
```

### 4.5 Factory Reset

**Brain::factoryReset()** addition:
```cpp
void Brain::factoryReset() {
    // ... existing factory reset code
    
    // Clear MechNet provisioning
    config->putBool(name, CONFIG_KEY_MECHNET_INITIALIZED, false);
    config->remove(name, CONFIG_KEY_MECHNET_NETWORK_NAME);
    config->remove(name, CONFIG_KEY_MECHNET_CHANNEL);
    config->remove(name, CONFIG_KEY_MECHNET_PSK);
    
    // ... existing component factory resets
}
```

---

## 5. Phase 3: DataPort Remote Node Support

### 5.1 DataPort Overview

**Purpose**: Remote LED display board showing status information (battery level, WiFi signal, mode indicators)

**Communication Pattern**:
- **Brain → DataPort**: Commands to update LED display (frequent)
- **DataPort → Brain**: Status messages, diagnostics (infrequent)

### 5.2 DataPortCmdHandler Implementation

**Pattern**: Similar to `StreamCmdHandler` but routes to MechNet instead of serial

```cpp
// include/droid/command/DataPortCmdHandler.h
class DataPortCmdHandler : public CmdHandler {
public:
    DataPortCmdHandler(const char* name, droid::core::System* system, 
                       MechNet::MechNetMaster* mechNetMaster);
    
    bool process(const char* device, const char* command) override;
    
private:
    MechNet::MechNetMaster* mechNetMaster;
    char targetNodeName[32];  // e.g., "DataPort-A1B2"
    
    void updateTargetNode();  // Find connected DataPort node
};
```

**Implementation**:
```cpp
bool DataPortCmdHandler::process(const char* device, const char* command) {
    if (strcmp(device, "DataPort") != 0) {
        return false;  // Not for us
    }
    
    // Find connected DataPort node (if not already cached)
    if (targetNodeName[0] == '\0') {
        updateTargetNode();
    }
    
    // Send command to DataPort (best-effort delivery for display updates)
    if (targetNodeName[0] != '\0') {
        bool sent = mechNetMaster->sendTo(targetNodeName, command, true);  // requiresAck=true
        if (!sent) {
            logger->log(name, WARN, "Failed to send to DataPort: %s\n", command);
        }
        return true;
    }
    
    logger->log(name, DEBUG, "No DataPort node connected\n");
    return true;  // Command consumed even if not sent
}

void DataPortCmdHandler::updateTargetNode() {
    // Scan connected nodes for DataPort
    char nodeName[32];
    for (uint8_t i = 0; i < mechNetMaster->connectedNodeCount(); i++) {
        if (mechNetMaster->getConnectedNodeName(i, nodeName, sizeof(nodeName))) {
            if (strncmp(nodeName, "DataPort", 8) == 0) {
                strncpy(targetNodeName, nodeName, sizeof(targetNodeName));
                logger->log(name, INFO, "DataPort node found: %s\n", targetNodeName);
                return;
            }
        }
    }
}
```

**ActionMgr Integration** (Brain.cpp):
```cpp
// In Brain constructor, after actionMgr creation:
actionMgr->addCmdHandler(new droid::command::DataPortCmdHandler("DataPort", system, mechNetMaster));
```

**Usage Example**:
```cpp
// In Action.map or runtime commands:
cmdMap["BatteryLow"] = "DataPort>BATT:15";
cmdMap["StatusOK"] = "DataPort>LED:G255";
```

### 5.3 DataPort Message Protocol

**Format**: **TBD** (to be defined during DataPort firmware implementation)

**Conceptual examples** (actual protocol will be determined in DataPort implementation):
- `BATT:<percentage>` - Update battery display
- `LED:<color>` - Set status LED color
- `TEXT:<message>` - Display text on screen
- `MODE:<mode>` - Update mode indicator

---

## 6. Phase 4: MechRingController Implementation

### 6.1 Controller Design Pattern

**Pattern**: Follow `DualSonyNavController` state management model

```cpp
class MechRingController : public Controller {
public:
    MechRingController(const char* name, droid::core::System* system, 
                       MechNet::MechNetMaster* mechNetMaster);
    
    // BaseComponent lifecycle
    void init() override;
    void task() override;
    void factoryReset() override;
    
    // Controller interface
    int8_t getJoystickPosition(Joystick, Axis) override;
    String getAction() override;
    ControllerType getType() override { return MECHRING; }

private:
    struct RingState {
        char uniqueName[32];           // "DriveRing-A3F2"
        volatile int8_t joystick_x;    // -100 to +100
        volatile int8_t joystick_y;    // -100 to +100
        volatile uint32_t buttonState; // Bitmask
        volatile uint32_t lastMsgTime; // millis()
        volatile bool isConnected;
    };
    
    RingState driveRing;  // Maps to RIGHT joystick
    RingState domeRing;   // Maps to LEFT joystick
    
    MechNet::MechNetMaster* mechNetMaster;  // Reference (not owned)
    std::map<String, String> triggerMap;
    static MechRingController* instance;
    
    void processIncomingMessages();
    void parseRingMessage(const String& msg, RingState& ring);
    void updateRingNodeNames();
    void faultCheck();
    String getTrigger();
};
```

**Key Responsibilities**:
- Poll MechNetMaster for incoming ring messages
- Parse message payloads into RingState structs
- Detect connection loss via MechNet link monitoring
- Map button combinations to action strings via triggerMap

### 6.2 Ring Message Protocol

**Ring → Brain Messages**

**Frequency**: 20-50 Hz (configurable on ring firmware)  
**Reliability**: Unreliable (`requiresAck=false`) for high-frequency joystick data  
**Format**: **TBD** (to be defined during MechRing implementation)

Messages will encode:
- Joystick X/Y positions
- Button state bitmask
- Optional: Battery level, signal strength

**Example conceptual format** (actual format TBD):
```
"J:-45,67|B:0x3A|V:3.7"
```

**Brain → Ring Messages**

**Use Case**: Configuration updates, status commands, LED control  
**Reliability**: Best-effort (`requiresAck=true`) for critical commands  
**Format**: **TBD**

**Example conceptual commands** (actual format TBD):
- `"LED:R255,G0,B0"` - Set ring LED color
- `"VIBRATE:500"` - Haptic feedback pulse
- `"DISABLE"` - Enter failsafe mode

**Note**: Command protocol will be defined in coordination with MechRing firmware implementation.

### 6.3 Controller Task Flow

**Initialization (`init()`)**

```cpp
void MechRingController::init() {
    // Load trigger map defaults + config overrides
    triggerMap.clear();
    #include "settings/MechRingTrigger.map"
    for (const auto& mapEntry : triggerMap) {
        String override = config->getString(name, trigger, action);
        if (override != action) {
            triggerMap[trigger] = override;
        }
    }
    
    // Initialize ring state
    memset(&driveRing, 0, sizeof(RingState));
    memset(&domeRing, 0, sizeof(RingState));
    
    // Note: MechNetMaster already initialized by Brain before controller creation
}
```

**Main Loop (`task()`)**

```cpp
void MechRingController::task() {
    // 1. Process all incoming messages from rings
    processIncomingMessages();
    
    // 2. Update connection status from MechNet
    updateRingNodeNames();
    
    // 3. Fault detection
    faultCheck();
}
```

**Message Processing**:
```cpp
void MechRingController::processIncomingMessages() {
    while (mechNetMaster->messageAvailable()) {
        String msg = mechNetMaster->nextMessage();
        String sender = mechNetMaster->lastSender();
        
        // Route to correct ring state
        if (sender.startsWith("DriveRing")) {
            parseRingMessage(msg, driveRing);
            driveRing.lastMsgTime = millis();
            strncpy(driveRing.uniqueName, sender.c_str(), sizeof(driveRing.uniqueName));
        } else if (sender.startsWith("DomeRing")) {
            parseRingMessage(msg, domeRing);
            domeRing.lastMsgTime = millis();
            strncpy(domeRing.uniqueName, sender.c_str(), sizeof(domeRing.uniqueName));
        }
    }
}
```

**Connection Monitoring**:
```cpp
void MechRingController::updateRingNodeNames() {
    // Query MechNet for connected node status
    driveRing.isConnected = false;
    domeRing.isConnected = false;
    
    char nodeName[32];
    for (uint8_t i = 0; i < mechNetMaster->connectedNodeCount(); i++) {
        if (mechNetMaster->getConnectedNodeName(i, nodeName, sizeof(nodeName))) {
            if (strncmp(nodeName, "DriveRing", 9) == 0) {
                driveRing.isConnected = true;
            } else if (strncmp(nodeName, "DomeRing", 8) == 0) {
                domeRing.isConnected = true;
            }
        }
    }
}
```

**Fault Detection**

```cpp
void MechRingController::faultCheck() {
    bool bothConnected = driveRing.isConnected && domeRing.isConnected;
    
    if (!faultState && !bothConnected) {
        faultState = true;
        logger->log(name, ERROR, "Lost connection to ring controller(s)\n");
    } else if (faultState && bothConnected) {
        faultState = false;
        logger->log(name, INFO, "Ring controllers reconnected\n");
    }
}
```

**Benefits of MechNet Link Monitoring**:
- Automatic heartbeat messages when idle (1s threshold)
- 3s timeout detection (2 missed heartbeats)
- No manual sequence number tracking required
- Automatic reconnection when rings come back online

---

## 7. Phase 5: Generalized Remote Command Routing

### 7.1 Objective

Allow **any** remote node type to receive commands through ActionMgr, similar to how serial devices (Dome, Body, Audio) currently work.

### 7.2 MechNetCmdHandler Base Pattern

**Generic handler for any MechNet remote node**:

```cpp
// include/droid/command/MechNetCmdHandler.h
class MechNetCmdHandler : public CmdHandler {
public:
    MechNetCmdHandler(const char* devicePrefix, droid::core::System* system,
                      MechNet::MechNetMaster* mechNetMaster);
    
    bool process(const char* device, const char* command) override;
    
protected:
    MechNet::MechNetMaster* mechNetMaster;
    String devicePrefix;  // e.g., "Panel", "Sensor", "Audio2"
    char targetNodeName[32];
    
    virtual void updateTargetNode();
};
```

**Usage** (future panel controller example):
```cpp
// In Brain constructor:
actionMgr->addCmdHandler(new MechNetCmdHandler("Panel", system, mechNetMaster));

// Commands route automatically:
// "Panel>:OP01" -> Checks for connected "Panel-*" node -> Sends command
```

### 7.3 Fallback Strategy

**Priority order** for command routing:
1. Check if MechNet remote node connected (e.g., `Panel-A1B2`)
2. If connected, send via MechNet
3. If not connected, fall back to serial (existing `StreamCmdHandler`)

**Example implementation**:
```cpp
// Modified StreamCmdHandler logic:
bool StreamCmdHandler::process(const char* device, const char* command) {
    // Check if device is available via MechNet first
    if (brain->mechNetHasDevice(device)) {
        // Let MechNet handler take it
        return false;
    }
    
    // Fall back to serial stream
    if (strcmp(device, name) == 0) {
        deviceStream->println(command);
        return true;
    }
    return false;
}
```

This allows **seamless migration**: initially use serial Dome controller, later swap to MechNet Dome controller without changing Action.map.

---

## 8. Phase 1: Provisioning & Configuration

### 8.1 Administrative Console Interface

**New Feature**: Provisioning console for network configuration management

**Provisioned Parameters** (stored in NVS config partition):
- **Network Name**: e.g., `"R2D2Net"` (must match all nodes)
- **WiFi Channel**: 1-13 (must match all nodes, avoid WiFi interference)
- **PSK**: 32-byte pre-shared key for HMAC authentication
- **Optional**: ACK timeout (ms), max retries, rate limits

**Console Commands** (conceptual):
```
mechnet config network "R2D2Net"
mechnet config channel 6
mechnet config psk <generate|set HEX>
mechnet save
mechnet status
```

**Security Considerations**:
- PSK generation via `MechNet/docs/api/generate_psk.py` or console command
- PSK never logged or displayed (only hash shown)
- Factory reset clears PSK (requires reprovisioning)

### 8.2 Configuration Storage

**Config Keys** (Brain component):
```cpp
#define CONFIG_KEY_MECHNET_INITIALIZED      "MNInit"
#define CONFIG_KEY_MECHNET_NETWORK_NAME     "MNNetName"
#define CONFIG_KEY_MECHNET_CHANNEL          "MNChannel"
#define CONFIG_KEY_MECHNET_PSK              "MNPSK"
```

**Defaults** (used only if not provisioned):
```cpp
#define CONFIG_DEFAULT_MECHNET_NETWORK_NAME  "MechNet"
#define CONFIG_DEFAULT_MECHNET_CHANNEL       6
// PSK has no default - must be provisioned
```

**Note**: Remote node names (DriveRing, DomeRing, DataPort, etc.) are configured on the remote devices themselves, not in Brain config. Brain discovers them via MechNet announcements.

---

## 9. File Structure

### 9.1 Phase 1 Files (Provisioning Console)

**New Files**:
```
src/droid/brain/MechNetProvisioning.cpp   # Console command handlers for MechNet config
include/droid/brain/MechNetProvisioning.h # Provisioning interface
```

**Modified Files**:
```
src/droid/brain/LocalCmdHandler.cpp       # Add MechNet provisioning commands
```

### 9.2 Phase 2 Files (Brain as Master)

**Modified Files**:
```
src/droid/brain/Brain.h                   # Add MechNetMaster* member
src/droid/brain/Brain.cpp                 # Initialize MechNetMaster, task integration
platformio.ini                            # Add MechNet library dependency
```

### 9.3 Phase 3 Files (DataPort Support)

**New Files**:
```
include/droid/command/
    DataPortCmdHandler.h              # Command handler for DataPort remote node

src/droid/command/
    DataPortCmdHandler.cpp            # Implementation
```

**Modified Files**:
```
src/droid/brain/Brain.cpp             # Add DataPortCmdHandler to actionMgr
```

### 9.4 Phase 4 Files (MechRingController)

**New Files**:
```
include/droid/controller/
    MechRingController.h              # Controller implementation

src/droid/controller/
    MechRingController.cpp            # Controller logic

settings/
    MechRingTrigger.map               # Button-to-action mapping
```

**Modified Files**:
```
src/droid/brain/Brain.cpp             # Add MechRingController instantiation case
include/droid/controller/Controller.h # Add MECHRING to ControllerType enum
```

### 9.5 Phase 5 Files (Generalized Routing)

**New Files**:
```
include/droid/command/
    MechNetCmdHandler.h               # Base class for MechNet remote handlers

src/droid/command/
    MechNetCmdHandler.cpp             # Generic implementation
```

**Modified Files**:
```
src/droid/command/StreamCmdHandler.cpp  # Add MechNet fallback logic
```

### 9.6 Files NOT Needed

The following files from the original design are **eliminated** by using MechNet:

- ~~`include/shared/blering/MechRingESPNow.h`~~ → MechNet handles ESP-NOW
- ~~`src/shared/blering/MechRingESPNow.cpp`~~ → MechNet handles ESP-NOW

---

## 10. Design Rationale

### 10.1 Why MechNet?

| Aspect | Custom Implementation | MechNet Library |
|--------|----------------------|-----------------|
| **Protocol** | Custom binary format + manual CRC | Built-in message serializer with auth |
| **Security** | None (plaintext ESP-NOW) | HMAC-SHA256 + AES-128-CCM |
| **Callbacks** | C-style ISR callbacks, volatile state | Polling API, no ISR complexity |
| **Reliability** | Manual retry/ACK logic | Configurable per-message reliability |
| **Link monitoring** | Custom timeout tracking | Automatic heartbeat/timeout |
| **Sequencing** | Manual sequence numbers | Built-in with wraparound handling |
| **Node discovery** | MAC address pre-config | Auto-discovery via announcements |
| **Maintenance** | ~300-500 lines custom code | ~50 lines using facade API |

### 10.2 Polling vs. Callback Pattern

**Decision**: Use MechNet facade API (polling) instead of core API (callbacks)

**Rationale**:
- **Simpler state management**: No volatile variables or ISR-safe code required
- **Easier debugging**: All processing in main loop context
- **Proven pattern**: Matches existing controller implementations (task-based processing)
- **Message queue**: Built-in 48-message queue (8 per remote × 6 remotes) prevents loss

### 10.3 Phased Implementation Strategy

**Decision**: Implement in 5 distinct phases rather than all-at-once

**Benefits**:
- **Risk mitigation**: Each phase delivers working functionality independently
- **Validation**: Architecture validated at each step before building on it
- **Early value**: DataPort support (Phase 3) delivers value before MechRingController complete
- **Debugging**: Isolates issues to specific phase
- **Flexibility**: Can reprioritize phases based on hardware availability

### 10.4 Protocol Agnostic Master

**Decision**: Brain doesn't parse remote node protocols, just routes messages

**Benefits**:
- **Extensibility**: New remote node types integrate without Brain changes
- **Separation of concerns**: Protocol knowledge stays with handlers/controllers
- **Simplicity**: Brain just matches node name prefixes to handlers

### 10.5 Provisioning Console

**Decision**: Store network config in NVS partition, not hardcoded constants

**Benefits**:
- **Security**: PSK not committed to source control
- **Flexibility**: Single firmware binary for all droids
- **Reconfiguration**: Change network params without recompilation
- **Multi-droid**: Each droid can have unique network name/PSK

---

## 11. Implementation Roadmap

### 11.1 Phase 1: Provisioning Console (Week 1)

**Tasks**:
1. Create `MechNetProvisioning.h/.cpp` with console command handlers
2. Implement config storage for network name, channel, PSK
3. Add PSK generation command (uses ESP32 hardware RNG)
4. Add status display command (show current config, connected nodes)
5. Integrate into `LocalCmdHandler`

**Validation**:
- Console commands work: `mechnet config network "R2D2Net"`
- PSK generates and stores correctly
- Config persists across reboots
- Factory reset clears MechNet config

### 11.2 Phase 2: Brain as Master (Week 1-2)

**Tasks**:
1. Add MechNet dependency to `platformio.ini`
2. Add `MechNetMaster*` member to `Brain.h`
3. Implement initialization in `Brain::init()` using provisioned config
4. Add `mechNetMaster->task()` to `Brain::task()`
5. Implement basic message routing (`processInboundMechNetMessages()`)
6. Add factory reset support

**Validation**:
- Brain initializes as MechNet Master
- Can see announcement messages from test remotes (via MechNetCentral)
- Logging shows connected/disconnected nodes
- Factory reset clears network

### 11.3 Phase 3: DataPort Support (Week 2-3)

**Tasks**:
1. Create `DataPortCmdHandler.h/.cpp`
2. Implement command routing to DataPort remote node
3. Add to `actionMgr` in Brain constructor
4. Define DataPort message protocol (coordinate with DataPort firmware)
5. Test with actual DataPort hardware

**Validation**:
- Commands route correctly: `DataPort>LED:R255`
- DataPort responds to commands
- Link monitoring detects DataPort disconnection
- Action.map commands work

### 11.4 Phase 4: MechRingController (Week 3-4)

**Tasks**:
1. Create `MechRingController.h/.cpp`
2. Implement BaseComponent lifecycle
3. Implement Controller interface
4. Add message parsing for joystick/button data
5. Create `MechRingTrigger.map`
6. Add to Brain controller instantiation
7. Test with MechRing hardware

**Validation**:
- Joystick data received and parsed correctly
- Button triggers fire actions
- Connection loss detected and handled
- Trigger map overrides work

### 11.5 Phase 5: Generalized Routing (Week 4-5)

**Tasks**:
1. Create `MechNetCmdHandler.h/.cpp` base class
2. Refactor `DataPortCmdHandler` to use base class
3. Implement fallback logic in `StreamCmdHandler`
4. Add generic handlers for future nodes
5. Document pattern for new node types

**Validation**:
- Generic handler works for new node types
- Fallback to serial works when MechNet node not connected
- Multiple remote nodes coexist correctly

---

## 12. Testing Strategy

### 12.1 Unit Testing (Per Phase)

**Phase 1 (Provisioning)**:
- Config storage/retrieval
- PSK generation entropy
- Factory reset clearing

**Phase 2 (Master Node)**:
- Initialization with valid/invalid config
- Task loop performance (no blocking)
- Message queue overflow handling

**Phase 3 (DataPort)**:
- Command routing to correct node
- Node discovery on connect/disconnect

**Phase 4 (MechRingController)**:
- Joystick value normalization
- Trigger detection logic
- State management

**Phase 5 (Routing)**:
- Fallback logic
- Multiple handler priority

### 12.2 Integration Testing

**MechNetCentral as Simulation**:

Use `MechNetCentral` to simulate remote nodes before hardware ready:

```bash
# Terminal 1: MechMind Brain
pio run -t upload && pio device monitor

# Terminal 2: Simulated DataPort
cd MechNet/examples/mechnet_central
pio run -e remoteExample -t upload
# Configure: network="R2D2Net", node="DataPort", PSK=<shared key>

# Terminal 3: Simulated DriveRing
# (Second ESP32 device, different node name)
```

**Test Scenarios**:
- Multiple nodes connecting simultaneously
- Node disconnection/reconnection
- Command routing to multiple node types
- Message queue saturation (high-frequency data)

### 12.3 Hardware Testing

**With Actual Devices**:
- MechRing joystick latency (target: <50ms)
- DataPort LED update responsiveness
- Range testing (30-50 feet)
- Interference testing (active WiFi nearby)
- Battery endurance on remote nodes
- Failsafe behavior on connection loss

---

## 13. Open Questions & Future Enhancements

### 13.1 Current Unknowns

1. **MechRing message format**: Exact protocol (TBD during MechRing implementation)
2. **DataPort message format**: LED command syntax (TBD during DataPort implementation)
3. **Update frequency**: Optimal joystick data rate (20Hz? 50Hz?)
4. **Battery monitoring**: Include battery voltage in ring messages?
5. **Haptic feedback**: Brain → Ring commands for vibration?
6. **Max concurrent nodes**: Will 6-node limit be sufficient?

### 13.2 Future Enhancements

- **Node persistence**: Remember node unique names across reboots (avoid rediscovery)
- **Multiple ring pairs**: Support more than 2 rings (e.g., additional input devices)
- **OTA firmware updates**: Push firmware to remote nodes via MechNet
- **Telemetry dashboard**: RSSI, packet loss, latency stats for diagnostics
- **Gesture recognition**: Accelerometer data from rings for motion-based commands
- **Audio streaming**: Send audio clips to remote sound boards
- **Sensor fusion**: Combine data from multiple remote sensors
- **Mesh networking**: Remote nodes relay messages (future MechNet feature)

---

## 14. References

### MechNet Documentation
- [MechNet README](../../MechNet/README.md) - Library overview and quick start
- [MechNetCentral Example](../../MechNet/examples/mechnet_central/) - Reference implementation
- [MechNet Requirements & Design](../../MechNet/docs/design/MechNet-Requirements-and-Design-v2.md) - Protocol details
- [Security Architecture](../../MechNet/docs/design/Security-Architecture-Design.md) - Authentication/encryption

### MechMind Architecture
- [Controller.h](../include/droid/controller/Controller.h) - Base interface
- [DualSonyNavController](../include/droid/controller/DualSonyNavController.h) - Dual-device reference pattern
- [DualRingController](../include/droid/controller/DualRingController.h) - BLE ring implementation
- [Brain.cpp](../src/droid/brain/Brain.cpp) - Component instantiation
- [BaseComponent.h](../include/droid/core/BaseComponent.h) - Lifecycle pattern
- [hardware.config.h](../settings/hardware.config.h) - Configuration defaults

---

## Document History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | Jan 28, 2026 | Initial MechNet-based design (replaces ESP-NOW custom implementation) |
| 2.0 | Jan 28, 2026 | Reframed as multi-phase MechNet integration; Brain becomes general-purpose Master node supporting multiple remote types |
