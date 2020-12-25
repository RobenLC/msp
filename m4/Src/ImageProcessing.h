
//*****************************************************************************
//	Image Job idx
// 
// iJob_idx is attached for the current job or for the chained next job
//*****************************************************************************
#define ImgProc_None             	0
#define NewImageIn             		1
#define ImageProcDone             	2
#define NewAreaReq             		3
#define ChkBox1L             		4
#define ChkBox1R             		5
#define SRNBox1L             		6
#define SRNBox1R             		7
#define ChkBox2L             		8
#define ChkBox2R             		9
#define SRNBox2L             		10
#define SRNBox2R             		11


//*****************************************************************************
//	iJobRtnCode: Image Job idx Status code
// 
//*****************************************************************************
#define iJobOK						0
#define iJobImgInquirey				1
#define iJobImgComplete				2
#define ImageOverSize				3
#define ImageZeroSize				4
#define iJobCurDone					5
#define iJobSaveIntPM				6
#define iJobImgOCR					7 
#define iJobImgMatch				8
#define iJobImgNoMatch				9

#define	BMPfilename_Length			48
#define	DebugBMPNmae				0

#define   APP2SCANNER_Resolu_1200dpi        1
#define   APP2SCANNER_Resolu_600dpi         2
#define   APP2SCANNER_Resolu_300dpi         3
#define   APP2SCANNER_Resolu_200dpi         4
#define   APP2SCANNER_Resolu_150dpi         5
#define   APP2SCANNER_Resolu_100dpi         6
#define   APP2SCANNER_Resolu_75dpi          7

#define         NOLED                   0x00
#define         GREENLED                0x01
#define         BLUELED                 0x02
#define         REDLED                  0x03
#define         C3LED_RGB               0x04
#define         UVLED_R                 0x05    //R: reflection
#define         UVLED_T                 0x06    //T: Transmission
#define         IR1LED_R                0x07
#define         IR1LED_T                0x08
#define         IR2LED_R                0x09
#define         IR2LED_T                0x0A

typedef struct	
{
		unsigned char	BKNote_Layers;			//number of color layer in each page of scan
		unsigned char	LEDMode;				//?
		unsigned char	LEDH2L1;				//if BMP/4_layers, it only use the first 4 LEDs
		unsigned char	LEDH4L3;
		unsigned char	LEDH6L5;
		unsigned char	LEDH8L7;
		unsigned char	SelLayerNum;
}	t_ImageLayers;


#if DebugBMPNmae
typedef struct		
{
	char			BMPfilename[BMPfilename_Length];
    int			 	NoteScan_Resolution; //the resolution used in scan image
	Imgheader		BKNoteImageRect;	//the rectangle information of the BKNote, the orginal point (0,0) is the bottom left cornor.
    int			 	SeqIdx;				//the sequence number during proceesing for the current image file
    int			 	iJobIdx;			//assigned for the image processing job
	Imgheader		ImageRect;			//the rectagle information for the image using in the assigned iJobIdx, valid if width not equal to zero
    int			 	iJobRtnCode;		//The return status code for the iJobIdx
	t_ImageLayers	ImageLayerInfo; 	//Provide the LED information for each layer
    int			 	IpPAMemorySize;		//The size of the attached memory 
    unsigned char  	*AttImgData;    	//usually, pointer to the t_imageIP	if there is an image data attached
}   t_ImageParam;     
#else
typedef struct		
{
	Imgheader		BKNoteImageRect;	//the rectangle information of the BKNote, the orginal point (0,0) is the bottom left cornor.
    int			 	SeqIdx;				//the sequence number during proceesing for the current image file
    int			 	iJobIdx;			//assigned for the image processing job
	Imgheader		ImageRect;			//the rectagle information for the image using in the assigned iJobIdx, valid if width not equal to zero
    int			 	iJobRtnCode;		//The return status code for the iJobIdx
	t_ImageLayers	ImageLayerInfo; 	//Provide the LED information for each layer
    int			 	IpPAMemorySize;		//The size of the attached memory 
    unsigned char  	*AttImgData;    	//usually, pointer to the t_imageIP	if there is an image data attached
}   t_ImageParam;       
#endif

void Clean_SRNImage_Block(imageIP *DstImage);
void Clean_SRNImage_BlockTH(imageIP *DstImage, unsigned char THStage);

//void Marking_IndAlpha_ImgBlk(TCHAR *folder, TCHAR *fn, int DstImgFileSize, unsigned char *SNRImagePtr, IMAGEIP_Ptr DstImage, int HighLine, int LowLine,  AlphaB_Block_Ptr OCRImg_Blk);
void Get_CHKP_NTD_Pos(imageIP *DstImage, Pos_XY *CHKP_Pos, int Match_PixelNum);
void Check_SRNBoxH_NTD(imageIP *DstImage, ImgRectC *SRN_Box, int Match_PixelNum);

void Clean_CHKImage_Block(imageIP *DstImage);
void ImageJobDispatch(t_rjob_cmd *BKJobcmd);
imageIP *RetrieveImgSet (t_ImageParam *SrcBKJob_Param, unsigned char ShareData);
void SetCHKBoxReqArea(t_ImageParam *BKimage_param);
void Duplicate_imageparam(t_ImageParam *SrcImageIP_Param, t_ImageParam *DstImageIP_Param);
imageIP *RetrieveImgSetOrg (t_ImageParam *SrcBKJob_Param);	//should be removed later


unsigned char SelNTDImageLayer(t_ImageLayers *ImgLayerInfo, unsigned char tBillNum);

void cmd_BKCMD_SVAE_PCAREA( t_rjob_cmd *SavePcmd );




