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
LOGGER->setLogLevel("R2D2", INFO);
LOGGER->setLogLevel("StubPWM", INFO);
LOGGER->setLogLevel("StubCtrl", INFO);
LOGGER->setLogLevel("PCA9685", DEBUG);
LOGGER->setLogLevel("DualSony", INFO);
LOGGER->setLogLevel("DualRing", INFO);
LOGGER->setLogLevel("Dome_DRV8871", INFO);
LOGGER->setLogLevel("Drive_DRV8871", DEBUG);
LOGGER->setLogLevel("DomeMgr", INFO);
LOGGER->setLogLevel("ActionMgr", INFO);
LOGGER->setLogLevel("HCRDriver", INFO);
LOGGER->setLogLevel("AudioMgr", INFO);
LOGGER->setLogLevel("Sabertooth", INFO);
LOGGER->setLogLevel("DriveMgr", DEBUG);
LOGGER->setLogLevel("CmdLogger", INFO);
LOGGER->setLogLevel("Dome", INFO);
LOGGER->setLogLevel("Body", INFO);
LOGGER->setLogLevel("HCR", INFO);
LOGGER->setLogLevel("Panel", INFO);
LOGGER->setLogLevel("Brain", INFO);
LOGGER->setLogLevel("DualRingBLE", INFO);
LOGGER->setLogLevel("driveRing", INFO);
LOGGER->setLogLevel("domeRing", INFO);
LOGGER->setLogLevel("ringBLECB", INFO);
#endif
