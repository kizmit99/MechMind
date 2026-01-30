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

        // Try to find a connected node matching the device prefix
        char nodeName[32];
        if (mechNetMasterNode->findNodeByPrefix(device, nodeName, sizeof(nodeName))) {
            logger->log(name, DEBUG, "Routing %s>%s to node %s\n", 
                       device, command, nodeName);
            
            bool sent = mechNetMasterNode->sendCommand(nodeName, command, true);
            if (!sent) {
                logger->log(name, WARN, "Failed to send to %s: %s\n", nodeName, command);
            }
            return true;  // Command consumed (even if send failed)
        }

        // No matching node found, let other handlers try
        return false;
    }
}
