#pragma once
#include <Arduino.h>

namespace droid::command {
    class CmdHandler {
    public:
        virtual bool process(const char* device, const char* command) = 0;
    };
}