#include "droid/services/System.h"

namespace droid::services {
    System::System(const char* name, Stream* out, Logger::Level defaultLogLevel) :
        name(name),
        logger(out, defaultLogLevel) {
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