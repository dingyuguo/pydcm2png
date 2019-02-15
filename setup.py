from setuptools import *
import numpy as np
import os
import glob
depends_libs=glob.glob('/tmp/dcmtk-DCMTK-3.6.4/build/lib/*.so')
print(depends_libs)

with open("README.md", "r") as fh:
    long_description = fh.read()
_dcm2png = Extension(name='pydcm2png/_dcm2png',
                 sources=['pydcm2png/dcm2png.i','pydcm2png/dcm2png.cpp'],
                 language='c++',
                 swig_opts=['-c++'],
                 include_dirs=['/usr/local/lib/python2.7/site-packages/numpy/core/include/',
                               '/usr/local/lib/python3.5/dist-packages/numpy/core/include/',
                               '/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.14.sdk/usr/include',
                               '/usr/local/Cellar/gcc/7.3.0_1/include/c++/7.3.0/x86_64-apple-darwin17.3.0/',
                               '/usr/local/Cellar/gcc/7.3.0_1/include/c++/7.3.0/',
                               '/usr/local/include/',
                               '/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.14.sdk/usr/include/'],
                     libraries=['ofstd','oflog','dcmdata','i2d',
                                'dcmimgle','dcmimage','dcmjpeg',
                                'ijg8','ijg12','ijg16','dcmjpls','charls','dcmtls','dcmnet','dcmsr',
                                'cmr','dcmdsig','dcmwlm','dcmqrdb','dcmpstat','dcmrt','dcmiod','dcmfg','dcmseg',
                                'dcmtract','dcmpmap'],
                 library_dirs=[
                     '/tmp/dcmtk-DCMTK-3.6.4/build/lib/',
                               '/usr/local/Cellar/gcc/7.3.0_1/lib/gcc/7',
                               '/usr/local/lib/python3.6/site-packages/numpy/lib/',
                               '/usr/local/lib',
                               '/usr/local/opt/',
                        '/usr/lib/x86_64-linux-gnu/','/home/imgserver/anaconda3/lib/'],
                 extra_compile_args=[  # The g++ (4.8) in Travis needs this
        '-shared','-std=c++11','-g'],
                 depends=depends_libs,
                 runtime_libraties_dirs=['/tmp/dcmtk-DCMTK-3.6.4/build/lib/'],
                 )

setup(
    name="pydcm2png",
    version="0.0.1.8",
    author="dingyuguo",
    author_email="dyg1993@foxmail.com",
    description="Convert DICOM file to PNG file",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/pypa/sampleproject",
    classifiers=[
        "Programming Language :: Python :: 2",
        "License :: OSI Approved :: MIT License",
    ],
    ext_modules=[_dcm2png],
    packages=['pydcm2png'],
    package_dir={'pydcm2png':'pydcm2png/'},
    package_data={'pydcm2png':['_dcm2png.so']},
    requries=['dcmtk'],
)
