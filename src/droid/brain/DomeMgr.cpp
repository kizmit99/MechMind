/*
 * Droid Brain Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#include "droid/brain/DomeMgr.h"

#define CONFIG_KEY_DOMEMGR_SPEED                "Speed"
#define CONFIG_KEY_DOMEMGR_360TIME              "360TimeMs"
#define CONFIG_KEY_DOMEMGR_DEADBAND             "Deadband"
#define CONFIG_KEY_DOMEMGR_AUTODOME_ENABLE      "AutoEnable"
#define CONFIG_KEY_DOMEMGR_AUTODOME_AUTOENABLE  "AutoAutoEnable"
#define CONFIG_KEY_DOMEMGR_AUTODOME_AUTO_IDLE   "AutoIdleMs"
#define CONFIG_KEY_DOMEMGR_AUTODOME_MIN_SPEED   "AutoMinSpeed"
#define CONFIG_KEY_DOMEMGR_AUTODOME_MAX_SPEED   "AutoMaxSpeed"
#define CONFIG_KEY_DOMEMGR_AUTODOME_MIN_DELAY   "AutoMinDelayMs"
#define CONFIG_KEY_DOMEMGR_AUTODOME_MAX_DELAY   "AutoMaxDelayMs"
// #define CONFIG_KEY_DOMEMGR_AUTODOME_AUDIO       "AutoAudio"
// #define CONFIG_KEY_DOMEMGR_AUTODOME_LIGHTS      "AutoLights"

#define CONFIG_DEFAULT_DOMEMGR_SPEED                100
#define CONFIG_DEFAULT_DOMEMGR_360TIME              5000
#define CONFIG_DEFAULT_DOMEMGR_DEADBAND             16
#define CONFIG_DEFAULT_DOMEMGR_AUTODOME_ENABLE      true
#define CONFIG_DEFAULT_DOMEMGR_AUTODOME_AUTOENABLE  true
#define CONFIG_DEFAULT_DOMEMGR_AUTODOME_AUTO_IDLE   30000
#define CONFIG_DEFAULT_DOMEMGR_AUTODOME_MIN_SPEED   30
#define CONFIG_DEFAULT_DOMEMGR_AUTODOME_MAX_SPEED   80
#define CONFIG_DEFAULT_DOMEMGR_AUTODOME_MIN_DELAY   3000
#define CONFIG_DEFAULT_DOMEMGR_AUTODOME_MAX_DELAY   20000
// #define CONFIG_DEFAULT_DOMEMGR_AUTODOME_AUDIO       true
// #define CONFIG_DEFAULT_DOMEMGR_AUTODOME_LIGHTS      true

namespace {     //Local helper methods
    int8_t scale(int8_t in, int8_t min, int8_t max) {
        if (in == 0) {
            return 0;   //Maintain zero
        }
        return map(in, -100, 100, min, max);
    }

    long randomBetween(long min, long max) {
        return min + random() % (max - min + 1);
    }

    int16_t pickNewAngle(int16_t currentAngle, int16_t min, int16_t max) {
        int16_t newAngle = (currentAngle * 0.3) + randomBetween(min, max);
        if (newAngle < min) newAngle = min;
        if (newAngle > max) newAngle = max;
        return newAngle;
    }

    int16_t calcRotationTime(int8_t speed, int8_t maxSpeed, int16_t rotationTimeAtMax) {
        float scale = (float) maxSpeed / (float) speed;
        return rotationTimeAtMax * scale;
    }

    //Note this method may return negative times
    int16_t calcTimeToAngle(int16_t startAngle, int16_t endAngle, int8_t speed, float ramp, int16_t revolveTime, int8_t maxSpeed) {
        int16_t distance = endAngle - startAngle;
        int16_t newRevolveTime = calcRotationTime(speed, maxSpeed, revolveTime);
        float T0 = ((float) speed) / ramp;
        float D0 = 360.0f * (T0 / (2 * newRevolveTime));

        //Check if T1 >= 0
        if ((2 * D0) > abs(distance)) {
            //If not, adjust for the case where distance is too short to full speed to be reached
            float distanceRad = abs((endAngle - startAngle) * 2.0 * PI / 360.0);
            float motionTime = sqrt((4.0 * distanceRad)/(((ramp * 1000.0 * 2.0 * PI)/maxSpeed)/revolveTime));
            if (distance < 0) {motionTime = -motionTime;}
            return (int16_t) (motionTime / 2.0);
        }

        //Calculate D1 and T1
        float D1 = abs(distance) - (2 * D0);
        float T1 = (D1 * newRevolveTime) / 360.0f;

        //Total Run Time (T0 + T1)
        int16_t runTime = (int16_t) (T0 + T1);
        if (distance < 0) {runTime = -runTime;}

        return runTime;
    }
}

namespace droid::brain {
    DomeMgr::DomeMgr(const char* name, droid::core::System* system, droid::controller::Controller* controller, droid::motor::MotorDriver* domeMotor) : 
        BaseComponent(name, system),
        controller(controller),
        domeMotor(domeMotor) {}

    void DomeMgr::init() {
        speed = config->getInt(name, CONFIG_KEY_DOMEMGR_SPEED, CONFIG_DEFAULT_DOMEMGR_SPEED);
        rotationTimeMs = config->getInt(name, CONFIG_KEY_DOMEMGR_360TIME, CONFIG_DEFAULT_DOMEMGR_360TIME);
        deadband = config->getInt(name, CONFIG_KEY_DOMEMGR_DEADBAND, CONFIG_DEFAULT_DOMEMGR_DEADBAND);
        autoEnabled = config->getBool(name, CONFIG_KEY_DOMEMGR_AUTODOME_ENABLE, CONFIG_DEFAULT_DOMEMGR_AUTODOME_ENABLE);
        autoAutoEnabled = config->getBool(name, CONFIG_KEY_DOMEMGR_AUTODOME_AUTOENABLE, CONFIG_DEFAULT_DOMEMGR_AUTODOME_AUTOENABLE);
        autoIdleMs = config->getInt(name, CONFIG_KEY_DOMEMGR_AUTODOME_AUTO_IDLE, CONFIG_DEFAULT_DOMEMGR_AUTODOME_AUTO_IDLE);
        autoMinSpeed = config->getInt(name, CONFIG_KEY_DOMEMGR_AUTODOME_MIN_SPEED, CONFIG_DEFAULT_DOMEMGR_AUTODOME_MIN_SPEED);
        autoMaxSpeed = config->getInt(name, CONFIG_KEY_DOMEMGR_AUTODOME_MAX_SPEED, CONFIG_DEFAULT_DOMEMGR_AUTODOME_MAX_SPEED);
        autoMinDelayMs = config->getInt(name, CONFIG_KEY_DOMEMGR_AUTODOME_MIN_DELAY, CONFIG_DEFAULT_DOMEMGR_AUTODOME_MIN_DELAY);
        autoMaxDelayMs = config->getInt(name, CONFIG_KEY_DOMEMGR_AUTODOME_MAX_DELAY, CONFIG_DEFAULT_DOMEMGR_AUTODOME_MAX_DELAY);
        // autoAudio = config->getBool(name, CONFIG_KEY_DOMEMGR_AUTODOME_AUDIO, CONFIG_DEFAULT_DOMEMGR_AUTODOME_AUDIO);
        // autoLights = config->getBool(name, CONFIG_KEY_DOMEMGR_AUTODOME_LIGHTS, CONFIG_DEFAULT_DOMEMGR_AUTODOME_LIGHTS);

        autoDomeActive = false;
    }

    void DomeMgr::factoryReset() {
        config->clear(name);
        config->putInt(name, CONFIG_KEY_DOMEMGR_SPEED, CONFIG_DEFAULT_DOMEMGR_SPEED);
        config->putInt(name, CONFIG_KEY_DOMEMGR_360TIME, CONFIG_DEFAULT_DOMEMGR_360TIME);
        config->putInt(name, CONFIG_KEY_DOMEMGR_DEADBAND, CONFIG_DEFAULT_DOMEMGR_DEADBAND);
        config->putBool(name, CONFIG_KEY_DOMEMGR_AUTODOME_ENABLE, CONFIG_DEFAULT_DOMEMGR_AUTODOME_ENABLE);
        config->putBool(name, CONFIG_KEY_DOMEMGR_AUTODOME_AUTOENABLE, CONFIG_DEFAULT_DOMEMGR_AUTODOME_AUTOENABLE);
        config->putInt(name, CONFIG_KEY_DOMEMGR_AUTODOME_AUTO_IDLE, CONFIG_DEFAULT_DOMEMGR_AUTODOME_AUTO_IDLE);
        config->putInt(name, CONFIG_KEY_DOMEMGR_AUTODOME_MIN_SPEED, CONFIG_DEFAULT_DOMEMGR_AUTODOME_MIN_SPEED);
        config->putInt(name, CONFIG_KEY_DOMEMGR_AUTODOME_MAX_SPEED, CONFIG_DEFAULT_DOMEMGR_AUTODOME_MAX_SPEED);
        config->putInt(name, CONFIG_KEY_DOMEMGR_AUTODOME_MIN_DELAY, CONFIG_DEFAULT_DOMEMGR_AUTODOME_MIN_DELAY);
        config->putInt(name, CONFIG_KEY_DOMEMGR_AUTODOME_MAX_DELAY, CONFIG_DEFAULT_DOMEMGR_AUTODOME_MAX_DELAY);
        // config->putBool(name, CONFIG_KEY_DOMEMGR_AUTODOME_AUDIO, CONFIG_DEFAULT_DOMEMGR_AUTODOME_AUDIO);
        // config->putBool(name, CONFIG_KEY_DOMEMGR_AUTODOME_LIGHTS, CONFIG_DEFAULT_DOMEMGR_AUTODOME_LIGHTS);
    }

    void DomeMgr::logConfig() {
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_DOMEMGR_SPEED, config->getString(name, CONFIG_KEY_DOMEMGR_SPEED, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_DOMEMGR_360TIME, config->getString(name, CONFIG_KEY_DOMEMGR_360TIME, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_DOMEMGR_DEADBAND, config->getString(name, CONFIG_KEY_DOMEMGR_DEADBAND, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_DOMEMGR_AUTODOME_ENABLE, config->getString(name, CONFIG_KEY_DOMEMGR_AUTODOME_ENABLE, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_DOMEMGR_AUTODOME_AUTOENABLE, config->getString(name, CONFIG_KEY_DOMEMGR_AUTODOME_AUTOENABLE, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_DOMEMGR_AUTODOME_AUTO_IDLE, config->getString(name, CONFIG_KEY_DOMEMGR_AUTODOME_AUTO_IDLE, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_DOMEMGR_AUTODOME_MIN_SPEED, config->getString(name, CONFIG_KEY_DOMEMGR_AUTODOME_MIN_SPEED, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_DOMEMGR_AUTODOME_MAX_SPEED, config->getString(name, CONFIG_KEY_DOMEMGR_AUTODOME_MAX_SPEED, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_DOMEMGR_AUTODOME_MIN_DELAY, config->getString(name, CONFIG_KEY_DOMEMGR_AUTODOME_MIN_DELAY, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_DOMEMGR_AUTODOME_MAX_DELAY, config->getString(name, CONFIG_KEY_DOMEMGR_AUTODOME_MAX_DELAY, "").c_str());
        // logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_DOMEMGR_AUTODOME_AUDIO, config->getString(name, CONFIG_KEY_DOMEMGR_AUTODOME_AUDIO, "").c_str());
        // logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_DOMEMGR_AUTODOME_LIGHTS, config->getString(name, CONFIG_KEY_DOMEMGR_AUTODOME_LIGHTS, "").c_str());
    }

    void DomeMgr::failsafe() {
        //NOOP - other BaseComponents will be notified directly
    }

    void DomeMgr::task() {
        int8_t joyX = controller->getJoystickPosition(droid::controller::Controller::Joystick::LEFT, droid::controller::Controller::Axis::X);
        if (abs(joyX) <= deadband) {
            joyX = 0;
        }
        joyX = scale(joyX, -speed, speed);
        controller->setCritical(abs(joyX) > 0);

        if (joyX != 0) {
            domeMotor->setMotorSpeed(0, joyX);
            //Disable autoDome
            droidState->autoDomeEnable = false;
            lastManualMove = millis();
            autoDomeActive = false;
        } else {
            bool moved = doAutoDome();
            if (!moved) {
                domeMotor->setMotorSpeed(0, 0);
            }
         }
    }

    bool DomeMgr::doAutoDome() {
        unsigned long now = millis();

        //Check for auto enable
        if (!droidState->autoDomeEnable &&
            autoEnabled && 
            autoAutoEnabled &&
            !autoDomeActive && 
            (now > (lastManualMove + autoIdleMs))) {
            droidState->autoDomeEnable = true;
            autoDomeActive = true;
            autoDomeMoving = false;
            autoDomeAngle = 0;
            autoDomeNextMove = now;
            logger->log(name, DEBUG, "AutoDome is activating after Idle time expired\n");
        }

        if (autoDomeActive) {
            if (autoDomeMoving &&
                (now >= autoDomeNextStop)) {
                autoDomeMoving = false;
                autoDomeSpeed = 0;
                logger->log(name, DEBUG, "AutoDome should have reached desired position, stopping\n");
            } else if (now >= autoDomeNextMove) {
                //Choose new position and speed
                int16_t newDomeAngle = pickNewAngle(autoDomeAngle, -160, 160);
                autoDomeSpeed = randomBetween(autoMinSpeed, autoMaxSpeed);
                //Determine how long it will take to get there
                int16_t timeToEndAngle = calcTimeToAngle(autoDomeAngle, newDomeAngle, autoDomeSpeed, 1.0f, rotationTimeMs, speed);
                if (timeToEndAngle < 0) {   //Moving in the negative direction
                    autoDomeSpeed = -autoDomeSpeed;
                    timeToEndAngle = -timeToEndAngle;
                }
                autoDomeNextStop = now + timeToEndAngle;
                //Choose time for next move
                autoDomeNextMove = autoDomeNextStop + randomBetween(autoMinDelayMs, autoMaxDelayMs);
                //Begin moving to new position
                logger->log(name, DEBUG, "AutoDome updating position, was: %d, new target: %d, speed: %d\n", autoDomeAngle, newDomeAngle, autoDomeSpeed);
                autoDomeAngle = newDomeAngle;
                autoDomeMoving = true;
            }
        } else {
            autoDomeMoving = false;
            autoDomeSpeed = 0;
        }

        if (autoDomeMoving) {
            domeMotor->setMotorSpeed(0, autoDomeSpeed);
            return true;
        } else {
            return false;
        }
    }
}
