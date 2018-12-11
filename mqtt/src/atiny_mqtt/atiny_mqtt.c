#include <string.h>

#include "atiny.h"
#include "atiny_log.h"
#include "atiny_mqtt.h"

int atiny_mqtt_packetid(atiny_connection_t *nc)
{
    atiny_mqtt_proto_data_t *pd = (atiny_mqtt_proto_data_t *)nc->proto_data;
    return pd->next_packetid = (pd->next_packetid == MAX_PACKET_ID) ? 1 : pd->next_packetid + 1;
}


int atiny_mqtt_parser(atiny_buf_t *io, atiny_mqtt_msg_t *amm)
{
    int len = 0;
    int rem_len = 0;

    len = mqtt_decode_fixhead(io->data, &amm->type, &amm->dup, (unsigned char *)&amm->qos, &amm->retained, &rem_len);

    if(io->len < 2) return MQTTPACKET_BUFFER_TOO_SHORT;

    switch(amm->type)
    {
        case MQTT_PACKET_TYPE_PINGREQ:
            break;
        case MQTT_PACKET_TYPE_CONNACK:
        case MQTT_PACKET_TYPE_PUBACK:
            break;
        case MQTT_PACKET_TYPE_SUBACK:
            {
                int i = 0;
                mqtt_suback_opt_t options;
                mqtt_decode_suback(io->data, len, &options);
                for(i = 0; i < options.count; i++)
                {
                    if(options.suback_payload.ret_code[i] != 0x80)
                    {
                        ATINY_LOG(LOG_DEBUG, "suback msgid:%d\n", options.suback_head.packet_id);
						amm->mqtt_data->messageHandlers[i].efficient = 1;
                    }
                    else
                    {
                        amm->mqtt_data->messageHandlers[i].efficient = 0;
                    }
                }
            }
            break;
        case MQTT_PACKET_TYPE_UNSUBACK:
            break;
        case MQTT_PACKET_TYPE_PUBLISH:
            {
                mqtt_publish_opt_t options;
                int i = 0;

                mqtt_decode_publish(io->data, len, &options);
                amm->payloadlen = options.publish_payload.msg_len;
                ATINY_LOG(LOG_DEBUG, "publish payload len:%d", (int)amm->payloadlen);
                amm->payload = options.publish_payload.msg;
                for(i = 0; i < ATINY_MQTT_BUILTIN_NUM; i++)
                {
                    if(memcmp(options.publish_head.topic, amm->mqtt_data->messageHandlers[i].topicFilter, options.publish_head.topic_len) == 0)
                    {
                        if(amm->mqtt_data->messageHandlers[i].efficient)
                        {
                            amm->mqtt_data->messageHandlers[i].cb(amm->payload);
                        }
                    }
                }
            }
            break;
    }

    amm->len = len + rem_len;
    ATINY_LOG(LOG_DEBUG, "real len:%d", (int)amm->len);
    return amm->len;
}

void atiny_mqtt_event_handler(atiny_connection_t *nc, int event, void *event_data)
{
    int len;
    atiny_mqtt_msg_t amm = {0};
    atiny_mqtt_proto_data_t *data = (atiny_mqtt_proto_data_t *)nc->proto_data;
    amm.mqtt_data = data;
    atiny_time_t now;
    atiny_buf_t *io = &nc->recv_buf;
    nc->user_handler(nc, event, event_data);
    switch(event)
    {
        case ATINY_EV_RECV:
            while(1)
            {
                len = atiny_mqtt_parser(io, &amm);
                if(len <= 0)
                {
                    /* TODO */
                    break;
                }
                nc->user_handler(nc, ATINY_EV_MQTT_BASE + amm.type, &amm);
                memmove(nc->recv_buf.data, nc->recv_buf.data + len, nc->recv_buf.len - len);
                nc->recv_buf.len -= len;
            }
            break;
        case ATINY_EV_POLL:
            now = atiny_gettime_ms();
            ATINY_LOG(LOG_DEBUG, "poll 0x%ld  0x%ld", now, data->last_time);
            if(((now - data->last_time) > data->keep_alive*1000) && (data->last_time > 0) && (data->keep_alive > 0))
            {
                ATINY_LOG(LOG_DEBUG, "Send ping request");
                atiny_mqtt_ping(nc);
            }
            break;
    }
}

