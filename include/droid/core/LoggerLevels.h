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

#ifdef LOGGER
LOGGER->setLogLevel("R2D2", DEBUG);
LOGGER->setLogLevel("StubPWM", DEBUG);
LOGGER->setLogLevel("StubCtrl", DEBUG);
LOGGER->setLogLevel("PCA9685", DEBUG);
LOGGER->setLogLevel("DualSony", DEBUG);
LOGGER->setLogLevel("DualRing", INFO);
LOGGER->setLogLevel("DRV8871", DEBUG);
LOGGER->setLogLevel("DomeMgr", DEBUG);
LOGGER->setLogLevel("ActionMgr", DEBUG);
LOGGER->setLogLevel("HCRDriver", DEBUG);
LOGGER->setLogLevel("AudioMgr", DEBUG);
LOGGER->setLogLevel("Sabertooth", DEBUG);
LOGGER->setLogLevel("DriveMgr", DEBUG);
LOGGER->setLogLevel("CmdLogger", DEBUG);
LOGGER->setLogLevel("Dome", DEBUG);
LOGGER->setLogLevel("Body", DEBUG);
LOGGER->setLogLevel("HCR", DEBUG);
LOGGER->setLogLevel("Brain", DEBUG);
LOGGER->setLogLevel("DualRingBLE", INFO);
LOGGER->setLogLevel("driveRing", INFO);
LOGGER->setLogLevel("domeRing", INFO);
LOGGER->setLogLevel("ringBLECB", INFO);
#endif
