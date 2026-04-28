#include "app.h"
#include "SSD1306.h"
#include "fonts.h"
#include "KEYPAD.h"
#include "SX1276.h"
#include "UI.h"
#include "FLASH.h"
#include <stdio.h>
#include <string.h>

/* =========================
 * 통신 프로토콜 정의
 * =========================
 * [H1][H2][ID][LEN][DATA...][CHK]
 *
 * H1, H2 : 고정 헤더 (패킷 시작 식별)
 * ID     : 송신자 ID
 * LEN    : 데이터 길이
 * DATA   : 실제 메시지
 * CHK    : XOR 체크섬
 */
#define PROTO_H1  0x08
#define PROTO_H2  0x06

#define MY_ID     0x01   // 현재 기기 ID

#define TYPE_TEXT 0x01

/* 마지막으로 수신한 송신자 ID */
static uint8_t last_rx_id = 0;

static uint8_t BuildPacket(uint8_t *out, char *msg)
{
    uint8_t len = strlen(msg);
    uint8_t idx = 0;
    uint8_t chk = 0;

    out[idx++] = PROTO_H1;
    out[idx++] = PROTO_H2;
    out[idx++] = MY_ID;
    out[idx++] = len;

    for(int i = 0; i < len; i++)
    {
        out[idx++] = msg[i];
    }

    // checksum (간단 XOR)
    for(int i = 0; i < idx; i++)
    {
        chk ^= out[i];
    }

    out[idx++] = chk;

    return idx; // 전체 길이
}

/* =========================
 * 상태 머신 관련 변수
 * ========================= */
static app_state_t state;         // 현재 상태
static app_state_t prev_state = -1; // 이전 상태 (화면 1회 갱신용)

static uint8_t menu_index = 0;    // 메뉴 선택 인덱스
static uint32_t tx_done_time = 0; // TX 완료 시간 (딜레이용)

/* =========================
 * LoRa 디바이스 핸들
 * ========================= */
static sx1276_t lora;
extern SPI_HandleTypeDef hspi2;

/* =========================
 * 저장 슬롯 관리
 * ========================= */
#define SLOT_COUNT 5
#define MSG_LEN    31

static char slot_buf[SLOT_COUNT][MSG_LEN + 1]; // 슬롯 저장 버퍼
static uint8_t slot_index = 0;                // 현재 선택 슬롯

static uint8_t save_edit_mode = 0;  // 0: 보기 / 1: 편집

/* =========================
 * RX / TX 데이터 버퍼
 * ========================= */
static char rx_msg[32];                 // 수신 메시지

static char tx_buf[MAX_LEN + 1] = {0}; // 송신 입력 버퍼
static uint8_t cursor = 0;             // TX 커서 위치

/* =========================
 * 내부 함수 선언
 * ========================= */
static void TX_ClearBuffer(void);
static void TX_AddChar(char key);
static void TX_DeleteChar(void);
static void ProcessReceivedPacket(uint8_t *buf, uint8_t len);
/* =========================
 * 데이터 수신 시 부저음
 * ========================= */
extern void Buzzer_On(void);
extern void Buzzer_Off(void);

/* =========================
 * TX 버퍼 제어
 * ========================= */
static void TX_ClearBuffer(void)
{
    memset(tx_buf, 0, sizeof(tx_buf));
    cursor = 0;
}

static void TX_AddChar(char key)
{
    if(cursor >= MAX_LEN)
    {
        /* 꽉 찼으면 완전히 무시 */
        cursor = MAX_LEN;
        tx_buf[MAX_LEN] = '\0';
        return;
    }

    tx_buf[cursor] = key;
    cursor++;
    tx_buf[cursor] = '\0';
}

static void TX_DeleteChar(void)
{
    if(cursor == 0)
    {
        return;
    }

    cursor--;
    tx_buf[cursor] = '\0';
}


/* =========================
 * 초기화
 * ========================= */
void APP_Init(void)
{
    state = STATE_RX;
    menu_index = 0;
    TX_ClearBuffer();
    UI_DrawRX(rx_msg, last_rx_id);
    sx1276_init(&lora, &hspi2, GPIOA, GPIO_PIN_8, SX1276_FREQ_915MHZ);
    FLASH_LoadSlots(slot_buf);
}

