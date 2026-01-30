/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/MechMind
 */

#include "droid/network/MechNetNode.h"

#define CONFIG_KEY_MECHNET_ENABLE        "MNEnable"
#define CONFIG_KEY_MECHNET_INITIALIZED   "MNInit"
#define CONFIG_KEY_MECHNET_NETWORK_NAME  "MNNetName"
#define CONFIG_KEY_MECHNET_CHANNEL       "MNChannel"
#define CONFIG_KEY_MECHNET_PSK           "MNPSK"

#define CONFIG_DEFAULT_MECHNET_ENABLE       false
#define CONFIG_DEFAULT_MECHNET_INITIALIZED  false
#define CONFIG_DEFAULT_MECHNET_NETWORK_NAME "MechNet"
#define CONFIG_DEFAULT_MECHNET_CHANNEL      6
#define CONFIG_DEFAULT_MECHNET_PSK          "0000000000000000000000000000000000000000000000000000000000000000"

namespace droid::network {
    MechNetNode::MechNetNode(const char* name, droid::core::System* system) :
        BaseComponent(name, system),
        mechNetMaster(nullptr),
        initialized(false) {
    }

    MechNetNode::~MechNetNode() {
        if (mechNetMaster) {
            delete mechNetMaster;
        }
    }

    void MechNetNode::init() {
        // Check if MechNet is enabled
        bool enabled = config->getBool(name, CONFIG_KEY_MECHNET_ENABLE, CONFIG_DEFAULT_MECHNET_ENABLE);
        if (!enabled) {
            logger->log(name, INFO, "MechNet disabled\n");
            return;
        }

        bool provisioned = config->getBool(name, CONFIG_KEY_MECHNET_INITIALIZED, CONFIG_DEFAULT_MECHNET_INITIALIZED);

        if (!provisioned) {
            logger->log(name, WARN, "MechNet enabled but not provisioned - run mechnet-provision\n");
            return;
        }

        // Load provisioned configuration
        String networkName = config->getString(name, CONFIG_KEY_MECHNET_NETWORK_NAME, CONFIG_DEFAULT_MECHNET_NETWORK_NAME);
        uint8_t channel = config->getInt(name, CONFIG_KEY_MECHNET_CHANNEL, CONFIG_DEFAULT_MECHNET_CHANNEL);
        String pskHex = config->getString(name, CONFIG_KEY_MECHNET_PSK, CONFIG_DEFAULT_MECHNET_PSK);

        // Validate PSK (must be 64 hex characters = 32 bytes)
        if (pskHex.length() != 64) {
            logger->log(name, ERROR, "Invalid PSK length: %d (expected 64 hex chars)\n", pskHex.length());
            return;
        }

        // Convert hex string to bytes
        uint8_t psk[32];
        for (size_t i = 0; i < 32; i++) {
            char byteStr[3] = {pskHex[i*2], pskHex[i*2 + 1], 0};
            psk[i] = (uint8_t)strtol(byteStr, NULL, 16);
        }

        // Create MechNetMaster instance
        // Note: MechNetMaster constructor requires network name and optional Stream for logging
        // We don't pass a stream for logging to avoid duplicate logs (MechNet logs through its facade)
        mechNetMaster = new MechNet::MechNetMaster(networkName.c_str(), nullptr);

        // Configure MechNet
        MechNet::NetworkConfig netCfg;
        netCfg.psk = psk;
        netCfg.pskLen = 32;
        netCfg.channel = channel;

        // Initialize MechNet
        if (!mechNetMaster->begin(&netCfg)) {
            logger->log(name, ERROR, "MechNet initialization failed\n");
            delete mechNetMaster;
            mechNetMaster = nullptr;
            return;
        }

        initialized = true;
        logger->log(name, INFO, "MechNet Master initialized: %s (channel %d)\n", 
                   networkName.c_str(), channel);
    }

    void MechNetNode::factoryReset() {
        config->putBool(name, CONFIG_KEY_MECHNET_ENABLE, CONFIG_DEFAULT_MECHNET_ENABLE);
        config->putBool(name, CONFIG_KEY_MECHNET_INITIALIZED, CONFIG_DEFAULT_MECHNET_INITIALIZED);
        config->putString(name, CONFIG_KEY_MECHNET_NETWORK_NAME, CONFIG_DEFAULT_MECHNET_NETWORK_NAME);
        config->putInt(name, CONFIG_KEY_MECHNET_CHANNEL, CONFIG_DEFAULT_MECHNET_CHANNEL);
        config->putString(name, CONFIG_KEY_MECHNET_PSK, CONFIG_DEFAULT_MECHNET_PSK);
    }

