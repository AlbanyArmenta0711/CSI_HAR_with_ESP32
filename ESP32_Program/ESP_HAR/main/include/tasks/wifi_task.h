#ifndef WIFI_TASK_H
#define WIFI_TASK_H 

#include <esp_wifi_types.h>
#include "freertos/FreeRTOS.h"
#include "Freertos/task.h"

/*
 * Constants for Wi-Fi FreeRTOS task 
 */
#define WIFI_TASK_STACK_SIZE 4096
#define WIFI_TASK_PRIORITY 5 
#define WIFI_TASK_CORE_ID 0 

/*
 * Constants for Wi-Fi Access Point configuration
 */
#define WIFI_TASK_CSI_COLLECTION_SSID "CSI_Collection"
#define WIFI_TASK_CSI_COLLECTION_PASSWORD "root1234"
#define WIFI_TASK_AP_IP_ADDRESS "192.168.1.1"
#define WIFI_TASK_AP_NETMASK "255.255.255.0"
#define WIFI_TASK_AP_HIDDEN 0 //1 - Hide SSID, 0 - Show SSID 
#define WIFI_TASK_CHANNEL 1
#define WIFI_TASK_MAX_CONNECTIONS 5
#define WIFI_TASK_BEACON_INTERVAL 100
#define WIFI_TASK_BANDWIDTH WIFI_BW_HT20
#define WIFI_TASK_AUTH_MODE WIFI_AUTH_WPA2_PSK

/*
 * Utils for handling messages between tasks
 */

//Enum for Wi-Fi message IDs
typedef enum wifi_task_message_id{
    MSG_WIFI_START_AP_MODE = 0,
    MSG_WIFI_START_SENSING
} wifi_task_message_id_enum;

//Message structure
typedef struct wifi_task_message{
    wifi_task_message_id_enum msgID; 
    void * data; 
} wifi_task_message_t; 

/*
 * Function for sending a message to the Wi-Fi task queue
 * @param msg, the message to send to the task queue
 * @return pdTrue if the message was successfully posted to the task queue
 */
//Function for sending a message to the Wi-Fi task queue
BaseType_t wifi_task_send_message(wifi_task_message_t msg); 

/*
 * Function for getting the current Wi-Fi configuration
 * @return a pointer to the current Wi-Fi configuration
 */
wifi_config_t * wifi_task_get_wifi_config(void); 

/*
 * Function to be called by main task to start the Wi-Fi task
 */
void wifi_task_start(void);

#endif 