

//*****************************************************************************
//	Image Job idx
// 
// iJob_idx is attached for the current job or for the chained next job
//*****************************************************************************
#define ImgProc_None             	0
#define NewImageIn             		1
#define NewAreaReq             		2
#define ImageProcDone             	3
#define ChkBox1             		4
#define ChkBox2             		5
#define SRNBox1             		6
#define SRNBox2             		7


//*****************************************************************************
//	Image Job idx Status code
// 
// iJob_idx is attached for the current job or for the chained next job
//*****************************************************************************
#define iJobOK						0
#define iJobImgInquirey				1
#define iJobImgComplete				2
#define ImageOverSize				3
#define ImageZeroSize				4


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

