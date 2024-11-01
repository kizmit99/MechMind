#pragma once
#include "droid/services/Config.h"
#include "droid/services/Log.h"

namespace droid::services {
    class System {
    public:
        System(const char* name, Stream* out);
        const char* getName();
        Config getConfig();
        Log getLog();

    private:
        const char* name;
        Config config;
        Log log;
    };
}