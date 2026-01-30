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
#include "droid/network/MechNetNode.h"
#include "droid/core/System.h"
#include "settings/hardware.config.h"

namespace droid::brain {
    ConsoleHandler* ConsoleHandler::s_instance = nullptr;

    ConsoleHandler::ConsoleHandler(Stream& stream, Brain* brain)
        : _console(stream), _brain(brain), _wizardState(WizardState::NONE) {
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
        _console.println("  loglevel <lvl> [comp] - Set log level (0-4, optional component)");
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
        _console.println("MechNet:");
        _console.println("  mechnet-status        - Show MechNet status and connected nodes");
        _console.println("  mechnet-provision     - Configure MechNet (network, channel, PSK)");
        _console.println("  mechnet-start         - Enable MechNet (hot-start without restart)");
        _console.println("  mechnet-stop          - Disable MechNet (stops network)");
        _console.println("  mechnet-send <node> <msg> - Send message (no ACK)");
        _console.println("  mechnet-send-ack <node> <msg> - Send message (requires ACK)");
        _console.println("");
    }

    void ConsoleHandler::handleCommand(const char* line) {
        // If in wizard mode, route to wizard handler
        if (_wizardState != WizardState::NONE) {
            processWizardInput(String(line));
            return;
        }
        
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
        else if (strcmp(cmdBuf, "mechnet-status") == 0) {
            handleMechNetStatus();
        }
        else if (strcmp(cmdBuf, "mechnet-provision") == 0) {
            handleMechNetProvision();
        }
        else if (strcmp(cmdBuf, "mechnet-start") == 0) {
            handleMechNetStart();
        }
        else if (strcmp(cmdBuf, "mechnet-stop") == 0) {
            handleMechNetStop();
        }
        else if (strcmp(cmdBuf, "mechnet-send") == 0) {
            handleMechNetSend(argsBuf, false);
        }
        else if (strcmp(cmdBuf, "mechnet-send-ack") == 0) {
            handleMechNetSend(argsBuf, true);
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
    }

    void ConsoleHandler::handleLogLevel(const char* args) {
        char levelStr[16];
        char component[64];
        component[0] = '\0';
        
        // Parse: "<level> [component]"
        int parsed = sscanf(args, "%15s %63s", levelStr, component);
        
        if (parsed < 1) {
            _console.println("Usage: loglevel <level> [component]");
            _console.println("  level: 0-4 or DEBUG|INFO|WARN|ERROR|FATAL");
            _console.println("  component: optional, if omitted applies to all");
            _console.println("Examples:");
            _console.println("  loglevel WARN           # Set all to WARN");
            _console.println("  loglevel 2              # Set all to WARN (same as above)");
            _console.println("  loglevel DEBUG DriveMgr # Set DriveMgr to DEBUG");
            return;
        }
        
        // Parse level - try numeric first, then string
        int level = -1;
        const char* levelNames[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
        
        // Try as number
        if (strlen(levelStr) == 1 && levelStr[0] >= '0' && levelStr[0] <= '4') {
            level = levelStr[0] - '0';
        } else {
            // Try as enum name (case-insensitive)
            for (int i = 0; i < 5; i++) {
                if (strcasecmp(levelStr, levelNames[i]) == 0) {
                    level = i;
                    break;
                }
            }
        }
        
        if (level < 0 || level > 4) {
            _console.println("Error: level must be 0-4 or DEBUG|INFO|WARN|ERROR|FATAL");
            return;
        }
        
        if (strlen(component) == 0) {
            // No component specified - set all
            _brain->getSystem()->getLogger()->setAllLogLevels((LogLevel)level);
        } else {
            // Specific component (show confirmation for targeted change)
            _brain->getSystem()->getLogger()->setLogLevel(component, (LogLevel)level);
            _console.println("✓ %s → %s", component, levelNames[level]);
        }
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

    void ConsoleHandler::handleMechNetStatus() {
        auto* mechNet = _brain->getMechNetMaster();
        auto* config = _brain->getSystem()->getConfig();
        
        _console.println("");
        _console.println("MechNet Status");
        _console.println("═══════════════════════════════════");
        
        // Get complete config from MechNetMasterNode
        droid::network::MechNetConfig mnCfg = mechNet->getConfig();
        
        _console.println("Enabled:      %s", mnCfg.enabled ? "Yes" : "No");
        _console.println("Network Name: %s", mnCfg.networkName.c_str());
        _console.println("WiFi Channel: %d", mnCfg.channel);
        _console.println("PSK:          %s", mnCfg.pskHex.c_str());
        _console.println("───────────────────────────────────");
        _console.println("Running:      %s", (mechNet && mechNet->isInitialized()) ? "Yes" : "No");
        
        if (mechNet && mechNet->isInitialized()) {
            uint8_t nodeCount = mechNet->connectedNodeCount();
            _console.println("Connected Nodes (%d):", nodeCount);
            
            if (nodeCount == 0) {
                _console.println("  (none)");
            } else {
                char nodeName[32];
                for (uint8_t i = 0; i < nodeCount; i++) {
                    if (mechNet->getConnectedNodeName(i, nodeName, sizeof(nodeName))) {
                        _console.println("  %d. %s", i + 1, nodeName);
                    }
                }
            }
        }
        
        _console.println("");
    }

    void ConsoleHandler::handleMechNetProvision() {
        startProvisionWizard();
    }
    
    void ConsoleHandler::handleMechNetStart() {
        auto* mechNet = _brain->getMechNetMaster();
        
        if (mechNet->isInitialized()) {
            _console.println("! MechNet already running");
            return;
        }
        
        // Validate config before starting
        droid::network::MechNetConfig mnCfg = mechNet->getConfig();
        bool validPsk = (mnCfg.pskHex.length() == 64);
        bool validName = (mnCfg.networkName.length() > 0);
        
        if (!validPsk || !validName) {
            _console.println("! MechNet config invalid - run mechnet-provision first");
            return;
        }
        
        // Hot-start (runtime only, doesn't change config)
        if (mechNet->start()) {
            _console.println("✓ MechNet started: %s (channel %d)", mnCfg.networkName.c_str(), mnCfg.channel);
        } else {
            _console.println("! MechNet start failed (see logs)");
        }
    }
    
    void ConsoleHandler::handleMechNetStop() {
        auto* mechNet = _brain->getMechNetMaster();
        
        if (!mechNet->isInitialized()) {
            _console.println("! MechNet not running");
            return;
        }
        
        // Hot-stop (runtime only, doesn't change config)
        mechNet->stop();
        _console.println("✓ MechNet stopped");
    }
    
    // ============================================================================
    // Wizard State Machine Implementation
    // ============================================================================
    
    void ConsoleHandler::processWizardInput(const String& input) {
        switch (_wizardState) {
            case WizardState::PROVISION_NETWORK_NAME:
                handleProvisionNetworkName(input);
                break;
            case WizardState::PROVISION_CHANNEL:
                handleProvisionChannel(input);
                break;
            case WizardState::PROVISION_PSK:
                handleProvisionPsk(input);
                break;
            case WizardState::PROVISION_ENABLED:
                handleProvisionEnabled(input);
                break;
            case WizardState::PROVISION_CONFIRM:
                handleProvisionConfirm(input);
                break;
            default:
                break;
        }
    }
    
    void ConsoleHandler::startProvisionWizard() {
        auto* config = _brain->getSystem()->getConfig();
        
        // Enter wizard mode - async messages stay queued until wizard completes
        _console.wizardStart();
        
        _wizardState = WizardState::PROVISION_NETWORK_NAME;
        
        _console.println("");
        _console.println("MechNet Provisioning");
        _console.println("═══════════════════════════════════");
        
        String currentName = config->getString("MechNet", "MNNetName", "MechNet");
        _console.print("Network name [%s]: ", currentName.c_str());
    }
    
    void ConsoleHandler::handleProvisionNetworkName(const String& input) {
        auto* config = _brain->getSystem()->getConfig();
        String trimmed = input;
        trimmed.trim();
        
        if (trimmed.length() == 0) {
            // Keep current value
            _wizardNetworkName = config->getString("MechNet", "MNNetName", "MechNet");
        } else if (trimmed.length() > 32) {
            _console.println("! Network name too long (max 32 chars) - keeping current");
            _wizardNetworkName = config->getString("MechNet", "MNNetName", "MechNet");
        } else {
            _wizardNetworkName = trimmed;
        }
        
        // Transition to next state
        _wizardState = WizardState::PROVISION_CHANNEL;
        
        int currentChannel = config->getInt("MechNet", "MNChannel", 6);
        _console.print("WiFi channel (1-13) [%d]: ", currentChannel);
    }
    
    void ConsoleHandler::handleProvisionChannel(const String& input) {
        auto* config = _brain->getSystem()->getConfig();
        String trimmed = input;
        trimmed.trim();
        
        if (trimmed.length() == 0) {
            // Keep current value
            _wizardChannel = config->getInt("MechNet", "MNChannel", 6);
        } else {
            int channel = trimmed.toInt();
            if (channel < 1 || channel > 13) {
                _console.println("! Invalid channel - keeping current");
                _wizardChannel = config->getInt("MechNet", "MNChannel", 6);
            } else {
                _wizardChannel = channel;
            }
        }
        
        // Transition to next state
        _wizardState = WizardState::PROVISION_PSK;
        
        String currentPsk = config->getString("MechNet", "MNPSK", "");
        if (currentPsk.length() == 64) {
            _console.print("PSK (64 hex chars) [%s]: ", currentPsk.c_str());
        } else {
            _console.print("PSK (64 hex chars): ");
        }
    }
    
    void ConsoleHandler::handleProvisionPsk(const String& input) {
        auto* config = _brain->getSystem()->getConfig();
        String trimmed = input;
        trimmed.trim();
        
        if (trimmed.length() == 0) {
            // Try to keep current value
            String currentPsk = config->getString("MechNet", "MNPSK", "");
            if (currentPsk.length() == 64) {
                _wizardPskHex = currentPsk;
            } else {
                _console.println("! No PSK configured and none entered - aborting");
                _wizardState = WizardState::NONE;
                return;
            }
        } else if (trimmed.length() != 64) {
            _console.println("! PSK must be exactly 64 hex characters - aborting");
            _wizardState = WizardState::NONE;
            return;
        } else {
            // Validate hex format
            bool valid = true;
            for (size_t i = 0; i < 64; i++) {
                char c = trimmed[i];
                if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
                    valid = false;
                    break;
                }
            }
            
            if (!valid) {
                _console.println("! PSK must contain only hex characters (0-9, a-f, A-F) - aborting");
                _wizardState = WizardState::NONE;
                return;
            }
            
            _wizardPskHex = trimmed;
        }
        
        // Transition to enabled prompt
        _wizardState = WizardState::PROVISION_ENABLED;
        
        bool currentEnabled = config->getBool("MechNet", "MNEnable", false);
        _console.print("Enable at startup? (yes/no) [%s]: ", currentEnabled ? "yes" : "no");
    }
    
    void ConsoleHandler::handleProvisionEnabled(const String& input) {
        auto* config = _brain->getSystem()->getConfig();
        String trimmed = input;
        trimmed.trim();
        trimmed.toLowerCase();
        
        if (trimmed.length() == 0) {
            // Keep current value
            _wizardEnabled = config->getBool("MechNet", "MNEnable", false);
        } else if (trimmed == "yes" || trimmed == "y") {
            _wizardEnabled = true;
        } else if (trimmed == "no" || trimmed == "n") {
            _wizardEnabled = false;
        } else {
            _console.println("! Invalid input - keeping current");
            _wizardEnabled = config->getBool("MechNet", "MNEnable", false);
        }
        
        // Transition to confirmation
        _wizardState = WizardState::PROVISION_CONFIRM;
        _console.println("");
        _console.println("Network: %s | Channel: %d | Enabled: %s", 
                        _wizardNetworkName.c_str(), _wizardChannel, _wizardEnabled ? "Yes" : "No");
        _console.println("PSK: %s", _wizardPskHex.c_str());
        _console.print("Save? (yes/no) [no]: ");
    }
    
    void ConsoleHandler::handleProvisionConfirm(const String& input) {
        auto* config = _brain->getSystem()->getConfig();
        String trimmed = input;
        trimmed.trim();
        trimmed.toLowerCase();
        
        if (trimmed == "yes" || trimmed == "y") {
            // Save configuration using instance method
            auto* mechNet = _brain->getMechNetMaster();
            droid::network::MechNetConfig mnCfg;
            mnCfg.enabled = _wizardEnabled;
            mnCfg.networkName = _wizardNetworkName;
            mnCfg.channel = _wizardChannel;
            mnCfg.pskHex = _wizardPskHex;
            mechNet->saveConfig(mnCfg);
            
            if (_wizardEnabled) {
                _console.println("✓ Provisioned (enabled at startup). Restart or use 'mechnet-start' to activate.");
            } else {
                _console.println("✓ Provisioned (disabled at startup). Use 'mechnet-start' to activate.");
            }
        } else {
            _console.println("Provisioning cancelled.");
        }
        
        // Exit wizard mode (flushes queued async messages and shows prompt)
        _wizardState = WizardState::NONE;
        _console.wizardComplete();
    }

    void ConsoleHandler::handleMechNetSend(const char* args, bool requiresAck) {
        auto* mechNet = _brain->getMechNetMaster();
        
        // Check if MechNet is initialized
        if (!mechNet || !mechNet->isInitialized()) {
            _console.println("! MechNet not initialized");
            return;
        }
        
        // Parse arguments: <nodeName> <message>
        char nodeName[64];
        char message[192];
        
        // Find first space to split nodeName and message
        const char* spacePtr = strchr(args, ' ');
        if (!spacePtr) {
            _console.println("Usage: mechnet-send%s <nodeName> <message>", requiresAck ? "-ack" : "");
            _console.println("Example: mechnet-send%s DataPort-6168 @v1 status", requiresAck ? "-ack" : "");
            return;
        }
        
        // Extract node name
        size_t nodeNameLen = spacePtr - args;
        strncpy(nodeName, args, min(nodeNameLen, sizeof(nodeName) - 1));
        nodeName[min(nodeNameLen, sizeof(nodeName) - 1)] = '\0';
        
        // Extract message (trim leading spaces)
        const char* msgStart = spacePtr + 1;
        while (*msgStart == ' ' && *msgStart != '\0') {
            msgStart++;
        }
        strncpy(message, msgStart, sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';
        
        if (strlen(message) == 0) {
            _console.println("! Message cannot be empty");
            return;
        }
        
        // Check if node is connected
        bool nodeFound = false;
        uint8_t nodeCount = mechNet->connectedNodeCount();
        char connectedNodeName[64];
        
        for (uint8_t i = 0; i < nodeCount; i++) {
            if (mechNet->getConnectedNodeName(i, connectedNodeName, sizeof(connectedNodeName))) {
                if (strcmp(connectedNodeName, nodeName) == 0) {
                    nodeFound = true;
                    break;
                }
            }
        }
        
        if (!nodeFound) {
            _console.println("! MechNet node '%s' not found!", nodeName);
            return;
        }
        
        // Send message (only report failures)
        if (!mechNet->sendCommand(nodeName, message, requiresAck)) {
            _console.println("! Failed to send to %s", nodeName);
        }
    }
}

