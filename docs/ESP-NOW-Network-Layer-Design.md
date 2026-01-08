# ESP-NOW Network Layer Design

## Architecture Summary

This design implements a **shared library architecture** for ESP-NOW communication across multiple ESP32 projects (MechMind hub, ring controllers, panel controllers, etc.). The core network layer is protocol-agnostic and reusable; device-specific implementations consume it via a well-defined adapter pattern.

- **Shared Library**: `MechNet` (separate PlatformIO project, versioned independently)
- **Integration Pattern**: Each firmware project depends on the library and implements device-specific handlers
- **Protocol**: Bidirectional, selective-reliability ESP-NOW with acknowledgment and retry logic
- **Safety**: Strict message ordering per peer, optional emergency invalidation for critical commands

---

## Executive Summary

This document proposes a robust, flexible ESP-NOW "network layer" that enables MechMind (central hub) to communicate with multiple peripheral ESP32 devices in a **star topology**. The design supports:

- **Bidirectional communication** (controller input → hub; commands → peripherals)
- **Optional reliability** (critical messages get acks + retries; best-effort messages don't)
- **Lossy-network resilience** without requiring acks for every message
- **Device discovery & addressing** (static MACs or runtime peer registration)
- **Priority queuing** (critical messages ahead of best-effort)
- **Integration with existing patterns** (CmdHandler, ActionMgr, DroidState)

---

## 0. Shared Library Architecture

### Library vs. Application Code

The ESP-NOW implementation is split between a **reusable library** and **application-specific code**:

#### Core Library: `MechNet`

**Maintained in**: Separate PlatformIO project (separate Git repository), versioned independently  
**Responsibility**: Protocol implementation, message handling, peer management, reliability logic

**Contents**:
```
MechNet/
├── library.json              # PlatformIO library manifest
├── README.md                 # Installation & basic usage
├── src/
│   ├── MechMindESPNow/
│   │   ├── ESPNowNetworkManager.h/.cpp
│   │   ├── ESPNowPeerRegistry.h/.cpp
│   │   ├── ESPNowMessage.h/.cpp
│   │   ├── ESPNowTypes.h        # Message structs, constants
│   │   └── ILogger.h            # Abstract logger interface
│   └── internal/
│       ├── SequenceBuffer.h/.cpp
│       └── PendingAckTracker.h/.cpp
├── examples/
│   ├── hub_basic/
│   └── peripheral_basic/
└── test/
    ├── test_network_manager.cpp
    ├── test_message_ordering.cpp
    └── test_ack_retry_logic.cpp
```

**Dependencies**:
- Arduino Framework (for ESP32)
- Standard C++ (STL)
- No project-specific code

**API Surface** (what applications depend on):
```cpp
namespace MechMindESPNow {
    class ESPNowNetworkManager {
        void init(const char* channelMask);
        bool sendMessage(const uint8_t* destMac, const uint8_t* payload,
                        uint8_t payloadLen, bool requiresAck, uint16_t ackTimeoutMs);
        void registerReceiveCallback(ReceiveCallback cb);
        void task();
        PeerStatus getPeerStatus(const uint8_t* mac);
    };
    
    // Message types and constants
    struct ESPNowMessageHeader { /* ... */ };
    struct ESPNowMessage { /* ... */ };
}
```

#### Application Code: MechMind Hub & Peripherals

**Maintained in**: Individual firmware projects (or shared folder with multiple PlatformIO environments)  
**Responsibility**: Device-specific integration, CmdHandler implementations, payload definitions

**Examples**:

*MechMind Hub* (`include/droid/command/ESPNowCmdHandler.h`):
```cpp
class ESPNowCmdHandler : public CmdHandler {
    bool process(const char* device, const char* command) override;
    
private:
    MechMindESPNow::ESPNowNetworkManager* netMgr;
    bool determineCriticality(const char* command);
};
```

*Ring Controller Firmware* (standalone project):
```cpp
class RingInputSender {
    void init() {
        netMgr->addPeer(hubMac, false, true);  // Sender only
        netMgr->task();
    }
    
    void sendJoystickData(int x, int y, uint32_t buttons) {
        // Serialize to RingInputMessage (device-specific payload)
        netMgr->sendMessage(hubMac, payload, len, false);
    }
};
```

### Integration Pattern

Each firmware project:

1. **Adds library dependency** in `platformio.ini`:
   ```ini
   [env:default]
   lib_deps = 
       MechNet @ ^1.0.0
   ```

2. **Implements device-specific adapters** (logger, message handlers):
   ```cpp
   class LoggerAdapter : public MechMindESPNow::ILogger {
       Logger* localLogger;
   public:
       void log(const char* tag, uint8_t level, const char* fmt, ...) override {
           localLogger->log(tag, (LogLevel)level, fmt, ...);
       }
   };
   ```

3. **Uses library for all ESP-NOW communication**, never calling ESP-NOW APIs directly

### Why Separate Library?

| Aspect | Benefit |
|--------|---------|
| **Reusability** | Same network layer for hub, rings, panels, future peripherals |
| **Consistency** | Identical message format, ack logic, retry behavior across all devices |
| **Maintainability** | Bug fixes and protocol improvements apply everywhere |
| **Versioning** | Track protocol version independently (e.g., v1.2 of network layer might work with v2.0+ of firmware) |
| **Testing** | Unit tests written once, benefit all projects |
| **Decoupling** | Network layer doesn't depend on MechMind's Config/Logger/BaseComponent classes |

### Local Development Workflow

During development, use symlink to test library changes:

```ini
[env:dev]
lib_deps = symlink:///path/to/MechNet
```

### Release Workflow

1. Tag library version (e.g., `v1.0.0`)
2. Publish to GitHub or PlatformIO Registry
3. Update firmware projects to reference tagged version
4. Firmware versions tie to library version (e.g., MechMind v2.1 requires ESP-NOW-Library v1.2+)

---



### Current State
- `ESPNowCmdHandler` exists but is unimplemented
- MechRingController (proposed) will need ESP-NOW to receive joystick/button data
- Future ESP32 peripherals will need to receive commands from MechMind (dome rotation, light sequences, panel control, etc.)
- ESP-NOW is **lossy** by design—no guaranteed delivery, no broadcast retries

### Requirements
1. **Some messages are critical** (e.g., "stop all motors"): Must arrive reliably
2. **Some messages are optional** (e.g., "play idle sound #3"): Nice-to-have, if dropped it's acceptable
3. **Bidirectional** (hub ↔ peripherals), not just hub → peripherals
4. **Efficient** (don't ack every message; ESP-NOW bandwidth is limited)
5. **Debuggable** (can see message delivery status, timeouts, retries)
6. **Scalable** (can add/remove peripherals without recompile)

---

## 2. Core Design: Message Structure

### Message Header (Required for All)

```cpp
struct ESPNowMessageHeader {
    uint16_t messageId;          // Unique per sender (for ack correlation)
    uint8_t  sequence;           // For detecting missed/reordered messages
    uint8_t  flags;              // See flags below
    uint32_t timestamp;          // millis() when sent (receiver can detect stale data)
    uint8_t  protocol_version;   // Future-proof: currently 1
    uint8_t  payload_length;     // Actual payload size
    // Total: 10 bytes (leaves ~240 bytes for payload in 250-byte ESP-NOW limit)
};

// Flags: individual bits
#define ESPNOW_FLAG_REQUIRES_ACK        0x01  // Sender expects acknowledgement
#define ESPNOW_FLAG_IS_ACK              0x02  // This message IS an acknowledgement
#define ESPNOW_FLAG_RETRANSMISSION      0x04  // Retried message (for receiver debugging)
#define ESPNOW_FLAG_INVALIDATES_PRIOR   0x08  // Receiver should discard buffered messages with older seq
#define ESPNOW_FLAG_DEVICE_ANNOUNCE     0x10  // Device discovery/registration message
#define ESPNOW_FLAG_FLUSH_BEFORE_SEND   0x20  // (Sender-side only) Drain queue before sending this
```

### Message Structure

```cpp
struct ESPNowMessage {
    ESPNowMessageHeader header;
    uint8_t payload[ESPNOW_MAX_PAYLOAD];  // ~240 bytes
    
    // Convenience method
    bool requiresAck() const { return header.flags & ESPNOW_FLAG_REQUIRES_ACK; }
    bool isAck() const { return header.flags & ESPNOW_FLAG_IS_ACK; }
};
```

### Acknowledgement Format (Lightweight)

An ack is itself a small message:

```cpp
struct ESPNowAck {
    uint16_t ackedMessageId;
    uint8_t  ackedSequence;
    uint16_t rss;              // Received Signal Strength (for link quality monitoring)
    uint32_t ackerTimestamp;   // When ack was generated
};
```

When sent, this ack struct becomes the **payload** of an ESPNowMessage with the `IS_ACK` flag set.

---

## 3. Network Layer Architecture

### Component: ESPNowNetworkManager (New Core Service)

Location: `include/shared/espnow/ESPNowNetworkManager.h` and `src/shared/espnow/ESPNowNetworkManager.cpp`

```cpp
namespace espnow {
    class ESPNowNetworkManager {
    public:
        ESPNowNetworkManager(Logger* logger);
        
        // Initialization
        void init(const char* channelMask = "auto");  // auto-detect channel or manual
        void addPeer(const uint8_t* mac, bool isReceiver = true, bool isSender = true);
        void removePeer(const uint8_t* mac);
        
        // Sending
        bool sendMessage(const uint8_t* destMac, const uint8_t* payload, 
                        uint8_t payloadLen, bool requiresAck = false, 
                        uint8_t priority = ESPNOW_PRIORITY_NORMAL,
                        uint16_t ackTimeoutMs = 500);
        
        // Receiving (inbound callbacks—see section below)
        void registerReceiveCallback(void (*callback)(const uint8_t* mac, 
                                                      const ESPNowMessage* msg));
        
        // Status & diagnostics
        struct PeerStatus {
            uint8_t mac[6];
            uint32_t messagesSent;
            uint32_t messagesReceived;
            uint32_t acksReceived;
            uint32_t acksTimeout;
            uint32_t lastSeenTime;
            int8_t   lastRSS;
            bool     isConnected;
        };
        
        PeerStatus getPeerStatus(const uint8_t* mac);
        void logAllPeerStatus();
        
        // Maintenance (called from Brain.task())
        void task();  // Handles ack timeouts, retransmissions, housekeeping
        
    private:
        struct PendingAck {
            uint8_t destMac[6];
            uint16_t messageId;
            uint8_t sequence;
            uint32_t sentTime;
            uint16_t retryCount;
            uint8_t priority;
            ESPNowMessage originalMessage;  // For retransmit
        };
        
        std::map<uint8_t*, PeerStatus> peers;
        std::vector<PendingAck> pendingAcks;
        std::queue<ESPNowMessage> outboundQueue;
        
        void onReceive(const uint8_t* mac, const uint8_t* data, int len);
        void onSend(const uint8_t* mac, esp_now_send_status_t status);
        void processPendingAcks();
        void retransmit(PendingAck& pending);
        void handleIncomingAck(const uint8_t* mac, const ESPNowAck* ack);
        
        // Callback wrappers (C-style for ESP-NOW)
        static void onReceiveWrapper(const esp_now_recv_info_t *recv_info,
                                     const uint8_t *data, int len);
        static void onSendWrapper(const uint8_t *mac_addr, esp_now_send_status_t status);
    };
}
```

---

## 4. Integration Points

### A. System-Level Integration

Update `System` class to own the network manager:

```cpp
class System {
    // ...existing...
    espnow::ESPNowNetworkManager* getNetworkManager();
private:
    espnow::ESPNowNetworkManager networkManager;
};
```

This makes the network layer a **first-class system service** alongside Logger, Config, DroidState.

### B. MechRingController Integration

Instead of directly calling ESP-NOW APIs, MechRingController uses NetworkManager:

```cpp
class MechRingController : public Controller {
    // ...
    void init() override {
        auto netMgr = system->getNetworkManager();
        netMgr->addPeer(leftRingMac, true, false);   // Receiver only
        netMgr->addPeer(rightRingMac, true, false);
        netMgr->registerReceiveCallback(MechRingController::onMessageReceived);
    }
    
    static void onMessageReceived(const uint8_t* mac, const ESPNowMessage* msg) {
        // Parse payload as MechRingMessage, update state
        instance->updateRingState(mac, (const MechRingMessage*)msg->payload);
    }
    
private:
    struct MechRingMessage {  // Payload format (not header)
        uint8_t sequence;
        uint8_t ringId;
        int8_t joystick_x, joystick_y;
        uint32_t buttonState;
        uint16_t crc16;
    };
};
```

### C. ESPNowCmdHandler Implementation

Replace the TODO stub with real implementation:

```cpp
class ESPNowCmdHandler : public CmdHandler {
    bool process(const char* device, const char* command) override {
        // Command format: "espnow:<mac>:<payload>" or device-specific format
        // Use NetworkManager to send command to specified MAC
        
        auto netMgr = system->getNetworkManager();
        bool isCritical = determineCriticality(command);  // "stop" → true, "sound" → false
        return netMgr->sendMessage(destMac, payload, payloadLen, 
                                   isCritical, ESPNOW_PRIORITY_NORMAL);
    }
    
private:
    bool determineCriticality(const char* command) {
        // Heuristic: safety-critical commands require ack
        if (strstr(command, "stop") || strstr(command, "emergency")) return true;
        if (strstr(command, "motor") && strstr(command, "disable")) return true;
        return false;  // Default: best-effort
    }
};
```

---

## 5. Handling Lossy Networks: Reliability Guarantees

### Strategy: **Selective Retransmission**

Not all messages need acks. The sender **decides** based on importance:

| Message Type | Requires Ack? | Max Retries | Timeout | Flags |
|--------------|---------------|------------|---------|-------|
| **Emergency** (stop, failsafe) | ✓ Yes | 3 | 100ms | `REQUIRES_ACK \| FLUSH_BEFORE_SEND \| INVALIDATES_PRIOR` |
| **Safety-critical** (panel lock) | ✓ Yes | 2 | 300ms | `REQUIRES_ACK` |
| **State-critical** (mode changes) | ✓ Yes | 2 | 300ms | `REQUIRES_ACK` |
| **Best-effort** (animations, sounds) | ✗ No | — | — | (none) |
| **Controller input** (joystick) | ✗ No | — | — | `INVALIDATES_PRIOR` (latest only) |

### Retry Logic (in ESPNowNetworkManager.task())

```cpp
void ESPNowNetworkManager::processPendingAcks() {
    uint32_t now = millis();
    
    for (auto& pending : pendingAcks) {
        uint32_t elapsedMs = now - pending.sentTime;
        
        if (elapsedMs >= ACK_TIMEOUT_MS) {
            if (pending.retryCount < MAX_RETRIES) {
                logger->log("ESPNow", WARNING, 
                    "Ack timeout for msg %u to %02X:%02X... retry %u\n",
                    pending.messageId, pending.destMac[0], pending.destMac[1],
                    pending.retryCount + 1);
                retransmit(pending);
                pending.retryCount++;
                pending.sentTime = now;
            } else {
                logger->log("ESPNow", ERROR, 
                    "Max retries exceeded for msg %u to %02X:%02X\n",
                    pending.messageId, pending.destMac[0], pending.destMac[1]);
                // Remove from pending; notify sender via callback
                pending.markForRemoval = true;
            }
        }
    }
    
    // Remove expired entries
    pendingAcks.erase(
        std::remove_if(pendingAcks.begin(), pendingAcks.end(),
                      [](const PendingAck& p) { return p.markForRemoval; }),
        pendingAcks.end()
    );
}
```

### Sender-Side Resilience

Sender can register a callback to be notified of delivery failure:

```cpp
netMgr->sendMessage(mac, payload, len, true, ESPNOW_PRIORITY_HIGH,
                   500, [](bool success, uint16_t msgId) {
    if (!success) {
        logger->log("MySystem", ERROR, "Critical message %u failed delivery!\n", msgId);
        // Trigger failsafe or retry at application level
    }
});
```

---

## 6. Device Discovery & Addressing

### Problem
Hard-coding MACs in firmware is brittle. Devices may change, new devices added.

### Solution: Device Registry + Announcements

```cpp
struct ESPNowPeerRegistry {
    struct PeerInfo {
        uint8_t mac[6];
        char deviceName[32];       // "LeftRing", "RightRing", "DomePanels", etc.
        uint32_t deviceType;       // Bitmask: PEER_TYPE_CONTROLLER, PEER_TYPE_COMMAND_SINK, etc.
        uint32_t lastHeartbeat;
    };
    
    void registerPeer(const uint8_t* mac, const char* name, uint32_t deviceType);
    PeerInfo* lookupByName(const char* name);
    PeerInfo* lookupByMac(const uint8_t* mac);
    void heartbeat(const uint8_t* mac);  // Update lastHeartbeat
};
```

**Device Announcement Protocol:**

At startup, each peripheral sends a discovery message with the `DEVICE_ANNOUNCE` flag:

```cpp
struct ESPNowDeviceAnnounce {
    char deviceName[32];
    uint32_t deviceType;
    uint32_t fwVersion;
    uint8_t capabilities;  // Bitmask: what can this device do?
};
```

MechMind's startup sequence:
1. Initialize NetworkManager, enter "discovery mode"
2. Broadcast a "discovery request" (multicast to all known MACs or discovery MAC)
3. Collect announcements from peripherals (timeout 2-5 seconds)
4. Auto-register discovered peers
5. Log all peers and their capabilities

---

## 7. Outbound Queuing & Ordering (CRITICAL SAFETY CONSIDERATIONS)

### ⚠️ Why Priority Queuing Is DANGEROUS

**DO NOT USE PRIORITY QUEUING.** It breaks ordering guarantees and creates safety hazards.

**Example failure scenario:**
```
Sender queues:  setMotorSpeed=10, setMotorSpeed=20, setMotorSpeed=50, STOP(critical)
Priority queue: STOP jumps to head → [STOP, sms=10, sms=20, sms=50]
Receiver sees:  STOP, sms=10, sms=20, sms=50 ← MOTORS REACTIVATE! 💥
```

### ✓ Safe Approach: FIFO with Selective Acks

```cpp
// NO priority enum needed - all messages FIFO per peer
```

Queue implementation:
- **Strict FIFO per peer** (messages to PeerA cannot pass each other)
- Separate queues per peer (messages to PeerA can overtake messages to PeerB)
- **Selective acknowledgement**: Critical messages require ack+retry, but don't jump queue
- **Flush semantics**: Sender can flush queue before critical message (see below)

### Sequence Numbers: Strict Monotonic Ordering

Every message includes a **per-peer sequence number** (8-bit wrapping counter):

```cpp
struct ESPNowMessageHeader {
    uint16_t messageId;          // Unique per sender (for ack correlation)
    uint8_t  sequence;           // Per-peer monotonic counter (wraps at 255)
    uint8_t  flags;
    uint32_t timestamp;
    uint8_t  protocol_version;
    uint8_t  payload_length;
};
```

**Sender responsibilities:**
- Maintain separate sequence counter per peer
- Increment on every message sent (wrapping at 255)
- Never reuse sequence numbers (except on wrap)

**Receiver responsibilities:**
- Track `lastReceivedSeq` per sender
- Detect gaps: `if ((receivedSeq - lastReceivedSeq) % 256 > 1)` → messages were dropped
- **Critical: NEVER skip messages due to out-of-order arrival**
- Buffer out-of-order messages and process in sequence order (see below)

### Receiver-Side Reordering Buffer

To handle network reordering without discarding messages:

```cpp
class SequenceBuffer {
    struct BufferedMessage {
        uint8_t sequence;
        ESPNowMessage message;
        uint32_t receivedTime;
    };
    
    std::map<uint8_t, BufferedMessage> buffer;  // Key = sequence number
    uint8_t nextExpectedSeq = 0;
    
    void onReceive(const ESPNowMessage* msg) {
        if (msg->header.sequence == nextExpectedSeq) {
            // In-order arrival: process immediately
            processMessage(msg);
            nextExpectedSeq = (nextExpectedSeq + 1) % 256;
            
            // Check if buffered messages can now be processed
            drainBuffer();
        } else if (isAhead(msg->header.sequence, nextExpectedSeq)) {
            // Out-of-order (arrived early): buffer it
            buffer[msg->header.sequence] = {msg->header.sequence, *msg, millis()};
            
            // Timeout old buffered messages (likely dropped predecessor)
            cleanupStaleBuffers();
        } else {
            // Duplicate or very old message: discard
            logger->log("ESPNow", DEBUG, "Discarding old/duplicate seq %u (expected %u)\n",
                       msg->header.sequence, nextExpectedSeq);
        }
    }
    
    void drainBuffer() {
        while (buffer.count(nextExpectedSeq)) {
            processMessage(&buffer[nextExpectedSeq].message);
            buffer.erase(nextExpectedSeq);
            nextExpectedSeq = (nextExpectedSeq + 1) % 256;
        }
    }
    
    void cleanupStaleBuffers() {
        uint32_t now = millis();
        for (auto it = buffer.begin(); it != buffer.end(); ) {
            if (now - it->second.receivedTime > REORDER_BUFFER_TIMEOUT_MS) {
                logger->log("ESPNow", WARNING, "Dropping seq %u after timeout (gap in sequence)\n",
                           it->first);
                // Assume message(s) before this were permanently lost
                nextExpectedSeq = it->first;
                drainBuffer();
                return;
            }
            ++it;
        }
    }
};
```

**Key parameters:**
- `REORDER_BUFFER_TIMEOUT_MS = 200` — How long to wait for missing messages
- Buffer size limit (e.g., max 8 messages) to prevent memory exhaustion

### Flush-Before-Critical Pattern

For truly urgent messages (e.g., emergency stop), sender can **drain the queue** before sending:

```cpp
bool sendCriticalMessage(const uint8_t* destMac, const uint8_t* payload, uint8_t len) {
    // Option 1: Block until queue drains (safest, but adds latency)
    while (!isPeerQueueEmpty(destMac)) {
        task();  // Process pending sends
        delayMicroseconds(100);
    }
    
    return sendMessage(destMac, payload, len, true, ESPNOW_FLUSH_BEFORE_SEND);
}

// Or Option 2: Flush queue (send all pending messages immediately)
bool sendMessage(..., uint8_t flags) {
    if (flags & ESPNOW_FLUSH_BEFORE_SEND) {
        flushQueueForPeer(destMac);  // Send all pending messages now
    }
    // Then queue this message
}
```

**Usage:**
```cpp
// Normal commands: queued FIFO
netMgr->sendMessage(motorMac, "setSpeed:10", 11, false);
netMgr->sendMessage(motorMac, "setSpeed:20", 11, false);
netMgr->sendMessage(motorMac, "setSpeed:50", 11, false);

// Emergency stop: flush queue first, then send with ack
netMgr->sendCriticalMessage(motorMac, "STOP", 4);  // Guarantees ordering
```

### Alternative: Sequence-Based Invalidation

Add a flag to mark messages that **invalidate prior commands**:

```cpp
#define ESPNOW_FLAG_INVALIDATES_PRIOR  0x20  // Receiver should ignore earlier seqs
```

Receiver logic:
```cpp
if (msg->header.flags & ESPNOW_FLAG_INVALIDATES_PRIOR) {
    // Clear any buffered messages with older sequences
    for (auto it = buffer.begin(); it != buffer.end(); ) {
        if (isBefore(it->first, msg->header.sequence)) {
            logger->log("ESPNow", INFO, "Invalidating buffered seq %u (superceded by %u)\n",
                       it->first, msg->header.sequence);
            it = buffer.erase(it);
        } else {
            ++it;
        }
    }
    // Process this critical message immediately, even if out of order
    processMessage(msg);
    nextExpectedSeq = (msg->header.sequence + 1) % 256;
}
```

**When to use:**
- Emergency stop commands
- Failsafe triggers
- Mode changes that reset actuator state

**Trade-off:** May discard legitimate buffered messages, but guarantees critical message takes effect immediately.

---

---

## 8. Link Quality Monitoring

Track connection health per peer:

```cpp
struct PeerStatus {
    uint8_t mac[6];
    uint32_t messagesSent;
    uint32_t messagesReceived;
    uint32_t acksReceived;
    uint32_t acksTimeout;
    uint32_t lastSeenTime;
    int8_t lastRSS;              // Received Signal Strength (dBm)
    float deliveryRate;          // acksReceived / messagesSent (requires-ack only)
    bool isConnected;
};
```

Exposed via `getNetworkStatus()` for logging and diagnostics:

```cpp
void logNetworkHealth() {
    auto netMgr = system->getNetworkManager();
    for (const auto& peer : netMgr->getAllPeers()) {
        logger->log("Network", INFO, 
            "%s (%02X:%02X...) - Delivery: %.1f%% | RSS: %d dBm | Last: %lu ms\n",
            peer.deviceName, peer.mac[0], peer.mac[1],
            peer.deliveryRate * 100, peer.lastRSS,
            millis() - peer.lastSeenTime);
    }
}
```

---

## 9. Integration with ActionMgr & CmdHandlers

The network layer is **transparent** to ActionMgr:

1. **Outbound:** ActionMgr queues command "espnow:device:action" → ESPNowCmdHandler → NetworkManager.sendMessage()
2. **Inbound:** Network receives joystick data → MechRingController updates state → getAction() returns next action

Example flow:

```
ActionMgr.fireAction("play_dome_animation")
  ↓
CmdHandler.process("espnow", "dome_panels:play_animation:id=5")
  ↓
ESPNowCmdHandler determines: requiresAck=(animation != critical)=false
  ↓
ESPNowNetworkManager.sendMessage(domePanelsMac, payload, false)
  ↓
Peripheral receives, plays animation immediately (no ack needed)
```

---

## 10. Configuration

Add to `hardware.config.h`:

```cpp
// ESP-NOW Network Layer Config
#define ESPNOW_CHANNEL                  11              // WiFi channel (must match peripherals)
#define ESPNOW_PEER_DISCOVERY_TIMEOUT   5000            // ms to wait for device announces
#define ESPNOW_ACK_TIMEOUT_MS           500             // ms to wait for ack
#define ESPNOW_MAX_RETRIES              2               // Attempts before giving up
#define ESPNOW_QUEUE_SIZE               32              // Max pending messages
#define ESPNOW_RATE_LIMIT_PER_SEC       50              // Max messages per peripheral per second

// Device Type Flags
#define ESPNOW_DEVICE_TYPE_CONTROLLER   0x01
#define ESPNOW_DEVICE_TYPE_COMMAND_SINK 0x02
#define ESPNOW_DEVICE_TYPE_SENSOR       0x04
```

Add to Config via NVS:

```cpp
config->putInt("ESPNow", "Channel", ESPNOW_CHANNEL);
config->putInt("ESPNow", "AckTimeoutMs", ESPNOW_ACK_TIMEOUT_MS);
// Peer registry stored as JSON or CSV in NVS
```

---

## 11. File Structure

### Library Repository: `MechNet`

**New PlatformIO project** (separate Git repository):

```
MechNet/
├── library.json                          # PlatformIO library metadata
│   └── name: "MechNet"
│   └── version: "1.0.0"
│   └── repository: "https://github.com/user/MechNet"
│
├── README.md                             # Installation & quick start
├── CHANGELOG.md                          # Version history & breaking changes
├── LICENSE                               # CC BY-NC-SA 4.0 (matching MechMind)
│
├── src/MechMindESPNow/
│   ├── ESPNowNetworkManager.h
│   ├── ESPNowNetworkManager.cpp
│   ├── ESPNowPeerRegistry.h
│   ├── ESPNowPeerRegistry.cpp
│   ├── ESPNowMessage.h                   # Message serialization/deserialization
│   ├── ESPNowMessage.cpp
│   ├── ESPNowTypes.h                     # Structs, flags, constants
│   ├── ILogger.h                         # Abstract logger (no MechMind dependencies)
│   │
│   └── internal/
│       ├── SequenceBuffer.h
│       ├── SequenceBuffer.cpp
│       └── PendingAckTracker.h
│
├── examples/
│   ├── hub_basic/
│   │   ├── src/main.cpp
│   │   └── platformio.ini
│   └── peripheral_basic/
│       ├── src/main.cpp
│       └── platformio.ini
│
├── test/
│   ├── test_message_ordering.cpp
│   ├── test_ack_retry.cpp
│   ├── test_peer_registry.cpp
│   └── CMakeLists.txt
│
└── platformio.ini                        # Library build configuration
```

### MechMind Hub: Integration Files

**Modified files** in `MechMind/`:

- `include/droid/core/System.h` 
  - Add `MechMindESPNow::ESPNowNetworkManager* getNetworkManager()`
  - Create LoggerAdapter to implement `ILogger` interface

- `src/droid/core/System.cpp`
  - Initialize ESPNowNetworkManager with LoggerAdapter
  - Call `networkManager->init()` in System constructor or init phase

- `include/droid/command/ESPNowCmdHandler.h`
  - Implement full CmdHandler with criticality detection
  - Define device-specific message payload structs (e.g., for dome commands)

- `src/droid/command/ESPNowCmdHandler.cpp`
  - Real implementation using ESPNowNetworkManager
  - Payload serialization logic

- `src/droid/brain/Brain.cpp`
  - Add `netMgr->task()` call in `Brain::task()`
  - Initialize peer registry with known peripheral MACs

- `settings/hardware.config.h`
  - Add ESP-NOW channel, timeout, retry constants
  - Define device type flags

**New files** in `MechMind/`:

- `include/droid/adapters/ESPNowLoggerAdapter.h` (optional, if logger integration is complex)
  - Adapts MechMind's Logger to `ILogger` interface

### Peripheral Firmware: Ring Controller (Example)

**New standalone project** (or multi-env in shared monorepo):

```
MechRing-Controller/
├── platformio.ini
│   └── lib_deps = MechNet @ ^1.0.0
│
├── include/
│   ├── ring_config.h
│   ├── RingInputSender.h
│   ├── RingBLEReceiver.h
│   └── LoggerAdapter.h
│
├── src/
│   ├── main.cpp
│   ├── RingInputSender.cpp
│   └── LoggerAdapter.cpp
│
└── README.md
```

### Peripheral Firmware: Panel Controller (Example)

```
MechPanel-Controller/
├── platformio.ini
│   └── lib_deps = MechNet @ ^1.0.0
│
├── include/
│   ├── panel_config.h
│   ├── PanelCommandReceiver.h
│   └── LoggerAdapter.h
│
├── src/
│   ├── main.cpp
│   ├── PanelCommandReceiver.cpp
│   └── LoggerAdapter.cpp
│
└── README.md
```

### Summary: Separate PlatformIO Projects

Each project is independent:
- **MechNet** (Library): Standalone PlatformIO project/library
- **MechMind** (Hub): PlatformIO project depending on MechNet
- **MechRing-Controller**: PlatformIO project depending on MechNet
- **MechPanel-Controller**: PlatformIO project depending on MechNet

Each has its own `platformio.ini`, Git repository, and versioning. They reference MechNet via PlatformIO's library dependency mechanism.

---

## 12. Safety Comparison: Different Approaches to Critical Messages

| Approach | Ordering Guarantee | Reliability | Latency | Safety Risk |
|----------|-------------------|-------------|---------|-------------|
| **Priority Queue** (❌ REJECTED) | ❌ Broken | ✓ Acks/retries | ✓ Low | ⚠️ **HIGH** (critical msgs arrive before non-critical, causing wrong final state) |
| **FIFO + Selective Acks** (✓ RECOMMENDED) | ✓ Strict | ✓ Acks for critical | ⚠️ Queuing delay | ✓ Safe (order preserved) |
| **FIFO + Flush-Before-Critical** | ✓ Strict | ✓ Acks + immediate | ✓ Low (queue drained) | ✓ Safe + fast |
| **Sequence-Based Invalidation** | ⚠️ Gaps allowed | ✓ Acks + force-process | ✓ Low (skip buffered) | ✓ Safe (old msgs discarded) |
| **No Acks (Current)** | ✓ Strict (if all arrive) | ❌ None | ✓ Very low | ⚠️ **MEDIUM** (silent drops) |

### Recommended Strategy: **Hybrid Approach**

Use **different mechanisms for different message types**:

| Message Type | Strategy | Flags | Example |
|--------------|----------|-------|---------|
| **Emergency stop** | Flush + Ack + Invalidate | `REQUIRES_ACK \| FLUSH_BEFORE_SEND \| INVALIDATES_PRIOR` | Motor emergency stop, failsafe trigger |
| **Safety-critical commands** | FIFO + Ack | `REQUIRES_ACK` | Panel lock, power enable/disable |
| **State changes** | FIFO + Ack | `REQUIRES_ACK` | Mode changes, configuration updates |
| **Best-effort commands** | FIFO, no ack | (none) | Animations, idle sounds, non-critical lights |
| **High-frequency sensor data** | FIFO, no ack, invalidates | `INVALIDATES_PRIOR` | Joystick positions (latest value is what matters) |

### Code Example: Smart Criticality Detection

```cpp
class ESPNowNetworkManager {
    bool sendMessage(const uint8_t* destMac, const uint8_t* payload, 
                    uint8_t payloadLen, MessageCriticality criticality) {
        
        uint8_t flags = 0;
        
        switch (criticality) {
            case CRITICALITY_EMERGENCY:
                // Flush queue, require ack, invalidate prior commands
                flags = ESPNOW_FLAG_REQUIRES_ACK | 
                        ESPNOW_FLAG_FLUSH_BEFORE_SEND |
                        ESPNOW_FLAG_INVALIDATES_PRIOR;
                break;
                
            case CRITICALITY_SAFETY:
                // Require ack, maintain order
                flags = ESPNOW_FLAG_REQUIRES_ACK;
                break;
                
            case CRITICALITY_STATE:
                // Require ack for reliability, but queue normally
                flags = ESPNOW_FLAG_REQUIRES_ACK;
                break;
                
            case CRITICALITY_BEST_EFFORT:
                // No ack, FIFO queuing
                flags = 0;
                break;
                
            case CRITICALITY_EPHEMERAL:
                // Latest value only (e.g., joystick), invalidate old buffered values
                flags = ESPNOW_FLAG_INVALIDATES_PRIOR;
                break;
        }
        
        return sendMessageInternal(destMac, payload, payloadLen, flags);
    }
};
```

### Edge Case: Retransmissions and Ordering

**Problem:** If message seq=5 times out and is retransmitted, but receiver already processed seq=6, should seq=5 be processed?

**Solution:** 
- **Retransmissions keep original sequence number** (don't increment)
- Receiver checks: `if (seq < nextExpectedSeq && !(flags & INVALIDATES_PRIOR))` → discard as duplicate
- For critical messages with `INVALIDATES_PRIOR`, process even if older sequence

```cpp
void onReceiveRetransmission(const ESPNowMessage* msg) {
    if (msg->header.flags & ESPNOW_FLAG_RETRANSMISSION) {
        if (msg->header.sequence < nextExpectedSeq) {
            // Already processed newer messages
            if (msg->header.flags & ESPNOW_FLAG_INVALIDATES_PRIOR) {
                logger->log("ESPNow", WARNING, 
                    "Processing RETRANSMIT of critical seq %u (even though seq %u already seen)\n",
                    msg->header.sequence, nextExpectedSeq);
                processMessage(msg);  // Force-process critical retransmit
            } else {
                logger->log("ESPNow", DEBUG, "Ignoring duplicate retransmit seq %u\n", 
                           msg->header.sequence);
                sendAck(msg);  // Still ack it so sender stops retrying
            }
        } else {
            // Normal processing (seq was missing, now arrived)
            onReceive(msg);
        }
    }
}
```

---

## 15. Suggested Implementation Phases

### Phase 0: Create Library Scaffold (Foundation)
- [ ] Create `MechNet` PlatformIO project/repository
- [ ] Implement `ESPNowTypes.h` (message structures, flags, constants)
- [ ] Implement `ILogger` interface
- [ ] Set up PlatformIO library manifest and examples
- [ ] Write README with installation instructions
- **Deliverable**: Empty library with correct structure, can be imported by other projects

### Phase 1: Core Network Layer (MVP - Library)
- [ ] Implement `ESPNowNetworkManager` (send/receive/ack/retry logic)
- [ ] Implement `ESPNowPeerRegistry` (peer add/remove/status)
- [ ] Implement message serialization in `ESPNowMessage`
- [ ] Add unit tests for message ordering and sequence logic
- [ ] Add basic examples (hub-to-peripheral simple send)
- [ ] Logging & diagnostics framework
- **Deliverable**: Library tested with 2-3 peripherals communicating with hub

### Phase 2: MechMind Hub Integration
- [ ] Create `MechMindLoggerAdapter` to bridge library's ILogger
- [ ] Update `System` to own ESPNowNetworkManager
- [ ] Implement real `ESPNowCmdHandler` (command routing via network)
- [ ] Integrate library into MechMind build (add to `lib_deps`)
- [ ] Update `Brain.task()` to call `netMgr->task()`
- [ ] Test sending commands to simulated peripherals
- **Deliverable**: Hub can send commands via ESP-NOW to any peer

### Phase 3: First Peripheral Integration (Ring Controller)
- [ ] Create standalone `MechRing-Controller` firmware project
- [ ] Implement `LoggerAdapter` for ring firmware
- [ ] Create `RingInputSender` using library's NetworkManager
- [ ] Define ring-specific payload structs (joystick + buttons)
- [ ] Test joystick data reception on hub
- **Deliverable**: Hub receives joystick input from ring controller

### Phase 4: Multi-Peripheral Expansion
- [ ] Create `MechPanel-Controller` firmware
- [ ] Implement command receiver for panel actions
- [ ] Test hub sending panel commands
- [ ] Verify message ordering and ack timeout handling
- [ ] Stress test with rapid commands to multiple peripherals
- **Deliverable**: Hub can control multiple device types simultaneously

### Phase 5: Advanced Features (Library)
- [ ] Device discovery & peer registry persistence (NVS)
- [ ] Peer registry JSON serialization
- [ ] Link quality monitoring dashboard
- [ ] Rate limiting & congestion detection
- [ ] Library versioning policy & changelog
- **Deliverable**: Production-ready library with full feature set

### Phase 6: Hardening & Deployment
- [ ] Field testing with real hardware
- [ ] Failsafe recovery (network timeout handling)
- [ ] OTA firmware update strategy for peripherals
- [ ] Documentation & user guide
- [ ] Tag library v1.0.0 and publish
- **Deliverable**: Stable, deployed network layer

---

## 16. Version Management & Compatibility

### Semantic Versioning for Library

Library version format: `MAJOR.MINOR.PATCH`

- **MAJOR**: Protocol changes that break compatibility (e.g., new message header format)
- **MINOR**: Feature additions, backward-compatible (e.g., new message flags)
- **PATCH**: Bug fixes, no API changes

### Protocol Version Field

Message header includes:

```cpp
struct ESPNowMessageHeader {
    uint8_t protocol_version;   // Currently 1; increment on breaking changes
    // ...
};
```

Receiver logic:

```cpp
void onReceive(const ESPNowMessage* msg) {
    if (msg->header.protocol_version != ESPNOW_PROTOCOL_VERSION) {
        logger->log("ESPNow", WARNING, 
            "Incompatible protocol version: got %u, expected %u\n",
            msg->header.protocol_version, ESPNOW_PROTOCOL_VERSION);
        return;  // Discard message
    }
    // Process message
}
```

### Firmware & Library Compatibility Matrix

Example:

| MechMind Ver | Requires Library Ver | Ring Controller | Panel Controller |
|--------------|----------------------|-----------------|------------------|
| 2.0          | ESP-NOW ^1.0.0       | ^1.0.0          | ^1.0.0           |
| 2.1          | ESP-NOW ^1.1.0       | ^1.1.0          | ^1.1.0           |
| 3.0          | ESP-NOW ^2.0.0       | ^2.0.0          | ^2.0.0           |

### Deprecation & Migration

When breaking protocol change is needed:

1. **v1.2.0**: Add new message format (support both old + new)
2. **v1.3.0**: Mark old format deprecated in README
3. **v2.0.0**: Remove old format support, bump MAJOR version
4. Firmware projects update when ready (no forced deprecation)

---

## 17. Comparison: Selective Ack vs. Always-Ack

| Approach | Pros | Cons |
|----------|------|------|
| **Selective Ack (Proposed)** | Efficient bandwidth use; critical messages guaranteed; best-effort messages don't block sender | More complex; requires explicit priority flags |
| **Always-Ack** | Simple; every message guaranteed | ESP-NOW bandwidth limited; ack floods can congest network; unnecessary overhead for non-critical data |
| **No Ack (Current)** | Minimal overhead | No reliability; silent failures; hard to debug |

---

## 18. Library Dependency Abstraction

### Problem: Library Must Not Depend on MechMind

The library cannot import from `droid/core/System.h`, `shared/common/Logger.h`, etc. It must be self-contained.

### Solution: Dependency Injection via Interfaces

#### Logger Abstraction

Library defines:

```cpp
// MechMindESPNow/ILogger.h
namespace MechMindESPNow {
    enum LogLevel {
        DEBUG = 0,
        INFO = 1,
        WARNING = 2,
        ERROR = 3
    };
    
    class ILogger {
    public:
        virtual ~ILogger() = default;
        virtual void log(const char* tag, LogLevel level, 
                        const char* format, ...) = 0;
    };
}
```

Each application implements:

```cpp
// MechMind/include/droid/adapters/MechMindLoggerAdapter.h
class MechMindLoggerAdapter : public MechMindESPNow::ILogger {
private:
    Logger* mechmindLogger;
    
    static LogLevel convertLevel(MechMindESPNow::LogLevel level) {
        switch (level) {
            case MechMindESPNow::DEBUG: return LogLevel::DEBUG;
            case MechMindESPNow::INFO: return LogLevel::INFO;
            // ...
        }
    }
    
public:
    MechMindLoggerAdapter(Logger* logger) : mechmindLogger(logger) {}
    
    void log(const char* tag, MechMindESPNow::LogLevel level,
            const char* format, ...) override {
        va_list args;
        va_start(args, format);
        mechmindLogger->logv(tag, convertLevel(level), format, args);
        va_end(args);
    }
};
```

Ring Controller implements its own (simpler) logger.

### Benefits

- **Library is framework-agnostic**: Works with any project using any logging system
- **Type-safe**: Compile-time checking of interface contracts
- **Testable**: Can inject mock logger for unit tests
- **Flexible**: Each application maps library LogLevel to its own LogLevel enum

---

## 19. Build Configuration for Separate Projects

Each PlatformIO project maintains its own `platformio.ini` with independent build configurations.

### MechNet Library (library/platformio.ini)

```ini
[platformio]
default_envs = lib

[env:lib]
platform = espressif32
board = esp32dev
framework = arduino
; Library doesn't typically build to a board, but can be tested
```

### MechMind Hub (mechmind-hub/platformio.ini)

```ini
[env:default]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
    MechNet @ ^1.0.0
    USB-Host-Shield
    NimBLE-Arduino
```

### Ring Controller (ring-controller/platformio.ini)

```ini
[env:default]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
    MechNet @ ^1.0.0
    NimBLE-Arduino
```

### Panel Controller (panel-controller/platformio.ini)

```ini
[env:default]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
    MechNet @ ^1.0.0
```

**Building**: Each project is built independently:
```bash
cd mechmind-hub && platformio run
cd ring-controller && platformio run
cd panel-controller && platformio run
```

Or use symlink for local development:
```ini
[env:dev]
lib_deps = symlink:///path/to/MechNet
```

---

## 20. Testing Strategy for Library

### Unit Tests (Library Level)

Run independently, don't require ESP32 hardware:

```bash
cd MechNet
platformio test
```

Test areas:
- Message serialization/deserialization
- Sequence number wrapping logic
- Reordering buffer (out-of-order message handling)
- Ack timeout and retry calculation
- Peer registry (add/remove/lookup)

### Integration Tests (Per-Application)

Test library in context of actual firmware:

```cpp
// Ring Controller test
void test_send_joystick_data() {
    RingInputSender sender(&loggerAdapter);
    sender.init();
    
    // Simulate joystick input
    sender.sendJoystickData(50, -30, 0x0001);
    
    // Verify message was queued
    assert(netMgr->getPendingMessageCount() == 1);
}
```

### Hardware Tests (End-to-End)

Two physical ESP32 boards:

```cpp
Hub:        Send "command" → Hub queues message
Peripheral: Receive "command" → Log receipt + ack
Hub:        Verify ack received within timeout
```

---

## 15. Suggested Implementation Phases

### Phase 1: Core Network Layer (MVP)
- [ ] ESPNowNetworkManager (send/receive/ack logic)
- [ ] Basic peer management (add/remove/status)
- [ ] Logging & diagnostics
- [ ] Tested with 2-3 peripherals

### Phase 2: MechRingController Integration
- [ ] Refactor MechRingController to use NetworkManager
- [ ] Device announcement support
- [ ] Test joystick data reception

### Phase 3: Command Distribution
- [ ] Implement ESPNowCmdHandler for outbound commands
- [ ] Priority queuing
- [ ] Retry logic

### Phase 4: Advanced Features
- [ ] Peer registry (NVS persistence)
- [ ] Runtime peer discovery
- [ ] Link quality monitoring dashboard
- [ ] Rate limiting & congestion control

---

## 21. Open Questions

1. **WiFi interference:** Will MechMind run WiFi (for telemetry/OTA updates)? If so, both on same channel or separate channels?
2. **Broadcast support:** Do you need to broadcast to all peripherals simultaneously, or always point-to-point?
3. **Peripheral heterogeneity:** Will peripherals have different capabilities (some accept commands only, others send sensor data)? Should the registry track this?
4. **Failsafe strategy:** If ack times out multiple times, what happens? Retry forever, give up, trigger failsafe?
5. **Message latency:** What's acceptable latency for joystick input? (Should be <50ms ideally)
6. **Device count:** How many peripheral ESP32 devices maximum? (Affects queue sizing, registry size)

---

## 22. References

- [ESP-NOW Docs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html)
- [ESP-NOW Best Practices](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/esp-now.html#esp-now-best-practices) (esp. on channel selection, bandwidth)
- [Current ESPNowCmdHandler](../src/droid/command/ESPNowCmdHandler.cpp) (to be enhanced)
- [System.h](../include/droid/core/System.h) (integration point)
- [ActionMgr](../include/droid/command/ActionMgr.h) (how commands flow)
