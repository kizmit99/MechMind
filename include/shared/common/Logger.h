#pragma once
#include <Arduino.h>

#define LOGGER_NAME "Logger"

enum LogLevel {
        DEBUG, INFO, WARN, ERROR, FATAL};

struct LogConfigEntry {
public:
    LogConfigEntry(const char* compName, LogLevel level) :
        level(level),
        next(NULL) {
        strncpy(this->compName, compName, sizeof(this->compName));
        this->compName[sizeof(this->compName) - 1] = '\0';
    };
    LogConfigEntry* next;
    char compName[16];
    LogLevel level;
};

class Logger {
public:
    Logger(Print* out, LogLevel defaultLevel) :
        out(out) {
        logConfigList = new LogConfigEntry(LOGGER_NAME, defaultLevel);
    }

    void log(const char* compName, LogLevel level, const char *format, ...)  __attribute__ ((format (printf, 4, 5))) {
        updateLevel(level);
        if (out) {
            if (level >= getLogLevel(compName)) {
                out->printf("%s : %d : %s : ", levelStr[level], millis(), compName);
                va_list args; 
                va_start(args, format); 
                vsnprintf(buf, sizeof(buf), format, args); 
                va_end(args);
                out->print(buf);
            }
        }
    }

    void printf(const char* compName, LogLevel level, const char *format, ...) __attribute__ ((format (printf, 4, 5))) {
        updateLevel(level);
        if (out) {
            if (level >= getLogLevel(compName)) {
                va_list args; 
                va_start(args, format); 
                vsnprintf(buf, sizeof(buf), format, args); 
                va_end(args);
                out->print(buf);
            }
        }
    }

    void setLogLevel(const char* compName, LogLevel level) {
        LogConfigEntry* head = logConfigList;
        LogConfigEntry* entry = head;
        while (entry != NULL) {
            if (strcmp(entry->compName, compName) == 0) {
                break;
            }
            entry = entry->next;
        }
        if (entry == NULL) {
            entry = new LogConfigEntry(compName, level);
            entry->next = head->next;
            head->next = entry;
        }
        entry->level = level;
    }

    LogLevel getLogLevel(const char* compName) {
        LogConfigEntry* head = logConfigList;
        LogConfigEntry* entry = head;
        while (entry != NULL) {
            if (strcmp(entry->compName, compName) == 0) {
                break;
            }
            entry = entry->next;
        }
        if (entry == NULL) {
            entry = head;
        }
        return entry->level;
    }

    LogLevel getMaxLevel() {
        return maxLevel;
    }

    void clear() {
        maxLevel = DEBUG;
    }

private:
    const char* const levelStr[7] = {
        "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    Print* out;
    char buf[256];
    LogLevel maxLevel;
    LogConfigEntry* logConfigList;

    void updateLevel(LogLevel newLevel) {
        if (newLevel > maxLevel) {
            maxLevel = newLevel;
        }
    }
};
