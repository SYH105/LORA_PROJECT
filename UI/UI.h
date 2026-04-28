#ifndef UI_H
#define UI_H

#include <stdint.h>

/* RX */
void UI_DrawRX(const char* msg, uint8_t id);

/* MENU */
void UI_DrawMenu(uint8_t menu_index);

/* TX */
void UI_DrawTX(const char* buf, uint8_t cursor);

/* SAVE */
void UI_DrawSlot(uint8_t idx, const char* buf);
void UI_DrawSaveEdit(uint8_t idx, const char* buf, uint8_t cursor);

/* TX DONE */
void UI_DrawTxDone(void);

#endif
