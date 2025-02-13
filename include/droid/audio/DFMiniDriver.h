/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/MechMind
 */

#pragma once
#include "droid/audio/AudioDriver.h"

namespace droid::audio {
    class DFMiniDriver : public AudioDriver {
    public:
        DFMiniDriver(const char* name, droid::core::System* system, Stream* out);
        const char* getPlaySoundCmd(char* cmdBuf, size_t buflen, uint8_t bank, uint8_t sound);
        const char* getSetVolumeCmd(char* cmdBuf, size_t buflen, float newVolume);
        const char* getStopCmd(char* cmdBuf, size_t buflen);
        const char* getEnableRandomCmd(char* cmdBuf, size_t buflen, bool enable);
        bool executeCmd(const char* deviceCmd);

        void init() override;

    private:
        Stream* out = nullptr;
        ulong powerOnTime = 0;
        bool waiting = true;

        void sendMsg(uint8_t command, uint8_t parm1 = 0, uint8_t parm2 = 0);
    };
}