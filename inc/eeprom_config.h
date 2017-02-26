#ifndef _EEPROM_CONFIG_H_
#define _EEPROM_CONFIG_H_


#include "chip.h"
#include "state_types.h"
#include "console_types.h"
#include "state_types.h"
#include "lc1024.h"
#include "config.h"
#include "board.h"

#define EEPROM_DATA_START 0x000000
#define STORAGE_VERSION 0x02
#define CHECKSUM_BYTESIZE 1
#define VERSION_BYTESIZE 1

void EEPROM_Init(LPC_SSP_T *pSSP, uint32_t baud, uint8_t cs_gpio, uint8_t cs_pin);
uint8_t EEPROM_ChangeConfig(rw_loc_lable_t rw_loc, uint32_t val);
bool EEPROM_LoadPackConfig(PACK_CONFIG_T *pack_config);
void Write_EEPROM_PackConfig_Defaults(void);

#endif
