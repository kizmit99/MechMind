#pragma once

#include <Preferences.h>

namespace droid::services {
    class Config {
    public:
        Config();
        void putString(const char* nspace, const char* key, const char* value);
        String getString(const char* nspace, const char* key, const char* defaultValue);
        size_t getString(const char* nspace, const char* key, char* value, size_t maxLen);
        void clear(const char* nspace);
        void remove(const char* nspace, const char* key);
        bool isKey(const char* nspace, const char* key);

    private:
        Preferences preferences;
    };
}
