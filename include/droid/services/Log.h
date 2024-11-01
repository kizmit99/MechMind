#pragma once
#include <Arduino.h>

namespace droid::services {
    class Log {
    public:
        Log();
        Log(Print* out);
        void setOut(Print* out);
        void print(const char* compName, const char* msg);
        void println(const char* compName, const char* msg);
        void printf(const char* compName, const char *format, ...)  __attribute__ ((format (printf, 3, 4)));

    private:
        Print* out;
        char buf[100];
    };
}