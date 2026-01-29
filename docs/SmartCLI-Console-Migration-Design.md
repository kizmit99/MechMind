# MechMind SmartCLI Console Migration Design

**Version:** 1.0  
**Date:** January 28, 2026  
**Status:** Design Ready

---

## 1. Overview

### 1.1 Purpose

Migrate MechMind's console implementation from custom character-by-character processing to the SmartCLI library, improving the administrative interface while preserving programmatic command execution capabilities.

### 1.2 Goals

- ✅ Replace custom console input processing with SmartCLI::Console
- ✅ Maintain **100% functional equivalence** for existing commands
- ✅ Preserve LocalCmdHandler for programmatic/ActionMgr command routing
- ✅ Add ConsoleHandler for direct admin console interaction
- ✅ Enable async message display (background events don't interrupt typing)
- ✅ Support wizard-based workflows for future provisioning tasks
- ✅ Keep existing Logger infrastructure unchanged

### 1.3 Non-Goals (Future Work)

- ❌ MechNet provisioning commands (separate phase)
- ❌ Command history / tab completion (SmartCLI doesn't provide this)
- ❌ Refactoring Brain component structure
- ❌ Changing existing command syntax or behavior

---

## 2. Current Architecture Analysis

### 2.1 Current Console Flow

**Input Processing** ([Brain.cpp](../src/droid/brain/Brain.cpp)):
```cpp
void Brain::task() {
    if (CONSOLE_STREAM != NULL) {
        processConsoleInput(CONSOLE_STREAM);
    }
    // ... other tasks
}

void Brain::processConsoleInput(Stream* cmdStream) {
    while (cmdStream->available()) {
        char in = cmdStream->read();
        
        // Manual backspace handling
        if (in == '\b') {
            cmdStream->print("\b \b");
            if (bufIndex > 0) bufIndex--;
        } else {
            cmdStream->print(in);  // Echo
            inputBuf[bufIndex++] = in;
        }
        
        // Line termination
        if ((in == '\r') || (in == '\n')) {
            if (bufIndex > 1) {
                inputBuf[bufIndex - 1] = 0;
                actionMgr->queueCommand("Brain", inputBuf, millis());
            }
            bufIndex = 0;
        }
    }
}
```

**Command Routing**:
1. User types command → buffered in `inputBuf[]`
2. On newline → queued to ActionMgr as `"Brain>command"`
3. ActionMgr routes to LocalCmdHandler (device == "Brain")
4. LocalCmdHandler parses and executes

**Limitations**:
- No async message support (background events would corrupt input line)
- Manual echo/backspace handling
- Buffer stored in Brain class (32 commands share state)
- No multi-step wizard support

### 2.2 LocalCmdHandler Commands

**Current Commands** ([LocalCmdHandler.cpp](../src/droid/brain/LocalCmdHandler.cpp)):
- `Help` / `?` - Command list
- `StickEnable/Disable/Toggle` - Drive joystick control
- `SpeedChange` - Toggle turbo speed
- `DomeAutoOn/Off/Toggle` - Auto dome control
- `DomePAllToggle`, `HoloAutoToggle`, `HoloLightsTogl`, `BodyPAllToggle`, `MusingsToggle` - State toggles
- `Gesture` - Trigger gesture action (TODO)
- `Restart` - System reboot
- `FactoryReset` - Clear config and restart
- `ListConfig` - Print all config
- `SetConfig <namespace> <key> <value>` - Modify config
- `SetAction <action> <cmdList>` - Override action mapping
- `Play <command>` - Execute action/command (admin testing)
- `ResetAction <action>` - Restore default mapping
- `TestPanel <panel> <value>` - Servo test
- `LogLevel <component> <level>` - Logger control

**Command Characteristics**:
- Space-delimited parsing (`cmd parm1 parm2`)
- Direct Brain method calls OR `brain->fireAction()` forwarding
- Case-insensitive command matching
- Help text printed to `console` Stream

### 2.3 Command Categorization for Migration

**Critical decision**: Which commands belong in the interactive console vs. programmatic API?

**Category A: Admin/Provisioning Commands (→ ConsoleHandler)**

These are **interactive admin tools** used for system configuration, diagnostics, and testing:

| Command | Purpose | Rationale |
|---------|---------|-----------|
| `Help` / `?` | Show console help | Interactive only |
| `Restart` | System reboot | Admin operation |
| `FactoryReset` | Clear config and reboot | Dangerous admin operation |
| `ListConfig` | Print all config | Diagnostic/debugging |
| `SetConfig <ns> <key> <val>` | Direct config manipulation | Admin configuration |
| `LogLevel <comp> <lvl>` | Runtime logger control | Debugging tool |
| `SetAction <action> <cmd>` | Override action mapping | Testing/customization |
| `ResetAction <action>` | Restore default mapping | Testing/customization |
| `Play <action>` | Execute action/command | **Admin testing tool** (triggers ActionMgr) |
| `TestPanel <panel> <val>` | Servo position test | Hardware testing/calibration |

**Key insight**: None of these are referenced in Action.map or called from other components. They're purely interactive admin commands.

**Category B: Application Runtime Commands (→ LocalCmdHandler via ActionMgr)**

These modify **runtime state** and are triggered from Action.map or controller events:

| Command | Purpose | Triggered By |
|---------|---------|--------------|
| `StickEnable/Disable/Toggle` | Drive control state | Action.map (controller triggers) |
| `SpeedChange` | Toggle turbo speed | Action.map |
| `DomeAutoOn/Off/Toggle` | Auto dome behavior | Action.map |
| `DomePAllToggle` | Dome panels + state | Action.map |
| `HoloAutoToggle` | Holo state + action | Action.map |
| `HoloLightsTogl` | Holo lights + action | Action.map |
| `BodyPAllToggle` | Body panels + state | Action.map |
| `MusingsToggle` | Audio musing + action | Action.map |
**Implements Category A commands only** (admin/provisioning):

```cpp
// include/droid/brain/ConsoleHandler.h
namespace droid::brain {
    class ConsoleHandler {
    public:
        ConsoleHandler(Stream& stream, Brain* brain);
        
        // Lifecycle
        void begin();    // Initialize Console, show banner
        void process();  // Call from Brain::task()
        
        // Static logging (for background events)
        static void debug(const char* fmt, ...);
        static void log(const char* fmt, ...);
        
    private:
        static ConsoleHandler* s_instance;
        
        SmartCLI::Console _console;
        Brain* _brain;
        
        // Console callbacks
        void handleCommand(const char* line);
        void showBanner();
        void showHelp();
        
        // Admin command handlers (10 commands)
        void handleRestart();
        void handleFactoryReset();
        void handleListConfig();
        void handleSetConfig(const char* args);
        void handleSetAction(const char* args);
        void handleResetAction(const char* args);
        void handlePlay(const char* args);  // Admin testing (routes to ActionMgr)
        void handleTestPanel(const char* args);
        void handleLogLevel(const char* args);
    };
}
```

**Key Features**:
- Owns `SmartCLI::Console` instance
- Direct Brain method calls (no ActionMgr routing except `Play`)
- **`Play` command**: Admin tool to test actions via ActionMgr (e.g., `Play StickEnable`her components (ActionMgr-routed)
- **Brain**: Business logic, unaware of input source

### 3.2 ConsoleHandler Design

**Pattern**: Follow MechNetCentral `ConsoleHandler` example

```cpp
// include/droid/brain/ConsoleHandler.h
namespace droid::brain {
    class ConsoleHandler {
    public:
        ConsoleHandler(Stream& stream, Brain* brain);
        
        // Lifecycle
        void begin();    // Initialize Console, show banner
        void process();  // Call from Brain::task()
        
        // Static logging (for background events)
        static void debug(const char* fmt, ...);
        static void log(const char* fmt, ...);
        
    private:
        static ConsoleHandler* s_instance;
        
        SmartCLI::Console _console;
        Brain* _brain;
        
        // Console callbacks
        void handleCommand(const char* line);
        void showBanner();
        void showHelp();
        
        // Command handlers (same logic as LocalCmdHandler)
        void handleStickEnable();
        void handleRestart();
        void handleFactoryReset();
        void handleListConfig();
        void handleSetConfig(const char* args);
        void handlePlay(const char* args);
        // ... etc
    };
}
```

**Key Features**:
- Owns `SmartCLI::Console` instance
- Direct Brain method calls (no ActionMgr routing)
- Maintains command compatibility (same syntax)
- Static logging for background events
- Simplified to Category B commands only** (application runtime):

```cpp
bool LocalCmdHandler::process(const char* device, const char* command) {
    // Only handles 9 runtime state commands:
    // - StickEnable/Disable/Toggle
    // - SpeedChange
    // - DomeAutoOn/Off/Toggle
    // - DomePAllToggle
    // - HoloAutoToggle
    // - HoloLightsTogl
    // - BodyPAllToggle
    // - MusingsToggle
    // - Gesture
    
    // All admin commands removed (migrated to ConsoleHandler)
    // Help text output via logger (for programmatic callers)
}
```

**Changes**:
- **Removed commands**: Help, Restart, FactoryReset, ListConfig, SetConfig, SetAction, ResetAction, Play, TestPanel, LogLevel
- **Kept commands**: Runtime state toggles triggered from Action.map
- Help text via `logger` instead of `console` Stream
- Remains for programmatic/component-initiated commands via ActionMgr
- No longer has interactive console responsibilitie
**Changes**:
- Help text output via `logger` instead of `console` Stream
- No longer primary user interface (ConsoleHandler takes that role)
- Remains for programmatic/component-initiated commands

---

## 4. Migration Strategy

### 4.1 File Structure

**New Files**:
```
include/droid/brain/
    ConsoleHandler.h              # SmartCLI console interface
    ConsoleStream.h               # Stream adapter for Logger

src/droid/brain/
    ConsoleHandler.cpp            # Implementation
    ConsoleStream.cpp             # Stream adapter implementation
```

**Modified Files**:
```main.cpp                      # Use ConsoleStream for System
src/droid/brain/Brain.h           # Add ConsoleHandler member
src/droid/brain/Brain.cpp         # Replace processConsoleInput
src/droid/brain/LocalCmdHandler.cpp  # Update help output
settings/hardware.config.h        # Remove LOGGER_STREAM definitionsmber
src/droid/brain/Brain.cpp         # Replace processConsoleInput
src/droid/brain/LocalCmdHandler.cpp  # Update help output
```API unchanged (still uses Stream)
src/shared/common/Logger.cpp      # Logger implementation unchanged

**Unchanged Files**:
```
include/shared/common/Logger.h    # Logger stays as-is
include/droid/command/ActionMgr.h # ActionMgr routing unchanged
```

### 4.2 Implementation Phases

#### Phase 1: Add SmartCLI Dependency

**platformio.ini**:
```ini
lib_deps =Stream Adapter

**include/droid/brain/ConsoleStream.h** (see section 6.1 for full code)

**src/droid/brain/ConsoleStream.cpp** (see section 6.1 for full code)

**Purpose**: Routes Logger output through SmartCLI::Console async queue

#### Phase 3: Create Console
    https://github.com/kizmit99/SmartCLI.git#v0.1.1
    # ... existing deps
```

#### Phase 2: Create ConsoleHandler

**include/droid/brain/ConsoleHandler.h**:
```cpp
#pragma once
#include <Arduino.h>
#include <smartcli/SmartCLI.h>

namespace droid::brain {
    class Brain;  // Forward declaration
    
    class ConsoleHandler {
    public:
        ConsoleHandler(Stream& stream, Brain* brain);
        
        void begin();
        void process();
        
        static void debug(const char* fmt, ...);
        static void log(const char* fmt, ...);
        
    private:
        static ConsoleHandler* s_instance;
        
        SmartCLI::Console _console;
        Brain* _brain;
        
        void handleCommand(const char* line);
        void showBanner();
        void showHelp();
        
        // Command handlers (one per existing LocalCmdHandler command)
        void handleStickCommand(const char* args);
        void handleDomeAutoCommand(const char* args);
        void handleRestart();
        void handleFactoryReset();
        void handleListConfig();
        void handleSetConfig(const char* args);
        void handleSetAction(const char* args);
        void handlePlay(const char* args);
        voi4: Update main.cpp

**main.cpp changes**:
```cpp
#include "droid/brain/ConsoleStream.h"

// Global ConsoleStream instance (replaces LOGGER_STREAM)
droid::brain::ConsoleStream consoleStream;

void setup() {
    CONSOLE_STREAM_SETUP;   // Serial.begin(115200)
    // LOGGER_STREAM_SETUP removed
    DOME_STREAM_SETUP;
    // ... other streams
    
    delay(500);

    bufferedStream = new BufferedStream(LOGGER_STREAM, 10240);  // REMOVE THIS
    sys = new droid::core::System(&consoleStream, DEBUG);  // Use adapter
    brain = new droid::brain::Brain("R2D2", sys);

    brain->init();
    brain->logConfig();
}

void loop() {
    brain->task();
    // bufferedStream->task();  // REMOVE THIS (no longer needed)
}
```

**Note**: BufferedStream removed - SmartCLI handles async queuing

#### Phase 5 handleResetAction(const char* args);
        void handleTestPanel(const char* args);
        void handleLogLevel(const char* args);
    };
}
```

**src/droid/brain/ConsoleHandler.cpp**:
- Implement command parsing (split command/args)
- Dispatch to handler methods
- Call Brain methods directly (no ActionMgr)
- Output via `_console.println()` / `_console.print()`

#### Phase 3: Integrate into Brain

**Brain.h changes**:
```cpp
class Brain : droid::core::BaseComponent {
public:
    // Expose methods for ConsoleHandler to call directly
    void reboot();
    void factoryReset();
    void fireAction(const char* action);
    void overrideCmdMap(const char* action, const char* cmd);
    void logConfig();
    
private:
    ConsoleHandler* consoleHandler;
    
    // Remo6e: void processConsoleInput(Stream*);
    // Remove: char inputBuf[100], uint8_t bufIndex;
};
```

**Brain.cpp constructor**:
```cpp
Brain::Brain(const char* name, droid::core::System* system) : 
    BaseComponent(name, system) {
    
    // ... existing component construction
    
    // Create ConsoleHandler (if CONSOLE_STREAM defined)
    #ifdef CONSOLE_STREAM
        consoleHandler = new ConsoleHandler(CONSOLE_STREAM, this);
    #else
        consoleHandler = nullptr;
    #endif
}
```

**Brain.cpp init()**:
```cpp
void Brain::init() {
    // ... existing initialization
    
    if (consoleHandler) {
        consoleHandler->begin();  // Show banner, first prompt
    }
}
```

**Brain.cpp task()**:
```cpp
void Brain::task() {
    unsigned long begin = millis();
    
    // Process console input (new way)
    if (consoleHandler) {
        consoleHandler->process();
    }
    
    // ... existing component tasks
}
```

#### Phase 4: Update LocalCmdHandler

**Minimal changes** - just help output:

```cpp
void LocalCmdHandler::printHelp() {
    // Change from: if (console != NULL) console->print(...)
    // To: logger->log(name, INFO, "Commands:\n");
    
    logger->log(name, INFO, "  Help or ? - Print this list\n");
    logger->log(name, INFO, "  StickEnable - Enable drive joystick\n");
    // ... etc
}

void LocalCmdHandler::printCmdHelp(const char* cmdName, const char* cmdDescription) {
    logger->log(name, INFO, "  %s - %s\n", cmdName, cmdDescription);
}
```

---

## 5. Detailed Implementation Examples

### 5.1 Migrating LocalCmdHandler Code

**Pattern for moving commands to ConsoleHandler**:

**LocalCmdHandler (old)**:
```cpp
void LocalCmdHandler::printHelp() {
    if (console != NULL) {
        console->print("\n");
        console->print("Commands:");
        console->printf("  %s\n", "Help or ?");
        console->printf("    %s\n", "Print this list");
    }
}
```

**ConsoleHandler (new)**:
```cpp
void ConsoleHandler::showHelp() {
    _console.println("");
    _console.println("Commands:");
    _console.println("  %s", "Help or ?");
    _console.println("    %s", "Print this list");
}
```

**Key changes**:
- `console->print()` → `_console.print()`
- `console->printf()` → `_console.println()` (SmartCLI uses println for formatted output)
- `\n` in format strings → removed (println auto-appends newline)
- `if (console != NULL)` checks → removed (ConsoleHandler always has console)

**Note**: ~50+ console->print/printf calls in LocalCmdHandler.cpp need this conversion

### 5.2 ConsoleHandler Command Parsing

**Pattern**: Similar to LocalCmdHandler but cleaner

```cpp
void ConsoleHandler::handleCommand(const char* line) {
    // Empty line - do nothing (Console auto-shows prompt)
    if (strlen(line) == 0) return;
    
    // Parse command and arguments
    char cmdBuf[64];
    char argsBuf[192];
    
    // Split on first space
    const char* spacePtr = strchr(line, ' ');
    if (spacePtr) {
        size_t cmdLen = spacePtr - line;
        strncpy(cmdBuf, line, min(cmdLen, sizeof(cmdBuf) - 1));
        cmdBuf[min(cmdLen, sizeof(cmdBuf) - 1)] = '\0';
        
        // Trim leading spaces from args
        const char* argsStart = spacePtr + 1;
        while ( (Category A: Admin commands only)
    if (strcmp(cmdBuf, "help") == 0 || strcmp(cmdBuf, "?") == 0) {
        showHelp();
    }
    else if (strcmp(cmdBuf, "restart") == 0) {
        handleRestart();
    }
    else if (strcmp(cmdBuf, "factoryreset") == 0) {
        handleFactoryReset();
    }
    else if (strcmp(cmdBuf, "listconfig") == 0) {
        handleListConfig();
    }
    else if (strcmp(cmdBuf, "setconfig") == 0) {
        handleSetConfig(argsBuf);
    }
    else if (strcmp(cmdBuf, "setaction") == 0) {
        handleSetAction(argsBuf);
    }
    else if (strcmp(cmdBuf, "resetaction") == 0) {
        handleResetAction(argsBuf);
    }
    else if (strcmp(cmdBuf, "play") == 0) {
        handlePlay
    else if (strcmp(cmdBuf, "listconfig") == 0) {
        handleListConfig();
    }
    else if (strcmp(cmdBuf, "setconfig") == 0) {
        handleSetConfig(argsBuf);
    }
    else if (strcmp(cmdBuf, "setaction") == 0) {
        handleSetAction(argsBuf);
    }
    else if (strcmp(cmdBuf, "play") == 0) {
        handlePlay(argsBuf);
    }
    else if (strcmp(cmdBuf, "resetaction") == 0) {
        handleResetAction(argsBuf);
    }
    else if (strcmp(cmdBuf, "testpanel") == 0) {
        handleTestPanel(argsBuf);
    }
    else if (strcmp(cmdBuf, "loglevel") == 0) {
        handleLogLevel(argsBuf);
    }
    else {
        _console.println("Unknown command: %s (type 'help' for list)", cmdBuf);
    }
    
    // Console automatically shows prompt after this returns
}                      _brain->getDroidState()->stickEnable ? "enabled" : "disabled");
    }
}
```

**Direct Brain method call**:
```cpp
void ConsoleHandler::handleRestart() {
    _console.println("Restarting system...");
    _brain->reboot();  // Direct call, no ActionMgr
}
```

**Config modification** (multi-parameter parsing):
```cpp
void ConsoleHandler::handleSetConfig(const char* args) {
    char namespace_buf[32];
    char key_buf[32];
    char value_buf[128];
    
    // Parse: "namespace key value"
    if (sscanf(args, "%31s %31s %127[^\n]", namespace_buf, key_buf, value_buf) != 3) {
        _console.println("Usage: setconfig <namespace> <key> <value>");
        return;
    }
    
    // Validate lengths
    if (strlen(namespace_buf) > 15 || strlen(key_buf) > 15) {
        _console.println("Error: namespace/key max 15 characters");
        return;
    }
    
    // Apply config
    _brain->getConfig()->putString(namespace_buf, key_buf, value_buf);
    _console.println("✓ Set %s.%s = %s", namespace_buf, key_buf, value_buf);
}
```

**Forwarding to ActionMgr** (for actions):
```cpp
void ConsoleHandler::handlePlay(const char* args) {
    if (strlen(args) == 0) {
        _console.println("Usage: play <action>");
        return;
    }
    
    _brain->fireAction(args);  // Triggers ActionMgr processing
    _console.println("✓ Executed: %s", args);
}
```

### 5.4 Help Text

```cpp
void ConsoleHandler::showHelp() {
    _console.println("");
  Admin testing via ActionMgr** (Play command):
```cpp
void ConsoleHandler::handlePlay(const char* args) {
    if (strlen(args) == 0) {
        _console.println("Usage: play <action>");
        _console.println("Examples:");
        _console.println("  play StickEnable      - Enable drive stick");
        _console.println("  play Happy            - Trigger happy action");
        _console.println("  play Dome>:OP01       - Send dome command");
        return;
    }
    
    _brain->fireAction(args);  // Routes to ActionMgr → handlers
    _console.println("✓ Executed: %s", args);
}
```

**Purpose**: Admin tool to test any action/command without controller. Can trigger:
- Category B commands (e.g., `play StickEnable`)
- Action.map actions (e.g.,MechMind Admin Console");
    _console.println("═══════════════════════════════════");
    _console.println("");
    _console.println("System Commands:");
    _console.println("  help, ?               - Show this help");
    _console.println("  restart               - Reboot system");
    _console.println("  factoryreset          - Restore defaults and reboot");
    _console.println("");
    _console.println("Configuration:");
    _console.println("  listconfig            - Print all configuration");
    _console.println("  setconfig <ns> <k> <v> - Set config value");
    _console.println("  loglevel <comp> <lvl> - Set log level (0-4)");
    _console.println("");
    _console.println("Action Mapping:");
    _console.println("  setaction <action> <cmd> - Override action mapping");
    _console.println("  resetaction <action>  - Restore default mapping");
    _console.println("");
    _console.println("Testing:");
    _console.println("  play <action>         - Execute action/command");
    _console.println("                          Examples: StickEnable, Happy, Dome>:OP01");
    _console.println("  testpanel <n> <val>   - Test panel servo (1-%d, 500-2500uS)", LOCAL_PANEL_COUNT
    _console.println("═══════════════════════════════════");
    _console.println("          MechMind v1.0");
    _console.println("═══════════════════════════════════");
    _console.println("");
    _console.println("Type 'help' for command list");
    _console.println("");
}

void ConsoleHandler::begin() {
    s_instance = this;
    
    _console.setPrompt("> ");
    
    _console.onBanner([this]() {
        showBanner();
    });
    
    _console.onCommand([this](const char* line) {
        handleCommand(line);
    });
    
    _console.begin();  // Shows banner + prompt
}
```

### 5.6 Static Logging (Background Events)

```cpp
// Static wrappers for global access
void ConsoleHandler::debug(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    SmartCLI::Console::vdebug(fmt, args);
    va_end(args);
}

void ConsoleHandler::log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    SmartCLI::Console::vlog(fmt, args);
    va_end(args);
}

// Usage in Brain or other components:
void Brain::task() {
    // ... processing
    
    if (someBackgroundEvent) {
        ConsoleHandler::log("Controller connected");  // Queued, won't interrupt typing
    }
}
```

---

## 6. Logger Integration

### 6.1 Stream Adapter Pattern

**Goal**: Route Logger output through SmartCLI::Console so all messages use async queue (won't interrupt typing)

**New Component**: ConsoleStream adapter class

```cpp
// include/droid/brain/ConsoleStream.h
#pragma once
#include <Arduino.h>

namespace droid::brain {
    /**
     * Stream adapter that routes Logger output to SmartCLI::Console.
     * 
     * Logger writes to this Stream → forwarded to Console::log() → async queue
     * This prevents Logger output from interrupting user typing.
     */
    class ConsoleStream : public Stream {
    public:
        ConsoleStream();
        
        // Stream interface (write methods)
        size_t write(uint8_t c) override;
        size_t write(const uint8_t* buffer, size_t size) override;
        
        // Stream interface (read methods - not used by Logger)
        int available() override { return 0; }
        int read() override { return -1; }
        int peek() override { return -1; }
        void flush() override {}
        
    private:
        char lineBuffer[256];
        size_t bufferPos;
        
        void flushLine();
    };
}
```

**Implementation**:
```cpp
// src/droid/brain/ConsoleStream.cpp
#include "droid/brain/ConsoleStream.h"
#include <smartcli/SmartCLI.h>

namespace droid::brain {
    ConsoleStream::ConsoleStream() : bufferPos(0) {
        lineBuffer[0] = '\0';
    }
    
    size_t ConsoleStream::write(uint8_t c) {
        if (c == '\n' || c == '\r') {
            flushLine();
            return 1;
        }
        
        if (bufferPos < sizeof(lineBuffer) - 1) {
            lineBuffer[bufferPos++] = c;
            lineBuffer[bufferPos] = '\0';
        }
        
        return 1;
    }
    
    size_t ConsoleStream::write(const uint8_t* buffer, size_t size) {
        for (size_t i = 0; i < size; i++) {
            write(buffer[i]);
        }
        return size;
    }
    
    void ConsoleStream::flushLine() {
        if (bufferPos > 0) {
            // Route to Console async queue (won't interrupt typing)
            SmartCLI::Console::log("%s", lineBuffer);
            bufferPos = 0;
            lineBuffer[0] = '\0';
        }
    }
}
```

### 6.2 System Integration

**main.cpp changes**:
```cpp
#include "droid/brain/Brain.h"
#include "droid/brain/ConsoleStream.h"
#include "droid/core/System.h"

droid::brain::ConsoleStream consoleStream;  // Stream adapter
droid::core::System* sys;
droid::brain::Brain* brain;

void setup() {
    CONSOLE_STREAM_SETUP; 
    // Note: LOGGER_STREAM_SETUP removed - using ConsoleStream instead
    DOME_STREAM_SETUP;
    BODY_STREAM_SETUP;
    // ... etc
    
    delay(500);

    // System now uses ConsoleStream instead of raw Serial
    sys = new droid::core::System(&consoleStream, DEBUG);
    brain = new droid::brain::Brain("R2D2", sys);

    brain->init();
    brain->logConfig();
}
```

**Benefits**:
- All Logger output routed through SmartCLI async queue
- Logger messages won't corrupt user input
- Logger API unchanged (still uses Stream interface)
- Unified output pathway (Console controls all serial output)

### 6.3 Stream Configuration

**hardware.config.h updates**:
```cpp
// Console stream for user interaction
#define CONSOLE_STREAM &Serial
#define CONSOLE_STREAM_SETUP Serial.begin(115200)

// Logger output routed through ConsoleStream adapter
// (no LOGGER_STREAM or LOGGER_STREAM_SETUP needed)

// Other streams unchanged
#define DOME_STREAM &Serial3
#define DOME_STREAM_SETUP Serial3.begin(2400, SWSERIAL_8N1, 32, 4)
// ... etc
```

### 6.4 Output Flow

**Before migration**:
```
Logger → LOGGER_STREAM (Serial) → USB
                                   ↓ (may corrupt user typing)
                                 Console input
```

**After migration**:
```
Logger → ConsoleStream → SmartCLI::Console::log() → Async Queue
                                                          ↓
                                                    (waits for idle)
                                                          ↓
                                                   Serial output
                                                   (no corruption)
```

---

## 7. Testing Strategy

### 7.1 Functional Equivalence Checklist

**For each existing command**:
- [ ] `Help` - Shows command list (new format OK)
- [ ] `StickEnable/Disable/Toggle` - Updates droidState correctly
- [ ] `DomeAutoOn/Off/Toggle` - Updates droidState correctly
- [ ] `Restart` - Reboots system
- [ ] `FactoryReset` - Clears config and reboots
- [ ] `ListConfig` - Prints all config (may route through logger)
- [ ] `SetConfig` - Updates config values
- [ ] `SetAction` - Overrides action mappings
- [ ] `Play` - Executes actions via ActionMgr
- [ ] `ResetAction` - Restores default mappings
- [ ] `TestPanel` - Stream.h
- [ ] Create ConsoleStream.cpp (Stream adapter for Logger)
- [ ] Create ConsoleSends PWM test commands
- [ ] `LogLevel` - Updates logger levels

### 7.2 Console Features Validation

**SmartCLI-specific features**:
- [ ] Backspamain.cpp
  - [ ] Add ConsoleStream global instance
  - [ ] Pass &consoleStream to System constructor
  - [ ] Remove BufferedStream usage
- [ ] Modify Brain.h (add ConsoleHandler member, expose methods)
- [ ] Modify Brain.cpp
  - [ ] Constructor: create ConsoleHandler
  - [ ] init(): call consoleHandler->begin()
  - [ ] task(): call consoleHandler->process()
  - [ ] Remove processConsoleInput() method
  - [ ] Remove inputBuf/bufIndex members
- [ ] Modify LocalCmdHandler.cpp
  - [ ] Update help output to use logger
- [ ] Modify hardware.config.h
  - [ ] Remove LOGGER_STREAM and LOGGER_STREAM_SETUP definitions
**Programmatic command routing still works**:
- [ ] `actionMgr->queueCommand("Brain", "StickEnable")` → LocalCmdHandler executes
- [ ] Other components can trigger Brain commands via ActionMgr
- [ ] LocalCmdHandler help text goes to logger (not console)

### 7.4 Integration Testing

**Scenarios**:
1. **Fresh boot**: Banner shows, prompt appears
2. **Type command**: Execute `stickenable`, verify state change
3. **Background event**: Logger message appears without breaking prompt
4. **Invalid command**: Error message, prompt redisplays
5. **Multi-parameter command**: `setconfig` parses correctly
6. **Reboot command**: System restarts cleanly
7. **Factory reset**: Config cleared, system restarts
8. **Headless mode**: System works without console (CONSOLE_STREAM == NULL)

---

## 8. Migration Checklist

### 8.1 Pre-Migration

- [ ] Review MechNetCentral ConsoleHandler as reference
- [ ] Verify SmartCLI v0.1.1 available
- [ ] Document current command list and expected behavior
- [ ] Create test plan for functional equivalence

### 8.2 Implementation

- [ ] Add SmartCLI dependency to platformio.ini
- [ ] Create ConsoleHandler.h
- [ ] Create ConsoleHandler.cpp
  - [ ] Command parsing
  - [ ] All command handlers
  - [ ] Help text
  - [ ] Banner
- [ ] Modify Brain.h (add ConsoleHandler member, expose methods)
- [ ] Modify Brain.cpp
  - [ ] Constructor: create ConsoleHandler
  - [ ] init(): call consoleHandler->begin()
  - [ ] task(): call consoleHandler->process()
  - [ ] 10 admin command handlers (see section 2.3)
  - [ ] Help text (organized by category)putBuf/bufIndex members
- [ ] Modify LocalCmdHandler.cpp
  - [ ] Update help output to use logger

### 8.3 Testing

- [ ] Compile without errors
- [ ] Upload to device
- [ ] Verify banner displays
- [ ] Test each command from checklist
- [ ] Verify async message display
- [ ] Test LocalCmdHandler still works (via ActionMgr)
- [ ] Test headless mode (if applicable)

### 8.4 Documentation

- [ ] Update README with new console features
- [ ] Document ConsoleHandler API for future commands
- [ ] Note wizard mode availability for provisioning

---

## 9. Future Enhancements (Post-Migration)

### 9.1 MechNet Provisioning (Phase 2)

With SmartCLI wizard support ready:

```cpp
void ConsoleHandler::handleMechNetProvision() {
    _wizardState = WizardState::MECHNET_NETWORK_NAME;
    _console.wizardStart();
    
    _console.println("═══════════════════════════════════");
    _console.println("   MechNet Provisioning Wizard");
    _console.println("═══════════════════════════════════");
    _console.print("Network name: ");
}

void ConsoleHandler::processWizard(const char* input) {
    switch (_wizardState) {
        case WizardState::MECHNET_NETWORK_NAME:
            // Store network name
            _wizardState = WizardState::MECHNET_CHANNEL;
            _console.print("WiFi channel (1-13): ");
            break;
        
        case WizardState::MECHNET_CHANNEL:
            // Validate and store channel
            _wizardState = WizardState::MECHNET_PSK;
            _console.print("Generate PSK? (y/n): ");
            break;
        
        // ... etc
        
        case WizardState::MECHNET_CONFIRM:
            if (input[0] == 'y') {
                _brain->saveMechNetConfig(_wizardConfig);
                _console.println("✓ MechNet configured!");
            }Stream Buffer Size

**Question**: How large should ConsoleStream line buffer be?

**Current design**: 256 bytes
- Most Logger messages < 100 chars
- Handles long ListConfig output lines
- Small enough to avoid heap fragmentation

**Alternative**: Smaller buffer with truncation warning
- **Pro**: Less memory usage
- **Con**: May truncate important diagnostic messages

**Decision**: 256 bytes, monitor actual usage
- `mechnet status` - Show network config and connected nodes
- `mechnet start/stop` - Control MechNet Master
- `controller pair` - Interactive controller pairing
- `audio test` - Audio driver testing
- `motor calibrate` - Motor calibration wizard

### 9.3 Background Event Logging

**Example - Controller events**:
```cpp
// In DualRingController::task()
if (connectionLost) {
    ConsoleHandler::log("[RING] Connection lost: %s", ringName);
}

if (connectionRestored) {
    ConsoleHandler::log("[RING] Connection restored: %s", ringName);
}
```

**MechNet events** (future):
```cpp
// In MechNetMaster callback
void onNodeConnected(const char* nodeName) {
    ConsoleHandler::log("[MECHNET] Node connected: %s", nodeName);
}
```

---

## 10. Open Questions

### 10.1 Prompt Style

**Options**:
- `> ` (simple, current de-facto standard)
- `MechMind> ` (shows component name)
- `R2D2> ` (shows droid name from Brain constructor)

**Decision**: Start with `> `, make configurable later if needed

### 10.2 Console vs Logger Output

**Current approach**: Keep separate
- Logger: Diagnostic info, errors, warnings
- Console: User interaction, command responses

**Alternative**: Route all Logger output through Console async queue
- **Pro**: Unified output, better formatting
- **Con**: Logger becomes dependent on Console, adds complexity

**Decision**: Keep separate for now, revisit if user confusion

### 10.3 LocalCmdHandler Future

**Options**:
1. Keep both handlers indefinitely (clear separation)
2. Deprecate LocalCmdHandler, migrate all to ConsoleHandler
3. Merge into single handler with dual input paths

**Current decision**: Keep both - clean separation of concerns

### 10.4 Headless Mode

**Question**: What if CONSOLE_STREAM is NULL?
- ConsoleHandler not created
- Brain runs without interactive console
- LocalCmdHandler still works for programmatic commands

**Validation needed**: Test headless operation after migration

---

## 11. References

### 11.1 MechNetCentral Example

- [SmartCLI Migration Design](../../MechNet/examples/mechnet_central/docs/SmartCLI-Migration-Design.md) - Full migration example
- [ConsoleHandler.h](../../MechNet/examples/mechnet_central/src/cli/ConsoleHandler.h) - Reference implementation
- [ConsoleHandler.cpp](../../MechNet/examples/mechnet_central/src/cli/ConsoleHandler.cpp) - Command parsing pattern

### 11.2 SmartCLI Documentation

- [SmartCLI README](https://github.com/kizmit99/SmartCLI/blob/main/README.md) - Library overview
- [Quick Reference](https://github.com/kizmit99/SmartCLI/blob/main/docs/QUICK_REFERENCE.md) - API examples
- [basic_cli Example](https://github.com/kizmit99/SmartCLI/tree/main/examples/basic_cli) - Simple usage pattern

### 11.3 MechMind Architecture

- [Brain.cpp](../src/droid/brain/Brain.cpp) - Current console implementation
- [LocalCmdHandler.cpp](../src/droid/brain/LocalCmdHandler.cpp) - Existing commands
- [hardware.config.h](../settings/hardware.config.h) - Stream configuration

---

## 12. Success Criteria

**Migration is complete when**:

1. ✅ All existing console commands work identically
2. ✅ Console shows banner on startup
3. ✅ User can type without interference from background events
4. ✅ LocalCmdHandler still handles ActionMgr-routed commands
5. ✅ No regressions in Brain functionality
6. ✅ Code compiles without warnings
7. ✅ Device boots and accepts commands via serial
8. ✅ Wizard infrastructure ready for future provisioning

**Non-functional requirements**:
- Console response time < 100ms for simple commands
- No memory leaks (heap usage stable)
- Build size increase < 50KB (SmartCLI overhead)

---

## Document History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | Jan 28, 2026 | Initial design for SmartCLI migration |
