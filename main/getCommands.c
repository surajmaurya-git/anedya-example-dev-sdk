/*
            Example : Control Esp inbuilt led through Anedya command feature
*/

#include "getCommands.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "anedya.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sync.h"

static const char *TAG = "Get_Commands";

void getCommands_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Starting Get Commands Task");
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    command_handler.is_command_processed =1;
    for (;;)
    {
        xEventGroupWaitBits(ConnectionEvents, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
        xEventGroupWaitBits(OtaEvents, OTA_NOT_IN_PROGRESS_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
        xEventGroupWaitBits(ConnectionEvents, MQTT_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

        
        if (command_handler.is_command_processed == 0)
        {
            command_handler.is_command_processed = 1;
            ESP_LOGI(TAG, "Processing command: %s", command_handler.command_obj->command);
            printf("command type : %d\n",command_handler.command_obj->cmd_data_type);
            // if(command_handler.command_obj->cmd_data_type==ANEDYA_DATATYPE_BINARY){
            if(1){
                if(strcmp(command_handler.command_obj->data,"on")==0){
                    gpio_set_level(GPIO_NUM_2, true);
                    printf("Led turned on\n");
                }else if(strcmp(command_handler.command_obj->data,"off")==0){
                    gpio_set_level(GPIO_NUM_2, false);
                    printf("Led turned off\n");
                }else{
                    ESP_LOGE(TAG, "Invalid Command");
                }

            }else if(command_handler.command_obj->cmd_data_type==ANEDYA_DATATYPE_BINARY){
                ESP_LOGI(TAG, "Received Binary Data");
            }
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}
