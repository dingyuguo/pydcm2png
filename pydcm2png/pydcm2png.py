import os
#os.environ['DCMDICTPATH']='/usr/local/share/dcmtk_old/dicom.dic'
import imp
_,p,_ = imp.find_module('pydcm2png')
if os.path.isfile(p):
    p=''
os.environ['DCMDICTPATH']=os.path.join(p,'dicom.dic')
from .dcm2png import get_width
from .dcm2png import get_height
from .dcm2png import pixel_data

def get_pixel_data(dcm_file):
    width = get_width(dcm_file)
    height = get_height(dcm_file)
    dcm_vector = pixel_data(dcm_file,width*height)
    dcm_img = dcm_vector.reshape(height,width)
    return dcm_img

