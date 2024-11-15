#include "droid/audio/SparkDriver.h"
#include "droid/audio/AudioMgr.h"

#define SPARK_MIN_VOLUME 82
#define SPARK_EMPTY_SOUND 252
#define SPARK_CMD_RANDOM_ON     "RandomOn"
#define SPARK_CMD_RANDOM_OFF    "RandomOff"

namespace droid::audio {
    SparkDriver::SparkDriver(Stream* out) : 
        out(out) {}

    const char* SparkDriver::getPlaySoundCmd(char* cmdBuf, size_t buflen, uint8_t bank, uint8_t sound) {
        uint8_t filenum = decodeFilenum(bank, sound);
        snprintf(cmdBuf, buflen, "t%c", filenum);
        return cmdBuf;
    }

    const char* SparkDriver::getSetVolumeCmd(char* cmdBuf, size_t buflen, float newVolume) {
        uint8_t volume = SPARK_MIN_VOLUME - (newVolume * SPARK_MIN_VOLUME);
        snprintf(cmdBuf, buflen, "v%c", volume);
        return cmdBuf;
    }

    const char* SparkDriver::getStopCmd(char* cmdBuf, size_t buflen) {
        return getPlaySoundCmd(cmdBuf, buflen, 0, SPARK_EMPTY_SOUND);
    }

    const char* SparkDriver::getEnableRandomCmd(char* cmdBuf, size_t buflen, bool enable) {
        if (enable) {
            return SPARK_CMD_RANDOM_ON;
        } else {
            return SPARK_CMD_RANDOM_OFF;
        }
    }

    bool SparkDriver::executeCmd(const char* deviceCmd) {
        out->print(deviceCmd);
        return true;
    }
}