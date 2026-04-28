#include "SSD1306.h"
#include <stdint.h>
#include "fonts.h"

// =============================
// Command 전송
// =============================
void ssd1306_W_Command(uint8_t cmd)
{
    uint8_t buffer[2] = {0};
    buffer[0] = 0x00;  // Co=0, D/C=0
    buffer[1] = cmd;

    HAL_I2C_Master_Transmit(&ssd1306_I2C_PORT, (ssd1306_Address << 1), buffer, 2, HAL_MAX_DELAY);
}

// =============================
// Data 전송
// =============================
void ssd1306_W_Data(uint8_t* data_buffer, uint16_t buffer_size)
{
    HAL_I2C_Mem_Write(&ssd1306_I2C_PORT, (ssd1306_Address << 1), 0x40, 1, data_buffer, buffer_size, HAL_MAX_DELAY);
}

// =============================
// 초기화
// =============================
void ssd1306_Init(void)
{
    ssd1306_W_Command(0xA8); // MUX
    ssd1306_W_Command(0x3F);

    ssd1306_W_Command(0xD3);
    ssd1306_W_Command(0x00);

    ssd1306_W_Command(0x40);

    ssd1306_W_Command(0xA1);
    ssd1306_W_Command(0xC8);

    ssd1306_W_Command(0xDA);
    ssd1306_W_Command(0x12);

    ssd1306_W_Command(0x20);
    ssd1306_W_Command(0x02); // Page mode

    ssd1306_W_Command(0x81);
    ssd1306_W_Command(0x7F);

    ssd1306_W_Command(0xA4);
    ssd1306_W_Command(0xA6);

    ssd1306_W_Command(0xD5);
    ssd1306_W_Command(0x80);

    ssd1306_W_Command(0x8D);
    ssd1306_W_Command(0x14);

    ssd1306_W_Command(0xAF);
}

// =============================
// 화면 클리어
// =============================
void ssd1306_Clear(void)
{
    uint8_t buffer[128] = {0};

    for(uint8_t page = 0; page < 8; page++)
    {
        ssd1306_Set_Coord(page, 0);
        ssd1306_W_Data(buffer, 128);
    }
}

// =============================
// 화면 채우기
// =============================
void ssd1306_Fill_Screen(uint8_t data)
{
    uint8_t buffer[128];

    for(uint8_t i = 0; i < 128; i++)
        buffer[i] = data;

    for(uint8_t page = 0; page < 8; page++)
    {
        ssd1306_Set_Coord(page, 0);
        ssd1306_W_Data(buffer, 128);
    }
}

// =============================
// 좌표 설정
// =============================
void ssd1306_Set_Coord(uint8_t page, uint8_t col)
{
    ssd1306_W_Command(0xB0 + page);
    ssd1306_W_Command(0x00 + (col & 0x0F));
    ssd1306_W_Command(0x10 + (col >> 4));
}

void ssd1306_W_Char(uint8_t character_Code, uint8_t page, uint16_t column)
{
    const FontDef *font = &Font_7x10;
    uint8_t pages_per_char = (font->height + 7) / 8;   // 10픽셀 높이 -> 2페이지
    uint8_t char_Buffer[7 * 2] = {0};

    if (character_Code < 32 || character_Code > 126)
    {
        character_Code = '?';
    }

    const uint16_t *char_data = &font->data[(character_Code - 32) * font->height];

    for (uint8_t row = 0; row < font->height; row++)
    {
        uint16_t row_bits = char_data[row] >> (16 - font->width);

        for (uint8_t col = 0; col < font->width; col++)
        {
            if (row_bits & (1U << (font->width - 1 - col)))
            {
                char_Buffer[col + (row / 8) * font->width] |= (1U << (row % 8));
            }
        }
    }

    for (uint8_t p = 0; p < pages_per_char; p++)
    {
        ssd1306_Set_Coord(page + p, column);
        ssd1306_W_Data(&char_Buffer[p * font->width], font->width);
    }
}

void ssd1306_W_String(char *str, uint8_t page, uint8_t col)
{
    const FontDef *font = &Font_7x10;
    uint8_t pages_per_char = (font->height + 7) / 8;

    while (*str)
    {
        if ((col + font->width) > 127)
        {
            col = 0;
            page += pages_per_char;

            if ((page + pages_per_char - 1) > 7)
            {
                break;
            }
        }

        ssd1306_W_Char((uint8_t)*str, page, col);
        col += font->width;
        str++;
    }
}

// =============================
// 비트맵 출력
// =============================
void ssd1306_DrawBitmap(const uint8_t *bitmap, uint8_t page, uint8_t col, uint8_t width, uint8_t height)
{
    for(uint8_t i = 0; i < height/8; i++)
    {
        ssd1306_Set_Coord(page + i, col);
        ssd1306_W_Data((uint8_t*)&bitmap[i * width], width);
    }
}
