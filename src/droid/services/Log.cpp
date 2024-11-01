#include "droid/services/Log.h"

namespace droid::services {
    Log::Log() {}

    Log::Log(Print* out) {
        this->out = out;
    }

    void Log::setOut(Print* out) {
        this->out = out;
    }

    void Log::print(const char* compName, const char* msg) {
        if (out) {
            out->printf("%s : %s", compName, msg);
        }
    }

    void Log::println(const char* compName, const char* msg) {
        if (out) {
            out->printf("%s : %s\n", compName, msg);
        }
    }

    void Log::printf(const char* compName, const char *format, ...) {
        if (out) {
            out->printf("%s : ", compName);
            va_list args; 
            va_start(args, format); 
            vsnprintf(buf, sizeof(buf), format, args); 
            va_end(args);
            out->print(buf);
         }
    }
}