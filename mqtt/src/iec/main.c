#include <pthread.h>
#include <unistd.h>
#include <string.h>

#include "iec_log.h"
#include "iec_adapter.h"
#include "iec.h"
#include "iec_mqtt.h"
#include "iec_config.h"
#include "mqtt_packet.h"


static iec_device_info_t default_dev_info;
extern iec_if_funcs_t linux_sock;

typedef struct iec_mqtt_builtin_topic
{
    char *topic;
    QoS_e qos;
	iec_mqtt_msg_handler cb;
} iec_mqtt_builtin_topic_t;

void dev_message_cb(void *msg)
{
    IEC_LOG(LOG_INFO, "dev_message_cb: %s",(char *)msg);
}

void twins_message_cb(void *msg)
{
    IEC_LOG(LOG_INFO, "twins_message_cb: %s",(char *)msg);
}

void dev_message_update_cb(void *msg)
{
    IEC_LOG(LOG_INFO, "dev_message_update_cb: %s",(char *)msg);
}

void twins_message_update_cb(void *msg)
{
    IEC_LOG(LOG_INFO, "twins_message_update_cb: %s",(char *)msg);
}

void dev_message_handle_cb(void *msg)
{
    IEC_LOG(LOG_INFO, "dev_message_handle_cb: %s",(char *)msg);
}


static iec_mqtt_builtin_topic_t ief_builtin_topic[IEC_MQTT_BUILTIN_NUM] = 
{
    {
        .topic = IEC_PROJECT_ID"/device/"IEC_DEVICE_ID"/properties/result",
        .qos = QOS0,
        .cb = dev_message_cb,
    },
    {
        .topic = IEC_PROJECT_ID"/device/"IEC_DEVICE_ID"/twins/result",
        .qos = QOS0,
        .cb = twins_message_cb,
    },
    {
        .topic = IEC_PROJECT_ID"/device/"IEC_DEVICE_ID"/properties/todevice",
        .qos = QOS0,
        .cb = dev_message_update_cb,
    },
    {
        .topic = IEC_PROJECT_ID"/device/"IEC_DEVICE_ID"/twins/expected",
        .qos = QOS0,
        .cb = twins_message_update_cb,
    },
    {
        .topic = IEC_PROJECT_ID"/device/"IEC_DEVICE_ID"/messages/todevice/#",
        .qos = QOS0,
        .cb = dev_message_handle_cb,
    },
};

