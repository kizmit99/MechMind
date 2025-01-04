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
#include "settings/hardware.config.h"
#include "droid/command/CmdLogger.h"
#include "droid/command/StreamCmdHandler.h"
#include "droid/brain/LocalCmdHandler.h"
#include "droid/audio/AudioCmdHandler.h"
#include "droid/brain/PanelCmdHandler.h"
#include "droid/services/NoPWMService.h"
#include "droid/services/PCA9685PWM.h"
#include "droid/controller/DualSonyMoveController.h"
#include "droid/controller/DualRingController.h"
#include "droid/controller/PS3BtController.h"
#include "droid/controller/PS3UsbController.h"
#include "droid/controller/StubController.h"
#include "droid/motor/PWMMotorDriver.h"
#include "droid/motor/SabertoothDriver.h"
#include "droid/motor/CytronSmartDriveDuoDriver.h"
#include "droid/motor/StubMotorDriver.h"
#include "droid/audio/HCRDriver.h"
#include "droid/audio/DFMiniDriver.h"
#include "droid/audio/SparkDriver.h"
#include "droid/audio/StubAudioDriver.h"


#define CONFIG_KEY_BRAIN_INITIALIZED        "Initialized"
#define CONFIG_KEY_BRAIN_CONTROLLER         "Controller"
#define CONFIG_KEY_BRAIN_PWMSERVICE         "PWMService"
#define CONFIG_KEY_BRAIN_DRIVE_MOTOR        "DriveMotor"
#define CONFIG_KEY_BRAIN_DOME_MOTOR         "DomeMotor"
#define CONFIG_KEY_BRAIN_AUDIO_DRIVER       "AudioDriver"
#define CONFIG_KEY_BRAIN_STICK_ENABLE       "StartDriveOn"
#define CONFIG_KEY_BRAIN_TURBO_ENABLE       "StartTurboOn"
#define CONFIG_KEY_BRAIN_AUTODOME_ENABLE    "StartDomeOn"

#define CONFIG_DEFAULT_BRAIN_INITIALIZED     true
#define CONFIG_DEFAULT_BRAIN_STICK_ENABLE    true
#define CONFIG_DEFAULT_BRAIN_TURBO_ENABLE    false
#define CONFIG_DEFAULT_BRAIN_AUTODOME_ENABLE false

