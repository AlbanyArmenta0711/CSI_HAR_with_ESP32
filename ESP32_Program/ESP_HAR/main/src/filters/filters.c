#include <stdlib.h>
#include <string.h>
#include "../../include/filters/filters.h"
#include "../../include/filters/lowpass.h"
#include "esp_log.h"
#include <math.h>

static const char * TAG = "FIR";

int compare(const void *a, const void *b) {
  float fa = *(const float *)a;
  float fb = *(const float *)b;
  return (fa > fb) - (fa < fb);
}

static float get_median(float *vector, int size) {
  float median;
  int idx_lower = 0, idx_upper = 0;
  // Obtained a sorted version of vector
  float *sorted_vector = (float *)malloc(sizeof(float) * size);
  memcpy(sorted_vector, vector, sizeof(float) * size);
  qsort(sorted_vector, size, sizeof(float), compare);

  // Know if vector size if even or odd
  if (size % 2 == 0) { // Size is even
    idx_lower = size / 2 - 1;
    idx_upper = size / 2;
    median = (sorted_vector[idx_lower] + sorted_vector[idx_upper]) / 2;
  } else { // Size is odd
    idx_upper =
        size / 2; // As array starts at 0, adding 1 to size is not required
    median = sorted_vector[idx_lower];
  }
  free(sorted_vector); 
  return median;
}

fir_filter_t* init_fir_filter() {
    fir_filter_t *filter = (fir_filter_t *) malloc(sizeof(fir_filter_t)); 
    int order = coeffs_len - 1;
    filter->b_coeffs = (float *) malloc(sizeof(float) * coeffs_len);
    memcpy(filter->b_coeffs, b_coeffs, sizeof(float) * coeffs_len);
    filter->order = order;
    filter->z = (float *) malloc(sizeof(float) * order);
    memset(filter->z, 0.00, sizeof(float) * order);
    return filter;
}

void filt(fir_filter_t *filter, float *input, float size, float *output) {
  float x_i;
  float y_i = 0;
  *(filter->z) = *input;
  for (int i = 0; i < size; i++) {
    x_i = *(input + i);
    y_i = 0;
    // Apply filter considering previous inputs (z)
    for (int j = 0; j <= filter->order; j++) {
      if (j == 0) {
        y_i = filter->b_coeffs[0] * x_i;
      } else {
        float mult = filter->b_coeffs[j] * (*(filter->z + j));
        y_i += mult;
      }
    }
    *(output + i) = y_i;
    // Update z values
    for (int j = filter->order; j > 0; j--)
      *(filter->z + j) = *(filter->z + (j - 1));
    *(filter->z) = x_i; 

  }
}

void hampel_identifier(float *data, int size, int win_size, int t) {
  float median;
  float *s_vector;
  float *csi_window;
  float x_i;
  float S;
  int start_idx;
  int x_i_idx = 0;
  int end_idx;
  int current_win_size;
  s_vector = (float *)malloc(sizeof(float) * size);
  float k = 1.4826;

  // Initiliaze data filtered with csi vector values
  //memcpy(hampel_filt, input, sizeof(float) * size);

  do {
    start_idx = x_i_idx - win_size / 2;
    if (start_idx < 0)
      start_idx = 0;
    end_idx = x_i_idx + win_size / 2 - 1;
    if (end_idx >= size)
      end_idx = size - 1;
    x_i = data[x_i_idx];
    current_win_size = end_idx - start_idx + 1;
    csi_window = &data[start_idx]; // csi_window points to starting index
    median = get_median(csi_window, current_win_size);

    // Substract the median from each value in the window
    memcpy(s_vector, csi_window, sizeof(float) * current_win_size);
    for (int i = 0; i < current_win_size; i++) {
      float test_num = s_vector[i];
      s_vector[i] = fabs(s_vector[i] - median);
    }
    S = k * get_median(s_vector, current_win_size);
    if (fabs(x_i - median) > t * S)
      // x_i is an outlier
      data[x_i_idx] = median;

    x_i_idx++;
  } while (x_i_idx <= size - 1);

  free(s_vector); 
  //start_idx++;
}