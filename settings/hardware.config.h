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
#include <SoftwareSerial.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

//Partition Name for the Config Store
#define CONFIG_PARTITION_NAME           "config"

//Configuration Options for pluggeable components
#define CONTROLLER_OPTION_DUALRING      "DualRing"
#define CONTROLLER_OPTION_SONYNAV       "SonyNav"
#define CONTROLLER_OPTION_PS3BT         "PS3Bt"
#define CONTROLLER_OPTION_PS3USB        "PS3Usb"
#define CONTROLLER_OPTION_STUB          "None"

#define PWMSERVICE_OPTION_PCA9685       "PCA9685"
#define PWMSERVICE_OPTION_STUB          "None"

#define MOTOR_DRIVER_OPTION_SABERTOOTH  "Sabertooth"
#define MOTOR_DRIVER_OPTION_PWMMOTOR    "PWMMotor"
#define MOTOR_DRIVER_OPTION_CYTRON      "Cytron"
#define MOTOR_DRIVER_OPTION_PWMMOTOR    "PWMMotor"
#define MOTOR_DRIVER_OPTION_STUB        "None"

#define AUDIO_DRIVER_OPTION_HCR         "HCR"
#define AUDIO_DRIVER_OPTION_DFMINI      "DFMini"
#define AUDIO_DRIVER_OPTION_SPARKFUN    "SparkFun"
#define AUDIO_DRIVER_OPTION_STUB        "None"

//Default Configuration Options (modify to suit your needs)
#define CONFIG_DEFAULT_CONTROLLER       CONTROLLER_OPTION_SONYNAV
#define CONFIG_DEFAULT_PWMSERVICE       PWMSERVICE_OPTION_PCA9685
#define CONFIG_DEFAULT_DRIVE_MOTOR      MOTOR_DRIVER_OPTION_PWMMOTOR
#define CONFIG_DEFAULT_DOME_MOTOR       MOTOR_DRIVER_OPTION_PWMMOTOR
#define CONFIG_DEFAULT_AUDIO_DRIVER     AUDIO_DRIVER_OPTION_DFMINI

extern EspSoftwareSerial::UART Serial3;

//Stream configurations (modify to suit your needs)
#define LOGGER_STREAM &Serial
#define LOGGER_STREAM_SETUP Serial.begin(115200)
#define CONSOLE_STREAM LOGGER_STREAM
#define CONSOLE_STREAM_SETUP
#define DOME_STREAM &Serial3
#define DOME_STREAM_SETUP Serial3.begin(2400, SWSERIAL_8N1, 32, 4)
#define SABERTOOTH_STREAM &Serial2
#define SABERTOOTH_STREAM_SETUP Serial2.begin(9600)
#define CYTRON_STREAM &Serial2
#define CYTRON_STREAM_SETUP
#define BODY_STREAM &Serial
#define BODY_STREAM_SETUP
#define AUDIO_STREAM &Serial1
#define AUDIO_STREAM_SETUP Serial1.begin(9600, SERIAL_8N1, 33, 25)

//PCA9685PWM Config
#define PCA9685_I2C_ADDRESS 0x40
#define PCA9685_OUTPUT_ENABLE_PIN 15
#define PCA9685_OSC_FREQUENCY 25000000
#define PCA9685_PWM_FREQ_HZ 50

//DomeMgr Motor config
#define PWMSERVICE_DOME_MOTOR_OUT1 0
#define PWMSERVICE_DOME_MOTOR_OUT2 1
#define DOMEMOTOR_POWER_RAMP 10.0

//DriveMgr Motor config
#define DRIVEMOTOR_POWER_RAMP 10.0
//Left Motor
#define PWMSERVICE_DRIVE_MOTOR0_OUT1 4
#define PWMSERVICE_DRIVE_MOTOR0_OUT2 5
//Right Motor
#define PWMSERVICE_DRIVE_MOTOR1_OUT1 2
#define PWMSERVICE_DRIVE_MOTOR1_OUT2 3

//DualSonyNavController (modify to restrict connections to only the specified MAC addresses)
#define CONFIG_DEFAULT_SONY_RIGHT_MAC       "XX:XX:XX:XX:XX:XX"
#define CONFIG_DEFAULT_SONY_ALT_RIGHT_MAC   "XX:XX:XX:XX:XX:XX"
#define CONFIG_DEFAULT_SONY_LEFT_MAC        "XX:XX:XX:XX:XX:XX"
#define CONFIG_DEFAULT_SONY_ALT_LEFT_MAC    "XX:XX:XX:XX:XX:XX"

//PS3Controller (modify to restrict connections to only the specified MAC addresses)
#define CONFIG_DEFAULT_PS3_MAC              "XX:XX:XX:XX:XX:XX"
#define CONFIG_DEFAULT_PS3_ALT_MAC          "XX:XX:XX:XX:XX:XX"

//Local Panel Servo Config
#define LOCAL_PANEL_COUNT             10
#define PWMSERVICE_PANEL_FIRST_OUT    6
