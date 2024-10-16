#ifndef CSI_TASK_H
#define CSI_TASK_H

#include <stdbool.h>
#include "esp_wifi.h"
#include "Freertos/FreeRTOS.h"
#include "Freertos/task.h"

/*
 * Constants for the CSI task
 */
#define CSI_TASK_STACK_SIZE 2048
#define CSI_TASK_PRIORITY 4 //Lower priority if compared to Wi-Fi task
#define CSI_TASK_CORE_ID 1

/*
 * Default configuration values for CSI 
 */
#define CSI_TASK_LLTF true //128 bytes of data per packet
#define CSI_TASK_HT_LTF false //256 bytes of data per packet
#define CSI_TASK_STBC_HT_LTF false //384 bytes of data per packet
#define CSI_TASK_LTF_MERGE false 
#define CSI_TASK_CHANNEL_FILTER false 
#define CSI_TASK_MANU_SCALE false 
#define CSI_TASK_SHIFT false 

//Enum for CSI message IDs
typedef enum csi_task_message_id {
    MSG_CSI_START_CSI_COLLECTION = 0
} csi_task_message_id_enum;

typedef struct csi_task_message {
    csi_task_message_id_enum msgID; 
} csi_task_message_t;

/*
 * Function for sending a message to the csi task queue
 * @param msg: msg to be sent
 */
BaseType_t csi_task_send_message(csi_task_message_t msg);

/*
 * Function to start the CSI task
 */
void csi_task_start();

#endif