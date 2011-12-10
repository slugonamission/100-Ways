#ifndef __SD_DEFINES_H__
#define __SD_DEFINES_H__

// Commands
#define CMD0_IDLE 0x00
#define CMD55_APP_CMD 0x37  // Application specific command
#define ACMD41_SEND_OP_COND 0x29
#define CMD59_CRC_ON_OFF 0x3b

#define R1_IDLE 0x01

// Errors
#define SD_INIT_SUCCESS 0    // Successful init
#define SD_INIT_BUS_BUSY 1   // WaitDeviceIdle failed
#define SD_INIT_NO_IDLE 2    // Sending CMD0 failed
#define SD_INIT_CMD55 3
#define SD_INIT_CMD59 4

#define SD_WAITR1_SUCCESS 0
#define SD_WAITR1_TIMEOUT 1
#define SD_WAITR1_TOKEN 2
#define SD_WAITR1_BUS_BUSY 3

#endif
