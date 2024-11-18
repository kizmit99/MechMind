#pragma once
#include <Arduino.h>

namespace droid::services {
    struct LogConfigEntry;

    class Logger {
    public:
        enum Level {
            DEBUG, INFO, WARN, ERROR, FATAL};

        Logger(Print* out, Level defaultLevel);
        void log(const char* compName, Level level, const char *format, ...)  __attribute__ ((format (printf, 4, 5)));
        void printf(const char *format, ...) __attribute__ ((format (printf, 2, 3)));
        void setLogLevel(const char* compName, Level level);
        Level getLogLevel(const char* compName);
        Level getMaxLevel();
        void clear();

    private:
        const char* const levelStr[6] = {
            "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
        Print* out;
        char buf[256];
        Level maxLevel;
        LogConfigEntry* logConfigList;

        void updateLevel(Level newLevel);
    };

    struct LogConfigEntry {
    public:
        LogConfigEntry(const char* compName, Logger::Level level, LogConfigEntry* prev) :
            compName(compName),
            level(level),
            prev(prev) {};
        LogConfigEntry* prev;
        LogConfigEntry* next = NULL;
        const char* compName;
        Logger::Level level;
    };
}