namespace droid::brain {
    Brain::Brain(const char* name, droid::core::System* system) : 
        BaseComponent(name, system) {

        //Construct optional/pluggable components

        String whichService = config->getString(name, CONFIG_KEY_BRAIN_PWMSERVICE, CONFIG_DEFAULT_PWMSERVICE);
        logger->log(name, DEBUG, "Requested PWMService: %s\n", whichService);
        if (whichService == PWMSERVICE_OPTION_PCA9685) {
            logger->log(name, DEBUG, "Initializing PCA9685\n");
            pwmService = new droid::services::PCA9685PWM("PCA9685", system, PCA9685_I2C_ADDRESS, PCA9685_OUTPUT_ENABLE_PIN);
        } else {
            logger->log(name, DEBUG, "Initializing PWMStub\n");
            pwmService = new droid::services::NoPWMService("PWMStub", system);
        }
        system->setPWMService(pwmService);

        whichService = config->getString(name, CONFIG_KEY_BRAIN_CONTROLLER, CONFIG_DEFAULT_CONTROLLER);
        logger->log(name, DEBUG, "Requested Controller: %s\n", whichService);
        if (whichService == CONTROLLER_OPTION_DUALRING) {
            logger->log(name, DEBUG, "Initializing DualRing\n");
            controller = new droid::controller::DualRingController(CONTROLLER_OPTION_DUALRING, system);
        } else if (whichService == CONTROLLER_OPTION_SONYMOVE) {
            logger->log(name, DEBUG, "Initializing DualSony\n");
            controller = new droid::controller::DualSonyMoveController(CONTROLLER_OPTION_SONYMOVE, system);
        } else if (whichService == CONTROLLER_OPTION_PS3BT) {
            logger->log(name, DEBUG, "Initializing PS3Bt\n");
            controller = new droid::controller::PS3BtController(CONTROLLER_OPTION_PS3BT, system);
        } else if (whichService == CONTROLLER_OPTION_PS3USB) {
            logger->log(name, DEBUG, "Initializing PS3Usb\n");
            controller = new droid::controller::PS3UsbController(CONTROLLER_OPTION_PS3USB, system);
        } else {
            logger->log(name, DEBUG, "Initializing ControllerStub\n");
            controller = new droid::controller::StubController("ControllerStub", system);
        }

        whichService = config->getString(name, CONFIG_KEY_BRAIN_DRIVE_MOTOR, CONFIG_DEFAULT_DRIVE_MOTOR);
        logger->log(name, DEBUG, "Requested DriveMotor: %s\n", whichService);
        if (whichService == MOTOR_DRIVER_OPTION_SABERTOOTH) {
            logger->log(name, DEBUG, "Initializing Drive Sabertooth\n");
            driveMotorDriver = new droid::motor::SabertoothDriver("DriveSaber", system, (byte) 128, SABERTOOTH_STREAM);
        } else if (whichService == MOTOR_DRIVER_OPTION_CYTRON) {
            logger->log(name, DEBUG, "Initializing DriveCytron\n");
            driveMotorDriver = new droid::motor::CytronSmartDriveDuoMDDS30Driver("DriveCytron", system, (byte) 128, CYTRON_STREAM);
        } else if (whichService == MOTOR_DRIVER_OPTION_PWMMOTOR) {
            logger->log(name, DEBUG, "Initializing DrivePWM\n");
            driveMotorDriver = new droid::motor::PWMMotorDriver("DrivePWM", system, PWMSERVICE_DRIVE_MOTOR0_OUT1, PWMSERVICE_DRIVE_MOTOR0_OUT2, PWMSERVICE_DRIVE_MOTOR1_OUT1, PWMSERVICE_DRIVE_MOTOR1_OUT2);
        } else {
            logger->log(name, DEBUG, "Initializing DriveStub\n");
            driveMotorDriver = new droid::motor::StubMotorDriver("DriveStub", system);
        }

        whichService = config->getString(name, CONFIG_KEY_BRAIN_DOME_MOTOR, CONFIG_DEFAULT_DOME_MOTOR);
        logger->log(name, DEBUG, "Requested DomeMotor: %s\n", whichService);
        if (whichService == MOTOR_DRIVER_OPTION_PWMMOTOR) {
            logger->log(name, DEBUG, "Initializing DomePWM\n");
            domeMotorDriver = new droid::motor::PWMMotorDriver("DomePWM", system, PWMSERVICE_DOME_MOTOR_OUT1, PWMSERVICE_DOME_MOTOR_OUT2, -1, -1);
        } else {
            logger->log(name, DEBUG, "Initializing DomeStub\n");
            domeMotorDriver = new droid::motor::StubMotorDriver("DomeStub", system);
        }

        whichService = config->getString(name, CONFIG_KEY_BRAIN_AUDIO_DRIVER, CONFIG_DEFAULT_AUDIO_DRIVER);
        logger->log(name, DEBUG, "Requested AudioDriver: %s\n", whichService);
        if (whichService == AUDIO_DRIVER_OPTION_HCR) {
            logger->log(name, DEBUG, "Initializing HCRDriver\n");
            audioDriver = new droid::audio::HCRDriver("HCRDriver", system, AUDIO_STREAM);
        } else if (whichService == AUDIO_DRIVER_OPTION_DFMINI) {
            logger->log(name, DEBUG, "Initializing DFMiniDriver\n");
            audioDriver = new droid::audio::DFMiniDriver("DFMiniDriver", system, AUDIO_STREAM);
        } else if (whichService == AUDIO_DRIVER_OPTION_SPARKFUN) {
            logger->log(name, DEBUG, "Initializing SparkDriver\n");
            audioDriver = new droid::audio::SparkDriver("SparkDriver", system, AUDIO_STREAM);
        } else {
            logger->log(name, DEBUG, "Initializing AudioStub\n");
            audioDriver = new droid::audio::StubAudioDriver("AudioStub", system);
        }

        domeMgr = new droid::brain::DomeMgr("DomeMgr", system, controller, domeMotorDriver);
        driveMgr = new droid::brain::DriveMgr("DriveMgr", system, controller, driveMotorDriver);
        actionMgr = new droid::command::ActionMgr("ActionMgr", system, controller);
        audioMgr = new droid::audio::AudioMgr("AudioMgr", system, audioDriver);



        actionMgr->addCmdHandler(new droid::command::CmdLogger("CmdLogger", system));

        actionMgr->addCmdHandler(new droid::command::StreamCmdHandler("Dome", system, DOME_STREAM));
        actionMgr->addCmdHandler(new droid::command::StreamCmdHandler("Body", system, BODY_STREAM));
        actionMgr->addCmdHandler(new droid::audio::AudioCmdHandler("Audio", system, audioMgr));
        actionMgr->addCmdHandler(new droid::brain::LocalCmdHandler("Brain", system, this, CONSOLE_STREAM));
        droid::brain::PanelCmdHandler* panelCmdHandler = new droid::brain::PanelCmdHandler("Panel", system);
        actionMgr->addCmdHandler(panelCmdHandler);

        //Setup list of all Active BaseComponents
        componentList.push_back(pwmService);
        componentList.push_back(controller);
        componentList.push_back(domeMotorDriver);
        componentList.push_back(driveMotorDriver);
        componentList.push_back(domeMgr);
        componentList.push_back(driveMgr);
        componentList.push_back(audioMgr);
        componentList.push_back(audioDriver);
        componentList.push_back(actionMgr);
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
        #include "settings/LoggerLevels.config.h"

        //Initialize all BaseComponents
        for (droid::core::BaseComponent* component : componentList) {
            component->init();
        }
    }

