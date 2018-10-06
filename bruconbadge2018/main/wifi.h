#ifndef __WIFI_H
#define __WIFI_H
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_event_loop.h>
#include <lwip/inet.h>
#include <lwip/ip4_addr.h>
#include <lwip/dns.h>

#include "string.h"
#include "settings.h"
#include "settings.priv.h"



extern uint8_t mac[6] ;
extern volatile char * macstr;
extern volatile char * hostname;
extern TickType_t last_click;


bool wifiIsConnected();
bool isdnsfound();
void wificonnect();
esp_err_t wifi_event_cb(void *ctx, system_event_t *event);
void needWifi(bool need);
void BruconWifiStop();

#endif
