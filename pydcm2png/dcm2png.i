%module dcm2png
%{
    #define SWIG_FILE_WITH_INIT
    #include"dcm2png.h"
    extern int  input_file_dcm_to_png(const char * dcm_file,const char * png_file);
    extern unsigned int get_width(const char * dcm_file);
    extern unsigned int get_height(const char * dcm_file);
%}
%include "numpy.i"

%init %{
    import_array();
%}

%apply (unsigned short * ARGOUT_ARRAY1, int DIM1) {( unsigned short * data, int n)}
%include "dcm2png.h"
extern int  input_file_dcm_to_png(const char * dcm_file,const char * png_file);

extern unsigned int get_width(const char * dcm_file);

extern unsigned int get_height(const char * dcm_file);

extern void pixel_data(const char * dcm_file,unsigned short * data,int n);
class DCMOP{
	public:
		DCMOP();
		DCMOP(string file_name);
		~DCMOP();
		inline DicomImage * to_DicomImage(E_TransferSyntax xfer);
	void decode_dcm(E_TransferSyntax xfer);
	bool get_and_display_minmax();
	bool apply_window(int opt_windowType = 0,OFCmdUnsignedInt opt_windowParameter = 1, OFCmdFloat opt_windowCenter = 0.0,OFCmdFloat opt_windowWidth = 0.0);
	void dump_voi_LUT();
	bool open(string file_name);
	int get_window_type(unsigned long window_cnt,unsigned long voi_lut_cnt,bool no_window=false);

		string to_string(int i);

		bool convert_to_image(string output_file="ding_test.png");

		bool save_as_img(const char * opt_ofname,E_FileType opt_fileType = EFT_16bitPNG);/* default: 16-bit PGM/PPM */

		long rows;
		long cols;
		unsigned short channels_num;
	private:
			DcmDataset * _dataset;
			DcmMetaInfo *_meta_info;
			DcmFileFormat * _file_format;
			DicomImage *di;
			E_TransferSyntax xfer;
			OFBool           opt_ignoreVoiLutDepth ;
			OFCmdUnsignedInt    opt_frame;
			OFCmdUnsignedInt    opt_frameCount;
			string _current_dcm_name;
			const uint8_t  * _pixData_u8 ;
			const int16_t  * _pixData_s16;
			bool aleady_open_flag;
			double minVal ;
			double maxVal ;
	
};

