#include "cc.h"
typedef uint32_t Fapi_StatusType;
#define Fapi_Status_Success 0U
#define C28_FLASH_WORD_OCTETS 2U
#define FLASH_APP_ADDR 0x0A0000UL
#define FLASH_APP_SIZE_WORDS 0x20000UL
#define FLASH_APP_SIZE_OCTETS (FLASH_APP_SIZE_WORDS * 2UL)
#define FLASH_APP_BSIZE FLASH_APP_SIZE_WORDS
#define DATA_FLASH_PROGRAM_SIZE_WORDS 8U
Fapi_StatusType Program_dataFlashSector(const uint16_t *, uint32_t, uint32_t);
Fapi_StatusType Erase_dataFlashSector(uint32_t, uint32_t);
