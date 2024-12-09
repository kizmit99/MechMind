/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/MechMind
 */

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "shared/blering/DualRingBLE.h"
#include "shared/blering/Ring.h"

namespace blering {

    static const char HID_SERVICE[] = "1812";
    static const char HID_REPORT_MAP[] = "2A4B";
    static const char HID_REPORT_DATA[] = "2A4D";
    static const char HID_PROTOCOL_MODE[] = "2A4E";
    static const char BATTERY_SERVICE[] = "180F";
    static const char BATTERY_LEVEL[] = "2A19";

    static NimBLEUUID HID_REPORT_DATA_UUID = NimBLEUUID(HID_REPORT_DATA);
    static NimBLEUUID HID_PROTOCOL_MODE_UUID = NimBLEUUID(HID_PROTOCOL_MODE);

    void scanEndedCB(NimBLEScanResults results);
    static uint32_t scanTime = 0; /** 0 = scan forever */

    #define CONFIG_KEY_BLERING_DRIVEMAC "DriveMAC"
    #define CONFIG_KEY_BLERING_DOMEMAC  "DomeMAC"

    #define CONFIG_DEFAULT_BLERING_DRIVEMAC "XX:XX:XX:XX:XX:XX"
    #define CONFIG_DEFAULT_BLERING_DOMEMAC  "XX:XX:XX:XX:XX:XX"

    char DriveMAC[RingAddressMaxLen] = CONFIG_DEFAULT_BLERING_DRIVEMAC;
    char DomeMAC[RingAddressMaxLen]  = CONFIG_DEFAULT_BLERING_DOMEMAC;

    /** Define a class to handle the client callbacks */
    class ClientCallbacks : public NimBLEClientCallbacks {
    public:
        ClientCallbacks() {}

        void init(const char* name, Logger* logger) {
            this->name = name;
            this->logger = logger;
        }

    private:
        void onConnect(NimBLEClient* pClient) {
            const char* peerAddress = pClient->getPeerAddress().toString().c_str();
            logger->log(name, INFO, "Connected to: %s\r\n", peerAddress);
            pClient->updateConnParams(120,120,0,60);
            Ring* driveRing = rings.getRing(DualRingBLE::Drive);
            Ring* domeRing = rings.getRing(DualRingBLE::Dome);
            if (!strncmp(domeRing->address, peerAddress, sizeof(domeRing->address))) {
                domeRing->onConnect();
            }
            if (!strncmp(driveRing->address, peerAddress, sizeof(driveRing->address))) {
                driveRing->onConnect();
            }
        };

        void onDisconnect(NimBLEClient* pClient) {
            const char* peerAddress = pClient->getPeerAddress().toString().c_str();
            logger->log(name, INFO, "%s Disconnected\n", peerAddress);
            Ring* driveRing = rings.getRing(DualRingBLE::Drive);
            Ring* domeRing = rings.getRing(DualRingBLE::Dome);
            if (!strncmp(domeRing->address, peerAddress, sizeof(domeRing->address))) {
                domeRing->onDisconnect();
                domeRing->waitingFor = true;
            }
            if (!strncmp(driveRing->address, peerAddress, sizeof(driveRing->address))) {
                driveRing->onDisconnect();
                driveRing->waitingFor = true;
            }
            if (!NimBLEDevice::getScan()->isScanning()) {
                logger->log(name, INFO, "Restarting scan");
                NimBLEDevice::getScan()->start(scanTime, scanEndedCB);
            }
        };

        /** Pairing process complete, we can check the results in ble_gap_conn_desc */
        void onAuthenticationComplete(ble_gap_conn_desc* desc){
            if(!desc->sec_state.encrypted) {
                logger->log(name, WARN, "Encrypt connection failed - disconnecting\n");
                /** Find the client with the connection handle provided in desc */
                NimBLEDevice::getClientByID(desc->conn_handle)->disconnect();
                return;
            }
        };

        const char* name;
        Logger* logger;
    };

    /** Create a single global instance of the callback class to be used by all clients */
    static ClientCallbacks clientCB;

    /** Define a class to handle the callbacks when advertisments are received */
    class AdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    public:
        AdvertisedDeviceCallbacks() {}

        AdvertisedDeviceCallbacks* init(const char* name, Logger* logger) {
            this->name = name;
            this->logger = logger;
            return this;
        }

