#pragma once
#include <Arduino.h>
#include "droid/command/CmdHandler.h"
#include "droid/services/PassiveComponent.h"
#include "droid/audio/AudioMgr.h"

namespace droid::audio {
    class AudioCmdHandler : public droid::command::CmdHandler {
    public:
        AudioCmdHandler(const char* name, droid::services::System* system, AudioMgr* audioMgr);
        bool process(const char* device, const char* command);

    private:
        AudioMgr* audioMgr;

        bool parseCmd(const char* command);
    };
}