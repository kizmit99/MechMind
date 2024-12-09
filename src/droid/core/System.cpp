/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/MechMind
 */

#include "droid/core/System.h"

namespace droid::core {
    System::System(Stream* logStream, LogLevel defaultLogLevel) :
        logger(logStream, defaultLogLevel),
        config("Config", &logger) {}

    Config* System::getConfig() {
        return &config;
    }

    Logger* System::getLogger() {
        return &logger;
    }

    droid::services::DroidState* System::getDroidState() {
        return &droidState;
    }

    void System::setPWMService(droid::services::PWMService* pwmService) {
        this->pwmService = pwmService;
    }

    droid::services::PWMService* System::getPWMService() {
        return pwmService;
    }
}