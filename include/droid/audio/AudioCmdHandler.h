/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 \* For more information, visit https://github\.com/kizmit99/DroidBrain
 \*/


#pragma once
#include "droid/command/CmdHandler.h"
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