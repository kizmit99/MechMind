/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/MechMind
 */

#include "droid/brain/ConsoleStream.h"
#include <smartcli/SmartCLI.h>

namespace droid::brain {
    ConsoleStream::ConsoleStream() : bufferPos(0) {
        lineBuffer[0] = '\0';
    }
    
    size_t ConsoleStream::write(uint8_t c) {
        // Flush on newline or carriage return
        if (c == '\n' || c == '\r') {
            flushLine();
            return 1;
        }
        
        // Buffer character if space available
        if (bufferPos < MAX_LINE_LENGTH - 1) {
            lineBuffer[bufferPos++] = c;
            lineBuffer[bufferPos] = '\0';
        } else {
            // Buffer full - flush and start new line
            flushLine();
            lineBuffer[bufferPos++] = c;
            lineBuffer[bufferPos] = '\0';
        }
        
        return 1;
    }
    
    size_t ConsoleStream::write(const uint8_t* buffer, size_t size) {
        for (size_t i = 0; i < size; i++) {
            write(buffer[i]);
        }
        return size;
    }
    
    void ConsoleStream::flushLine() {
        if (bufferPos > 0) {
            // Route to Console async queue (won't interrupt typing)
            SmartCLI::Console::log("%s", lineBuffer);
            
            // Reset buffer
            bufferPos = 0;
            lineBuffer[0] = '\0';
        }
    }
}
