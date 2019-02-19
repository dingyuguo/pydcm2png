/*************************************************************************
    > File Name: dcm2png.h
    > Author: laoding
    > Mail: dyg1993@foxmail.com 
    > Created Time: 五  1/25 16:35:22 2019
 ************************************************************************/

#include<iostream>
#include<string>
#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmjpeg/djdecode.h"
#include "dcmtk/dcmjpeg/dipijpeg.h"

#include "dcmtk/dcmdata/dctagkey.h"
#include "dcmtk/dcmdata/dcdeftag.h"
#include "dcmtk/config/osconfig.h"  
#include "dcmtk/dcmdata/dctk.h"  
#include "dcmtk/dcmjpeg/djdecode.h"
#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#include "dcmtk/ofstd/ofstdinc.h"
#include "dcmtk/ofstd/ofconapp.h"        /* for OFConsoleApplication */
#include "dcmtk/ofstd/ofcmdln.h"         /* for OFCommandLine */
#include "dcmtk/ofstd/ofstd.h"           /* for OFStandard */

#define INCLUDE_CSTDIO
#define INCLUDE_CSTRING
#include "dcmtk/ofstd/ofstdinc.h"

#include "dcmtk/dcmdata/dctk.h"          /* for various dcmdata headers */
#include "dcmtk/dcmdata/cmdlnarg.h"      /* for prepareCmdLineArgs */
#include "dcmtk/dcmdata/dcuid.h"         /* for dcmtk version name */
#include "dcmtk/dcmdata/dcrledrg.h"      /* for DcmRLEDecoderRegistration */

#include "dcmtk/dcmimgle/dcmimage.h"     /* for DicomImage */
#include "dcmtk/dcmimgle/digsdfn.h"      /* for DiGSDFunction */
#include "dcmtk/dcmimgle/diciefn.h"      /* for DiCIELABFunction */

#include "dcmtk/ofstd/ofconapp.h"        /* for OFConsoleApplication */
#include "dcmtk/ofstd/ofcmdln.h"         /* for OFCommandLine */

#include "dcmtk/dcmimage/diregist.h"     /* include to support color images */
#include "dcmtk/ofstd/ofstd.h"           /* for OFStandard */

#ifdef BUILD_DCM2PNM_AS_DCMJ2PNM
#include "dcmtk/dcmjpeg/djdecode.h"      /* for dcmjpeg decoders */
#include "dcmtk/dcmjpeg/dipijpeg.h"      /* for dcmimage JPEG plugin */
#endif

#ifdef BUILD_DCM2PNM_AS_DCML2PNM
#include "dcmtk/dcmjpls/djdecode.h"      /* for dcmjpls decoders */
#endif

using namespace std;

class DCMOP{
	public:
		DCMOP();
		DCMOP(const char *file_name);
		~DCMOP();
		void to_DicomImage();
		void decode_dcm(E_TransferSyntax xfer);
		bool get_and_display_minmax();
		bool apply_window(int opt_windowType = 0,OFCmdUnsignedInt opt_windowParameter = 1,\
			       	OFCmdFloat opt_windowCenter = 0.0,OFCmdFloat opt_windowWidth = 0.0);
		void dump_voi_LUT();
		bool open(const char * file_name);
		int get_window_type(unsigned long window_cnt,unsigned long voi_lut_cnt,bool no_window=false);
		bool apply_normal_window();
		void * get_pixel_data();
		unsigned long width;
		unsigned long height;
		unsigned short channels_num;
		bool isMonochrome();
	private:
		DcmDataset * _dataset;
		DcmMetaInfo *_meta_info;
		DcmFileFormat * _file_format;
		DicomImage *di;
		E_TransferSyntax xfer;
		OFBool           opt_ignoreVoiLutDepth ;
		OFCmdUnsignedInt    opt_frame;
		OFCmdUnsignedInt    opt_frameCount;
		char * _current_dcm_name;
		bool aleady_open_flag;
		double minVal ;
		double maxVal ;
};

extern void pixel_data(const char * dcm_file,unsigned short* data,int n);

extern unsigned int get_width(const char * dcm_file);

extern unsigned int get_height(const char * dcm_file);
