
#include "valueStore.h"
#include "sync.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "driver/gpio.h"

const char *TAG = "VALUE_STORE";
static TaskHandle_t current_task;

void valueStore_task(void *pvParameters)
{
    current_task = xTaskGetCurrentTaskHandle();
    uint32_t ulNotifiedValue;
    while (1)
    {
        xEventGroupWaitBits(ConnectionEvents, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
        xEventGroupWaitBits(ConnectionEvents, MQTT_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
        xEventGroupWaitBits(OtaEvents, OTA_NOT_IN_PROGRESS_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

        // ======================= Set float Value ================================
        // For more info visit: https://docs.anedya.io/valuestore
        anedya_txn_t vs_txn;

        const char *key = "testKey";
        float value = 1.00;
        anedya_txn_register_callback(&vs_txn, TXN_COMPLETE, &current_task);
        anedya_err_t v_err = anedya_op_valuestore_set_float(&anedya_client, &vs_txn, key, value);
        if (v_err != ANEDYA_OK)
        {
            ESP_LOGI("CLIENT", "%s", anedya_err_to_name(v_err));
        }
        xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, 30000 / portTICK_PERIOD_MS);
        if (ulNotifiedValue == 0x01)
        {
            if (vs_txn.is_success && vs_txn.is_complete)
            {
                printf("--------------------------------\n");
                printf("%s: Key:'%s', Value: '%f'  Key Value Set\n", TAG, key, value);
                printf("--------------------------------\n");
            }
        }
        else
        {
            // ESP_LOGI("CLIENT", "TXN Timeout");
            ESP_LOGE(TAG, "Failed to set Key Value to Anedya");
        }


        //============================= Set bool Value ================================
        const char *boolKey = "testKey2";
        bool boolValue = true;

        v_err = anedya_op_valuestore_set_bool(&anedya_client, &vs_txn, boolKey, boolValue);

        if (v_err != ANEDYA_OK)
        {
            ESP_LOGI("CLIENT", "%s", anedya_err_to_name(v_err));
        }
        xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, 30000 / portTICK_PERIOD_MS);
        if (ulNotifiedValue == 0x01)
        {
            if (vs_txn.is_success && vs_txn.is_complete)
            {
                printf("--------------------------------\n");
                printf("%s: Key:'%s', Value: '%d'  Key Value Set\n", TAG, boolKey, boolValue);
                printf("--------------------------------\n");
            }
        }
        else
        {
            // ESP_LOGI("CLIENT", "TXN Timeout");
            ESP_LOGE(TAG, "Failed to set Key Value to Anedya");
        }

        // value set, hold the task
        for (;;)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}
