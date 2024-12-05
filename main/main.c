#include <stdio.h>

#include "anedya.h"
#include "anedya_op_commands.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "driver/gpio.h"

#include "wifi.h"
#include "sync.h"
#include "uuid.h"

#include "timeManagement.h"
#include "otaManagement.h"
#include "submitData.h"

sync_data_t gatewaystate;
anedya_config_t anedya_client_config;
anedya_client_t anedya_client;

static const char *TAG = "MAIN";

static void MQTT_ON_Connect(anedya_context_t ctx)
{
    ESP_LOGI("CLIENT", "On connect handler");
    EventGroupHandle_t *handle = (EventGroupHandle_t *)ctx;
    xEventGroupSetBits(*handle, BIT3);
    xEventGroupSetBits(ConnectionEvents, MQTT_CONNECTED_BIT);
}

static void MQTT_ON_Disconnect(anedya_context_t ctx)
{
    ESP_LOGI("CLIENT", "On disconnect handler");
    xEventGroupClearBits(ConnectionEvents, MQTT_CONNECTED_BIT);
}

void cl_event_handler(anedya_client_t *client, anedya_event_t event, void *event_data)
{
    switch (event)
    {
        //=============================================== Commands Handler ======================================================
        // For more info visit: https://docs.anedya.io/commands/intro/
    case ANEDYA_EVENT_COMMAND:
        anedya_command_obj_t *command_obj = (anedya_command_obj_t *)event_data;
        printf(" Received Commands: %s\n", command_obj->command);
        anedya_req_cmd_status_update_t command_status_update = {
            .status = ANEDYA_CMD_STATUS_RECEIVED,
            .data_type = command_obj->cmd_data_type,
            .data_len = command_obj->data_len,
            .data = (unsigned char *)command_obj->data,
        };

        // command_status_update.cmdId = (uint8_t)command_obj->cmdId;
        // anedya_op_cmd_status_update(&anedya_client, NULL, &command_status_update);

        if (command_obj->cmd_data_type == ANEDYA_DATATYPE_STRING)
        {
            if (strcmp(command_obj->command, "led") == 0)
            {
                if (strcmp(command_obj->data, "on") == 0)
                {
                    gpio_set_level(GPIO_NUM_2, true);
                    printf("Led turned on\n");
                    command_status_update.status = ANEDYA_CMD_STATUS_SUCCESS;
                    anedya_op_cmd_status_update(&anedya_client, NULL, &command_status_update);
                }
                else if (strcmp(command_obj->data, "off") == 0)
                {
                    gpio_set_level(GPIO_NUM_2, false);
                    printf("Led turned off\n");
                    command_status_update.status = ANEDYA_CMD_STATUS_SUCCESS;
                    anedya_op_cmd_status_update(&anedya_client, NULL, &command_status_update);
                }
                else
                {
                    command_status_update.status = ANEDYA_CMD_STATUS_FAILED;
                    anedya_op_cmd_status_update(&anedya_client, NULL, &command_status_update);
                    ESP_LOGE(TAG, "Invalid Command");
                }
            }
        }
        else if (command_obj->cmd_data_type == ANEDYA_DATATYPE_BINARY)
        {
            printf("Data: ");
            for (int i = 0; i < command_obj->data_len; i++)
            {
                printf("%c", command_obj->data[i]);
            }
            printf("\n");
        }
        break;
        // =======================================================================================================================
        // ============================================== Valuestore Handler =====================================================
        //                                      It Will be called when valuestore is updated
        // For more info visit: https://docs.anedya.io/valuestore/

    case ANEDYA_EVENT_VS_UPDATE_FLOAT:
        printf(" Received Events \n");
        ESP_LOGI("CLIENT", "Valuestore update notified: float");
        anedya_valustore_obj_float_t *data = (anedya_valustore_obj_float_t *)event_data;
        ESP_LOGI("CLIENT", "Key Updated: %s Value:%f", data->key, data->value);
        break;
    case ANEDYA_EVENT_VS_UPDATE_BOOL:
        printf(" Received Events \n");
        ESP_LOGI("CLIENT", "Valuestore update notified: bool");
        break;
        // ======================================================================================================================
    }
}

