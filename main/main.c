/*
2025. Complutense University of Madrid.
Final Project. Subjects: IoT Node Architecture, Networks, Protocols and Interfaces I, and Networks, Protocols and Interfaces II.
Development of a system for measuring air quality (CO2 levels) in the classrooms of the Faculty of Computer Science at Complutense University of Madrid.

Group 5. Members: Pablo Alcalde, Diego Alejandro de Celis, Diego Pellicer, Jaime Garzón.
*/

#include "cJSON.h"
#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "softAP_provision.h"
#include "softap_provision_types.h"
#include "mqtt_controller.h"
#include "nvs_structures.h"
#include "sgp30.h"
#include "sgp30_types.h"
#include "thingsboard_types.h"
#include <esp_wifi.h>
#include <string.h>
#include "mbedtls/x509_crt.h"
#include "power_manager.h"
#include "wifi_power_manager.h"
#include "sntp_sync.h"

#include "esp_log.h"

#define DEFAULT_MEASURING_TIME 10
#define DEVICE_SDA_IO_NUM 21
#define DEVICE_SCL_IO_NUM 22
#define PROVISIONING_SOFTAP

static char *TAG = "MAIN";

/* sgp30 required structures. Global variables*/

i2c_master_bus_handle_t i2c_master_bus_handle;
esp_event_loop_handle_t imc_event_loop_handle;
SemaphoreHandle_t sgp30_req_measurement;
sgp30_measurement_log_t sgp30_log;
uint16_t send_time = 30;
thingsboard_cfg_t thingsboard_cfg;
wifi_credentials_t wifi_credentials;



char *prepare_meassure_send(long ts, sgp30_measurement_t measurement);

/**
 * @brief This function handles Wi-Fi connection events, attempting to reconnect automatically with an exponential backoff if a disconnection occurs. 
   It logs important events and resets the retry counter when the connection is successful.
 *
 * @param void *arg. Additional argument passed to the function.
 * @param esp_event_base_t event_base. Event base.
 * @param int32_t event_id. Event identifier.
 * @param void *event_data. Event data.
 * @return
 *
 */
static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
)
{
    static int retry_count = 0;

    switch (event_id)
    {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "Connecting to WiFi network...");
            esp_wifi_connect(); /*Trying to connect*/
            break;
        case WIFI_EVENT_STA_CONNECTED:
            retry_count = 0; /* We reset the retry counter*/
            ESP_LOGI(TAG, "Successfully connected to WiFi connection.");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "Disconnected from WiFi...trying again");
            if (retry_count < 5)
            {
                int delay = (1 << retry_count)
                            * 1000; /* Backoff exponential (1s, 2s, 4s...)*/
                ESP_LOGI(TAG, "Retrying in %d ms...", delay);
                vTaskDelay(pdMS_TO_TICKS(delay)
                ); // Wait before trying again
                esp_wifi_connect();
                retry_count++;
            } else
            {
                ESP_LOGE(
                    TAG,
                    "Could not connect to WiFi after several attempts"
                );
                /*Additional actions if all attempts fail*/
            }
            break;
        default:
            break;
    }
}

/**
 * @brief This function handles new measurements from the SGP30 sensor, creates a log entry with the measurement and the time, converts the measurement to a JSON representation, and publishes it via MQTT. 
 *  Additionally, the code contains comments for enqueueing the measurement and storing the log entry.
 *
 * @param void *handler_args. Additional arguments passed to the function.
 * @param esp_event_base_t base. Event base.
 * @param int32_t event_id. Event identifier.
 * @param void *event_data. Event data.
 * @return
 *
 */
static void sgp30_on_new_measurement(
    void *handler_args,
    esp_event_base_t base,
    int32_t event_id,
    void *event_data
)
{
    sgp30_timed_measurement_t new_log_entry;
    time(&new_log_entry.time);

    new_log_entry.measurement = *((sgp30_measurement_t *)event_data);
    ESP_LOGI(
        TAG,
        "To send:\n\tMeasured eCO2= %d TVOC= %d",
        new_log_entry.measurement.eCO2,
        new_log_entry.measurement.TVOC
    );
    
    /* sgp30_measurement_enqueue(&new_log_entry, &sgp30_log);*/
    char* measurement_JSON_repr = prepare_meassure_send(new_log_entry.time, new_log_entry.measurement);
    mqtt_publish(measurement_JSON_repr, strlen(measurement_JSON_repr));
    /*Send or store log_entry*/
}

/**
 * @brief This function handles events that change the data transmission interval, logging the change, 
 *  assigning the new interval, and restarting the SGP30 sensor measurement with the new interval.
 *
 * @param void *handler_args. Additional arguments passed to the function.
 * @param esp_event_base_t base. Event base.
 * @param int32_t event_id. Event identifier.
 * @param void *event_data. Event data.
 * @return
 *
 */
