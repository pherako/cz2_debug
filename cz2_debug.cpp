


#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "include/RingBuffer.hpp"
#include "include/ComfortZoneII.hpp"
#include "include/debug.h"
#include "include/version.h"

#undef DEBUG_SHORT_NAME
#define DEBUG_SHORT_NAME __FILE__

#define DEVICE_ADDRESS 99
uint8_t REQUEST_INFO_TEMPLATE[] = {1, 0, DEVICE_ADDRESS, 0, 3, 0, 0, 11, 0, 255, 255, 0, 0};  // Note: Replace the table and row values, and calc checksum before sending out request
RingBuffer rs485InputBuf;
RingBuffer rs485OutputBuf;

#define NUM_ZONES 4
ComfortZoneII CzII((uint8_t)NUM_ZONES); //4 zones

static uint16_t src_addr = 8;
static uint16_t dst_addr = 9;
static int dbg_lvl = 1;

static void printhelp(char * name){
    // see long_opts in main
    printf("================================================================================\n");
    printf("usage: %s [options]\n", name                                                       );
    printf("  options:\n"                                                                      );
    printf("    -h, --help            : show this help dialog\n"                               );
    printf("    -v, --verbose         : enables debug prints\n"                                );
    printf("    -q, --quiet           : prints only on failure\n"                              );
    printf("        --version         : show version info\n"                                   );
    printf("================================================================================\n");
    printf("    -f, --file FILE       : load clk config register values from file\n"           );
    printf("================================================================================\n");
    printf("    -s, --source ADDR     : who %s will be (default %02x)\n", name, src_addr       );
    printf("    -d, --destination ADDR: who %s will talk to (default %02x)\n", name, dst_addr  );
//  printf("    -D, --regdump FILE    : dump all rows of every table\n"                        );
//  printf("    -w, --write ADDR DATA : write hex data to clk config register\n"               );
    printf("    -r, --read TBL_ROW    : issue a read request\n"                                );
    printf("================================================================================\n");
    printf("    -i, --info            : print information about settings and quit\n"           );
    printf("================================================================================\n");
}

static void print_version_info(char* name){
    P_DFL(dbg_lvl, "%s Working Copy Rev: %s \n", name, VERSION);
};
//
//   Debug dump of the current frame including the checksum bytes.  Spaces are inserted for
//   readability between the major sections of the frame.
//

void dumpFrame(RingBuffer* ringBuffer) {

  if (ringBuffer->length() == 0)
    return;

  // Data Size
  uint8_t dataLength = ringBuffer->peek(ComfortZoneII::DATA_LENGTH_POS);

  // Checksum
  uint16_t crc = ringBuffer->peek(ComfortZoneII::DATA_START_POS + dataLength)
               +(ringBuffer->peek(ComfortZoneII::DATA_START_POS + dataLength + 1) << 8);

  // Destination
  uint16_t dst_addr = ringBuffer->peek(ComfortZoneII::DEST_ADDRESS_POS)
                    +(ringBuffer->peek(ComfortZoneII::DEST_ADDRESS_POS + 1) << 8);

  // Source
  uint16_t src_addr = ringBuffer->peek(ComfortZoneII::SOURCE_ADDRESS_POS)
                    +(ringBuffer->peek(ComfortZoneII::SOURCE_ADDRESS_POS + 1) << 8);

  // Function
  uint32_t function = ringBuffer->peek(ComfortZoneII::FUNCTION_POS)
                    +(ringBuffer->peek(ComfortZoneII::FUNCTION_POS - 1) << 8)
                    +(ringBuffer->peek(ComfortZoneII::FUNCTION_POS - 2) << 16);

  fprintf(stdout, ANSI_COLOR_CYAN "crc=%04x src=%04x dst=%04x sz=%-3d ", crc, src_addr, dst_addr, dataLength);
  char f_str[4] = "";
  switch(function & 0xff) {
        case ComfortZoneII::RESPONSE_FUNCTION: strncpy(f_str, "rsp", 3); break;;
        case ComfortZoneII::READ_FUNCTION    : strncpy(f_str, "rd ", 3); break;
        case ComfortZoneII::WRITE_FUNCTION   : strncpy(f_str, "wr ", 3); break;
        case 0x15                            : strncpy(f_str, "err", 3); break;
        default                              : strncpy(f_str, "ukn", 3); break;
  }
  fprintf(stdout, "fn=%06x (%s)", function, f_str);

  // Data
  for (uint8_t pos = ComfortZoneII::DATA_START_POS; pos < (ComfortZoneII::DATA_START_POS + dataLength); pos++) {
      fprintf(stdout, "%02x ", ringBuffer->peek(pos));
  }
  fprintf(stdout, ANSI_COLOR_RESET);
  fflush(stdout);
}

