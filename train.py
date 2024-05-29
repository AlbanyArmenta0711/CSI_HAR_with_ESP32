import numpy as np 
import keras 
from keras import layers
from keras import ops 
import tensorflow as tf 
import time

#Load files
trn_set_x = np.load('trn_set_x.npy')
trn_set_y = np.load('trn_set_y.npy')
tst_set_x = np.load('tst_set_x.npy')
tst_set_y = np.load('tst_set_y.npy')
#Convert labels to categorical
trn_set_y = keras.utils.to_categorical(trn_set_y)
tst_set_y = keras.utils.to_categorical(tst_set_y)

#Create an input node using TF Functional API
inputs = keras.Input(shape = (650,64))

#Construct the net
conv1d_layer = layers.Conv1D(
    filters = 64, 
    kernel_size = 3,
    activation = "relu"
)
net = conv1d_layer(inputs)
net = layers.MaxPool1D()(net)
net = layers.Dropout(0.2)(net)
net = layers.Conv1D(
    filters = 32,
    kernel_size = 3,
    activation = "relu"
)(net)
net = layers.MaxPool1D()(net)
net = layers.Dropout(0.2)(net)
net = layers.Flatten()(net)
net = layers.Dense(
    units = 16,
    activation = "relu"
)(net) 
outputs = layers.Dense(
    units = 3,
    activation = "softmax"
)(net) 

#Create the model
model = keras.Model(
    inputs = inputs,
    outputs = outputs, 
    name = "CSI_HAR_model"
)
model.summary()

#Specify training configuration
batch_size = 6 
num_epochs = 20
criterion = keras.losses.CategoricalCrossentropy()
optimizer = keras.optimizers.Adam(learning_rate = 0.0001)
model.compile(
    loss = criterion,
    optimizer = optimizer,
    metrics = ["accuracy"]
)

#Train the model according to the configuration specified
history = model.fit(
    x = trn_set_x,
    y = trn_set_y, 
    batch_size = batch_size, 
    epochs = num_epochs,
    validation_split = 0.2
)

#Test the model with x_test
times = []
for i in range(32):
    t_obs = tst_set_x[i,:,:]
    t_obs = t_obs[np.newaxis,...]
    y_obs = tst_set_y[i]
    y_obs = y_obs[np.newaxis,...]
    inicio = time.time_ns()
    test_scores = model.evaluate(t_obs, y_obs,verbose = 2)
    fin = time.time_ns()
    times.append(fin-inicio)
    print("Test loss:", test_scores[0])
    print("Test accuracy:", test_scores[1])

print(np.mean(times))
#Save the model
#model.export("har_model", "tf_saved_model")
