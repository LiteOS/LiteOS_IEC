#include <string.h>

#include "iec.h"
#include "iec_log.h"
#ifdef WITH_DTLS
#include "iec_mbed_ssl.h"
#endif

int iec_parse_address(struct sockaddr_in *addr, char *server_ip, unsigned int server_port)
{
    int rc = -1;
    struct addrinfo *result = NULL;
    struct addrinfo hints = {0, AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0, NULL, NULL, NULL};

    if ((rc = getaddrinfo(server_ip, NULL, &hints, &result)) == 0)
    {
        struct addrinfo *res = result;

        while (res)
        {
            if (res->ai_family == AF_INET)
            {
                result = res;
                break;
            }
            res = res->ai_next;
        }

        if (result->ai_family == AF_INET)
        {
            addr->sin_port = htons((unsigned short)server_port);
            addr->sin_family = AF_INET;
            addr->sin_addr = ((struct sockaddr_in *)(result->ai_addr))->sin_addr;
        }
        else
            rc = -1;

        freeaddrinfo(result);
    }

    return rc;
}


void iec_dispatch_event(iec_connection_t *nc, iec_event_handler event_handler, void *user_data, int event, void *event_data)
{
    if(event_handler == NULL)
    {
        event_handler = nc->proto_handler ? nc->proto_handler : nc->user_handler;
    }

    if(event_handler)
    {
        event_handler(nc, event, event_data);
    }
}

void iec_nc_connect_cb(iec_connection_t *nc)
{
#if WITH_DTLS
    int ret;
#endif

    IEC_LOG(LOG_DEBUG, "iec_nc_connect_cb");
    nc->flags &= ~IEC_FG_CONNECTING;
#if WITH_DTLS
    ret = iec_ssl_handshake(nc);
    if(ret == 0)
    {
        nc->flags &= ~(IEC_FG_CAN_RD | IEC_FG_CAN_WR);
    }
    else if(ret == MBEDTLS_ERR_SSL_WANT_READ)
        nc->flags |= IEC_FG_CAN_RD;
    else if(ret == MBEDTLS_ERR_SSL_WANT_WRITE)
        nc->flags |= IEC_FG_CAN_WR;
#endif

    iec_dispatch_event(nc, NULL, NULL, IEC_EV_CONNECTED,NULL);
}

void iec_nc_can_write_cb(iec_connection_t *nc)
{
    int rc = 0;
    const unsigned char *buf = nc->send_buf.data;
    size_t len = nc->send_buf.len;
    IEC_LOG(LOG_DEBUG, "iec_nc_can_write_cb len:%d", (int)len);

    nc->flags &= ~IEC_FG_CAN_WR;
#if WITH_DTLS
    if(len > 0)
        rc = mbedtls_ssl_write(((iec_ssl_ctx_t *)nc->ssl_handler)->ssl,  buf, len);

    if (rc == MBEDTLS_ERR_SSL_WANT_WRITE)
    {
        nc->flags |= IEC_FG_CAN_WR;
    }
    else if (rc < 0)
    {
        nc->flags |= IEC_FG_RECONNECT;
    }
    else
    {
        nc->flags &= ~IEC_FG_CAN_WR;
    }
#else
    if(len > 0)
        rc = nc->mgr->interface->ifuncs->send(nc, buf, len);
#endif

    if(rc < 0)
    {
        IEC_LOG(LOG_ERR, "iec write error");
        nc->flags |= IEC_FG_RECONNECT;
    }
    else if(rc > 0)
    {
        nc->last_time = iec_gettime_ms();
        if(nc->send_buf.len > rc)
            memmove(nc->send_buf.data, nc->send_buf.data + rc, nc->send_buf.len - rc);
        nc->send_buf.len -= rc;
    }
    iec_dispatch_event(nc, NULL, NULL, IEC_EV_SEND, NULL);
}

