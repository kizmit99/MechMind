# MechMind MechNet Phase 2 Integration Design

**Version:** 1.0  
**Date:** January 29, 2026  
**Status:** Design Ready

---

## 1. Overview

### 1.1 Purpose

Integrate MechNet library into MechMind Brain as an optional Master node component, enabling secure ESP-NOW communication with remote nodes. This phase establishes the foundation for wireless controller support (Phase 4) and remote device command routing (Phase 3, 5).

### 1.2 Goals

- ✅ Add MechNetMasterNode as optional BaseComponent
- ✅ Implement provisioning console commands (mechnet-status, mechnet-provision)
- ✅ Store network configuration in NVS (network name, channel, PSK)
- ✅ Enable/disable MechNet support via configuration
- ✅ Integrate into Brain component lifecycle (init, task, factoryReset)
- ✅ Implement basic inbound message routing infrastructure
- ✅ Add MechNetOutboundHandler for command routing to remote nodes
- ✅ Remove deprecated ESPNowCmdHandler
- ✅ Validate with MechNetCentral simulation

### 1.3 Non-Goals (Future Phases)

- ❌ MechRingController implementation (Phase 4)
- ❌ DataPort command handler (Phase 3)
- ❌ Node-specific message protocols
- ❌ Automatic configuration or PSK generation
- ❌ OTA firmware updates to remote nodes

---

## 2. Architecture Overview

### 2.1 Component Hierarchy

```
Brain (BaseComponent)
  ├─ MechNetMasterNode (optional BaseComponent)
  │   └─ MechNet::MechNetMaster (facade API)
  ├─ DomeMgr
  ├─ DriveMgr
  ├─ ActionMgr
  │   ├─ MechNetOutboundHandler (if MechNet enabled)
  │   ├─ StreamCmdHandler ("Dome")
  │   ├─ StreamCmdHandler ("Body")
  │   └─ ... other handlers
  └─ ... other components
```

### 2.2 Data Flow

**Provisioning Flow**:
```
Admin Console
    ↓ mechnet-provision wizard
Config (NVS)
    ↓ Brain::init()
MechNetMasterNode::init()
    ↓ MechNet::MechNetMaster::begin()
ESP-NOW Network Active
```

**Outbound Command Flow** (Brain → Remote Node):
```
ActionMgr::queueCommand("DataPort", "LED:R255")
    ↓
MechNetOutboundHandler::process()
    ↓ findNodeByPrefix("DataPort") → "DataPort-A1B2"
MechNetMasterNode::sendCommand("DataPort-A1B2", "LED:R255")
    ↓
MechNet::MechNetMaster::sendTo()
    ↓
ESP-NOW → Remote Node
```

**Inbound Message Flow** (Remote Node → Brain):
```
ESP-NOW Message Received
    ↓
MechNet::MechNetMaster (receive queue)
    ↓ Brain::task()
MechNetMasterNode::task()
    ↓ Brain::processInboundMechNetMessages()
Brain::routeMessageToHandler("DataPort-A1B2", "STATUS:OK")
    ↓ (Phase 2: just log unknown messages)
Logger
```

---

## 3. Component Design: MechNetMasterNode

### 3.1 Class Definition

**File**: `include/droid/network/MechNetMasterNode.h`

```cpp
/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 */

#pragma once
#include "droid/core/BaseComponent.h"
#include <facade/MechNetMaster.h>

namespace droid::network {
    class MechNetMasterNode : public droid::core::BaseComponent {
    public:
        MechNetMasterNode(const char* name, droid::core::System* system);
        ~MechNetMasterNode();

        // BaseComponent lifecycle
        void init() override;
        void factoryReset() override;
        void task() override;
        void logConfig() override;
        void failsafe() override;

        // Public API for handlers and Brain
        bool sendCommand(const char* nodeName, const char* command, bool requiresAck = true);
        bool findNodeByPrefix(const char* prefix, char* nodeNameOut, size_t buflen);
        uint8_t connectedNodeCount();
        bool getConnectedNodeName(uint8_t index, char* nameOut, size_t buflen);
        bool messageAvailable();
        String nextMessage();
        String lastSenderName();
        bool isInitialized() { return initialized; }

    private:
        MechNet::MechNetMaster* mechNetMaster;
        bool initialized;
    };
}
```

### 3.2 Implementation

**File**: `src/droid/network/MechNetMasterNode.cpp`

