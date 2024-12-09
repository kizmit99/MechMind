/*
 * MechMind Program
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
LOGGER->setLogLevel("PCA9685", INFO);
LOGGER->setLogLevel("DualSony", DEBUG);
LOGGER->setLogLevel("DualRing", DEBUG);
LOGGER->setLogLevel("Dome_DRV8871", INFO);
LOGGER->setLogLevel("Drive_DRV8871", INFO);
LOGGER->setLogLevel("DomeMgr", INFO);
LOGGER->setLogLevel("ActionMgr", INFO);
LOGGER->setLogLevel("HCRDriver", INFO);
LOGGER->setLogLevel("AudioMgr", INFO);
LOGGER->setLogLevel("Sabertooth", INFO);
LOGGER->setLogLevel("DriveMgr", INFO);
LOGGER->setLogLevel("CmdLogger", INFO);
LOGGER->setLogLevel("Dome", INFO);
LOGGER->setLogLevel("Body", INFO);
LOGGER->setLogLevel("HCR", INFO);
LOGGER->setLogLevel("Panel", INFO);
LOGGER->setLogLevel("Brain", INFO);
LOGGER->setLogLevel("DualRingBLE", DEBUG);
LOGGER->setLogLevel("driveRing", DEBUG);
LOGGER->setLogLevel("domeRing", DEBUG);
LOGGER->setLogLevel("ringBLECB", DEBUG);
#endif
