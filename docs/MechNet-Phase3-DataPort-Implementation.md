# MechNet Phase 3: DataPort Remote Node Support - Implementation Design

**Status**: Implementation Specification  
**Date**: January 30, 2026  
**Phase**: 3 of 5 (MechNet Integration)  
**Dependencies**: Phase 2 (Brain as MechNet Master) - COMPLETE  
**Related Documents**: 
- [MechRingController-MechNet-Design.md](MechRingController-MechNet-Design.md) - Overall MechNet integration plan
- [MechNet-Phase2-Integration-Design.md](MechNet-Phase2-Integration-Design.md) - Phase 2 implementation

---

## 1. Overview

### 1.1 Objectives

Phase 3 establishes **bidirectional communication** with remote MechNet nodes, using DataPort LED display boards as the reference implementation. This phase creates reusable patterns for all future remote node types (MechRing controllers in Phase 4, panel controllers in Phase 5, etc.).

**Key Deliverables**:
1. **Broadcast command routing** - Send commands to multiple matching remote nodes
2. **Inbound message handler pattern** - Extensible architecture for processing messages from remote nodes
3. **DataPort integration** - Reference implementation for command and message handlers

### 1.2 Scope

**In Scope**:
- Broadcast support in MechNetCmdHandler (send to all matching nodes)
- New `droid::message` namespace with MsgHandler base class
- DataPortMsgHandler for processing inbound DataPort messages
- Brain integration of MsgHandler lifecycle
- Callback-based node enumeration (no heap allocation in hot path)
- Action.map examples for DataPort commands

**Out of Scope**:
- DataPort protocol parsing (Phase 3 just logs messages; parsing added in future enhancements)
- MechRing controller support (Phase 4)
- Generalized remote command routing (Phase 5)
- Auto-discovery of remote node capabilities (Phase 5)

### 1.3 Design Principles

- **Pattern Consistency**: MsgHandler mirrors CmdHandler architecture (both are BaseComponents)
- **Zero Heap Allocation**: Use callbacks for node enumeration instead of returning vectors
- **Extensibility**: New remote node types add new MsgHandler subclasses
- **Protocol Agnostic**: Brain routes messages without understanding node-specific protocols
- **Broadcast Semantics**: Commands with matching device prefixes go to ALL matching nodes

---

## 2. Architecture Overview

### 2.1 Command Flow (Brain → Remote Node)

```
Action.map Entry:
"DataPortStatus" = "DataPort>@V1 status"
         ↓
ActionMgr.fireAction("DataPortStatus")
         ↓
ActionMgr.executeCommands() → iterates cmdHandlers
         ↓
MechNetCmdHandler.process("DataPort", "@V1 status")
         ↓
MechNetNode.findAllNodesByPrefix("DataPort", callback)
         ↓  (callback invoked for each match)
MechNetNode.sendCommand("DataPort-6168", "@V1 status")
MechNetNode.sendCommand("DataPort-3eaf", "@V1 status")
         ↓
MechNet::MechNetMaster.sendTo(nodeName, message, requiresAck=true)
```

**Key Change from Phase 2**: `findNodeByPrefix()` (returns first match) replaced with `findAllNodesByPrefix()` (callback for each match).

### 2.2 Message Flow (Remote Node → Brain)

```
DataPort ESP32 sends message via MechNet
         ↓
MechNet::MechNetMaster receives message
         ↓
Brain.processInboundMechNetMessages()
         ↓
Brain.routeMessageToHandler(sender="DataPort-6168", message="BATT:85")
         ↓  (iterates inboundMsgHandlers)
DataPortMsgHandler.handleMessage(sender, message)
         ↓
  - Checks if sender.startsWith("DataPort")
  - Logs message (Phase 3)
  - Returns true (message consumed)
         ↓
  (Future phases: Parse message, update DroidState, trigger actions)
```

### 2.3 New Namespace: `droid::message`

