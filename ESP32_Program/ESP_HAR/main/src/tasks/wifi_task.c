#include "../../include/tasks/wifi_task.h"

#include <string.h>
#include <netdb.h>
#include <esp_wifi_types.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_mac.h"

#include "../../include/tasks/csi_task.h"

//Tag for identifying Wi-Fi app msgs 
static const char * TAG = "Wi-Fi TASK";

static QueueHandle_t wifi_task_queue; 

//netif object
esp_netif_t * g_esp_netif_ap = NULL; 

/*
 * Wi-Fi app event handler
 * @param args, aside from event data, that is passed to the handler when it is called
 * @param event_base the base id of the event to register the handler for
 * @param event_id the id of the event to handle
 * @param event_data event data
 */
static void wifi_event_handler(void * args, esp_event_base_t event_base, int32_t event_id, void * event_data){
    wifi_event_ap_staconnected_t * sta_conn_event; 
    char ssid[32] = "";
    if(event_base == WIFI_EVENT){
        switch(event_id){
            //Transmitter has been connected to receiver access point
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
                break;

            //Station has been connected to the access point
            case WIFI_EVENT_AP_STACONNECTED:
                sta_conn_event = (wifi_event_ap_staconnected_t *) event_data; 
                ESP_LOGI(TAG, "station connected, AID = %d",  sta_conn_event->aid);
                wifi_task_message_t msg = {
                    .msgID = MSG_WIFI_START_SENSING,
                    .data = NULL
                };
                wifi_task_send_message(msg); 
                break; 

           default:
           break;
        }
    } 
}

BaseType_t wifi_task_send_message(wifi_task_message_t msg){
    return xQueueSend(wifi_task_queue, &msg, portMAX_DELAY);
}

/*
 * Function to configure Wi-Fi as access point after initialization
 */
static esp_netif_t * wifi_config_ap(){
    //Create default network access point
    esp_netif_t * esp_netif_ap = esp_netif_create_default_wifi_ap(); 
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_TASK_CSI_COLLECTION_SSID,
            .password = WIFI_TASK_CSI_COLLECTION_PASSWORD, 
            .ssid_len = strlen(WIFI_TASK_CSI_COLLECTION_SSID),
            .channel = WIFI_TASK_CHANNEL,
            .ssid_hidden = WIFI_TASK_AP_HIDDEN,
            .max_connection = WIFI_TASK_MAX_CONNECTIONS,
            .authmode = WIFI_TASK_AUTH_MODE
        }
    };
    
    //Set static IP 
    ESP_ERROR_CHECK(esp_netif_dhcps_stop(esp_netif_ap)); //Stop DHCP service to configure it
    esp_netif_ip_info_t ap_ip_info; 
    memset(&ap_ip_info, 0, sizeof(esp_netif_ip_info_t));
    ap_ip_info.ip.addr = ipaddr_addr(WIFI_TASK_AP_IP_ADDRESS);
    ap_ip_info.netmask.addr = ipaddr_addr(WIFI_TASK_AP_NETMASK);
    ap_ip_info.gw.addr = ipaddr_addr(WIFI_TASK_AP_IP_ADDRESS);
    if(esp_netif_set_ip_info(esp_netif_ap, &ap_ip_info) != ESP_OK){
        ESP_LOGE(TAG, "Failed to set static IP info");
    }
    ESP_ERROR_CHECK(esp_netif_dhcps_start(esp_netif_ap)); //Start DHCP service again
    //Show access point information
    ESP_LOGI(TAG, "SSID: %s", WIFI_TASK_CSI_COLLECTION_SSID);
    ESP_LOGI(TAG, "IP: %s", WIFI_TASK_AP_IP_ADDRESS);
    ESP_LOGI(TAG, "Gateway: %s", WIFI_TASK_AP_IP_ADDRESS);
    ESP_LOGI(TAG, "Netmask: %s", WIFI_TASK_AP_NETMASK);
    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_AP,mac));
    ESP_LOGI(TAG, "MAC address as AP: "MACSTR"", MAC2STR(mac));
    ESP_ERROR_CHECK(esp_efuse_mac_get_default(mac));
    ESP_LOGI(TAG, "MAC address as STA: "MACSTR"", MAC2STR(mac));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_bandwidth(ESP_IF_WIFI_AP, WIFI_TASK_BANDWIDTH));
    ESP_LOGI(TAG, "Wi-Fi app configuration finished.");

    //Start Wi-Fi service
    ESP_ERROR_CHECK(esp_wifi_start());

    return esp_netif_ap; 
}


/*
 * Function to initialize Wi-Fi access point
 */
static void wifi_init() {
    //Initialize the TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());
    //Create system Event task and initialize an application event's callback function
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, 
        ESP_EVENT_ANY_ID,
        &wifi_event_handler, //Function callback for when a Wi-Fi event occurs
        NULL,
        NULL
    ));

    //Set default configuration
    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT(); 
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));
    ESP_LOGI(TAG, "Wi-Fi init phase finished");
}


/*
 * Wi-Fi task to be running in core
 */
static void wifi_task(void * parameters) {
    wifi_task_message_t msg = {
        .msgID = MSG_WIFI_START_AP_MODE,
        .data = NULL
    }; 
    //Send message to start access point
    wifi_task_send_message(msg); 

    while(1) {
        //Revisar portMAX_DELAY con el callback de CSI 
        if (xQueueReceive(wifi_task_queue, &msg, portMAX_DELAY)) {
            switch(msg.msgID) {
                case MSG_WIFI_START_AP_MODE:
                    ESP_LOGI(TAG, "received MSG_START_AP_MODE");
                    wifi_init();
                    g_esp_netif_ap = wifi_config_ap(); 
                    ESP_LOGI(TAG, "AP started");
                break;

                case MSG_WIFI_START_SENSING:
                    ESP_LOGI(TAG, "received MSG_WIFI_START_SENSING");
                    //CSI task will be started
                    csi_task_start(); 
                    
                break; 

                default:
                break;
            }
        }
    }

}

void wifi_task_start(void) {
    ESP_LOGI(TAG, "Starting task...");
    //Create message queue for task communication
    wifi_task_queue = xQueueCreate(10, sizeof(wifi_task_message_t));
    //Create task
    xTaskCreatePinnedToCore(&wifi_task, "wifi_task", WIFI_TASK_STACK_SIZE, NULL, WIFI_TASK_PRIORITY, NULL, WIFI_TASK_CORE_ID);
}