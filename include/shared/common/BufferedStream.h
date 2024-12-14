#pragma once
#include <Arduino.h>

class BufferedStream : public Stream {
private:
    HardwareSerial* wrapped = nullptr;
    uint8_t* buffer = nullptr;
    size_t bufferSize = 0;
    size_t writeIndex = 0;
    size_t readIndex = 0;
    size_t totalSize = 0;

    virtual bool isEmpty() {
        return (totalSize == 0);
    }

    virtual bool isFull() {
        return (totalSize == bufferSize);
    }

    virtual void append(uint8_t c) {
        buffer[writeIndex] = c;
        writeIndex = (writeIndex + 1) % bufferSize;
        totalSize++;
    }

    virtual uint8_t getNext() {
        uint8_t result = buffer[readIndex];
        readIndex = (readIndex + 1) % bufferSize;
        totalSize--;
        return result;
    }

public:
    BufferedStream(HardwareSerial* hwSerial, size_t bufSize = 1024) : 
        wrapped(hwSerial),
        bufferSize(bufSize) {

        buffer = new uint8_t[bufSize];
    }

    ~BufferedStream() {
        delete[] buffer;
    }

    virtual size_t write(uint8_t c) override {
        if (isEmpty() &&
            wrapped->availableForWrite()) {
            //No need to buffer, write directly to the stream
            return wrapped->write(c);
        } else if (isFull()) {
            //Do a blocking write to free up a char in the buffer
            wrapped->write(getNext());
        }
        //Write into the buffer
        append(c);
        return 1;
    }

    void task() {
        while (!isEmpty() && wrapped->availableForWrite()) {
            wrapped->write(getNext());
        }
    }

    virtual int available() override {
        return wrapped->available();
    }

    virtual int read() override {
        return wrapped->read();
    }

    virtual int peek() override {
        return wrapped->peek();
    }

    virtual void flush() override {
        wrapped->flush();
    }
};