void iec_nc_can_read_cb(iec_connection_t *nc)
{
    int rc = 0;
    unsigned char *buf = nc->recv_buf.data + nc->recv_buf.len;
    size_t len = nc->recv_buf.size - nc->recv_buf.len;
    nc->flags &= ~IEC_FG_CAN_RD;

    IEC_LOG(LOG_DEBUG, "iec_nc_can_read_cb len:%d", (int)len);

#if WITH_DTLS
    if(len > 0)  rc = mbedtls_ssl_read(((iec_ssl_ctx_t *)nc->ssl_handler)->ssl, (unsigned char *) buf, len);

    if (rc == MBEDTLS_ERR_SSL_WANT_READ)
    {
        nc->flags |= IEC_FG_CAN_RD;
    }
    else if (rc < 0)
    {
        nc->flags |= IEC_FG_RECONNECT;
    }
    else
    {
        nc->flags &= ~IEC_FG_CAN_RD;
    }
#else
    if(len > 0)
        rc = nc->mgr->interface->ifuncs->recv(nc, buf, len);
#endif

    if(rc< 0)
    {
        IEC_LOG(LOG_ERR, "read error");
		nc->flags |= IEC_FG_RECONNECT;
    }
    else if(rc > 0)
    {
        nc->last_time = iec_gettime_ms();
        nc->recv_buf.len += rc;
        iec_dispatch_event(nc, NULL, NULL, IEC_EV_RECV, NULL);
    }
}

static void iec_nc_poll_cb(iec_connection_t *nc)
{
    unsigned long int now = iec_gettime_ms();
    iec_dispatch_event(nc, NULL, NULL, IEC_EV_POLL, &now);
}

static void iec_mgr_handle_conn(iec_connection_t *nc)
{
    iec_nc_poll_cb(nc);

    if(nc->flags & IEC_FG_CONNECTING)
    {
        iec_nc_connect_cb(nc);
    }

    if(nc->flags & IEC_FG_CAN_RD)
    {
        iec_nc_can_read_cb(nc);
    }

    if(nc->flags & IEC_FG_CAN_WR)
    {
        iec_nc_can_write_cb(nc);
    }
}


void iec_sock_init(iec_if_t *interface)
{
    (void)interface;
    IEC_LOG(LOG_DEBUG, " using select()");
}

void iec_sock_uninit(iec_if_t *interface)
{

}

void iec_sock_connect(iec_connection_t *nc)
{
    int rc = 0;

    nc->sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (nc->sock_fd == -1) {
        IEC_LOG(LOG_ERR, "socket error");
        return;
    }

    rc = connect(nc->sock_fd, (struct sockaddr *)&nc->address, sizeof(nc->address));
    if(rc < 0)
        IEC_LOG(LOG_ERR, "sock %d rc %d ",  nc->sock_fd, rc);
}

void iec_sock_discon(iec_connection_t *nc)
{
    int rc = 0;

    if(nc->sock_fd == -1)
        return;

    rc = close(nc->sock_fd);
    if(rc < 0)
        IEC_LOG(LOG_ERR, "sock %d rc %d ",  nc->sock_fd, rc);
}


iec_time_t iec_sock_poll(iec_if_t *interface, int timeout_ms)
{
    iec_time_t now;
    fd_set rfds, wfds, efds;
    iec_manager_t *m = interface->mgr;
    struct timeval tv;
    int max_fd = -1;
    int rc = 0;

    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&efds);

    FD_SET(m->nc->sock_fd, &rfds);
    if(m->nc->send_buf.len > 0)
    {
        FD_SET(m->nc->sock_fd, &wfds);
        FD_SET(m->nc->sock_fd, &efds);
    }

    max_fd = m->nc->sock_fd;

    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    rc = select(max_fd + 1, &rfds, &wfds, &efds, &tv);
	now = iec_gettime_ms();

    if(rc == -1)
        IEC_LOG(LOG_ERR, "select() error");
    else if(rc > 0)
    {
        if(FD_ISSET(m->nc->sock_fd, &rfds)) {m->nc->flags |= IEC_FG_CAN_RD;}
        if(FD_ISSET(m->nc->sock_fd, &wfds)) {m->nc->flags |= IEC_FG_CAN_WR;}
        if(FD_ISSET(m->nc->sock_fd, &efds)) {m->nc->flags |= IEC_FG_ERR;}
    }

	iec_mgr_handle_conn(m->nc);


    return now;
}

int iec_sock_send(iec_connection_t *nc, const void *buf, size_t len)
{
    int rc;

    rc = send(nc->sock_fd, buf, len, 0);
    IEC_LOG(LOG_DEBUG, "sock send len:%d", rc);

    return rc;
}

int iec_sock_recv(iec_connection_t *nc, void *buf, size_t len)
{
    int rc;

    rc = recv(nc->sock_fd, buf, len, 0);
	IEC_LOG(LOG_DEBUG, "sock recv len:%d", rc);

    return rc;
}

iec_if_funcs_t linux_sock = 
{
    iec_sock_init,
    iec_sock_uninit,
    iec_sock_connect,
    iec_sock_discon,
    iec_sock_poll,
    iec_sock_send,
    iec_sock_recv,
};
