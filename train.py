import numpy as np 
import keras 
from keras import layers
from keras import ops 

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
lstm_layer = layers.LSTM(
    units = 128,
    use_cudnn=False
)
net = lstm_layer(inputs)
net = layers.Dense(
    units = 128,
    activation = "relu"
)(net)
net = layers.Dense(
    units = 128,
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
batch_size = 12 
num_epochs = 500
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
test_scores = model.evaluate(tst_set_x, tst_set_y,verbose = 2)
print("Test loss:", test_scores[0])
print("Test accuracy:", test_scores[1])

#Save the model
model.export("har_model", "tf_saved_model")
