#include "i2c.h"
#include <lpc17xx_i2c.h>
#include <lpc17xx_pinsel.h>

#define I2C_DEV LPC_I2C1

void i2c_init()
{
    PINSEL_CFG_Type pins;

    // Now attempt to set up some I2C stuff
    // Init the ports used for I2C
    pins.Funcnum = 3;     // I2C is function 3 for the following pins
    pins.OpenDrain = 0;   // Erm?
    pins.Pinmode = PINSEL_PINMODE_NORMAL;  // Standard pin 
    pins.Portnum = 0;     // Port 0
    pins.Pinnum = 0;      // Pin 0
    PINSEL_ConfigPin(&pins);  // Do config!
    pins.Pinnum = 1;
    PINSEL_ConfigPin(&pins);    

    // Now try and set up I2C
    I2C_Init(I2C_DEV, 100000);
    I2C_Cmd(I2C_DEV, ENABLE);    
//    NVIC_SetPriority(I2C1_IRQn, ((0x00<<3)|0x01));
//    NVIC_EnableIRQ(I2C1_IRQn);
}

void I2C1_IRQHandler(void)
{
    I2C_MasterHandler(I2C_DEV);
}

Status i2c_send(int devID, uint8_t* data, int data_len)
{
    I2C_M_SETUP_Type i2c; 

    // Try to send it
    i2c.sl_addr7bit = devID >> 1; // Device 0?
    i2c.tx_data = data;
    i2c.tx_length = data_len;
    i2c.rx_data = NULL;
    i2c.rx_length = 0;
    i2c.retransmissions_max = 3;
    //i2c.callback = 0;

    return I2C_MasterTransferData(I2C_DEV, &i2c, I2C_TRANSFER_POLLING);
}
