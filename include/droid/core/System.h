#pragma once
#include "shared/common/Config.h"
#include "shared/common/Logger.h"
#include "droid/services/DroidState.h"

// Forward declaration of PWMService
namespace droid::services {
    class PWMService;
}

namespace droid::core {
    class System {
    public:
        System(Stream* out, LogLevel defaultLogLevel);
        Config* getConfig();
        Logger* getLogger();
        void setPWMService(droid::services::PWMService*);
        droid::services::PWMService* getPWMService();
        droid::services::DroidState* getDroidState();

    private:
        Config config;
        Logger logger;
        droid::services::DroidState droidState;
        droid::services::PWMService* pwmService;
    };
}