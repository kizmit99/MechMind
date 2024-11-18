#pragma once
#include "CmdHandler.h"

namespace droid::command {
    class CmdLogger : public CmdHandler {
    public:
        CmdLogger(const char* name, droid::core::System* sys);
        bool process(const char* device, const char* command);

    private:
        Stream* stream;
    };
}