```cpp
#include "droid/network/MechNetMasterNode.h"

#define CONFIG_KEY_MECHNET_ENABLED       "MNEnable"
#define CONFIG_KEY_MECHNET_INITIALIZED   "MNInit"
#define CONFIG_KEY_MECHNET_NETWORK_NAME  "MNNetName"
#define CONFIG_KEY_MECHNET_CHANNEL       "MNChannel"
#define CONFIG_KEY_MECHNET_PSK           "MNPSK"

#define CONFIG_DEFAULT_MECHNET_ENABLED      false
#define CONFIG_DEFAULT_MECHNET_INITIALIZED  false
#define CONFIG_DEFAULT_MECHNET_NETWORK_NAME "MechNet"
#define CONFIG_DEFAULT_MECHNET_CHANNEL      6

namespace droid::network {
    
    MechNetMasterNode::MechNetMasterNode(const char* name, droid::core::System* system) :
        BaseComponent(name, system),
        mechNetMaster(nullptr),
        initialized(false) {
    }

    MechNetMasterNode::~MechNetMasterNode() {
        if (mechNetMaster) {
            delete mechNetMaster;
        }
    }

    void MechNetMasterNode::init() {
        bool enabled = config->getBool(name, CONFIG_KEY_MECHNET_ENABLED, 
                                       CONFIG_DEFAULT_MECHNET_ENABLED);
        bool provisioned = config->getBool(name, CONFIG_KEY_MECHNET_INITIALIZED, 
                                           CONFIG_DEFAULT_MECHNET_INITIALIZED);

        if (!enabled) {
            logger->log(name, INFO, "MechNet disabled in config\n");
            return;
        }

        if (!provisioned) {
            logger->log(name, WARN, "MechNet enabled but not provisioned - run mechnet-provision\n");
            return;
        }

        // Load provisioned configuration
        String networkName = config->getString(name, CONFIG_KEY_MECHNET_NETWORK_NAME, 
                                               CONFIG_DEFAULT_MECHNET_NETWORK_NAME);
        uint8_t channel = config->getInt(name, CONFIG_KEY_MECHNET_CHANNEL, 
                                         CONFIG_DEFAULT_MECHNET_CHANNEL);
        String pskHex = config->getString(name, CONFIG_KEY_MECHNET_PSK, "");

        // Validate PSK (must be 64 hex characters = 32 bytes)
        if (pskHex.length() != 64) {
            logger->log(name, ERROR, "Invalid PSK length: %d (expected 64 hex chars)\n", 
                       pskHex.length());
            return;
        }

        // Convert hex string to bytes
        uint8_t psk[32];
        for (size_t i = 0; i < 32; i++) {
            char byteStr[3] = {pskHex[i*2], pskHex[i*2 + 1], 0};
            psk[i] = (uint8_t)strtol(byteStr, NULL, 16);
        }

        // Create MechNetMaster instance
        // Note: MechNetMaster constructor requires network name, logger is optional
        mechNetMaster = new MechNet::MechNetMaster(networkName.c_str(), logger);

        // Configure MechNet
        MechNet::NetworkConfig netCfg;
        netCfg.psk = psk;
        netCfg.pskLen = 32;
        netCfg.channel = channel;

        // Initialize MechNet
        if (!mechNetMaster->begin(&netCfg)) {
            logger->log(name, ERROR, "MechNet initialization failed\n");
            delete mechNetMaster;
            mechNetMaster = nullptr;
            return;
        }

        initialized = true;
        logger->log(name, INFO, "MechNet Master initialized: %s (channel %d)\n", 
                   networkName.c_str(), channel);
    }

    void MechNetMasterNode::factoryReset() {
        config->putBool(name, CONFIG_KEY_MECHNET_ENABLED, CONFIG_DEFAULT_MECHNET_ENABLED);
        config->putBool(name, CONFIG_KEY_MECHNET_INITIALIZED, CONFIG_DEFAULT_MECHNET_INITIALIZED);
        config->putString(name, CONFIG_KEY_MECHNET_NETWORK_NAME, CONFIG_DEFAULT_MECHNET_NETWORK_NAME);
        config->putInt(name, CONFIG_KEY_MECHNET_CHANNEL, CONFIG_DEFAULT_MECHNET_CHANNEL);
        config->remove(name, CONFIG_KEY_MECHNET_PSK);
    }

    void MechNetMasterNode::task() {
        if (!mechNetMaster) return;
        
        // MechNet housekeeping (retries, heartbeats, link monitoring)
        mechNetMaster->task();
    }

    void MechNetMasterNode::logConfig() {
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_MECHNET_ENABLED, 
                   config->getString(name, CONFIG_KEY_MECHNET_ENABLED, ""));
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_MECHNET_INITIALIZED, 
                   config->getString(name, CONFIG_KEY_MECHNET_INITIALIZED, ""));
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_MECHNET_NETWORK_NAME, 
                   config->getString(name, CONFIG_KEY_MECHNET_NETWORK_NAME, ""));
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_MECHNET_CHANNEL, 
                   config->getString(name, CONFIG_KEY_MECHNET_CHANNEL, ""));
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_MECHNET_PSK, 
                   config->getString(name, CONFIG_KEY_MECHNET_PSK, ""));
    }

    void MechNetMasterNode::failsafe() {
        // No action needed - MechNet has no motors/actuators to stop
    }

    bool MechNetMasterNode::sendCommand(const char* nodeName, const char* command, bool requiresAck) {
        if (!mechNetMaster) return false;
        return mechNetMaster->sendTo(nodeName, command, requiresAck);
    }

    bool MechNetMasterNode::findNodeByPrefix(const char* prefix, char* nodeNameOut, size_t buflen) {
        if (!mechNetMaster) return false;

        char nodeName[32];
        for (uint8_t i = 0; i < mechNetMaster->connectedNodeCount(); i++) {
            if (mechNetMaster->getConnectedNodeName(i, nodeName, sizeof(nodeName))) {
                if (strncmp(nodeName, prefix, strlen(prefix)) == 0) {
                    strncpy(nodeNameOut, nodeName, buflen);
                    nodeNameOut[buflen - 1] = '\0';
                    return true;
                }
            }
        }
        return false;
    }

    uint8_t MechNetMasterNode::connectedNodeCount() {
        if (!mechNetMaster) return 0;
        return mechNetMaster->connectedNodeCount();
    }

    bool MechNetMasterNode::getConnectedNodeName(uint8_t index, char* nameOut, size_t buflen) {
        if (!mechNetMaster) return false;
        return mechNetMaster->getConnectedNodeName(index, nameOut, buflen);
    }

    bool MechNetMasterNode::messageAvailable() {
        if (!mechNetMaster) return false;
        return mechNetMaster->messageAvailable();
    }

    String MechNetMasterNode::nextMessage() {
        if (!mechNetMaster) return "";
        return mechNetMaster->nextMessage();
    }

    String MechNetMasterNode::lastSenderName() {
        if (!mechNetMaster) return "";
        return mechNetMaster->lastSenderName();
    }
}
```

