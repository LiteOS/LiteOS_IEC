#ifndef __IEC_H__
#define __IEC_H__

#include <stdio.h>
#include <stdlib.h>
#ifdef WITH_LWIP
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/errno.h"
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#endif


#define IEC_FG_UDP (1 << 0)
#define IEC_FG_CONNECTING (1 << 1)

#define IEC_FG_CAN_RD (1 << 2)
#define IEC_FG_CAN_WR (1 << 3)
#define IEC_FG_ERR    (1 << 4)

#define IEC_FG_RECONNECT  (1 << 5)


#define IEC_EV_POLL       (1)
#define IEC_EV_CONNECTED  (2)
#define IEC_EV_SEND       (3)
#define IEC_EV_RECV       (4)
#define IEC_EV_CLOSE      (5)
#define IEC_EC_RECONN     (6)

typedef unsigned long int iec_time_t;

typedef struct iec_connection iec_connection_t;
typedef struct iec_if iec_if_t;
typedef struct iec_buf iec_buf_t;
typedef struct iec_if_funcs iec_if_funcs_t;
typedef struct iec_manager iec_manager_t;

typedef void (*iec_event_handler)(iec_connection_t *nc, int event, void *event_data);

typedef struct iec_buf
{
    unsigned char *data;
    size_t len;    //data len
    size_t size;   //total size
} iec_buf_t;

typedef struct iec_manager
{
    iec_connection_t *nc;
    iec_if_t *interface;
} iec_manager_t;

typedef struct iec_connection
{
    iec_manager_t *mgr;
    int flags;
    int sock_fd;
    struct sockaddr_in address;
    iec_buf_t recv_buf;
    iec_buf_t send_buf;
    iec_time_t last_time;
    iec_event_handler user_handler;
    iec_event_handler proto_handler;
    void *user_data;
    void *proto_data;
#ifdef WITH_DTLS
    void *ssl_handler;
#endif
} iec_connection_t;

typedef struct iec_if
{
    iec_manager_t *mgr;
    iec_if_funcs_t *ifuncs;
} iec_if_t;

typedef struct iec_if_funcs
{
    void (*if_init)(iec_if_t *interface);
    void (*if_uninit)(iec_if_t *interface);
    void (*if_connect)(iec_connection_t *nc);
    void (*if_discon)(iec_connection_t *nc);
    iec_time_t (*if_poll)(iec_if_t *interface, int timeout_ms);
    int (*if_send)(iec_connection_t *nc, const void *buf, size_t len);
    int (*if_recv)(iec_connection_t *nc, void *buf, size_t len);
} iec_if_funcs_t;

typedef struct iec_device_info
{
    iec_if_funcs_t *ifuncs;
} iec_device_info_t;

typedef struct iec_ssl_psk
{
    const unsigned char *psk_id;
    size_t psk_id_len;
    const unsigned char *psk;
    size_t psk_len;
} iec_ssl_psk_t;


typedef struct iec_ssl_ca
{
    const  char *server_name;
    const  unsigned char *ca_cert;
    size_t ca_cert_len;
    const  unsigned char *client_cert;
    size_t client_cert_len;
    const  unsigned char *client_key;
    size_t client_key_len;
} iec_ssl_ca_t;

typedef union iec_ssl_param
{
    iec_ssl_psk_t psk;
    iec_ssl_ca_t ca;
} iec_ssl_param_u;


typedef struct iec_connect_param
{
    int proto_type;
    char *server_ip;
    unsigned int server_port;
#ifdef WITH_DTLS
    // add some param used in dtls, like ca, key etc.
	iec_ssl_param_u ssl_param;
#endif
} iec_connect_param_t;

void iec_register_proto(iec_connection_t *nc, iec_event_handler proto_handler);


void iec_init(iec_manager_t *m,  iec_device_info_t *param);
iec_connection_t* iec_connect(iec_manager_t *m, iec_event_handler cb, iec_connect_param_t param);
void iec_poll(iec_manager_t *m, int timeout_ms);


unsigned long int iec_gettime_ms(void);

#endif