**Rationale**: Separate namespace for inbound message handling to distinguish from:
- `droid::command` (outbound command handlers)
- `droid::network` (MechNet infrastructure)

**Location**:
- Headers: `include/droid/message/`
- Implementation: `src/droid/message/`

---

## 3. Component Specifications

### 3.1 MsgHandler Base Class

**Purpose**: Abstract base class for handling inbound messages from remote MechNet nodes. Follows the same pattern as `CmdHandler` for outbound commands.

**File**: `include/droid/message/MsgHandler.h`

```cpp
/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 */

#pragma once
#include "droid/core/BaseComponent.h"

namespace droid::message {
    class MsgHandler : public droid::core::BaseComponent {
    public:
        MsgHandler(const char* name, droid::core::System* system) :
            BaseComponent(name, system) {}

        // Virtual methods required by BaseComponent
        // Most subclasses will use default implementations (NOOPs)
        void init() override {}
        void factoryReset() override {}
        void task() override {}
        void logConfig() override {}
        void failsafe() override {}

        /**
         * Process an inbound message from a remote MechNet node.
         * 
         * @param sender Unique node name (e.g., "DataPort-6168", "DriveRing-A3F2")
         * @param message Protocol-specific message content
         * @return true if this handler processed the message, false to try next handler
         */
        virtual bool handleMessage(const String& sender, const String& message) = 0;
    };
}
```

