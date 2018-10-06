#ifndef __SETTINGS_H
#define __SETTINGS_H
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

#define WIFI_PASSWORD  ""
#define WIFI_SSID      ""


char * get_wifi_netname();
char * get_wifi_pass();
char * get_client_cert();

char * set_wifi_netname();
char * set_wifi_pass();
char * set_client_cert();


#endif
