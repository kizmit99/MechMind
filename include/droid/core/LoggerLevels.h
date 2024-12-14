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

#ifdef LOGGER
LOGGER->setLogLevel("Main", INFO);
LOGGER->setLogLevel("R2D2", DEBUG);
LOGGER->setLogLevel("PCA9685", INFO);
LOGGER->setLogLevel("PWMStub", INFO);
LOGGER->setLogLevel("DualSony", DEBUG);
LOGGER->setLogLevel("DualRing", INFO);
LOGGER->setLogLevel("ControllerStub", INFO);
LOGGER->setLogLevel("DomeDRV8871", INFO);
LOGGER->setLogLevel("DomeStub", INFO);
LOGGER->setLogLevel("DomeMgr", INFO);
LOGGER->setLogLevel("Sabertooth", INFO);
LOGGER->setLogLevel("DriveDRV8871", INFO);
LOGGER->setLogLevel("DriveStub", INFO);
LOGGER->setLogLevel("DriveMgr", INFO);
LOGGER->setLogLevel("ActionMgr", INFO);
LOGGER->setLogLevel("HCRDriver", DEBUG);
LOGGER->setLogLevel("DFMiniDriver", DEBUG);
LOGGER->setLogLevel("SparkDriver", DEBUG);
LOGGER->setLogLevel("AudioStub", DEBUG);
LOGGER->setLogLevel("AudioMgr", DEBUG);
LOGGER->setLogLevel("CmdLogger", INFO);
LOGGER->setLogLevel("Dome", INFO);
LOGGER->setLogLevel("Body", INFO);
LOGGER->setLogLevel("Audio", DEBUG);
LOGGER->setLogLevel("Panel", INFO);
LOGGER->setLogLevel("Brain", DEBUG);
LOGGER->setLogLevel("DualRingBLE", INFO);
LOGGER->setLogLevel("driveRing", INFO);
LOGGER->setLogLevel("domeRing", INFO);
LOGGER->setLogLevel("ringBLECB", INFO);
LOGGER->setLogLevel("AdvDevCB", INFO);
#endif
