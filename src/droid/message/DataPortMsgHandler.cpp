/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/MechMind
 */

#include "droid/message/DataPortMsgHandler.h"

namespace droid::message {
    
    DataPortMsgHandler::DataPortMsgHandler(const char* name, droid::core::System* system) :
        MsgHandler(name, system) {
    }

    bool DataPortMsgHandler::handleMessage(const String& sender, const String& message) {
        // Check if this message is from a DataPort node
        if (!sender.startsWith("DataPort")) {
            return false;  // Not for us, try next handler
        }

        // Phase 3: Just log the message
        logger->log(name, INFO, "RX [%s]: %s\n", sender.c_str(), message.c_str());

        // Future phases: Parse message protocol and react
        // Example protocol (TBD based on DataPort firmware):
        //   "BATT:85" -> Update battery level in DroidState
        //   "STATUS:OK" -> Clear error flags
        //   "@V1 status:ready" -> Response to status query

        return true;  // Message consumed
    }
}
