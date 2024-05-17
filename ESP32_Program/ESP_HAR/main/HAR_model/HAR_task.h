/*
 * Header file that includes constants and function prototypes for the HAR task
 * Created on: 05/16/2024
 * by: Jesus A. Armenta-Garcia
 */

#ifndef HAR_TASK_H
#define HAR_TASK_H
#include "Freertos/FreeRTOS.h"
#include "Freertos/task.h"

/*
 * Constants for Wi-Fi FreeRTOS task
 */
#define HAR_TASK_STACK_SIZE 8192
#define HAR_TASK_PRIORITY 3
#define HAR_TASK_CORE_ID 1 

//Enum for messaging with the task when CSI measurement is received
typedef enum har_task_message_id {
    HAR_TASK_MSG_CSI_RECEIVED = 0, 
    HAR_TASK_MSG_TASK_INIT,
    HAR_TASK_MSG_TASK_STOP
} har_task_message_id_enum;

typedef struct csi_data {
    int8_t * csi; 
    uint16_t csi_len; 
} csi_data_t; 

//Struct containing message information
typedef struct har_task_message {
    har_task_message_id_enum msg_id; 
    csi_data_t data; 
} har_task_message_t; 

/*
 * Function to start the HAR task, which receives CSI raw measurements for Human Activity Recognition
 * @return 1 on success creating the task, 0 otherwise 
 */
int start_har_task(); 

#endif