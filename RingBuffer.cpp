// Copyright - Jared Gaillard - 2016
// MIT License
//
// Arduino CZII Project

//#include "Arduino.h"
#include "include/RingBuffer.hpp"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

RingBuffer::RingBuffer()
{
}

/**
   Adds uint8_t to the head of the buffer
*/
bool RingBuffer::add(uint8_t value) {
  if ((bufferHeadPos + 1) % MAX_BUFFER_SIZE == bufferTailPos) {
    return false;
  }

  buffer[bufferHeadPos] = value;
  bufferHeadPos = (bufferHeadPos + 1) % MAX_BUFFER_SIZE;
  return true;
}

/**
   Returns the uint8_t from the specified buffer position
*/
uint8_t RingBuffer::peek(uint16_t position) {
  if ( bufferHeadPos == bufferTailPos)
    return -1;

  return buffer[(bufferTailPos + position) % MAX_BUFFER_SIZE];
}

/**
   sets the value for the specified position
*/
void RingBuffer::set(uint16_t position, uint8_t value) {
  buffer[position] = value;
}

/**
   Returns the uint8_t from the specified buffer position
*/
uint8_t RingBuffer::read() {
  if ( bufferHeadPos == bufferTailPos)
    return -1;

  uint8_t value = buffer[bufferTailPos];
  shift(1);
  return value;
}

/**
  Shift the buffer by quantity specified.
*/
void RingBuffer::shift(uint16_t quantity) {
  bufferTailPos = (bufferTailPos + quantity) % MAX_BUFFER_SIZE;
}

uint16_t RingBuffer::length() {
  return (MAX_BUFFER_SIZE + bufferHeadPos - bufferTailPos) % MAX_BUFFER_SIZE;
}

void RingBuffer::reset() {
  bufferHeadPos = 0;
  bufferTailPos = 0;
}

/**
   debug dump of the entire buffer
*/
void RingBuffer::dump(uint16_t bufferLength) {
  fprintf(stderr, "%s: ", __PRETTY_FUNCTION__);
  for (int pos = 0; pos < bufferLength; pos++) {
    uint8_t value = peek(pos);
    fprintf(stderr, "%02x ", value);
  }
  fflush(stderr);
}

