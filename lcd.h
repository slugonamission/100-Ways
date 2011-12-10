#ifndef __LCD_H__
#define __LCD_H__

#include <lpc_types.h>

void lcd_write_string_nl(char* str);
void lcd_write_string(char* str, int length);
void lcd_seek(uint8_t addr);
void lcd_clear_screen();
void lcd_init();

#endif
