#include <string.h>
#include <stdio.h>
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/system_setup.h"
#include <esp_attr.h>
#include "esp_log.h"

#include "../../include/tasks/predictor_task.h"
#include "../../include/model/model.h"


namespace {
  const tflite::Model *model = nullptr; 
  const char * TAG = "Predictor";
  tflite::MicroInterpreter* interpreter = nullptr;
  TfLiteTensor* input = nullptr;
  TfLiteTensor* output = nullptr;
  constexpr int kTensorArenaSize = 210950;
  EXT_RAM_BSS_ATTR uint8_t tensor_arena[kTensorArenaSize];
  const int data_rows = 650;
  const int data_cols = 64;
  const int input_size = data_rows * data_cols;  
}

int predict(float * input_data) {
    memcpy(input->data.f, input_data, sizeof(float) * 650 * 64);
    //Run inference
    TfLiteStatus invoke_status = interpreter->Invoke(); 
    if (invoke_status != kTfLiteOk) {
        ESP_LOGE(TAG, "Invoke failed");
    }
    //Process the inference results
    float score_f[3] = {0};
    float max_score = 0; 
    int label = -1; 
    for (int i = 0; i < 3; i++){
        score_f[i] = output->data.f[i];
        if (score_f[i] > max_score) {
            max_score = score_f[i];
            label = i; 
        }
        
        ESP_LOGI(TAG, "probability of class %d: %f", i, score_f[i]);
    }
    return label; 
}

static int setup_model() {
    //Map the model into a usable data structure 
    model = tflite::GetModel(g_model); 
    if (model->version() != TFLITE_SCHEMA_VERSION) {
         MicroPrintf("Model provided is schema version %d not equal to supported "
                "version %d.", model->version(), TFLITE_SCHEMA_VERSION);
        return -1; 
    } 
    static tflite::MicroMutableOpResolver<10> op_resolver;
    TF_LITE_ENSURE_STATUS(op_resolver.AddFullyConnected());
    TF_LITE_ENSURE_STATUS(op_resolver.AddQuantize());
    TF_LITE_ENSURE_STATUS(op_resolver.AddSoftmax());
    TF_LITE_ENSURE_STATUS(op_resolver.AddRelu());
    TF_LITE_ENSURE_STATUS(op_resolver.AddAdd()); 
    TF_LITE_ENSURE_STATUS(op_resolver.AddExpandDims()); 
    TF_LITE_ENSURE_STATUS(op_resolver.AddDequantize());
    TF_LITE_ENSURE_STATUS(op_resolver.AddReshape());
    TF_LITE_ENSURE_STATUS(op_resolver.AddMaxPool2D());
    TF_LITE_ENSURE_STATUS(op_resolver.AddConv2D());
    static tflite::MicroInterpreter static_interpreter(model, op_resolver, tensor_arena, kTensorArenaSize);
    interpreter = &static_interpreter; 

    //Allocate memory for the models' tensors
    TfLiteStatus allocate_status = interpreter->AllocateTensors(); 
    int num_bytes_tensor = interpreter->arena_used_bytes();
    if (allocate_status != kTfLiteOk) {
        ESP_LOGW(TAG, "Error allocating model tensors");
        return -1; 
    } else {
        ESP_LOGI(TAG, "Tensors allocated for model, size in bytes: %d", num_bytes_tensor); 
    }

    input = interpreter->input(0); 
    output = interpreter->output(0); 
    return 1; 
}

int init_model() {
    int err; 
    err = setup_model(); 
    return err; 
}