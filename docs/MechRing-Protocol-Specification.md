# MechRing Controller Protocol Specification

**Version**: 1.0  
**Date**: January 31, 2026  
**Status**: Specification Ready for Implementation

---

## 1. Overview

### 1.1 Purpose

This document defines the application-level protocol for MechRing wireless controllers communicating with MechMind Brain over MechNet (ESP-NOW transport). MechRings are handheld joystick+button controllers worn on the user's hands, used in pairs for droid control.

### 1.2 Design Principles

- **Human-readable format**: Delimited text messages for debugging and development
- **Compact but clear**: Balance message size with readability
- **High-frequency updates**: 50 Hz when active for responsive control
- **Battery-optimized**: Adaptive rate (1 Hz when idle) for extended runtime
- **Safety-critical**: Fast connection loss detection for emergency stop
- **Leverages MechNet**: Transport reliability, encryption, and link monitoring handled by MechNet library

---

## 2. Network Architecture

### 2.1 Topology

```
MechMind Brain (MechNet Master)
    ├── DriveRing-A3F2 (MechNet Remote) → RIGHT joystick
    └── DomeRing-B81D  (MechNet Remote) → LEFT joystick
```

- **Brain**: Single MechNet Master node
- **Rings**: Two MechNet Remote nodes with role-based names
- **Transport**: ESP-NOW via MechNet library (secure, reliable)

### 2.2 Node Naming Strategy

**Role-based provisioning** (configured via USB console on each ring):

| Node Base Name | Purpose | Brain Mapping |
|---------------|---------|---------------|
| `DriveRing` | Body/drive control | RIGHT joystick |
| `DomeRing` | Dome/head control | LEFT joystick |

MechNet automatically appends MAC address suffix for uniqueness:
- `DriveRing` → `DriveRing-A3F2` (last 2 bytes of MAC)
- `DomeRing` → `DomeRing-B81D`

**Provisioning Example** (via ring USB console):
```
> provision
Network Name: R2D2Net
Node Role: Remote
Node Base Name: DriveRing
Channel: 6
PSK: [paste 64-char hex]
```

---

## 3. Message Format

### 3.1 Ring → Brain: State Update Message

**Purpose**: Report current joystick position and button states  
**Frequency**: Adaptive (50 Hz active, 1 Hz idle)  
**Reliability**: Unreliable (`requiresAck=false`) - high frequency makes retries unnecessary  
**Format**: `S:<x>,<y>,<bitmask>[,<voltage>]`

#### Fields

| Field | Type | Range | Description |
|-------|------|-------|-------------|
| `S:` | Literal | - | Message type identifier (State) |
| `<x>` | int8_t | -128 to +127 | Joystick X-axis (negative = left, positive = right, 0 = center) |
| `<y>` | int8_t | -128 to +127 | Joystick Y-axis (negative = back, positive = forward, 0 = center) |
| `<bitmask>` | uint8_t hex | 00 to FF | Button state bitmask (see section 3.2) |
| `<voltage>` | uint16_t | 0 to 9999 | *Optional* Battery voltage in millivolts (e.g., 3750 = 3.75V) |

#### Button Bitmask (8 bits)

```
Bit 7: Reserved (must be 0)
Bit 6: Recessed button (mode change, not for quick input)
Bit 5: Push button 4
Bit 4: Push button 3
Bit 3: Push button 2
Bit 2: Push button 1
Bit 1: Trigger button
Bit 0: Joystick press button
```

**Examples**:
- `0x00` = No buttons pressed
- `0x01` = Joystick button pressed
- `0x02` = Trigger pressed
- `0x03` = Joystick + trigger (combo)
- `0x06` = Trigger + button 1
- `0x4F` = Multiple buttons (trigger + buttons 1, 2, 3, recessed)

#### Battery Voltage Reporting

- **Included**: Every 5 seconds only (reduces message size most of the time)
- **Omitted**: If hardware doesn't support battery reading
- **Brain behavior**: Logs voltage when received, optional low-battery warning

#### Example Messages

```
S:0,0,00                    // Centered joystick, no buttons
S:64,-64,02                 // X=64 (right), Y=-64 (back), trigger pressed
S:0,127,00,3750             // Full forward, battery at 3.75V (periodic report)
S:-128,-45,4F               // Full left, multiple buttons pressed
S:0,0,00,3200               // Centered, no buttons, battery low (3.2V)
```

#### Message Size

- **Typical**: 10-15 bytes (`S:64,-64,02`)
- **With battery**: 15-20 bytes (`S:0,127,00,3750`)
- **ESP-NOW payload limit**: 250 bytes (5-12x headroom for future fields)

---

## 4. Adaptive Transmission Strategy

### 4.1 Ring Firmware Behavior

**Active Mode** (joystick moved or buttons pressed):
- **Rate**: 50 Hz (20ms interval)
- **Purpose**: Responsive, low-latency control
- **Entry**: Any joystick movement outside deadzone OR any button press
- **Exit**: 1 second of no activity (joystick centered, no buttons)

