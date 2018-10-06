#include "wifi.h"

#include "freertos/event_groups.h"

#include "esp_event_loop.h"

ip_addr_t ip_Addr;
ip4_addr_t ip;
ip4_addr_t gw;
ip4_addr_t msk;


volatile bool bConnected = false;
volatile bool bneedWifi = true;
volatile bool bDNSFound = false;


bool wifiIsConnected()
{
    return bConnected;

}

bool isdnsfound()
{
    return bDNSFound;

}


esp_err_t wifi_event_cb(void *ctx, system_event_t *event)
{
    switch( event->event_id){
    case SYSTEM_EVENT_STA_START:
        printf("sta start\n");
         esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        printf("got ip\n");
        ip = event->event_info.got_ip.ip_info.ip;
        gw = event->event_info.got_ip.ip_info.gw;
        msk = event->event_info.got_ip.ip_info.netmask;
        bConnected = true;
        //     xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        printf("connected!\n");

        break;
    case SYSTEM_EVENT_STA_STOP:
    case SYSTEM_EVENT_STA_DISCONNECTED:
    case SYSTEM_EVENT_STA_LOST_IP:
             /* This is a workaround as ESP32 WiFi libs don't currently
                auto-reassociate. */

        bConnected = false;
        //xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
    default:
        printf("unmanaged wifi event : %d\n",event->event_id);

        break;
    }

    return ESP_OK;
}

void BruconWifiStop()
{

    if(ESP_OK == esp_wifi_stop()){
      bConnected = 0;bneedWifi=false;
      
    }

}


void needWifi(bool need)
{
   if(bConnected)
     return;
    bneedWifi=need;
    if(need)
        last_click = xTaskGetTickCount();
    while(!bConnected)
        vTaskDelay(100 / portTICK_PERIOD_MS);
}

char wifi_inited =0;
void wificonnect()
{
    while(1){
        if(bConnected){	  
            vTaskDelay(500 / portTICK_PERIOD_MS);bneedWifi = false;
            continue;
        }
        if(bneedWifi){
            last_click = xTaskGetTickCount();
	    if(wifi_inited == 0){
            wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
            ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
            ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
            ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
            wifi_config_t sta_config;
            strcpy( (char*)sta_config.sta.ssid, get_wifi_netname() );
            strcpy( (char*)sta_config.sta.password, get_wifi_pass() );
            printf("connecting to %s \n",sta_config.sta.ssid);
            ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &sta_config) );
	    wifi_inited=1;
	    }

	    if( !bConnected ){
            ESP_ERROR_CHECK( esp_wifi_start() );
            ESP_ERROR_CHECK( esp_wifi_connect() );
	     while( !bConnected )
	      vTaskDelay(2);
                ;
		printf("Got IP: %s\n", inet_ntoa( ip ) );
		printf("Net mask: %s\n", inet_ntoa( msk ) );
		printf("Gateway: %s\n", inet_ntoa( gw ) );
		IP_ADDR4( &ip_Addr, 0,0,0,0 );
		bneedWifi = false;
	    }

           


          
        }

    }
}
