import tensorflow as tf 
import numpy as np 
import keras 

MODEL_PATH = "har_model"
x_test = np.float32(np.load("tst_set_x.npy"))

def representative_data_gen():
  for input_value in tf.data.Dataset.from_tensor_slices(x_test).batch(1).take(100):
    yield [input_value]

#Convert the model to TFLite
converter = tf.lite.TFLiteConverter.from_saved_model(MODEL_PATH)
#Model Quantization for fixed point
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.representative_dataset = representative_data_gen
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS]
converter._experimental_lower_tensor_list_ops = False
tflite_quant_model = converter.convert()
#Save the model
with open('model_quant.tflite', 'wb') as f:
  f.write(tflite_quant_model)