**Design Notes**:
- Inherits from `BaseComponent` for lifecycle management (follows CmdHandler pattern)
- Default implementations for lifecycle methods (most handlers won't need custom logic)
- `handleMessage()` returns `bool` (first handler to return true wins)
- Handlers check sender prefix to determine if message is for them

**File**: `src/droid/message/MsgHandler.cpp`

Not needed - header-only base class.

---

### 3.2 DataPortMsgHandler

**Purpose**: Handle inbound messages from DataPort remote nodes (LED display boards).

**File**: `include/droid/message/DataPortMsgHandler.h`

```cpp
/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 */

#pragma once
#include "droid/message/MsgHandler.h"

namespace droid::message {
    class DataPortMsgHandler : public MsgHandler {
    public:
        DataPortMsgHandler(const char* name, droid::core::System* system);

        bool handleMessage(const String& sender, const String& message) override;

    private:
        // Future: Add state tracking, protocol parsing, etc.
    };
}
```

**File**: `src/droid/message/DataPortMsgHandler.cpp`

```cpp
/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 */

#include "droid/message/DataPortMsgHandler.h"

namespace droid::message {
    
    DataPortMsgHandler::DataPortMsgHandler(const char* name, droid::core::System* system) :
        MsgHandler(name, system) {
    }

    bool DataPortMsgHandler::handleMessage(const String& sender, const String& message) {
        // Check if this message is from a DataPort node
        if (!sender.startsWith("DataPort")) {
            return false;  // Not for us, try next handler
        }

        // Phase 3: Just log the message
        logger->log(name, INFO, "RX [%s]: %s\n", sender.c_str(), message.c_str());

        // Future phases: Parse message protocol and react
        // Example protocol (TBD based on DataPort firmware):
        //   "BATT:85" -> Update battery level in DroidState
        //   "STATUS:OK" -> Clear error flags
        //   "@V1 status:ready" -> Response to status query

        return true;  // Message consumed
    }
}
```

**Design Notes**:
- Simple implementation for Phase 3 (logging only)
- Extensible for future protocol parsing
- Returns true after logging (prevents "unhandled message" warnings)

---

### 3.3 MechNetNode Enhancements

**Purpose**: Add callback-based node enumeration to avoid heap allocation in hot path.

**File**: `include/droid/network/MechNetNode.h`

**Changes**:

```cpp
// Add new method declaration (after existing findNodeByPrefix method):

/**
 * Find all connected nodes matching a device name prefix.
 * Invokes callback for each matching node.
 * 
 * @param prefix Device name prefix (e.g., "DataPort")
 * @param callback Function called for each match: callback(nodeName)
 */
void findAllNodesByPrefix(const char* prefix, 
                          std::function<void(const char*)> callback);
```

**File**: `src/droid/network/MechNetNode.cpp`

**Implementation** (add after existing `findNodeByPrefix` method):

```cpp
void MechNetNode::findAllNodesByPrefix(const char* prefix, 
                                       std::function<void(const char*)> callback) {
    if (!mechNetMaster) return;

    char nodeName[32];
    for (uint8_t i = 0; i < mechNetMaster->connectedNodeCount(); i++) {
        if (mechNetMaster->getConnectedNodeName(i, nodeName, sizeof(nodeName))) {
            if (strncmp(nodeName, prefix, strlen(prefix)) == 0) {
                callback(nodeName);  // Invoke callback for each match
            }
        }
    }
}
```

**Design Notes**:
- Uses `std::function<void(const char*)>` callback (standard C++ pattern)
- No heap allocation (nodeName is stack-allocated, callback receives pointer)
- Callback invoked synchronously for each matching node
- Caller can capture context via lambda closure

---

### 3.4 MechNetCmdHandler Updates

**Purpose**: Support broadcast to multiple matching nodes with per-node logging.

**File**: `include/droid/command/MechNetCmdHandler.h`

No changes required (existing interface sufficient).

**File**: `src/droid/command/MechNetCmdHandler.cpp`

**Changes** (replace entire `process()` method):

```cpp
bool MechNetCmdHandler::process(const char* device, const char* command) {
    if (!mechNetMasterNode || !mechNetMasterNode->isInitialized()) {
        return false;  // MechNet not available
    }

    // Track if any nodes matched
    bool foundNodes = false;

    // Send to all nodes matching device prefix
    mechNetMasterNode->findAllNodesByPrefix(device, [&](const char* nodeName) {
        foundNodes = true;
        
        bool sent = mechNetMasterNode->sendCommand(nodeName, command, true);
        if (sent) {
            logger->log(name, DEBUG, "TX [%s]: %s>%s\n", 
                       nodeName, device, command);
        } else {
            logger->log(name, WARN, "Failed to send to %s: %s>%s\n", 
                       nodeName, device, command);
        }
    });

    // If no matching nodes found, let other handlers try
    if (!foundNodes) {
        return false;
    }

    return true;  // Command consumed (sent to one or more nodes)
}
```

**Design Notes**:
- Lambda captures `foundNodes`, `device`, `command` by reference
- DEBUG log for successful sends (per-node)
- WARN log for failed sends (per-node)
- Returns false if no matching nodes (allows fallback to serial handlers)
- Returns true if at least one node matched (even if sends failed)

---

### 3.5 Brain Integration

**Purpose**: Integrate MsgHandler pattern into Brain lifecycle and message routing.

**File**: `include/droid/brain/Brain.h`

**Changes**:

```cpp
// Add forward declaration (after existing forward declarations):
namespace droid::message {
    class MsgHandler;  // Forward declaration
}

// Add to Brain class private members (after existing componentList):
std::vector<droid::message::MsgHandler*> inboundMsgHandlers;
```

**File**: `src/droid/brain/Brain.cpp`

**Changes**:

**1. Add includes** (at top of file after existing includes):

```cpp
#include "droid/message/DataPortMsgHandler.h"
```

**2. Update Brain constructor** (after ActionMgr cmdHandler registration):

```cpp
// In Brain::Brain() constructor, after:
// actionMgr->addCmdHandler(new droid::brain::LocalCmdHandler("Brain", system, this));

// Register inbound message handlers
inboundMsgHandlers.push_back(new droid::message::DataPortMsgHandler("DataPortMsg", system));
```

**3. Update Brain::init()** (after component initialization loop):

```cpp
// After existing component initialization:
//   for (droid::core::BaseComponent* component : componentList) {
//       component->init();
//   }

// Initialize inbound message handlers
for (droid::message::MsgHandler* handler : inboundMsgHandlers) {
    handler->init();
}
```

**4. Update Brain::factoryReset()** (after component factory reset loop):

```cpp
// After existing component factory resets

// Factory reset inbound message handlers
for (droid::message::MsgHandler* handler : inboundMsgHandlers) {
    handler->factoryReset();
}
```

**5. Update Brain::logConfig()** (after component logConfig loop):

```cpp
// After existing component logConfig calls:
//   for (droid::core::BaseComponent* component : componentList) {
//       component->logConfig();
//   }

// Log config for inbound message handlers
for (droid::message::MsgHandler* handler : inboundMsgHandlers) {
    handler->logConfig();
}
```

**6. Update Brain::failsafe()** (after component failsafe loop):

```cpp
// After existing component failsafe calls:
//   for (droid::core::BaseComponent* component : componentList) {
//       component->failsafe();
//   }

// Failsafe for inbound message handlers
for (droid::message::MsgHandler* handler : inboundMsgHandlers) {
    handler->failsafe();
}
```

**7. Replace Brain::routeMessageToHandler()** (entire method):

```cpp
void Brain::routeMessageToHandler(const String& sender, const String& message) {
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

**Design Notes**:
- MsgHandlers have full lifecycle management (init, factoryReset, task, logConfig, failsafe)
- Separate vector from `componentList` for clarity (MsgHandlers are message-specific)
- First handler to return true wins (same pattern as ActionMgr cmdHandlers)
- Unhandled messages logged at DEBUG level (not an error - expected for unknown node types)
- Uses `name` (not hard-coded string) for logger calls

---

### 3.6 Action.map Example

**Purpose**: Demonstrate DataPort command routing via Action.map.

**File**: `settings/Action.map`

**Addition** (add to end of file):

```cpp
// DataPort Commands (Phase 3 - MechNet Remote Node)
cmdMap["DataPortStatus"]    = "DataPort>@V1 status";
```

**Design Notes**:
- Uses existing Action.map syntax: `device>command`
- `DataPort>` prefix matches any node with name starting with "DataPort" (e.g., "DataPort-6168")
- Command `@V1 status` is DataPort-specific protocol (format TBD by DataPort firmware)
- If multiple DataPort nodes connected, command broadcasts to all

---

## 4. Implementation Checklist

### 4.1 New Files to Create

- [ ] `include/droid/message/MsgHandler.h` - Base class for inbound message handlers
- [ ] `include/droid/message/DataPortMsgHandler.h` - DataPort message handler header
- [ ] `src/droid/message/DataPortMsgHandler.cpp` - DataPort message handler implementation

### 4.2 Existing Files to Modify

- [ ] `include/droid/network/MechNetNode.h` - Add `findAllNodesByPrefix()` declaration
- [ ] `src/droid/network/MechNetNode.cpp` - Implement `findAllNodesByPrefix()`
- [ ] `src/droid/command/MechNetCmdHandler.cpp` - Update `process()` for broadcast with logging
- [ ] `include/droid/brain/Brain.h` - Add `inboundMsgHandlers` vector and forward declaration
- [ ] `src/droid/brain/Brain.cpp` - Integrate MsgHandler lifecycle and update `routeMessageToHandler()`
- [ ] `settings/Action.map` - Add DataPort example command

### 4.3 Directory Structure

Ensure `include/droid/message/` and `src/droid/message/` directories exist:

```
MechMind/
├── include/
│   └── droid/
│       ├── message/          # NEW - Inbound message handlers
│       │   ├── MsgHandler.h
│       │   └── DataPortMsgHandler.h
│       ├── command/          # Existing - Outbound command handlers
│       ├── network/          # Existing - MechNet infrastructure
│       └── ...
└── src/
    └── droid/
        ├── message/          # NEW - Inbound message handler implementations
        │   └── DataPortMsgHandler.cpp
        ├── command/
        ├── network/
        └── ...
```

---

## 5. Testing Plan

### 5.1 Prerequisites

1. **MechNet Provisioned on Brain**:
   - Use console command: `mechnet-provision` (from Phase 1)
   - Configure network name, channel, PSK
   - Verify: `mechnet-status` shows "initialized: true"

2. **DataPort Firmware Ready**:
   - DataPort board programmed with MechNet Remote Node firmware
   - Provisioned with same network name, channel, PSK as Brain
   - Base node name set to "DataPort" (will auto-generate unique name like "DataPort-6168")

### 5.2 Test Cases

#### Test 1: MechNet Connection
**Objective**: Verify Brain detects DataPort node connection.

**Steps**:
1. Power on Brain (MechNet initialized)
2. Power on DataPort board
3. Monitor Brain serial console

**Expected Output**:
```
[MechNet] INFO: Node connected: DataPort-6168
```

**Pass Criteria**: Brain logs node connection with unique name.

---

#### Test 2: Outbound Command (Single Node)
**Objective**: Verify command routing from ActionMgr to single DataPort node.

**Steps**:
1. Connect one DataPort node
2. Fire action: `brain.fireAction("DataPortStatus")`
3. Monitor Brain serial console

**Expected Output**:
```
[MechNetOut] DEBUG: TX [DataPort-6168]: DataPort>@V1 status
```

**Pass Criteria**: 
- MechNetCmdHandler logs DEBUG message
- DataPort receives command (verify on DataPort serial monitor)

---

#### Test 3: Outbound Command (Multiple Nodes)
**Objective**: Verify broadcast to multiple matching nodes.

**Steps**:
1. Connect two DataPort nodes (e.g., "DataPort-6168", "DataPort-3eaf")
2. Fire action: `brain.fireAction("DataPortStatus")`
3. Monitor Brain serial console

**Expected Output**:
```
[MechNetOut] DEBUG: TX [DataPort-6168]: DataPort>@V1 status
[MechNetOut] DEBUG: TX [DataPort-3eaf]: DataPort>@V1 status
```

**Pass Criteria**: 
- Both nodes receive command
- Two DEBUG log entries (one per node)

---

#### Test 4: Inbound Message Handling
**Objective**: Verify DataPortMsgHandler processes messages from DataPort.

**Steps**:
1. Connect DataPort node
2. Trigger DataPort to send message (e.g., button press or timed status update)
3. Monitor Brain serial console

**Expected Output**:
```
[DataPortMsg] INFO: RX [DataPort-6168]: BATT:85
```

**Pass Criteria**: 
- Message logged by DataPortMsgHandler
- No "Unhandled message" warning

---

#### Test 5: Send Failure Handling
**Objective**: Verify WARN logging when send fails.

**Steps**:
1. Connect DataPort node
2. Power off DataPort (simulate link loss)
3. Fire action: `brain.fireAction("DataPortStatus")`
4. Monitor Brain serial console

**Expected Output** (after MechNet retry timeout):
```
[MechNetOut] WARN: Failed to send to DataPort-6168: DataPort>@V1 status
```

**Pass Criteria**: WARN log indicates send failure.

---

#### Test 6: Unhandled Message
**Objective**: Verify unknown node messages logged at DEBUG level.

**Steps**:
1. Connect a future remote node type (e.g., "PanelCtrl-ABCD") that doesn't have a MsgHandler
2. Trigger node to send message
3. Monitor Brain serial console

**Expected Output**:
```
[R2D2] DEBUG: Unhandled MechNet RX [PanelCtrl-ABCD]: Some message
```

**Pass Criteria**: 
- Message logged at DEBUG level (not WARN/ERROR)
- Uses Brain's `name` (e.g., "R2D2"), not hard-coded string

---

### 5.3 Validation Checklist

- [ ] MechNet node connection/disconnection detected
- [ ] Commands route to single DataPort node
- [ ] Commands broadcast to multiple DataPort nodes
- [ ] DataPort messages handled by DataPortMsgHandler
- [ ] Send failures logged at WARN level
- [ ] Unhandled messages logged at DEBUG level (no crashes)
- [ ] No heap allocation during `findAllNodesByPrefix()` calls
- [ ] All lifecycle methods called for MsgHandlers (init, factoryReset, etc.)

---

## 6. Integration with Existing Code

### 6.1 CmdHandler Pattern (Outbound)

MechNetCmdHandler follows existing `CmdHandler` pattern:

- Registered in `ActionMgr` via `addCmdHandler()` (done in Brain constructor)
- `process(device, command)` method called by `ActionMgr::executeCommands()`
- Returns `false` if device prefix doesn't match (allows fallback)
- Returns `true` if command consumed (even if send failed)

**Example**: If MechNet not provisioned or DataPort not connected, MechNetCmdHandler returns `false`, allowing serial-based handlers (e.g., StreamCmdHandler) to try.

### 6.2 MsgHandler Pattern (Inbound)

MsgHandler mirrors CmdHandler architecture:

- Both inherit from `BaseComponent`
- Both use "first handler to claim it wins" pattern
- Both return `bool` to indicate if message/command was processed
- Both registered in Brain (MsgHandlers in `inboundMsgHandlers`, CmdHandlers in ActionMgr)

**Consistency Benefits**:
- Developers familiar with CmdHandler can easily understand MsgHandler
- Same lifecycle patterns (init, factoryReset, task, etc.)
- Same registration patterns (vector of handlers, iterate until one returns true)

### 6.3 BaseComponent Lifecycle

MsgHandlers participate in full BaseComponent lifecycle:

| Lifecycle Method | When Called | MsgHandler Default Behavior |
|-----------------|-------------|----------------------------|
| `init()` | Brain startup | NOOP (most handlers don't need initialization) |
| `factoryReset()` | Config reset | NOOP (no config to clear in Phase 3) |
| `task()` | Every loop iteration | NOOP (handlers are reactive, not polling) |
| `logConfig()` | After init | NOOP (no config to log in Phase 3) |
| `failsafe()` | Error condition | NOOP (no actuators to disable) |

**Future Extension**: Phase 4 MechRingMsgHandler may override `task()` for joystick state timeout detection.

### 6.4 Memory Management

**Static Allocation Pattern**:
- All handlers created in Brain constructor with `new`
- Pointers stored in vectors (`inboundMsgHandlers`, ActionMgr's `cmdHandlers`)
- No dynamic creation/destruction during runtime
- Destructors called on Brain destruction (application lifetime)

**Callback Pattern (No Heap)**:
- `findAllNodesByPrefix()` uses callbacks instead of returning `std::vector<String>`
- Avoids heap allocation in hot path (command routing)
- Stack-allocated buffer reused for each callback invocation

---

## 7. Future Enhancements (Post-Phase 3)

### 7.1 DataPort Protocol Parsing

Currently DataPortMsgHandler just logs messages. Future enhancement:

```cpp
bool DataPortMsgHandler::handleMessage(const String& sender, const String& message) {
    if (!sender.startsWith("DataPort")) return false;

    // Parse protocol (example - actual protocol TBD)
    if (message.startsWith("BATT:")) {
        int battLevel = message.substring(5).toInt();
        droidState->batteryLevel = battLevel;
        logger->log(name, INFO, "Battery: %d%%\n", battLevel);
    } else if (message.startsWith("STATUS:")) {
        String status = message.substring(7);
        logger->log(name, INFO, "Status: %s\n", status.c_str());
    } else {
        logger->log(name, DEBUG, "Unknown DataPort msg: %s\n", message.c_str());
    }

    return true;
}
```

### 7.2 Action Triggers from Messages

Allow messages to trigger actions:

```cpp
// In DataPortMsgHandler
if (message == "BUTTON:PRESSED") {
    // Trigger action via Brain (would need Brain reference)
    brain->fireAction("DataPortButtonPress");
}
```

### 7.3 Multiple Handler Types per Node

Some remote nodes may send different message types:

```cpp
// In routeMessageToHandler(), allow multiple handlers to process
for (auto* handler : inboundMsgHandlers) {
    handler->handleMessage(sender, message);  // Don't stop on first true
}
```

**Trade-off**: Current design (stop on first true) is simpler and sufficient for Phase 3.

---

## 8. Code Review Checklist

Before merging Phase 3 implementation:

### 8.1 Code Quality
- [ ] All new files have proper license headers
- [ ] Namespace usage consistent (`droid::message` for handlers)
- [ ] No hard-coded strings in logger calls (use component `name`)
- [ ] No heap allocation in `findAllNodesByPrefix()`
- [ ] Comments explain protocol-specific behavior

### 8.2 Architecture
- [ ] MsgHandler follows BaseComponent pattern
- [ ] MsgHandler lifecycle integrated into Brain
- [ ] Broadcast logic in MechNetCmdHandler uses callbacks
- [ ] No coupling between DataPortMsgHandler and Brain (only via logger/config)

### 8.3 Logging
- [ ] DEBUG level for successful sends (per-node)
- [ ] WARN level for failed sends (per-node)
- [ ] INFO level for DataPort messages received
- [ ] DEBUG level for unhandled messages (not ERROR)

### 8.4 Testing
- [ ] All test cases from Section 5 pass
- [ ] Multiple DataPort nodes tested (broadcast verified)
- [ ] Send failure handling tested (disconnect node during send)
- [ ] Unknown node type tested (unhandled message path)

### 8.5 Documentation
- [ ] Action.map commented with DataPort examples
- [ ] MsgHandler.h has clear interface documentation
- [ ] Brain.h documents `inboundMsgHandlers` purpose

---

## 9. References

### 9.1 Existing Code Patterns

- **CmdHandler Pattern**: [include/droid/command/CmdHandler.h](../include/droid/command/CmdHandler.h)
- **BaseComponent Lifecycle**: [include/droid/core/BaseComponent.h](../include/droid/core/BaseComponent.h)
- **ActionMgr Handler Registration**: [src/droid/brain/Brain.cpp](../src/droid/brain/Brain.cpp) (see ActionMgr cmdHandler registration)
- **MechNet Facade API**: [MechNet README](../../MechNet/README.md)

### 9.2 Related Documentation

- **Overall MechNet Plan**: [MechRingController-MechNet-Design.md](MechRingController-MechNet-Design.md)
- **Phase 2 Implementation**: [MechNet-Phase2-Integration-Design.md](MechNet-Phase2-Integration-Design.md)
- **MechNet Security**: [MechNet Security Architecture](../../MechNet/docs/design/Security-Architecture-Design.md)

### 9.3 Next Phase

**Phase 4: MechRing Controller Implementation**
- MechRingMsgHandler for joystick/button data from rings
- MechRingController class (implements Controller interface)
- Trigger mapping and fault detection
- See [MechRingController-MechNet-Design.md](MechRingController-MechNet-Design.md) Section 6

---

## 10. Summary

Phase 3 establishes the **foundation for bidirectional MechNet communication**:

1. **Outbound**: Commands broadcast to all matching remote nodes (MechNetCmdHandler)
2. **Inbound**: Messages routed to protocol-specific handlers (MsgHandler pattern)
3. **Reference Implementation**: DataPort LED display boards (logging only in Phase 3)

**Key Architectural Contributions**:
- `droid::message` namespace for inbound handlers
- MsgHandler base class (mirrors CmdHandler for consistency)
- Callback-based node enumeration (zero heap allocation)
- Extensible pattern for future remote node types

**What's Next**: Phase 4 will implement MechRing wireless controllers using the MsgHandler pattern established here, parsing joystick data from ring messages and implementing the full Controller interface.
