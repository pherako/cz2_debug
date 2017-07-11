

#include "ComfortZoneII.hpp"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "RingBuffer.hpp"
//#include "ComfortZoneII.hpp"

RingBuffer rs485InputBuf;
RingBuffer rs485OutputBuf;

#define NUM_ZONES 4
ComfortZoneII CzII((uint8_t)NUM_ZONES); //4 zones

//
//   Debug dump of the current frame including the checksum bytes.  Spaces are inserted for
//   readability between the major sections of the frame.
//
void dumpFrame(RingBuffer ringBuffer) {

  if (ringBuffer.length() == 0)
    return;

  // Data Size
  uint8_t dataLength = ringBuffer.peek(ComfortZoneII::DATA_LENGTH_POS);

  // Checksum
  uint16_t crc = ringBuffer.peek(ComfortZoneII::DATA_START_POS + dataLength)
               +(ringBuffer.peek(ComfortZoneII::DATA_START_POS + dataLength + 1) << 8);

  // Destination
  uint16_t dst_addr = ringBuffer.peek(ComfortZoneII::DEST_ADDRESS_POS)
                    +(ringBuffer.peek(ComfortZoneII::DEST_ADDRESS_POS + 1) << 8);

  // Source
  uint16_t src_addr = ringBuffer.peek(ComfortZoneII::SOURCE_ADDRESS_POS)
                    +(ringBuffer.peek(ComfortZoneII::SOURCE_ADDRESS_POS + 1) << 8);



  // Function
  uint32_t function = ringBuffer.peek(ComfortZoneII::FUNCTION_POS)
                    +(ringBuffer.peek(ComfortZoneII::FUNCTION_POS - 1) << 8)
                    +(ringBuffer.peek(ComfortZoneII::FUNCTION_POS - 2) << 16);

  fprintf(stdout, "crc=%04x src=%04x dst=%04x sz=%-3d ", crc, src_addr, dst_addr, dataLength);
  char f_str[4];
  switch(function & 0xff) {
        case 0x06: strncpy(f_str, "rsp", 3); break;;
        case 0x0b: strncpy(f_str, "rd ", 3); break;
        case 0x0c: strncpy(f_str, "wr ", 3); break;
        case 0x15: strncpy(f_str, "err", 3); break;
        default  : strncpy(f_str, "ukn", 3); break;
  }
  fprintf(stdout, "fn=%06x (%s)", function, f_str);

  // Data
  for (uint8_t pos = ComfortZoneII::DATA_START_POS; pos < (ComfortZoneII::DATA_START_POS + dataLength); pos++) {
    fprintf(stdout, "%02x ", ringBuffer.peek(pos));
  }

  fflush(stdout);
}
//
//  Publish CZII data to the MQTT feed
//
//
void publishCZIIData(RingBuffer ringBuffer) {
  printf("\nRS485: ");
  dumpFrame(ringBuffer);
  CzII.update(ringBuffer);

  printf("\n");
  Zone* zone;
  for (uint8_t zz = 0; zz < NUM_ZONES; zz++){
      zone = CzII.getZone(zz);
      uint8_t cool_set = zone->getCoolSetpoint();
      uint8_t heat_set = zone->getHeatSetpoint();
      float   temperature = zone->getTemperature();
      uint8_t humidity = zone->getHumidity();
      uint8_t damper_posn = zone->getDamperPosition();
      printf("\t[%d] cool_set = %d heat_set = %d temp = %2.2f hum = %d damper =%d\n",
          zz          ,
          cool_set    ,
          heat_set    ,
          temperature ,
          humidity    ,
          damper_posn
      );

  }


}
//  This method detects if the current buffer has a valid data frame.  If none is found the buffer is shifted
//  and we return false.
//
//  Carrier Comfort Zone || (CZII) data frame structure:
//    For more info see: https://github.com/jwarcd/CZII_to_MQTT/wiki/CZII-Serial-Protocol
//    (Note: Similar to the Carrier Infinity protocol: https://github.com/nebulous/infinitude/wiki/Infinity-serial-protocol)
//
//   |-----------------------------------------------------------------------------------|
//   |                                       Frame                                       |
//   |-----------------------------------------------------------------------------------|
//   |                      Header                          |           |                |
//   |-------------------------------------------------------           |                |
//   | 2 bytes | 2 bytes | 1 uint8_t |  2 bytes  | 1 uint8_t|   Data    |   Checksum     |
//   |-----------------------------------------------------------------------------------|
//   | Dest    | Source  | Data   | Reserved  | Function    |  0-255    |    2 bytes     |
//   | Address | Address | Length |           |             |  bytes    |                |
//   |-----------------------------------------------------------------------------------|
//
//    Example Data: 9 0   1 0   3   0 0 11   0 9 1   213 184
//      Destination = 9
//      Source      = 1
//      Data Length = 3
//      Function    = 11         (Read Request)
//      Data        = 0 9 1      (Table 9, Row 1)
//      Checksum    = 213 184
//
//   CZII Function Codes:
//      6 (0x06) Response
//           1 Byte Length, Data=0x00 – Seems to be an ACK to a write
//           Variable Length > 3 bytes – a response to a read request
//      11 (0x0B) Read Request
//           3 uint8_t Length, Data=Table and row of data to get
//      12 (0x0C) Write Request
//           Variable Length > 3 bytes
//           First 3 bytes of data are table and row to write to
//           Following bytes are data to write
//      21 (0x15) Error
//           1 Byte Length, Data=0x00
//
//

