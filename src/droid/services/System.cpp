#include "droid/services/System.h"

namespace droid::services {
    System::System(const char* name, Stream* out, Logger::Level defaultLogLevel, PWMService* pwmService) :
        name(name),
        logger(out, defaultLogLevel),
        pwmService(pwmService) {
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

    PWMService* System::getPWMService() {
        return pwmService;
    }

    DroidState* System::getDroidState() {
        return &droidState;
    }
}