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
#include "droid/core/BaseComponent.h"
#include <facade/MechNetMaster.h>

namespace droid::network {
    // Configuration structure for MechNet provisioning
    struct MechNetConfig {
        bool enabled;
        bool initialized;
        String networkName;
        int channel;
        String pskHex;  // 64 hex characters
    };

    class MechNetNode : public droid::core::BaseComponent {
    public:
        MechNetNode(const char* name, droid::core::System* system);

        ~MechNetNode();

        // BaseComponent lifecycle
        void init() override;
        void factoryReset() override;
        void task() override;
        void logConfig() override;
        void failsafe() override;

        // Public API for handlers and Brain
        MechNetConfig getConfig();
        void saveConfig(const MechNetConfig& cfg);
        bool sendCommand(const char* nodeName, const char* command, bool requiresAck = true);
        bool findNodeByPrefix(const char* prefix, char* nodeNameOut, size_t buflen);
        uint8_t connectedNodeCount();
        bool getConnectedNodeName(uint8_t index, char* nameOut, size_t buflen);
        bool messageAvailable();
        String nextMessage();
        String lastSender();
        bool isInitialized() { return initialized; }

    private:
        MechNet::MechNetMaster* mechNetMaster;
        bool initialized;
    };
}
