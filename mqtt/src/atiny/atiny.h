#ifndef __ATINY_H__
#define __ATINY_H__

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>


#define ATINY_FG_UDP (1 << 0)
#define ATINY_FG_CONNECTING (1 << 1)

#define ATINY_FG_CAN_RD (1 << 2)
#define ATINY_FG_CAN_WR (1 << 3)
#define ATINY_FG_ERR    (1 << 4)

#define ATINY_FG_RECONNECT  (1 << 5)


#define ATINY_EV_POLL       1
#define ATINY_EV_CONNECTED  2
#define ATINY_EV_SEND       3
#define ATINY_EV_RECV       4
#define ATINY_EV_CLOSE      5

typedef unsigned long int atiny_time_t;

typedef struct atiny_connection atiny_connection_t;
typedef struct atiny_if atiny_if_t;
typedef struct atiny_buf atiny_buf_t;
typedef struct atiny_if_funcs atiny_if_funcs_t;
typedef struct atiny_manager atiny_manager_t;

typedef void (*atiny_event_handler)(atiny_connection_t *nc, int event, void *event_data);

typedef struct atiny_buf
{
    unsigned char *data;
    size_t len;    //data len
    size_t size;   //total size
} atiny_buf_t;

typedef struct atiny_manager
{
    atiny_connection_t *nc;
    atiny_if_t *interface;
} atiny_manager_t;

typedef struct atiny_connection
{
    atiny_manager_t *mgr;
    int flags;
    int sock_fd;
    struct sockaddr_in address;
    atiny_buf_t recv_buf;
    atiny_buf_t send_buf;
    atiny_time_t last_time;
    atiny_event_handler user_handler;
    atiny_event_handler proto_handler;
    void *user_data;
    void *proto_data;
#ifdef WITH_DTLS
    void *ssl_handler;
#endif
} atiny_connection_t;

typedef struct atiny_if
{
    atiny_manager_t *mgr;
    atiny_if_funcs_t *ifuncs;
} atiny_if_t;

typedef struct atiny_if_funcs
{
    void (*init)(atiny_if_t *interface);
    void (*uninit)(atiny_if_t *interface);
    void (*connect)(atiny_connection_t *nc);
    void (*discon)(atiny_connection_t *nc);
    atiny_time_t (*poll)(atiny_if_t *interface, int timeout_ms);
    int (*send)(atiny_connection_t *nc, const void *buf, size_t len);
    int (*recv)(atiny_connection_t *nc, void *buf, size_t len);
} atiny_if_funcs_t;

typedef struct atiny_device_info
{
    atiny_if_funcs_t *ifuncs;
} atiny_device_info_t;

typedef struct atiny_ssl_psk
{
    const unsigned char *psk_id;
    size_t psk_id_len;
    const unsigned char *psk;
    size_t psk_len;
} atiny_ssl_psk_t;


typedef struct atiny_ssl_ca
{
    const  char *server_name;
    const  unsigned char *ca_cert;
    size_t ca_cert_len;
    const  unsigned char *client_cert;
    size_t client_cert_len;
    const  unsigned char *client_key;
    size_t client_key_len;
} atiny_ssl_ca_t;

typedef union atiny_ssl_param
{
    atiny_ssl_psk_t psk;
    atiny_ssl_ca_t ca;
} atiny_ssl_param_u;


typedef struct atiny_connect_param
{
    int proto_type;
    char *server_ip;
    unsigned int server_port;
#ifdef WITH_DTLS
    // add some param used in dtls, like ca, key etc.
	atiny_ssl_param_u ssl_param;
#endif
} atiny_connect_param_t;

void atiny_register_proto(atiny_connection_t *nc, atiny_event_handler proto_handler);


void atiny_init(atiny_manager_t *m,  atiny_device_info_t *param);
atiny_connection_t* atiny_connect(atiny_manager_t *m, atiny_event_handler cb, atiny_connect_param_t param);
void atiny_poll(atiny_manager_t *m, int timeout_ms);


unsigned long int atiny_gettime_ms(void);

#endif
