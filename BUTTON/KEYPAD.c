#include "KEYPAD.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>

// -----------------------------------------------------------
// LED 핀 설정
// -----------------------------------------------------------
#define KEYPAD_LED_PORT             GPIOC
#define KEYPAD_LED_PIN              GPIO_PIN_13
#define KEYPAD_LED_ON()             HAL_GPIO_WritePin(KEYPAD_LED_PORT, KEYPAD_LED_PIN, GPIO_PIN_RESET)
#define KEYPAD_LED_OFF()            HAL_GPIO_WritePin(KEYPAD_LED_PORT, KEYPAD_LED_PIN, GPIO_PIN_SET)
#define KEYPAD_LED_TOGGLE()         HAL_GPIO_TogglePin(KEYPAD_LED_PORT, KEYPAD_LED_PIN)

// -----------------------------------------------------------
// 행 (ROW) 핀 설정 - 출력
// -----------------------------------------------------------
#define SET_ROW_1_HIGH              HAL_GPIO_WritePin(KEYPAD_ROW_1_PORT, KEYPAD_ROW_1_PIN, GPIO_PIN_SET)
#define SET_ROW_2_HIGH              HAL_GPIO_WritePin(KEYPAD_ROW_2_PORT, KEYPAD_ROW_2_PIN, GPIO_PIN_SET)
#define SET_ROW_3_HIGH              HAL_GPIO_WritePin(KEYPAD_ROW_3_PORT, KEYPAD_ROW_3_PIN, GPIO_PIN_SET)
#define SET_ROW_4_HIGH              HAL_GPIO_WritePin(KEYPAD_ROW_4_PORT, KEYPAD_ROW_4_PIN, GPIO_PIN_SET)
#define SET_ROW_1_LOW               HAL_GPIO_WritePin(KEYPAD_ROW_1_PORT, KEYPAD_ROW_1_PIN, GPIO_PIN_RESET)
#define SET_ROW_2_LOW               HAL_GPIO_WritePin(KEYPAD_ROW_2_PORT, KEYPAD_ROW_2_PIN, GPIO_PIN_RESET)
#define SET_ROW_3_LOW               HAL_GPIO_WritePin(KEYPAD_ROW_3_PORT, KEYPAD_ROW_3_PIN, GPIO_PIN_RESET)
#define SET_ROW_4_LOW               HAL_GPIO_WritePin(KEYPAD_ROW_4_PORT, KEYPAD_ROW_4_PIN, GPIO_PIN_RESET)

// -----------------------------------------------------------
// 열 (COLUMN) 핀 설정 - 입력
// -----------------------------------------------------------
#define READ_COLUMN_1               HAL_GPIO_ReadPin(KEYPAD_COLUMN_1_PORT, KEYPAD_COLUMN_1_PIN)
#define READ_COLUMN_2               HAL_GPIO_ReadPin(KEYPAD_COLUMN_2_PORT, KEYPAD_COLUMN_2_PIN)
#define READ_COLUMN_3               HAL_GPIO_ReadPin(KEYPAD_COLUMN_3_PORT, KEYPAD_COLUMN_3_PIN)
#define READ_COLUMN_4               HAL_GPIO_ReadPin(KEYPAD_COLUMN_4_PORT, KEYPAD_COLUMN_4_PIN)

// -----------------------------------------------------------
// 전역 변수들 - 키패드 상태 추적
// -----------------------------------------------------------
static uint32_t g_keyPressStartTime = 0;
static char g_lastPressedKey = KEYPAD_NOT_PRESSED;
static uint8_t g_isKeyReleased = 0;

// -----------------------------------------------------------
// 매크로 및 상수
// -----------------------------------------------------------
#define DEBOUNCE_DELAY_MS           50          // 디바운싱 지연 시간
#define LONG_PRESS_THRESHOLD_1_MS   1000        // 2초 롱프레스
#define LONG_PRESS_THRESHOLD_2_MS   2500        // 4초 롱프레스
//------------------------------------------------------------

