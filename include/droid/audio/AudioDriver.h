/*
 * Droid Brain Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#pragma once
#include "droid/core/BaseComponent.h"

/***********************************************************
 *  On the Sparkfun MP3, there are a maximum of 255 sound files
 *  They must be named NNN-xxxx.mp3
 *  Where NNN = 001 to 255
 *  The numbering ranges are predetermined, 25 sounds per
 *  bank category
 *       Bank 1: gen sounds, numbered 001 to 025
 *       Bank 2: chat sounds, numbered 026 to 050
 *       Bank 3: happy sounds, numbered 051 to 075
 *       Bank 4: sad sounds, numbered 076 to 100
 *       Bank 5: whistle sounds, numbered 101 to 125
 *       Bank 6: scream sounds, numbered 126 to 150
 *       Bank 7: Leia sounds, numbered 151 to 175
 *       Bank 8: sing sounds (deprecated, not used by R2 Touch)
 *       Bank 9: mus sounds, numbered 176 to 181
 *
 *  The pre-made R2 sound library contains only a few non-copyrighted music sounds.
 *  Sound 202, 203 and 205 are "beeped music" place-holders, meant to be replaced with the
 *  original score of Star Wars, Empire March, and Cantina respectively
 *
 *  If you add your own sounds in Bank 9, make sure you update the last variable
 *  MP3_BANK9_SOUNDS below to reflect the total number of sounds
 *
 ***********************************************************/

#define MP3_MAX_BANKS                9  // nine banks
#define MP3_MAX_SOUNDS_PER_BANK     25  // no more than 25 sound in each
#define MP3_BANK_CUTOFF             4   // cutoff for banks that play "next" sound on $x

// for the random sounds, needs to know max sounds of first 5 banks
// only important for sounds below cutoff
#define MP3_BANK1_SOUND_COUNT 19 // gen sounds, numbered 001 to 025
#define MP3_BANK2_SOUND_COUNT 18 // chat sounds, numbered 026 to 050
#define MP3_BANK3_SOUND_COUNT 7  // happy sounds, numbered 051 to 075
#define MP3_BANK4_SOUND_COUNT 4  // sad sounds, numbered 076 to 100
#define MP3_BANK5_SOUND_COUNT 3  // whistle sounds, numbered 101 to 125
#define MP3_BANK6_SOUND_COUNT MP3_MAX_SOUNDS_PER_BANK    // scream sounds, numbered 126 to 150
#define MP3_BANK7_SOUND_COUNT MP3_MAX_SOUNDS_PER_BANK    // Leia sounds, numbered 151 to 175
#define MP3_BANK8_SOUND_COUNT MP3_MAX_SOUNDS_PER_BANK    // sing sounds (deprecated, not used by R2 Touch)
#define MP3_BANK9_SOUND_COUNT MP3_MAX_SOUNDS_PER_BANK // mus sounds, numbered 201 t0 225

namespace droid::audio {
    class AudioDriver : public droid::core::BaseComponent {
    public:
        AudioDriver(const char* name, droid::core::System* system) :
            BaseComponent(name, system) {}
            
        //Virtual methods required by BaseComponent declared here as NOOPs for concrete sub-classes
        void init() {}
        void factoryReset() {}
        void task() {}
        void logConfig() {}
        void failsafe() {}

        virtual const char* getPlaySoundCmd(char* cmdBuf, size_t buflen, uint8_t bank, uint8_t sound) = 0;
        virtual const char* getSetVolumeCmd(char* cmdBuf, size_t buflen, float newVolume) = 0;
        virtual const char* getStopCmd(char* cmdBuf, size_t buflen) = 0;
        virtual const char* getEnableRandomCmd(char* cmdBuf, size_t buflen, bool enable) = 0;
        const char* getSetMuseMinCmd(char* cmdBuf, size_t buflen, uint32_t millis) {return NULL;}
        const char* getSetMuseMaxCmd(char* cmdBuf, size_t buflen, uint32_t millis) {return NULL;}
        virtual bool executeCmd(const char* deviceCmd) = 0;

        const uint8_t maxSounds[MP3_MAX_BANKS] = {
            MP3_BANK1_SOUND_COUNT,
            MP3_BANK2_SOUND_COUNT,
            MP3_BANK3_SOUND_COUNT,
            MP3_BANK4_SOUND_COUNT,
            MP3_BANK5_SOUND_COUNT,
            MP3_BANK6_SOUND_COUNT,
            MP3_BANK7_SOUND_COUNT,
            MP3_BANK8_SOUND_COUNT,
            MP3_BANK9_SOUND_COUNT};

    protected:
        uint8_t decodeFilenum(uint8_t bank, uint8_t sound) {
            if (bank == 0) {
                return sound;
            }
            uint8_t filenum = 0;
            uint8_t bankIndex = bank - 1;
            if (sound > 25) {
                sound = 0;
            }
            if (sound == 0) {
                if (bank <= MP3_BANK_CUTOFF) {
                    lastPlayed[bankIndex]++;
                    if (lastPlayed[bankIndex] > maxSounds[bankIndex]) {
                        lastPlayed[bankIndex] = 1;
                    }
                } else {
                    lastPlayed[bankIndex] = 1;
                }
                return (bankIndex * MP3_MAX_SOUNDS_PER_BANK) + lastPlayed[bankIndex];
            } else {
                if (sound > maxSounds[bankIndex]) {
                    sound = maxSounds[bankIndex];
                }
                lastPlayed[bankIndex] = sound;
                return (bankIndex * MP3_MAX_SOUNDS_PER_BANK) + sound;
            }
        }

        uint8_t lastPlayed[MP3_MAX_BANKS] = {0};

        enum SoundKind {
            GenSounds = 1,
            ChatSounds = 2,
            HappySounds = 3,
            SadSounds = 4,
            WhistleSounds = 5,
            ScreamSounds = 6,
            LeiaSounds = 7,
            SingSounds = 8,
            MusicSounds = 9};
    };
}