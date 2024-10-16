#include "../../include/tasks/processor_task.h"
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "../../common/custom_queue.h"
#include "../../include/filters/filters.h"

static const char * TAG = "Processor TASK";
static QueueHandle_t processor_task_queue; 
static EXT_RAM_BSS_ATTR fir_filter_t * lowpass_filter[64];
static EXT_RAM_BSS_ATTR csi_sc_data_t *subcarriers_window[64]; 
//Processed CSI will be flatten for prediction (64xPROCESSOR_WINDOW_SIZE)
static EXT_RAM_BSS_ATTR float * processed_csi;
//static EXT_RAM_BSS_ATTR float * processed_csi[64][PROCESSOR_WINDOW_SIZE]; 
static csi_amp_phase_t *amp_phase_estimation; 

static bool obs_rdy; 

BaseType_t processor_task_send_message(processor_task_message_t msg) {
    return xQueueSend(processor_task_queue, &msg, portMAX_DELAY);
}

void get_amp_phase(csi_data_t * csi_data, csi_amp_phase_t *amp_phase) {
    int8_t re[64] = {0};
    int8_t im[64] = {0};
    int im_idx = 0;
    int re_idx = 0;
    for (int i = 0; i < 128; i++) {
        if (i % 2 == 0) { // Imaginary
            im[im_idx] = csi_data->buf[i];
            im_idx++;
        } else { // Real
            re[re_idx] = csi_data->buf[i];
            re_idx++;
        }
    }
    for (int i = 0; i < 64; i++) {
        float pow_im = pow(im[i], 2);
        float pow_re = pow(re[i], 2);
        if (pow_im != HUGE_VALF || pow_re != HUGE_VALF)
            *(amp_phase->amp + i) = sqrt( pow_im + pow_re );
        else {
            ESP_LOGW(TAG, "error estimating amplitude, value set to nan");
            *(amp_phase->amp + i) = NAN;
        }
        //*(phase + i) = atan2(im[i], re[i]);
    }

}

/*
 * Processor task
 */
static void processor_task() {
    processor_task_message_t msg = {
        .msgID = MSG_PROCESSOR_START,
        .data = NULL
    };
    processor_task_send_message(msg);

    while(1) {        
        if(xQueueReceive(processor_task_queue, &msg, portMAX_DELAY)) {
            switch(msg.msgID) {
                case MSG_PROCESSOR_START:
                    ESP_LOGI(TAG, "received MSG_PROCESSOR_START");
                    break;
                case MSG_PROCESSOR_PUSH_AND_PROCESS:
                    ESP_LOGI(TAG, "received MSG_PROCESSOR_PUSH");
                    //Estimate amplitude and phase of all subcarriers
                    get_amp_phase((csi_data_t *)msg.data, amp_phase_estimation);
                    
                        for (int i = 0; i < 64; i++) {
                            //Get amplitude and phase from csi variable for each subcarrier to build a time window for each
                            queue_csi_t single_csi_amp_phase = {
                                .amp =amp_phase_estimation->amp[i],
                                .phase = amp_phase_estimation->phase[i]
                            };
                            //Apply FIR filter to csi amplitude
                            if (i == 16)
                                ESP_LOGI(TAG, "before filter %f", single_csi_amp_phase.amp);
                            filt(lowpass_filter[i], &single_csi_amp_phase.amp, 1 , &single_csi_amp_phase.amp);
                            if (i == 16)
                                ESP_LOGI(TAG, "after filter %f", single_csi_amp_phase.amp);
                            //Enqueue filtered incoming amp and phase to corresponding subcarrier
                            if (enqueue(&(subcarriers_window[i]->csi_data), &single_csi_amp_phase) == 0) {
                                //Check if csi_window has WINDOW_SIZE elements
                                if (get_count(subcarriers_window[i]->csi_data) >= PROCESSOR_WINDOW_SIZE) {
                                    //Apply hampel filter to current window
                                    float * sc_to_hamp = (float *) queue_amp_to_ptr(&(subcarriers_window[i]->csi_data));
                                    hampel_identifier(sc_to_hamp, PROCESSOR_WINDOW_SIZE, HAMPEL_WIN_SIZE, HAMPEL_THRESHOLD);
                                    //Build observation
                                    for(int j = 0; j < PROCESSOR_WINDOW_SIZE; j++) {
                                        processed_csi[j * 64 + i] = sc_to_hamp[j];
                                    } 
                                    obs_rdy = true; //Observation is ready, now it can classify
                                    //Dequeue an element
                                    queue_csi_t * csi_dequeued = (queue_csi_t *) dequeue(&(subcarriers_window[i]->csi_data));
                                    
                                }
                            } else {
                                ESP_LOGW(TAG, "error pushing data subcarrier %d!", i);
                            }
                        }
                    free(((csi_data_t *) msg.data)->buf);
                    free(msg.data);
                    //Avoid wdt trigger
                    //vTaskDelay(1/portTICK_PERIOD_MS); 
                    break; 

            }
        }
        
    }
}

void processor_task_start(void) { 
    ESP_LOGI(TAG, "Starting task...");
    //Create message queue for task communication
    processor_task_queue = xQueueCreate(20, sizeof(processor_task_message_t));
    //Initialize the input buffer and the window
    ESP_LOGI(TAG, "data queues initialized");
    amp_phase_estimation = (csi_amp_phase_t *) malloc(sizeof(csi_amp_phase_t));
    float *amp = (float *) malloc(sizeof(float) * 64); 
    float *phase = (float *) malloc(sizeof(float) * 64);
    amp_phase_estimation->amp = amp; 
    amp_phase_estimation->phase = phase; 
    //init each subcarriers_window queue and filter for each subcarrier 
    for (int i = 0; i < 64; i++) {
        subcarriers_window[i] = (csi_sc_data_t *) malloc(sizeof(csi_sc_data_t));
        init_queue(&(subcarriers_window[i]->csi_data), TYPE_CSI_PROCESSED, PROCESSOR_WINDOW_SIZE + 50); 
        lowpass_filter[i] = init_fir_filter(); 
    }
    processed_csi = (float *) malloc(sizeof(float) * PROCESSOR_WINDOW_SIZE * 64);
    ESP_LOGI(TAG, "FIR filter initialized");
    //Create task
    xTaskCreatePinnedToCore(&processor_task, "processor_task", PROCESSOR_TASK_STACK_SIZE, NULL, PROCESSOR_TASK_PRIORITY, NULL, PROCESSOR_TASK_CORE_ID);
}