//  Provess CZII data
void processCZIIData(RingBuffer* ringBuffer) {
  P_NFO(0, "RS485: ");
  dumpFrame(ringBuffer);
  if(dbg_lvl > 2) ringBuffer->dump(ringBuffer->length());
  CzII.update(ringBuffer);

  printf("\n");
  Zone* zone;
  for (uint8_t zz = 0; zz < NUM_ZONES; zz++){
      zone = CzII.getZone(zz);
      uint8_t cool_set    = zone->getCoolSetpoint();
      uint8_t heat_set    = zone->getHeatSetpoint();
      float   temperature = zone->getTemperature();
      uint8_t humidity    = zone->getHumidity();
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

void rs485_EnqueFrame(uint8_t values[], uint8_t size) {
  if (rs485OutputBuf.length() + size > RingBuffer::MAX_BUFFER_SIZE) {
    P_ERR("%s: out of space in buffer, skipping\n", __FUNCTION__);
    return;
  }

  // update checksum
  uint8_t checksum1 =  size - 2;
  uint16_t crc = ModRTU_CRC(values, checksum1);
  values[checksum1] = (uint8_t)(crc & 0xff);
  values[checksum1 + 1] = (uint8_t)(crc >> 8);

  for (uint8_t i = 0; i < size; i++) {
    uint8_t value = values[i];
    rs485OutputBuf.add(value);
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

int processInputFrame() {

    uint16_t bufferLength = rs485InputBuf.length();

    // see if the buffer has at least the minimum size for a frame
    if (bufferLength < ComfortZoneII::MIN_MESSAGE_SIZE ) {
        P_DBG(dbg_lvl, 3, "%s: bufferLength (%2d) < MIN_MESSAGE_SIZE (%d)\n", __FUNCTION__, bufferLength, ComfortZoneII::MIN_MESSAGE_SIZE);
        return ENODATA;
    }

    uint8_t source      = rs485InputBuf.peek(ComfortZoneII::SOURCE_ADDRESS_POS);
    uint8_t destination = rs485InputBuf.peek(ComfortZoneII::DEST_ADDRESS_POS);
    uint8_t dataLength  = rs485InputBuf.peek(ComfortZoneII::DATA_LENGTH_POS);
    uint8_t function    = rs485InputBuf.peek(ComfortZoneII::FUNCTION_POS);
    static uint8_t last_function = 0;

    uint16_t checksum1Pos = ComfortZoneII::DATA_START_POS + dataLength;
    uint16_t checksum2Pos = checksum1Pos + 1;
    uint16_t frameLength = checksum2Pos + 1;

    // Make sure we have enough data for this frame
    uint16_t frameBufferDiff = frameLength - bufferLength;
    if (frameBufferDiff > 0 && frameBufferDiff < 30) {
        P_DBG(dbg_lvl, 3, "%s: waiting for full frame is %02d shb %02d...\n", __FUNCTION__, bufferLength, frameLength);
        return ENODATA;
    }

    uint16_t sumData = rs485InputBuf.peek(checksum1Pos) + (rs485InputBuf.peek(checksum2Pos) << 8);
    uint16_t crc = ModRTU_CRC(rs485InputBuf, checksum1Pos);

    if (sumData != crc){
        P_ERR("Detected CRC error or misalignment in data is=%04x shb=%04x xor=%04x\n", sumData, crc, sumData ^ crc);
        fflush(stderr);
        rs485InputBuf.dump(bufferLength);
        printf("\n"); fflush(stdout);
        rs485InputBuf.shift(1);
        return ECOMM;
    }

    if(function == ComfortZoneII::RESPONSE_FUNCTION && last_function == ComfortZoneII::WRITE_FUNCTION){
        P_DBG(dbg_lvl, 0, "skipping wr ack\n");
        rs485InputBuf.shift(frameLength);
        last_function = function;
        return EIDRM; //let injectors know there will be a pause
    } else {
        processCZIIData(&rs485InputBuf);
    }

    rs485InputBuf.shift(frameLength);

    last_function = function;

    return 0;
}

#define get_char_from_stream(stream, var) fread(&var, sizeof(var), 1, stream)
static int cz2_read(FILE* fd, uint16_t rd_addr){
    char d;
    P_DBG(dbg_lvl, 1, "waiting our turn to read\n");
    while (get_char_from_stream(fd, d)){
        if (!rs485InputBuf.add(d)) {
            P_ERR("input buffer overrun!");
            return ENOSR;
        }

        uint8_t destination = rs485InputBuf.peek(ComfortZoneII::DEST_ADDRESS_POS);
        uint8_t function    = rs485InputBuf.peek(ComfortZoneII::FUNCTION_POS);

        if (destination == src_addr && function == ComfortZoneII::RESPONSE_FUNCTION){
            P_DBG(dbg_lvl, 3, "receive buffer match dest=%04x fn=%02x\n", destination, function);
            if (!processInputFrame()) { //good frame
                P_NFO("received response\n");
                return 0;
            }
        } else if (processInputFrame() == EIDRM) { //inject after write ack
            P_NFO(dbg_lvl, "rd@ %04x\n", rd_addr);
            REQUEST_INFO_TEMPLATE[0] = dst_addr;
            REQUEST_INFO_TEMPLATE[2] = src_addr;
            REQUEST_INFO_TEMPLATE[9] = (uint8_t)(rd_addr & 0xff);
            REQUEST_INFO_TEMPLATE[10] = (uint8_t)(rd_addr >> 8);
            rs485_EnqueFrame(REQUEST_INFO_TEMPLATE, array_len(REQUEST_INFO_TEMPLATE));
            int len = rs485OutputBuf.length();
            rs485OutputBuf.dump(len);

            int tx_cnt = fwrite(rs485OutputBuf.access(), sizeof(uint8_t), len, fd);

            if(tx_cnt == len) {
                P_NFO(dbg_lvl, "issued read @ %04x\n", rd_addr);
                processCZIIData(&rs485OutputBuf);
                rs485OutputBuf.shift(len);
            } else {
                perror(ANSI_COLOR_RED "<err>");
                P_ERR("%s fwrite cnt is %d shb %d\n", __FUNCTION__, tx_cnt, len);
                return EINVAL;
            }
        }

    }
}
static int framecnt = 0;

void sig_handler(int signo)
{
    if (signo == SIGINT)
        printf("processed %d frames. exiting\n", framecnt);

    fprintf(stdout, ANSI_COLOR_RESET);
    fflush(stdout);
    fprintf(stderr, ANSI_COLOR_RESET);
    fflush(stderr);
    exit(0);
}

int main (int argc, char * argv[]){
    char *target_file = NULL;
    uint8_t data = 0;
    uint8_t last_function = 0;
    uint16_t addr = 0;
    int sv;//for errno
    FILE* fd = 0;
    uint8_t d;

    if (signal(SIGINT, sig_handler) == SIG_ERR)
        P_ERR("Can't catch SIGINT\n");

    if (argc > 1 && strcmp(argv[1], "--quiet"))
        P_DFL(dbg_lvl, "CZ2 Debug Utility\n");

    // options flags
    int WriteData       = 0; // Write register
    int ReadData        = 0; // Read register
    int WriteHelpText   = 0; // Display help dialog
    int PrintRegDump    = 0; // Dump all regs in binary form
    int PrintInfo       = 0; // Print misc info

    // optarg junk
    int c;
    int option_index = 0;

    struct option long_opts[] = {
        {"src"    , required_argument , NULL , 's'},
        {"dst"    , required_argument , NULL , 'd'},
        {"regdump", required_argument , NULL , 'D'},
        {"file"   , required_argument , NULL , 'f'},
        {"help"   , no_argument       , NULL , 'h'},
        {"info"   , no_argument       , NULL , 'i'},
        {"read"   , required_argument , NULL , 'r'},
        {"write"  , required_argument , NULL , 'w'},
        {"verbose", optional_argument , NULL , 'v'},
        {"quiet"  , optional_argument , NULL , 'q'},
        {"version", no_argument       , NULL , 'V'},
        {"monitor", no_argument       , NULL , 'm'},
        {0        , 0                 , NULL , 0  }
    };

    const char short_opts[] = ":s:d:D:f:hir:w:v:qVm";
    while ((c = getopt_long(argc, argv, short_opts, long_opts, &option_index)) != -1) { //consume the arguments
        switch (c) {
            case 's':
                src_addr = strtoul(optarg, NULL, 16);
                break;

            case 'd':
                dst_addr = strtoul(optarg, NULL, 16);
                break;

//          case 'D':
//              target_file = optarg;
//              PrintRegDump=1;
//              break;

            case 'V':
                print_version_info(argv[0]);
                return 0;

            case 'q':
                dbg_lvl = -1;
                break;

            case 'v':
                switch (strtoul(optarg, NULL, 10)) {
                    case 0:
                        dbg_lvl = 0;
                        break;
                    case 1:
                        dbg_lvl = 1;
                        break;
                    case 2:
                        dbg_lvl = 2;
                        break;
                    case 3:
                        dbg_lvl = 3;
                        break;
                    case 4:
                        dbg_lvl = 4;
                        break;
                    default:
                        P_ERR("unknown verbosity level requeted(%s), [0-4]\n", optarg);
                        return -1;
                }
                break;

            case 'f':
                target_file = optarg;
                break;

            case 'r':
                addr = (uint16_t)strtoul(optarg, NULL, 16);
                ReadData = 1;
                break;

          //case 'w': //optarg = argv[optind-1]
          //    addr = (uint16_t)strtoul(argv[optind - 1], NULL, 16);
          //    data = (uint8_t) strtoul(argv[optind    ], NULL, 16);
          //    WriteData = 1;
          //    break;

            case 'i':
                PrintInfo = 1;
                break;

            case ':':
                switch (optopt){
                    case 'v': //defaulting to non-driver debug
                        dbg_lvl = 1;
                        break;
                    default:
                        P_ERR("option -%c is missing a required arg\n", optopt);
                        printhelp(argv[0]);
                        return -1;
                }
                break;

            default:
                P_ERR("%s: option `-%c' is invalid\n", argv[0], optopt);
            case 'h':
                printhelp(argv[0]);
                return 0;
                break;
        }
    }

    /* Display help dialog. */
    if (WriteHelpText) {
        printhelp(argv[0]);
        exit(EXIT_SUCCESS);
    }

    P_DBG(dbg_lvl, 0, "WriteData       = 0x%08x\n", WriteData      );
    P_DBG(dbg_lvl, 0, "ReadData        = 0x%08x\n", ReadData       );
    P_DBG(dbg_lvl, 0, "WriteHelpText   = 0x%08x\n", WriteHelpText  );
    P_DBG(dbg_lvl, 0, "PrintRegDump    = 0x%08x\n", PrintRegDump   );
    P_DBG(dbg_lvl, 0, "PrintInfo       = 0x%08x\n", PrintInfo      );
    P_DBG(dbg_lvl, 0, "src_addr        = 0x%08x\n", src_addr       );
    P_DBG(dbg_lvl, 0, "dst_addr        = 0x%08x\n", dst_addr       );
    P_DBG(dbg_lvl, 0, "dbg_lvl         = 0x%08x\n", dbg_lvl        );
    P_DBG(dbg_lvl, 0, "addr            = 0x%08x\n", addr           );
    P_DBG(dbg_lvl, 0, "data            = 0x%08x\n", data           );
    P_DBG(dbg_lvl, 0, "target_file     = %-10s\n" , target_file    );
    ///////////////////////////////////////////////////////////////////////////
    //
    if (target_file == NULL){
        P_ERR("target_file (-f) required\n");
        return EINVAL;
    }

    errno = 0;
    fd = fopen(target_file, "rwb+");
    if(!fd) {
        perror(ANSI_COLOR_RED "<err>");
        P_ERR("failed to open %s\n", target_file);
        return -1;
    }

    if (ReadData) {
       if(cz2_read(fd, addr)){
          P_ERR("failed to write to %s\n", target_file);
       }
    } else if (WriteData) {
        P_DBG(dbg_lvl, 1, "waiting our turn to write\n");
        while (get_char_from_stream(fd, d)){
            if (!rs485InputBuf.add(d))
                P_ERR("input buffer overrun!");

            if (processInputFrame() == EIDRM) { //inject after write ack
                P_NFO(dbg_lvl, "wr@ %04x\n", addr);
                goto cleanup;
            }
        }
    } 
 // else if (Update) {
 //    for (ii
 //    if(cz2_read(fd, 0x0601)){
 //       P_ERR("failed to read @ from %s\n", target_file);
 //    }
 // }

    P_DBG(dbg_lvl, 1, "starting monitor loop\n");
    do{
        while (get_char_from_stream(fd, d)){
            if (!rs485InputBuf.add(d)){
                P_ERR("input buffer overrun!");
                return ENOSR;
            }

            if (!processInputFrame())
                framecnt++;
        }
    } while (1);

    //lastReceivedMessageTimeMillis = millis();
cleanup:
    printf("\n");

    fclose(fd);
}