---

## 4. Component Design: MechNetOutboundHandler

### 4.1 Class Definition

**File**: `include/droid/command/MechNetOutboundHandler.h`

```cpp
/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 */

#pragma once
#include "droid/command/CmdHandler.h"
#include "droid/network/MechNetMasterNode.h"

namespace droid::command {
    class MechNetOutboundHandler : public CmdHandler {
    public:
        MechNetOutboundHandler(const char* name, droid::core::System* system, 
                               droid::network::MechNetMasterNode* masterNode);

        bool process(const char* device, const char* command) override;

    private:
        droid::network::MechNetMasterNode* mechNetMasterNode;
    };
}
```

### 4.2 Implementation

**File**: `src/droid/command/MechNetOutboundHandler.cpp`

```cpp
#include "droid/command/MechNetOutboundHandler.h"

namespace droid::command {
    
    MechNetOutboundHandler::MechNetOutboundHandler(const char* name, 
                                                   droid::core::System* system,
                                                   droid::network::MechNetMasterNode* masterNode) :
        CmdHandler(name, system),
        mechNetMasterNode(masterNode) {
    }

    bool MechNetOutboundHandler::process(const char* device, const char* command) {
        if (!mechNetMasterNode || !mechNetMasterNode->isInitialized()) {
            return false;  // MechNet not available
        }

        // Try to find a connected node matching the device prefix
        char nodeName[32];
        if (mechNetMasterNode->findNodeByPrefix(device, nodeName, sizeof(nodeName))) {
            logger->log(name, DEBUG, "Routing %s>%s to node %s\n", 
                       device, command, nodeName);
            
            bool sent = mechNetMasterNode->sendCommand(nodeName, command, true);
            if (!sent) {
                logger->log(name, WARN, "Failed to send to %s: %s\n", nodeName, command);
            }
            return true;  // Command consumed (even if send failed)
        }

        // No matching node found, let other handlers try
        return false;
    }
}
```

