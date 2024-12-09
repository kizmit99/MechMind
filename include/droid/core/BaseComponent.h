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
#include "droid/core/System.h"

namespace droid::core {
    class BaseComponent {
    public:
        /**
         * @brief Constructor for a new BaseComponent.
         * Note that the name* must be specified as a literal string ("someName") or at
         * the very least as a char* that is pointing to a char[] that will be available and 
         * constant through the life of the program.  This method will not copy the contents
         * of the string, it only maintains the pointer you provide.
         * 
         * @param name 
         * @param system 
         */
        BaseComponent(const char* name, droid::core::System* system) :
            name(name),
            system(system),
            logger(system->getLogger()),
            config(system->getConfig()),
            droidState(system->getDroidState()) {}

        virtual void init() = 0;
        virtual void factoryReset() = 0;
        virtual void task() = 0;
        virtual void logConfig() = 0;
        virtual void failsafe() = 0;

        const char* name;
    protected:
        droid::core::System* system;
        Logger* logger;
        Config* config;
        droid::services::DroidState* droidState;
    };
}