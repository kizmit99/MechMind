#pragma once
#include "droid/command/CmdHandler.h"
#include "droid/core/PassiveComponent.h"
#include "droid/audio/AudioMgr.h"

namespace droid::audio {
    class AudioCmdHandler : public droid::command::CmdHandler {
    public:
        AudioCmdHandler(const char* name, droid::core::System* system, AudioMgr* audioMgr);
        bool process(const char* device, const char* command);

    private:
        AudioMgr* audioMgr;

        bool parseCmd(const char* command);
    };
}