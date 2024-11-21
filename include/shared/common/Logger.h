#pragma once
#include <Arduino.h>

//Forward declaration of Config class
class Config;

#define LOGGER_NAME     "Logger"
#define LOGGER_NAME_KEY "Names"

enum LogLevel {
        FORCE, DEBUG, INFO, WARN, ERROR, FATAL};

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
            if ((level == 0) || (level >= getLogLevel(compName))) {
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
            if ((level == 0) || (level >= getLogLevel(compName))) {
                va_list args; 
                va_start(args, format); 
                vsnprintf(buf, sizeof(buf), format, args); 
                va_end(args);
                out->print(buf);
            }
        }
    }

    void setLogLevel(const char* compName, LogLevel level, bool save = true) {
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
        if (save) {
            saveLogLevel(compName, level);
        }
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

    void setConfig(Config* config) {
        this->config = config;
        loadLogConfig();
    }

    void logConfig();

    void factoryReset();

private:
    const char* const levelStr[7] = {
        "FORCE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
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

    LogLevel logLevelFromString(String level) {
        for (int i = 0; i < 7; i++) {
            if (level == levelStr[i]) {
                return static_cast<LogLevel>(i);
            }
        }
        return logConfigList->level;
    }

    void loadLogConfig();

    void saveLogLevel(const char* compName, LogLevel level);
};

#include "shared/common/Config.h"

inline void Logger::factoryReset() {
    if (!config) {return;}
    config->clear(LOGGER_NAME);
}

inline void Logger::logConfig() {
    if (!config) {return;}
    String keys = config->getString(LOGGER_NAME, LOGGER_NAME_KEY, "");
    log(LOGGER_NAME, INFO, "Config %s = %s\n", LOGGER_NAME_KEY, keys.c_str());
    keys.trim();
    char buf[keys.length() + 1];
    keys.toCharArray(buf, keys.length() + 1);
    char* token = strtok(buf, ":");
    while (token != NULL) {
        log(LOGGER_NAME, INFO, "Config %s = %s\n", token, config->getString(LOGGER_NAME, token, ""));
        token = strtok(NULL, ":");
    }
}

inline void Logger::loadLogConfig() {
    if (!config) {return;}
    String keys = config->getString(LOGGER_NAME, LOGGER_NAME_KEY, "");
    keys.trim();
    char buf[keys.length() + 1];
    keys.toCharArray(buf, keys.length() + 1);
    char* token = strtok(buf, ":");
    while (token != NULL) {
        String level = config->getString(LOGGER_NAME, token, "");
        if (level != "") {
            setLogLevel(token, logLevelFromString(level), false);
        }
        token = strtok(NULL, ":");
    }
}

inline void Logger::saveLogLevel(const char* compName, LogLevel level) {
    if (!config) {return;}
    String keys = config->getString(LOGGER_NAME, LOGGER_NAME_KEY, "");
    keys.trim();
    char buf[keys.length() + 1];
    keys.toCharArray(buf, keys.length() + 1);
    char* token = strtok(buf, ":");
    bool found = false;
    while (token != NULL) {
        if (strcmp(compName, token) == 0) {
            found = true;
            break;
        }
        token = strtok(NULL, ":");
    }
    if (!found) {
        String newKeys = keys + ":" + compName;
        config->putString(LOGGER_NAME, LOGGER_NAME_KEY, newKeys.c_str());
    }
    config->putString(LOGGER_NAME, compName, levelStr[(int) level]);
}
