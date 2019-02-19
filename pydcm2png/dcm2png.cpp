/*************************************************************************
    > File Name: dcm2png.cpp
    > Author: laoding
    > Mail: dyg1993@foxmail.com 
    > Created Time: 一  2/ 5 14:22:09 2018
 ************************************************************************/

#include "dcm2png.h"

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

