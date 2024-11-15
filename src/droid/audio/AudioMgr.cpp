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
    AudioMgr::AudioMgr(const char* name, droid::services::System* system, AudioDriver* driver) :
        name(name),
        logger(system->getLogger()),
        config(system->getConfig()),
        droidState(system->getDroidState()),
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
        AudioCmd* instruction = audioCmdList.initLoop();
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

                instruction = audioCmdList.deleteCommand(instruction);
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
        char buf[AUDIO_MAX_COMMAND_LEN];
        queueCommand(driver->getSetVolumeCmd(buf, AUDIO_MAX_COMMAND_LEN, newVolume));
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
        char buf[AUDIO_MAX_COMMAND_LEN];
        const char* cmd = driver->getSetMuseMinCmd(buf, AUDIO_MAX_COMMAND_LEN, millis);
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
        char buf[AUDIO_MAX_COMMAND_LEN];
        const char* cmd = driver->getSetMuseMaxCmd(buf, AUDIO_MAX_COMMAND_LEN, millis);
        if (cmd != NULL) {
            queueCommand(cmd);
        }
    }

    uint32_t AudioMgr::getRandomMaxMs() {
        return this->maxRandomMilliSeconds;
    }
    
    void AudioMgr::playSound(uint8_t bank, uint8_t sound) {
        logger->log(name, DEBUG, "playSound(%d, %d)\n", bank, sound);
        char buf[AUDIO_MAX_COMMAND_LEN];
        queueCommand(driver->getPlaySoundCmd(buf, AUDIO_MAX_COMMAND_LEN, bank, sound));
    }
    
    void AudioMgr::stop() {
        logger->log(name, DEBUG, "stop()\n");
        char buf[AUDIO_MAX_COMMAND_LEN];
        queueCommand(driver->getStopCmd(buf, AUDIO_MAX_COMMAND_LEN));
    }
    
    void AudioMgr::enableRandom(bool enable, uint8_t secondsInFuture) {
        logger->log(name, DEBUG, "enableRandom(%d, %d)\n", enable, secondsInFuture);
        char buf[AUDIO_MAX_COMMAND_LEN];
        queueCommand(driver->getEnableRandomCmd(buf, AUDIO_MAX_COMMAND_LEN, enable), (((int) secondsInFuture) * 1000));
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
        AudioCmd* newAudioCmd = audioCmdList.addCommand();
        strncpy(newAudioCmd->command, command, AUDIO_MAX_COMMAND_LEN);
        newAudioCmd->executeTime = millis() + delayMs;
    }

    AudioCmd* AudioCmdList::addCommand() {
        AudioCmd* freeRec = NULL;
        uint8_t index = 0;
        for (index = 0; index < AUDIO_COMMAND_QUEUE_SIZE; index++) {
            if (!list[index].isActive) {
                freeRec = &list[index];
                break;
            }
        }
        if (freeRec == NULL) {  //List is full
            return NULL;
        }
        //link prev record to this one
        if (tail != NULL) {
            tail->next = freeRec;
        }
        //Prepare record for reuse
        freeRec->isActive = true;
        freeRec->prev = tail;       //Always add to end of list
        freeRec->next = NULL;
        tail = freeRec;
        if (head == NULL) {   //List was empty
            head = freeRec;
        }
        return freeRec;
    }
    
    AudioCmd* AudioCmdList::deleteCommand(AudioCmd* entry) {
        if (entry == NULL) {
            return NULL;
        }
        if (head == NULL) {
            return NULL;
        }
        if (entry->prev == NULL) {   //Was first entry in list
            head = entry->next;
        } else {
            entry->prev->next = entry->next;
        }
        if (tail == entry) {    //Was last entry in list
            tail = entry->prev;
        }
        AudioCmd* next = entry->next;
        if (next != NULL) {
            next->prev = entry->prev;
        }

        //Clear record for reuse
        entry->command[0] = 0;
        entry->executeTime = 0;
        entry->isActive = false;
        entry->next = NULL;
        entry->prev = NULL;

        return next;
    }

    AudioCmd* AudioCmdList::initLoop() {
        return head;
    }
    
    AudioCmd* AudioCmdList::getNext(AudioCmd* entry) {
        if (entry == NULL) {
            return NULL;
        }
        return entry->next;
    }
    
    void AudioCmdList::dump(const char *name, droid::services::Logger* logger) {
        int i = 0;
        AudioCmd* cmd = head;
        while (cmd != NULL) {
            logger->log(name, INFO, "AudioCmdList[%d] cmd: %s\n", i, cmd->command);
            i++;
            cmd = cmd->next;
        }
    }
}