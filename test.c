#include <lpc17xx_gpio.h>
#include <lpc17xx_timer.h>
#include <lpc17xx_dac.h>
#include <lpc17xx_pinsel.h> 
#include <stdlib.h>

#include "i2c.h"
#include "lcd.h"
#include "sd.h"
#include "sd_defines.h"
#include "100ways.h"
#include "7seg.h"

#include "usbhost_lpc17xx.h"

#include <stdio.h>

#define DATA_LENGTH 6
#define SEG_ADDR 0x70
#define SCREEN_ADDR 0x76

#define READ_BLOCKS 2

volatile long x = 0;

uint8_t* buf;
int blockNum = 0;
int c = 0;
int b = 0;
int way = 0;
unsigned long int tim1_count = 0;
int path = 0;
char wayTxt[3] = "000";

void TIMER0_IRQHandler(void)
{
    uint16_t frame = 0;

    if(TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT) != SET)
    {
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
        return;
    }
    
    if(c >= READ_BLOCKS*512)
        c = 0;

    // Play it!
    frame = (buf[c++]);
    
    frame <<= 2;
    
    DAC_UpdateValue(LPC_DAC, frame);    
    
    TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
}

void TIMER1_IRQHandler(void)
{
    int way_len = 0;

    if(tim1_count != 0)
    {
        tim1_count--;
        // FUCK YEAH HACKY DELAYS!
        TIM_ClearIntPending(LPC_TIM1, TIM_MR0_INT);
        return;
    }

    if(TIM_GetIntStatus(LPC_TIM1, TIM_MR0_INT) != SET)
    {
        TIM_ClearIntPending(LPC_TIM1, TIM_MR0_INT);
        return;
    }

    if(path == 0)
    {
        // Check for a new way
        if(b > ways_offset[way])
        {
            lcd_seek(0);
            lcd_write_string("                ", 16);
            
            
            path++;
            tim1_count = 100;
        }
    }
    else if(path == 1)
    {
        lcd_seek(0x40);
        lcd_write_string("                ", 16);
        path++;
        tim1_count = 100;
    }    
    else if(path == 2)
    {
        seg_increment();
        path++;
        tim1_count = 100;
    }
    else if(path == 3)
    {
        lcd_seek(0);
        //                01234567890123456
        lcd_write_string("There are 100ways", 16);
        lcd_seek(0x40);
        lcd_write_string("  to love a cat. ", 16);
        path++;
        tim1_count = 400000;
    }
    else if(path == 4)
    {
        lcd_seek(0x00);
        lcd_write_string("                ", 16);
        path++;
        tim1_count = 100;
    }
    else if(path == 5)
    {
        lcd_seek(0x40);
        lcd_write_string("                ", 16);
        path++;
        tim1_count = 100;
    }
    else if(path == 6)
    {
        lcd_seek(0);
        lcd_write_string("100 loving ways" , 15);
        path++;
        tim1_count = 250000;
    }
    else if(path == 7)
    {
        lcd_seek(0x00);
        lcd_write_string("                ", 16);
        path++;
        tim1_count = 100;
    }
    else if(path == 8)
    {
        lcd_seek(0x40);
        lcd_write_string("                ", 16);
        path++;
        tim1_count = 100;
    }
    else if(path == 9)
    {
        wayTxt[2]++;
        if(wayTxt[2] == ':')
        {
            wayTxt[2] = '0';
            wayTxt[1]++;
            if(wayTxt[1] == ':')
            {
                wayTxt[0]++;
                wayTxt[1] = '0';
            }
        }

        path++;
        tim1_count = 100;
    }
    else if(path == 10)
    {
        lcd_seek(0x0);
        lcd_write_string("   Way ", 7);
        lcd_write_string(wayTxt, 3);
        tim1_count = 150000;
        path++;
    }
    else if(path == 11)
    {
        lcd_clear_screen();
        way_len = strlen(ways[way]);
        lcd_write_string(ways[way], way_len > 16 ? 16 : way_len);
        
        if(way_len > 16)
        {
            lcd_seek(0x40);
            lcd_write_string(((char*)ways[way])+16, way_len-16);
        }
        
        way++;
        path = 0;
        tim1_count = 0;
    }
        

    TIM_ClearIntPending(LPC_TIM1, TIM_MR0_INT);
}

