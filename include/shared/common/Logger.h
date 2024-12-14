/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/MechMind
 */

#pragma once
#include <Arduino.h>

#define LOGGER_NAME "Logger"

enum LogLevel {
        DEBUG, INFO, WARN, ERROR, FATAL};

struct LogConfigEntry {
public:
    LogConfigEntry(const char* compName, LogLevel level) :
        level(level),
        name(compName),
        next(NULL) {
    };
    LogConfigEntry* next = nullptr;
    const char* name = nullptr;
    LogLevel level = DEBUG;
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

    /**
     * @brief Set the minumum LogLevel that will be logged, for the given component name
     * Note that the compName* must be specified as a literal string ("someName") or at
     * the very least as a char* that is pointing to a char[] that will be available and 
     * constant through the life of the program.  This method will not copy the contents
     * of the string, it only maintains the pointer you provide.
     * 
     * @param compName 
     * @param level 
     */
    void setLogLevel(const char* compName, LogLevel level) {
        LogConfigEntry* head = logConfigList;
        LogConfigEntry* entry = head;
        while (entry != NULL) {
            if (strcasecmp(entry->name, compName) == 0) {
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
            if (strcasecmp(entry->name, compName) == 0) {
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
    Print* out = nullptr;
    char buf[256] = {0};
    LogLevel maxLevel = DEBUG;
    LogConfigEntry* logConfigList = nullptr;

    void updateLevel(LogLevel newLevel) {
        if (newLevel > maxLevel) {
            maxLevel = newLevel;
        }
    }
};
