/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/MechMind
 */

#include "droid/brain/Brain.h"
#include "droid/core/hardware.h"
#include "droid/command/StreamCmdHandler.h"
#include "droid/command/CmdLogger.h"
#include "droid/brain/LocalCmdHandler.h"
#include "droid/audio/AudioCmdHandler.h"

#define CONFIG_KEY_BRAIN_INITIALIZED     "Initialized"
#define CONFIG_KEY_BRAIN_STICK_ENABLE    "StartDriveOn"
#define CONFIG_KEY_BRAIN_TURBO_ENABLE    "StartTurboOn"
#define CONFIG_KEY_BRAIN_AUTODOME_ENABLE "StartDomeOn"

#define CONFIG_DEFAULT_BRAIN_INITIALIZED     true
#define CONFIG_DEFAULT_BRAIN_STICK_ENABLE    true
#define CONFIG_DEFAULT_BRAIN_TURBO_ENABLE    false
#define CONFIG_DEFAULT_BRAIN_AUTODOME_ENABLE false

namespace droid::brain {
    Brain::Brain(const char* name, droid::core::System* system) : 
        BaseComponent(name, system),
        system(system),
#ifdef BUILD_FOR_DEBUGGER
        pwmService("StubPWM", system),
        controller("StubCtrl", system),
#else
        pwmService("PCA9685", system, PCA9685_I2C_ADDRESS, PCA9685_OUTPUT_ENABLE_PIN),
        controller("DualSony", system),
//        controller("DualRing", system),
#endif
        domeMotorDriver("Dome_DRV8871", system, PWMSERVICE_DOME_MOTOR_OUT1, PWMSERVICE_DOME_MOTOR_OUT2, -1, -1),
        domeMgr("DomeMgr", system, &controller, &domeMotorDriver),
        actionMgr("ActionMgr", system, &controller),
//        audioDriver("HCRDriver", system, HCR_STREAM),
        audioDriver("DFMiniDriver", system, DFMINI_STREAM),
        audioMgr("AudioMgr", system, &audioDriver),
//        driveMotorDriver("Sabertooth", system, (byte) 128, SABERTOOTH_STREAM),
        driveMotorDriver("Drive_DRV8871", system, PWMSERVICE_DRIVE_MOTOR0_OUT1, PWMSERVICE_DRIVE_MOTOR0_OUT2, PWMSERVICE_DRIVE_MOTOR1_OUT1, PWMSERVICE_DRIVE_MOTOR1_OUT2),
        driveMgr("DriveMgr", system, &controller, &driveMotorDriver) {

        system->setPWMService(&pwmService);  //Necessary to avoid circular reference at initialization

        actionMgr.addCmdHandler(new droid::command::CmdLogger("CmdLogger", system));

        actionMgr.addCmdHandler(new droid::command::StreamCmdHandler("Dome", system, DOME_STREAM));
        actionMgr.addCmdHandler(new droid::command::StreamCmdHandler("Body", system, BODY_STREAM));
        actionMgr.addCmdHandler(new droid::audio::AudioCmdHandler("HCR", system, &audioMgr));
        actionMgr.addCmdHandler(new droid::brain::LocalCmdHandler("Brain", system, this, CONSOLE_STREAM));
        droid::brain::PanelCmdHandler* panelCmdHandler = new droid::brain::PanelCmdHandler("Panel", system);
        actionMgr.addCmdHandler(panelCmdHandler);

        //Setup list of all Active BaseComponents
        componentList.push_back(&pwmService);
        componentList.push_back(&controller);
        componentList.push_back(&domeMotorDriver);
        componentList.push_back(&driveMotorDriver);
        componentList.push_back(&domeMgr);
        componentList.push_back(&driveMgr);
        componentList.push_back(&audioMgr);
        componentList.push_back(&audioDriver);
        componentList.push_back(&actionMgr);
        componentList.push_back(panelCmdHandler);
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

        //Initialize the Logger log levels for all Components
        #define LOGGER logger
        #include "droid/core/LoggerLevels.h"

        //Initialize all BaseComponents
        for (droid::core::BaseComponent* component : componentList) {
            component->init();
        }
    }

    void Brain::factoryReset() {
        config->putBool(name, CONFIG_KEY_BRAIN_INITIALIZED, CONFIG_DEFAULT_BRAIN_INITIALIZED);
        config->putBool(name, CONFIG_KEY_BRAIN_STICK_ENABLE, CONFIG_DEFAULT_BRAIN_STICK_ENABLE);
        config->putBool(name, CONFIG_KEY_BRAIN_TURBO_ENABLE, CONFIG_DEFAULT_BRAIN_TURBO_ENABLE);
        config->putBool(name, CONFIG_KEY_BRAIN_AUTODOME_ENABLE, CONFIG_DEFAULT_BRAIN_AUTODOME_ENABLE);
        for (droid::core::BaseComponent* component : componentList) {
            component->factoryReset();
            logger->setLogLevel(component->name, logger->getLogLevel(component->name));
        }
    }

    void Brain::failsafe() {
        for (droid::core::BaseComponent* component : componentList) {
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

    void Brain::processConsoleInput(Stream* cmdStream) {
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
        unsigned long begin = millis();
        if (CONSOLE_STREAM != NULL) {
            processConsoleInput(CONSOLE_STREAM);
        }
        for (droid::core::BaseComponent* component : componentList) {
            unsigned long compBegin = millis();
            component->task();
            unsigned long compTime = millis() - compBegin;
            if (compTime > 10) {
                logger->log(component->name, WARN, "subTask took %d millis to execute!\n", compTime);
            }
        }

        if (logger->getMaxLevel() >= ERROR) {
            failsafe();
            logger->clear();
        }
        unsigned long time = millis() - begin;
        if (time > 30) {
            logger->log(name, WARN, "Task took %d millis to execute!\n", time);
        }
    }

    void Brain::logConfig() {
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_BRAIN_INITIALIZED, config->getString(name, CONFIG_KEY_BRAIN_INITIALIZED, ""));
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_BRAIN_STICK_ENABLE, config->getString(name, CONFIG_KEY_BRAIN_STICK_ENABLE, ""));
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_BRAIN_TURBO_ENABLE, config->getString(name, CONFIG_KEY_BRAIN_TURBO_ENABLE, ""));
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_BRAIN_AUTODOME_ENABLE, config->getString(name, CONFIG_KEY_BRAIN_AUTODOME_ENABLE, ""));
        for (droid::core::BaseComponent* component : componentList) {
            component->logConfig();
        }
    }
}