---

## 5. Console Commands

### 5.1 mechnet-status

**Purpose**: Display current MechNet configuration and connected nodes

**Syntax**: `mechnet-status`

**Output Example**:
```
MechNet Status
==============
Enabled:      Yes
Provisioned:  Yes
Network Name: R2D2Net
WiFi Channel: 6
PSK:          abc123def456789012345678901234567890123456789012345678901234
Initialized:  Yes

Connected Nodes (2):
  1. DriveRing-A3F2  (RSSI: -45 dBm)
  2. DataPort-B7E1   (RSSI: -52 dBm)
```

**Implementation Location**: `src/droid/brain/LocalCmdHandler.cpp`

### 5.2 mechnet-provision

**Purpose**: Wizard-guided provisioning of MechNet configuration

**Syntax**: `mechnet-provision`

**Wizard Flow**:
```cpp
void LocalCmdHandler::mechnetProvisionWizard(SmartCLI::Console& console) {
    console.println("\nMechNet Provisioning Wizard");
    console.println("============================\n");

    // Step 1: Network Name
    String networkName = console.promptString("Network Name", "MechNet");
    if (networkName.length() == 0 || networkName.length() > 32) {
        console.println("Error: Network name must be 1-32 characters");
        return;
    }

    // Step 2: WiFi Channel
    int channel = console.promptInt("WiFi Channel (1-13)", 6);
    if (channel < 1 || channel > 13) {
        console.println("Error: Channel must be 1-13");
        return;
    }

    // Step 3: PSK (hex string)
    String pskHex = console.promptString("Pre-Shared Key (64 hex chars)", "");
    if (pskHex.length() != 64) {
        console.println("Error: PSK must be exactly 64 hex characters (32 bytes)");
        return;
    }
    // Validate hex format
    for (size_t i = 0; i < 64; i++) {
        char c = pskHex[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
            console.println("Error: PSK must contain only hex characters (0-9, a-f, A-F)");
            return;
        }
    }

    // Step 4: Confirm
    console.println("\nConfiguration Summary:");
    console.printf("  Network Name: %s\n", networkName.c_str());
    console.printf("  WiFi Channel: %d\n", channel);
    console.printf("  PSK:          %s\n", pskHex.c_str());
    console.println();

    String confirm = console.promptString("Save configuration? (yes/no)", "no");
    if (confirm != "yes" && confirm != "y") {
        console.println("Provisioning cancelled.");
        return;
    }

    // Save to config
    config->putBool("MechNet", "MNInit", true);
    config->putBool("MechNet", "MNEnable", true);
    config->putString("MechNet", "MNNetName", networkName);
    config->putInt("MechNet", "MNChannel", channel);
    config->putString("MechNet", "MNPSK", pskHex);

    console.println("\n✓ MechNet provisioned successfully!");
    console.println("Restart the system to activate MechNet.");
}
```

**SmartCLI Integration**:
```cpp
// In LocalCmdHandler.cpp, add to command registration:
void LocalCmdHandler::registerConsoleCommands(SmartCLI::Console& console) {
    // ... existing commands ...
    
    console.registerCommand("mechnet-status", [this](SmartCLI::Console& c, const String&) {
        mechnetStatus(c);
    });
    
    console.registerCommand("mechnet-provision", [this](SmartCLI::Console& c, const String&) {
        mechnetProvisionWizard(c);
    });
}
```

---

## 6. Brain Integration

### 6.1 Brain.h Modifications

```cpp
namespace droid::brain {
    class Brain : droid::core::BaseComponent {
    public:
        Brain(const char* name, droid::core::System* system);

        // ... existing methods ...
        
        // Public API for command handlers
        droid::network::MechNetMasterNode* getMechNetMaster() { return mechNetMasterNode; }

    private:
        // ... existing components ...
        
        droid::network::MechNetMasterNode* mechNetMasterNode;
        
        void processInboundMechNetMessages();
        void routeMessageToHandler(const String& sender, const String& message);
    };
}
```

### 6.2 Brain.cpp Constructor

