#include <string.h>

#include "atiny.h"
#include "atiny_log.h"
#ifdef WITH_DTLS
#include "atiny_mbed_ssl.h"
#endif

int atiny_parse_address(struct sockaddr_in *addr, char *server_ip, unsigned int server_port)
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


void atiny_dispatch_event(atiny_connection_t *nc, atiny_event_handler event_handler, void *user_data, int event, void *event_data)
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

void atiny_nc_connect_cb(atiny_connection_t *nc)
{
#if WITH_DTLS
    int ret;
#endif

    ATINY_LOG(LOG_DEBUG, "atiny_nc_connect_cb");
    nc->flags &= ~ATINY_FG_CONNECTING;
#if WITH_DTLS
    ret = atiny_ssl_handshake(nc);
    if(ret == 0)
    {
        nc->flags &= ~(ATINY_FG_CAN_RD | ATINY_FG_CAN_WR);
    }
    else if(ret == MBEDTLS_ERR_SSL_WANT_READ)
        nc->flags |= ATINY_FG_CAN_RD;
    else if(ret == MBEDTLS_ERR_SSL_WANT_WRITE)
        nc->flags |= ATINY_FG_CAN_WR;
#endif

    atiny_dispatch_event(nc, NULL, NULL, ATINY_EV_CONNECTED,NULL);
}

void atiny_nc_can_write_cb(atiny_connection_t *nc)
{
    int rc = 0;
    const unsigned char *buf = nc->send_buf.data;
    size_t len = nc->send_buf.len;
    ATINY_LOG(LOG_DEBUG, "atiny_nc_can_write_cb len:%d", (int)len);

    nc->flags &= ~ATINY_FG_CAN_WR;
#if WITH_DTLS
    if(len > 0)
        rc = mbedtls_ssl_write(((atiny_ssl_ctx_t *)nc->ssl_handler)->ssl,  buf, len);

    if (rc == MBEDTLS_ERR_SSL_WANT_WRITE)
    {
        nc->flags |= ATINY_FG_CAN_WR;
    }
    else if (rc < 0)
    {
        nc->flags |= ATINY_FG_RECONNECT;
    }
    else
    {
        nc->flags &= ~ATINY_FG_CAN_WR;
    }
#else
    if(len > 0)
        rc = nc->mgr->interface->ifuncs->send(nc, buf, len);
#endif

    if(rc < 0)
    {
        ATINY_LOG(LOG_ERR, "atiny write error");
        nc->flags |= ATINY_FG_RECONNECT;
    }
    else if(rc > 0)
    {
        nc->last_time = atiny_gettime_ms();
        if(nc->send_buf.len > rc)
            memmove(nc->send_buf.data, nc->send_buf.data + rc, nc->send_buf.len - rc);
        nc->send_buf.len -= rc;
    }
    atiny_dispatch_event(nc, NULL, NULL, ATINY_EV_SEND, NULL);
}

void atiny_nc_can_read_cb(atiny_connection_t *nc)
{
    int rc = 0;
    unsigned char *buf = nc->recv_buf.data + nc->recv_buf.len;
    size_t len = nc->recv_buf.size - nc->recv_buf.len;
    nc->flags &= ~ATINY_FG_CAN_RD;

    ATINY_LOG(LOG_DEBUG, "atiny_nc_can_read_cb len:%d", (int)len);

#if WITH_DTLS
    if(len > 0)  rc = mbedtls_ssl_read(((atiny_ssl_ctx_t *)nc->ssl_handler)->ssl, (unsigned char *) buf, len);

    if (rc == MBEDTLS_ERR_SSL_WANT_READ)
    {
        nc->flags |= ATINY_FG_CAN_RD;
    }
    else if (rc < 0)
    {
        nc->flags |= ATINY_FG_RECONNECT;
    }
    else
    {
        nc->flags &= ~ATINY_FG_CAN_RD;
    }
#else
    if(len > 0)
        rc = nc->mgr->interface->ifuncs->recv(nc, buf, len);
#endif

    if(rc< 0)
    {
        ATINY_LOG(LOG_ERR, "read error");
		nc->flags |= ATINY_FG_RECONNECT;
    }
    else if(rc > 0)
    {
        nc->last_time = atiny_gettime_ms();
        nc->recv_buf.len += rc;
        atiny_dispatch_event(nc, NULL, NULL, ATINY_EV_RECV, NULL);
    }
}

