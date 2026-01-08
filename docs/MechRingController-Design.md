# MechRingController Design Recommendation

## Overview

This document outlines the design for adding a new `MechRingController` to MechMind—a controller implementation using two custom MechRing devices communicating via ESP-NOW protocol.

---

## 1. Architecture Fit (Strengths of Current Pattern)

The existing controller pattern is well-designed for this use case:

- **Abstract `Controller` base** defines a clean interface: `getJoystickPosition()`, `getAction()`, with lifecycle methods (`init()`, `task()`, `factoryReset()`)
- **Pluggable instantiation** in Brain.cpp via config strings—no code changes needed to wire it in once written
- **Trigger mapping** pattern (triggerMap + config) is reusable for custom actions
- **Fault checking** pattern (connection loss detection) is proven across PS3/Sony implementations
- **Single-instance constraint** is documented (comment in Brain.cpp line 188); all controllers use static singleton pattern with assertions

---

## 2. Key Design Decisions for MechRingController

### A. Hardware Architecture

- Two ESP-NOW peers (left/right MechRing remotes) → single Brain receiver
- ESP-NOW callback functions will handle inbound messages asynchronously
- Store joystick + button state from both rings in instance variables (updated by callback)
- `task()` method calls fault checking; `getAction()` and `getJoystickPosition()` read the cached state

### B. State Management Pattern

Follow the **DualSonyNavController** model (NOT DualRingController—yours is simpler):

- Create a `struct RingState` holding:
  - Volatile state variables (joystick X/Y, button flags) updated by ESP-NOW callback
  - Connection tracking (MAC, last message time, isConnected flag)
  - Fault counters (message timeouts, CRC errors)
- Two instances: `RingState leftRing`, `RingState rightRing`
- This separation matches the LEFT/RIGHT joystick enum in the base Controller class

### C. Message Protocol Design (Custom)

Define a simple binary struct for ESP-NOW payload:

```cpp
struct MechRingMessage {
    uint8_t sequence;           // detect missed messages
    uint8_t ringId;             // 0=left, 1=right
    int8_t joystick_x;          // -128 to +127
    int8_t joystick_y;          // -128 to +127
    uint32_t buttonState;       // bitmask for all buttons/modifiers
    uint16_t crc16;             // simple checksum
};
```

Characteristics:
- ~16 bytes → fits within ESP-NOW 250-byte limit
- Sequence numbers allow timeout detection (missed 3+ consecutive messages = disconnection)

### D. ESP-NOW Integration

Create a **separate helper class** `MechRingESPNow` (parallel to existing `DualRingBLE`):

- Owns the ESP-NOW registration and callbacks
- Singleton or static methods (since ESP-NOW callbacks are C-style functions)
- Called from MechRingController's `init()` and `task()`
- Location: `include/shared/blering/MechRingESPNow.h` and `src/shared/blering/MechRingESPNow.cpp`

This mirrors how `DualRingBLE` abstracts BLE details away from `DualRingController`.

### E. Trigger Mapping

- Create `settings/MechRingTrigger.map` following the existing pattern
- Define button combinations (e.g., "LC" = left ring clicker C, "LA+LD" = A+D combo)
- Same runtime config override mechanism as PS3/Sony controllers

### F. Configuration Keys (hardware.config.h)

Add to hardware.config.h:

```cpp
#define CONTROLLER_OPTION_MECHRING              "MechRing"
#define CONFIG_DEFAULT_MECHRING_LEFT_MAC        "XX:XX:XX:XX:XX:XX"
#define CONFIG_DEFAULT_MECHRING_RIGHT_MAC       "XX:XX:XX:XX:XX:XX"
#define CONFIG_DEFAULT_MECHRING_TIMEOUT         200  // ms before declaring disconnection
```

---

## 3. File Structure

Files to create:
- `include/droid/controller/MechRingController.h`
- `include/shared/blering/MechRingESPNow.h`
- `src/droid/controller/MechRingController.cpp`
- `src/shared/blering/MechRingESPNow.cpp`
- `settings/MechRingTrigger.map`