    void Brain::factoryReset() {
        config->putBool(name, CONFIG_KEY_BRAIN_INITIALIZED, CONFIG_DEFAULT_BRAIN_INITIALIZED);
        config->putString(name, CONFIG_KEY_BRAIN_CONTROLLER, CONFIG_DEFAULT_CONTROLLER);
        config->putString(name, CONFIG_KEY_BRAIN_PWMSERVICE, CONFIG_DEFAULT_PWMSERVICE);
        config->putString(name, CONFIG_KEY_BRAIN_DRIVE_MOTOR, CONFIG_DEFAULT_DRIVE_MOTOR);
        config->putString(name, CONFIG_KEY_BRAIN_DOME_MOTOR, CONFIG_DEFAULT_DOME_MOTOR);
        config->putString(name, CONFIG_KEY_BRAIN_AUDIO_DRIVER, CONFIG_DEFAULT_AUDIO_DRIVER);
        config->putBool(name, CONFIG_KEY_BRAIN_STICK_ENABLE, CONFIG_DEFAULT_BRAIN_STICK_ENABLE);
        config->putBool(name, CONFIG_KEY_BRAIN_TURBO_ENABLE, CONFIG_DEFAULT_BRAIN_TURBO_ENABLE);
        config->putBool(name, CONFIG_KEY_BRAIN_AUTODOME_ENABLE, CONFIG_DEFAULT_BRAIN_AUTODOME_ENABLE);

        //The Controllers are special because they cannot be instantiated twice
        if (controller->getType() == droid::controller::Controller::ControllerType::DUAL_RING) {
            controller->factoryReset();
        } else {
            droid::controller::DualRingController* drController = new droid::controller::DualRingController(CONTROLLER_OPTION_DUALRING, system);
            drController->init();
            drController->factoryReset();
        }
        if (controller->getType() == droid::controller::Controller::ControllerType::DUAL_SONY) {
            controller->factoryReset();
        } else {
            droid::controller::DualSonyMoveController* dsController = new droid::controller::DualSonyMoveController(CONTROLLER_OPTION_SONYMOVE, system);
            dsController->init();
            dsController->factoryReset();
        }
        if (controller->getType() == droid::controller::Controller::ControllerType::PS3_BT) {
            controller->init();
            controller->factoryReset();
        } else {
            droid::controller::PS3BtController* ps3Controller = new droid::controller::PS3BtController(CONTROLLER_OPTION_PS3BT, system);
            ps3Controller->init();
            ps3Controller->factoryReset();
        }
        if (controller->getType() == droid::controller::Controller::ControllerType::PS3_USB) {
            controller->init();
            controller->factoryReset();
        } else {
            droid::controller::PS3UsbController* ps3Controller = new droid::controller::PS3UsbController(CONTROLLER_OPTION_PS3USB, system);
            ps3Controller->init();
            ps3Controller->factoryReset();
        }
        if (controller->getType() == droid::controller::Controller::ControllerType::STUB) {
            controller->factoryReset();
        } else {
            droid::controller::StubController* stubController = new droid::controller::StubController("ControllerStub", system);
            stubController->init();
            stubController->factoryReset();
        }

        //The rest of the components are more straight-forward
        (new droid::services::PCA9685PWM("PCA9685", system, PCA9685_I2C_ADDRESS, PCA9685_OUTPUT_ENABLE_PIN))->factoryReset();
        (new droid::services::NoPWMService("PWMStub", system))->factoryReset();
        (new droid::motor::SabertoothDriver("DriveSaber", system, (byte) 128, SABERTOOTH_STREAM))->factoryReset();
        (new droid::motor::CytronSmartDriveDuoMDDS30Driver("DriveCytron", system, (byte) 128, CYTRON_STREAM))->factoryReset();
        (new droid::motor::PWMMotorDriver("DrivePWM", system, PWMSERVICE_DRIVE_MOTOR0_OUT1, PWMSERVICE_DRIVE_MOTOR0_OUT2, PWMSERVICE_DRIVE_MOTOR1_OUT1, PWMSERVICE_DRIVE_MOTOR1_OUT2))->factoryReset();
        (new droid::motor::StubMotorDriver("DriveStub", system))->factoryReset();
        (new droid::motor::PWMMotorDriver("DomePWM", system, PWMSERVICE_DOME_MOTOR_OUT1, PWMSERVICE_DOME_MOTOR_OUT2, -1, -1))->factoryReset();
        (new droid::motor::StubMotorDriver("DomeStub", system))->factoryReset();
        (new droid::audio::HCRDriver("HCRDriver", system, AUDIO_STREAM))->factoryReset();
        (new droid::audio::DFMiniDriver("DFMiniDriver", system, AUDIO_STREAM))->factoryReset();
        (new droid::audio::SparkDriver("SparkDriver", system, AUDIO_STREAM))->factoryReset();
        (new droid::audio::StubAudioDriver("AudioStub", system))->factoryReset();
        (new droid::audio::AudioMgr("AudioMgr", system, audioDriver))->factoryReset();
        (new droid::audio::AudioCmdHandler("Audio", system, audioMgr))->factoryReset();
        (new droid::command::ActionMgr("ActionMgr", system, controller))->factoryReset();
        (new droid::command::CmdLogger("CmdLogger", system))->factoryReset();
        (new droid::command::StreamCmdHandler("Dome", system, DOME_STREAM))->factoryReset();
        (new droid::command::StreamCmdHandler("Body", system, BODY_STREAM))->factoryReset();
        (new droid::brain::DomeMgr("DomeMgr", system, controller, domeMotorDriver))->factoryReset();
        (new droid::brain::DriveMgr("DriveMgr", system, controller, driveMotorDriver))->factoryReset();
        (new droid::brain::LocalCmdHandler("Brain", system, this, CONSOLE_STREAM))->factoryReset();
        (new droid::brain::PanelCmdHandler("Panel", system))->factoryReset();
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

    void Brain::overrideCmdMap(const char* action, const char* cmd) {
        actionMgr->overrideCmdMap(action, cmd);
    }

    void Brain::fireAction(const char* action) {
        actionMgr->fireAction(action);
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
                    actionMgr->queueCommand("Brain", inputBuf, millis());
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
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_BRAIN_CONTROLLER, config->getString(name, CONFIG_KEY_BRAIN_CONTROLLER, ""));
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_BRAIN_PWMSERVICE, config->getString(name, CONFIG_KEY_BRAIN_PWMSERVICE, ""));
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_BRAIN_DRIVE_MOTOR, config->getString(name, CONFIG_KEY_BRAIN_DRIVE_MOTOR, ""));
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_BRAIN_DOME_MOTOR, config->getString(name, CONFIG_KEY_BRAIN_DOME_MOTOR, ""));
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_BRAIN_AUDIO_DRIVER, config->getString(name, CONFIG_KEY_BRAIN_AUDIO_DRIVER, ""));
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_BRAIN_STICK_ENABLE, config->getString(name, CONFIG_KEY_BRAIN_STICK_ENABLE, ""));
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_BRAIN_TURBO_ENABLE, config->getString(name, CONFIG_KEY_BRAIN_TURBO_ENABLE, ""));
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_BRAIN_AUTODOME_ENABLE, config->getString(name, CONFIG_KEY_BRAIN_AUTODOME_ENABLE, ""));
        for (droid::core::BaseComponent* component : componentList) {
            component->logConfig();
        }
    }
}