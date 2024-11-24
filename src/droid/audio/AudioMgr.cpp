/*
 * Droid Brain Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#include "droid/audio/AudioMgr.h"

#define CONFIG_KEY_MAX_VOLUME       "MaxVolume"
#define CONFIG_KEY_MIN_VOLUME       "MinVolume"
#define CONFIG_KEY_VOLUME           "Volume"
#define CONFIG_KEY_RANDOM_ENABLED   "RandomOn"
#define CONFIG_KEY_RANDOM_MIN       "MinRandomMs"
#define CONFIG_KEY_RANDOM_MAX       "MaxRandomMs"

#define CONFIG_DEFAULT_MAX_VOLUME       1.0
#define CONFIG_DEFAULT_MIN_VOLUME       0.0
#define CONFIG_DEFAULT_VOLUME           0.5
#define CONFIG_DEFAULT_RANDOM_ENABLED   1
#define CONFIG_DEFAULT_RANDOM_MIN       1000
#define CONFIG_DEFAULT_RANDOM_MAX       10000

#define ONE_HOUR_IN_MS 3600000

#define AUDIO_CMD_RANDOM_ON     "RandomOn"
#define AUDIO_CMD_RANDOM_OFF    "RandomOff"

namespace droid::audio {
    AudioMgr::AudioMgr(const char* name, droid::core::System* system, AudioDriver* driver) :
        BaseComponent(name, system),
        driver(driver) {}

    void AudioMgr::init() {
        this->volume = config->getFloat(name, CONFIG_KEY_VOLUME, CONFIG_DEFAULT_VOLUME);
        this->maxVolume = config->getFloat(name, CONFIG_KEY_MAX_VOLUME, CONFIG_DEFAULT_MAX_VOLUME);
        this->minVolume = config->getFloat(name, CONFIG_KEY_MIN_VOLUME, CONFIG_DEFAULT_MIN_VOLUME);
        bool enableRandomPlay = config->getBool(name, CONFIG_KEY_RANDOM_ENABLED, CONFIG_DEFAULT_RANDOM_ENABLED);
        this->minRandomMilliSeconds = config->getInt(name, CONFIG_KEY_RANDOM_MIN, CONFIG_DEFAULT_RANDOM_MIN);
        this->maxRandomMilliSeconds = config->getInt(name, CONFIG_KEY_RANDOM_MAX, CONFIG_DEFAULT_RANDOM_MAX);

        enableRandom(enableRandomPlay);
    }
    
    void AudioMgr::task() {
        unsigned long currentTime = millis();
        droid::core::Instruction* instruction = audioCmdList.initLoop();
        while (instruction != NULL) {
            if (currentTime >= instruction->executeTime) {

                logger->log(name, DEBUG, "Executing AudioCmd: %s at time: %d\n", instruction->command, currentTime);

                if (strncasecmp(AUDIO_CMD_RANDOM_ON, instruction->command, sizeof(AUDIO_CMD_RANDOM_ON)) == 0) {
                    randomPlayEnabled = true;
                } else if (strncasecmp(AUDIO_CMD_RANDOM_OFF, instruction->command, sizeof(AUDIO_CMD_RANDOM_OFF)) == 0) {
                    randomPlayEnabled = false;
                } else {
                    bool processed = driver->executeCmd(instruction->command);
                    if (!processed) {
                        logger->log(name, WARN, "AudioCmd was not handled: %s\n", instruction->command);
                    }
                }

                instruction = audioCmdList.deleteInstruction(instruction);
            } else {
                instruction = instruction->next;
            }
        }

        randomPlay();
    }
    
    void AudioMgr::factoryReset() {
        config->clear(name);
        config->putFloat(name, CONFIG_KEY_VOLUME, CONFIG_DEFAULT_VOLUME);
        config->putFloat(name, CONFIG_KEY_MAX_VOLUME, CONFIG_DEFAULT_MAX_VOLUME);
        config->putFloat(name, CONFIG_KEY_MIN_VOLUME, CONFIG_DEFAULT_MIN_VOLUME);
        config->putBool(name, CONFIG_KEY_RANDOM_ENABLED, CONFIG_DEFAULT_RANDOM_ENABLED);
        config->putInt(name, CONFIG_KEY_RANDOM_MIN, CONFIG_DEFAULT_RANDOM_MIN);
        config->putInt(name, CONFIG_KEY_RANDOM_MAX, CONFIG_DEFAULT_RANDOM_MAX);
    }
    
    void AudioMgr::logConfig() {
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_VOLUME, config->getString(name, CONFIG_KEY_VOLUME, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_MAX_VOLUME, config->getString(name, CONFIG_KEY_MAX_VOLUME, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_MIN_VOLUME, config->getString(name, CONFIG_KEY_MIN_VOLUME, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_RANDOM_ENABLED, config->getString(name, CONFIG_KEY_RANDOM_ENABLED, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_RANDOM_MIN, config->getString(name, CONFIG_KEY_RANDOM_MIN, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_RANDOM_MAX, config->getString(name, CONFIG_KEY_RANDOM_MAX, "").c_str());
    }

    void AudioMgr::failsafe() {
        stop();
    }
    
    void AudioMgr::setMaxVolume(float maxVolume) {
        if (maxVolume < 0) {
            maxVolume = 0.0;
        }
        if (maxVolume > 1.0) {
            maxVolume = 1.0;
        }
        config->putFloat(name, CONFIG_KEY_MAX_VOLUME, maxVolume);
        this->maxVolume = maxVolume;
        if (volume > maxVolume) {
            setVolume(maxVolume);
        }
    }
    
    float AudioMgr::getMaxVolume() {
        return maxVolume;
    }
    
    void AudioMgr::setMinVolume(float minVolume) {
        if (minVolume < 0) {
            minVolume = 0.0;
        }
        if (minVolume > 1.0) {
            minVolume = 1.0;
        }
        config->putFloat(name, CONFIG_KEY_MIN_VOLUME, minVolume);
        if (volume < minVolume) {
            setVolume(minVolume);
        }
    }
    
    float AudioMgr::getMinVolume() {
        return minVolume;
    }
    
    void AudioMgr::setVolume(float newVolume) {
        logger->log(name, DEBUG, "setVolume(%f)\n", newVolume);
        if (newVolume < minVolume) {
            newVolume = minVolume;
        }
        if (newVolume > maxVolume) {
            newVolume = maxVolume;
        }
        config->putFloat(name, CONFIG_KEY_VOLUME, newVolume);
        this->volume = newVolume;
        queueCommand(driver->getSetVolumeCmd(cmdBuffer, INSTRUCTIONLIST_COMMAND_LEN, newVolume));
    }
    
    float AudioMgr::getVolume() {
        return this->volume;
    }

    void AudioMgr::setRandomMinMs(uint32_t millis){
        if (millis < 1000) {     // Arbitrarily cap to between 1 second and 1 hour
            millis = 1000;
        }
        if (millis > ONE_HOUR_IN_MS) {
            millis = ONE_HOUR_IN_MS;
        }
        config->putInt(name, CONFIG_KEY_RANDOM_MIN, millis);
        this->minRandomMilliSeconds = millis;
        const char* cmd = driver->getSetMuseMinCmd(cmdBuffer, INSTRUCTIONLIST_COMMAND_LEN, millis);
        if (cmd != NULL) {
            queueCommand(cmd);
        }
    }

    uint32_t AudioMgr::getRandomMinMs() {
        return this->minRandomMilliSeconds;
    }

    void AudioMgr::setRandomMaxMs(uint32_t millis) {
        if (millis < 1000) {     // Arbitrarily cap to between 1 second and 1 hour
            millis = 1000;
        }
        if (millis > ONE_HOUR_IN_MS) {
            millis = ONE_HOUR_IN_MS;
        }
        config->putInt(name, CONFIG_KEY_RANDOM_MIN, millis);
        this->maxRandomMilliSeconds = millis;
        const char* cmd = driver->getSetMuseMaxCmd(cmdBuffer, INSTRUCTIONLIST_COMMAND_LEN, millis);
        if (cmd != NULL) {
            queueCommand(cmd);
        }
    }

    uint32_t AudioMgr::getRandomMaxMs() {
        return this->maxRandomMilliSeconds;
    }
    
    void AudioMgr::playSound(uint8_t bank, uint8_t sound) {
        logger->log(name, DEBUG, "playSound(%d, %d)\n", bank, sound);
        queueCommand(driver->getPlaySoundCmd(cmdBuffer, INSTRUCTIONLIST_COMMAND_LEN, bank, sound));
    }
    
    void AudioMgr::stop() {
        logger->log(name, DEBUG, "stop()\n");
        queueCommand(driver->getStopCmd(cmdBuffer, INSTRUCTIONLIST_COMMAND_LEN));
    }
    
    void AudioMgr::enableRandom(bool enable, uint8_t secondsInFuture) {
        logger->log(name, DEBUG, "enableRandom(%d, %d)\n", enable, secondsInFuture);
        queueCommand(driver->getEnableRandomCmd(cmdBuffer, INSTRUCTIONLIST_COMMAND_LEN, enable), (((int) secondsInFuture) * 1000));
    }

    bool AudioMgr::isRandomEnabled() {
        return randomPlayEnabled;
    }

    void AudioMgr::randomPlay() {
        if (randomPlayEnabled) {
            unsigned long now = millis();
            if (now > nextRandomTime) {
                nextRandomTime = now + ((random() % (maxRandomMilliSeconds - minRandomMilliSeconds + 1)) + minRandomMilliSeconds);

                uint8_t bank = random(1, 5);    // Plays a random sound from the first 5 banks only
                uint8_t sound = random(1, driver->maxSounds[bank]);
                playSound(bank, sound);
            }
        }
    }
    
    void AudioMgr::queueCommand(const char* command, unsigned long delayMs) {
        logger->log(name, DEBUG, "queueCommand(%s, %d)\n", command, delayMs);
        droid::core::Instruction* newAudioCmd = audioCmdList.addInstruction();
        if (newAudioCmd == NULL) {
            logger->log(name, WARN, "Command Queue is Full.  Dropping command: %s\n", command);
            audioCmdList.dump(name, logger, WARN);
            return;
        }
        strncpy(newAudioCmd->command, command, INSTRUCTIONLIST_COMMAND_LEN);
        newAudioCmd->executeTime = millis() + delayMs;
    }
}