/*************************************************************************
    > File Name: dcm2png.cpp
    > Author: laoding
    > Mail: dyg1993@foxmail.com 
    > Created Time: 一  2/ 5 14:22:09 2018
 ************************************************************************/

#include "dcm2png.h"

char * get_file_without_extension(const char * fs)
{
	//int begin = file_name.find_last_of("/");
	string file_name(fs);
	int end = file_name.find_last_of(".");
	int len  =  end ;
	cout << file_name.substr(0,len) << endl;
	return (char *)(file_name.substr(0,len).c_str());
}

DCMOP::DCMOP(){}

DCMOP::DCMOP(const char * file_name)
{
	width = 0;
	height = 0;
	minVal = 0.0;
	maxVal = 0.0;
	channels_num = 0;
	_current_dcm_name = (char *)file_name;
	di = NULL;
	_file_format = new DcmFileFormat();
	_dataset = NULL;
	_meta_info = NULL;
	aleady_open_flag = false;
	opt_ignoreVoiLutDepth = OFFalse;  /* default: do not ignore VOI LUT bit depth */
	opt_frame = 1;                    /* default: first frame */
	opt_frameCount = 1;               /* default: one frame */
	open(file_name);
}
DCMOP::~DCMOP()
{
	aleady_open_flag = false;
	if(di)
		delete di;
};
void DCMOP::to_DicomImage()
{
	unsigned long	opt_compatibilityMode = CIF_MayDetachPixelData | CIF_TakeOverExternalDataset;
	Sint32 frameCount;
	if (_dataset->findAndGetSint32(DCM_NumberOfFrames, frameCount).bad())
		frameCount = 1;
	if ((opt_frameCount == 0) || ((opt_frame == 1) && (opt_frameCount == OFstatic_cast(Uint32, frameCount))))
	{
		// since we process all frames anyway, decompress the complete pixel data (if required)
		opt_compatibilityMode |= CIF_DecompressCompletePixelData;
	}
	if ((frameCount > 1) && !(opt_compatibilityMode & CIF_DecompressCompletePixelData))
	{
		// use partial read access to pixel data (only in case of multiple frames, but not for all frames)
		opt_compatibilityMode |= CIF_UsePartialAccessToPixelData;
	}
	di = new DicomImage(_file_format,xfer, opt_compatibilityMode, opt_frame - 1, opt_frameCount);
	if(di == NULL)
	{
		cout << "DicomImage: get out of Memory" << endl;
		//return NULL;
	}
	if(di->getStatus() != EIS_Normal)
	{
		cout << "DicomImage status is Bad" << endl;
		//return NULL;
	}
}
void DCMOP::decode_dcm(E_TransferSyntax xfer)
{
	const char*	transferSyntax = NULL;
	_meta_info->findAndGetString(DCM_TransferSyntaxUID, transferSyntax);//获得传输语法字符串
	string losslessTransUID = "1.2.840.10008.1.2.4.70";
	string lossTransUID = "1.2.840.10008.1.2.4.51";
	string losslessP14 = "1.2.840.10008.1.2.4.57";
	string lossyP1 = "1.2.840.10008.1.2.4.50";
	string lossyRLE = "1.2.840.10008.1.2.5";
	if (transferSyntax == losslessTransUID || transferSyntax == lossTransUID || 
			transferSyntax == losslessP14 || transferSyntax == lossyP1)
	{
		DJDecoderRegistration::registerCodecs();
		_dataset->chooseRepresentation(EXS_LittleEndianExplicit, NULL);		//对压缩的图像像素进行解压
		DJDecoderRegistration::cleanup();
	}
	else if (transferSyntax == lossyRLE)
	{
		DcmRLEDecoderRegistration::registerCodecs();
		_dataset->chooseRepresentation(EXS_LittleEndianExplicit, NULL);
		DcmRLEDecoderRegistration::cleanup();
	}
	else
	{
		_dataset->chooseRepresentation(xfer, NULL);
	}
	OFString csetString;
	if (_dataset->findAndGetOFStringArray(DCM_SpecificCharacterSet, csetString).good())
	{
		if (csetString.compare("ISO_IR 192") != 0 && csetString.compare("ISO_IR 6") != 0)
		{
#ifdef DCMTK_ENABLE_CHARSET_CONVERSION
			/* convert all DICOM strings to UTF-8 */
			OFCondition status = _dataset->convertToUTF8();
			if (status.bad())
			{
				cout << "Convert UTF-8 Error" << endl;
			}
#else
			;
#endif
		}
	}
}
bool DCMOP::get_and_display_minmax()
{
	int minmaxValid = di->getMinMaxValues(minVal, maxVal);
	if (minmaxValid)
	{
		char minmaxText[30];
		OFStandard::ftoa(minmaxText, sizeof(minmaxText), maxVal, OFStandard::ftoa_format_f, 0, 0);
		cout <<  "  maximum pixel value : " << minmaxText << endl;
		OFStandard::ftoa(minmaxText, sizeof(minmaxText), minVal, OFStandard::ftoa_format_f, 0, 0);
		cout << "  minimum pixel value : " << minmaxText << endl;
	}
	return true;
}
bool DCMOP::apply_window(int opt_windowType,OFCmdUnsignedInt opt_windowParameter, OFCmdFloat opt_windowCenter ,OFCmdFloat opt_windowWidth )
{
	/*Now , We didnot deal with crop and flip */
	OFCmdUnsignedInt    opt_roiLeft = 0, opt_roiTop = 0, opt_roiWidth = 0, opt_roiHeight = 0;
	/* process VOI parameters */
	switch (opt_windowType)
	{
		case 1: /* use the n-th VOI window from the image file */
			if ((opt_windowParameter < 1) || (opt_windowParameter > di->getWindowCount()))
			{
				cout <<  "cannot select VOI window " << opt_windowParameter << ", only " << di->getWindowCount() << " window(s) in file" << endl;
				return 1;
			}
			if (!di->setWindow(opt_windowParameter - 1))
				cout << "cannot select VOI window " << opt_windowParameter << endl;
#if DCM_DEBUG
			cout << "activating VOI window " << opt_windowParameter << endl;
			double w,c;
			if(di->getWindow(c,w))
				cout << "WW = " << w << "\tWC = " << c << endl;
#endif
			break;
		case 2: /* use the n-th VOI look up table from the image file */
			if ((opt_windowParameter < 1) || (opt_windowParameter > di->getVoiLutCount()))
			{
				cout << "cannot select VOI LUT " << opt_windowParameter << ", only " << di->getVoiLutCount() << " LUT(s) in file" << endl;
				return 1;
			}
#if DCM_DEBUG
			cout << "activating VOI LUT " << opt_windowParameter << endl;
#endif
			if (!di->setVoiLut(opt_windowParameter - 1, !opt_ignoreVoiLutDepth ? ELM_IgnoreValue : ELM_UseValue))
				cout << "cannot select VOI LUT " << opt_windowParameter << endl;
			break;
		case 3: /* Compute VOI window using min-max algorithm */
					cout <<  "activating VOI window min-max algorithm" << endl;
					if (!di->setMinMaxWindow(0))
						cout << "cannot compute min/max VOI window" << endl;
					break;
				case 4: /* Compute VOI window using Histogram algorithm, ignoring n percent */
					cout << "activating VOI window histogram algorithm, ignoring " << opt_windowParameter << "%" << endl;
					if (!di->setHistogramWindow(OFstatic_cast(double, opt_windowParameter)/100.0))
						cout << "cannot compute histogram VOI window" << endl;
					break;
				case 5: /* Compute VOI window using center and width */
					cout <<  "activating VOI window center=" << opt_windowCenter << ", width=" << opt_windowWidth << endl;
					if (!di->setWindow(opt_windowCenter, opt_windowWidth))
						cout << "cannot set VOI window to specified values" << endl;
					break;
				case 6: /* Compute VOI window using min-max algorithm ignoring extremes */
					cout <<  "activating VOI window min-max algorithm, ignoring extreme values" << endl;
					if (!di->setMinMaxWindow(1))
						cout << "cannot compute min/max VOI window" << endl;
					break;
				case 7: /* Compute region of interest VOI window */
					cout << "activating region of interest VOI window" << endl;
					if (!di->setRoiWindow(opt_roiLeft, opt_roiTop, opt_roiWidth, opt_roiHeight))
						cout << "cannot compute region of interest VOI window" << endl;
					break;
				default: /* no VOI windowing */
					if (di->isMonochrome())
					{
						cout <<  "disabling VOI window computation" << endl;
						if (!di->setNoVoiTransformation())
							cout <<  "cannot ignore VOI window" << endl;
					}
					break;
			}
	EF_VoiLutFunction   opt_voiFunction = di->getVoiLutFunction();
	/* VOI LUT function */
	if (opt_voiFunction != EFV_Default)
	{
#if DCM_DEBUG
		if (opt_voiFunction == EFV_Linear)
			cout <<  "setting VOI LUT function to LINEAR" << endl;
		else if (opt_voiFunction == EFV_Sigmoid)
			cout <<  "setting VOI LUT function to SIGMOID" << endl;
#endif
		if (!di->setVoiLutFunction(opt_voiFunction))
			cout << "cannot set VOI LUT function" << endl;
	}
	ES_PresentationLut  opt_presShape = di->getPresentationLutShape();
	/* process presentation LUT parameters */
	if (opt_presShape != ESP_Default)
	{
#if DCM_DEBUG
		if (opt_presShape == ESP_Identity)
			cout << "setting presentation LUT shape to IDENTITY" << endl;
		else if (opt_presShape == ESP_Inverse)
			cout << "setting presentation LUT shape to INVERSE" << endl;
		else if (opt_presShape == ESP_LinOD)
			cout << "setting presentation LUT shape to LIN OD" << endl;
#endif
		if (!di->setPresentationLutShape(opt_presShape))
			cout <<  "cannot set presentation LUT shape" << endl;
	}
	return true;
}
void DCMOP::dump_voi_LUT()
{
	unsigned long count;
	/* dump VOI windows */
	OFString explStr, funcStr;
	count = di->getWindowCount();
	EF_VoiLutFunction   opt_voiFunction = di->getVoiLutFunction();
	switch (opt_voiFunction)
	{
		case EFV_Default:
			funcStr = "<default>";
			break;
		case EFV_Linear:
			funcStr = "LINEAR";
			break;
		case EFV_Sigmoid:
			funcStr = "SIGMOID";
			break;
	}
	cout <<  "  VOI LUT function    : " << funcStr.data()<< endl;
	cout << "  VOI windows in file : " << di->getWindowCount() << endl;
	/* dump VOI LUTs */
	count = di->getVoiLutCount();
	cout <<  "  VOI LUTs in file    : " << count << endl;
	for (int i = 0; i < count; i++)
	{
		if (di->getVoiLutExplanation(i, explStr) == NULL)
			cout << "  - <no explanation>" << endl;
		else
			cout << "  - " << explStr.data() << endl;
	}
	/* dump presentation LUT shape */
	OFString shapeStr;
	ES_PresentationLut  opt_presShape = di->getPresentationLutShape();
	switch (opt_presShape)
	{
		case ESP_Default:
			shapeStr = "<default>";
			break;
		case ESP_Identity:
			shapeStr = "IDENTITY";
			break;
		case ESP_Inverse:
			shapeStr = "INVERSE";
			break;
		case ESP_LinOD:
			shapeStr = "LIN OD";
			break;
	}
	cout << "  presentation shape  : " << shapeStr.data() << endl;
	/* dump overlays */
	cout <<  "  overlays in file    : " << di->getOverlayCount() << endl;
}
bool DCMOP::open(const char * file_name)
{
	_current_dcm_name = (char *)file_name;
	OFCondition status = _file_format->loadFile(file_name);
	if(!status.good())
	{
		cout << "Failed to Load " << file_name << endl;
		exit(0);
	}
	aleady_open_flag = true;
	_dataset = _file_format->getDataset();
	_meta_info = _file_format->getMetaInfo();
	xfer = _dataset->getOriginalXfer();//得到传输语法
	decode_dcm(xfer);
	to_DicomImage();
	width = di->getWidth();
	height = di->getHeight();
	return true;
}
int DCMOP::get_window_type(unsigned long window_cnt,unsigned long voi_lut_cnt,bool no_window)
{
	EF_VoiLutFunction   voi_func = di->getVoiLutFunction();
	int opt_windowType = 0;
	if(voi_lut_cnt > 0 && voi_func == EFV_Sigmoid)
	{
		opt_windowType = 2;/* use the n-th VOI look up table from the image file */
	}
	else if (window_cnt > 0)
	{
		opt_windowType = 1;/* use the n-th VOI window from the image file */
	}
	else
	{
		opt_windowType = 0;/* no VOI windowing */
	}
	opt_windowType = no_window?0:opt_windowType;
	return opt_windowType;
}
char * DCMOP::to_string(int i)
{
	std::stringstream ss;
	ss << i;
	return (char *)(ss.str().c_str());
}
bool DCMOP::apply_normal_window()
{
#if DCM_DEBUG
	get_and_display_minmax();
	dump_voi_LUT();
#endif
	unsigned long window_cnt = di->getWindowCount();
	unsigned long voi_lut_cnt = di->getVoiLutCount();
	int opt_windowType = get_window_type(window_cnt,voi_lut_cnt);
	OFCmdUnsignedInt	opt_windowParameter = 1;
	if(opt_windowType == 2)
	{
		opt_windowType = 2;/* use the n-th VOI look up table from the image file */
		{
			apply_window(opt_windowType,opt_windowParameter);
		}
	}
	else if (opt_windowType == 1)
	{
		opt_windowType = 1; /* use the n-th VOI window from the image file */
		{
			apply_window(opt_windowType,opt_windowParameter);
		}
	}
	else
	{
		opt_windowType = 0;/* no VOI windowing */
		apply_window(opt_windowType,opt_windowParameter);
	}
	return true;
}
bool DCMOP::convert_to_image(string  output)
{
	const char * output_file = output.c_str();
	get_and_display_minmax();
	dump_voi_LUT();
	unsigned long window_cnt = di->getWindowCount();
	unsigned long voi_lut_cnt = di->getVoiLutCount();
	int opt_windowType = get_window_type(window_cnt,voi_lut_cnt);
	OFCmdUnsignedInt	opt_windowParameter = 0;
	if(opt_windowType == 2)
	{
		opt_windowType = 2;/* use the n-th VOI look up table from the image file */
		for(int i = 0; i < voi_lut_cnt; i++)
		{
			opt_windowParameter = i+1;
			OFString explStr;
			string post_fix;
			if (di->getVoiLutExplanation(i, explStr) == NULL)
				post_fix = to_string(i);
			else
				post_fix = string(explStr.c_str());
			string _output_file = string(get_file_without_extension(output_file))+"_"+post_fix+".png";
			apply_window(opt_windowType,opt_windowParameter);
			save_as_img(_output_file.c_str());
		}
	}
	else if (opt_windowType == 1)
	{
		opt_windowType = 1; /* use the n-th VOI window from the image file */
		for(int i = 0; i < window_cnt; i++)
		{
			opt_windowParameter = i+1;
			string _output_file = string(get_file_without_extension(output_file))+"_"+to_string(i)+".png";
			apply_window(opt_windowType,opt_windowParameter);
			save_as_img(_output_file.c_str());
		}
	}
	else
	{
		opt_windowType = 0;/* no VOI windowing */
		apply_window(opt_windowType,opt_windowParameter);
		save_as_img(output_file);
	}
	return true;
}
bool DCMOP::save_as_img(const char * opt_ofname,E_FileType opt_fileType)/* default: 16-bit PGM/PPM */
{
#if SAVE_PNG
	/* write selected frame(s) to file */
	int result = 0;
	FILE *ofile = NULL;
	OFString ofname;
	unsigned int fcount = OFstatic_cast(unsigned int, ((opt_frameCount > 0) && (opt_frameCount <= di->getFrameCount())) ? opt_frameCount : di->getFrameCount());
	const char *ofext = NULL;
	/* determine default file extension */
	switch (opt_fileType)
	{
		case EFT_BMP:
		case EFT_8bitBMP:
		case EFT_24bitBMP:
		case EFT_32bitBMP:
			ofext = "bmp";
			break;
		case EFT_JPEG:
			ofext = "jpg";
			break;
		case EFT_TIFF:
			ofext = "tif";
			break;
		case EFT_PNG:
		case EFT_16bitPNG:
			ofext = "png";
			break;
		default:
			if (di->isMonochrome()) ofext = "pgm"; else ofext = "ppm";
			break;
	}
	if (fcount < opt_frameCount)
	{
		cout <<  "cannot select " << opt_frameCount << " frames, limiting to " << fcount << " frames" << endl;
	}
	OFBool              opt_multiFrame = OFFalse;         /* default: no multiframes */
	OFBool              opt_useFrameNumber = OFFalse;     /* default: use frame counter */
	for (unsigned int frame = 0; frame < fcount; frame++)
	{
		if (opt_ofname)
		{
			/* output to file */
			if (opt_multiFrame)
			{
				OFOStringStream stream;
				/* generate output filename */
				stream << opt_ofname << ".";
				if (opt_useFrameNumber)
					stream << "f" << (opt_frame + frame);
				else
					stream << frame;
				stream << "." << ofext << OFStringStream_ends;
				/* convert string stream into a character string */
				OFSTRINGSTREAM_GETSTR(stream, buffer_str)
					ofname.assign(buffer_str);
				OFSTRINGSTREAM_FREESTR(buffer_str)
			}else
				ofname.assign(opt_ofname);
			cout << "writing frame " << (opt_frame + frame) << " to " << ofname.data() << endl;
			ofile = fopen(ofname.c_str(), "wb");
			if (ofile == NULL)
			{
				cout <<  "cannot create file " << ofname.data() << endl;
				return 1;
			}
		} 
		else 
		{
			/* output to stdout */
			ofile = stdout;
			cout << "writing frame " << (opt_frame + frame) << " to stdout" << endl;;
		}
		/* finally create output image file */
		OFCmdUnsignedInt    opt_fileBits = 0;                 /* default: 0 */
#ifdef WITH_LIBTIFF
		// TIFF parameters
		// #ifdef HAVE_LIBTIFF_LZW_COMPRESSION
		DiTIFFCompression   opt_tiffCompression = E_tiffLZWCompression;
#else
		DiTIFFCompression   opt_tiffCompression = E_tiffPackBitsCompression;
#endif
		DiTIFFLZWPredictor  opt_lzwPredictor = E_tiffLZWPredictorDefault;
		OFCmdUnsignedInt    opt_rowsPerStrip = 0;
#ifdef WITH_LIBPNG
		// PNG parameters
		DiPNGInterlace      opt_interlace = E_pngInterlaceAdam7;
		DiPNGMetainfo       opt_metainfo  = E_pngFileMetainfo;
#endif
		switch (opt_fileType)
		{
			case EFT_RawPNM:
				result = di->writeRawPPM(ofile, 8, frame);
				break;
			case EFT_8bitPNM:
				result = di->writePPM(ofile, 8, frame);
				break;
			case EFT_16bitPNM:
				result = di->writePPM(ofile, 16, frame);
				break;
			case EFT_NbitPNM:
				result = di->writePPM(ofile, OFstatic_cast(int, opt_fileBits), frame);
				break;
			case EFT_BMP:
				result = di->writeBMP(ofile, 0, frame);
				break;
			case EFT_8bitBMP:
				result = di->writeBMP(ofile, 8, frame);
				break;
			case EFT_24bitBMP:
				result = di->writeBMP(ofile, 24, frame);
				break;
			case EFT_32bitBMP:
				result = di->writeBMP(ofile, 32, frame);
				break;
#ifdef BUILD_DCM2PNM_AS_DCMJ2PNM
			case EFT_JPEG:
				{
					/* initialize JPEG plugin */
					DiJPEGPlugin plugin;
					plugin.setQuality(OFstatic_cast(unsigned int, opt_quality));
					plugin.setSampling(opt_sampling);
					result = di->writePluginFormat(&plugin, ofile, frame);
				}
				break;
#endif
#ifdef WITH_LIBTIFF
			case EFT_TIFF:
				{
					/* initialize TIFF plugin */
					DiTIFFPlugin tiffPlugin;
                    tiffPlugin.setCompressionType(opt_tiffCompression);
                    tiffPlugin.setLZWPredictor(opt_lzwPredictor);
                    tiffPlugin.setRowsPerStrip(OFstatic_cast(unsigned long, opt_rowsPerStrip));
					result = di->writePluginFormat(&tiffPlugin, ofile, frame);
				}
				break;
#endif
#ifdef WITH_LIBPNG
			case EFT_PNG:
			case EFT_16bitPNG:
				{
					/* initialize PNG plugin */
					DiPNGPlugin pngPlugin;
					pngPlugin.setInterlaceType(opt_interlace);
					pngPlugin.setMetainfoType(opt_metainfo);
					if (opt_fileType == EFT_16bitPNG)
						pngPlugin.setBitsPerSample(16);
					result = di->writePluginFormat(&pngPlugin, ofile, frame);
				}
				break;
#endif
#ifdef PASTEL_COLOR_OUTPUT
			case EFT_PastelPNM:
				result = di->writePPM(ofile, MI_PastelColor, frame);
				break;
#endif
			default:
				if (opt_ofname)
					result = di->writeRawPPM(ofile, 8, frame);
				else /* stdout */
					result = di->writePPM(ofile, 8, frame);
				break;
		}
		if (opt_ofname)
			fclose(ofile);
		if (!result)
		{
			cout <<  "cannot write frame" << endl;
				return false;
		}
	}
#endif
	return true;
}
void * DCMOP::get_pixel_data()
{
	unsigned long width = di->getWidth();
	unsigned long height = di->getHeight();
	const int bit_depth = 16;
	const unsigned long frame = 0;
	const int planar = 0;
	const void * data = di->getOutputData(bit_depth,frame,planar);
	uint16_t *  row_ptr = (uint16_t *)data;
	int max = 0;
	int min = 0;
	if(data!=NULL)
	{
		for(int i =0; i < width*height;i++)
		{
			max = max > *(row_ptr+i)?max:*(row_ptr+i);
			min = min < *(row_ptr+i)?min:*(row_ptr+i);
		}
	}
	else
	{
		cout << "data is NULL " << endl;
	}
#if DCM_DEBUG
	cout << "data size = "<< di->getOutputDataSize() << endl;
	cout << "rows = " << height << endl;
	cout << "cols = " << width << endl;
	cout << "depth = " << di->getDepth() << endl;
	cout << "frame_cnt = " << di->getFrameCount() << endl;
	printf("max = %d\n", max);
	printf("min = %d\n", min);
#endif
	return (void *)data;
}