//------------------------------------------------------------
/* 키패드 버튼 값 배열 */
const char Keypad_Button_Values[4][4] = {
		{'A', 'B', 'C', 'D'},
		{'E', 'F', 'G', 'H'},
		{'I', 'J', 'K', 'L'},
		{'*', '/', '&', '#'}
};

/* 롱프레스 1 (2초) 값 배열 */
const char Keypad_LongPress_Values1[4][4] = {
		{'M', 'N', 'O', 'P'},
		{'Q', 'R', 'S', 'T'},
		{'U', 'V', 'W', 'X'},
		{'Y', 'Z', '+', '-'}
};
/* 롱프레스 2 (4초) 값 배열 */
const char Keypad_LongPress_Values2[4][4] = {
		{'1', '2', '3', '.'},
		{'4', '5', '6', '$'},
		{'7', '8', '9', ')'},
		{'?', '0', '%', '('}
};

// -----------------------------------------------------------
// 내부 함수 선언
// -----------------------------------------------------------
static void Set_Keypad_Row(uint8_t Row);
static char Scan_Keypad(void);

/**
 * @brief  키패드의 특정 행을 LOW로 설정하고 다른 행은 HIGH로 설정합니다.
 * @param  Row: 설정할 행 번호 (1-4). 0이면 모든 행을 HIGH로 설정.
 * @retval None
 */
static void Set_Keypad_Row(uint8_t Row)
{
    /* 모든 행을 HIGH로 설정 */
    SET_ROW_1_HIGH;
    SET_ROW_2_HIGH;
    SET_ROW_3_HIGH;
    SET_ROW_4_HIGH;

    /* 특정 행을 LOW로 설정 */
    if(Row == 1) SET_ROW_1_LOW;
    if(Row == 2) SET_ROW_2_LOW;
    if(Row == 3) SET_ROW_3_LOW;
    if(Row == 4) SET_ROW_4_LOW;
}


static char Scan_Keypad(void)
{
    char keyFound = KEYPAD_NOT_PRESSED;
    for (int row = 1; row <= 4; row++)
    {
        Set_Keypad_Row(row);
        HAL_Delay(1); // 핀 상태가 안정되기를 기다리는 짧은 지연

        if (!READ_COLUMN_1) keyFound = Keypad_Button_Values[0][row-1];
        else if (!READ_COLUMN_2) keyFound = Keypad_Button_Values[1][row-1];
        else if (!READ_COLUMN_3) keyFound = Keypad_Button_Values[2][row-1];
        else if (!READ_COLUMN_4) keyFound = Keypad_Button_Values[3][row-1];

        if (keyFound != KEYPAD_NOT_PRESSED) return keyFound;
    }
    Set_Keypad_Row(0); // 스캔 종료 후 모든 행을 HIGH로 설정
    return KEYPAD_NOT_PRESSED;
}

void KEYPAD_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 행 핀 (출력, 푸시풀) 설정 */
    GPIO_InitStruct.Pin = KEYPAD_ROW_1_PIN | KEYPAD_ROW_2_PIN | KEYPAD_ROW_3_PIN | KEYPAD_ROW_4_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(KEYPAD_ROW_1_PORT, &GPIO_InitStruct);
    Set_Keypad_Row(0); // 초기 상태는 모든 행을 HIGH로 설정

    /* 열 핀 (입력, 풀업) 설정 */
    GPIO_InitStruct.Pin = KEYPAD_COLUMN_1_PIN | KEYPAD_COLUMN_2_PIN | KEYPAD_COLUMN_3_PIN | KEYPAD_COLUMN_4_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(KEYPAD_COLUMN_1_PORT, &GPIO_InitStruct);

    /* LED 핀 (출력, 푸시풀) 설정 */
    GPIO_InitStruct.Pin = KEYPAD_LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(KEYPAD_LED_PORT, &GPIO_InitStruct);
    KEYPAD_LED_OFF();
}