```cpp
Brain::Brain(const char* name, droid::core::System* system) : 
    BaseComponent(name, system),
    mechNetMasterNode(nullptr) {

    // Create MechNetMasterNode FIRST (before controllers/handlers need it)
    mechNetMasterNode = new droid::network::MechNetMasterNode("MechNet", system);

    // ... existing PWMService creation ...

    // ... existing controller creation ...
    // NOTE: Phase 4 will add MechRingController case here
    
    // ... existing motor/audio driver creation ...
    
    // Create managers
    domeMgr = new droid::brain::DomeMgr("DomeMgr", system, controller, domeMotorDriver);
    driveMgr = new droid::brain::DriveMgr("DriveMgr", system, controller, driveMotorDriver);
    actionMgr = new droid::command::ActionMgr("ActionMgr", system, controller);
    audioMgr = new droid::audio::AudioMgr("AudioMgr", system, audioDriver);

    // Register command handlers
    actionMgr->addCmdHandler(new droid::command::CmdLogger("CmdLogger", system));
    
    // Add MechNetOutboundHandler if MechNet enabled
    if (mechNetMasterNode) {
        actionMgr->addCmdHandler(new droid::command::MechNetOutboundHandler(
            "MechNetOut", system, mechNetMasterNode));
    }
    
    actionMgr->addCmdHandler(new droid::command::StreamCmdHandler("Dome", system, DOME_STREAM));
    actionMgr->addCmdHandler(new droid::command::StreamCmdHandler("Body", system, BODY_STREAM));
    actionMgr->addCmdHandler(new droid::audio::AudioCmdHandler("Audio", system, audioMgr));
    actionMgr->addCmdHandler(new droid::brain::LocalCmdHandler("Brain", system, this));
    droid::brain::PanelCmdHandler* panelCmdHandler = new droid::brain::PanelCmdHandler("Panel", system);
    actionMgr->addCmdHandler(panelCmdHandler);

    // NOTE: ESPNowCmdHandler removed in Phase 2

    // Create ConsoleHandler (if CONSOLE_STREAM defined)
    #ifdef CONSOLE_STREAM
        consoleHandler = new ConsoleHandler(*CONSOLE_STREAM, this);
    #else
        consoleHandler = nullptr;
    #endif

    // Setup component list for lifecycle management
    componentList.push_back(pwmService);
    componentList.push_back(mechNetMasterNode);  // Add to lifecycle
    componentList.push_back(controller);
    componentList.push_back(domeMotorDriver);
    componentList.push_back(driveMotorDriver);
    componentList.push_back(domeMgr);
    componentList.push_back(driveMgr);
    componentList.push_back(audioMgr);
    componentList.push_back(audioDriver);
    componentList.push_back(actionMgr);
    componentList.push_back(panelCmdHandler);
}
```

### 6.3 Brain.cpp Task Loop

```cpp
void Brain::task() {
    unsigned long begin = millis();
    
    // Process console input (SmartCLI)
    if (consoleHandler) {
        consoleHandler->process();
    }
    
    // Process inbound MechNet messages (before component tasks)
    processInboundMechNetMessages();
    
    // Component task loop (includes mechNetMasterNode->task())
    for (droid::core::BaseComponent* component : componentList) {
        unsigned long compBegin = millis();
        component->task();
        unsigned long compTime = millis() - compBegin;
        if (compTime > 10) {
            logger->log(component->name, WARN, "subTask took %d millis to execute!\n", compTime);
        }
    }

    if (logger->getMaxLevel() >= ERROR) {
        failsafe();
        logger->clear();
    }
    unsigned long time = millis() - begin;
    if (time > 30) {
        logger->log(name, WARN, "Task took %d millis to execute!\n", time);
    }
}
```

### 6.4 Inbound Message Processing

```cpp
void Brain::processInboundMechNetMessages() {
    if (!mechNetMasterNode || !mechNetMasterNode->isInitialized()) {
        return;
    }
    
    while (mechNetMasterNode->messageAvailable()) {
        String message = mechNetMasterNode->nextMessage();
        String sender = mechNetMasterNode->lastSenderName();
        
        routeMessageToHandler(sender, message);
    }
}

void Brain::routeMessageToHandler(const String& sender, const String& message) {
    // Phase 2: Log all incoming messages (routing logic added in later phases)
    logger->log("MechNet", DEBUG, "RX [%s]: %s\n", sender.c_str(), message.c_str());
    
    // Phase 4 will add: if (sender.startsWith("DriveRing") || sender.startsWith("DomeRing"))
    // Phase 3 will add: if (sender.startsWith("DataPort"))
}
```

### 6.5 Factory Reset

```cpp
void Brain::factoryReset() {
    // ... existing Brain config reset ...
    
    // Component factory resets (includes mechNetMasterNode)
    for (droid::core::BaseComponent* component : componentList) {
        component->factoryReset();
    }
    
    // ... existing controller factory resets ...
}
```

