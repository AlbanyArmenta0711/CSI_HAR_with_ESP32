#ifndef PROCESSOR_TASK_H
#define PROCESSOR_TASK_H

#include "freertos/FreeRTOS.h"
#include "Freertos/task.h"
#include "../../common/common.h"
#include "../../common/custom_queue.h"

/*
 * Constants for processor_task
 */
#define PROCESSOR_TASK_STACK_SIZE 4096
#define PROCESSOR_TASK_PRIORITY 3
#define PROCESSOR_TASK_CORE_ID 1

#define PROCESSOR_WINDOW_SIZE 400

typedef struct hampel_filtered {
  float * data_filtered;
  int * indeces; 
} hampel_filtered_t;

//Enum for Processor message IDs
typedef enum processor_task_message_id {
  MSG_PROCESSOR_START = 0,
  MSG_PROCESSOR_PUSH_AND_PROCESS
} processor_task_message_id_enum; 

//Message structure
typedef struct processor_task_message {
  processor_task_message_id_enum msgID;
  void * data; 
} processor_task_message_t; 

//Struct that represents a time series of a single CSI subcarrier 
typedef struct csi_sc_data {
  m_queue_t csi_data;  
} csi_sc_data_t; 

/*
 * Function for sending a message to the Processor task queue
 * @param msg, the message to send to the task queue
 * @return pdTrue if the message was successfully posted to the task queue
 */
//Function for sending a message to the Processor task queue
BaseType_t processor_task_send_message(processor_task_message_t msg); 


/*
 * Function to calculate amplitude and phase from raw complex CSI data.
 * @param csi_data: pointer to raw complex CSI data
 * @param amp_phase: pointer to where CSI calculations will be stored
 */
void get_amp_phase(csi_data_t * csi_data, csi_amp_phase_t *amp_phase);


/*
 * Function to obtain the indeces of the k most sensitive subcarriers according to
 * variance selection method.
 * @param k: number of subcarriers to select
 * @param csi_matrix: pointer to the csi matrix
 * @param size: number of timesteps of the matrix (rows)
 * @return pointer to a vector storing k indeces
 */
int * variance_sc_selection(int k, float *csi_matrix, int size); 

/*
 * Function to be called to start the processor task
 */
void processor_task_start(void); 


#endif