char KEYPAD_Get_Key(void)
{
    char currentKey = Scan_Keypad();
    uint32_t pressDuration;
    static uint32_t lastDebounceTime = 0;
    static uint32_t lastToggleTime = 0;
    static uint8_t blinkCount = 0;

    // 현재 눌린 키가 있다면
    if (currentKey != KEYPAD_NOT_PRESSED) {
        // 디바운싱: 이전에 눌렸던 키와 같고, 충분한 시간이 지나지 않았으면 무시
        if (currentKey == g_lastPressedKey && (HAL_GetTick() - lastDebounceTime < DEBOUNCE_DELAY_MS)) {
            return KEYPAD_NOT_PRESSED;
        }

        // 새로운 키가 눌렸을 때
        if (currentKey != g_lastPressedKey) {
            g_keyPressStartTime = HAL_GetTick(); // 키 누름 시작 시간 기록
            g_lastPressedKey = currentKey;
            lastDebounceTime = HAL_GetTick();
            g_isKeyReleased = 0;
            KEYPAD_LED_ON(); // LED 켜기
            blinkCount = 0; // 깜빡임 카운터 초기화
        }

        pressDuration = HAL_GetTick() - g_keyPressStartTime;

        // 롱프레스 레벨 2 (4초)
        if (pressDuration >= LONG_PRESS_THRESHOLD_2_MS) {
            if (blinkCount < 2) {
                if (HAL_GetTick() - lastToggleTime >= 100) { // 100ms마다
                    KEYPAD_LED_TOGGLE();
                    HAL_Delay(100);
                    KEYPAD_LED_TOGGLE();
                    lastToggleTime = HAL_GetTick();
                    blinkCount++;
                }
            }
        }
        // 롱프레스 레벨 1 (2초)
        else if (pressDuration >= LONG_PRESS_THRESHOLD_1_MS) {
            if (blinkCount == 0) { // 한 번만 깜빡이도록
                KEYPAD_LED_TOGGLE();
                HAL_Delay(100);
                KEYPAD_LED_TOGGLE();
                lastToggleTime = HAL_GetTick();
                blinkCount = 1; // 이미 깜빡였음을 표시
            }
        }
        return KEYPAD_NOT_PRESSED;
    }
    // 키가 눌려있지 않은 상태
    else {
        // 이전에 눌렸던 키가 있었다면 (키 떼짐 감지)
        if (g_lastPressedKey != KEYPAD_NOT_PRESSED && !g_isKeyReleased) {
            pressDuration = HAL_GetTick() - g_keyPressStartTime;

            for(volatile int i = 0; i < 72000; i++);


            char keyToReturn = KEYPAD_NOT_PRESSED;

            // 롱프레스 레벨 2 (4초 이상)
            if (pressDuration >= LONG_PRESS_THRESHOLD_2_MS) {
                // 배열에서 해당하는 값 찾기
                for(int i = 0; i < 4; i++) {
                    for(int j = 0; j < 4; j++) {
                        if (Keypad_Button_Values[j][i] == g_lastPressedKey) {
                            keyToReturn = Keypad_LongPress_Values2[j][i];
                            break;
                        }
                    }
                }
            }
            // 롱프레스 레벨 1 (2초 이상)
            else if (pressDuration >= LONG_PRESS_THRESHOLD_1_MS) {
                // 배열에서 해당하는 값 찾기
                for(int i = 0; i < 4; i++) {
                    for(int j = 0; j < 4; j++) {
                        if (Keypad_Button_Values[j][i] == g_lastPressedKey) {
                            keyToReturn = Keypad_LongPress_Values1[j][i];
                            break;
                        }
                    }
                }
            }
            // 일반 누름
            else {
                keyToReturn = g_lastPressedKey;
            }

            g_lastPressedKey = KEYPAD_NOT_PRESSED; // 상태 초기화
            g_isKeyReleased = 1;
            KEYPAD_LED_OFF(); // LED 끄기
            return keyToReturn;
        }
    }

    return KEYPAD_NOT_PRESSED; // 눌린 키가 없을 때
}
