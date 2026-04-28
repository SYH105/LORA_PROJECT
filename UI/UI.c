#include "UI.h"
#include "SSD1306.h"
#include "fonts.h"
#include <stdio.h>
#include <string.h>

/* =========================
 * 설정값
 * ========================= */
#define LINE1_MAX   13
#define LINE2_MAX   18
#define MAX_LEN     (LINE1_MAX + LINE2_MAX)

/* =========================
 * RX 화면
 * ========================= */
void UI_DrawRX(const char* msg, uint8_t id)
{
    char buf[20];

    ssd1306_Clear();

    ssd1306_W_String("LORA RX", 0, 0);
    ssd1306_W_String("MSG:", 2, 0);
    ssd1306_W_String((char*)msg, 2, 35);

    snprintf(buf, sizeof(buf), "ID:%02X", id);
    ssd1306_W_String(buf, 6, 0);
}
/* =========================
 * MENU 화면
 * ========================= */
void UI_DrawMenu(uint8_t menu_index)
{
    ssd1306_Clear();

    ssd1306_W_String("MENU", 0, 0);

    if(menu_index == 0) ssd1306_W_String(">1.SEND", 2, 0);
    else                ssd1306_W_String(" 1.SEND", 2, 0);

    if(menu_index == 1) ssd1306_W_String(">2.SAVE", 4, 0);
    else                ssd1306_W_String(" 2.SAVE", 4, 0);

    if(menu_index == 2) ssd1306_W_String(">3.LOAD", 6, 0);
    else                ssd1306_W_String(" 3.LOAD", 6, 0);
}
/* =========================
 * TX 화면
 * ========================= */
void UI_DrawTX(const char* buf, uint8_t cursor)
{
    char line1[LINE1_MAX + 1];
    char line2[LINE2_MAX + 1];
    uint8_t i;

    /* =========================
     * 1. 커서 보호
     * ========================= */
    if(cursor > MAX_LEN)
    {
        cursor = MAX_LEN;
    }

    /* =========================
     * 2. 공백으로 초기화
     * ========================= */
    for(i = 0; i < LINE1_MAX; i++)
        line1[i] = ' ';
    line1[LINE1_MAX] = '\0';

    for(i = 0; i < LINE2_MAX; i++)
        line2[i] = ' ';
    line2[LINE2_MAX] = '\0';

    /* =========================
     * 3. 데이터 복사
     * ========================= */
    for(i = 0; i < LINE1_MAX; i++)
    {
        if(buf[i] == '\0') break;
        line1[i] = buf[i];
    }

    for(i = 0; i < LINE2_MAX; i++)
    {
        if(buf[LINE1_MAX + i] == '\0') break;
        line2[i] = buf[LINE1_MAX + i];
    }

    /* =========================
     * 4. 커서 표시
     * ========================= */
    if(cursor < MAX_LEN)
    {
        if(cursor < LINE1_MAX)
        {
            line1[cursor] = '_';
        }
        else
        {
            line2[cursor - LINE1_MAX] = '_';
        }
    }

    /* =========================
     * 5. 화면 출력
     * ========================= */
    ssd1306_Clear();

    ssd1306_W_String("SEND MODE", 0, 0);

    ssd1306_W_String("MSG:", 2, 0);
    ssd1306_W_String(line1, 2, 32);

    ssd1306_W_String(line2, 4, 0);

    ssd1306_W_String("SEL:SEND,DEL:BACK", 6, 0);
}
/* =========================
 * SAVE EDIT 화면
 * ========================= */
void UI_DrawSaveEdit(uint8_t idx, const char* buf, uint8_t cursor)
{
    char line1[LINE1_MAX + 1];
    char line2[LINE2_MAX + 1];
    uint8_t i;

    /* =========================
     * 1. 초기화 (공백 채우기)
     * ========================= */
    for(i = 0; i < LINE1_MAX; i++) line1[i] = ' ';
    line1[LINE1_MAX] = '\0';

    for(i = 0; i < LINE2_MAX; i++) line2[i] = ' ';
    line2[LINE2_MAX] = '\0';

    /* =========================
     * 2. 데이터 복사
     * ========================= */
    for(i = 0; i < LINE1_MAX; i++)
    {
        if(buf[i] == '\0') break;
        line1[i] = buf[i];
    }

    for(i = 0; i < LINE2_MAX; i++)
    {
        if(buf[LINE1_MAX + i] == '\0') break;
        line2[i] = buf[LINE1_MAX + i];
    }

    /* =========================
     * 3. 커서 표시
     * ========================= */
    if(cursor < MAX_LEN)
    {
        if(cursor < LINE1_MAX)
        {
            line1[cursor] = '_';
        }
        else
        {
            line2[cursor - LINE1_MAX] = '_';
        }
    }

    /* =========================
     * 4. 화면 출력
     * ========================= */
    ssd1306_Clear();

    char title[16];
    snprintf(title, sizeof(title), "Slot%d EDIT", idx + 1);
    ssd1306_W_String(title, 0, 0);

    ssd1306_W_String("MSG:", 2, 0);
    ssd1306_W_String(line1, 2, 32);
    ssd1306_W_String(line2, 4, 0);

    ssd1306_W_String("#:SAVE *:CANCEL", 6, 0);
}
/* =========================
 * LOAD 화면
 * ========================= */
void UI_DrawSlot(uint8_t idx, const char* buf)
{
    char line1[14] = {0};
    char line2[19] = {0};

    ssd1306_Clear();

    char title[10];
    sprintf(title, "Slot%d", idx + 1);
    ssd1306_W_String(title, 0, 0);

    ssd1306_W_String("MSG:", 2, 0);

    /* 1줄 */
    strncpy(line1, buf, 13);
    ssd1306_W_String(line1, 2, 32);

    /* 2줄 */
    strncpy(line2, &buf[13], 18);
    ssd1306_W_String(line2, 4, 0);
}
