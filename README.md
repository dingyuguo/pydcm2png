# pydcm2png
https://pypi.org/project/pydcm2png/
	This usefull tool is for converting DICOM format file to PNG file. 
	DICOM file is a common file format in medical image processing, so we always need to import DICOM file to our project.
	Cause of the complex procedure, that's not a easy thing to using it, though there are some tools for that, like dcmtk(c++),pydcm(python) and some other tools; dcmtk is a c++ library, it's a very powerful tools when process dicom file, but it requires developer know lots of details of dicom format and c++ knowledge; pydcm is a light tool in python, but it only can deal with part of dicom format and it can not convert pixel data of dicom to png file rightly based on DIOCM procedure, for example, DICOM format file consists of varity of storgae format including int8,uint8,int16,uint16 etc, also we need to conside different alogrithms dealing with such different storage format. 
	So I developped pydcm2png tools for overcoming problems above. Now pydcm2png support py2/py3 on Linux.
        Using the following command can easily install pydcm2png:
		pip install pydcm2png
 	This tool was developped with C++ and python. 
        I use dcmtk(https://support.dcmtk.org/docs/index.html) STK as image processing library.
