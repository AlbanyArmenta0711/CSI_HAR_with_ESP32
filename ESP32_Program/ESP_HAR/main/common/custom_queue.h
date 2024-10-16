/*
 * Header file for describing a queue and its method
 * Created on: 08/08/2024
 * by: Jesus A. Armenta-Garcia
 */

#ifndef CUSTOM_QUEUE_H
#define CUSTOM_QUEUE_H


#include <stdint.h>
#include "common.h"

typedef enum dtype { TYPE_INT = 0, TYPE_CSI , TYPE_CSI_PROCESSED} dtype_e;

typedef struct queue_csi {
  float amp;
  float phase; 
} queue_csi_t;

typedef struct m_queue {
  union {
    int *data_int;
    csi_data_t *data_csi;
    queue_csi_t *csi_features; 
  } data;
  int max_numel;
  int head;
  int tail;
  dtype_e stored_type; 
} m_queue_t;

/*
 * Function to initialize a queue according to the size of data to be stored and
 * the maximum number of elements that can be stored.
 * @param q: pointer to the queue to be initialized.
 * @param dtype: data type to be stored in the queue.
 * @param max_numel: maximum number of elements that can be stored in the queue.
 */
void init_queue(m_queue_t *q, dtype_e dtype, int max_numel);

/*
 * Function to get queue content as pointer to each element in queue 
 */
void * queue_amp_to_ptr(m_queue_t *q);

/*
 * Function to check if queue is empty.
 * @param q: queue to be checked.
 * @return 1 if queue is empty, 0 otherwise.
 */
int is_empty(m_queue_t *q);

/*
 * Function to check if queue is full.
 * @param q: queue to be checked.
 * @return 1 if queue is full, 0 otherwise.
 */
int is_full(m_queue_t *q);

/*
 * Function to add a new element to the queue.
 * @param q: queue to add the element to.
 * @param value: pointer to the new element to be added.
 * @return 0 if the element was added successfully, -1 otherwise.
 */
int enqueue(m_queue_t *q, void *value);

/*
 * Function to remove an element from the queue.
 * @param q: queue to remove the element from.
 * @return pointer to the removed element, NULL if the queue is empty.
 */
void *dequeue(m_queue_t *q);

/*
 * Function to show the queue content.
 * @param q: queue to be shown.
 */
void show_queue(m_queue_t q);

/*
 * Function that returns the count of elements in the queue.
 * @param q: queue to be counted.
 * @return number of elements in the queue.
 */
int get_count(m_queue_t q);

#endif