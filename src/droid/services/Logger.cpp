#include "droid/services/Logger.h"

namespace droid::services {
    Logger::Logger() {}

    Logger::Logger(Print* out) {
        this->out = out;
    }

    void Logger::log(const char* compName, Level level, const char *format, ...) {
        if (out) {
            out->printf("%s : %s : ", levelStr[level], compName);
            va_list args; 
            va_start(args, format); 
            vsnprintf(buf, sizeof(buf), format, args); 
            va_end(args);
            out->print(buf);
         }
    }

    void Logger::clear() {
        maxLevel = NONE;
    }

    Logger::Level Logger::getMaxLevel() {
        return maxLevel;
    }

    void Logger::updateLevel(Level newLevel) {
        if (newLevel > maxLevel) {
            maxLevel = newLevel;
        }
    }
}