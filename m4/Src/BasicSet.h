
#include "main.h"

#define BLACK 0
//#define WHITE 1
#define WHITE 255

typedef struct  
{
	int yr;				//Height,y, 	// Rows and columns in the image 
	int	xc;             //Width,x
	int oyi; 			//y, 		// Origin 
	int	oxj;             //x, 
}	ImgRectC;

typedef struct  
{
	int yr;				//Height,y, 	// Rows and columns in the image 
	int	xc;             //Width,x
	int oyi; 			//y, 		// Origin 
	int	oxj;             //x, 
}	Imgheader;


/*      The IMAGE data structure        */
typedef  struct  {
		Imgheader *iRect;            	/* Pointer to header */
		unsigned char **data;           	/* Pixel values */
}	imageIP;

typedef  struct  {
	unsigned char **data;
	int yr, xc, oyi, oxj;
}	SElement;


//typedef struct imageIP *IMAGEIP_Ptr;
//typedef struct SElement *SElement_Ptr;


void freeImage(imageIP *z);
void freeImageMem(imageIP *Img_Ptr);
void freeSEimage(SElement *z);

SElement *Allocate_SElement (int yr, int xc);
imageIP *AllocateNewImage(int yr, int xc);
imageIP *AllocateNewImage_Mem(int yr, int xc);
imageIP *AllocateNewImage_2DMem(int yr, int xc, unsigned char *SrcImgData, unsigned char ShareData);

void CopyImageBlock(unsigned char *SrcImage, imageIP *DstImage);
void FetchImageBlock(imageIP *SrcImage, imageIP *DstImage, int NOrgY, int NOrgX);
//imageIP *ImageFormatP1(t_image_param *img);
int CountImagePixel(imageIP *SrcImage, ImgRectC *SRN_Box, unsigned char PMaxValue);

int CopyByte2Int(unsigned char *SrcPtr);
//IMAGEIP_Ptr AllocateNewImage(int yr, int xc);
void CopyInt2Byte(unsigned char *SrcPtr, int IntData);		//4 bytes
int	GetSizOf_imageIP(void);



