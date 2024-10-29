#include "../../include/tasks/processor_task.h"
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "../../common/custom_queue.h"
#include "../../include/filters/filters.h"
#include "../../include/tasks/predictor.h"
#include "../../include/tasks/csi_task.h"
#include "esp_timer.h"

static const char * TAG = "Processor TASK";
static QueueHandle_t processor_task_queue; 
static EXT_RAM_BSS_ATTR fir_filter_t * lowpass_filter[NUM_SC];
static EXT_RAM_BSS_ATTR csi_sc_data_t *subcarriers_window[NUM_SC]; 
//Processed CSI will be flatten for prediction (64xPROCESSOR_WINDOW_SIZE)
static EXT_RAM_BSS_ATTR float * processed_csi;
//static EXT_RAM_BSS_ATTR float * processed_csi[64][PROCESSOR_WINDOW_SIZE]; 
static csi_amp_phase_t *amp_phase_estimation; 

//Updated when new amplitudes are calculated and used for rescaling [0,1]
static EXT_RAM_BSS_ATTR float min_sc_values[NUM_SC];
static EXT_RAM_BSS_ATTR float max_sc_values[NUM_SC];

static esp_timer_handle_t message_timer;

//static ignored_csi_sc [] = {0, 1, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37};

static bool classify_obs = false; 

BaseType_t processor_task_send_message(processor_task_message_t msg) {
    //Block for 20ms max if queue is full (for CSI task)
    return xQueueSend(processor_task_queue, &msg, (20/portTICK_PERIOD_MS));
}

void get_amp_phase(csi_data_t * csi_data, csi_amp_phase_t *amp_phase) {
    int8_t re[NUM_SC] = {0};
    int8_t im[NUM_SC] = {0};
    int im_idx = 0;
    int re_idx = 0;
    for (int i = 0; i < 128; i++) {
        if ((i > 3 && i < 54) || i > 75) {
            if (i % 2 == 0) { // Imaginary
                im[im_idx] = csi_data->buf[i];
                im_idx++;
            } else { // Real
                re[re_idx] = csi_data->buf[i];
                re_idx++;
            }
        }
    }
    for (int i = 0; i < NUM_SC; i++) {
        float pow_im = pow(im[i], 2);
        float pow_re = pow(re[i], 2);
        float amp_i; 
        if (pow_im != HUGE_VALF || pow_re != HUGE_VALF) {
            amp_i = sqrt( pow_im + pow_re );
            *(amp_phase->amp + i) = amp_i; 
            //Check if value is greater or smaller than max/min saved values
            if (amp_i > max_sc_values[i]) 
                max_sc_values[i] = amp_i; 
            if (amp_i < min_sc_values[i])
                min_sc_values[i] = amp_i; 
        }
        else {
            ESP_LOGW(TAG, "error estimating amplitude, value set to nan");
            *(amp_phase->amp + i) = NAN;
        }
        //*(phase + i) = atan2(im[i], re[i]);
    }

}


static void timer_callback(void * args) {
    if (!classify_obs)
        classify_obs = true; 
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
                    csi_task_message_t csi_msg = {
                        .msgID = MSG_CSI_PROCESSOR_STARTED,
                    };
                    csi_task_send_message(csi_msg); 
                    break;
                case MSG_PROCESSOR_PUSH_AND_PROCESS:
                    ESP_LOGI(TAG, "received MSG_PROCESSOR_PUSH");
                    //Estimate amplitude and phase of all subcarriers
                    get_amp_phase((csi_data_t *)msg.data, amp_phase_estimation);
                    
                        for (int i = 0; i < NUM_SC; i++) {
                            //Get amplitude and phase from csi variable for each subcarrier to build a time window for each
                            queue_csi_t single_csi_amp_phase = {
                                .amp =amp_phase_estimation->amp[i],
                                .phase = amp_phase_estimation->phase[i]
                            };
                            //Apply FIR filter to csi amplitude
                            //if (i == 16)
                            //    ESP_LOGI(TAG, "before filter %f", single_csi_amp_phase.amp);
                            filt(lowpass_filter[i], &single_csi_amp_phase.amp, 1 , &single_csi_amp_phase.amp);
                            //if (i == 16)
                            //    ESP_LOGI(TAG, "after filter %f", single_csi_amp_phase.amp);
                            //Enqueue filtered incoming amp and phase to corresponding subcarrier
                            if (enqueue(&(subcarriers_window[i]->csi_data), &single_csi_amp_phase) == 0) {
                                
                                if (get_count(subcarriers_window[0]->csi_data) > PROCESSOR_WINDOW_SIZE) 
                                    dequeue(&(subcarriers_window[i]->csi_data));
                            }
                        }
                        //If its time to classify...
                        if (get_count(subcarriers_window[0]->csi_data) >= PROCESSOR_WINDOW_SIZE && classify_obs == true) {
                            ESP_LOGI(TAG, "preparing observation");
                            for(int i = 0 ; i < NUM_SC; i++) {
                                float * sc_arr = (float *) queue_amp_to_ptr(&(subcarriers_window[i]->csi_data), max_sc_values[i], min_sc_values[i]);
                                //hampel_identifier(sc_arr, PROCESSOR_WINDOW_SIZE, HAMPEL_WIN_SIZE, HAMPEL_THRESHOLD);
                                for(int j = 0; j < PROCESSOR_WINDOW_SIZE; j++) {
                                    processed_csi[j * NUM_SC + i] = sc_arr[j];
                                } 
                                if (sc_arr != NULL)
                                    free(sc_arr); 
                            }
                            ESP_LOGI(TAG, "classifying");
                            predict(processed_csi);
                            classify_obs = false; 
                        }
                    if (msg.data != NULL) {
                        if (((csi_data_t *) msg.data)->buf != NULL)
                            free(((csi_data_t *) msg.data)->buf);
                        free(msg.data);
                    }
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
    float *amp = (float *) malloc(sizeof(float) * NUM_SC); 
    float *phase = (float *) malloc(sizeof(float) * NUM_SC);
    amp_phase_estimation->amp = amp; 
    amp_phase_estimation->phase = phase; 
    //init each subcarriers_window queue and filter for each subcarrier 
    for (int i = 0; i < NUM_SC; i++) {
        subcarriers_window[i] = (csi_sc_data_t *) malloc(sizeof(csi_sc_data_t));
        init_queue(&(subcarriers_window[i]->csi_data), TYPE_CSI_PROCESSED, PROCESSOR_WINDOW_SIZE + 20); 
        lowpass_filter[i] = init_fir_filter(); 

        //Initialize min and max values for rescaling [0,1]
        min_sc_values[i] = 9999;
        max_sc_values[i] = -9999;
    }
    processed_csi = (float *) malloc(sizeof(float) * PROCESSOR_WINDOW_SIZE * NUM_SC);
    ESP_LOGI(TAG, "FIR filter initialized");

    //Create a timer so every second classification will be done
    esp_timer_create_args_t timer_config = {
        .arg = NULL,
        .callback = timer_callback,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "classification_timer",
        .skip_unhandled_events = true 
        
    };
    
    ESP_ERROR_CHECK(esp_timer_create(&timer_config, &message_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(message_timer,1000000));

    //Create task
    xTaskCreatePinnedToCore(&processor_task, "processor_task", PROCESSOR_TASK_STACK_SIZE, NULL, PROCESSOR_TASK_PRIORITY, NULL, PROCESSOR_TASK_CORE_ID);
}