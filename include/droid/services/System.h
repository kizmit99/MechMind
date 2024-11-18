#pragma once
#include "droid/services/Config.h"
#include "droid/services/Logger.h"
#include "droid/services/DroidState.h"

constexpr droid::services::Logger::Level DEBUG = droid::services::Logger::Level::DEBUG; 
constexpr droid::services::Logger::Level INFO = droid::services::Logger::Level::INFO; 
constexpr droid::services::Logger::Level WARN = droid::services::Logger::Level::WARN; 
constexpr droid::services::Logger::Level ERROR = droid::services::Logger::Level::ERROR; 
constexpr droid::services::Logger::Level FATAL = droid::services::Logger::Level::FATAL;

// Forward declaration of PWMService
namespace droid::services {
    class PWMService;
}

namespace droid::services {
    class System {
    public:
        System(Stream* out, Logger::Level defaultLogLevel);
        Config* getConfig();
        Logger* getLogger();
        void setPWMService(PWMService*);
        PWMService* getPWMService();
        DroidState* getDroidState();

    private:
        Config config;
        Logger logger;
        DroidState droidState;
        PWMService* pwmService;
    };
}