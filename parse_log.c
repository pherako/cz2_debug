

#include "ComfortZoneII.hpp"

#include <stdio.h>
#include <unistd.h>
#include "RingBuffer.hpp"

RingBuffer rs485InputBuf;
RingBuffer rs485OutputBuf;

int main (int argc, char * argv[]){
    FILE* fd = 0;
    if (argc != 2) {
        printf("<err> filename required\n");
        return -1;
    } else {
        fd = fopen(argv[1], "r");
        if(!fd) {
            printf("<err> failed to open %s\n", argv[1]);
            return -1;
        }

    }

    char c;
    int cnt;
    while(fread(&c, sizeof(c), 1, fd)){
        if (cnt % 16 == 0)
            printf("\n %08x", cnt);
        if (cnt % 8 == 0)
            printf(" ");

        printf("%02x ", ~c & 0xff);
        cnt++;
    }
    printf ("\n read %d bytes from %s", cnt, argv[1]);

    fclose(fd);
    return 0;
}
