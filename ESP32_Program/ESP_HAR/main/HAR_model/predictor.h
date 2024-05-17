#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

#include "../common/common.h"

/*
 * Function to make a prediction using the model allocated
 * @param input: struct of type model_input_t containing the input and its data for the model
 * @return the class label or -1 if an error in prediction.
 */
EXTERNC int predict(float * input_data); 

/*
 * Function to setup the model for HAR
 * @return 1 on success, -1 otherwise
 */
EXTERNC int init_model(); 

#undef EXTERNC