#ifndef APP_H
#define APP_H

#include "main.h"

#define LINE1_MAX 13
#define LINE2_MAX 18
#define MAX_LEN   (LINE1_MAX + LINE2_MAX)
// 상태 정의
typedef enum {
    STATE_RX,
    STATE_MENU,
    STATE_TX,
    STATE_TX_DONE,
    STATE_SAVE,
    STATE_LOAD
} app_state_t;

// 함수
void APP_Init(void);
void APP_Run(void);

#endif
