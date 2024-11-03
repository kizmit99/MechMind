#include "droid/services/System.h"

namespace droid::services {
    System::System(const char* name, Stream* out) :
        name(name),
        logger(out) {
    }

    const char* System::getName() {
        return name;
    }

    Config* System::getConfig() {
        return &config;
    }

    Logger* System::getLogger() {
        return &logger;
    }
}