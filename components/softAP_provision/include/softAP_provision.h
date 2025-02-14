#ifndef SOFTAP_PROVISION_H
#define SOFTAP_PROVISION_H
#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_event.h"
#include "cJSON.h"
#include "thingsboard_types.h"
#include "softap_provision_types.h"


#define PROV_QR_VERSION         "v1"
#define PROV_TRANSPORT_SOFTAP   "softap"

/*[NVS]*/
typedef enum {
    THINGSBOARD_URL_OBTAINED,
} provision_event_t;

#if CONFIG_EXAMPLE_PROV_SECURITY_VERSION_2
#if CONFIG_EXAMPLE_PROV_SEC2_DEV_MODE
#define EXAMPLE_PROV_SEC2_USERNAME          "wifiprov"
#define EXAMPLE_PROV_SEC2_PWD               "abcd1234"

/* This salt,verifier has been generated for username = "wifiprov" and password = "abcd1234"
   IMPORTANT NOTE: For production cases, this must be unique to every device
   and should come from device manufacturing partition.*/
static const char sec2_salt[] = {
    0x03, 0x6e, 0xe0, 0xc7, 0xbc, 0xb9, 0xed, 0xa8, 0x4c, 0x9e, 0xac, 0x97, 0xd9, 0x3d, 0xec, 0xf4
};

