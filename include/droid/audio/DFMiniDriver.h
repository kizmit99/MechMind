#pragma once
#include <DFRobotDFPlayerMini.h>
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

    private:
        Stream* out;
        DFRobotDFPlayerMini dfPlayer;
        bool initSuccess = false;
    };
}