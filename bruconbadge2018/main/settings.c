#include "settings.h"
#include "settings.priv.h"


char * get_wifi_netname()
{
    return WIFI_SSID;

}

char * get_wifi_pass(    )
{
    return WIFI_PASSWORD;

}

char * get_client_cert();