static const char sec2_verifier[] = {
    0x7c, 0x7c, 0x85, 0x47, 0x65, 0x08, 0x94, 0x6d, 0xd6, 0x36, 0xaf, 0x37, 0xd7, 0xe8, 0x91, 0x43,
    0x78, 0xcf, 0xfd, 0x61, 0x6c, 0x59, 0xd2, 0xf8, 0x39, 0x08, 0x12, 0x72, 0x38, 0xde, 0x9e, 0x24,
    0xa4, 0x70, 0x26, 0x1c, 0xdf, 0xa9, 0x03, 0xc2, 0xb2, 0x70, 0xe7, 0xb1, 0x32, 0x24, 0xda, 0x11,
    0x1d, 0x97, 0x18, 0xdc, 0x60, 0x72, 0x08, 0xcc, 0x9a, 0xc9, 0x0c, 0x48, 0x27, 0xe2, 0xae, 0x89,
    0xaa, 0x16, 0x25, 0xb8, 0x04, 0xd2, 0x1a, 0x9b, 0x3a, 0x8f, 0x37, 0xf6, 0xe4, 0x3a, 0x71, 0x2e,
    0xe1, 0x27, 0x86, 0x6e, 0xad, 0xce, 0x28, 0xff, 0x54, 0x46, 0x60, 0x1f, 0xb9, 0x96, 0x87, 0xdc,
    0x57, 0x40, 0xa7, 0xd4, 0x6c, 0xc9, 0x77, 0x54, 0xdc, 0x16, 0x82, 0xf0, 0xed, 0x35, 0x6a, 0xc4,
    0x70, 0xad, 0x3d, 0x90, 0xb5, 0x81, 0x94, 0x70, 0xd7, 0xbc, 0x65, 0xb2, 0xd5, 0x18, 0xe0, 0x2e,
    0xc3, 0xa5, 0xf9, 0x68, 0xdd, 0x64, 0x7b, 0xb8, 0xb7, 0x3c, 0x9c, 0xfc, 0x00, 0xd8, 0x71, 0x7e,
    0xb7, 0x9a, 0x7c, 0xb1, 0xb7, 0xc2, 0xc3, 0x18, 0x34, 0x29, 0x32, 0x43, 0x3e, 0x00, 0x99, 0xe9,
    0x82, 0x94, 0xe3, 0xd8, 0x2a, 0xb0, 0x96, 0x29, 0xb7, 0xdf, 0x0e, 0x5f, 0x08, 0x33, 0x40, 0x76,
    0x52, 0x91, 0x32, 0x00, 0x9f, 0x97, 0x2c, 0x89, 0x6c, 0x39, 0x1e, 0xc8, 0x28, 0x05, 0x44, 0x17,
    0x3f, 0x68, 0x02, 0x8a, 0x9f, 0x44, 0x61, 0xd1, 0xf5, 0xa1, 0x7e, 0x5a, 0x70, 0xd2, 0xc7, 0x23,
    0x81, 0xcb, 0x38, 0x68, 0xe4, 0x2c, 0x20, 0xbc, 0x40, 0x57, 0x76, 0x17, 0xbd, 0x08, 0xb8, 0x96,
    0xbc, 0x26, 0xeb, 0x32, 0x46, 0x69, 0x35, 0x05, 0x8c, 0x15, 0x70, 0xd9, 0x1b, 0xe9, 0xbe, 0xcc,
    0xa9, 0x38, 0xa6, 0x67, 0xf0, 0xad, 0x50, 0x13, 0x19, 0x72, 0x64, 0xbf, 0x52, 0xc2, 0x34, 0xe2,
    0x1b, 0x11, 0x79, 0x74, 0x72, 0xbd, 0x34, 0x5b, 0xb1, 0xe2, 0xfd, 0x66, 0x73, 0xfe, 0x71, 0x64,
    0x74, 0xd0, 0x4e, 0xbc, 0x51, 0x24, 0x19, 0x40, 0x87, 0x0e, 0x92, 0x40, 0xe6, 0x21, 0xe7, 0x2d,
    0x4e, 0x37, 0x76, 0x2f, 0x2e, 0xe2, 0x68, 0xc7, 0x89, 0xe8, 0x32, 0x13, 0x42, 0x06, 0x84, 0x84,
    0x53, 0x4a, 0xb3, 0x0c, 0x1b, 0x4c, 0x8d, 0x1c, 0x51, 0x97, 0x19, 0xab, 0xae, 0x77, 0xff, 0xdb,
    0xec, 0xf0, 0x10, 0x95, 0x34, 0x33, 0x6b, 0xcb, 0x3e, 0x84, 0x0f, 0xb9, 0xd8, 0x5f, 0xb8, 0xa0,
    0xb8, 0x55, 0x53, 0x3e, 0x70, 0xf7, 0x18, 0xf5, 0xce, 0x7b, 0x4e, 0xbf, 0x27, 0xce, 0xce, 0xa8,
    0xb3, 0xbe, 0x40, 0xc5, 0xc5, 0x32, 0x29, 0x3e, 0x71, 0x64, 0x9e, 0xde, 0x8c, 0xf6, 0x75, 0xa1,
    0xe6, 0xf6, 0x53, 0xc8, 0x31, 0xa8, 0x78, 0xde, 0x50, 0x40, 0xf7, 0x62, 0xde, 0x36, 0xb2, 0xba
};
#endif

/**
 * @brief Function to evaluate how security will be managed based on production or testing phase.
 *
 * @param const char **salt. Salt message.
 * @param uint16_t *salt_len. Lenght of salt.
 * @return esp_err_t
 *
 */
esp_err_t example_get_sec2_salt(const char **salt, uint16_t *salt_len);

/**
 * @brief This function follows a similar logic to the previous one: it decides how to handle obtaining 
   the security verifier based on whether it is in development or production mode. 
 *
 * @param const char **verifier. Security verifier.
 * @param const char **verifier. Security verifier.
 * @return
 *
 */
esp_err_t example_get_sec2_verifier(const char **verifier, uint16_t *verifier_len);
#endif

/**
 * @brief Function that handles events related to obtaining an IP and provisioning Wi-Fi credentials, 
   managing both successes and failures, and logging relevant information
 *
 * @param void *arg. Additional parameter used for the function.
 * @param esp_event_base_t event_base. Base of the event.
 * @param int32_t event_id. ID of the event.
 * @param void *event_data. Data of event.
 * @return
 *
 */
