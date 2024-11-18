/////////////COMMAND VOCABULARY///////////
// Play sound command by bank/sound numbers
// $xyy
// x=bank number
// yy=sound number. If none, next sound is played in the bank
//
// Other commands
// $c
// where c is a command character
// R - random from 4 first banks
// O - sound off
// L - Leia message (bank 7 sound 1)
// C - Cantina music (bank 9 sound 5)
// c - Beep cantina (bank 9 sound 1)
// S - Scream (bank 6 sound 1)
// F - Faint/Short Circuit (bank 6 sound 3)
// D - Disco (bank 9 sound 6)
// s - stop sounds
// + - volume up
// - - volume down
// m - volume mid
// f - volume max
// p - volume min
// W - Star Wars music (bank 9 sound 2)
// M - Imperial March (bank 9 sound 3)
//
///////////////////////////////////////////////

#include "droid/audio/AudioCmdHandler.h"

namespace droid::audio {
    AudioCmdHandler::AudioCmdHandler(const char* name, droid::core::System* system, AudioMgr* audioMgr) :
        CmdHandler(name, system),
        audioMgr(audioMgr) {}
    
    bool AudioCmdHandler::process(const char* device, const char* command) {
        if ((strcasecmp(name, device) != 0) ||
            (command == NULL)) {
            return false;
        }

        logger->log(name, DEBUG, "AudioCmdHandler asked to process cmd: %s\n", command);
        return parseCmd(command);
    }

    bool AudioCmdHandler::parseCmd(const char* command) {
        if (command[0] != '$') {
            return false;
        }
        // if the command character is a digit, this should be a play sound command
        if (isdigit(command[1])) {
            uint8_t cmdlen = strnlen(command, 5);
            if ((cmdlen < 2) || (cmdlen > 4)) {
                return false;
            }
            audioMgr->enableRandom(false);
            uint8_t bank = command[2] - '0';
            uint8_t sound = 0;
            if (cmdlen > 2) {
                sound = atoi(&command[2]);
            }
            audioMgr->playSound(bank, sound);
            return true;
        }

        bool randomOn = audioMgr->isRandomEnabled();
        float minVolume = audioMgr->getMinVolume();
        float maxVolume = audioMgr->getMaxVolume();
        float step = 0;

        switch(command[1]) {
            case 'R':   // R - random from 4 first banks
                audioMgr->enableRandom(true);
                break;

            case 'O':   // O - sound off
                audioMgr->enableRandom(false);
                audioMgr->setVolume(0);
                break;

            case 'L':   // L - Leia message (bank 7 sound 1)
                audioMgr->playSound(7, 1);
                if (randomOn) {
                    audioMgr->enableRandom(true, 44);
                }
                break;

            case 'C':   // C - Cantina music (bank 9 sound 5)
                audioMgr->playSound(8, 5);
                if (randomOn) {
                    audioMgr->enableRandom(true, 56);
                }
                break;

            case 'c':   // c - Beep cantina (bank 9 sound 1)
                audioMgr->playSound(8, 1);
                if (randomOn) {
                    audioMgr->enableRandom(true, 27);
                }
                break;

            case 'S':   // S - Scream (bank 6 sound 1)
                audioMgr->playSound(6, 1);
                if (randomOn) {
                    audioMgr->enableRandom(true, 12);
                }
                break;

            case 'F':   // F - Faint/Short Circuit (bank 6 sound 3)
                audioMgr->playSound(6,3);
                if (randomOn) {
                    audioMgr->enableRandom(true, 12);
                }
                break;
            case 'D':   // D - Disco (bank 9 sound 6)
                audioMgr->playSound(8, 6);
                if (randomOn) {
                    audioMgr->enableRandom(true, 40);
                }
                break;

            case 's':   // s - stop sounds
                audioMgr->enableRandom(false);
                audioMgr->stop();
                break;

            case '+':   // + - volume up
                step = (maxVolume - minVolume) / 10.0;
                audioMgr->setVolume(audioMgr->getVolume() + step);
                break;

            case '-':   // - - volume down
                step = (maxVolume - minVolume) / 10.0;
                audioMgr->setVolume(audioMgr->getVolume() - step);
                break;

            case 'm':   // m - volume mid
                step = (maxVolume - minVolume) / 2.0;
                audioMgr->setVolume(minVolume + step);
                break;

            case 'f':   // f - volume max
                audioMgr->setVolume(maxVolume);
                break;

            case 'p':   // p - volume min
                audioMgr->setVolume(minVolume);
                break;

            case 'W':   // W - Star Wars music (bank 9 sound 2)
                audioMgr->enableRandom(false);
                audioMgr->playSound(8, 2);
                break;

            case 'M':   // M - Imperial March (bank 9 sound 3)
                audioMgr->enableRandom(false);
                audioMgr->playSound(8, 3);
                break;

            default:
                return false;
        }
        return true;
    }
}
