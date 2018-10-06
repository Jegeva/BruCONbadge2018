#ifndef __URL_H
#define __URL_H
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include "expat.h"
#include <string.h>
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/rsa.h"
#include "mbedtls/pk.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#include "mbedtls/x509_csr.h"


#include "esp_tls.h"
#include "wifi.h"

extern const uint8_t server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const uint8_t server_root_cert_pem_end[]   asm("_binary_server_root_cert_pem_end");

char * get_sched();

extern volatile char isgeneratingRSA;
extern volatile char isgettingClientCert;
extern volatile char ispostingAlc;
void task_genrsa(char * ret);
void task_getclientcert(char * ret);
void restore_clicert( 	mbedtls_x509_crt ** cert,mbedtls_pk_context ** pk_ctx_dcert  );
void send_alc_reading(int32_t * alc);
extern volatile char isgettingsched;
extern char * netsched;
extern mbedtls_x509_crt * clicert;
extern mbedtls_pk_context * pk_ctx_clicert;


#endif
