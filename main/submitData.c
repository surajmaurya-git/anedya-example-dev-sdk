#include "submitData.h"
#include "sync.h"
#include "anedya.h"
#include "esp_log.h"

static const char *TAG = "SubmitData";
static TaskHandle_t current_task;

// Float Data variables
char *variable_identifier = "temp";
float variable_value = 5.20;

// Geo Data variables
char *geo_variable_identifier = "location";
double latitude = 0.0;
double longitude = 0.0;

uint64_t timestamp_ms = 0;

static bool submitFloatData(char *variable_identifier, float variable_value, uint64_t timestamp_ms);
static bool submitGeoData(char *variable_identifier, double latitude, double longitude, uint64_t timestamp_ms);

void submitData_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Starting Data Submission Task");
    current_task = xTaskGetCurrentTaskHandle();

    // Wait for device to connect to WiFi
    xEventGroupWaitBits(ConnectionEvents, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    printf("Waiting for client connection\n");
    while (!anedya_client.is_connected)
    {
        ESP_LOGI("OTA", "Waiting for client connection");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    for (;;)
    {
        while (!anedya_client.is_connected)
        {
            ESP_LOGI("OTA", "Waiting for client connection");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        xEventGroupWaitBits(OtaEvents, OTA_NOT_IN_PROGRESS_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
        submitFloatData(variable_identifier, variable_value, timestamp_ms);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

static bool submitFloatData(char *variable_identifier, float variable_value, uint64_t timestamp_ms)
{
    // Submit Float Data to Anedya
    uint32_t ulNotifiedValue;
    anedya_txn_t sd_txn;
    anedya_txn_register_callback(&sd_txn, TXN_COMPLETE, &current_task);

    // Submit Data
    anedya_err_t aerr = anedya_op_submit_float_req(&anedya_client, &sd_txn, variable_identifier, variable_value, timestamp_ms);
    if (aerr != ANEDYA_OK)
    {
        ESP_LOGI("CLIENT", "%s", anedya_err_to_name(aerr));
    }
    xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, 30000 / portTICK_PERIOD_MS);
    if (ulNotifiedValue == 0x01)
    {
        if (sd_txn.is_success && sd_txn.is_complete)
        {
            printf("%s: Data Pushed to Anedya\n", TAG);
            return true;
        }
        else
        {
            ESP_LOGE(TAG, "Failed to pushed data to Anedya, Check Variable Identifier");
            return false;
        }
    }
    else
    {
        ESP_LOGI("CLIENT", "TXN Timeout");
        return false;
    }
}

static bool submitGeoData(char *variable_identifier, double latitude, double longitude, uint64_t timestamp_ms)
{
    return true;
}