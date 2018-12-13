#include <string.h>

#include "iec.h"
#include "iec_log.h"
#include "iec_adapter.h"
#include "iec_config.h"
#include "iec_sock.h"
#ifdef WITH_DTLS
#include "iec_mbed_ssl.h"
#endif


static void iec_buf_malloc(iec_buf_t *abuf, size_t size)
{
    abuf->size = size;
    abuf->len = 0;
    abuf->data = (unsigned char *)iec_malloc(size);

    IEC_ASSERT(abuf->data, IEC_ASSERT_MSG_MALLOC_ERR);
}

static void iec_buf_free(iec_buf_t *abuf)
{
    abuf->size = 0;
    abuf->len = 0;
    iec_free((void *)abuf->data);
}

void iec_register_proto(iec_connection_t *nc, iec_event_handler proto_handler)
{
    nc->proto_handler = proto_handler;
}

void iec_init(iec_manager_t *m, iec_device_info_t *param)
{
    IEC_ASSERT( m, IEC_ASSERT_MSG_PARAM_NULL);
    IEC_ASSERT( param, IEC_ASSERT_MSG_PARAM_NULL)

    m->interface = (iec_if_t *)iec_malloc(sizeof(iec_if_t));
    IEC_ASSERT(m->interface, IEC_ASSERT_MSG_MALLOC_ERR);

    m->interface->mgr = m;
    m->interface->ifuncs = param->ifuncs;
    m->interface->ifuncs->init(m->interface);
}

iec_connection_t* iec_connect(iec_manager_t *m, iec_event_handler cb, iec_connect_param_t param)
{
#ifdef WITH_DTLS
    int ret;
#endif
    iec_connection_t *nc = NULL;
    if((nc = (iec_connection_t *)iec_malloc(sizeof(iec_connection_t))) != NULL)
    {
        nc->mgr = m;
        m->nc = nc;
        nc->flags |= (param.proto_type == SOCK_STREAM) ? 0 : IEC_FG_UDP;
        nc->flags |= IEC_FG_CONNECTING;
        nc->sock_fd = -1;
        nc->user_handler = cb;
        nc->proto_handler = NULL;
        nc->last_time = iec_gettime_ms();
        iec_buf_malloc(&(nc->send_buf), IEC_SEND_BUF_SIZE);
        iec_buf_malloc(&(nc->recv_buf), IEC_RECV_BUF_SIZE);
        if(iec_parse_address(&(nc->address), param.server_ip, param.server_port) < 0)
        {
            iec_buf_free(&(nc->send_buf));
            iec_buf_free(&(nc->recv_buf));
            iec_free(nc);
            return NULL;
        }
    }
    else
        return nc;

#ifdef WITH_DTLS
    ret = iec_ssl_init(nc, &param.ssl_param);
    if(ret != 0)
    {
        iec_buf_free(&(nc->send_buf));
        iec_buf_free(&(nc->recv_buf));
        iec_free(nc);
        return NULL;
    }
#endif

    m->interface->ifuncs->connect(nc);
    return nc;
}

void iec_poll(iec_manager_t *m, int timeout_ms)
{
    m->interface->ifuncs->poll(m->interface, timeout_ms);
}
