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
    class StubAudioDriver : public AudioDriver {
    public:
        StubAudioDriver(const char* name, droid::core::System* system) :
            AudioDriver(name, system) {}

        const char* getPlaySoundCmd(char* cmdBuf, size_t buflen, uint8_t bank, uint8_t sound) {return "";}
        const char* getSetVolumeCmd(char* cmdBuf, size_t buflen, float newVolume) {return "";}
        const char* getStopCmd(char* cmdBuf, size_t buflen) {return "";}
        const char* getEnableRandomCmd(char* cmdBuf, size_t buflen, bool enable) {return "";}
        const char* getSetMuseMinCmd(char* cmdBuf, size_t buflen, uint32_t millis) {return "";}
        const char* getSetMuseMaxCmd(char* cmdBuf, size_t buflen, uint32_t millis) {return "";}
        bool executeCmd(const char* deviceCmd) {return true;}
    };
}