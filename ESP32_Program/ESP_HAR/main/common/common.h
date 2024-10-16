#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <esp_wifi_types.h>

typedef enum activity_label {
    FALL = 0, 
    RUN,
    WALK
} activity_label_enum;

typedef struct model_output {
    activity_label_enum predicted_activity; 
    float probability; 
} model_output_t;

typedef struct model_input {
    int steps;
    int subcarriers;
    float * csi_amps;
} model_input_t;

typedef struct csi_data{
    wifi_pkt_rx_ctrl_t pkt_ctrl;
    uint8_t mac_addr[6]; 
    uint16_t len;   
    int8_t * buf; 
} csi_data_t;

typedef struct csi_amp_phase {
  float *amp;
  float *phase;
} csi_amp_phase_t;


#endif