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
#include <Arduino.h>

namespace droid::brain {
    /**
     * Stream adapter that routes Logger output to SmartCLI::Console.
     * 
     * Logger writes to this Stream → forwarded to Console::log() → async queue
     * This prevents Logger output from interrupting user typing.
     * 
     * Usage:
     *   ConsoleStream consoleStream;
     *   System sys(&consoleStream, DEBUG);
     * 
     * Implementation notes:
     * - Buffers characters until newline detected
     * - Forwards complete lines to SmartCLI::Console::log()
     * - Read methods return no data (Logger only writes)
     */
    class ConsoleStream : public Stream {
    public:
        ConsoleStream();
        
        // Stream interface (write methods)
        size_t write(uint8_t c) override;
        size_t write(const uint8_t* buffer, size_t size) override;
        
        // Stream interface (read methods - not used by Logger)
        int available() override { return 0; }
        int read() override { return -1; }
        int peek() override { return -1; }
        void flush() override {}
        
    private:
        static const size_t MAX_LINE_LENGTH = 256;
        char lineBuffer[MAX_LINE_LENGTH];
        size_t bufferPos;
        
        void flushLine();
    };
}
