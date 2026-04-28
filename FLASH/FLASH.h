#ifndef FLASH_H
#define FLASH_H

#include <stdint.h>

#define SLOT_COUNT 5
#define SLOT_SIZE  32   // 31 + '\0'

void FLASH_SaveSlot(uint8_t slot, char msg[SLOT_SIZE]);
void FLASH_LoadSlot(uint8_t slot, char msg[SLOT_SIZE]);
void FLASH_LoadSlots(char slot_buf[SLOT_COUNT][SLOT_SIZE]);

#endif
