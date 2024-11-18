#include <Arduino.h>
#include "droid/brain/Brain.h"
#include "droid/command/StreamCmdHandler.h"
#include "droid/command/CmdLogger.h"
#include "droid/brain/LocalCmdHandler.h"
#include "droid/audio/AudioCmdHandler.h"
#include "droid/services/ActiveComponent.h"

#define CONFIG_KEY_BRAIN_INITIALIZED     "Initialized"
#define CONFIG_KEY_BRAIN_STICK_ENABLE    "StartDriveOn"
#define CONFIG_KEY_BRAIN_TURBO_ENABLE    "StartTurboOn"
#define CONFIG_KEY_BRAIN_AUTODOME_ENABLE "StartDomeOn"

#define CONFIG_DEFAULT_BRAIN_INITIALIZED     true
#define CONFIG_DEFAULT_BRAIN_STICK_ENABLE    true
#define CONFIG_DEFAULT_BRAIN_TURBO_ENABLE    false
#define CONFIG_DEFAULT_BRAIN_AUTODOME_ENABLE false

namespace droid::brain {
    Brain::Brain(const char* name, droid::services::System* system) : 
        ActiveComponent(name, system),
        system(system),
#ifdef BUILD_FOR_DEBUGGER
        pwmService("StubPWM", system),
        controller("StubCtrl", system),
#else
        pwmService("PCA9685", system, PCA9685_I2C_ADDRESS, PCA9685_OUTPUT_ENABLE_PIN),
        controller("DualSony", system),
#endif
        domeMotorDriver("DRV8871", system, PWMSERVICE_DOME_MOTOR_OUT1, PWMSERVICE_DOME_MOTOR_OUT2),
        domeMgr("DomeMgr", system, &controller, &domeMotorDriver),
        actionMgr("ActionMgr", system, &controller),
        audioDriver("HCRDriver", system, &HCR_STREAM),
        audioMgr("AudioMgr", system, &audioDriver),
        driveMotorDriver("Sabertooth", system, (byte) 128, SABERTOOTH_STREAM),
        driveMgr("DriveMgr", system, &controller, &driveMotorDriver) {

        system->setPWMService(&pwmService);  //Necessary to avoid circular reference at initialization

        actionMgr.addCmdHandler(new droid::command::CmdLogger("CmdLogger", system));

        //TODO Implement configurable Serial ports
        actionMgr.addCmdHandler(new droid::command::StreamCmdHandler("Dome", system, &DOME_STREAM));
        actionMgr.addCmdHandler(new droid::command::StreamCmdHandler("Body", system, &BODY_STREAM));
        actionMgr.addCmdHandler(new droid::audio::AudioCmdHandler("HCR", system, &audioMgr));
        actionMgr.addCmdHandler(new droid::brain::LocalCmdHandler("Brain", system, this));

        //Setup list of all ActiveComponents
        componentList.push_back(&pwmService);
        componentList.push_back(&controller);
        componentList.push_back(&domeMotorDriver);
        componentList.push_back(&driveMotorDriver);
        componentList.push_back(&domeMgr);
        componentList.push_back(&driveMgr);
        componentList.push_back(&audioMgr);
        componentList.push_back(&actionMgr);
    }

    void Brain::init() {
        bool initialized = config->getBool(name, CONFIG_KEY_BRAIN_INITIALIZED, false);
        if (!initialized) {
            logger->log(name, INFO, "Brain has not been initialized, performing a factory reset.");
            factoryReset();
        }
        droidState->stickEnable = config->getBool(name, CONFIG_KEY_BRAIN_STICK_ENABLE, CONFIG_DEFAULT_BRAIN_STICK_ENABLE);
        droidState->turboSpeed = config->getBool(name, CONFIG_KEY_BRAIN_TURBO_ENABLE, CONFIG_DEFAULT_BRAIN_TURBO_ENABLE);
        droidState->autoDomeEnable = config->getBool(name, CONFIG_KEY_BRAIN_AUTODOME_ENABLE, CONFIG_DEFAULT_BRAIN_AUTODOME_ENABLE);
        for (droid::services::ActiveComponent* component : componentList) {
            component->init();
        }
        pwmService.setOscFreq(PCA9685_OSC_FREQUENCY);
        controller.setDeadband(CONTROLLER_DEADBAND);
    }

    void Brain::factoryReset() {
        config->putBool(name, CONFIG_KEY_BRAIN_INITIALIZED, CONFIG_DEFAULT_BRAIN_INITIALIZED);
        config->putBool(name, CONFIG_KEY_BRAIN_STICK_ENABLE, CONFIG_DEFAULT_BRAIN_STICK_ENABLE);
        config->putBool(name, CONFIG_KEY_BRAIN_TURBO_ENABLE, CONFIG_DEFAULT_BRAIN_TURBO_ENABLE);
        config->putBool(name, CONFIG_KEY_BRAIN_AUTODOME_ENABLE, CONFIG_DEFAULT_BRAIN_AUTODOME_ENABLE);
        for (droid::services::ActiveComponent* component : componentList) {
            component->factoryReset();
        }
    }

    void Brain::failsafe() {
        for (droid::services::ActiveComponent* component : componentList) {
            component->failsafe();
        }
    }

    void Brain::reboot() {
        logger->log(name, WARN, "System Restarting...\n");
        sleep(2);
        ESP.restart();
    }

    void Brain::overrideCmdMap(const char* trigger, const char* cmd) {
        actionMgr.overrideCmdMap(trigger, cmd);
    }

    void Brain::trigger(const char* trigger) {
        actionMgr.fireTrigger(trigger);
    }

    void Brain::processCmdInput(Stream* cmdStream) {
        //Check for incoming serial commands
        while (cmdStream->available()) {
            char in = cmdStream->read();
            if (in == '\b') {    //backspace
                cmdStream->print("\b \b");
                if (bufIndex > 0) {
                    bufIndex--;
                }
            } else {
                cmdStream->print(in);
                if (in == '\r') {
                    cmdStream->print('\n');
                }
                if (bufIndex < sizeof(inputBuf)) {
                    inputBuf[bufIndex] = in;
                    bufIndex++;
                } else {
                    //Buffer overrun, don't store incoming char
                }
            }
            if ((in == '\r') || (in == '\n')) {
                //end of input
                if (bufIndex > 1) {     //Skip empty commands
                    inputBuf[bufIndex - 1] = 0;
                    actionMgr.queueCommand("Brain", inputBuf, millis());
                }
                bufIndex = 0;
            }
        }
    }

    void Brain::task() {
        processCmdInput(&LOGGER_STREAM);
        for (droid::services::ActiveComponent* component : componentList) {
            component->task();
        }

        if (logger->getMaxLevel() >= ERROR) {
            failsafe();
            logger->clear();
        }
    }

    void Brain::logConfig() {
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_BRAIN_INITIALIZED, config->getString(name, CONFIG_KEY_BRAIN_INITIALIZED, "0"));
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_BRAIN_STICK_ENABLE, config->getString(name, CONFIG_KEY_BRAIN_STICK_ENABLE, "0"));
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_BRAIN_TURBO_ENABLE, config->getString(name, CONFIG_KEY_BRAIN_TURBO_ENABLE, "0"));
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_BRAIN_AUTODOME_ENABLE, config->getString(name, CONFIG_KEY_BRAIN_AUTODOME_ENABLE, "0"));
        for (droid::services::ActiveComponent* component : componentList) {
            component->logConfig();
        }
    }
}