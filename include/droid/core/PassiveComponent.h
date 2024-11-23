#pragma once
#include "droid/core/System.h"

namespace droid::core {
    class PassiveComponent {
    public:
    /**
     * @brief Construct a new PassiveComponent.
     * Note that the name* must be specified as a literal string ("someName") or at
     * the very least as a char* that is pointing to a char[] that will be available and 
     * constant through the life of the program.  This method will not copy the contents
     * of the string, it only maintains the pointer you provide.
     * 
     * @param name 
     * @param system 
     */
        PassiveComponent(const char* name, droid::core::System* system) :
            name(name),
            system(system),
            logger(system->getLogger()),
            config(system->getConfig()),
            droidState(system->getDroidState()) {}

        const char* name;
    protected:
        droid::core::System* system;
        Logger* logger;
        Config* config;
        droid::services::DroidState* droidState;
    };
}