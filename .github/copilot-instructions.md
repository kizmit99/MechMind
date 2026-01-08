# MechMind AI Coding Guidelines

## Architecture Overview

MechMind is an ESP32-based central processing unit for Astromech-style droids. It follows a **plugin architecture** with a hierarchical component system:

- **System** ([include/droid/core/System.h](../include/droid/core/System.h)): Central dependency container providing Config, Logger, DroidState, and PWMService
- **Brain** ([include/droid/brain/Brain.h](../include/droid/brain/Brain.h)): Main orchestrator that manages all subsystems (DomeMgr, DriveMgr, ActionMgr, AudioMgr, Controllers)
- **BaseComponent** ([include/droid/core/BaseComponent.h](../include/droid/core/BaseComponent.h)): Abstract base class all major components inherit from, defining lifecycle methods: `init()`, `factoryReset()`, `task()`, `logConfig()`, `failsafe()`

### Data Flow: Input → Processing → Output

1. **Input**: Controller hardware (Sony NavController, PS3, DualRing BLE ring, etc.) → `Controller.getAction()` and joystick positions
2. **Processing**: `ActionMgr` maps controller input to actions, queues commands via `CmdHandler` implementations
3. **Output**: Commands routed to motors (Sabertooth, PWM, Cytron), audio drivers (DFMini, HCR, SparkFun), or external boards (Marcduino, AstroPixel)

All components are registered in Brain's `componentList` and called sequentially in `brain->task()` (loop).

## Key Patterns & Conventions

### Plugin System
- **Controllers** ([include/droid/controller/](../include/droid/controller/)): Implement `Controller` interface; multiple implementations exist (`DualSonyNavController`, `DualRingController`, `PS3BtController`, `PS3UsbController`, `StubController`)
- **Command Handlers** ([include/droid/command/CmdHandler.h](../include/droid/command/CmdHandler.h)): Implement `CmdHandler.process(device, command)` for subsystems (Marcduino, AstroPixel, stream handlers)
- **Motor Drivers** ([include/droid/motor/](../include/droid/motor/)): Implement `MotorDriver` interface; support both `arcadeDrive()` (joystick) and `setMotorSpeed()` (direct control)
- **Audio Drivers** ([include/droid/audio/AudioDriver.h](../include/droid/audio/AudioDriver.h)): Pluggable implementations for different sound boards
- **PWM Services** ([include/droid/services/PWMService.h](../include/droid/services/PWMService.h)): Abstract interface for PWM hardware (PCA9685, stub implementations)

### Configuration
- Hardware configuration is **compile-time** via [settings/hardware.config.h](../settings/hardware.config.h): Stream definitions, pin mappings, component type defaults
- Runtime configuration is **runtime-mutable** via `Config` class, persisted to ESP32 NVS partition named "config"
- Components are instantiated in Brain based on config-selected types (e.g., `CONFIG_DEFAULT_CONTROLLER`)

### Logging
- Use `logger->log(LOGNAME, LOGLEVEL, format, ...)` pattern; LogLevel enum defined in [shared/common/Logger.h](../shared/common/Logger.h)
- LOGNAME should be a string literal representing the component (e.g., "DriveMgr")
- Levels: DEBUG, INFO, WARNING, ERROR

### Command Execution
- `ActionMgr` maintains a `cmdMap` (action → instruction string) and `InstructionList` queue
- Commands are parsed as "device:command" pairs and routed to registered `CmdHandlers` (e.g., "marcduino:dome_wave" → Marcduino handler)
- Timing is managed via `queueCommand()` with optional delay; commands execute in `ActionMgr.executeCommands()`

## Build & Deployment

**Build System**: PlatformIO (platformio.ini)  
**Target**: ESP32 dev board (esp32dev) on Arduino framework  
**Libraries**: USB-Host-Shield, NimBLE-Arduino (Bluetooth), Adafruit PWM Servo Driver, Reeltwo, EspSoftwareSerial  
**Flash Size**: 4MB with custom partitions ([settings/partitions.csv](../settings/partitions.csv))

**Build Command**: `platformio run` (use VS Code PlatformIO extension)  
**Upload**: `platformio run --target upload`  
**Monitor**: `platformio device monitor` (115200 baud)

## Important Notes

- **No dynamic memory allocation for component names**: Names must be string literals or persistent pointers (see [include/droid/core/BaseComponent.h](../include/droid/core/BaseComponent.h) constructor comment)
- **Joystick ranges**: Controller axis values are **-100 to +100** (left/reverse = negative, right/forward = positive)
- **Motor speed ranges**: Motor drivers expect **-100 to +100** speed values
- **Stream management**: Each subsystem has its own serial stream defined in hardware.config.h to avoid conflicts
- **Non-commercial license**: CC BY-NC-SA 4.0; verify licensing for any contributions

## File Organization

- [include/droid/](../include/droid/): Component interfaces and headers
- [src/droid/](../src/droid/): Implementations grouped by subsystem
- [include/shared/](../include/shared/): Shared utilities (Logger, Config, BLE Ring helpers)
- [settings/](../settings/): Hardware pins, serial streams, partition table