unsigned int get_width(const char * dcm_file)
{
	DCMOP dcm_op(dcm_file);
	return (unsigned int)dcm_op.width;
}

unsigned int get_height(const char * dcm_file)
{
	DCMOP dcm_op(dcm_file);
	return (unsigned int)dcm_op.height;
}

int  input_file_dcm_to_png(const char * dcm_file,const char * png_file)
{
	DCMOP dcm_op(dcm_file);
	dcm_op.convert_to_image(png_file);
	dcm_op.get_pixel_data();
	return 0;
}

bool DCMOP::isMonochrome()
{
	return di->isMonochrome();
}
void pixel_data(const char * dcm_file,unsigned short * data,int n)
{

	DCMOP dcm_op(dcm_file);
	dcm_op.apply_normal_window();
	uint16_t * pixel_data =(uint16_t *)dcm_op.get_pixel_data();
	int bpp = 2;
	if(dcm_op.isMonochrome())
		bpp = 2;
	else
	{
		cout << "Warnning: Not gray mode, RGB mode" << endl;
		bpp = 6;
	}
	memmove(data,pixel_data,n*bpp);
}
int main()
{
	input_file_dcm_to_png("/Users/ding/my_git/hua_jian/dcm2png/src/10/fujian_without_dealing/fujian.dcm","hello.png");
	//input_file_dcm_to_png("/Users/ding/my_git/hua_jian/dcm2png/src/10/10_9.dcm","hello.png");
	return 0;
}

