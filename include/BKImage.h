

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

typedef struct  
{
	int yr;				//Height,y, 	// Rows and columns in the image 
	int	xc;             //Width,x
	int oyi; 			 //y, 		// Origin 
	int	oxj;             //x, 
}	t_Imgheader;

typedef struct  
{
	t_Imgheader 	*imgRect;            	/* Pointer to header */
	unsigned char 	**imgData;           		/* Pixel values */
}	t_imageIP;

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
//
//this data structure is used for passs the image parameter in the Jobcmd.
//the host send the image block information with this data structure
typedef struct		
{
	t_Imgheader		BKNoteImageRect;	//the rectangle information of the BKNote, the orginal point (0,0) is the bottom left cornor.
    int			 	SeqIdx;				//the sequence number during proceesing for the current image file
    int			 	iJobIdx;			//assigned for the image processing job
	t_Imgheader		ImageRect;			//the rectagle information for the image using in the assigned iJobIdx, valid if width not equal to zero
    int			 	iJobRtnCode;		//The return status code for the iJobIdx
	t_ImageLayers	ImageLayerInfo; 	//Provide the LED information for each layer
    int			 	IpPAMemorySize;		//The size of the attached memory 
    unsigned char  	*AttImgData;    	//usually, pointer to the t_imageIP	if there is an image data attached
}   t_ImageParam;       


t_imageIP  *AllocateNewImage(int yr, int xc);
void SetNewImageIP(int yr, int xc, t_imageIP *NewImage_Ptr);
unsigned char *AllocateImageIP_Param( t_ImageParam	*SrcBKJobImg_Param);
unsigned char *AllocateImgParamImgData( t_ImageParam	*SrcBKJobImg_Param);

int CopyByte2Int(unsigned char *SrcPtr);
void CopyInt2Byte(unsigned char *SrcPtr, int IntData);
int	GetSizOf_imageIP(void);
void CopyImage2IPBlock(unsigned char *SrcImage, t_imageIP *DstImage);
void Copy2Blocks(unsigned char *SrcPtrIn, unsigned char *DstPtrIn, int DLength);


