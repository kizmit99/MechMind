/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#include "droid/audio/DFMiniDriver.h"

#define DFMINI_VOLUME_MAX 30
#define DFMINI_EQ_NORMAL 0
#define DFMINI_CMD_RANDOM_ON     "RandomOn"
#define DFMINI_CMD_RANDOM_OFF    "RandomOff"


namespace droid::audio {
    DFMiniDriver::DFMiniDriver(const char* name, droid::core::System* system, Stream* out) : 
        AudioDriver(name, system),
        out(out) {
        initSuccess = dfPlayer.begin(*out, false, true);
        dfPlayer.EQ(DFMINI_EQ_NORMAL);
    }

    const char* DFMiniDriver::getPlaySoundCmd(char* cmdBuf, size_t buflen, uint8_t bank, uint8_t sound) {
        uint8_t filenum = decodeFilenum(bank, sound);
        snprintf(cmdBuf, buflen, "t%c", filenum);
        return cmdBuf;
    }

    const char* DFMiniDriver::getSetVolumeCmd(char* cmdBuf, size_t buflen, float newVolume) {
        uint8_t volume = newVolume * DFMINI_VOLUME_MAX;
        snprintf(cmdBuf, buflen, "v%c", volume);
        return cmdBuf;
    }

    const char* DFMiniDriver::getStopCmd(char* cmdBuf, size_t buflen) {
        return "s";
    }

    const char* DFMiniDriver::getEnableRandomCmd(char* cmdBuf, size_t buflen, bool enable) {
        if (enable) {
            return DFMINI_CMD_RANDOM_ON;
        } else {
            return DFMINI_CMD_RANDOM_OFF;
        }
    }
    bool DFMiniDriver::executeCmd(const char* deviceCmd) {
        if (!initSuccess) {
            return false;
        }
        switch (deviceCmd[0]) {
            case 't':
                dfPlayer.play(deviceCmd[1]);
                break;

            case 'v':
                dfPlayer.volume(deviceCmd[1]);
                break;

            case 's':
                dfPlayer.stop();
                break;

            default:
                return false;
        }
        return true;
    }
}