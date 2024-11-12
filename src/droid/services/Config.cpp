#include "droid/services/Config.h"

namespace droid::services {
    Config::Config() {}

    void Config::putString(const char* nspace, const char* key, const char* value) {
        Serial.println("Debug 1");
        if (preferences.begin(nspace, false)) {
            Serial.println("Debug 2");
            preferences.putString(key, value);
            Serial.println("Debug 3");
            preferences.end();
            Serial.println("Debug 4");
        }
        Serial.println("Debug 5");
    }

    String Config::getString(const char* nspace, const char* key, const char* defaultValue) {
        String value = defaultValue;
        if (preferences.begin(nspace, true)) {
            value = preferences.getString(key, defaultValue);
            preferences.end();
        }
        return value;
    }

    size_t Config::getString(const char* nspace, const char* key, char* value, size_t maxLen) {
        size_t length = 0;
        if (preferences.begin(nspace, true)) {
            length = preferences.getString(key, value, maxLen);
            preferences.end();
        }
        return length;
    }

    void Config::clear(const char* nspace) {
        if (preferences.begin(nspace, false)) {
            preferences.clear();
            preferences.end();
        }
    }

    void Config::remove(const char* nspace, const char* key) {
        if (preferences.begin(nspace, false)) {
            preferences.remove(key);
            preferences.end();
        }
    }

    bool Config::isKey(const char* nspace, const char* key) {
        bool isKey = false;
        if (preferences.begin(nspace, false)) {
            isKey = preferences.isKey(key);
            preferences.end();
        }
        return isKey;
    }
}
