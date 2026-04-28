#ifndef SSD1306_H_
#define SSD1306_H_

#include "stm32f1xx_hal.h"
#include "fonts.h"

#define ssd1306_I2C_PORT hi2c1
#define ssd1306_Address 0x3C
#define font_width 7

extern I2C_HandleTypeDef hi2c1;
extern void Error_Handler(void);

void ssd1306_Init(void);
void ssd1306_W_Command(uint8_t cmd);
void ssd1306_W_Data(uint8_t* data_buffer, uint16_t buffer_size);
void ssd1306_Clear(void);
void ssd1306_Fill_Screen(uint8_t data);
void ssd1306_Set_Coord(uint8_t page, uint8_t col);
void ssd1306_W_Char(uint8_t ch, uint8_t page, uint16_t col);
void ssd1306_W_String(char *str, uint8_t page, uint8_t col);
void ssd1306_DrawBitmap(const uint8_t* bitmap, uint8_t page, uint8_t col, uint8_t width, uint8_t height);

#endif