static void mqtt_on_new_interval(
    void *handler_args,
    esp_event_base_t base,
    int32_t event_id,
    void *event_data
)
{
    send_time = *((int *)event_data);
    ESP_LOGI(TAG, "Changing transmission interval %d seconds", send_time);
    sgp30_restart_measuring( send_time);
}

/**
 * @brief This function handles SNTP time synchronization events, 
   logging the event, obtaining the current time, and setting the time in the power manager.
 *
 * @param void *handler_args. Additional arguments passed to the function.
 * @param esp_event_base_t base. Event base.
 * @param int32_t event_id. Event identifier.
 * @param void *event_data. Event data.
 * @return
 *
 */
static void sntp_on_sync_time(
    void *handler_args,
    esp_event_base_t base,
    int32_t event_id,
    void *event_data
)
{
    ESP_LOGI(TAG, "Time synchronized, changing deep sleep");
    struct tm timeinfo;
    time_t time_now;

    time(&time_now);
    localtime_r(&time_now, &timeinfo);
    power_manager_set_sntp_time(&timeinfo);
}

/**
 * @brief This function handles new baseline events from the SGP30 sensor, creating a new baseline entry, 
 *  storing it, and logging the eCO2 and TVOC values along with the timestamp.
 *
 * @param void *handler_args. Additional arguments passed to the function.
 * @param esp_event_base_t base. Event base.
 * @param int32_t event_id. Event identifier.
 * @param void *event_data. Event data.
 * @return
 *
 */
static void sgp30_on_new_baseline(
    void *handler_args,
    esp_event_base_t base,
    int32_t event_id,
    void *event_data
)
{
    sgp30_timed_measurement_t new_baseline;
    new_baseline.measurement = *((sgp30_measurement_t *)event_data),
    time(&new_baseline.time);
    ESP_ERROR_CHECK(storage_set((const sgp30_timed_measurement_t*) &new_baseline));
    ESP_LOGI(
        TAG,
        "Baseline eCO2= %d TVOC= %d at timestamp %s",
        new_baseline.measurement.eCO2,
        new_baseline.measurement.TVOC,
        ctime(&new_baseline.time)
    );
}

#ifndef DEBUGGING_NVS
static const sgp30_event_handler_register_t sgp30_registered_events[] = {
    { SGP30_EVENT_NEW_MEASUREMENT, sgp30_on_new_measurement },
    { SGP30_EVENT_NEW_MEASUREMENT,    sgp30_on_new_baseline }
};

/**
 * @brief This function initializes the I2C master bus with a specific configuration, including the I2C port, SCL and SDA pins, and the use of internal pull-up resistors. If the initialization fails, an error is logged, 
 * and the corresponding error code is returned. In case of success, ESP_OK is returned.
 *
 * @param i2c_master_bus_handle_t *bus_handle. Pointer to the I2C master bus handle.
 * @return esp_err_t ESP_OK.
 * @return esp_err_t ERROR.
 *
 */

static const size_t sgp30_registered_events_len =
    sizeof(sgp30_registered_events) / sizeof(sgp30_registered_events[0]);
#endif

static esp_err_t init_i2c(i2c_master_bus_handle_t *bus_handle)
{
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = DEVICE_SCL_IO_NUM,
        .sda_io_num = DEVICE_SDA_IO_NUM,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_RETURN_ON_ERROR(
        i2c_new_master_bus(&i2c_bus_config, bus_handle),
        TAG,
        "Could not initialize new master bus"
    );

    return ESP_OK;
}

/**
 * @brief This function takes a measurement from the SGP30 sensor and its timestamp, 
 * creates a JSON representation of the data, and returns this JSON representation as a string.
 *
 * @param long ts. Timestamp of the measurement.
 * @param sgp30_measurement_t measurement. Structure containing the eCO2 and TVOC measurements.
 * @return char*, data_to_send by JSON.
 */
char *prepare_meassure_send(long ts, sgp30_measurement_t measurement)
{
    cJSON *json_data = cJSON_CreateObject();
    cJSON *measurement_json = cJSON_CreateObject();
    char *data_to_send;
    cJSON_AddNumberToObject(json_data, "ts", ts);
    cJSON_AddNumberToObject(measurement_json, "eCO2", measurement.eCO2);
    cJSON_AddNumberToObject(measurement_json, "TVOC", measurement.TVOC);
    cJSON_AddItemToObjectCS(json_data, "values", measurement_json);
    data_to_send = cJSON_PrintUnformatted(json_data);

    cJSON_Delete(json_data);
    return data_to_send;
}