Files to modify:
- `src/droid/brain/Brain.cpp` — add instantiation case
- `settings/hardware.config.h` — add constants
- `include/droid/controller/Controller.h` — add `MECHRING` to `ControllerType` enum

---

## 4. Key Implementation Details

### MechRingController.h Structure

```cpp
class MechRingController : public Controller {
private:
    struct RingState {
        char MAC[20];
        volatile int8_t joystick_x, joystick_y;
        volatile uint32_t buttonState;
        volatile uint32_t lastMsgTime;
        volatile uint8_t lastSeqNum;
        volatile bool isConnected;
    };
    
    RingState leftRing, rightRing;
    std::map<String, String> triggerMap;
    static MechRingController* instance;  // singleton pattern
    
    void faultCheck();
    String getTrigger();
};
```

### ESP-NOW Callback (in MechRingESPNow)

```cpp
void onDataReceived(const esp_now_recv_info_t *recv_info, 
                    const uint8_t *data, int len) {
    // Validate CRC, route to correct ring state, update timestamp
    // Call instance->onMessageReceived(msg)
}
```

### Fault Detection Logic

- **Connection loss**: Message timeout (e.g., 500ms with no update)
- **Bad data**: CRC failures, out-of-sequence messages
- Same timeout pattern as PS3BtController (activeTimeout, inactiveTimeout config keys)

---

## 5. Integration Points in Brain.cpp

In the controller instantiation switch statement (~line 67):

```cpp
} else if (whichService == CONTROLLER_OPTION_MECHRING) {
    logger->log(name, DEBUG, "Initializing MechRing\n");
    controller = new droid::controller::MechRingController(CONTROLLER_OPTION_MECHRING, system);
} else {
```

Also update `Controller.h` enum:

```cpp
enum ControllerType {
    STUB, DUAL_SONY, DUAL_RING, PS3_BT, PS3_USB, MECHRING
};
```

---

## 6. Design Rationale

| Aspect | Rationale |
|--------|-----------|
| **Reuse existing patterns** | Follows singleton, trigger mapping, fault checking already proven in codebase |
| **Separate helper class** | ESP-NOW logic isolated (parallel to DualRingBLE), not tangled with action logic |
| **RingState struct** | Matches existing LEFT/RIGHT enum and proven pattern from DualSonyNavController |
| **Binary message format** | Compact, fast parsing, deterministic size for ESP-NOW |
| **Trigger mapping** | Allows flexible button combinations without code changes; config-overridable |
| **Fault detection** | Built-in timeout detection, message sequencing, CRC validation |

---

## 7. Open Questions

Before implementation, clarify:

1. **Message frequency** — How often will rings send updates? (Recommend 50Hz = 20ms)
2. **WiFi coexistence** — Does MechMind run WiFi? (May need careful channel selection for ESP-NOW)
3. **Pairing mechanism** — How will left/right rings identify themselves? (MAC address in message, or one-time config?)
4. **Button combinations** — Which multi-button combos do you want to support? (Affects trigger map design)
5. **Deadband requirements** — Will you need joystick deadband filtering like PS3 controllers do?

---

## 8. Implementation Sequence (Suggested)

1. Add constants to `hardware.config.h`
2. Update `Controller.h` enum
3. Create `MechRingESPNow` helper class (handles ESP-NOW protocol)
4. Create `MechRingController` class (implements Controller interface)
5. Create `MechRingTrigger.map` (default button-to-action mappings)
6. Update `Brain.cpp` instantiation logic
7. Test with hardware

---

## 9. References

- [Controller.h](../include/droid/controller/Controller.h) — Base interface
- [DualSonyNavController](../include/droid/controller/DualSonyNavController.h) — Reference for dual-device pattern
- [DualRingBLE.h](../include/shared/blering/DualRingBLE.h) — Reference for hardware abstraction helper
- [Brain.cpp](../src/droid/brain/Brain.cpp) — Where controller instantiation occurs
- [hardware.config.h](../settings/hardware.config.h) — Where configuration defaults live
