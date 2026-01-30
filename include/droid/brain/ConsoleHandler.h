/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/MechMind
 */

#pragma once
#include <Arduino.h>
#include <smartcli/SmartCLI.h>

namespace droid::brain {
    class Brain;  // Forward declaration
    
    /**
     * Interactive admin console using SmartCLI library.
     * 
     * Handles Category A commands (admin/provisioning):
     * - System: Help, Restart, FactoryReset
     * - Config: ListConfig, SetConfig, LogLevel
     * - Actions: SetAction, ResetAction, Play
     * - Testing: TestPanel
     * 
     * Category B commands (runtime state) remain in LocalCmdHandler
     * and are triggered via ActionMgr from controller events.
     */
    class ConsoleHandler {
    public:
        ConsoleHandler(Stream& stream, Brain* brain);
        
        // Lifecycle
        void begin();    // Initialize Console, show banner
        void process();  // Call from Brain::task()
        
        // Static logging for background events (async, won't interrupt typing)
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
        
        // Admin command handlers
        void handleRestart();
        void handleFactoryReset();
        void handleListConfig();
        void handleSetConfig(const char* args);
        void handleSetAction(const char* args);
        void handleResetAction(const char* args);
        void handlePlay(const char* args);
        void handleTestPanel(const char* args);
        void handleLogLevel(const char* args);
        void handleMechNetStatus();
        void handleMechNetProvision();
        void handleMechNetSend(const char* args, bool requiresAck);
        
        // Wizard state machine
        enum class WizardState {
            NONE,                        // Not in wizard
            PROVISION_NETWORK_NAME,      // Step 1: Network name
            PROVISION_CHANNEL,           // Step 2: WiFi channel
            PROVISION_PSK,               // Step 3: PSK input
            PROVISION_CONFIRM            // Step 4: Confirmation
        };
        
        WizardState _wizardState;
        String _wizardNetworkName;
        int _wizardChannel;
        String _wizardPskHex;
        
        void processWizardInput(const String& input);
        void startProvisionWizard();
        void handleProvisionNetworkName(const String& input);
        void handleProvisionChannel(const String& input);
        void handleProvisionPsk(const String& input);
        void handleProvisionConfirm(const String& input);
    };
}
