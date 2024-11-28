#include "submitData.h"
#include "sync.h"
#include "anedya.h"
#include "esp_log.h"

static const char *TAG = "SubmitData";
static TaskHandle_t current_task;

void submitData_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Starting Data Submission Task");
    current_task = xTaskGetCurrentTaskHandle();
    uint32_t ulNotifiedValue;
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

        // Submit Float Data to Anedya
        anedya_txn_t hb_txn;
        anedya_txn_register_callback(&hb_txn, TXN_COMPLETE, &current_task);
        anedya_err_t aerr = anedya_op_submit_float_req(&anedya_client, &hb_txn, "temp", 1.0,0);
        if (aerr != ANEDYA_OK)
        {
            ESP_LOGI("CLIENT", "%s", anedya_err_to_name(aerr));
        }
        xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, 30000 / portTICK_PERIOD_MS);
        if (ulNotifiedValue == 0x01)
        {
            // ESP_LOGI("CLIENT", "TXN Complete");
            ESP_LOGI("CLIENT", " Data Pushed to Anedya");
        }
        else
        {
            // ESP_LOGI("CLIENT", "TXN Timeout");
            ESP_LOGI("CLIENT", "Failed to pushed data to Anedya");
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}