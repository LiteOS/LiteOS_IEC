#ifndef __ATINY_MBED_SSL_H__
#define __ATINY_MBED_SSL_H__

#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/platform.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/timing.h"
#include "mbedtls/error.h"

#include "atiny.h"



typedef struct atiny_ssl_ctx
{
    mbedtls_ssl_context *ssl;
    mbedtls_ssl_config *conf;
    mbedtls_entropy_context *entropy;
    mbedtls_ctr_drbg_context *ctr_drbg;
    mbedtls_x509_crt *cacert;
    mbedtls_x509_crt *clicert;
    mbedtls_pk_context *pkey;
    mbedtls_timing_delay_context *timer;
} atiny_ssl_ctx_t;

int atiny_ssl_send( void *ctx, const unsigned char *buf, size_t len);
int atiny_ssl_recv( void *ctx, unsigned char *buf, size_t len);

int atiny_ssl_init(atiny_connection_t *nc, atiny_ssl_param_u *ssl_param);
int atiny_ssl_handshake(atiny_connection_t *nc);
void atiny_ssl_destroy(atiny_connection_t *nc);


#endif
