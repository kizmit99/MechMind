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
//#define BUILD_FOR_DEBUGGER

//PCA9685PWM Config
#define PCA9685_I2C_ADDRESS 0x40
#define PCA9685_OUTPUT_ENABLE_PIN 15
#define PCA9685_OSC_FREQUENCY 27000000

//DomeMgr Motor config
#define PWMSERVICE_DOME_MOTOR_OUT1 0
#define PWMSERVICE_DOME_MOTOR_OUT2 1
#define DOMEMOTOR_POWER_RAMP 10.0

//Stream configurations
#define LOGGER_STREAM &Serial
#define LOGGER_STREAM_SETUP Serial.begin(115200)
#define CONSOLE_STREAM &Serial
#define CONSOLE_STREAM_SETUP
#define DOME_STREAM &Serial
#define DOME_STREAM_SETUP
#define SABERTOOTH_STREAM &Serial2
#define SABERTOOTH_STREAM_SETUP Serial2.begin(9600)
#define BODY_STREAM &Serial
#define BODY_STREAM_SETUP
#define HCR_STREAM &Serial
#define HCR_STREAM_SETUP

//DualSonyMoveController
#define CONFIG_DEFAULT_SONY_DEADBAND        20
#define CONFIG_DEFAULT_MAC                  "XX:XX:XX:XX:XX:XX"
#define CONFIG_DEFAULT_SONY_RIGHT_MAC       CONFIG_DEFAULT_MAC
#define CONFIG_DEFAULT_SONY_ALT_RIGHT_MAC   CONFIG_DEFAULT_MAC
#define CONFIG_DEFAULT_SONY_LEFT_MAC        CONFIG_DEFAULT_MAC
#define CONFIG_DEFAULT_SONY_ALT_LEFT_MAC    CONFIG_DEFAULT_MAC

//Local Panel Servo Config
#define LOCAL_PANEL_COUNT             12
#define PWMSERVICE_PANEL_FIRST_OUT    4

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
