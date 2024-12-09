/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

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