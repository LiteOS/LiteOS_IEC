#include "atiny_config.h"

#ifdef WITH_DTLS
const char server_name[] = SERVER_NAME;

const unsigned char mqtt_test_cas_pem[] = MQTT_TEST_CA_CRT;
const size_t mqtt_test_cas_pem_len = sizeof(mqtt_test_cas_pem);
const unsigned char mqtt_test_cli_pem[] = MQTT_TEST_CLI_CRT;
const size_t mqtt_test_cli_pem_len = sizeof(mqtt_test_cli_pem);
const unsigned char mqtt_test_cli_key[] = MQTT_TEST_CLI_KEY;
const size_t mqtt_test_cli_key_len = sizeof(mqtt_test_cli_key);
#endif
