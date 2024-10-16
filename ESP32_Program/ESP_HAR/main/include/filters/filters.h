#ifndef FIR_FILTER_H
#define FIR_FILTER_H

#define HAMPEL_WIN_SIZE 20
#define HAMPEL_THRESHOLD 3 

typedef struct fir_filter {
  float * b_coeffs;
  int order;
  float * z;
} fir_filter_t;

/*
 * Function to initialize the FIR filter. 
 * @return pointer to the FIR filter to be initialized.
 */
fir_filter_t* init_fir_filter();

/*
 * Function to apply a previously initialized FIR filter to a given input vector.
 * At first, sample delays (z) will be initialized at 0, but values will change
 * according to new samples at input signal. 
 * @param fir_filter: pointer to the FIR filter to be applied
 * @param input: pointer to the signal to be filtered
 * @param size: input size 
 * @param output: pointer to where the filtered signal will be stored
 */
void filt(fir_filter_t * filter, float * input, float size, float * output);

/*
 * Function to apply hampel identifier to csi amplitude/phase
 * @param data: csi vector of either amplitude or phase (single subcarrier), hampel will be stored
 * @param size: vector size
 * @param win_size: window size for hampel identifier
 * @param t: threshold for outlier detection
 */
void hampel_identifier(float *data, int size, int win_size, int t);

#endif 