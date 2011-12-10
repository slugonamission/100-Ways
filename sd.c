#include "sd.h"
#include "sd_defines.h"

#include "lcd.h"

#include <lpc17xx_pinsel.h>
#include <lpc17xx_gpio.h>
#include <lpc17xx_ssp.h>

#include <stdlib.h>

#define SSP LPC_SSP1

#define GETBIT(in, bit) ((in & (1<<bit)) >> bit)

// Forward decls
uint8_t crc_7(uint8_t old_crc, uint8_t data);
uint8_t crc_7final(uint8_t old_crc);

int sd_waitdeviceidle(uint32_t retries);
int sd_waitr1(uint8_t* data, uint32_t len, uint32_t retries);

#define SD_ON GPIO_ClearValue(0, (0x1<<11))
#define SD_OFF GPIO_SetValue(0, (0x1<<11))

int sd_init()
{
    int i;
    int retries = 0;
    int max_retries = 20;
    uint8_t sd_cmdbuf[4] = {0,0,0,0};
    uint8_t rxData;


    // Set up the SD pins
    PINSEL_CFG_Type pins;

    pins.Funcnum = 2;     
    pins.OpenDrain = 0;   
    pins.Pinmode = 0;
    pins.Portnum = 0;     
    pins.Pinnum = 7;      // Pin 9 - MOSI
    PINSEL_ConfigPin(&pins);  // Do config!
    pins.Pinnum = 8;      // 8 - MISO
    PINSEL_ConfigPin(&pins);    
    pins.Pinnum = 9;      // 7 - SCLK
    PINSEL_ConfigPin(&pins);
    
    pins.Pinnum = 11;     // 11 - SDCS
    pins.Funcnum = 0;
    PINSEL_ConfigPin(&pins);

    // Init the SPI periphial
    SSP_CFG_Type spi;
    SSP_ConfigStructInit(&spi); // Default the values
    spi.FrameFormat = SSP_FRAME_SPI; // SPI mode
    SSP_Init(SSP, &spi);
    SSP_Cmd(SSP, 1);

    // De-assert CS
    GPIO_SetDir(0, 1<<11, 1); // SDCS is an output
    //GPIO_SetValue(0, 0x1 << 11);  // Active low - don't assert the SD card
    SD_OFF;


    // Okay, now init the SD card
    if(!sd_waitdeviceidle(1000))
        return SD_INIT_BUS_BUSY;

    // Send CMD0
    while(retries < max_retries)
    {
        sd_sendcommand(CMD0_IDLE, sd_cmdbuf);
        
        // Wait for R1
        if(sd_waitr1(&rxData, 0, 1000) != SD_WAITR1_SUCCESS)
        {            
            retries++;
            continue;
        }

        if(rxData != R1_IDLE)
        {
            retries++;
            continue;
        }
        else
            break;
    }

    if(retries >= max_retries)
        return SD_INIT_NO_IDLE;

    // Start the SD card init procedure
    while(1)
    {
        sd_sendcommand(CMD55_APP_CMD, sd_cmdbuf);
        if(sd_waitr1(&rxData, 0, 1000) != SD_WAITR1_SUCCESS) return SD_INIT_CMD55;
        sd_sendcommand(ACMD41_SEND_OP_COND, sd_cmdbuf);
        sd_waitr1(&rxData, 0, 1000);
        if(rxData & R1_IDLE)
            for(i = 0;i<1000;i++);
        else
            break;
    }

    // Finally, enable CRC
    sd_cmdbuf[3] = 0x1;
    sd_sendcommand(CMD59_CRC_ON_OFF, sd_cmdbuf);
    if(sd_waitr1(&rxData, 0, 1000) != SD_WAITR1_SUCCESS) return SD_INIT_CMD59;

    return SD_INIT_SUCCESS;
}

