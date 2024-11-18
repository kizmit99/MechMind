#include "droid/services/System.h"

namespace droid::services {
    System::System(Stream* logStream, Logger::Level defaultLogLevel) :
        logger(logStream, defaultLogLevel) {
    }

    Config* System::getConfig() {
        return &config;
    }

    Logger* System::getLogger() {
        return &logger;
    }

    DroidState* System::getDroidState() {
        return &droidState;
    }

    void System::setPWMService(PWMService* pwmService) {
        this->pwmService = pwmService;
    }

    PWMService* System::getPWMService() {
        return pwmService;
    }
}