**Idle Mode** (no activity):
- **Rate**: 1 Hz (1000ms interval) - heartbeat only
- **Purpose**: Battery conservation
- **Entry**: 1 second continuous idle (50 consecutive "no activity" checks)
- **Exit**: Immediate on any activity detected

**Idle-to-Active Transition**:
- Send state update **immediately** when activity detected (zero latency)
- Resume 50 Hz rate

### 4.2 Deadzone Threshold

**Ring firmware** checks joystick against deadzone before determining activity:
```cpp
const int8_t DEADZONE = 5;  // ±5 out of ±128 range (~4%)
bool hasActivity = (abs(x) > DEADZONE || abs(y) > DEADZONE || buttons != 0);
```

This prevents noise/drift from keeping ring in active mode.

### 4.3 Battery Impact

| Mode | TX Rate | WiFi Power | Estimated Current |
|------|---------|------------|-------------------|
| **Active** | 50 Hz | Continuous | ~20-30mA |
| **Idle** | 1 Hz | Sleep between TX | ~1-2mA |
| **Typical use** (20% active) | Mixed | Adaptive | ~6-8mA |

**Battery life improvement**: ~75% reduction vs. continuous 50 Hz (2-4x runtime)

---

## 5. Connection Loss Detection

### 5.1 Brain-Side Timeout Strategy

MechRingController leverages the existing `Controller::setCritical()` mechanism (proven pattern from DualSonyNavController):

**Active Timeout** (stick control engaged):
- **Timeout**: 200ms
- **Trigger**: DomeMgr/DriveMgr call `controller->setCritical(true)` when joystick moved
- **Action**: Emergency stop, disable stick control, log error

**Inactive Timeout** (droid parked):
- **Timeout**: 10 seconds
- **Trigger**: `controller->setCritical(false)` when joystick centered
- **Action**: Log connection loss warning

### 5.2 Fault Detection Timeline

**Scenario 1: Ring loses power/WiFi while driving**
```
T+0ms:    Ring loses connection
T+0-20ms: Brain receives last message
T+200ms:  Brain detects timeout (no message in 200ms)
T+200ms:  Emergency stop triggered
T+210ms:  Motors commanded to safe state
```

**Scenario 2: Ring idle (droid parked)**
```
T+0ms:     Ring sending 1 Hz heartbeat
T+0-1000ms: Brain receives last heartbeat
T+10000ms:  Brain detects timeout (inactive mode)
T+10000ms:  Log warning, mark ring disconnected
```

### 5.3 Recovery Behavior

When ring reconnects:
- Brain logs reconnection
- User must **manually re-enable** stick control for safety (button press or command)
- **Alternative** (configurable): Auto-resume after 2 seconds stable connection

### 5.4 MechNet Link Monitoring

MechNet provides redundant safety layer:
- **Automatic heartbeat**: Sent after 1s idle (when ring in 1 Hz mode)
- **Link timeout**: 3 seconds (2 missed heartbeats)
- **Brain notification**: `MechNetMaster::connectedNodeCount()` reflects disconnect

Ring controller primarily uses application-level timeout (200ms/10s), MechNet timeout is backup.

---

## 6. Joystick Value Mapping

### 6.1 Ring Firmware → Message

**Input**: ADC reading from analog joystick
- 12-bit ADC: 0-4095 raw value
- ESP32 built-in ADC: 0-4095 (if accurate enough)

**Mapping to -128/+127**:
```cpp
int8_t mapJoystickX() {
    uint16_t raw = analogRead(JOYSTICK_X_PIN);  // 0-4095
    int16_t centered = raw - 2048;               // -2048 to +2047
    return (int8_t)(centered / 16);              // -128 to +127
}
```

**Calibration** (optional, stored in ring EEPROM):
- Center offset adjustment
- Min/max range scaling
- Per-axis calibration

### 6.2 Brain → Motor Commands

**MechRingController** converts -128/+127 → -100/+100 for motor commands:

```cpp
int8_t getJoystickPosition(Joystick stick, Axis axis) {
    int8_t rawValue = (stick == RIGHT) ? driveRing.joystick_x : domeRing.joystick_x;
    
    // Apply deadzone
    if (abs(rawValue) < deadbandThreshold) {
        return 0;
    }
    
    // Convert -128/+127 → -100/+100
    return (int8_t)((rawValue * 100) / 127);
}
```

**Deadband** (configurable):
- Default: ±16 (in -100/+100 scale)
- Applied in Brain, not ring firmware
- Prevents motor jitter from joystick noise

---

## 7. Configuration (Provisioning)

### 7.1 Ring Provisioning Parameters

**Configured via USB console** (not MechNet):

| Parameter | Example | Description |
|-----------|---------|-------------|
| Network Name | `R2D2Net` | Must match Brain network name |
| Node Base Name | `DriveRing` | Role identifier (DriveRing or DomeRing) |
| WiFi Channel | `6` | Must match Brain channel (1-13) |
| PSK | `[64-char hex]` | Pre-shared key (same across all nodes) |
| Update Rate | `50` | Active mode Hz (20-100 range) |
| Idle Threshold | `1000` | Ms before entering idle mode |
| Deadzone | `5` | Joystick deadzone in ±128 scale |

