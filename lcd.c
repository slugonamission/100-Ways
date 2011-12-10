#include "i2c.h"
#include "lcd.h"
#include <stdlib.h>
#include <lpc_types.h>
#include <string.h>

#define SCREEN_ADDR 0x76

void lcd_write_string_nl(char* str)
{
    int l = strlen(str);
    lcd_write_string(str, l);
}

void lcd_write_string(char* str, int length)
{
    int i;
    uint8_t* command = malloc(sizeof(uint8_t)*(length+1));
    if(!command)
        return;
    
    command[0] = 0x40; // RS
    for(i = 0;i<length;i++)
        command[i+1] = str[i] | 0x80;

    i2c_send(SCREEN_ADDR, command, length+1);

    free(command);
}

void lcd_seek(uint8_t addr)
{
    uint8_t command[2];
    command[0] = 0x00;
    command[1] = addr | 0x80;
    
    i2c_send(SCREEN_ADDR, command, 2);
}

void lcd_clear_screen()
{
    char command[0x4F];
    uint8_t command2[2];
    int i;

    for(i = 0;i<0x4F;i++)
        command[i] = ' ';

    lcd_write_string(command, 0x4F);

    // Go home
    command2[0] = 0x00;
    command2[1] = 0x02;
    i2c_send(SCREEN_ADDR, command2, 2);
}

void lcd_init()
{
    uint8_t screen_data[13];

    screen_data[0] = 0x00; // Command byte
    screen_data[1] = 0x34; // Function set, H=0, 16x2
    screen_data[2] = 0x0c; // Screen on, cursor on, no blink
    screen_data[3] = 0x06; // Move cursor right on write, no shift
    screen_data[4] = 0x35; // Function set, H=1
    screen_data[5] = 0x04; // Display config, left-to-right, top-to-bottom
    screen_data[6] = 0x10; // Temp co-efficient 0
    screen_data[7] = 0x42; // HVgen 3 stage
    screen_data[8] = 0x9f; // VLCD vB = 0xF
    screen_data[9] = 0x34; // Function set, H=0
    screen_data[10] = 0x80; // DRAM addr = 0
    screen_data[11] = 0x02; // Return home
    screen_data[12] = 0x01; // Clear screen

    i2c_send(SCREEN_ADDR, screen_data, 13);
}