---

## 7. Configuration Schema

### 7.1 NVS Partition Structure

**Namespace**: `MechNet` (component name)

| Key | Type | Description | Example |
|-----|------|-------------|---------|
| `MNEnable` | bool | Enable MechNet support | `true` |
| `MNInit` | bool | Has been provisioned | `true` |
| `MNNetName` | String | Network name (1-32 chars) | `"R2D2Net"` |
| `MNChannel` | int | WiFi channel (1-13) | `6` |
| `MNPSK` | String | PSK as hex string (64 chars) | `"abc123...def"` |

### 7.2 Defaults (hardware.config.h)

```cpp
// MechNet Configuration
#define CONFIG_DEFAULT_MECHNET_ENABLED      false
#define CONFIG_DEFAULT_MECHNET_NETWORK_NAME "MechNet"
#define CONFIG_DEFAULT_MECHNET_CHANNEL      6
// PSK has no default - must be provisioned
```

### 7.3 Configuration States

| State | MNEnable | MNInit | Behavior |
|-------|----------|--------|----------|
| **Disabled** | false | * | MechNet not initialized, no network activity |
| **Not Provisioned** | true | false | MechNet enabled but missing config, warning logged |
| **Provisioned** | true | true | MechNet fully operational |

---

## 8. File Structure

### 8.1 New Files

```
include/droid/network/
    MechNetMasterNode.h              # BaseComponent wrapper for MechNet

src/droid/network/
    MechNetMasterNode.cpp            # Implementation

include/droid/command/
    MechNetOutboundHandler.h         # Outbound command handler

src/droid/command/
    MechNetOutboundHandler.cpp       # Implementation
```

### 8.2 Modified Files

```
include/droid/brain/Brain.h          # Add mechNetMasterNode member
src/droid/brain/Brain.cpp            # Integration (constructor, task, routing)
src/droid/brain/LocalCmdHandler.cpp  # Add mechnet-status, mechnet-provision
platformio.ini                       # Add MechNet library dependency
settings/hardware.config.h           # Add MechNet config defaults
settings/LoggerLevels.config.h       # Add MechNet log level
```

### 8.3 Removed Files

```
include/droid/command/ESPNowCmdHandler.h  # Deprecated
src/droid/command/ESPNowCmdHandler.cpp    # Deprecated
```

---

## 9. Implementation Sequence

### 9.1 Step 1: Add MechNet Dependency

**platformio.ini** modifications:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

lib_deps =
    # ... existing dependencies ...
    MechNet @ ^0.2.0
```

**Validation**: `pio lib install` succeeds

### 9.2 Step 2: Create MechNetMasterNode Component

1. Create `include/droid/network/MechNetMasterNode.h`
2. Create `src/droid/network/MechNetMasterNode.cpp`
3. Implement BaseComponent lifecycle methods
4. Implement public API methods (sendCommand, findNodeByPrefix, etc.)

**Validation**: Builds without errors

### 9.3 Step 3: Create MechNetOutboundHandler

1. Create `include/droid/command/MechNetOutboundHandler.h`
2. Create `src/droid/command/MechNetOutboundHandler.cpp`
3. Implement CmdHandler::process() with prefix matching

**Validation**: Builds without errors

### 9.4 Step 4: Integrate into Brain

1. Add `#include "droid/network/MechNetMasterNode.h"` to Brain.cpp
2. Add member variable to Brain.h
3. Modify constructor (create component, add to componentList, register handler)
4. Implement `processInboundMechNetMessages()` and `routeMessageToHandler()`
5. Add `getMechNetMaster()` accessor

**Validation**: Builds without errors, Brain initializes

### 9.5 Step 5: Add Console Commands

1. Modify `src/droid/brain/LocalCmdHandler.cpp`
2. Implement `mechnetStatus()` method
3. Implement `mechnetProvisionWizard()` method
4. Register commands with ConsoleHandler

**Validation**: Commands appear in console, can be invoked

### 9.6 Step 6: Remove ESPNowCmdHandler

1. Remove files:
   - `include/droid/command/ESPNowCmdHandler.h`
   - `src/droid/command/ESPNowCmdHandler.cpp`
2. Remove any references in Brain.cpp

**Validation**: Builds without errors, no linker complaints

### 9.7 Step 7: Update Configuration Files

1. Add MechNet defaults to `settings/hardware.config.h`
2. Add MechNet log level to `settings/LoggerLevels.config.h`

