#include <stdio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <termios.h>

#define SOR 0x5a
#define EOR 0x5b
#define ESC 0x5c

#define LAPRF_TOR_ERROR         0xFFFF
#define LAPRF_TOR_RSSI          0xda01
#define LAPRF_TOR_RFSETUP       0xda02
#define LAPRF_TOR_STATE_CONTROL 0xda04
#define LAPRF_TOR_PASSING       0xda09
#define LAPRF_TOR_SETTINGS      0xda07
#define LAPRF_TOR_STATUS        0xda0a
#define LAPRF_TOR_TIME          0xDA0C

enum laprf_frame_state {
    LAPRF_FRAME_HUNT,
    LAPRF_FRAME_SYNC,
    LAPRF_FRAME_ESC
};

#if 0
typedef struct {
    uint16_t len;
    uint16_t crc;
    uint16_t type;
    uint8_t signature;
    uint8_t numbytes;
    uint8_t data[8];
}frame_t;
#endif
typedef struct {
    uint16_t len;
    uint16_t crc;
    uint16_t type;
    uint8_t data[256];
}frame_t;
typedef struct {
    uint8_t signature;
    uint8_t numbytes;
    uint8_t data[8];
}param_t;

uint8_t state = LAPRF_FRAME_HUNT;
uint8_t frame_buf[2048] = {0};
uint16_t frame_len = 0;

uint8_t pilotid;
uint32_t passingNumber;
uint64_t rtctime;

float minRssi;
float maxRssi;
float meanRssi;
float lastRssi;



void laprf_init(void)
{
    
}

int laprf_protocol_passing(uint8_t *buf, char *message)
{
 param_t *param = (param_t*)buf;
 char date[64];
    switch (param->signature){
        case 0x01:
            sprintf(&message[strlen(message)], "pilotid:%d ", param->data[0]);
            break;
        case 0x21:
            if (param->numbytes == 4){
                sprintf(&message[strlen(message)], "passingNumber:%d ", 
                    ((uint32_t)param->data[0]) |
                    ((uint32_t)param->data[1]<<8) | 
                    ((uint32_t)param->data[2]<<16) | 
                    ((uint32_t)param->data[3]<<24));
            }
            break;
        case 0x02:
            if (param->numbytes == 8){
                uint64_t rtctime =  ((uint64_t)param->data[0]) | 
                                    ((uint64_t)param->data[1]<<8) | 
                                    ((uint64_t)param->data[2]<<16) | 
                                    ((uint64_t)param->data[3]<<24) |
                                    ((uint64_t)param->data[4]<<32) | 
                                    ((uint64_t)param->data[5]<<40) | 
                                    ((uint64_t)param->data[6]<<48) | 
                                    ((uint64_t)param->data[7]<<56);
                time_t t = rtctime/100;
                strftime(date, sizeof(date), "%Y/%m/%d %a %H:%M:%S", localtime(&t));
                sprintf(&message[strlen(message)], "%s.%03lld ", date, rtctime%100);
            }
            break;
    }
    return param->numbytes;
}
int laprf_protocol_status(uint8_t *buf, char *message)
{
 param_t *param = (param_t*)buf;
    switch (param->signature){
        case 0x21:          // INPUT_VOLTAGE
            if (param->numbytes == 2){
                float batteryVoltage = ((param->data[0]) | (param->data[1]<<8)) / 1000.0f;
                sprintf(&message[strlen(message)],"vbat : %f ", batteryVoltage);
            }
            break;
    }
    return param->numbytes;
}

int laprf_protocol(uint8_t *buf, char *message)
{
 int ret = 0;
 int len = 0;
 frame_t *frame = (frame_t*)buf;

    switch(frame->type){
        case LAPRF_TOR_PASSING:
            sprintf(message, "pass:");
            ret = frame->len;
            while((len+8) < frame->len){
                len += laprf_protocol_passing(&frame->data[len], message);
            }
            strcat(message, "\r\n");
            break;
        case LAPRF_TOR_STATUS:
            sprintf(message, "pass:");
            ret = frame->len;
            while((len+8) < frame->len){
                len += laprf_protocol_status(&frame->data[len], message);
            }
            strcat(message, "\r\n");
            break;
    }
    return ret;
}



int laprf_data(uint8_t ch, char *message)
{
 int ret = 0;
    switch(state){
        case LAPRF_FRAME_HUNT:
            if (ch == SOR){
                state = LAPRF_FRAME_SYNC;
                frame_len = 0;
            }
            break;
        case LAPRF_FRAME_SYNC:
            switch(ch){
                case EOR:
                    ret = laprf_protocol(frame_buf, message);
                    strcat(message, "\r\n");
                    // パケット処理
                    state = LAPRF_FRAME_HUNT;
                    break;
                case ESC:
                    state = LAPRF_FRAME_ESC;
                    break;
                default:
                    frame_buf[frame_len++] = ch;
                    break;
            }
            break;
        case LAPRF_FRAME_ESC:
            frame_buf[frame_len++] = ch - 0x40;
            state = LAPRF_FRAME_SYNC;
            break;
    }

    if (frame_len >= sizeof(frame_buf)){
        state = LAPRF_FRAME_HUNT;
        frame_len = 0;
        ret = -1;
    }
    return ret;
}


