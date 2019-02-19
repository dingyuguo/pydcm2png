import os
#os.environ['DCMDICTPATH']='/usr/local/share/dcmtk_old/dicom.dic'
import imp
_,p,_ = imp.find_module('pydcm2png')
if os.path.isfile(p):
    p=''
os.environ['DCMDICTPATH']=os.path.join(p,'dicom.dic')
import dcm2png
def to_png(dcm_file):
    width = dcm2png.get_width(dcm_file)
    height = dcm2png.get_height(dcm_file)
    dcm_vector = dcm2png.pixel_data(dcm_file,width*height)
    dcm_img = dcm_vector.reshape(height,width)
    return dcm_img

