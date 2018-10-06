#include "brucon_nvs.h"


nvs_handle my_handle;
char nvs_inited = 0;

char bru_nvs_init()
{
    if(!nvs_inited){
        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            // NVS partition was truncated and needs to be erased
            // Retry nvs_flash_init
            ESP_ERROR_CHECK(nvs_flash_erase());
            err = nvs_flash_init();
        }
       if (err != ESP_OK) {
            return -1;
        }
        err = nvs_open("BruCON", NVS_READWRITE, &my_handle);
        if (err != ESP_OK) {
            printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
            return -1;
        } else {
            nvs_inited=1;

                return 0;
        }
    } else {
        return 0;
    }
}

char setBruCONConfigString(char * key,char * value)
{
    if(bru_nvs_init() == 0){
        if(nvs_set_str(my_handle,key,value) == 0)
            return 0;
        else
            return -1;
    } else {
        return -1;
    }
}


char getBruCONConfigFlag(char* flag)
{
    uint8_t flagval = 0;
    if(bru_nvs_init() == 0){
        nvs_get_u8(my_handle, flag, &flagval);
        return flagval;
    }
    return -1;
}

char setBruCONConfigFlag(char* flag,unsigned char flagval)
{
    if(bru_nvs_init() == 0){
        nvs_set_u8(my_handle, flag, flagval);
        return flagval;
    }
    return -1;
}
uint32_t getBruCONConfigUint32(char * flag)
{
    uint32_t flagval = 0;
    if(bru_nvs_init() == 0){
        nvs_get_u32(my_handle, flag, &flagval);
        return flagval;
    }
    return -1;
}

uint32_t setBruCONConfigUint32(char* flag,uint32_t flagval)
{
    if(bru_nvs_init() == 0){
        nvs_set_u32(my_handle, flag, flagval);
        return flagval;
    }
    return -1;
}
char getBruCONConfigString(char* flag, char ** flagval)
{
    if(bru_nvs_init() == 0){
        size_t required_size;
        nvs_get_str(my_handle, flag, NULL, &required_size);
        *flagval = malloc(required_size);
        nvs_get_str(my_handle, flag, *flagval, &required_size);
        return 0;
    }
    return -1;
}

char BruCONErase_key(char* key)
{
    if(bru_nvs_init() == 0){
        if(nvs_erase_key(my_handle,key)==ESP_OK){
            return 0;
        } else {

            return -1;
        }
    }
    nvs_commit(my_handle);
    return -1;
}