**Validation**: Brain logs MechNet status on boot

### 9.8 Step 8: Test with MechNetCentral

1. Provision MechNet on Brain via `mechnet-provision` wizard
2. Configure MechNetCentral remote example with matching config
3. Run both devices, verify connection
4. Test outbound command: `DataPort>TEST` (should appear in remote logs)
5. Verify `mechnet-status` shows connected node

**Validation**: End-to-end communication working

---

## 10. Testing Strategy

### 10.1 Unit Testing

**MechNetMasterNode Lifecycle**:
- ✅ `init()` with MechNet disabled → no initialization
- ✅ `init()` with enabled but not provisioned → warning logged
- ✅ `init()` with invalid PSK length → error logged
- ✅ `init()` with valid config → MechNet initialized
- ✅ `factoryReset()` clears all config keys

**MechNetOutboundHandler**:
- ✅ `process()` with MechNet disabled → returns false
- ✅ `process()` with no matching node → returns false
- ✅ `process()` with matching node → sends command, returns true
- ✅ Prefix matching: "DataPort" matches "DataPort-A1B2"

**Console Commands**:
- ✅ `mechnet-status` with MechNet disabled
- ✅ `mechnet-status` with MechNet enabled and connected nodes
- ✅ `mechnet-provision` wizard completes successfully
- ✅ `mechnet-provision` validates PSK format (64 hex chars)
- ✅ Config persists across reboots

### 10.2 Integration Testing

**Provisioning Flow**:
1. Factory reset Brain
2. Run `mechnet-provision` wizard
3. Enter network name, channel, PSK
4. Restart Brain
5. Verify MechNet initializes with correct config

**Command Routing**:
1. Provision Brain and remote node with matching config
2. Wait for connection (check `mechnet-status`)
3. Queue command via console: `Play DataPort>LED:R255`
4. Verify command appears in remote node logs
5. Verify MechNetOutboundHandler logged routing decision

**Fallback Behavior**:
1. Configure action that targets MechNet device: `cmdMap["Test"] = "DataPort>TEST"`
2. With node disconnected, fire action
3. Verify MechNetOutboundHandler returns false
4. Verify no errors in Brain logs

**Message Reception**:
1. Send message from remote node to Brain
2. Verify message appears in Brain logs (DEBUG level)
3. Verify sender name correctly captured

### 10.3 Hardware Testing

**MechNetCentral Simulation**:
- Use `MechNet/examples/mechnet_central` as remote node simulator
- Test with 1-3 simulated nodes
- Verify connection/disconnection handling
- Test rapid message sending (stress test receive queue)

**Range Testing**:
- Verify communication at 10, 20, 30 feet
- Test through walls/obstacles
- Monitor RSSI values (future: expose in mechnet-status)

**Reliability Testing**:
- Leave system running for extended period (1 hour+)
- Verify no memory leaks (monitor free heap)
- Verify heartbeat/timeout handling (disconnect remote, wait 3s)

---

## 11. Validation Criteria

### 11.1 Functional Requirements

- ✅ MechNet can be enabled/disabled via config
- ✅ Provisioning wizard accepts and validates all parameters
- ✅ PSK stored securely as hex string in NVS
- ✅ Brain initializes as MechNet Master when provisioned
- ✅ Outbound commands route to connected nodes via prefix matching
- ✅ Inbound messages logged (routing logic added in later phases)
- ✅ Factory reset clears MechNet config
- ✅ `mechnet-status` displays accurate information

### 11.2 Performance Requirements

- ✅ `Brain::task()` execution time < 30ms (including MechNet)
- ✅ No memory leaks over 1 hour runtime
- ✅ Command latency < 100ms (Brain → Remote)
- ✅ Message receive queue does not overflow (48 message capacity)

### 11.3 Integration Requirements

- ✅ Builds without errors with MechNet dependency
- ✅ No conflicts with existing components (Controller, PWMService, etc.)
- ✅ Console commands accessible via SmartCLI
- ✅ Compatible with all existing controller types (DualRing, SonyNav, PS3, Stub)

### 11.4 Code Quality

- ✅ Follows existing MechMind patterns (BaseComponent lifecycle, Config usage)
- ✅ Proper error handling (null checks, validation)
- ✅ Logging at appropriate levels (INFO for status, DEBUG for messages, ERROR for failures)
- ✅ No magic numbers (use CONFIG_KEY_* and CONFIG_DEFAULT_* macros)

---

## 12. Future Phase Dependencies

