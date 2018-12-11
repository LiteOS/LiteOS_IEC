#include <string.h>

#include "atiny.h"
#include "atiny_debug.h"
#include "atiny_adapter.h"
#include "atiny_config.h"
#include "atiny_sock.h"
#ifdef WITH_DTLS
#include "atiny_mbed_ssl.h"
#endif


static void atiny_buf_malloc(atiny_buf_t *abuf, size_t size)
{
    abuf->size = size;
    abuf->len = 0;
    abuf->data = (unsigned char *)atiny_malloc(size);

    ATINY_ASSERT(abuf->data, ATINY_ASSERT_MSG_MALLOC_ERR);
}

static void atiny_buf_free(atiny_buf_t *abuf)
{
    abuf->size = 0;
    abuf->len = 0;
    atiny_free((void *)abuf->data);
}

void atiny_register_proto(atiny_connection_t *nc, atiny_event_handler proto_handler)
{
    nc->proto_handler = proto_handler;
}

void atiny_init(atiny_manager_t *m, atiny_device_info_t *param)
{
    ATINY_ASSERT( m, ATINY_ASSERT_MSG_PARAM_NULL);
    ATINY_ASSERT( param, ATINY_ASSERT_MSG_PARAM_NULL)

    m->interface = (atiny_if_t *)atiny_malloc(sizeof(atiny_if_t));
    ATINY_ASSERT(m->interface, ATINY_ASSERT_MSG_MALLOC_ERR);

    m->interface->mgr = m;
    m->interface->ifuncs = param->ifuncs;
    m->interface->ifuncs->init(m->interface);
}

atiny_connection_t* atiny_connect(atiny_manager_t *m, atiny_event_handler cb, atiny_connect_param_t param)
{
#ifdef WITH_DTLS
    int ret;
#endif
    atiny_connection_t *nc = NULL;
    if((nc = (atiny_connection_t *)atiny_malloc(sizeof(atiny_connection_t))) != NULL)
    {
        nc->mgr = m;
        m->nc = nc;
        nc->flags |= (param.proto_type == SOCK_STREAM) ? 0 : ATINY_FG_UDP;
        nc->flags |= ATINY_FG_CONNECTING;
        nc->sock_fd = -1;
        nc->user_handler = cb;
        nc->proto_handler = NULL;
        nc->last_time = atiny_gettime_ms();
        atiny_buf_malloc(&(nc->send_buf), ATINY_SEND_BUF_SIZE);
        atiny_buf_malloc(&(nc->recv_buf), ATINY_RECV_BUF_SIZE);
        if(atiny_parse_address(&(nc->address), param.server_ip, param.server_port) < 0)
        {
            atiny_buf_free(&(nc->send_buf));
            atiny_buf_free(&(nc->recv_buf));
            atiny_free(nc);
            return NULL;
        }
    }
    else
        return nc;

#ifdef WITH_DTLS
    ret = atiny_ssl_init(nc, &param.ssl_param);
    if(ret != 0)
    {
        atiny_buf_free(&(nc->send_buf));
        atiny_buf_free(&(nc->recv_buf));
        atiny_free(nc);
        return NULL;
    }
#endif

    m->interface->ifuncs->connect(nc);
    return nc;
}

void atiny_poll(atiny_manager_t *m, int timeout_ms)
{
    m->interface->ifuncs->poll(m->interface, timeout_ms);
}
