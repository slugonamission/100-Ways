#ifndef __SD_H__
#define __SD_H__

#include <lpc_types.h>

int sd_init();
int sd_sendcommand(uint8_t cmd, uint8_t* args);
int sd_txrx(uint8_t* tx, uint8_t* rx, uint32_t count);


#endif
