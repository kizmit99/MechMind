#pragma once
#include <Arduino.h>

namespace droid::services {
    class Logger {
    public:
        enum Level {
            NONE, DEBUG, INFO, WARN, ERROR, FATAL};

        Logger();
        Logger(Print* out);
        void log(const char* compName, Level level, const char *format, ...)  __attribute__ ((format (printf, 4, 5)));
        Level getMaxLevel();
        void clear();

    private:
        const char* const levelStr[6] = {
            "NONE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
        Print* out;
        char buf[256];
        Level maxLevel;

        void updateLevel(Level newLevel);
    };
}
