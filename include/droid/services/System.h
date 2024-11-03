#pragma once
#include "droid/services/Config.h"
#include "droid/services/Logger.h"

constexpr droid::services::Logger::Level NONE = droid::services::Logger::Level::NONE; 
constexpr droid::services::Logger::Level DEBUG = droid::services::Logger::Level::DEBUG; 
constexpr droid::services::Logger::Level INFO = droid::services::Logger::Level::INFO; 
constexpr droid::services::Logger::Level WARN = droid::services::Logger::Level::WARN; 
constexpr droid::services::Logger::Level ERROR = droid::services::Logger::Level::ERROR; 
constexpr droid::services::Logger::Level FATAL = droid::services::Logger::Level::FATAL;

namespace droid::services {
    class System {
    public:
        System(const char* name, Stream* out);
        const char* getName();
        Config* getConfig();
        Logger* getLogger();

    private:
        const char* name;
        Config config;
        Logger logger;
    };
}