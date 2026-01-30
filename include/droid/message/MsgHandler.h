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

namespace droid::message {
    class MsgHandler : public droid::core::BaseComponent {
    public:
        MsgHandler(const char* name, droid::core::System* system) :
            BaseComponent(name, system) {}

        // Virtual methods required by BaseComponent
        // Most subclasses will use default implementations (NOOPs)
        void init() override {}
        void factoryReset() override {}
        void task() override {}
        void logConfig() override {}
        void failsafe() override {}

        /**
         * Process an inbound message from a remote MechNet node.
         * 
         * @param sender Unique node name (e.g., "DataPort-6168", "DriveRing-A3F2")
         * @param message Protocol-specific message content
         * @return true if this handler processed the message, false to try next handler
         */
        virtual bool handleMessage(const String& sender, const String& message) = 0;
    };
}
