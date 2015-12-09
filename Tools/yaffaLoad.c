#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdint.h>

#include "serialComms.h"
#include "client.h"

// extern int errno;
extern int enQueue(char *,char *,int );

void usage() {
    printf("\n\tUsage\n");

    printf("\t-d              Debug Mode\n");
    printf("\t-f <filename>   File to send\n");
    printf("\t-h|-?           Help\n");
    printf("\t-s <port name>  Set serial Port\n");
    printf("\t-v              Verbose\n");

    exit(0);
}

void waitForEol(int fd ) {  // EOL is '| ' '>> '
    uint8_t c;
    uint8_t pidx=0; // pipe index
    uint8_t cidx=0; // chevron index

    while( true ) {
        usleep(5000);
        read(fd, &c,1);

        if ( c == '|' ) {
            pidx++;
            cidx=0;
        }

        if( c == ' ' && pidx == 1 ) {
            return;
        }

        if( c == '>' ) {
            cidx++;
            pidx=0;
        }

        if ( c == ' ' && cidx == 1 ) {
            cidx=0;
        }
        if( c == ' ' && cidx == 2 ) {
            return;
        }
    }
}

int main( int argc, char *argv[]) {
    int opt;
    int debug =  1;
    int verbose = 1;
    char *serialPort=(char *)NULL;
    int ser;

    char outBuffer[255];
    char inBuffer[255];

    char fileName[PATH_MAX];
    bool fileNameSet=false;
    bool exitFlag = false;
    char *ptr;
    FILE *toSend;
    int len;
    char test[3];
    uint8_t idx=0;

    while ((opt = getopt(argc, argv, "df:h?s:v")) != -1) {
        switch(opt) {
            case 'd':
                debug=1;
                break;
            case 'f':
                strcpy(fileName,optarg);
                fileNameSet=true;
                break;
            case 'v':
                verbose=1;
                printf("\nVerbose mode on.\n");
                break;
            case 'h':
            case '?':
                usage();
                break;
            case 's':
                serialPort = (char *)malloc(strlen(optarg)+1);

                if( (char *)NULL == serialPort) {
                    perror("Failed to allocate memory for serial port");
                    exit(1);
                }
                strcpy(serialPort, optarg);
                break;
            default:
                break;
        }

    }

    if( false == fileNameSet) {
        fprintf(stderr,"You need to specify a file to send\n");
        exit(-1);
    }

    toSend=fopen(fileName,"r");
    if(!toSend) {
        fprintf(stderr,"Failed top open file %s\n", fileName);
        perror("Send File");
        exit(-1);
    }

    if(verbose) {
        printf("\nSerial Port:%s\n",serialPort);
    }
    ser = open (serialPort, O_RDWR | O_NOCTTY | O_SYNC);

    if( 0 > ser) {
        printf("Failed to open serial port:%d\n",errno);
        perror("failed");
        exit(2);
    }

    setInterfaceAttribs (ser, B19200, 0);
    setBlocking (ser, 1);

    write(ser, "\r", 1);

    idx=0;
    while( exitFlag == false ) { // looking for '>> '
        read( ser, outBuffer, 1);

        if( outBuffer[0] == 0) {
            exitFlag = true;
        }

        if( outBuffer[0] != '>' && outBuffer[0] != ' ') {
            idx=0;
        }
        if( outBuffer[0] == '>' ) {
            idx++;
        }
        if( outBuffer[0] == ' ' ) {
            idx++;
        }
        if( idx == 3 ) {
            exitFlag = true;
        }
    }

    ptr = outBuffer;
    while( ptr != NULL) {
        memset( outBuffer,0,sizeof(outBuffer));
        ptr = fgets( outBuffer, 255, toSend );

        if( strlen(outBuffer) > 1 ) {
            fprintf(stderr,"%s", ptr);
            usleep(5000); // wait 1 ms
            len = write( ser, outBuffer, strlen( outBuffer ));
            write( ser,"\r",1 );
            read( ser, inBuffer, len );
            read( ser, outBuffer, 1);
            waitForEol( ser );
        }
    }
    write( ser,"\x1a",1 );

    return(0);
}