static void atiny_nc_poll_cb(atiny_connection_t *nc)
{
    unsigned long int now = atiny_gettime_ms();
    atiny_dispatch_event(nc, NULL, NULL, ATINY_EV_POLL, &now);
}

static void atiny_mgr_handle_conn(atiny_connection_t *nc)
{
    atiny_nc_poll_cb(nc);

    if(nc->flags & ATINY_FG_CONNECTING)
    {
        atiny_nc_connect_cb(nc);
    }

    if(nc->flags & ATINY_FG_CAN_RD)
    {
        atiny_nc_can_read_cb(nc);
    }

    if(nc->flags & ATINY_FG_CAN_WR)
    {
        atiny_nc_can_write_cb(nc);
    }
}


void atiny_sock_init(atiny_if_t *interface)
{
    (void)interface;
    ATINY_LOG(LOG_DEBUG, " using select()");
}

void atiny_sock_uninit(atiny_if_t *interface)
{

}

void atiny_sock_connect(atiny_connection_t *nc)
{
    int rc = 0;

    nc->sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (nc->sock_fd == -1) {
        ATINY_LOG(LOG_ERR, "socket error");
        return;
    }

    rc = connect(nc->sock_fd, (struct sockaddr *)&nc->address, sizeof(nc->address));
    if(rc < 0)
        ATINY_LOG(LOG_ERR, "sock %d rc %d ",  nc->sock_fd, rc);
}

void atiny_sock_discon(atiny_connection_t *nc)
{
    int rc = 0;

    if(nc->sock_fd == -1)
        return;

    rc = close(nc->sock_fd);
    if(rc < 0)
        ATINY_LOG(LOG_ERR, "sock %d rc %d ",  nc->sock_fd, rc);
}


atiny_time_t atiny_sock_poll(atiny_if_t *interface, int timeout_ms)
{
    atiny_time_t now;
    fd_set rfds, wfds, efds;
    atiny_manager_t *m = interface->mgr;
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
	now = atiny_gettime_ms();

    if(rc == -1)
        ATINY_LOG(LOG_ERR, "select() error");
    else if(rc > 0)
    {
        if(FD_ISSET(m->nc->sock_fd, &rfds)) {m->nc->flags |= ATINY_FG_CAN_RD;}
        if(FD_ISSET(m->nc->sock_fd, &wfds)) {m->nc->flags |= ATINY_FG_CAN_WR;}
        if(FD_ISSET(m->nc->sock_fd, &efds)) {m->nc->flags |= ATINY_FG_ERR;}
    }

	atiny_mgr_handle_conn(m->nc);


    return now;
}

int atiny_sock_send(atiny_connection_t *nc, const void *buf, size_t len)
{
    int rc;

    rc = send(nc->sock_fd, buf, len, 0);
    ATINY_LOG(LOG_DEBUG, "sock send len:%d", rc);

    return rc;
}

int atiny_sock_recv(atiny_connection_t *nc, void *buf, size_t len)
{
    int rc;

    rc = recv(nc->sock_fd, buf, len, 0);
	ATINY_LOG(LOG_DEBUG, "sock recv len:%d", rc);

    return rc;
}

atiny_if_funcs_t linux_sock = 
{
    atiny_sock_init,
    atiny_sock_uninit,
    atiny_sock_connect,
    atiny_sock_discon,
    atiny_sock_poll,
    atiny_sock_send,
    atiny_sock_recv,
};
