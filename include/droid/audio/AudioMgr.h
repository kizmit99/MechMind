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
#include "droid/core/BaseComponent.h"
#include "droid/audio/AudioDriver.h"
#include "droid/core/InstructionList.h"

namespace droid::audio {
    class AudioMgr : public droid::core::BaseComponent {
    public:
        AudioMgr(const char* name, droid::core::System* system, AudioDriver* driver);

        //Override virtual methods from BaseComponent
        void init() override;
        void factoryReset() override;
        void task() override;
        void logConfig() override;
        void failsafe() override;

        void setMaxVolume(float maxVolume);
        float getMaxVolume();
        void setMinVolume(float minVolume);
        float getMinVolume();
        void setVolume(float newVolume);
        float getVolume();
        void playSound(uint8_t bank, uint8_t sound);
        void stop();
        void enableRandom(bool enable, uint8_t secondsInFuture = 0);
        bool isRandomEnabled();
        void setRandomMinMs(uint32_t millis);
        uint32_t getRandomMinMs();
        void setRandomMaxMs(uint32_t millis);
        uint32_t getRandomMaxMs();

    private:
        AudioDriver* driver;
        droid::core::InstructionList audioCmdList;
        char cmdBuffer[INSTRUCTIONLIST_COMMAND_LEN] = {0};
        float maxVolume;
        float minVolume;
        float volume;
        bool randomPlayEnabled;
        uint32_t nextRandomTime = 0;
        uint32_t lastScheduledCmd = 0;
        int minRandomMilliSeconds;
        int maxRandomMilliSeconds;
        int cmdStaggerMs = 0;

        void queueCommand(const char* command, unsigned long delayMs = 0);
        void randomPlay();
    };
}