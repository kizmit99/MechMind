/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/MechMind
 */

#include "droid/brain/ConsoleHandler.h"
#include "droid/brain/Brain.h"
#include "droid/core/System.h"
#include "settings/hardware.config.h"

namespace droid::brain {
    ConsoleHandler* ConsoleHandler::s_instance = nullptr;

    ConsoleHandler::ConsoleHandler(Stream& stream, Brain* brain)
        : _console(stream), _brain(brain) {
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
        
        _console.begin();  // Shows banner + first prompt
    }

    void ConsoleHandler::process() {
        _console.process();  // Handle incoming characters, execute commands
    }

    void ConsoleHandler::showBanner() {
        _console.println("");
        _console.println("═══════════════════════════════════");
        _console.println("          MechMind v1.0");
        _console.println("═══════════════════════════════════");
        _console.println("");
        _console.println("Type 'help' for command list");
        _console.println("");
    }

    void ConsoleHandler::showHelp() {
        _console.println("");
        _console.println("MechMind Admin Console");
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
        _console.println("  testpanel <n> <val>   - Test panel servo (1-%d, 500-2500uS)", LOCAL_PANEL_COUNT);
        _console.println("");
    }

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
            while (*argsStart == ' ' && *argsStart != '\0') {
                argsStart++;
            }
            strncpy(argsBuf, argsStart, sizeof(argsBuf) - 1);
            argsBuf[sizeof(argsBuf) - 1] = '\0';
        } else {
            // No arguments
            strncpy(cmdBuf, line, sizeof(cmdBuf) - 1);
            cmdBuf[sizeof(cmdBuf) - 1] = '\0';
            argsBuf[0] = '\0';
        }
        
        // Convert command to lowercase for case-insensitive matching
        for (char* p = cmdBuf; *p; p++) {
            *p = tolower(*p);
        }
        
        // Dispatch to handlers
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
            handlePlay(argsBuf);
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
    }

    void ConsoleHandler::handleRestart() {
        _console.println("Restarting system...");
        _brain->reboot();
    }

    void ConsoleHandler::handleFactoryReset() {
        _console.println("WARNING: Factory reset will restore all defaults!");
        _console.println("Initiating factory reset...");
        _brain->factoryReset();
        _brain->reboot();
    }

    void ConsoleHandler::handleListConfig() {
        _console.println("Current configuration:");
        _console.println("═══════════════════════════════════");
        _brain->logConfig();
    }

    void ConsoleHandler::handleSetConfig(const char* args) {
        char namespace_buf[32];
        char key_buf[32];
        char value_buf[128];
        
        // Parse: "namespace key value"
        if (sscanf(args, "%31s %31s %127[^\n]", namespace_buf, key_buf, value_buf) != 3) {
            _console.println("Usage: setconfig <namespace> <key> <value>");
            _console.println("Example: setconfig R2D2 Controller DualRing");
            return;
        }
        
        // Validate lengths (NVS namespace/key limits)
        if (strlen(namespace_buf) > 15) {
            _console.println("Error: namespace max 15 characters");
            return;
        }
        if (strlen(key_buf) > 15) {
            _console.println("Error: key max 15 characters");
            return;
        }
        
        // Apply config
        _brain->getSystem()->getConfig()->putString(namespace_buf, key_buf, value_buf);
        _console.println("✓ Set %s.%s = %s", namespace_buf, key_buf, value_buf);
    }

    void ConsoleHandler::handleSetAction(const char* args) {
        char action_buf[64];
        char cmd_buf[192];
        
        // Parse: "action command"
        const char* spacePtr = strchr(args, ' ');
        if (!spacePtr) {
            _console.println("Usage: setaction <action> <command>");
            _console.println("Example: setaction Happy Audio>$30;Dome>@0T1");
            return;
        }
        
        size_t actionLen = spacePtr - args;
        strncpy(action_buf, args, min(actionLen, sizeof(action_buf) - 1));
        action_buf[min(actionLen, sizeof(action_buf) - 1)] = '\0';
        
        const char* cmdStart = spacePtr + 1;
        while (*cmdStart == ' ' && *cmdStart != '\0') {
            cmdStart++;
        }
        strncpy(cmd_buf, cmdStart, sizeof(cmd_buf) - 1);
        cmd_buf[sizeof(cmd_buf) - 1] = '\0';
        
        _brain->overrideCmdMap(action_buf, cmd_buf);
        _console.println("✓ Set action '%s' = %s", action_buf, cmd_buf);
    }

    void ConsoleHandler::handleResetAction(const char* args) {
        if (strlen(args) == 0) {
            _console.println("Usage: resetaction <action>");
            _console.println("Example: resetaction Happy");
            return;
        }
        
        _brain->overrideCmdMap(args, NULL);
        _console.println("✓ Reset action '%s' to default", args);
    }

    void ConsoleHandler::handlePlay(const char* args) {
        if (strlen(args) == 0) {
            _console.println("Usage: play <action>");
            _console.println("Examples:");
            _console.println("  play StickEnable      - Enable drive stick");
            _console.println("  play Happy            - Trigger happy action");
            _console.println("  play Dome>:OP01       - Send dome command");
            return;
        }
        
        _brain->fireAction(args);
        _console.println("✓ Executed: %s", args);
    }

    void ConsoleHandler::handleTestPanel(const char* args) {
        int panel, value;
        
        if (sscanf(args, "%d %d", &panel, &value) != 2) {
            _console.println("Usage: testpanel <panel> <value>");
            _console.println("  panel: 1-%d", LOCAL_PANEL_COUNT);
            _console.println("  value: 500-2500 (microseconds)");
            return;
        }
        
        if (panel < 1 || panel > LOCAL_PANEL_COUNT) {
            _console.println("Error: panel must be 1-%d", LOCAL_PANEL_COUNT);
            return;
        }
        
        if (value < 500 || value > 2500) {
            _console.println("Warning: value outside typical range (500-2500)");
        }
        
        char buf[100];
        snprintf(buf, sizeof(buf), "Panel>:TP%03d%04d", panel, value);
        _brain->fireAction(buf);
        _console.println("✓ Set panel %d to %duS", panel, value);
    }

    void ConsoleHandler::handleLogLevel(const char* args) {
        char component[64];
        int level;
        
        if (sscanf(args, "%63s %d", component, &level) != 2) {
            _console.println("Usage: loglevel <component> <level>");
            _console.println("  level: 0=DEBUG, 1=INFO, 2=WARN, 3=ERROR, 4=FATAL");
            _console.println("Example: loglevel DriveMgr 0");
            return;
        }
        
        if (level < 0 || level > 4) {
            _console.println("Error: level must be 0-4");
            return;
        }
        
        _brain->getSystem()->getLogger()->setLogLevel(component, (LogLevel)level);
        
        const char* levelNames[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
        _console.println("✓ Set %s log level to %s", component, levelNames[level]);
    }

    // Static logging wrappers
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
}