int main()
{
    // Forward variable stuff
    uint8_t data[DATA_LENGTH];
    volatile unsigned int i = 0;

    uint32_t blocks, blockSize;
    uint8_t result[INQUIRY_LENGTH];
    
    int32_t rc;

    // Set up GPIO
    GPIO_SetDir(1, 0x1<<18 | 1<<20 | 1<<21 | 1<<23, 1); // Set pin 18 as an output
    GPIO_ClearValue(1, 0x1<<18 | 1<<20 | 1<<21 | 1<<23);

    GPIO_SetValue(1, 1<<20);
    // Analogue out

    i2c_init();
    lcd_init();
    seg_init();
    lcd_clear_screen();

    PINSEL_CFG_Type pins;
    pins.Funcnum = 2;     
    pins.OpenDrain = 0;   // Erm?
    pins.Pinmode = 0;
    pins.Portnum = 0;     // Port 0
    pins.Pinnum = 26;      // Pin 0
    PINSEL_ConfigPin(&pins);  // Do config!

    DAC_Init(LPC_DAC);
    DAC_SetBias(LPC_DAC, 0);
    DAC_UpdateValue(LPC_DAC, 1023);


    // Init USB
    Host_Init();
    rc = Host_EnumDev();
    if(rc != OK)
    {
        lcd_write_string_nl("Enum Failed");
        return 1;
    }

    rc = MS_Init(&blockSize, &blocks, result);

    if(rc != OK)
    {
        lcd_write_string_nl("MS Init Failed");
        return 1;
    }
    
    buf = (uint8_t*)malloc(READ_BLOCKS * blockSize);
    if(!buf)
    {
        lcd_write_string_nl("malloc fail");
        return 1;
    }

    // Init a timer
    TIM_TIMERCFG_Type t;
    t.PrescaleOption = TIM_PRESCALE_USVAL;
    t.PrescaleValue = 1;

    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &t);
    TIM_Init(LPC_TIM1, TIM_TIMER_MODE, &t);

    TIM_MATCHCFG_Type tt;
    tt.IntOnMatch = ENABLE;
    tt.MatchChannel = 0;
    tt.ResetOnMatch = ENABLE;
    tt.StopOnMatch = DISABLE;
    tt.MatchValue = 11;
    tt.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
    TIM_ConfigMatch(LPC_TIM0, &tt);
    TIM_ConfigMatch(LPC_TIM1, &tt);

    NVIC_SetPriority(TIMER0_IRQn, ((0x01<<3)|0x01));
    NVIC_EnableIRQ(TIMER0_IRQn);

    NVIC_SetPriority(TIMER1_IRQn, ((0x01<<3)|0x01));
    NVIC_EnableIRQ(TIMER1_IRQn);

    // Start reading the audio
    lcd_write_string_nl("Ready?");

    // Read a block
    MS_BulkRecv(b++, 1, buf);
    
    // Go!
    TIM_Cmd(LPC_TIM0, ENABLE);
    TIM_Cmd(LPC_TIM1, ENABLE);

    while(1)
    {
        // Read into the upper block
        MS_BulkRecv(b++, 1, buf+512);

        // Wait for the counter to hit the next block
        while(c <= 512);
        MS_BulkRecv(b++, 1, buf);
        while(c > 512);
    }

    i = 1;
    int dir = 0;

    // Ignore past this line...

    while(1)
    {
        while(TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT) == RESET);
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
        
        if(i%2 != 0)
            GPIO_ClearValue(1, 1<<23);
        else
            GPIO_SetValue(1, 1<<23);

        data[0] = 0;
        data[1] = 0x07;
        data[1] |= (i&0x7) << 4;
        i2c_send(SEG_ADDR, data, 2);

        if(dir == 0)
            i++;
        else
            i--;

        if(i == 8)
        {
            i = 7;
            dir = 1;
        }
        if(i == -1)
        {
            i = 0;
            dir = 0;
        }

    }

    while(1);

    return 0;
}
