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
#include "droid/message/MsgHandler.h"

namespace droid::message {
    class DataPortMsgHandler : public MsgHandler {
    public:
        DataPortMsgHandler(const char* name, droid::core::System* system);

        bool handleMessage(const String& sender, const String& message) override;

    private:
        // Future: Add state tracking, protocol parsing, etc.
    };
}
