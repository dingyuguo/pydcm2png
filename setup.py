from setuptools import *
import os
import glob

CLASSIFIERS = """\
Development Status :: 4 - Beta
Intended Audience :: Science/Research
Intended Audience :: Developers
License :: OSI Approved
Programming Language :: C++
Programming Language :: Python
Topic :: Software Development
Topic :: Scientific/Engineering
Operating System :: Microsoft :: Windows
Operating System :: POSIX
Operating System :: Unix
Operating System :: MacOS
"""

src_cc = glob.glob('pydcm2png/dcm*/*/*.c*')
src_cc.append('pydcm2png/dcm2png.i')
src_cc.append('pydcm2png/dcm2png.cpp')
src_cc.extend(glob.glob('pydcm2png/of*/*/*.c*'))

include_files = glob.glob('pydcm2png/include/*/*/*.h')
include_files = glob.glob('pydcm2png/include/*/*/*/*.h')
include_files = glob.glob('pydcm2png/include/*/*/*/*/*.h')
print(src_cc)


with open("README.md", "r") as fh:
    long_description = fh.read()
_dcm2png = Extension(name='pydcm2png/_dcm2png',
                 sources=src_cc,
                 language='c++',
                 swig_opts=['-c++'],
                 define_macros=[('oflog_EXPORTS','None')],
                 include_dirs=['/usr/local/lib/python2.7/site-packages/numpy/core/include/',
                               '/usr/local/lib/python3.5/dist-packages/numpy/core/include/',
                               'pydcm2png/include/',
                               'pydcm2png/dcmdata/libsrc/',
                               'pydcm2png/dcmjpeg/libijg8/',
                               'pydcm2png/dcmjpeg/libijg12/',
                               'pydcm2png/dcmjpeg/libijg16/',
                               '/usr/local/include/'],
                 libraries=['z'],
                 library_dirs=[
                               '/usr/local/lib/python3.6/site-packages/numpy/lib/',
                               '/usr/local/lib',
                               '/usr/lib/x86_64-linux-gnu/'],
                 extra_compile_args=[  # The g++ (4.8) in Travis needs this
                               '-shared','-std=c++11','-g'],
                 )

data_file = include_files.append('_dcm2png.so')
data_file = include_files.append('dicom.dic')
setup(
    name="pydcm2png",
    version="0.1.0.1",
    author="dingyuguo",
    author_email="dyg1993@foxmail.com",
    description="Get Device Independent Pixel Data From DICOM File",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com",
    classifiers=filter(None, CLASSIFIERS.split('\n')),
    ext_modules=[_dcm2png],
    packages=['pydcm2png'],
    package_dir={'pydcm2png':'pydcm2png/'},
    package_data={'pydcm2png':['_dcm2png.so','dicom.dic']},
    data_files=data_file,
    install_requires = ['numpy>=1.15.0'],
)
