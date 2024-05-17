import numpy as np 
import pandas as pd 

sample_selected = 62

tst_set_x = np.load('tst_set_x.npy')
tst_input_example = tst_set_x[sample_selected,:,:]

tst_set_y = np.load("tst_set_y.npy")
filename = str(tst_set_y[sample_selected]) + ".txt"
fp = open(filename,"a")


for idx_row in range(len(tst_input_example)):
    num_sc = len(tst_input_example[idx_row])
    for idx_sc in range(num_sc):
        amp = str(np.float32(tst_input_example[idx_row,idx_sc])) + ","
        fp.write(amp)

fp.close()