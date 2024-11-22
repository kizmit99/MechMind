#pragma once
#include <Arduino.h>
#include "ReportQueue.h"
#include "MagicseeR1.h"
#include <NimBLEDevice.h>

#define RingAddressMaxLen 18

namespace blering {
    class Ring {
    public:
        char address[RingAddressMaxLen];
        NimBLEAdvertisedDevice *advertisedDevice;
        NimBLEAddress bleAddress;
        volatile bool waitingFor;
        volatile bool connectTo;
        bool connected = false;

        Ring() {}

        void onConnect();
        void onDisconnect();
        void onReport(uint8_t* pData, size_t length);

        bool isButtonPressed(MagicseeR1::Button button);
        bool isButtonClicked(MagicseeR1::Button button);
        int8_t getJoystick(MagicseeR1::Direction direction);

        void init(const char* name, Logger* logger);
        void task();

        void setOtherRing(Ring *other) {otherRing = &(other->myRing);};
        void unpress(MagicseeR1::Button button);
        void printState();

    private:
        const char* name;
        Logger* logger;
        ReportQueue reportQueue;
        MagicseeR1 myRing;
        MagicseeR1 *otherRing;
        uint32_t droppedReports = 0;
        bool L2wasPressed = false;
    };
}