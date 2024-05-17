#ifndef COMMON_H
#define COMMON_H

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

#endif