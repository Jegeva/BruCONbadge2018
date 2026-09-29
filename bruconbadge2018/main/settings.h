#ifndef __SETTINGS_H
#define __SETTINGS_H
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

#define WIFI_PASSWORD  ""
#define WIFI_SSID      ""

// --- Long-press power-off -------------------------------------------------
// Hold PWROFF_HOLD_BTN for PWROFF_HOLD_MS to enter deep sleep; a press of
// PWRON_WAKE_BTN wakes the chip, which on the ESP32 is a full reset, so it
// boots straight back into the menu.
//
// The wake button MUST be an RTC-capable GPIO. Button B (GPIO16) and button A
// (GPIO17) are NOT RTC GPIOs and cannot wake the chip, so the wake button
// defaults to LEFT (GPIO26). Deep sleep is ~microamp, not truly zero: this
// board has no power latch, so the 3.3 V rail (and the LCD) stay powered.
#define POWEROFF_ENABLED 1
#define PWROFF_HOLD_BTN  PIN_BB    // GPIO16, hold to power off
#define PWROFF_HOLD_MS   3000      // hold duration in ms (set 20000 for 20 s)
#define PWRON_WAKE_BTN   PIN_BL    // GPIO26 (RTC-capable), press to power on
// --------------------------------------------------------------------------


char * get_wifi_netname();
char * get_wifi_pass();
char * get_client_cert();

char * set_wifi_netname();
char * set_wifi_pass();
char * set_client_cert();


#endif