void ev_handler(iec_connection_t *nc, int event, void *event_data)
{
    IEC_LOG(LOG_DEBUG, "ev_handler in main %d", event);
    iec_mqtt_msg_t *amm = (iec_mqtt_msg_t *)event_data;
    switch(event)
    {
        case IEC_EV_CONNECTED:
            {
                IEC_LOG(LOG_DEBUG, "now mqtt connect");
                mqtt_connect_opt_t options;

                iec_register_proto(nc, iec_mqtt_event_handler);
                memset(&options, 0, sizeof(options));
                options.connect_head = (mqtt_connect_head_t)MQTT_CONNECT_HEAD_INIT;
                options.connect_head.keep_alive = 60;
                options.connect_payload.client_id = "LiteIOT";
                options.connect_head.mqtt_connect_flag_u.bits.user_name_flag = 1;
                options.connect_head.mqtt_connect_flag_u.bits.psd_flag = 1;
                options.connect_payload.user_name = "LiteIOT";
                options.connect_payload.password = "123456";
                nc->proto_data = (void *)iec_malloc((size_t)sizeof(iec_mqtt_proto_data_t));
                iec_mqtt_connect(nc, &options);
            }
            break;
        case IEC_EC_RECONN:
            {
                IEC_LOG(LOG_DEBUG, "do socket re-connect@@@");
                #ifdef WITH_DTLS
                if(iec_ssl_reinit(nc)) break;
                #endif
                nc->mgr->interface->ifuncs->discon(nc);
                nc->mgr->interface->ifuncs->connect(nc);
            }
            break;
        case IEC_EV_MQTT_CONNACK:
            {
                IEC_LOG(LOG_DEBUG, "mqtt connect succuss");
                mqtt_subscribe_opt_t sub_options;
                sub_options.subscribe_payload.count = IEC_MQTT_BUILTIN_NUM;
                sub_options.subscribe_payload.topic = (char **)iec_malloc(IEC_MQTT_BUILTIN_NUM * sizeof(char **));
                sub_options.subscribe_payload.topic[0] = ief_builtin_topic[0].topic;
                sub_options.subscribe_payload.topic[1] = ief_builtin_topic[1].topic;
                sub_options.subscribe_payload.topic[2] = ief_builtin_topic[2].topic;
                sub_options.subscribe_payload.topic[3] = ief_builtin_topic[3].topic;
                sub_options.subscribe_payload.topic[4] = ief_builtin_topic[4].topic;
                sub_options.subscribe_payload.qoss[0] = ief_builtin_topic[0].qos;
                sub_options.subscribe_payload.qoss[1] = ief_builtin_topic[1].qos;
                sub_options.subscribe_payload.qoss[2] = ief_builtin_topic[2].qos;
                sub_options.subscribe_payload.qoss[3] = ief_builtin_topic[3].qos;
                sub_options.subscribe_payload.qoss[4] = ief_builtin_topic[4].qos;
                iec_mqtt_msg_handler cbs[IEC_MQTT_BUILTIN_NUM] = {ief_builtin_topic[0].cb, ief_builtin_topic[1].cb, ief_builtin_topic[2].cb, ief_builtin_topic[3].cb, ief_builtin_topic[4].cb};
                iec_mqtt_subscribe(nc, &sub_options, cbs);

                mqtt_publish_opt_t options;
                options.publish_head.topic = IEC_PROJECT_ID"/device/"IEC_DEVICE_ID"/properties";
                options.publish_payload.msg = "hello";
                options.dup = 0;
                options.qos = 1;
                options.retain = 0;
                iec_mqtt_publish(nc, &options);

                options.publish_head.topic = IEC_PROJECT_ID"/device/"IEC_DEVICE_ID"/properties/tocloud";
                options.publish_payload.msg = "hello1";
                options.dup = 0;
                options.qos = 1;
                options.retain = 0;
                iec_mqtt_publish(nc, &options);

                options.publish_head.topic = IEC_PROJECT_ID"/device/"IEC_DEVICE_ID"/twins";
                options.publish_payload.msg = "hello2";
                options.dup = 0;
                options.qos = 1;
                options.retain = 0;
                iec_mqtt_publish(nc, &options);

                mqtt_unsubscribe_opt_t unsub_options;
                unsub_options.unsubscribe_payload.count = 1;
                unsub_options.unsubscribe_payload.topic = (char **)iec_malloc(sizeof(char **));
                unsub_options.unsubscribe_payload.topic[0] = IEC_PROJECT_ID"/device/"IEC_DEVICE_ID"/properties/result";
                iec_mqtt_unsubscribe(nc, &unsub_options);
            }
            break;
        case IEC_EV_MQTT_PUBLISH:
            {
                IEC_LOG(LOG_DEBUG, "recv pushlish %s", (char *)amm->payload);
                int len = 0;
                mqtt_puback_opt_t options;

                if (amm->qos != QOS0)
                {
                    if (amm->qos == QOS1)
                    {
                        options.type = MQTT_PACKET_TYPE_PUBACK;
                        len = iec_mqtt_puback(nc, &options);
                    }
                    else if (amm->qos == QOS2)
                    {
                        options.type = MQTT_PACKET_TYPE_PUBREC;
                        len = iec_mqtt_puback(nc, &options);
                    }
                    if (len <= 0)
                        printf("error\n");
                }
           }
           break;
        default:
            break;
    }
}


int main()
{
    iec_manager_t mgr;
    iec_connect_param_t param;
    param.proto_type = SOCK_STREAM;
    param.server_ip = SERVER_IP;
    param.server_port = SERVER_PORT;
#ifdef WITH_DTLS
    param.ssl_param.ca.server_name = server_name;
    param.ssl_param.ca.ca_cert = mqtt_test_cas_pem;
    param.ssl_param.ca.ca_cert_len = mqtt_test_cas_pem_len;
    param.ssl_param.ca.client_cert = mqtt_test_cli_pem;
    param.ssl_param.ca.client_cert_len = mqtt_test_cli_pem_len;
    param.ssl_param.ca.client_key = mqtt_test_cli_key;
    param.ssl_param.ca.client_key_len = mqtt_test_cli_key_len;
#endif

    default_dev_info.ifuncs = &linux_sock;

    iec_init(&mgr, &default_dev_info);

    iec_connect(&mgr, ev_handler, param);
    for(;;)
    {
        iec_poll(&mgr, IEC_EVENTS_HANDLE_PERIOD_MS);
    }
}