    private:
        void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
            uint8_t advType = advertisedDevice->getAdvType();
            if ((advType == BLE_HCI_ADV_TYPE_ADV_DIRECT_IND_HD) ||
                (advType == BLE_HCI_ADV_TYPE_ADV_DIRECT_IND_LD) ||
                (advertisedDevice->haveServiceUUID() && advertisedDevice->isAdvertisingService(NimBLEUUID(HID_SERVICE)))) {
                logger->log(name, DEBUG, "Advertised HID Device found: %s\n", advertisedDevice->toString().c_str());
                logger->log(name, DEBUG, "Name = %s\n", advertisedDevice->getName().c_str());

                Ring* driveRing = rings.getRing(DualRingBLE::Drive);
                Ring* domeRing = rings.getRing(DualRingBLE::Dome);

                if (strstr(advertisedDevice->getName().c_str(), "Magicsee R1") != NULL) {
                    const char* peerAddress = advertisedDevice->getAddress().toString().c_str();
                    logger->log(name, INFO, "Found matching device with address: %s\n", peerAddress);

                    if (!strncmp(DriveMAC, advertisedDevice->getAddress().toString().c_str(), sizeof(DriveMAC))) {
                        //Found Drive Ring by saved MAC
                        if (driveRing->waitingFor) {
                            logger->log(name, INFO, "Reassigning to Drive\n");
                            driveRing->waitingFor = false;
                            driveRing->advertisedDevice = advertisedDevice;
                            driveRing->connectTo = true;
                        } else {
                            logger->log(name, INFO, "Drive Ring already assigned!\n");
                        }
                    } else if (!strncmp(DomeMAC, advertisedDevice->getAddress().toString().c_str(), sizeof(DomeMAC))) {
                        //Found Dome Ring by saved MAC
                        if (domeRing->waitingFor) {
                            logger->log(name, INFO, "Reassigning to Dome\n");
                            domeRing->waitingFor = false;
                            domeRing->advertisedDevice = advertisedDevice;
                            domeRing->connectTo = true;
                        } else {
                            logger->log(name, INFO, "Dome Ring already assigned!\n");
                        }
                    } else {
                        //We have an unknown Ring
                        if ((driveRing->waitingFor) &&
                            (DriveMAC[0] == 'X')) {
                            //Unrecognized Ring and Drive Ring doesn't have an assigned address yet
                            logger->log(name, INFO, "Assigning to Drive\n");
                            driveRing->waitingFor = false;
                            driveRing->advertisedDevice = advertisedDevice;
                            driveRing->connectTo = true;
                        } else if ((domeRing->waitingFor) &&
                                (DomeMAC[0] == 'X')) {
                            //Unrecognized Ring and Dome Ring doesn't have an assigned address yet
                            logger->log(name, INFO, "Assigning to Dome\n");
                            domeRing->waitingFor = false;
                            domeRing->advertisedDevice = advertisedDevice;
                            domeRing->connectTo = true;
                        } else {
                            logger->log(name, INFO, "Neither ring claimed the connection\n");
                        }
                    }
                } else {
                    logger->log(name, INFO, "Not a Match.  Device name: s\n", advertisedDevice->getName().c_str());
                }
                if ((!driveRing->waitingFor && !domeRing->waitingFor) && 
                    NimBLEDevice::getScan()->isScanning()) {
                    logger->log(name, DEBUG, "Stopping Scan in AdvDeviceCallback driveWait=%d, domeWait=%d\n", driveRing->waitingFor, domeRing->waitingFor);
                    NimBLEDevice::getScan()->stop();
                }
            }
        };

        const char* name;
        Logger* logger;
    };

    /** Notification / Indication receiving handler callback */
    void notifyCB(NimBLERemoteCharacteristic* pRemoteCharacteristic,
                uint8_t* pData, size_t length, bool isNotify) {
        if (pRemoteCharacteristic->getUUID() == HID_REPORT_DATA_UUID) {
            NimBLEAddress peerAddress = pRemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress();
            Ring* driveRing = rings.getRing(DualRingBLE::Drive);
            Ring* domeRing = rings.getRing(DualRingBLE::Dome);
            if (peerAddress == driveRing->bleAddress) {
                driveRing->onReport(pData, length);
            } else if (peerAddress == domeRing->bleAddress) {
                domeRing->onReport(pData, length);
            } else {
                rings.log(WARN, "Unexpected report from peer: %s\n", peerAddress.toString().c_str());
            }
        }
    };

    /** Handles the provisioning of clients and connects / interfaces with the server */
    bool connectToServer(Ring* ring) {
        NimBLEClient* pClient = nullptr;
        bool reconnected = false;

        /** Check if we have a client we should reuse first **/
        if (NimBLEDevice::getClientListSize()) {
            pClient = NimBLEDevice::getClientByPeerAddress(ring->advertisedDevice->getAddress());
            if (pClient && (pClient->getPeerAddress() == ring->bleAddress)) {
                if (!pClient->connect(ring->advertisedDevice, false)) {
                    rings.log(DEBUG, "Reconnect failed\n");
                    return false;
                }
                rings.log(DEBUG, "Reconnected client\n");
                reconnected = true;
            } else {
                // We don't already have a client that knows this device,
                //  we will check for a client that is disconnected that we can use.
                pClient = NimBLEDevice::getDisconnectedClient();
            }
        }

        /** No client to reuse? Create a new one. */
        if (pClient == NULL) {
            if (NimBLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS) {
                rings.log(WARN, "Max clients reached - no more connections available\n");
                return false;
            }

            pClient = NimBLEDevice::createClient();
            rings.log(DEBUG, "New client created\n");

            pClient->setClientCallbacks(&clientCB, false);
            pClient->setConnectionParams(12,12,0,51);
            pClient->setConnectTimeout(5);


            if (!pClient->connect(ring->advertisedDevice)) {
                /** Created a client but failed to connect, don't need to keep it as it has no data */
                NimBLEDevice::deleteClient(pClient);
                rings.log(DEBUG, "Failed to connect, deleted client\n");
                return false;
            }
        }

        if (!pClient->isConnected()) {
            if (!pClient->connect(ring->advertisedDevice)) {
                rings.log(DEBUG, "Failed to connect\n");
                return false;
            }
        }

        ring->bleAddress = pClient->getPeerAddress();

        strncpy(ring->address, pClient->getPeerAddress().toString().c_str(), sizeof(ring->address));
        ring->address[sizeof(ring->address) - 1] = '\0';
        rings.log(INFO, "Connected to: %s\n", ring->address);

        NimBLERemoteService *hidService = pClient->getService(HID_SERVICE);

        if (hidService != NULL) {
            std::vector<NimBLERemoteCharacteristic*>*charvector;
            charvector = hidService->getCharacteristics(true);
            // For each characteristic
            for (auto &it: *charvector) {
                if (it->getUUID() == HID_PROTOCOL_MODE_UUID) {
                    if (it->canRead()) {
                        it->readValue();
                    }
                } else if (it->getUUID() == HID_REPORT_DATA_UUID) {
                    if (it->canNotify()) {
                        if (it->subscribe(true, notifyCB)) {
                            rings.log(DEBUG, "subscribe notification OK\n");
                        } else {
                            /** Disconnect if subscribe failed */
                            rings.log(WARN, "subscribe notification failed\n");
                            pClient->disconnect();
                            return false;
                        }
                    }
                }
            }
        }

        rings.log(DEBUG, "Done with this device!\n");
        return true;
    }

    /** Callback to process the results of the last scan */
    void scanEndedCB(NimBLEScanResults results){
        rings.log(DEBUG, "Scan Ended\n");
    }

    void DualRingBLE::clearMACMap() {
        config->clear(name);
    }

    void DualRingBLE::log(LogLevel level, const char *format, ...) {
        char buf[100];
        if (logger) {
            va_list args; 
            va_start(args, format); 
            vsnprintf(buf, sizeof(buf), format, args);
            va_end(args);
            logger->log(name, level, buf);
        }
    }

    void DualRingBLE::init(const char* name, Logger* logger, Config* config) {
        this->name = name;
        this->logger = logger;
        this->config = config;

        driveRing.init("driveRing", logger);
        domeRing.init("domeRing", logger);
        clientCB.init("ringBLECB", logger);
        driveRing.setOtherRing(&domeRing);
        domeRing.setOtherRing(&driveRing);

        logger->log(name, DEBUG, "Before loading Prefs, Drive: %s, Dome: %s\n", DriveMAC, DomeMAC);
        strncpy(DriveMAC, config->getString(name, CONFIG_KEY_BLERING_DRIVEMAC, CONFIG_DEFAULT_BLERING_DRIVEMAC).c_str(), sizeof(DriveMAC));
        strncpy(DomeMAC, config->getString(name, CONFIG_KEY_BLERING_DOMEMAC, CONFIG_DEFAULT_BLERING_DOMEMAC).c_str(), sizeof(DomeMAC));
        logger->log(name, DEBUG, "After  loading Prefs, Drive: %s, Dome: %s\n", DriveMAC, DomeMAC);

        NimBLEDevice::init(name);
        //Begin listening for advertisements

        NimBLEDevice::setSecurityAuth(true, true, true);
        NimBLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db */
        NimBLEScan* pScan = NimBLEDevice::getScan();

        if ((DriveMAC[0] != 'X') &&
            (DomeMAC[0] != 'X')) {      //We have two MACs, enabled whitelist for scan
            logger->log(name, DEBUG, "Two MACs defined, enabling scan whitelist; drive: %s, dome: %s\n", DriveMAC, DomeMAC);
            NimBLEDevice::whiteListAdd(NimBLEAddress(DriveMAC));
            NimBLEDevice::whiteListAdd(NimBLEAddress(DomeMAC));
            pScan->setFilterPolicy(BLE_HCI_SCAN_FILT_USE_WL);
        } else {
            logger->log(name, DEBUG, "Not enabling scan whitelist; drive: %s, dome: %s\n", DriveMAC, DomeMAC);
        }
        
        /** create a callback that gets called when advertisers are found */
        pScan->setAdvertisedDeviceCallbacks((new AdvertisedDeviceCallbacks())->init("AdvDevCB", logger));

        /** Set scan interval (how often) and window (how long) in milliseconds */
        pScan->setInterval(22);
        pScan->setWindow(11);

        pScan->setActiveScan(false);

        logger->log(name, DEBUG, "Scanning\n");
        pScan->start(scanTime, scanEndedCB);
    }

    void DualRingBLE::factoryReset() {
        config->clear(name);
        config->putString(name, CONFIG_KEY_BLERING_DRIVEMAC, CONFIG_DEFAULT_BLERING_DRIVEMAC);
        config->putString(name, CONFIG_KEY_BLERING_DOMEMAC, CONFIG_DEFAULT_BLERING_DOMEMAC);
    }

    void DualRingBLE::logConfig() {
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_BLERING_DRIVEMAC, config->getString(name, CONFIG_KEY_BLERING_DRIVEMAC, "").c_str());
        logger->log(name, INFO, "Config %s = %s\n", CONFIG_KEY_BLERING_DOMEMAC, config->getString(name, CONFIG_KEY_BLERING_DOMEMAC, "").c_str());
    }

    void DualRingBLE::task() {
        if (driveRing.connectTo) {
            driveRing.connectTo = false;
            if (!connectToServer(&driveRing)) {
                driveRing.waitingFor = true;
            } else {
                if (strncmp(DriveMAC, driveRing.address, sizeof(DriveMAC))) {
                    config->putString(name, CONFIG_KEY_BLERING_DRIVEMAC, driveRing.address);
                }
            }
        }
        if (domeRing.connectTo) {
            domeRing.connectTo = false;
            if (!connectToServer(&domeRing)) {
                domeRing.waitingFor = true;
            } else {
                if (strncmp(DomeMAC, domeRing.address, sizeof(DomeMAC))) {
                    config->putString(name, CONFIG_KEY_BLERING_DOMEMAC, domeRing.address);
                }
            }
        }
        if ((driveRing.waitingFor || domeRing.waitingFor) && 
            !NimBLEDevice::getScan()->isScanning()) {
            NimBLEDevice::getScan()->start(scanTime, scanEndedCB);
        }

        driveRing.task();
        domeRing.task();
    }

    Ring *DualRingBLE::getRing(Controller controller) {
        switch (controller) {
            case Dome:  return &domeRing;
            case Drive: return &driveRing;
            default:
                logger->log(name, WARN, "ERROR in DualRingBLE::getRing, invalid controller\n");
                return NULL;
        }
    }

    bool DualRingBLE::isModifierPressed(Controller controller, Modifier button) {
        switch (button) {
            case A:    return getRing(controller)->isButtonPressed(MagicseeR1::A);
            case B:    return getRing(controller)->isButtonPressed(MagicseeR1::B);
            case L2:   return getRing(controller)->isButtonPressed(MagicseeR1::L2);
            default:   return false;
        }
    }

    bool DualRingBLE::isButtonPressed(Controller controller, Button button) {
        switch (button) {
            case Up:     return getRing(controller)->isButtonPressed(MagicseeR1::UP);
            case Down:   return getRing(controller)->isButtonPressed(MagicseeR1::DOWN);
            case Left:   return getRing(controller)->isButtonPressed(MagicseeR1::LEFT);
            case Right:  return getRing(controller)->isButtonPressed(MagicseeR1::RIGHT);
            default:     return false;
        }
    }

    bool DualRingBLE::isButtonClicked(Controller controller, Clicker button) {
        switch (button) {
            case C:     return getRing(controller)->isButtonClicked(MagicseeR1::C);
            case D:     return getRing(controller)->isButtonClicked(MagicseeR1::D);
            case L1:    return getRing(controller)->isButtonClicked(MagicseeR1::L1);
            default:    return false;
        }
    }

    int8_t DualRingBLE::getJoystick(Controller controller, Axis axis) {
        switch (axis) {
            case X:     return getRing(controller)->getJoystick(MagicseeR1::X);
            case Y:     return getRing(controller)->getJoystick(MagicseeR1::Y);
            default:    return 0;
        }
    }

    bool DualRingBLE::isConnected() {
        return (driveRing.connected &&
                domeRing.connected);
    }

    bool DualRingBLE::hasFault() {
        return false;
    }

    void DualRingBLE::printState() {
        logger->log(name, DEBUG, "Drive: ");
        driveRing.printState();
        logger->log(name, DEBUG, "Dome:  ");
        domeRing.printState();
    }
}