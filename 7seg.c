#include "7seg.h"
#include "i2c.h"
#include <lpc_types.h>

#define SEG_ADDR 0x70

uint8_t lut[10] = {
    0x3F,
    0x06,
    0x5B,
    0x4F,
    0x66,
    0x6D,
    0x7C,
    0x07,
    0x7F,
    0x6F
};

int acc1;
int acc2;
int acc3;

void seg_init()
{
    uint8_t arr[6];

    acc1 = acc2 = acc3 = 0;

    i2c_send(SEG_ADDR, arr, 2);

    arr[0] = 0x0;
    arr[1] = 0x77;
    arr[2] = 0x0;
    arr[3] = 0x0;
    arr[4] = 0x0;
    arr[5] = 0x0;

    i2c_send(SEG_ADDR, arr, 6);
}

void seg_increment()
{
    uint8_t arr[2];
    
    acc1++;
    if(acc1 > 9)
    {
        acc1 = 0;
        acc2++;
        if(acc2 > 9)
        {
            acc3++;
            arr[0] = 0x2;
            arr[1] = lut[acc3];
            i2c_send(SEG_ADDR, arr, 2);
        }
        arr[0] = 0x3;
        arr[1] = lut[acc2];
        i2c_send(SEG_ADDR, arr, 2);
    }
    arr[0] = 0x4;
    arr[1] = lut[acc1];
    i2c_send(SEG_ADDR, arr, 2);
}