**Stored in**: Ring EEPROM/NVS (persists across reboots)

### 7.2 Brain Configuration

**MechRingController config keys** (NVS storage):

| Key | Default | Description |
|-----|---------|-------------|
| `activeTimeout` | 200 | Emergency stop timeout (ms) when stick active |
| `inactiveTimeout` | 10000 | Connection warning timeout (ms) when idle |
| `deadbandX` | 16 | Joystick X deadband in ±100 scale |
| `deadbandY` | 16 | Joystick Y deadband in ±100 scale |

**Trigger map**: `settings/MechRingTrigger.map` (button combos → actions)

---

## 8. Future Extensions (Out of Scope for v1.0)

### 8.1 Brain → Ring Commands (Potential)

**LED Control**:
```
LED:<R>,<B>           // Example: LED:1,0 (red on, blue off)
```

**Configuration Updates**:
```
CFG|RATE:<hz>         // Change update rate
CFG|DEADZONE:<val>    // Adjust deadzone
```

**Haptic Feedback** (if hardware added):
```
VIB:<duration_ms>     // Example: VIB:200 (vibrate 200ms)
```

### 8.2 Ring → Brain Status Messages (Potential)

```
STATUS:BATT_LOW       // Battery warning
STATUS:CAL_ERROR      // Joystick calibration issue
```

### 8.3 Multi-Mode Support (Potential)

Rings could support multiple control modes (drive, panel, manipulator) selected via recessed button, with mode indicator via LED.

---

## 9. Implementation Notes

### 9.1 Message Parsing (Brain Side)

**Simple and fast** using `sscanf()`:

```cpp
void MechRingController::parseRingMessage(const String& msg, RingState& ring) {
    if (!msg.startsWith("S:")) return;
    
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
        }
    }
}
```

**Error handling**: Invalid messages ignored (logged at DEBUG level)

### 9.2 Message Construction (Ring Side)

```cpp
void sendStateUpdate(int8_t x, int8_t y, uint8_t buttons) {
    static unsigned long lastBatteryReport = 0;
    char msg[64];
    int len = snprintf(msg, sizeof(msg), "S:%d,%d,%02X", x, y, buttons);
    
    // Add battery voltage every 5 seconds
    if (millis() - lastBatteryReport >= 5000) {
        uint16_t voltage = readBatteryVoltage();  // If available
        if (voltage > 0) {
            snprintf(msg + len, sizeof(msg) - len, ",%u", voltage);
            lastBatteryReport = millis();
        }
    }
    
    mechNetRemote.sendTo(msg, false);  // Unreliable send
}
```

### 9.3 Performance Considerations

**Ring firmware**:
- ADC read: ~10µs (ESP32)
- Message construction: ~50µs (sprintf)
- MechNet send: ~2ms (WiFi TX)
- **Total loop time**: <3ms (plenty of headroom for 20ms interval)

**Brain processing**:
- Message parse: ~20µs (sscanf)
- State update: ~5µs
- **Impact on Brain loop**: Negligible (<0.1ms per ring)

---

## 10. Testing & Validation

### 10.1 Unit Tests (Ring Firmware)

- Joystick mapping accuracy (±1 LSB)
- Deadzone behavior (no activity in ±5 range)
- Idle detection (1s threshold)
- Rate transitions (active ↔ idle)
- Message formatting (verify `sscanf` can parse)

### 10.2 Integration Tests (Brain + Ring)

- Connection establishment (MechNet announce)
- Message reception at 50 Hz (verify <25ms latency)
- Timeout detection (200ms emergency stop)
- Reconnection handling
- Battery reporting (periodic reception)

### 10.3 Hardware Validation

- Range testing (30+ feet typical for ESP-NOW)
- Interference testing (2.4 GHz congestion)
- Battery life measurement (target: 8+ hours typical use)
- Failsafe behavior (ring power loss during motion)

---

## 11. Security Considerations

**Transport Security** (handled by MechNet):
- ✅ HMAC-SHA256 challenge-response authentication
- ✅ AES-128-CCM hardware encryption (ESP-NOW)
- ✅ Pre-shared key (PSK) provisioning
- ✅ Unique session keys per connection

**Application Security**:
- No command authentication needed (ring only reports state)
- Battery/status data is not sensitive
- Future commands (LED, config) don't require additional auth

---

## 12. References

- **MechNet Library**: [MechNet README](../../MechNet/README.md)
- **MechNet Security**: [Security Architecture Design](../../MechNet/docs/design/Security-Architecture-Design.md)
- **MechMind Controller Pattern**: [Controller.h](../include/droid/controller/Controller.h)
- **Sony Nav Reference**: [DualSonyNavController.cpp](../src/droid/controller/DualSonyNavController.cpp)

---

## 13. Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-01-31 | Kizmit99 | Initial specification |

---

**Status**: Ready for Phase 4 implementation (MechRingController + MechRing firmware)
