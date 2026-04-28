#include "FLASH.h"
#include "stm32f1xx_hal.h"
#include <string.h>

#define FLASH_BASE_ADDR   0x0800EC00  // 마지막 5페이지 시작 주소
#define FLASH_MAGIC       0x4C4F5241  // 'LORA'

static uint32_t FLASH_GetSlotAddr(uint8_t slot)
{
    return FLASH_BASE_ADDR + ((uint32_t)slot * FLASH_PAGE_SIZE);
}

static void FLASH_WriteHalfWord(uint32_t addr, uint16_t data)
{
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr, data);
}

/* ================= SLOT SAVE ================= */
void FLASH_SaveSlot(uint8_t slot, char msg[SLOT_SIZE])
{
    if(slot >= SLOT_COUNT)
    {
        return;
    }

    uint32_t addr = FLASH_GetSlotAddr(slot);
    uint32_t pageError;
    FLASH_EraseInitTypeDef erase;

    HAL_FLASH_Unlock();

    erase.TypeErase   = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = addr;
    erase.NbPages     = 1;

    HAL_FLASHEx_Erase(&erase, &pageError);

    /* MAGIC 저장 */
    FLASH_WriteHalfWord(addr,     (uint16_t)(FLASH_MAGIC & 0xFFFF));
    FLASH_WriteHalfWord(addr + 2, (uint16_t)((FLASH_MAGIC >> 16) & 0xFFFF));

    addr += 4;

    /* 메시지 저장 */
    for(int i = 0; i < SLOT_SIZE; i += 2)
    {
        uint16_t data;

        data  = (uint8_t)msg[i];
        data |= ((uint16_t)(uint8_t)msg[i + 1] << 8);

        FLASH_WriteHalfWord(addr, data);
        addr += 2;
    }

    HAL_FLASH_Lock();
}

/* ================= SLOT LOAD ================= */
void FLASH_LoadSlot(uint8_t slot, char msg[SLOT_SIZE])
{
    if(slot >= SLOT_COUNT)
    {
        return;
    }

    uint32_t addr = FLASH_GetSlotAddr(slot);

    uint32_t magic = 0;
    magic  = *(volatile uint16_t*)addr;
    magic |= ((uint32_t)(*(volatile uint16_t*)(addr + 2)) << 16);

    if(magic != FLASH_MAGIC)
    {
        memset(msg, 0, SLOT_SIZE);
        return;
    }

    addr += 4;

    for(int i = 0; i < SLOT_SIZE; i += 2)
    {
        uint16_t data = *(volatile uint16_t*)addr;

        msg[i]     = data & 0xFF;
        msg[i + 1] = (data >> 8) & 0xFF;

        addr += 2;
    }

    msg[SLOT_SIZE - 1] = '\0';
}

/* ================= ALL LOAD ================= */
void FLASH_LoadSlots(char slot_buf[SLOT_COUNT][SLOT_SIZE])
{
    for(uint8_t i = 0; i < SLOT_COUNT; i++)
    {
        FLASH_LoadSlot(i, slot_buf[i]);
    }
}
