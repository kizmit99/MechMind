#include "droid/services/Logger.h"

namespace droid::services {

    Logger::Logger(Print* out, Level defaultLevel) :
        out(out) {
        logConfigList = new LogConfigEntry("Logger", defaultLevel, NULL);
    }

    void Logger::setLogLevel(const char* compName, Logger::Level level) {
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

    Logger::Level Logger::getLogLevel(const char* compName) {
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

    void Logger::log(const char* compName, Level level, const char *format, ...) {
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

    void Logger::clear() {
        maxLevel = DEBUG;
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