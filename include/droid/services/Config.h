#pragma once

#include <Preferences.h>

namespace droid::services {
    class Config {
    public:
        Config();
        void putInt(const char* nspace, const char* key, int value);
        int getInt(const char* nspace, const char* key, int defaultValue);
        void putBool(const char* nspace, const char* key, bool value);
        bool getBool(const char* nspace, const char* key, bool defaultValue);
        void putFloat(const char* nspace, const char* key, float value);
        float getFloat(const char* nspace, const char* key, float defaultValue);
        void putString(const char* nspace, const char* key, const char* value);
        String getString(const char* nspace, const char* key, const char* defaultValue);
        void clear(const char* nspace);
        void remove(const char* nspace, const char* key);
        bool isKey(const char* nspace, const char* key);

    private:
        Preferences preferences;
    };
}
