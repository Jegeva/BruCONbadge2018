#ifndef __SETTINGS_H
#define __SETTINGS_H
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

#define WIFI_PASSWORD  ""
#define WIFI_SSID      ""

// --- Offline build switch -------------------------------------------------
// 1 = never bring up the Wi-Fi radio; boot straight to the menu using the
// schedule embedded in flash. On a weak/aged battery the Wi-Fi TX current
// burst is what drops the rail past the brownout detector and reboots the
// badge in a loop, so this both stops the loop and skips the ~40 s connect
// wait. 0 = stock behaviour (connect to `brucon`, enroll, fetch schedule).
#define OFFLINE_MODE 1
// --------------------------------------------------------------------------


char * get_wifi_netname();
char * get_wifi_pass();
char * get_client_cert();

char * set_wifi_netname();
char * set_wifi_pass();
char * set_client_cert();


#endif
