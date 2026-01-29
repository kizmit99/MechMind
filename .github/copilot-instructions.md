# MechMind AI Coding Guidelines

## Architecture Overview

MechMind is an ESP32-based central processing unit for Astromech-style droids. It follows a **plugin architecture** with a hierarchical component system:

- **System** ([include/droid/core/System.h](../include/droid/core/System.h)): Central dependency container providing Config, Logger, DroidState, and PWMService
- **Brain** ([include/droid/brain/Brain.h](../include/droid/brain/Brain.h)): Main orchestrator that manages all subsystems (DomeMgr, DriveMgr, ActionMgr, AudioMgr, Controllers)
- **BaseComponent** ([include/droid/core/BaseComponent.h](../include/droid/core/BaseComponent.h)): Abstract base class all major components inherit from, defining lifecycle methods: `init()`, `factoryReset()`, `task()`, `logConfig()`, `failsafe()`

### Component Lifecycle

All components follow this exact initialization sequence (see [src/main.cpp](../src/main.cpp) and [src/droid/brain/Brain.cpp](../src/droid/brain/Brain.cpp)):

1. **Construction**: Brain instantiates components based on runtime config (e.g., controller type from `CONFIG_DEFAULT_CONTROLLER`)
2. **Registration**: Components added to `Brain::componentList` vector
3. **Initialization**: `brain->init()` calls `component->init()` for each component, loading config values from NVS
4. **Execution Loop**: `brain->task()` iterates through `componentList`, calling `component->task()` every loop cycle
5. **Factory Reset**: `component->factoryReset()` writes default values back to NVS partition

Components **must not** perform heavy initialization in constructors—save it for `init()`.

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

### Configuration Management
- **Compile-time config**: [settings/hardware.config.h](../settings/hardware.config.h) defines streams (`DOME_STREAM`, `AUDIO_STREAM`, etc.), pin mappings, component type defaults (`CONFIG_DEFAULT_CONTROLLER`)
- **Runtime config**: Components store settings in ESP32 NVS partition named "config" via `Config` class methods (`getInt()`, `putString()`, etc.)
- **Config pattern**: Each component defines `CONFIG_KEY_*` and `CONFIG_DEFAULT_*` macros in its `.cpp` file (e.g., `CONFIG_KEY_DRIVEMGR_NORMALSPEED`)
- **Stream setup macros**: Initialize serial streams via macros like `DOME_STREAM_SETUP` (expands to `Serial3.begin(2400, SWSERIAL_8N1, 32, 4)`)
- **Logger levels**: Configure per-component log levels via [settings/LoggerLevels.config.h](../settings/LoggerLevels.config.h) (included in `Brain::init()` with `#define LOGGER logger`)

### Action Mapping & Command Execution
- **Action.map**: [settings/Action.map](../settings/Action.map) maps button/trigger names to command sequences (`cmdMap["StickEnable"] = "Brain>StickEnable"`)
- **Command syntax**: `device>command` (e.g., `Dome>:OP00`), semicolon-separated for sequences, `#` prefix for delays in milliseconds (`#500`)
- **Execution flow**: `ActionMgr::fireAction()` → `parseCommands()` → `queueCommand()` → `InstructionList` → `executeCommands()` routes to registered `CmdHandler.process(device, cmd)`
- **Override commands**: Runtime overrides stored in config via `ActionMgr::overrideCmdMap(action, cmd)`, fall back to Action.map defaults

### Logging
- Use `logger->log(LOGNAME, LOGLEVEL, format, ...)` pattern; LogLevel enum defined in [shared/common/Logger.h](../shared/common/Logger.h)
- LOGNAME should be a string literal representing the component (e.g., "DriveMgr")
- Levels: DEBUG, INFO, WARN, ERROR, FATAL
- Per-component levels configured in [settings/LoggerLevels.config.h](../settings/LoggerLevels.config.h)

## Build & Deployment

**Build System**: PlatformIO (platformio.ini)  
**Target**: ESP32 dev board (esp32dev) on Arduino framework  
**Libraries**: USB-Host-Shield, NimBLE-Arduino (Bluetooth), Adafruit PWM Servo Driver, Reeltwo, EspSoftwareSerial, SmartCLI, MechNet  
**Flash Size**: 4MB with custom partitions ([settings/partitions.csv](../settings/partitions.csv))

**Key Commands**:
- Build: `platformio run` (or use VS Code PlatformIO extension)
- Upload: `platformio run --target upload`
- Monitor: `platformio device monitor` (115200 baud, includes ESP32 exception decoder)
- Clean: `platformio run --target clean`

**Debug**: Configured for `esp-prog` with break on `setup()`, 500kHz speed

## Component Implementation Checklist

When adding a new component inheriting from `BaseComponent`:

1. **Constructor**: Accept `name` and `system`, pass to `BaseComponent(name, system)`. Do NOT perform heavy initialization here.
2. **init()**: Load config values via `config->getInt(name, CONFIG_KEY_*, CONFIG_DEFAULT_*)` pattern. Define config keys as macros in `.cpp` file.
3. **factoryReset()**: Write defaults back to config with `config->putInt(name, CONFIG_KEY_*, CONFIG_DEFAULT_*)`. Call `config->clear(name)` first.
4. **task()**: Called every loop iteration; perform real-time work here (read sensors, update motors, etc.)
5. **logConfig()**: Log current config values via `logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_*, config->getString(...))`
6. **failsafe()**: Stop/disable component safely (e.g., stop motors, turn off outputs)
7. **Registration**: Add to `Brain::componentList` in Brain constructor for automatic lifecycle management

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
