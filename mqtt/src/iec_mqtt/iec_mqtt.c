#include <string.h>

#include "iec.h"
#include "iec_log.h"
#include "iec_mqtt.h"

int iec_mqtt_packetid(iec_connection_t *nc)
{
    iec_mqtt_proto_data_t *pd = (iec_mqtt_proto_data_t *)nc->proto_data;
    return pd->next_packetid = (pd->next_packetid == MAX_PACKET_ID) ? 1 : pd->next_packetid + 1;
}


int iec_mqtt_parser(iec_buf_t *io, iec_mqtt_msg_t *amm)
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
                        IEC_LOG(LOG_DEBUG, "suback msgid:%d\n", options.suback_head.packet_id);
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
                IEC_LOG(LOG_DEBUG, "publish payload len:%d", (int)amm->payloadlen);
                amm->payload = options.publish_payload.msg;
                for(i = 0; i < IEC_MQTT_BUILTIN_NUM; i++)
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
    IEC_LOG(LOG_DEBUG, "real len:%d", (int)amm->len);
    return amm->len;
}

void iec_mqtt_event_handler(iec_connection_t *nc, int event, void *event_data)
{
    int len;
    iec_mqtt_msg_t amm = {0};
    iec_mqtt_proto_data_t *data = (iec_mqtt_proto_data_t *)nc->proto_data;
    amm.mqtt_data = data;
    iec_time_t now;
    iec_buf_t *io = &nc->recv_buf;
    nc->user_handler(nc, event, event_data);
    switch(event)
    {
        case IEC_EV_RECV:
            while(1)
            {
                len = iec_mqtt_parser(io, &amm);
                if(len <= 0)
                {
                    /* TODO */
                    break;
                }
                nc->user_handler(nc, IEC_EV_MQTT_BASE + amm.type, &amm);
                memmove(nc->recv_buf.data, nc->recv_buf.data + len, nc->recv_buf.len - len);
                nc->recv_buf.len -= len;
            }
            break;
        case IEC_EV_POLL:
            now = iec_gettime_ms();
            IEC_LOG(LOG_DEBUG, "poll 0x%ld  0x%ld", now, data->last_time);
            if(((now - data->last_time) > data->keep_alive*1000) && (data->last_time > 0) && (data->keep_alive > 0))
            {
                IEC_LOG(LOG_DEBUG, "Send ping request");
                iec_mqtt_ping(nc);
            }
            break;
    }
}

int iec_mqtt_connect(iec_connection_t *nc, mqtt_connect_opt_t *options)
{
    int rc = -1;
    iec_mqtt_proto_data_t *data;
    mqtt_connect_opt_t default_options = (mqtt_connect_opt_t)MQTT_CONNECT_OPT_INIT;
    int len = 0;

    if (options == NULL)
        options = &default_options; /* set default options if none were supplied */

    data = (iec_mqtt_proto_data_t *)nc->proto_data;
    data->keep_alive = options->connect_head.keep_alive;
    data->next_packetid = 1;

    if((len = mqtt_encode_connect((nc->send_buf.data + nc->send_buf.len), (nc->send_buf.size - nc->send_buf.len), options)) <= 0)
    {
        IEC_LOG(LOG_ERR, "mqtt connect error");
		return rc;
    }
    nc->send_buf.len += len;
    data->last_time = iec_gettime_ms();

    return 0;
}

int iec_mqtt_ping(iec_connection_t *nc)
{
    int len = 0;
    iec_mqtt_proto_data_t *data;
    data = (iec_mqtt_proto_data_t *)nc->proto_data;
    len = mqtt_encode_ping((nc->send_buf.data + nc->send_buf.len), (nc->send_buf.size - nc->send_buf.len));
    if(len > 0)
        nc->send_buf.len += len;
    data->last_time = iec_gettime_ms();

    return len;
}

int iec_mqtt_publish(iec_connection_t *nc, mqtt_publish_opt_t *options)
{
    int len = 0;

    iec_mqtt_proto_data_t *data;
    data = (iec_mqtt_proto_data_t *)nc->proto_data;

    if(options->qos)
        options->publish_head.packet_id = iec_mqtt_packetid(nc);
    len = mqtt_encode_publish((nc->send_buf.data + nc->send_buf.len), (nc->send_buf.size - nc->send_buf.len), options);
    if(len > 0)
        nc->send_buf.len += len;
    data->last_time = iec_gettime_ms();

    return len;
}

int iec_mqtt_subscribe(iec_connection_t *nc, mqtt_subscribe_opt_t *options, iec_mqtt_msg_handler *cbs)
{
    int len = 0;
    int i = 0;
    iec_mqtt_proto_data_t *data;
    data = (iec_mqtt_proto_data_t *)nc->proto_data;
    options->subscribe_head.packet_id = iec_mqtt_packetid(nc);

    for( i = 0; i < options->subscribe_payload.count; i++)
    {
        data->messageHandlers[i].topicFilter = options->subscribe_payload.topic[i];
        data->messageHandlers[i].cb = cbs[i];
    }

    len = mqtt_encode_subscribe((nc->send_buf.data + nc->send_buf.len), (nc->send_buf.size - nc->send_buf.len), options);
    if(len > 0)
        nc->send_buf.len += len;
    data->last_time = iec_gettime_ms();

    return len;
}

int iec_mqtt_puback(iec_connection_t *nc, mqtt_puback_opt_t *options)
{
    int len = 0;
    iec_mqtt_proto_data_t *data;
    data = (iec_mqtt_proto_data_t *)nc->proto_data;
    options->puback_head.packet_id = iec_mqtt_packetid(nc);

    len = mqtt_encode_puback((nc->send_buf.data + nc->send_buf.len), (nc->send_buf.size - nc->send_buf.len), options);
    if(len > 0)
        nc->send_buf.len += len;
    data->last_time = iec_gettime_ms();

    return len;
}

int iec_mqtt_unsubscribe(iec_connection_t *nc, mqtt_unsubscribe_opt_t *options)
{
    int len = 0;
    iec_mqtt_proto_data_t *data;
    data = (iec_mqtt_proto_data_t *)nc->proto_data;
    options->unsubscribe_head.packet_id = iec_mqtt_packetid(nc);

    len = mqtt_encode_unsubscribe((nc->send_buf.data + nc->send_buf.len), (nc->send_buf.size - nc->send_buf.len), options);
    if(len > 0)
        nc->send_buf.len += len;
    data->last_time = iec_gettime_ms();

    return len;
}
