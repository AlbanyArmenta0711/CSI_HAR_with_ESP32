import numpy as np 
import pandas as pd 
import os 

# 0 - Fall, 1 - Run, 2 - Walk
CLASSES = ["Fall", "Run", "Walk"]
TRN_DIR = "Datasets/Training/"
TST_DIR = "Datasets/Test/"
TRN_SAMPLES = 90
TST_SAMPLES = 30 

def csi_to_amp(csi_data):
    """
    Function to convert raw CSI to CSI amplitude

    Parameters
    ----------
    csi_data : ndarray  
        Raw CSI data

    Returns
    -------
    csi_amps : ndarray
        numpy array containing CSI amplitude values 
    """
    csi_amps = []
    #Get imaginary and real parts from csi_data
    re = []
    im = []
    #per row in csi_data
    for measurement in csi_data: 
        for idx in range(len(measurement)):
            if idx % 2 == 0:
                im.append(measurement[idx])
            else: 
                re.append(measurement[idx])
        csi_row = np.sqrt(np.power(im,2) + np.power(re,2))
        csi_amps.append(csi_row)
        im = []
        re = [] 

    return np.array(csi_amps) 


def load_file(filename): 
    """
    Function to load a CSI file given a filename

    Parameters
    ----------
    filename : str  
        Name of the raw CSI data file to be loaded

    Returns
    -------
    csi_data : ndarray
        numpy array containing CSI data of the specified file
    """
    data_frame = pd.read_csv(filename, sep = ' ')
    csi_data = data_frame.iloc[:, 1:129].to_numpy()
    return csi_data
    
trn_set_x = []
trn_set_y = []
tst_set_x = []
tst_set_y = []

#Get training data set 
for num_class in range(len(CLASSES)):
    dir_path = TRN_DIR + CLASSES[num_class] + '/'
    dir_list = os.listdir(dir_path)
    for file in dir_list:
        csi_data = load_file(dir_path + file)
        csi_amps = csi_to_amp(csi_data)
        #window will be truncated to 650 so all samples have the same size 
        csi_amps = csi_amps[0:650,:]
        trn_set_x.append(csi_amps)
        trn_set_y.append(num_class)
trn_set_x = np.array(trn_set_x)
trn_set_y = np.array(trn_set_y)
#Save training data into files
np.save("trn_set_x.npy", trn_set_x)
np.save("trn_set_y.npy", trn_set_y)

#Get test dataset 
for num_class in range(len(CLASSES)):
    dir_path = TST_DIR + CLASSES[num_class] + '/'
    dir_list = os.listdir(dir_path)
    for file in dir_list:
        csi_data = load_file(dir_path + file)
        csi_amps = csi_to_amp(csi_data)
        #window will be truncated to 650 so all samples have the same size 
        csi_amps = csi_amps[0:650,:]
        tst_set_x.append(csi_amps)
        tst_set_y.append(num_class)
tst_set_x = np.array(trn_set_x)
tst_set_y = np.array(trn_set_y)
#Save training data into files
np.save("tst_set_x.npy", tst_set_x)
np.save("tst_set_y.npy", tst_set_y)
