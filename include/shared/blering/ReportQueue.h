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
#include "shared/common/Logger.h"

#define REPORT_BUFFER_SIZE 4

namespace blering {
    typedef struct {
        uint8_t report[32];
        size_t report_len;
        bool isNotify;
    } ReportRecord;

    class ReportQueue {
    public:
        const char* name;
        Logger* logger;
        int8_t head = 0;
        int8_t tail = 0;
        int8_t count = 0;
        ReportRecord buffer[REPORT_BUFFER_SIZE];

        ReportQueue() {}

        void init(const char* name, Logger* logger);

        bool isFull() {return count == REPORT_BUFFER_SIZE;}
        bool isEmpty() {return count == 0;}
        ReportRecord *getNextFreeReportBuffer();
        ReportRecord *getNextReport();
        void releaseReportBuffer();
    };
}