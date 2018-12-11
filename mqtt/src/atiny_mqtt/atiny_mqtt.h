
#include "mqtt_packet.h"
#include "atiny_config.h"

#define ATINY_EV_MQTT_BASE             0x100
#define ATINY_EV_MQTT_CONNECT          (ATINY_EV_MQTT_BASE + MQTT_PACKET_TYPE_CONNECT)
#define ATINY_EV_MQTT_CONNACK          (ATINY_EV_MQTT_BASE + MQTT_PACKET_TYPE_CONNACK)
#define ATINY_EV_MQTT_PUBLISH          (ATINY_EV_MQTT_BASE + MQTT_PACKET_TYPE_PUBLISH)
#define ATINY_EV_MQTT_PUBACK           (ATINY_EV_MQTT_BASE + MQTT_PACKET_TYPE_PUBACK)
#define ATINY_EV_MQTT_PUBREC           (ATINY_EV_MQTT_BASE + MQTT_PACKET_TYPE_PUBREC)
#define ATINY_EV_MQTT_PUBREL           (ATINY_EV_MQTT_BASE + MQTT_PACKET_TYPE_PUBREL)
#define ATINY_EV_MQTT_PUBCOMP          (ATINY_EV_MQTT_BASE + MQTT_PACKET_TYPE_PUBCOMP)
#define ATINY_EV_MQTT_SUBSCRIBE        (ATINY_EV_MQTT_BASE + MQTT_PACKET_TYPE_SUBSCRIBE)
#define ATINY_EV_MQTT_SUBACK           (ATINY_EV_MQTT_BASE + MQTT_PACKET_TYPE_SUBACK)
#define ATINY_EV_MQTT_UNSUBSCRIBE      (ATINY_EV_MQTT_BASE + MQTT_PACKET_TYPE_UNSUBSCRIBE)
#define ATINY_EV_MQTT_UNSUBACK         (ATINY_EV_MQTT_BASE + MQTT_PACKET_TYPE_UNSUBACK)
#define ATINY_EV_MQTT_PINGREQ          (ATINY_EV_MQTT_BASE + MQTT_PACKET_TYPE_PINGREQ)
#define ATINY_EV_MQTT_PINGRESP         (ATINY_EV_MQTT_BASE + MQTT_PACKET_TYPE_PINGRESP)
#define ATINY_EV_MQTT_DISCONNECT       (ATINY_EV_MQTT_BASE + MQTT_PACKET_TYPE_DISCONNECT)



#define MAX_PACKET_ID 0xFFFF

typedef enum QoS 
{
    QOS0, 
    QOS1, 
    QOS2
} QoS_e;

typedef struct atiny_mqtt_proto_data  atiny_mqtt_proto_data_t;

typedef struct atiny_mqtt_msg
{
    unsigned char type;
    QoS_e qos;
    unsigned int len;
    unsigned char retained;
    unsigned char dup;
    unsigned short id;
    void *payload;
    size_t payloadlen;
	atiny_mqtt_proto_data_t *mqtt_data;
} atiny_mqtt_msg_t;

typedef struct atiny_mqtt_suback_data
{
    QoS_e grantedQoS;
} atiny_mqtt_suback_data_t;


typedef void (*atiny_mqtt_msg_handler)(void *);

typedef struct atiny_mqtt_proto_data
{
    unsigned short keep_alive;
    atiny_time_t last_time;
    unsigned int next_packetid;
    struct MessageHandlers
    {
        unsigned char efficient;
        const char* topicFilter;
        atiny_mqtt_msg_handler cb;
    } messageHandlers[ATINY_MQTT_BUILTIN_NUM];      /* Message handlers are indexed by subscription topic */
} atiny_mqtt_proto_data_t;


int atiny_mqtt_packetid(atiny_connection_t *nc);
int atiny_mqtt_connect(atiny_connection_t *nc, mqtt_connect_opt_t *options);
int atiny_mqtt_publish(atiny_connection_t *nc, mqtt_publish_opt_t *options);
int atiny_mqtt_subscribe(atiny_connection_t *nc, mqtt_subscribe_opt_t *options, atiny_mqtt_msg_handler *cbs);
int atiny_mqtt_puback(atiny_connection_t *nc, mqtt_puback_opt_t *options);
int atiny_mqtt_ping(atiny_connection_t *nc);
int atiny_mqtt_unsubscribe(atiny_connection_t *nc, mqtt_unsubscribe_opt_t *options);
void atiny_mqtt_event_handler(atiny_connection_t *nc, int event, void *event_data);
