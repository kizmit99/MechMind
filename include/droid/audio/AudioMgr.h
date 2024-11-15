#pragma once
#include <Arduino.h>
#include "droid/services/System.h"
#include "droid/audio/AudioDriver.h"

#define AUDIO_MAX_COMMAND_LEN 25
#define AUDIO_COMMAND_QUEUE_SIZE 20

namespace droid::audio {
    struct AudioCmd {
        char command[AUDIO_MAX_COMMAND_LEN];
        unsigned long executeTime; // Time when the instruction should be executed
        bool isActive;
        AudioCmd* next;
        AudioCmd* prev;
    };

    class AudioCmdList {
    public:
        AudioCmd* addCommand();
        AudioCmd* deleteCommand(AudioCmd*);  //Note, this method returns the NEXT Instruction* in the list
        AudioCmd* initLoop();
        AudioCmd* getNext(AudioCmd*);
        void dump(const char *name, droid::services::Logger* logger);

    private:
        AudioCmd list[AUDIO_COMMAND_QUEUE_SIZE];
        AudioCmd* head = 0;
        AudioCmd* tail = 0;
    };

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
        AudioCmdList audioCmdList;
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