    void MechNetNode::task() {
        if (!mechNetMaster) return;
        
        // MechNet housekeeping (retries, heartbeats, link monitoring)
        mechNetMaster->task();
    }

    void MechNetNode::logConfig() {
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_MECHNET_ENABLE, config->getString(name, CONFIG_KEY_MECHNET_ENABLE, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_MECHNET_INITIALIZED, config->getString(name, CONFIG_KEY_MECHNET_INITIALIZED, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_MECHNET_NETWORK_NAME, config->getString(name, CONFIG_KEY_MECHNET_NETWORK_NAME, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_MECHNET_CHANNEL, config->getString(name, CONFIG_KEY_MECHNET_CHANNEL, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_MECHNET_PSK, config->getString(name, CONFIG_KEY_MECHNET_PSK, "").c_str());
    }

    void MechNetNode::failsafe() {
        // No action needed - MechNet has no motors/actuators to stop
    }

    // Instance config methods
    MechNetConfig MechNetNode::getConfig() {
        MechNetConfig cfg;
        cfg.enabled = config->getBool(name, CONFIG_KEY_MECHNET_ENABLE, CONFIG_DEFAULT_MECHNET_ENABLE);
        cfg.initialized = config->getBool(name, CONFIG_KEY_MECHNET_INITIALIZED, CONFIG_DEFAULT_MECHNET_INITIALIZED);
        cfg.networkName = config->getString(name, CONFIG_KEY_MECHNET_NETWORK_NAME, CONFIG_DEFAULT_MECHNET_NETWORK_NAME);
        cfg.channel = config->getInt(name, CONFIG_KEY_MECHNET_CHANNEL, CONFIG_DEFAULT_MECHNET_CHANNEL);
        cfg.pskHex = config->getString(name, CONFIG_KEY_MECHNET_PSK, CONFIG_DEFAULT_MECHNET_PSK);
        return cfg;
    }

    void MechNetNode::saveConfig(const MechNetConfig& cfg) {
        config->putBool(name, CONFIG_KEY_MECHNET_ENABLE, cfg.enabled);
        config->putBool(name, CONFIG_KEY_MECHNET_INITIALIZED, cfg.initialized);
        config->putString(name, CONFIG_KEY_MECHNET_NETWORK_NAME, cfg.networkName.c_str());
        config->putInt(name, CONFIG_KEY_MECHNET_CHANNEL, cfg.channel);
        config->putString(name, CONFIG_KEY_MECHNET_PSK, cfg.pskHex.c_str());
    }
    
    bool MechNetNode::sendCommand(const char* nodeName, const char* command, bool requiresAck) {
        if (!mechNetMaster) return false;
        return mechNetMaster->sendTo(nodeName, command, requiresAck);
    }

    void MechNetNode::findAllNodesByPrefix(const char* prefix, std::function<void(const char*)> callback) {
        if (!mechNetMaster) return;

        char nodeName[32];
        for (uint8_t i = 0; i < mechNetMaster->connectedNodeCount(); i++) {
            if (mechNetMaster->getConnectedNodeName(i, nodeName, sizeof(nodeName))) {
                if (strncmp(nodeName, prefix, strlen(prefix)) == 0) {
                    callback(nodeName);  // Invoke callback for each match
                }
            }
        }
    }

    uint8_t MechNetNode::connectedNodeCount() {
        if (!mechNetMaster) return 0;
        return mechNetMaster->connectedNodeCount();
    }

    bool MechNetNode::getConnectedNodeName(uint8_t index, char* nameOut, size_t buflen) {
        if (!mechNetMaster) return false;
        return mechNetMaster->getConnectedNodeName(index, nameOut, buflen);
    }

    bool MechNetNode::messageAvailable() {
        if (!mechNetMaster) return false;
        return mechNetMaster->messageAvailable();
    }

    String MechNetNode::nextMessage() {
        if (!mechNetMaster) return "";
        return mechNetMaster->nextMessage();
    }

    String MechNetNode::lastSender() {
        if (!mechNetMaster) return "";
        return mechNetMaster->lastSender();
    }
}
