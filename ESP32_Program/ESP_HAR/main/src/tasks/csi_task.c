#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "../../include/tasks/csi_task.h"
#include "../../include/tasks/processor_task.h"

static const char * TAG = "CSI TASK";

static QueueHandle_t csi_task_queue; 
static int8_t processor_started = 0;

/* Callback that will be called when a new CSI data packet is available
 * @param ctx, context argument 
 * @param info, CSI data received
 */
static void IRAM_ATTR csi_received_callback(void * ctx, wifi_csi_info_t * info){
    uint8_t mac_sta[6] = {0x2c, 0xbc, 0xbb, 0x44, 0xd2, 0xb8};
    csi_data_t *csi_info = NULL;
    //Check for valid argument 
    if(!info || !info->buf){
        return; 
    }
    if(memcmp(info->mac, mac_sta, 6))
        return; 
    
    if (processor_started == 1) {
        ESP_LOGI(TAG, "csi callback, pushing new CSI measurement %u", info->rx_ctrl.timestamp);
        csi_info = (csi_data_t *) malloc(sizeof(csi_data_t));
        if (csi_info != NULL) {
            csi_info->pkt_ctrl = info->rx_ctrl;
            memcpy(csi_info->mac_addr, info->mac, sizeof(uint8_t) * 6);
            csi_info->len = info->len;
            csi_info->buf = (int8_t *) malloc(sizeof(int8_t) * info->len); 
            if (csi_info->buf != NULL) {
                memcpy(csi_info->buf, info->buf, sizeof(int8_t) * info->len);
                processor_task_message_t msg = {
                    .msgID = MSG_PROCESSOR_PUSH_AND_PROCESS,
                    .data = csi_info
                };
                if(processor_task_send_message(msg) != pdTRUE) {
                    ESP_LOGW(TAG, "CSI measurement not pushed into processor queue!!");
                    free(csi_info->buf);
                    free(csi_info); 
                }
            } else {
                ESP_LOGW(TAG, "space insufficient for memory allocation ");
                free(csi_info); 
            }
            
        } else {
            ESP_LOGW(TAG, "space insufficient for memory allocation ");
        }
        
    }
    

}

BaseType_t csi_task_send_message(csi_task_message_t msg) {
    return xQueueSend(csi_task_queue, &msg, portMAX_DELAY); 
}

static wifi_csi_config_t default_csi_config(void) {
    wifi_csi_config_t csi_config = {
        .lltf_en = CSI_TASK_LLTF,
        .htltf_en = CSI_TASK_HT_LTF,
        .stbc_htltf2_en = CSI_TASK_STBC_HT_LTF,
        .ltf_merge_en = CSI_TASK_LTF_MERGE, 
        .channel_filter_en = CSI_TASK_CHANNEL_FILTER, 
        .manu_scale = CSI_TASK_MANU_SCALE, 
        .shift = CSI_TASK_SHIFT
    };
    return csi_config; 
}

static void init_csi() {
    wifi_csi_config_t csi_config = default_csi_config(); 
    //ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter());
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
    //ESP_ERROR_CHECK(esp_wifi_set_csi_config(&csi_config));
    ESP_ERROR_CHECK(esp_wifi_set_csi_rx_cb(csi_received_callback, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_csi(true));
}

static void csi_task(void * config) {
    csi_task_message_t msg = {
        .msgID = MSG_CSI_START_CSI_COLLECTION
    };
    csi_task_send_message(msg);
    while(1) {
        if(xQueueReceive(csi_task_queue, &msg, portMAX_DELAY)) {
            switch(msg.msgID) {
                case MSG_CSI_START_CSI_COLLECTION: 
                    ESP_LOGI(TAG, "received MSG_CSI_START_CSI_COLLECTION");
                    init_csi();
                    ESP_LOGI(TAG, "initialized");
                    //Processor task will  be started
                    processor_task_start();
                    processor_started = 1; 
                break; 
            }
        }
    }
}

void csi_task_start() {
    ESP_LOGI(TAG, "Starting task...");
    csi_task_queue = xQueueCreate(10, sizeof(csi_task_message_t));
    //Create the CSI task
    processor_started = 0; 
    xTaskCreatePinnedToCore(&csi_task, "csi_task", CSI_TASK_STACK_SIZE, NULL, CSI_TASK_PRIORITY, NULL, CSI_TASK_CORE_ID); 
}