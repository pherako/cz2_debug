// Copyright - Jared Gaillard - 2016
// MIT License
//
// Arduino CZII Project

#ifndef Util_h
#define Util_h

//#include <Arduino.h>
#include "RingBuffer.hpp"
#include <stdio.h>

#define DEBUG_LOG
#define INFO_LOG

#ifdef DEBUG_LOG
#define debug_print(x)   fprintf(stderr, x)
#define debug_println(x)  fprintf(stderr, x "\n");
#else
#define debug_print(x)
#define debug_println(x)
#endif

#ifdef INFO_LOG
#define info_print(x)    fprintf(stdout, x)
#define info_println(x)  fprintf(stdout, x "\n");
#else
#define info_print(x)
#define info_println(x)
#endif

#define array_len( x )  ( sizeof( x ) / sizeof( x[0] ) )

#define FLOAT_MIN_VALUE -1000.0f  // We shouldn't have values less than this

uint16_t ModRTU_CRC(RingBuffer& ringBuffer, uint8_t length);
uint16_t ModRTU_CRC(uint8_t values[], uint8_t length);

#endif
