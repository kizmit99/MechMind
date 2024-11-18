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

namespace droid::core {
    class System {
    public:
        System(Stream* out, droid::services::Logger::Level defaultLogLevel);
        droid::services::Config* getConfig();
        droid::services::Logger* getLogger();
        void setPWMService(droid::services::PWMService*);
        droid::services::PWMService* getPWMService();
        droid::services::DroidState* getDroidState();

    private:
        droid::services::Config config;
        droid::services::Logger logger;
        droid::services::DroidState droidState;
        droid::services::PWMService* pwmService;
    };
}