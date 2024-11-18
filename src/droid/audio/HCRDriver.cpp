#include "droid/audio/HCRDriver.h"

namespace droid::audio {
    HCRDriver::HCRDriver(const char* name, droid::services::System* system, Stream* out) : 
        AudioDriver(name, system),
        out(out) {}

    const char* HCRDriver::getPlaySoundCmd(char* cmdBuf, size_t buflen, uint8_t bank, uint8_t sound) {
        uint8_t filenum = decodeFilenum(bank, sound);
        bool extreme = (sound > (maxSounds[bank - 1] / 2));
        snprintf(cmdBuf, buflen, "<CA%04d>", filenum);
        switch (bank) {
            case SoundKind::GenSounds:
                return "<MM>";

            case SoundKind::ChatSounds:
                return "<MM>";

            case SoundKind::HappySounds:
                if (extreme) {
                    return "<SH1>";
                } else {
                    return "<SH0>";
                }

            case SoundKind::SadSounds:
                if (extreme) {
                    return "<SS1>";
                } else {
                    return "<SS0>";
                }

            case SoundKind::WhistleSounds:
                if (extreme) {
                    return "<SM1>";
                } else {
                    return "<SM0>";
                }

            case SoundKind::ScreamSounds:
                if (sound == 1) {
                    return "<SC0>";
                } else if (sound == 2) {
                    return "<SC1>";
                } else {
                    return "<SE>";
                }
        }
        return cmdBuf;
    }

    const char* HCRDriver::getSetVolumeCmd(char* cmdBuf, size_t buflen, float newVolume) {
        uint8_t volPercent = newVolume * 100.0;
        snprintf(cmdBuf, buflen, "<PVV%d,PVA%d,PVB%d>", volPercent, volPercent, volPercent);
        return cmdBuf;
    }

    const char* HCRDriver::getStopCmd(char* cmdBuf, size_t buflen) {
        return "<PSG>";
    }

    const char* HCRDriver::getEnableRandomCmd(char* cmdBuf, size_t buflen, bool enable) {
        if (enable) {
            return "<M1>";
        } else {
            return "<M0>";
        }
    }

    const char* getSetMuseMinCmd(char* cmdBuf, size_t buflen, uint32_t millis) {
        int seconds = millis / 1000;
        snprintf(cmdBuf, buflen, "<MN%d>", seconds);
        return cmdBuf;
    }

    const char* getSetMuseMaxCmd(char* cmdBuf, size_t buflen, uint32_t millis) {
        int seconds = millis / 1000;
        snprintf(cmdBuf, buflen, "<MX%d>", seconds);
        return cmdBuf;
    }

    bool HCRDriver::executeCmd(const char* deviceCmd) {
        out->print(deviceCmd);
        return true;
    }
}