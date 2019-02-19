%module dcm2png
%{
    #define SWIG_FILE_WITH_INIT
    #include"dcm2png.h"
    extern unsigned int get_width(const char * dcm_file);
    extern unsigned int get_height(const char * dcm_file);
%}
%include "numpy.i"

%init %{
    import_array();
%}

%apply (unsigned short * ARGOUT_ARRAY1, int DIM1) {( unsigned short * data, int n)}
%include "dcm2png.h"

extern unsigned int get_width(const char * dcm_file);
extern unsigned int get_height(const char * dcm_file);
extern void pixel_data(const char * dcm_file,unsigned short * data,int n);


