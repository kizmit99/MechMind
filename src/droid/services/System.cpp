#include "droid/services/System.h"

namespace droid::services {
    System::System(const char* name, Stream* out) :
        name(name),
        log(out)
    {
        this->config = config;
        this->log = log;
    }

    const char* System::getName() {
        return name;
    }

    Config System::getConfig() {
        return config;
    }

    Log System::getLog() {
        return log;
    }
}