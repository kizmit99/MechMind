#include "droid/services/Config.h"

namespace droid::services {
    Config::Config() {}

    void Config::putInt(const char* nspace, const char* key, int value) {
        if (preferences.begin(nspace, false)) {
            preferences.putInt(key, value);
            preferences.end();
        }
    }

    int Config::getInt(const char* nspace, const char* key, int defaultValue) {
        int value = defaultValue;
        if (preferences.begin(nspace, true)) {
            value = preferences.getInt(key, defaultValue);
            preferences.end();
        }
        return value;
    }

    void Config::putBool(const char* nspace, const char* key, bool value) {
        if (preferences.begin(nspace, false)) {
            preferences.putBool(key, value);
            preferences.end();
        }
    }

    bool Config::getBool(const char* nspace, const char* key, bool defaultValue) {
        bool value = defaultValue;
        if (preferences.begin(nspace, true)) {
            value = preferences.getBool(key, defaultValue);
            preferences.end();
        }
        return value;
    }

    void Config::putString(const char* nspace, const char* key, const char* value) {
        if (preferences.begin(nspace, false)) {
            preferences.putString(key, value);
            preferences.end();
        }
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
