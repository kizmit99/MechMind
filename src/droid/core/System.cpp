#include "droid/core/System.h"

namespace droid::core {
    System::System(Stream* logStream, droid::services::Logger::Level defaultLogLevel) :
        logger(logStream, defaultLogLevel) {
    }

    droid::services::Config* System::getConfig() {
        return &config;
    }

    droid::services::Logger* System::getLogger() {
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