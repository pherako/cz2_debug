// Copyright - Jared Gaillard - 2016
// MIT License
//
// Arduino CZII Project

#ifndef RingBuffer_h
#define RingBuffer_h

#include <stdint.h>

class RingBuffer
{
  public:
    RingBuffer();
    bool add(uint8_t value);
    uint8_t peek(uint16_t position);
    void set(uint16_t position, uint8_t value);
    uint8_t read();
    void shift(uint16_t quantity);
    uint16_t length();
    void dump(uint16_t bufferLength);
    void reset();
    uint8_t* access();

    // Fixed size for now
    static const uint16_t MAX_BUFFER_SIZE = 256;

  private:
    uint8_t buffer[MAX_BUFFER_SIZE];
    uint16_t bufferTailPos = 0;
    uint16_t bufferHeadPos = 0;
};

#endif
