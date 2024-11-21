#include "shared/blering/ReportQueue.h"

namespace blering {
    void ReportQueue::init(const char* name, Logger* logger) {
        this->name = name;
        this->logger = logger;
    }

    ReportRecord *ReportQueue::getNextFreeReportBuffer() {
        if (isFull()) {
            logger->log(name, DEBUG, "Report Buffer overrun!\n");
            return NULL;
        }
        int8_t index = head;
        head = (head + 1) % REPORT_BUFFER_SIZE;
        count++;
        return &(buffer[index]);
    }

    ReportRecord *ReportQueue::getNextReport() {
        if (isEmpty()) {
            logger->log(name, DEBUG, "getNextReport called on EMPTY queue!\n");
            return NULL;
        }
        return &buffer[tail];
    }

    void ReportQueue::releaseReportBuffer() {
        if (isEmpty()) {
            logger->log(name, DEBUG, "Report Buffer released when empty!\n");
        } else {
            tail = (tail + 1) % REPORT_BUFFER_SIZE;
            count--;
        }
    }
}