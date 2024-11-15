#pragma once
#include <Arduino.h>
#include "droid/services/System.h"
#include "droid/audio/AudioDriver.h"
#include "droid/util/InstructionList.h"

namespace droid::audio {
    class AudioMgr {
    public:
        AudioMgr(const char* name, droid::services::System* system, AudioDriver* driver);
        void init();
        void task();
        void factoryReset();
        void logConfig();

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
        const char* name;
        droid::services::Logger* logger;
        droid::services::Config* config;
        droid::services::DroidState* droidState;
        AudioDriver* driver;
        droid::util::InstructionList audioCmdList;
        char cmdBuffer[INSTRUCTIONLIST_COMMAND_LEN] = {0};
        float maxVolume;
        float minVolume;
        float volume;
        bool randomPlayEnabled;
        uint32_t nextRandomTime = 0;
        int minRandomMilliSeconds;
        int maxRandomMilliSeconds;

        void queueCommand(const char* command, unsigned long delayMs = 0);
        void randomPlay();
    };
}