/* =========================
 * 메인 상태머신
 * ========================= */
void APP_Run(void)
{
    char key = KEYPAD_Get_Key();

    /* =========================
     * 1. 상태 진입 처리 (1회)
     * ========================= */
    if(prev_state != state)
    {
        switch(state)
        {
            case STATE_RX:
                TX_ClearBuffer();
                sx1276_set_rx_continuous(&lora);
                UI_DrawRX(rx_msg, last_rx_id);
                break;

            case STATE_MENU:
                TX_ClearBuffer();
                UI_DrawMenu(menu_index);
                break;

            case STATE_TX:
                TX_ClearBuffer();
                UI_DrawTX(tx_buf, cursor);
                break;

            case STATE_TX_DONE:
                TX_ClearBuffer();
                ssd1306_Clear();
                ssd1306_W_String("SEND OK!", 2, 0);
                break;

            case STATE_SAVE:
                save_edit_mode = 0;
                slot_index = 0;
                UI_DrawSlot(slot_index, slot_buf[slot_index]);
                break;

            case STATE_LOAD:
            	slot_index = 0;
            	UI_DrawSlot(slot_index, slot_buf[slot_index]);
                break;
        }

        prev_state = state;
    }

    /* =========================
     * 2. 상태 동작 처리
     * ========================= */
    switch(state)
    {
        /* ================= RX ================= */
        case STATE_RX:
        {
            if(sx1276_is_packet_available(&lora))
            {
                uint8_t buf[32];
                uint8_t err;

                uint8_t len = sx1276_fetch_received(&lora, buf, sizeof(buf), &err);

                if(err == SX1276_OK)
                {
                    if(len >= sizeof(buf)) len = sizeof(buf) - 1;
                    buf[len] = '\0';

                    //수신데이터 처리
                    ProcessReceivedPacket(buf, len);

                    UI_DrawRX(rx_msg, last_rx_id);

                    /* 수신 알림 비프 (삐삐) */
                    for(int i = 0; i < 2; i++)
                    {
                        Buzzer_On();
                        HAL_Delay(80);
                        Buzzer_Off();
                        HAL_Delay(80);
                    }

                }
            }

            if(key == '#')
            {
                state = STATE_MENU;
                return;
            }
        }
        break;

        /* ================= MENU ================= */
        case STATE_MENU:
        {
            if(key == 'B')
            {
                if(menu_index > 0) menu_index--;
                UI_DrawMenu(menu_index);
                return;
            }
            else if(key == 'J')
            {
                if(menu_index < 2) menu_index++;
                UI_DrawMenu(menu_index);
                return;
            }
            else if(key == '#')
            {
                if(menu_index == 0) state = STATE_TX;
                else if(menu_index == 1) state = STATE_SAVE;
                else if(menu_index == 2) state = STATE_LOAD;
                return;
            }
            else if(key == '*')
            {
                state = STATE_RX;
                return;
            }
        }
        break;

        /* ================= TX ================= */
        case STATE_TX:
        {
            if(key != KEYPAD_NOT_PRESSED)
            {
                if(key == '*')
                {
                    state = STATE_MENU;
                    return;
                }
                else if(key == '#')
                {
                    if(strlen(tx_buf) == 0) return;

                    uint8_t packet[64];
                    uint8_t len = BuildPacket(packet, tx_buf);

                    sx1276_start_tx(&lora, packet, len);
                    state = STATE_TX_DONE;
                    tx_done_time = HAL_GetTick();
                    return;
                }
                else if(key == '&')
                {
                    TX_DeleteChar();
                    UI_DrawTX(tx_buf, cursor);
                    return;
                }

                TX_AddChar(key);
                UI_DrawTX(tx_buf, cursor);
                return;
            }
        }
        break;

        /* ================= TX DONE ================= */
        case STATE_TX_DONE:
        {
            if(key == '*')
            {
                state = STATE_MENU;
                return;
            }

            if((HAL_GetTick() - tx_done_time) > 2000)
            {
                state = STATE_RX;
                return;
            }
        }
        break;

        /* ================= SAVE ================= */
        case STATE_SAVE:
        {
            if(save_edit_mode == 0)
            {
                if(key == 'E')
                {
                    if(slot_index > 0) slot_index--;
                    UI_DrawSlot(slot_index, slot_buf[slot_index]);
                    return;
                }
                else if(key == 'G')
                {
                    if(slot_index < SLOT_COUNT - 1) slot_index++;
                    UI_DrawSlot(slot_index, slot_buf[slot_index]);
                    return;
                }
                else if(key == '#')
                {
                    strncpy(tx_buf, slot_buf[slot_index], MAX_LEN);
                    tx_buf[MAX_LEN] = '\0';
                    cursor = strlen(tx_buf);

                    save_edit_mode = 1;
                    UI_DrawSaveEdit(slot_index, tx_buf, cursor);
                    return;
                }
                else if(key == '*')
                {
                    state = STATE_MENU;
                    return;
                }
            }
            else
            {
                if(key != KEYPAD_NOT_PRESSED)
                {
                    if(key == '*')
                    {
                        save_edit_mode = 0;
                        UI_DrawSlot(slot_index, slot_buf[slot_index]);
                        return;
                    }
                    else if(key == '#')
                    {
                    	strncpy(slot_buf[slot_index], tx_buf, MAX_LEN);
                    	slot_buf[slot_index][MAX_LEN] = '\0';

                    	FLASH_SaveSlot(slot_index, slot_buf[slot_index]);

                        save_edit_mode = 0;
                        UI_DrawSlot(slot_index, slot_buf[slot_index]);
                        return;
                    }
                    else if(key == '&')
                    {
                        TX_DeleteChar();
                        UI_DrawSaveEdit(slot_index, tx_buf, cursor);
                        return;
                    }

                    TX_AddChar(key);
                    UI_DrawSaveEdit(slot_index, tx_buf, cursor);
                    return;
                }
            }
        }
        break;

        /* ================= LOAD ================= */
        case STATE_LOAD:
        {
            if(key == 'E')
            {
                if(slot_index > 0) slot_index--;
                UI_DrawSlot(slot_index, slot_buf[slot_index]);
                return;
            }
            else if(key == 'G')
            {
                if(slot_index < SLOT_COUNT - 1) slot_index++;
                UI_DrawSlot(slot_index, slot_buf[slot_index]);
                return;
            }
            else if(key == '#')
            {
                strncpy(tx_buf, slot_buf[slot_index], MAX_LEN);
                tx_buf[MAX_LEN] = '\0';

                uint8_t packet[64];
                uint8_t len = BuildPacket(packet, tx_buf);

                sx1276_start_tx(&lora, packet, len);

                state = STATE_TX_DONE;
                tx_done_time = HAL_GetTick();
                return;
            }
            else if(key == '*')
            {
                state = STATE_MENU;
                return;
            }
        }
        break;
    }
}
/* =========================
 * 수신 패킷 처리
 * ========================= */
static void ProcessReceivedPacket(uint8_t *buf, uint8_t len)
{
    /* 1. 최소 길이 체크 */
    if(len < 5) return;

    /* 2. 헤더 확인 */
    if(buf[0] != PROTO_H1 || buf[1] != PROTO_H2)
        return;

    /* 3. ID / LENGTH 추출 */
    uint8_t rx_id  = buf[2];
    uint8_t rx_len = buf[3];

    /* 4. 데이터 길이 검증 */
    if(rx_len > 30) return;

    /* 5. 체크섬 검증 */
    uint8_t chk = 0;
    for(int i = 0; i < (4 + rx_len); i++)
    {
        chk ^= buf[i];
    }

    if(chk != buf[4 + rx_len])
        return; // 데이터 깨짐

    /* 6. 데이터 복사 */
    memcpy(rx_msg, &buf[4], rx_len);
    rx_msg[rx_len] = '\0';

    /* 7. 상태 업데이트 */
    last_rx_id = rx_id;
}