void app_main(void)
{
    char connkey[64] = CONFIG_CONNECTION_KEY; // Connection Key to connect to anedya
    uuid_t devid;
    uuid_parse(CONFIG_PHYSICAL_DEVICE_ID, devid); // UUID of the device

    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
    EventGroupHandle_t event_group = xEventGroupCreate();
    ConnectionEvents = xEventGroupCreate();
    OtaEvents = xEventGroupCreate();
    gatewaystate.DeviceTimeEvents = xEventGroupCreate();

    uint32_t ulNotifiedValue;
    // Set GPIO 2 as output
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);

    xEventGroupSetBits(ConnectionEvents, WIFI_FAIL_BIT);
    xEventGroupSetBits(OtaEvents, OTA_NOT_IN_PROGRESS_BIT);

    // Start WiFi Task
    xTaskCreate(wifi_task, "WIFI", 4096, NULL, 1, NULL);
    xEventGroupWaitBits(ConnectionEvents, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    xTaskCreate(&syncTime_task, "syncTime", 4096, &gatewaystate, 1, NULL);
    xEventGroupWaitBits(gatewaystate.DeviceTimeEvents, SYNCED_DEVICE_TIME_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    // ============================================= Anedya Config =============================================================
    anedya_config_init(&anedya_client_config, devid, connkey, strlen(connkey));          // Initialize config
    anedya_config_set_connect_cb(&anedya_client_config, MQTT_ON_Connect, &event_group);  // Set connect callback
    anedya_config_set_disconnect_cb(&anedya_client_config, MQTT_ON_Disconnect, NULL);    // Set disconnect callback
    anedya_config_register_event_handler(&anedya_client_config, cl_event_handler, NULL); // Set event handler
    anedya_config_set_region(&anedya_client_config, ANEDYA_REGION_AP_IN_1);              // Set region
    anedya_config_set_timeout(&anedya_client_config, 30000);                             // Set timeout

    // Initialize client
    anedya_client_init(&anedya_client_config, &anedya_client);
    anedya_err_t aerr = anedya_client_connect(&anedya_client);
    if (aerr != ANEDYA_OK)
    {
        ESP_LOGI("CLIENT", "%s", anedya_err_to_name(aerr));
    }
    ESP_LOGI("CLIENT", "Waiting for MQTT Connection");
    xEventGroupWaitBits(event_group, BIT3, pdFALSE, pdFALSE, 30000 / portTICK_PERIOD_MS);

    anedya_txn_t lg_txn;
    anedya_txn_register_callback(&lg_txn, TXN_COMPLETE, &current_task);
    unsigned long timestamp_ms = (unsigned long)time(NULL) * 1000;
    char message[13] = "DeviceBooted";
    // =================================================== Submit Log to Anedya ==================================================
    anedya_err_t err = anedya_op_submit_log(&anedya_client, &lg_txn, message, 13, timestamp_ms);
    // ===========================================================================================================================
    if (err != ANEDYA_OK)
    {
        ESP_LOGI("CLIENT", "%s", anedya_err_to_name(err));
    }
    xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, 30000 / portTICK_PERIOD_MS);
    if (ulNotifiedValue == 0x01)
    {
        if (lg_txn.is_success && lg_txn.is_complete)
        {
            printf("--------------------------------\n");
            printf("%s: '%s' Log sent\n", TAG, message);
            printf("--------------------------------\n");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to send Log to Anedya");
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to send Log to Anedya");
    }

    // =============================================== Handle OTA ================================================================
    // Manage Firmware Updates through Aneyda, For more info visit: https://docs.anedya.io/ota

    xTaskCreate(ota_management_task, "OTA", 40960, &gatewaystate, 1, NULL);

    // =============================================== Submit data to Anedya =====================================================
    // For more info visit: https://docs.anedya.io/getting-started/quickstart/

    xTaskCreate(submitData_task, "SUBMITDATA", 40960, NULL, 1, NULL);

    // ======================================================= Set and Get Key Value  ============================================
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
    // ===========================================================================================================================

    for (;;)
    {
        xEventGroupWaitBits(ConnectionEvents, MQTT_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
        xEventGroupWaitBits(OtaEvents, OTA_NOT_IN_PROGRESS_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

        anedya_txn_t hb_txn;
        anedya_txn_register_callback(&hb_txn, TXN_COMPLETE, &current_task);
        // ================================================ Send Heartbeat to Anedya ================================================
        // For more info visit: https://docs.anedya.io/getting-started/quickstart/

        anedya_err_t aerr = anedya_device_send_heartbeat(&anedya_client, &hb_txn);
        // ==========================================================================================================================
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
