#pragma once
#include <Preferences.h>

//Forward declaration of Logger class
class Logger;

class Config {
public:
    Config() {}

    void putInt(const char* nspace, const char* key, int value) {
        char buf[20];
        snprintf(buf, sizeof(buf), "%d", value);
        putString(nspace, key, buf);
    }

    int getInt(const char* nspace, const char* key, int defaultValue) {
        int value = defaultValue;
        char defaultStr[20];
        snprintf(defaultStr, sizeof(defaultStr), "%d", defaultValue);
        String configValue = getString(nspace, key, defaultStr);
        const char* cValue = configValue.c_str();
        char* endPtr;
        value = strtol(cValue, &endPtr, 10);
        if ((endPtr == cValue) || (*endPtr != '\0')) {   //parse error
            log("Parse ERROR in Config.getInt(%s, %s, %d) value from store: '%s'\n", nspace, key, defaultValue, cValue);
            value = defaultValue;
        }
        return value;
    }

    void putBool(const char* nspace, const char* key, bool value) {
        char buf[20];
        snprintf(buf, sizeof(buf), "%d", value);
        putString(nspace, key, buf);
    }

    bool getBool(const char* nspace, const char* key, bool defaultValue) {
        bool value = defaultValue;
        char defaultStr[20];
        snprintf(defaultStr, sizeof(defaultStr), "%d", defaultValue);
        String configValue = getString(nspace, key, defaultStr);
        const char* cValue = configValue.c_str();
        char* endPtr;
        value = (bool) strtol(cValue, &endPtr, 10);
        if ((endPtr == cValue) || (*endPtr != '\0')) {   //parse error
            log("Parse ERROR in Config.getBool(%s, %s, %d) value from store: '%s'\n", nspace, key, defaultValue, cValue);
            value = defaultValue;
        }
        return value;
    }

    void putFloat(const char* nspace, const char* key, float value) {
        char buf[20];
        snprintf(buf, sizeof(buf), "%.3f", value);
        putString(nspace, key, buf);
    }

    float getFloat(const char* nspace, const char* key, float defaultValue) {
        float value = defaultValue;
        char defaultStr[20];
        snprintf(defaultStr, sizeof(defaultStr), "%.3f", defaultValue);
        String configValue = getString(nspace, key, defaultStr);
        const char* cValue = configValue.c_str();
        char* endPtr;
        value = strtof(cValue, &endPtr);
        if ((endPtr == cValue) || (*endPtr != '\0')) {   //parse error
            log("Parse ERROR in Config.getFloat(%s, %s, %.3f) value from store: '%s'\n", nspace, key, defaultValue, cValue);
            value = defaultValue;
        }
        return value;
    }

    void putString(const char* nspace, const char* key, const char* value) {
        if (preferences.begin(nspace, false)) {
            preferences.putString(key, value);
            preferences.end();
        }
    }

    String getString(const char* nspace, const char* key, const char* defaultValue) {
        String value = defaultValue;
        if (preferences.begin(nspace, true)) {
            value = preferences.getString(key, defaultValue);
            preferences.end();
        }
        return value;
    }

    void clear(const char* nspace) {
        if (preferences.begin(nspace, false)) {
            preferences.clear();
            preferences.end();
        }
    }

    void remove(const char* nspace, const char* key) {
        if (preferences.begin(nspace, false)) {
            preferences.remove(key);
            preferences.end();
        }
    }

    bool isKey(const char* nspace, const char* key) {
        bool isKey = false;
        if (preferences.begin(nspace, false)) {
            isKey = preferences.isKey(key);
            preferences.end();
        }
        return isKey;
    }

    void setLogger(Logger* logger) {
        this->logger = logger;
    }

private:
    void log(const char* nspace, const char *format, ...)  __attribute__ ((format (printf, 3, 4)));

    Preferences preferences;
    Logger* logger;
};

#include "shared/common/Logger.h"

inline void Config::log(const char* nspace, const char *format, ...) {
    if (logger) {
        va_list args; 
        va_start(args, format); 
        logger->log(nspace, LogLevel::WARN, format, args);
    }
}