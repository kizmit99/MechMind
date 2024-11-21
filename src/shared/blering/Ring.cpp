#include "shared/blering/Ring.h"
#include "shared/blering/MagicseeR1.h"
#include "shared/blering/ReportQueue.h"

namespace blering {
    void Ring::onConnect() {
        connected = true;
    }

    void Ring::onDisconnect() {
        logger->log(name, DEBUG, "Ring.onDisconnect: %s\n", address);
        myRing.disconnect();
        waitingFor = true;
        connectTo = false;
        connected = false;
    }

    void Ring::onReport(uint8_t* pData, size_t length) {
        if (reportQueue.isFull()) {
            logger->log(name, DEBUG, "Report Buffer is full(%s), dropping a report, current dropped count: %u\n", address, ++droppedReports);
        } else {
            volatile ReportRecord* newRecord = reportQueue.getNextFreeReportBuffer();
            newRecord->report_len = length;
            memcpy((void *) newRecord->report, pData, min(length, sizeof(newRecord->report)));
        }
    }

    void Ring::init(const char* name, Logger* logger) {
        this->name = name;
        this->logger = logger;
        myRing.init(name, logger);
        
        address[0] = 0;
        advertisedDevice = NULL;
        waitingFor = true;
        connectTo = false;
    }

    void Ring::unpress(MagicseeR1::Button button) {
        myRing.unpress(button);
    }

    void Ring::task() {
        if (!reportQueue.isEmpty()) {
            ReportRecord *newReport = reportQueue.getNextReport();

            logger->log(name, DEBUG, "%s:", __func__);
            for (size_t i = 0; i < newReport->report_len; i++) {
                logger->printf(name, DEBUG, " %02x", newReport->report[i]);
            }
            //DBG_printf(", head: %d, tail: %d, count: %d, empty: %d, full: %d", reportQueueHead, reportQueueTail, reportQueueCount, reportQueueEmpty(), reportQueueFull());
            logger->printf(name, DEBUG, "\n");

            bool l2Before = myRing.isButtonPressed(MagicseeR1::L2);
            myRing.handleReport(newReport->report, newReport->report_len);
            reportQueue.releaseReportBuffer();
            bool l2After = myRing.isButtonPressed(MagicseeR1::L2);
            if (l2Before && !l2After) {
                otherRing->unpress(MagicseeR1::A);
                otherRing->unpress(MagicseeR1::B);
            }
    //        myRing.printState();
        }
    }

    bool Ring::isButtonPressed(MagicseeR1::Button button) {
        if ((myRing.getMode() != MagicseeR1::MODE_D) ||
            (otherRing->getMode() != MagicseeR1::MODE_D)) {
            return false;
        }

        //Enforce U/D/L/R quiet until joystick has been centered after L2 Pressed
        if (!myRing.isButtonPressed(MagicseeR1::UP) && 
            !myRing.isButtonPressed(MagicseeR1::DOWN) &&
            !myRing.isButtonPressed(MagicseeR1::LEFT) &&
            !myRing.isButtonPressed(MagicseeR1::RIGHT)) {
            L2wasPressed = false;
        }
        if (L2wasPressed &&
            ((button == MagicseeR1::UP) ||
            (button == MagicseeR1::DOWN) ||
            (button == MagicseeR1::LEFT) ||
            (button == MagicseeR1::RIGHT))) {
            return false;
        }
        bool value = myRing.isButtonPressed(button);
        if ((button == MagicseeR1::L2) &&
            (value == true)) {
            L2wasPressed = true;
        }
        return value;
    }

    bool Ring::isButtonClicked(MagicseeR1::Button button) {
        if ((myRing.getMode() != MagicseeR1::MODE_D) ||
            (otherRing->getMode() != MagicseeR1::MODE_D)) {
            return false;
        }
        return myRing.isButtonClicked(button);
    }

    #define SLOW 32
    #define FAST 127
    #define MID (SLOW + (FAST - SLOW)/2)

    int8_t Ring::getJoystick(MagicseeR1::Direction direction) {
        if ((myRing.getMode() != MagicseeR1::MODE_D) ||
            (otherRing->getMode() != MagicseeR1::MODE_D)) {
            return 0;
        }
        if (!isButtonPressed(MagicseeR1::L2)) {
            return 0;
        }
        int8_t value = 0;
        int8_t range = MID;
        if (otherRing->isButtonPressed(MagicseeR1::A)) {
            range = SLOW;
        } else if (otherRing->isButtonPressed(MagicseeR1::B)) {
            range = FAST;
        }
        switch (direction) {
            case MagicseeR1::X:
                if (myRing.isButtonPressed(MagicseeR1::LEFT) && !myRing.isButtonPressed(MagicseeR1::RIGHT)) {
                    value = -range;
                } else if (myRing.isButtonPressed(MagicseeR1::RIGHT) && !myRing.isButtonPressed(MagicseeR1::LEFT)) {
                    value = range;
                }
                break;

            case MagicseeR1::Y:
                if (myRing.isButtonPressed(MagicseeR1::UP) && !myRing.isButtonPressed(MagicseeR1::DOWN)) {
                    value = range;
                } else if (myRing.isButtonPressed(MagicseeR1::DOWN) && !myRing.isButtonPressed(MagicseeR1::UP)) {
                    value = -range;
                }
                break;

            default:
                value = 0;
        }
        if (value != 0) {
            printState();
        }
        return value;
    }

    void Ring::printState() {
        myRing.printState();
    }
}