void app_main(void)
{

    #ifdef DEBUGGING_NVS
    ESP_LOGI(TAG, "Starting NVS debugging");
    ESP_ERROR_CHECK(storage_init());
    storage_get(&thingsboard_cfg);
    mbedtls_x509_crt ca_cert;
    mbedtls_x509_crt_init(&ca_cert);
    ESP_LOGI(TAG, "CA VERIFICATION OUTPUT %d", mbedtls_x509_crt_parse(&ca_cert, (const unsigned char *) thingsboard_cfg.verification.certificate, strlen(thingsboard_cfg.verification.certificate) + 1));
    ESP_LOGI(TAG, "CHAIN VERIFICATION OUTPUT %d", mbedtls_x509_crt_parse(&ca_cert, (const unsigned char *) thingsboard_cfg.credentials.authentication.certificate, strlen(thingsboard_cfg.credentials.authentication.certificate) + 1));
    storage_get(&wifi_credentials);
    ESP_LOGI(TAG, "Wifi SSID: \n\t%s\n Wifi Password: \n\t%s", wifi_credentials.ssid, wifi_credentials.password);
    #else

    esp_event_loop_args_t imc_event_loop_args = {
        /* An event loop for sensoring related events*/
        .queue_size = 5,
        .task_name =
            "sgp30_event_loop_task", /* since it is a task it can be stopped */
        .task_stack_size = 4096,
        .task_priority = uxTaskPriorityGet(NULL),
        .task_core_id = tskNO_AFFINITY,
    };
    esp_event_loop_create(&imc_event_loop_args, &imc_event_loop_handle);

    /* Initialize the event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    //esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler_got_ip, NULL);

    ESP_ERROR_CHECK(storage_init());

    ESP_ERROR_CHECK(init_i2c(&i2c_master_bus_handle));
    ESP_ERROR_CHECK(
        sgp30_device_create(i2c_master_bus_handle, SGP30_I2C_ADDR, 400000)
    );

    /* Set up event listeners for SGP30 module.*/
    for (int i = 0; i < sgp30_registered_events_len; i++)
    {
        ESP_ERROR_CHECK(
            esp_event_handler_register_with(
                imc_event_loop_handle,
                SGP30_EVENT,
                sgp30_registered_events[i].event_id,
                sgp30_registered_events[i].event_handler,
                NULL
            )
        );
    }

    /* Set up event listeenr for MQTT module*/
    ESP_ERROR_CHECK(
        esp_event_handler_register_with(
            imc_event_loop_handle,
            MQTT_THINGSBOARD_EVENT,
            MQTT_NEW_SEND_TIME,
            mqtt_on_new_interval,
            NULL
        )
    );

        /* Set up event listeenr for MQTT module*/
    ESP_ERROR_CHECK(
        esp_event_handler_register_with(
            imc_event_loop_handle,
            SNTP_SYNC_EVENT,
            SNTP_SUCCESSFULL_SYNC,
            sntp_on_sync_time,
            NULL
        )
    );

    power_manager_init();
    wifi_power_save_init();

    esp_err_t got_thingboard_cfg = storage_get(&thingsboard_cfg);
    esp_err_t got_wifi_credentials = storage_get(&wifi_credentials);
    if( got_thingboard_cfg != got_wifi_credentials)
    {
        ESP_LOGE(TAG, "Provisioning State Corrupt");
        ESP_ERROR_CHECK(storage_erase());
        esp_restart();
    }
    else if(got_thingboard_cfg != ESP_OK)
    {
        #ifdef PROVISIONING_SOFTAP
        ESP_LOGI(TAG, "Device not provisioned");
        /*Start the init of the provision component, we actively wait it to finish the provision to continue*/
        ESP_ERROR_CHECK(softAP_provision_init(NULL, NULL));
        thingsboard_cfg = get_thingsboard_cfg();
        wifi_credentials = get_wifi_credentials();
        storage_set((const thingsboard_cfg_t*) &thingsboard_cfg);
        storage_set((const wifi_credentials_t*) &wifi_credentials);
        ESP_LOGI(TAG, "%s", thingsboard_cfg.address.uri);
        ESP_LOGI(TAG, "%s", thingsboard_cfg.verification.certificate);
        #else
        ESP_LOGE(TAG, "Device not provisioned");
        #endif
    }
    else
    {
        ESP_LOGI(TAG, "Device provisioned");
        ESP_ERROR_CHECK(softAP_provision_init(&thingsboard_cfg, &wifi_credentials));
    }

    init_sntp(imc_event_loop_handle);

    /* At this point a valid time is required*/
    /* We start the sensor*/
    sgp30_timed_measurement_t maybe_baseline;
    time_t time_now;

    time(&time_now);
    if (ESP_OK == storage_get(&maybe_baseline)
        && !sgp30_is_baseline_expired(maybe_baseline.time, time_now))
    {
        sgp30_init(imc_event_loop_handle, &maybe_baseline.measurement);
    } else
    {
        sgp30_init(imc_event_loop_handle, NULL);
    }
    /* Esto debería de iniciarse al tener un valor del intervalo, por MQTT*/
    /* (atributo compartido creo) Se inicia solo al mandar un evento*/
    /* SGP30_EVENT_NEW_INTERVAL*/
    
    //Tras haber sincronizado la hora con sntp ajustamos la hora de entrada en deep sleep
    mqtt_init(imc_event_loop_handle, &thingsboard_cfg);
    wifi_set_power_mode(WIFI_POWER_MODE_MAX_MODEM);
    sgp30_start_measuring(send_time);
    #endif
}