void event_handler_got_ip(void *arg, esp_event_base_t event_base, 
                        int32_t event_id, void *event_data);

/**
 * @brief This function handles events related to the provisioning of Wi-Fi credentials, 
   managing both successes and failures, and logging relevant information.
 *
 * @param void* arg. Additional argument used by the function.
 * @param esp_event_base_t event_base. Based of the event.
 * @param int32_t event_id. ID of the event. 
 * @param void* event_dat. Data of event.
 * @return
 *
 */
void provision_event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data);

/**
 * @brief This function configures and starts the Wi-Fi in station mode, 
   allowing the device to connect to available Wi-Fi networks.
 *
 * @param 
 * @return
 *
 */
void wifi_init_sta(void);

/**
 * @brief These functions configure the device's service name based on its MAC address and handle data received during a provisioning session, 
   logging success or failure and responding appropriately.
 *
 * @param char *service_name. char *service_name: Pointer to the string where the service name will be stored.
 * @param size_t max. Maximum size of the service 
 * @return
 *
 */
void get_device_service_name(char *service_name, size_t max);

/**
 * @brief "This function handles data received during a provisioning session, converting the input buffer into an integer, 
   preparing a 'SUCCESS' response, and handling memory errors if necessary.
 *
 * @param uint32_t session_id. Session ID. 
 * @param const uint8_t *inbuf. Pointer to the input.
 * @param ssize_t inlen. Length of the input buffer.
 * @param uint8_t **outbuf. Double pointer to the output.
 * @param ssize_t *outlen. Pointer to the length of the output.
 * @param void *priv_data. Private data (not used in this function).
 * @return ESP_OK. Success.
 * @return ESP_ERR_NO_MEM. if outbuf is NULL.
 * @return ESP_FAIL. If inbuf is NULL. 
 *
 */
esp_err_t data_to_receive_prov_data_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                                          uint8_t **outbuf, ssize_t *outlen, void *priv_data);

/**
 * @brief This function handles data received during a provisioning session for ThingsBoard, assigning different configurations based on the value of data_to_receive, 
   preparing a 'SUCCESS' response, and handling memory errors if necessary.
 *
 * @param uint32_t session_id. Session ID.
 * @param const uint8_t *inbuf. Pointer to the input buffer.
 * @param ssize_t inlen. Pointer to the input buffer
 * @param  uint8_t **outbuf. Double pointer to the output. 
 * @param ssize_t *outlen. Pointer to the length of the output .
 * @param void *priv_data. Private data.
 * @return ESP_OK.
 * @return ESP_ERR_NO_MEM. For outbuf is NULL.
 * @return ESP_FAIL. When other data_to_receive values are got. 
 *
 */
esp_err_t thingsboard_cnf_prov_data_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                                          uint8_t **outbuf, ssize_t *outlen, void *priv_data);

/**
 * @brief Function to get current thingsboard configuration details.
 *
 * @param 
 * @return thingsboard_cfg_t structure including thingsboard configuration info.
 *
 */
thingsboard_cfg_t get_thingsboard_cfg();

/**
 * @brief provide already provisioned Wi-Fi credentials.
 *
 * @param 
 * @return wifi_credentials_t structure including Wi-Fi credentials.
 *
 */
wifi_credentials_t get_wifi_credentials();

/**
 * @brief this function initializes the Wi-Fi provisioning process in SoftAP mode for ThingsBoard, 
   configuring and handling events as necessary.
 *
 * @param thingsboard_cfg_t *thingsboard_cfg: Pointer to the ThingsBoard configuration.
 * @param wifi_credentials_t *wifi_credentials: Pointer to the Wi-Fi credentials.
 * @return esp_err_t ESP_OK
 * @return esp_err_t failure for any error. 
 *
 */
esp_err_t softAP_provision_init(thingsboard_cfg_t *thingsboard_cfg, wifi_credentials_t *wifi_credentials);

#endif /* SOFTAP_PROVISION_H*/

