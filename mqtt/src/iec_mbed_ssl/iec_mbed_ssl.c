#include "iec_mbed_ssl.h"
#include "iec_log.h"
#include "iec_adapter.h"


static void *iec_calloc(size_t n, size_t size)
{
    void *p = iec_malloc(n * size);
    if(p)
    {
        memset(p, 0, n * size);
    }

    return p;
}

static void my_debug( void *ctx, int level,
                      const char *file, int line,
                      const char *str )
{
    const char *p, *basename;

    /* Extract basename from file */
    for( p = basename = file; *p != '\0'; p++ )
        if( *p == '/' || *p == '\\' )
            basename = p + 1;

    mbedtls_fprintf( (FILE *) ctx, "%s:%04d: |%d| %s", basename, line, level, str );
    fflush(  (FILE *) ctx  );
}


int iec_ssl_init(iec_connection_t *nc, iec_ssl_param_u *ssl_param)
{
    int ret;
    iec_ssl_ctx_t *ssl_ctx = NULL;
    const char *pers = "mqtt_client";

    if((ssl_ctx = iec_malloc(sizeof(iec_ssl_ctx_t))) != NULL)	
        nc->ssl_handler = ssl_ctx;

    (void)mbedtls_platform_set_calloc_free(iec_calloc, iec_free);
    (void)mbedtls_platform_set_snprintf(iec_snprintf);
    (void)mbedtls_platform_set_printf(iec_printf);

    ssl_ctx->ssl       = iec_malloc(sizeof(mbedtls_ssl_context));
    ssl_ctx->conf      = iec_malloc(sizeof(mbedtls_ssl_config));
    ssl_ctx->entropy   = iec_malloc(sizeof(mbedtls_entropy_context));
    ssl_ctx->ctr_drbg  = iec_malloc(sizeof(mbedtls_ctr_drbg_context));
    ssl_ctx->cacert    = iec_malloc(sizeof(mbedtls_x509_crt));
    ssl_ctx->clicert   = iec_malloc(sizeof(mbedtls_x509_crt));
    ssl_ctx->pkey      = iec_malloc(sizeof(mbedtls_pk_context));
    ssl_ctx->timer     = iec_malloc(sizeof(mbedtls_timing_delay_context));

#ifdef MBEDTLS_DEBUG_C
    extern void mbedtls_debug_set_threshold( int threshold );
    mbedtls_debug_set_threshold(100);
#endif
    mbedtls_ssl_init(ssl_ctx->ssl);
    mbedtls_ssl_config_init(ssl_ctx->conf);
    mbedtls_x509_crt_init(ssl_ctx->cacert);
    mbedtls_ctr_drbg_init(ssl_ctx->ctr_drbg);
    mbedtls_entropy_init(ssl_ctx->entropy);

    if( ( ret = mbedtls_ctr_drbg_seed(ssl_ctx->ctr_drbg, mbedtls_entropy_func, ssl_ctx->entropy,
                                      (const unsigned char *) pers,
                                      strlen( pers ) ) ) != 0 )
    {
        IEC_LOG(LOG_ERR, "failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret);
        goto exit;
    }

    IEC_LOG(LOG_DEBUG, "  . Loading the CA root certificate ...");

    ret = mbedtls_x509_crt_parse(ssl_ctx->cacert, ssl_param->ca.ca_cert, ssl_param->ca.ca_cert_len);
    if( ret < 0 )
    {
        IEC_LOG(LOG_DEBUG,  \
                    " failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n",
                    -ret );
        goto exit;
    }

    IEC_LOG(LOG_DEBUG, "	. Loading the client cert. and key ...");
    ret = mbedtls_x509_crt_parse(ssl_ctx->clicert, ssl_param->ca.client_cert, ssl_param->ca.client_cert_len);
    if( ret < 0 )
    {
        IEC_LOG(LOG_DEBUG,  \
                    " failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n",
                    -ret );
        goto exit;
    }

    ret = mbedtls_pk_parse_key(ssl_ctx->pkey, ssl_param->ca.client_key, ssl_param->ca.client_key_len, NULL, 0);
    if( ret < 0 )
    {
        IEC_LOG(LOG_DEBUG,  \
                  " failed\n	!  mbedtls_pk_parse_key returned -0x%x\n\n",
                  -ret );
        goto exit;
    }
    /*
     * 2. Setup stuff
     */
    IEC_LOG(LOG_DEBUG, "  . Setting up the DTLS structure..." );
    if( ( ret = mbedtls_ssl_config_defaults(ssl_ctx->conf,
                                            MBEDTLS_SSL_IS_CLIENT,
                                            MBEDTLS_SSL_TRANSPORT_STREAM,
                                            MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 )
    {
        IEC_LOG(LOG_ERR, " failed\n  ! mbedtls_ssl_config_defaults returned -0x%x", -ret );
        goto exit;
    }

    mbedtls_ssl_conf_authmode(ssl_ctx->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(ssl_ctx->conf, ssl_ctx->cacert, NULL);
    mbedtls_ssl_conf_rng(ssl_ctx->conf, mbedtls_ctr_drbg_random, ssl_ctx->ctr_drbg);
    mbedtls_ssl_conf_dbg(ssl_ctx->conf, my_debug, stdout);

    if( ( ret = mbedtls_ssl_conf_own_cert( ssl_ctx->conf, ssl_ctx->clicert, ssl_ctx->pkey ) ) != 0 )
    {
        IEC_LOG(LOG_ERR, " failed\n	! mbedtls_ssl_conf_own_cert returned %d\n\n", ret );
        goto exit;
    }

    if( ( ret = mbedtls_ssl_setup( ssl_ctx->ssl, ssl_ctx->conf ) ) != 0 )
    {
        IEC_LOG(LOG_ERR, " failed\n  ! mbedtls_ssl_setup returned -0x%x", -ret );
        goto exit;
    }

    if( ( ret = mbedtls_ssl_set_hostname( ssl_ctx->ssl, ssl_param->ca.server_name ) ) != 0 )
    {
        IEC_LOG(LOG_ERR,
                        " failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n",
                        ret );

        goto exit;
    }

    return 0;

exit:
    if (ssl_ctx->conf)
    {
        mbedtls_ssl_config_free(ssl_ctx->conf);
        mbedtls_free(ssl_ctx->conf);
    }

    if (ssl_ctx->ctr_drbg)
    {
        mbedtls_ctr_drbg_free(ssl_ctx->ctr_drbg);
        mbedtls_free(ssl_ctx->ctr_drbg);
    }

    if (ssl_ctx->entropy)
    {
        mbedtls_entropy_free(ssl_ctx->entropy);
        mbedtls_free(ssl_ctx->entropy);
    }

    if (ssl_ctx->ssl)
    {
        mbedtls_ssl_free(ssl_ctx->ssl);
        mbedtls_free(ssl_ctx->ssl);
    }

    if(ssl_ctx->cacert)
    {
        mbedtls_free(ssl_ctx->cacert);
    }

    if(ssl_ctx->clicert)
    {
        mbedtls_free(ssl_ctx->clicert);
    }

    if(ssl_ctx->pkey)
    {
        mbedtls_pk_free(ssl_ctx->pkey);
        mbedtls_free(ssl_ctx->pkey);
    }

    if(ssl_ctx->timer)
    {
        mbedtls_free(ssl_ctx->timer);
    }

    return -1;
}

int iec_ssl_send( void *ctx, const unsigned char *buf, size_t len)
{
    int ret;
    iec_connection_t *nc = (iec_connection_t *)ctx;
    ret = nc->mgr->interface->ifuncs->send(nc, buf, len);
    if(ret > 0)
        return ret;
    if(ret == 0)
        return MBEDTLS_ERR_SSL_WANT_WRITE;
    return MBEDTLS_ERR_NET_SEND_FAILED;
}

int iec_ssl_recv( void *ctx, unsigned char *buf, size_t len)
{
    int ret;
    iec_connection_t *nc = (iec_connection_t *)ctx;
    ret = nc->mgr->interface->ifuncs->recv(nc, buf, len);
    if(ret > 0)
        return ret;
    if(ret == 0)
        return MBEDTLS_ERR_SSL_WANT_READ;
    return MBEDTLS_ERR_NET_RECV_FAILED;
}


int iec_ssl_handshake(iec_connection_t *nc)
{
    int ret;
    unsigned char buf[MBEDTLS_SSL_MAX_CONTENT_LEN + 1];
    iec_ssl_ctx_t *ssl_ctx = (iec_ssl_ctx_t *)nc->ssl_handler;

    mbedtls_ssl_set_bio( ssl_ctx->ssl, nc, iec_ssl_send, iec_ssl_recv, NULL);

    IEC_LOG(LOG_DEBUG,"	. Performing the SSL/TLS handshake...");
    while( ( ret = mbedtls_ssl_handshake( ssl_ctx->ssl ) ) != 0 )
    {
        if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE
            && ret != MBEDTLS_ERR_SSL_TIMEOUT)
        {
            IEC_LOG(LOG_ERR, " failed\n	! mbedtls_ssl_handshake returned -0x%x", -ret );
            goto exit;
        }
    }

    IEC_LOG(LOG_DEBUG, " . Verifying peer X.509 certificate...");
    if( ( ret = mbedtls_ssl_get_verify_result( ssl_ctx->ssl ) ) != 0 )
    {
        char vrfy_buf[512];

        IEC_LOG(LOG_ERR, " failed\n" );

        mbedtls_x509_crt_verify_info(vrfy_buf, \
                                     sizeof( vrfy_buf ), \
                                     "    ! ", \
                                     ret );

        IEC_LOG(LOG_ERR, "%s\n", vrfy_buf );
    }
    else
        IEC_LOG(LOG_INFO, "ok\n");

    if( mbedtls_ssl_get_peer_cert( ssl_ctx->ssl ) != NULL )
    {
        mbedtls_x509_crt_info( (char *) buf, sizeof( buf ) - 1, "      ",
                       mbedtls_ssl_get_peer_cert( ssl_ctx->ssl ) );
    }

    IEC_LOG(LOG_DEBUG, "  . Performing renegotiation..." );
    while( ( ret = mbedtls_ssl_renegotiate( ssl_ctx->ssl ) ) != 0 )
    {
        if( ret != MBEDTLS_ERR_SSL_WANT_READ &&
            ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
            IEC_LOG(LOG_DEBUG, " failed\n	! mbedtls_ssl_renegotiate returned %d\n\n", ret );
            goto exit;
        }
    }
    IEC_LOG(LOG_DEBUG, " ok\n" );

    return 0;
exit:
    return ret;
}

void iec_ssl_destroy(iec_connection_t *nc)
{
    iec_ssl_ctx_t *ssl_ctx = (iec_ssl_ctx_t *)nc->ssl_handler;

    mbedtls_ssl_context          *ssl = NULL;

    mbedtls_ssl_config           *conf = NULL;
    mbedtls_ctr_drbg_context     *ctr_drbg = NULL;
    mbedtls_entropy_context      *entropy = NULL;
    mbedtls_timing_delay_context *timer = NULL;

    ssl        = ssl_ctx->ssl;
    conf       = ssl_ctx->conf;
    timer      = (mbedtls_timing_delay_context *)ssl_ctx->timer;

    if (conf)
    {
        ctr_drbg   = conf->p_rng;

        if (ctr_drbg)
        {
            entropy =  ctr_drbg->p_entropy;
        }
    }


    if (conf)
    {
        mbedtls_ssl_config_free(conf);
        mbedtls_free(conf);
        ssl->conf = NULL; //  need by mbedtls_debug_print_msg(), see mbedtls_ssl_free(ssl)
    }

    if (ctr_drbg)
    {
        mbedtls_ctr_drbg_free(ctr_drbg);
        mbedtls_free(ctr_drbg);
    }

    if (entropy)
    {
        mbedtls_entropy_free(entropy);
        mbedtls_free(entropy);
    }

    if (timer)
    {
        mbedtls_free(timer);
    }

    mbedtls_ssl_free(ssl);
    mbedtls_free(ssl);
}

