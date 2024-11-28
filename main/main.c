#include <stdio.h>

#include "anedya.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"

#include "sync.h"
#include "otaManagement.h"
#include "submitData.h"

#include "sync.h"
#include "wifi.h"
#include "uuid.h"
#include "timeManagement.h"

sync_data_t gatewaystate;
anedya_config_t anedya_client_config;
anedya_client_t anedya_client;

static const char *TAG = "MAIN";

static void MQTT_ON_Connect(anedya_context_t ctx)
{
    ESP_LOGI("CLIENT", "On connect handler");
    EventGroupHandle_t *handle = (EventGroupHandle_t *)ctx;
    xEventGroupSetBits(*handle, BIT3);
}

static void MQTT_ON_Disconnect(anedya_context_t ctx)
{
    ESP_LOGI("CLIENT", "On disconnect handler");
}

void cl_event_handler(anedya_client_t *client, anedya_event_t event, void *event_data)
{
    switch (event)
    {
    case ANEDYA_EVENT_VS_UPDATE_FLOAT:
        ESP_LOGI("CLIENT", "Valuestore update notified: float");
        anedya_valustore_obj_float_t *data = (anedya_valustore_obj_float_t *)event_data;
        ESP_LOGI("CLIENT", "Key Updated: %s Value:%f", data->key, data->value);
        break;
    }
}

void app_main(void)
{
    char connkey[64]=CONFIG_CONNECTION_KEY;  // Connection Key to connect to anedya
    uuid_t devid;
    uuid_parse(CONFIG_PHYSICAL_DEVICE_ID, devid); // UUID of the device

    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
    EventGroupHandle_t event_group = xEventGroupCreate();
    ConnectionEvents = xEventGroupCreate();
    OtaEvents = xEventGroupCreate();
    gatewaystate.DeviceTimeEvents = xEventGroupCreate();

    uint32_t ulNotifiedValue;


    xEventGroupSetBits(ConnectionEvents, WIFI_FAIL_BIT);
    xEventGroupSetBits(OtaEvents, OTA_NOT_IN_PROGRESS_BIT);

    // Start WiFi Task
    xTaskCreate(wifi_task, "WIFI", 4096, NULL, 1, NULL);
    xEventGroupWaitBits(ConnectionEvents, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

     xTaskCreate(&syncTime_task, "syncTime", 4096, &gatewaystate, 1, NULL);
    xEventGroupWaitBits(gatewaystate.DeviceTimeEvents, SYNCED_DEVICE_TIME_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    anedya_config_init(&anedya_client_config, devid, connkey, strlen(connkey)); // Initialize config
    anedya_config_set_connect_cb(&anedya_client_config, MQTT_ON_Connect, &event_group); // Set connect callback
    anedya_config_set_disconnect_cb(&anedya_client_config, MQTT_ON_Disconnect, NULL); // Set disconnect callback
    anedya_config_register_event_handler(&anedya_client_config, cl_event_handler, NULL); // Set event handler
    anedya_config_set_region(&anedya_client_config, ANEDYA_REGION_AP_IN_1); // Set region
    anedya_config_set_timeout(&anedya_client_config, 30000); // Set timeout

    // Initialize client
    anedya_client_init(&anedya_client_config, &anedya_client);
    anedya_err_t aerr = anedya_client_connect(&anedya_client);
    if (aerr != ANEDYA_OK)
    {
        ESP_LOGI("CLIENT", "%s", anedya_err_to_name(aerr));
    }
    ESP_LOGI("CLIENT", "Waiting for MQTT Connection");
    xEventGroupWaitBits(event_group, BIT3, pdFALSE, pdFALSE, 30000 / portTICK_PERIOD_MS);

    // Start OTA Task
    xTaskCreate(ota_management_task, "OTA", 40960, &gatewaystate, 1, NULL);

    //Submit Data Task
    xTaskCreate(submitData_task, "SUBMITDATA", 40960, NULL, 1, NULL);

    for (;;)
    {
        xEventGroupWaitBits(OtaEvents, OTA_NOT_IN_PROGRESS_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

        printf("==========================================================\n");
        printf(" Version | 0.8.0\n");
        printf("==========================================================\n");

        // Send heartbeats
        anedya_txn_t hb_txn;
        anedya_txn_register_callback(&hb_txn, TXN_COMPLETE, &current_task);
        anedya_err_t aerr = anedya_device_send_heartbeat(&anedya_client, &hb_txn);
        if (aerr != ANEDYA_OK)
        {
            ESP_LOGI("CLIENT", "%s", anedya_err_to_name(aerr));
        }
        xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, 30000 / portTICK_PERIOD_MS);
        if (ulNotifiedValue == 0x01)
        {
            // ESP_LOGI("CLIENT", "TXN Complete");
            printf("%s: Heartbeat sent\n", TAG);
        }
        else
        {
            // ESP_LOGI("CLIENT", "TXN Timeout");
            ESP_LOGE(TAG, "Failed to sent heartbeat");
        }

        vTaskDelay(30000 / portTICK_PERIOD_MS);
    }
}

