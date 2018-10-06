#ifndef __BRUCON_NVS_H
#define __BRUCON_NVS_H

#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

//void bru_nvs_init();

char getBruCONConfigFlag(char* flag);
char setBruCONConfigFlag(char* flag,unsigned char flagval);
char setBruCONConfigString(char * key,char * value);
char getBruCONConfigString(char * key,char ** value);
char BruCONErase_key(char* key);
uint32_t getBruCONConfigUint32(char * flag);
uint32_t setBruCONConfigUint32(char* flag,uint32_t flagval);
#endif
