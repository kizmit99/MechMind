/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/MechMind
 */

#include "droid/command/MechNetCmdHandler.h"

namespace droid::command {
    
    MechNetCmdHandler::MechNetCmdHandler(const char* name, 
                                                   droid::core::System* system,
                                                   droid::network::MechNetNode* masterNode) :
        CmdHandler(name, system),
        mechNetMasterNode(masterNode) {
    }

    bool MechNetCmdHandler::process(const char* device, const char* command) {
        if (!mechNetMasterNode || !mechNetMasterNode->isInitialized()) {
            return false;  // MechNet not available
        }

        // Track if any nodes matched
        bool foundNodes = false;

        // Send to all nodes matching device prefix
        mechNetMasterNode->findAllNodesByPrefix(device, [&](const char* nodeName) {
            foundNodes = true;
            
            bool sent = mechNetMasterNode->sendCommand(nodeName, command);
            if (sent) {
                logger->log(name, DEBUG, "TX [%s]: %s>%s\n", 
                           nodeName, device, command);
            } else {
                logger->log(name, WARN, "Failed to send to %s: %s>%s\n", 
                           nodeName, device, command);
            }
        });

        // If no matching nodes found, let other handlers try
        if (!foundNodes) {
            return false;
        }

        return true;  // Command consumed (sent to one or more nodes)
    }
}