int atiny_mqtt_connect(atiny_connection_t *nc, mqtt_connect_opt_t *options)
{
    int rc = -1;
    atiny_mqtt_proto_data_t *data;
    mqtt_connect_opt_t default_options = (mqtt_connect_opt_t)MQTT_CONNECT_OPT_INIT;
    int len = 0;

    if (options == NULL)
        options = &default_options; /* set default options if none were supplied */

    data = (atiny_mqtt_proto_data_t *)nc->proto_data;
    data->keep_alive = options->connect_head.keep_alive;
    data->next_packetid = 1;

    if((len = mqtt_encode_connect((nc->send_buf.data + nc->send_buf.len), (nc->send_buf.size - nc->send_buf.len), options)) <= 0)
    {
        ATINY_LOG(LOG_ERR, "mqtt connect error");
		return rc;
    }
    nc->send_buf.len += len;
    data->last_time = atiny_gettime_ms();

    return 0;
}

int atiny_mqtt_ping(atiny_connection_t *nc)
{
    int len = 0;
    atiny_mqtt_proto_data_t *data;
    data = (atiny_mqtt_proto_data_t *)nc->proto_data;
    len = mqtt_encode_ping((nc->send_buf.data + nc->send_buf.len), (nc->send_buf.size - nc->send_buf.len));
    if(len > 0)
        nc->send_buf.len += len;
    data->last_time = atiny_gettime_ms();

    return len;
}

int atiny_mqtt_publish(atiny_connection_t *nc, mqtt_publish_opt_t *options)
{
    int len = 0;

    atiny_mqtt_proto_data_t *data;
    data = (atiny_mqtt_proto_data_t *)nc->proto_data;

    if(options->qos)
        options->publish_head.packet_id = atiny_mqtt_packetid(nc);
    len = mqtt_encode_publish((nc->send_buf.data + nc->send_buf.len), (nc->send_buf.size - nc->send_buf.len), options);
    if(len > 0)
        nc->send_buf.len += len;
    data->last_time = atiny_gettime_ms();

    return len;
}

int atiny_mqtt_subscribe(atiny_connection_t *nc, mqtt_subscribe_opt_t *options, atiny_mqtt_msg_handler *cbs)
{
    int len = 0;
    int i = 0;
    atiny_mqtt_proto_data_t *data;
    data = (atiny_mqtt_proto_data_t *)nc->proto_data;
    options->subscribe_head.packet_id = atiny_mqtt_packetid(nc);

    for( i = 0; i < options->subscribe_payload.count; i++)
    {
        data->messageHandlers[i].topicFilter = options->subscribe_payload.topic[i];
        data->messageHandlers[i].cb = cbs[i];
    }

    len = mqtt_encode_subscribe((nc->send_buf.data + nc->send_buf.len), (nc->send_buf.size - nc->send_buf.len), options);
    if(len > 0)
        nc->send_buf.len += len;
    data->last_time = atiny_gettime_ms();

    return len;
}

int atiny_mqtt_puback(atiny_connection_t *nc, mqtt_puback_opt_t *options)
{
    int len = 0;
    atiny_mqtt_proto_data_t *data;
    data = (atiny_mqtt_proto_data_t *)nc->proto_data;
    options->puback_head.packet_id = atiny_mqtt_packetid(nc);

    len = mqtt_encode_puback((nc->send_buf.data + nc->send_buf.len), (nc->send_buf.size - nc->send_buf.len), options);
    if(len > 0)
        nc->send_buf.len += len;
    data->last_time = atiny_gettime_ms();

    return len;
}

int atiny_mqtt_unsubscribe(atiny_connection_t *nc, mqtt_unsubscribe_opt_t *options)
{
    int len = 0;
    atiny_mqtt_proto_data_t *data;
    data = (atiny_mqtt_proto_data_t *)nc->proto_data;
    options->unsubscribe_head.packet_id = atiny_mqtt_packetid(nc);

    len = mqtt_encode_unsubscribe((nc->send_buf.data + nc->send_buf.len), (nc->send_buf.size - nc->send_buf.len), options);
    if(len > 0)
        nc->send_buf.len += len;
    data->last_time = atiny_gettime_ms();

    return len;
}