### 12.1 Phase 3: DataPort Support

**Prerequisites from Phase 2**:
- ✅ MechNetMasterNode operational
- ✅ MechNetOutboundHandler routes commands
- ✅ Inbound message routing infrastructure exists

**Phase 3 Additions**:
- DataPortCmdHandler for status/diagnostic messages from DataPort
- DataPort message protocol definition
- Integration with existing display logic

### 12.2 Phase 4: MechRingController

**Prerequisites from Phase 2**:
- ✅ MechNetMasterNode operational
- ✅ Brain provides `getMechNetMaster()` accessor
- ✅ Inbound message routing infrastructure exists

**Phase 4 Additions**:
- MechRingController class (implements Controller interface)
- Ring message protocol parsing
- Trigger mapping (MechRingTrigger.map)
- Routing logic for "DriveRing-*" and "DomeRing-*" messages

**Configuration Dependency**:
```cpp
// Brain constructor (Phase 4)
String whichController = config->getString(name, CONFIG_KEY_BRAIN_CONTROLLER, ...);
if (whichController == CONTROLLER_OPTION_MECHRING) {
    // Validate MechNet is enabled
    bool mechNetEnabled = config->getBool("MechNet", "MNEnable", false);
    if (!mechNetEnabled) {
        logger->log(name, ERROR, "MechRingController requires MechNet to be enabled\n");
        // Fall back to StubController
        controller = new StubController("ControllerStub", system);
    } else {
        controller = new MechRingController("MechRing", system, mechNetMasterNode);
    }
}
```

### 12.3 Phase 5: Generalized Routing

**Prerequisites from Phase 2**:
- ✅ MechNetOutboundHandler establishes pattern
- ✅ Prefix-based node discovery working

**Phase 5 Additions**:
- Refactor DataPortCmdHandler to use common base class
- Fallback logic in StreamCmdHandler (check MechNet first)
- Generic handler registration for future node types

---

## 13. Open Questions & Decisions

### 13.1 Resolved Decisions

| Question | Decision | Rationale |
|----------|----------|-----------|
| Wrapper vs direct use? | MechNetMasterNode (wrapper) | Follows BaseComponent pattern, lifecycle management |
| Handler structure? | Separate MechNetOutboundHandler | Separation of concerns, extensible |
| PSK storage? | Hex string (64 chars) | No need for binary blob, easier to display/edit |
| Enable vs provisioned flags? | Both (MNEnable + MNInit) | Allows enable/disable without losing config |
| Wizard auto-restart? | Manual restart | Safer, allows verification before reboot |
| ESPNowCmdHandler? | Remove in Phase 2 | Clean up deprecated code early |

### 13.2 Future Considerations

- **Multi-network support**: Could Brain join multiple networks? (Not needed for R2D2 use case)
- **Node persistence**: Cache node names across reboots to speed up routing? (Defer to Phase 3+)
- **RSSI monitoring**: Expose signal strength in mechnet-status? (Nice-to-have, not critical)
- **Encryption verification**: How to validate PSK matches across nodes? (MechNet handles via auth failures)

---

## 14. References

### 14.1 MechNet Documentation

- [MechNet README](../../MechNet/README.md) - Library overview and facade API
- [MechNet v0.2.0 Release](https://github.com/kizmit99/MechNet/releases/tag/v0.2.0) - Published version
- [MechNet Requirements](../../MechNet/docs/design/MechNet-Requirements-and-Design-v2.md) - Protocol details
- [Security Architecture](../../MechNet/docs/design/Security-Architecture-Design.md) - HMAC-SHA256 + AES-128-CCM

### 14.2 MechMind Architecture

- [BaseComponent.h](../include/droid/core/BaseComponent.h) - Component lifecycle pattern
- [CmdHandler.h](../include/droid/command/CmdHandler.h) - Command handler interface
- [StreamCmdHandler.cpp](../src/droid/command/StreamCmdHandler.cpp) - Serial command routing pattern
- [Brain.cpp](../src/droid/brain/Brain.cpp) - Component instantiation and task loop
- [Config.h](../include/shared/common/Config.h) - NVS configuration API

### 14.3 Related Designs

- [SmartCLI Console Migration](SmartCLI-Console-Migration-Design.md) - Console integration pattern
- [MechRingController MechNet Design](MechRingController-MechNet-Design.md) - Overall multi-phase plan

---

## Document History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | Jan 29, 2026 | Initial Phase 2 design based on agreed architecture decisions |

