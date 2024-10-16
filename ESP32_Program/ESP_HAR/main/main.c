#include <stdio.h>
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "./include/tasks/wifi_task.h"
#include "./include/tasks/HAR_task.h"

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init(); 
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init(); 
    }
    ESP_ERROR_CHECK(ret);

    //Start Wi-Fi task
    wifi_task_start();

    start_har_task();

}