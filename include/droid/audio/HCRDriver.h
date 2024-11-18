#pragma once
#include "droid/audio/AudioDriver.h"

namespace droid::audio {
    class HCRDriver : public AudioDriver {
    public:
        HCRDriver(const char* name, droid::core::System* system, Stream* out);
        const char* getPlaySoundCmd(char* cmdBuf, size_t buflen, uint8_t bank, uint8_t sound);
        const char* getSetVolumeCmd(char* cmdBuf, size_t buflen, float newVolume);
        const char* getStopCmd(char* cmdBuf, size_t buflen);
        const char* getEnableRandomCmd(char* cmdBuf, size_t buflen, bool enable);
        const char* getSetMuseMinCmd(char* cmdBuf, size_t buflen, uint32_t millis);
        const char* getSetMuseMaxCmd(char* cmdBuf, size_t buflen, uint32_t millis);
        bool executeCmd(const char* deviceCmd);

    private:
        Stream* out;
    };
}