// Wait for the SD card to become idle
int sd_waitdeviceidle(uint32_t retries)
{
    uint8_t rb[2] = {0,0};
    uint32_t i = 0;
    int status;

    retries = 100000;

    while((rb[0] != 0xff) && (i < retries))
    {
        rb[0] = 0x00;
        status = sd_txrx(NULL, rb, 1);
        
        if(rb[0] == 0xff)
        {
            rb[0] = 0x00;
            sd_txrx(NULL, rb, 1);
            if(rb[0] == 0xff)
            {
                rb[0] = 0x00;
                sd_txrx(NULL, rb, 1);
            }
        }
        i++;
    }

    if(rb[0] == 0xff)
        return 1;
    else
        return 0;
}


/* **********************************************
 * Lifted from the example drivers as I
 * couldn't be bothered to write my own implementation
 * ********************************************** */
uint8_t crc_7(uint8_t old_crc, uint8_t data)
{
    uint8_t new_crc,x;
    
    new_crc = old_crc;
    for (x = 7; x >= 0; x--) {
        new_crc <<= 1;
        new_crc |= GETBIT(data,x);
        if (GETBIT(new_crc, 7) == 1) {
            new_crc ^= 0x89; /*CRC-7's polynomial is x^7 + x^3 + 1*/
        }
        if(x==0) break;
    }
    return new_crc;
}


uint8_t crc_7final(uint8_t old_crc)
{
    uint8_t new_crc,x;
    
    new_crc = old_crc;
    for (x = 0; x < 7; x++) {
        new_crc <<= 1;
        if (GETBIT(new_crc, 7) == 1) {
            new_crc ^= 0x89; /*CRC-7's polynomial is x^7 + x^3 + 1*/
        }
    }
    return new_crc;
}


/* Back to my code */
int sd_sendcommand(uint8_t cmd, uint8_t* args)
{
    uint8_t crc = 0;
    int i = 0;
    uint8_t* buf = malloc(sizeof(uint8_t)*6);
    
    buf[0] = cmd | 0x40; // Assert the correct start bits
    for(i = 1;i<5;i++)
        buf[i] = args[i-1];

    // Figure out the CRC
    for(i = 0;i<5;i++)
        crc = crc_7(crc, args[i]);
    crc = crc_7final(crc);

    buf[5] = (crc<<1) | 0x1;  // Assert the stop bit

    i = sd_txrx(buf, NULL, 6);

    free(buf);

    return i;
}

int sd_txrx(uint8_t* tx, uint8_t* rx, uint32_t count)
{
    uint32_t status;
    // Set up the SPI transaction
    SSP_DATA_SETUP_Type data;
    
    SD_ON;
    for(status = 0;status<1000;status++);
    
    data.tx_data = (void*)tx;
    data.rx_data = (void*)rx;
    data.length = count;
    
    status = SSP_ReadWrite(SSP, &data, SSP_TRANSFER_POLLING);
    SD_OFF;

    return status;
}

int sd_waitr1(uint8_t* data, uint32_t len, uint32_t retries)
{
    uint8_t rb[2];
    int i;
    int j;

    rb[0] = 0xff;
    i = 0;

    while(GETBIT(rb[0], 7) == 1) // Wait for start sequence
    {
        if(i > retries)
            return SD_WAITR1_TIMEOUT;
        rb[0] = 0x00;
        sd_txrx(NULL, rb, 1);

        //lcd_write_string(rb, 1);
        i++;
    }

    // We should now have R1
    *data = rb[0];

    if(len > 0) // Read anything after it
    {
        rb[0] = 0xff; // Wait for the data portion start bits
        i = 0;
        while(rb[0] != 0xfe)
        {
            if(i > retries)
                return SD_WAITR1_TIMEOUT;
            rb[0] = 0x00;
            sd_txrx(NULL, rb, 1);
            if(rb[0] != 0xff && rb[0] != 0xfe)
                return SD_WAITR1_TOKEN;
            i++;
        }

        sd_txrx(NULL, (data+1), len-1);
    }

    // Wait for internal SD operations to finish
    rb[0] = 0x00;
    i = 0;
    while(rb[0] != 0xff && i < 20)
    {
        rb[0] = 0x00;
        sd_txrx(NULL, rb, 1);
        for(j = 0;j<1000;j++);
        i++;
    }

    if(i >= 20)
        return SD_WAITR1_BUS_BUSY;
    
    return SD_WAITR1_SUCCESS;
}
