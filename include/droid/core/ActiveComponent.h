/*
 * Droid Brain Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#pragma once
#include "droid/core/PassiveComponent.h"

namespace droid::core {
    class ActiveComponent : public PassiveComponent {
    public:
    /**
     * @brief Construct a new ActiveComponent.
     * Note that the name* must be specified as a literal string ("someName") or at
     * the very least as a char* that is pointing to a char[] that will be available and 
     * constant through the life of the program.  This method will not copy the contents
     * of the string, it only maintains the pointer you provide.
     * 
     * @param name 
     * @param system 
     */
        ActiveComponent(const char* name, System* system) :
            PassiveComponent(name, system) {}

        virtual void init() = 0;
        virtual void factoryReset() = 0;
        virtual void task() = 0;
        virtual void logConfig() = 0;
        virtual void failsafe() = 0;
    };
}
