#pragma once
#include <Arduino.h>

//Forward declaration of Config class
class Config;

enum LogLevel {
        DEBUG, INFO, WARN, ERROR, FATAL};

struct LogConfigEntry {
public:
    LogConfigEntry(const char* compName, LogLevel level, LogConfigEntry* prev) :
        compName(compName),
        level(level),
        prev(prev) {};
    LogConfigEntry* prev;
    LogConfigEntry* next = NULL;
    const char* compName;
    LogLevel level;
};

class Logger {
public:
    Logger(Print* out, LogLevel defaultLevel) :
        out(out) {
        logConfigList = new LogConfigEntry("Logger", defaultLevel, NULL);
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

    void printf(const char *format, ...) __attribute__ ((format (printf, 2, 3))) {
        if (out) {
            va_list args; 
            va_start(args, format); 
            vsnprintf(buf, sizeof(buf), format, args); 
            va_end(args);
            out->print(buf);
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
            entry = new LogConfigEntry(compName, level, head);
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
        if (entry != NULL) {
            return entry->level;
        } else {
            return head->level;
        }
    }

    LogLevel getMaxLevel() {
        return maxLevel;
    }

    void clear() {
        maxLevel = DEBUG;
    }

    void setConfig(Config* config) {
        this->config = config;
    }

private:
    const char* const levelStr[6] = {
        "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    Print* out;
    char buf[256];
    LogLevel maxLevel;
    LogConfigEntry* logConfigList;
    Config* config;

    void updateLevel(LogLevel newLevel) {
        if (newLevel > maxLevel) {
            maxLevel = newLevel;
        }
    }
};

#include "shared/common/Config.h"
