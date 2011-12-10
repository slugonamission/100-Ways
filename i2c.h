#ifndef __I2C_H__
#define __I2C_H__

#include <lpc_types.h>

void i2c_init();
Status i2c_send(int devID, uint8_t* data, int data_len);

#endif
