#ifndef __IEC_MQTT_H__
#define __IEC_MQTT_H__
#include "mqtt_packet.h"
#include "iec_config.h"

#define IEC_EV_MQTT_BASE             0x100
#define IEC_EV_MQTT_CONNECT          (IEC_EV_MQTT_BASE + MQTT_PACKET_TYPE_CONNECT)
#define IEC_EV_MQTT_CONNACK          (IEC_EV_MQTT_BASE + MQTT_PACKET_TYPE_CONNACK)
#define IEC_EV_MQTT_PUBLISH          (IEC_EV_MQTT_BASE + MQTT_PACKET_TYPE_PUBLISH)
#define IEC_EV_MQTT_PUBACK           (IEC_EV_MQTT_BASE + MQTT_PACKET_TYPE_PUBACK)
#define IEC_EV_MQTT_PUBREC           (IEC_EV_MQTT_BASE + MQTT_PACKET_TYPE_PUBREC)
#define IEC_EV_MQTT_PUBREL           (IEC_EV_MQTT_BASE + MQTT_PACKET_TYPE_PUBREL)
#define IEC_EV_MQTT_PUBCOMP          (IEC_EV_MQTT_BASE + MQTT_PACKET_TYPE_PUBCOMP)
#define IEC_EV_MQTT_SUBSCRIBE        (IEC_EV_MQTT_BASE + MQTT_PACKET_TYPE_SUBSCRIBE)
#define IEC_EV_MQTT_SUBACK           (IEC_EV_MQTT_BASE + MQTT_PACKET_TYPE_SUBACK)
#define IEC_EV_MQTT_UNSUBSCRIBE      (IEC_EV_MQTT_BASE + MQTT_PACKET_TYPE_UNSUBSCRIBE)
#define IEC_EV_MQTT_UNSUBACK         (IEC_EV_MQTT_BASE + MQTT_PACKET_TYPE_UNSUBACK)
#define IEC_EV_MQTT_PINGREQ          (IEC_EV_MQTT_BASE + MQTT_PACKET_TYPE_PINGREQ)
#define IEC_EV_MQTT_PINGRESP         (IEC_EV_MQTT_BASE + MQTT_PACKET_TYPE_PINGRESP)
#define IEC_EV_MQTT_DISCONNECT       (IEC_EV_MQTT_BASE + MQTT_PACKET_TYPE_DISCONNECT)



#define MAX_PACKET_ID 0xFFFF

typedef enum QoS 
{
    QOS0, 
    QOS1, 
    QOS2
} QoS_e;

typedef struct iec_mqtt_proto_data  iec_mqtt_proto_data_t;

typedef struct iec_mqtt_msg
{
    unsigned char type;
    QoS_e qos;
    unsigned int len;
    unsigned char retained;
    unsigned char dup;
    unsigned short id;
    void *payload;
    size_t payloadlen;
	iec_mqtt_proto_data_t *mqtt_data;
} iec_mqtt_msg_t;

typedef struct iec_mqtt_suback_data
{
    QoS_e grantedQoS;
} iec_mqtt_suback_data_t;


typedef void (*iec_mqtt_msg_handler)(void *);

typedef struct iec_mqtt_proto_data
{
    unsigned short keep_alive;
    iec_time_t last_time;
    unsigned int next_packetid;
    struct MessageHandlers
    {
        unsigned char efficient;
        const char* topicFilter;
        iec_mqtt_msg_handler cb;
    } messageHandlers[IEC_MQTT_BUILTIN_NUM];      /* Message handlers are indexed by subscription topic */
} iec_mqtt_proto_data_t;


int iec_mqtt_packetid(iec_connection_t *nc);
int iec_mqtt_connect(iec_connection_t *nc, mqtt_connect_opt_t *options);
int iec_mqtt_publish(iec_connection_t *nc, mqtt_publish_opt_t *options);
int iec_mqtt_subscribe(iec_connection_t *nc, mqtt_subscribe_opt_t *options, iec_mqtt_msg_handler *cbs);
int iec_mqtt_puback(iec_connection_t *nc, mqtt_puback_opt_t *options);
int iec_mqtt_ping(iec_connection_t *nc);
int iec_mqtt_unsubscribe(iec_connection_t *nc, mqtt_unsubscribe_opt_t *options);
void iec_mqtt_event_handler(iec_connection_t *nc, int event, void *event_data);

#endif
