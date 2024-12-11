/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/MechMind
 */

#include "droid/audio/DFMiniDriver.h"

#define DFMINI_POWER_ON_DELAY 10000
#define DFMINI_VOLUME_MAX 30
#define DFMINI_EQ_NORMAL 0
#define DFMINI_CMD_RANDOM_ON     "RandomOn"
#define DFMINI_CMD_RANDOM_OFF    "RandomOff"


namespace droid::audio {
    DFMiniDriver::DFMiniDriver(const char* name, droid::core::System* system, Stream* out) : 
        AudioDriver(name, system),
        out(out) {
        powerOnTime = millis();
    }

    void DFMiniDriver::sendMsg(uint8_t command, uint8_t parm1, uint8_t parm2) {
        char message[11];
        message[0] = 0x7e;
        message[1] = 0xff;
        message[2] = 0x06;
        message[3] = command;
        message[4] = 0x00;
        message[5] = parm1;
        message[6] = parm2;
        uint16_t checksum = 0;
        for (int i = 1; i < 7; i++) {
            checksum += message[i];
        }
        checksum = -checksum;
        message[7] = (checksum >> 8) & 0xff;
        message[8] = checksum & 0xff;
        message[9] = 0xef;

        out->write(message, 10);
    }

    void DFMiniDriver::init() {
        if (millis() > powerOnTime + DFMINI_POWER_ON_DELAY) {
            logger->log(name, DEBUG, "Attempting to reset DFPlayer\n");
            waiting = false;
            sendMsg(0x0c);
        } else {
            logger->log(name, DEBUG, "Skipping DFPlayer init, waiting for power on delay\n");
        }
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
        if (waiting) {
            init();
        }
        if (waiting) {
            logger->log(name, DEBUG, "DFPlayer.executeCmd skipping because not initialized\n");
            return false;
        }
        switch (deviceCmd[0]) {
            case 't':
                logger->log(name, DEBUG, "DFPlayer.playMp3Folder(%d)\n", deviceCmd[1]);
                sendMsg(0x12, 0x00, deviceCmd[1]);
                break;

            case 'v':
                logger->log(name, DEBUG, "DFPlayer.volume(%d)\n", deviceCmd[1]);
                sendMsg(0x06, 0x00, deviceCmd[1]);
                break;

            case 's':
                logger->log(name, DEBUG, "DFPlayer.stop()\n");
                sendMsg(0x16);
                break;

            default:
                logger->log(name, DEBUG, "DFPlayer invalid command: %s\n", deviceCmd);
                return false;
        }
        return true;
    }
}