bool processInputFrame() {

  uint16_t bufferLength = rs485InputBuf.length();

  // see if the buffer has at least the minimum size for a frame
  if (bufferLength < ComfortZoneII::MIN_MESSAGE_SIZE ) {
    //debug_println("rs485InputBuf: bufferLength < MIN_MESSAGE_SIZE");
    return false;
  }

  uint8_t source = rs485InputBuf.peek(ComfortZoneII::SOURCE_ADDRESS_POS);
  uint8_t destination = rs485InputBuf.peek(ComfortZoneII::DEST_ADDRESS_POS);
  uint8_t dataLength = rs485InputBuf.peek(ComfortZoneII::DATA_LENGTH_POS);
  uint8_t function = rs485InputBuf.peek(ComfortZoneII::FUNCTION_POS);

  //debug_println("rs485InputBuf: source=" + String(source) + ", destination=" + String(destination) + ", dataLength=" + String(dataLength) + ", function=" + String(function));

  uint16_t checksum1Pos = ComfortZoneII::DATA_START_POS + dataLength;
  uint16_t checksum2Pos = checksum1Pos + 1;
  uint16_t frameLength = checksum2Pos + 1;

  // Make sure we have enough data for this frame
  uint16_t frameBufferDiff =  frameLength - bufferLength;
  if (frameBufferDiff > 0 && frameBufferDiff < 30) {
    // Don't have enough data yet, wait for another uint8_t...
    //fprintf(stderr, ".");
    return false;
  }

  fflush(stderr);

  uint16_t sumData = rs485InputBuf.peek(checksum1Pos) + (rs485InputBuf.peek(checksum2Pos) << 8);
  uint16_t crc = ModRTU_CRC(rs485InputBuf, checksum1Pos);

  if (sumData != crc){
    fprintf(stderr, "Detected CRC error or misalignment in data is=%04x shb=%04x xor=%04x\n", sumData, crc, sumData ^ crc);
    fflush(stderr);
    rs485InputBuf.dump(bufferLength);
    printf("\n"); fflush(stdout);
    rs485InputBuf.shift(1);
    return false;
  }

  publishCZIIData(rs485InputBuf);

  rs485InputBuf.shift(frameLength);

  //digitalWrite(BUILTIN_LED, HIGH);  // Flash LED while processing frames

  return true;
}

void processRs485InputStream(FILE* fd) {
  // Process input data

  uint8_t c;
  while (fread(&c, sizeof(c), 1, fd)){
    if (!rs485InputBuf.add(c))
      fprintf(stderr, "ERROR: INPUT BUFFER OVERRUN!");

    if (processInputFrame())
      fprintf(stderr, "FOUND GOOD CZII FRAME!");

    //lastReceivedMessageTimeMillis = millis();
  }
}


int main (int argc, char * argv[]){
    FILE* fd = 0;
    if (argc != 2) {
        fprintf(stderr, "<err> filename required\n");
        return -1;
    } else {
        fd = fopen(argv[1], "r");
        if(!fd) {
            fprintf(stderr, "<err> failed to open %s\n", argv[1]);
            return -1;
        }

    }

    processRs485InputStream(fd);

    printf("\n");

    fclose(fd);
    return 0;
}
