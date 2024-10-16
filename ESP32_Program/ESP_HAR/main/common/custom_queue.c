#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "esp_log.h"
#include "esp_mac.h"
#include "custom_queue.h"

static const char * TAG = "QUEUE DEBUG";

void init_queue(m_queue_t *q, dtype_e dtype, int max_numel) {
    switch (dtype) {
        case TYPE_INT:
            q->data.data_int = malloc(sizeof(int) * max_numel);
            break;

        case TYPE_CSI:
            q->data.data_csi = malloc(sizeof(csi_data_t) * max_numel);
            break;

        case TYPE_CSI_PROCESSED:
            q->data.csi_features = malloc(sizeof(queue_csi_t) * max_numel); 
            break; 
    }
    q->head = -1;
    q->tail = -1;
    q->max_numel = max_numel;
    q->stored_type = dtype;
}

int is_empty(m_queue_t *q) { return q->head == -1 ? 1 : 0; }

int is_full(m_queue_t *q) {
    if (q->head == 0 && q->tail == q->max_numel - 1)
        return 1;
    else if (q->tail == (q->head - 1) % (q->max_numel-1))
        return 1;
    else
        return 0;
}


int enqueue(m_queue_t *q, void *value) {
    if (is_full(q)) { 
        return -1;
    }
    else {
        if (is_empty(q)) {
            q->head = 0;
            q->tail = 0;
        }
        else if (q->tail == q->max_numel - 1 && q->head != 0) {
            q->tail = 0;
        }
        else {
            q->tail++;
        }
        switch (q->stored_type) {
            case TYPE_INT:
                q->data.data_int[q->tail] = *(int *) value;
                break;

            case TYPE_CSI:
                q->data.data_csi[q->tail] = *(csi_data_t *)value;
                break;

            case TYPE_CSI_PROCESSED:
                //q->data.csi_features[q->tail] = *(csi_amp_phase_t *)value; 
                q->data.csi_features[q->tail].amp = ((queue_csi_t *) value)->amp;
                //ESP_LOGI(TAG, "pushed %f", ((queue_csi_t *) value)->amp);
                q->data.csi_features[q->tail].phase = ((queue_csi_t *) value)->phase;
                break;
        }
    }
    
    return 0;
}


void * queue_amp_to_ptr(m_queue_t *q) {
    int queue_size = get_count((*q));
    float * ptr_start = (float *) malloc(sizeof(float) * queue_size);
    int idx_aux = q->head; 
    for(int i = 0; i < queue_size; i++ ) {
        ptr_start[i] = q->data.csi_features[idx_aux].amp;

        if(idx_aux == q->max_numel - 1) {
            idx_aux = 0; 
        } else {
            idx_aux++;
        }
    }

    return ptr_start; 

}

int get_count(m_queue_t q) {
    int size = q.head > q.tail ? (q.max_numel - q.head + q.tail + 1) : (q.tail - q.head + 1);
    return size; 
}

void show_queue(m_queue_t q)
{
    int idx = q.head;
    switch (q.stored_type) {
        case TYPE_INT:
            while (idx <= q.tail)
            {
                ESP_LOGI(TAG, "%d", q.data.data_int[idx]);
                idx++;
            }
            break;

        case TYPE_CSI:
            while (idx <= q.tail) {
                ESP_LOGI(TAG, "%u", q.data.data_csi[idx].pkt_ctrl.timestamp);
                idx ++;
            }
            break;

        case TYPE_CSI_PROCESSED:
            while (idx <= q.tail) {
                ESP_LOGI(TAG, "%f", q.data.csi_features[idx].amp);
                idx ++;
            }
            break;
    }
}

void *dequeue(m_queue_t *q) {
    void *data = NULL;
    if (is_empty(q)) {
        printf("Queue is empty! value not removed\n");
        return NULL;
    }
    else {
        switch (q->stored_type) {
            case TYPE_INT:
                data = (int *) malloc(sizeof(int));
                memcpy(data, &q->data.data_int[q->head], sizeof(int));
                break;

            case TYPE_CSI:
                data = (csi_data_t *) malloc(sizeof(csi_data_t));
                memcpy(data, &q->data.data_csi[q->head], sizeof(csi_data_t));
                break;

                
            case TYPE_CSI_PROCESSED:
                //data = (queue_csi_t *) malloc(sizeof(queue_csi_t));
                data = &(q->data.csi_features[q->head]);
                /*if (data != NULL) {
                    memcpy(data, &q->data.csi_features[q->head], sizeof(queue_csi_t));
                } else {
                    ESP_LOGW(TAG, "error allocating memory for dequeue operation");
                    return NULL; 
                }*/
                break;
        }

    }

    // Update indexes
    if (q->head == q->tail) { // Only one element in queue
        q->head = -1;
        q->tail = -1;
    }
    else if (q->head == q->max_numel - 1) { // Head is at last position
        q->head = 0;
    }
    else {
        q->head++;
    }
    return data;
}
