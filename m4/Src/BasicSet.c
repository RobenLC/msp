/* Includes ------------------------------------------------------------------*/
#include "../Inc/main.h"
//#include "log.h"
#include "arrayed_queue.h"
#include <string.h>

/*
#include "stdafx.h"

#include <stdexcept>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>
#include "CroppingLib.h"
#include <iostream>
#include <string>
#include <windows.h>
#include <Winbase.h>
#include <Pathcch.h>
*/
#include "BasicSet.h"
#include "Image_PreProcess.h"
#include "CFilters.h"
#include "BKNote_PreSet.h"
//#include "ConsoleBankNote.h"

/*
#using <system.drawing.dll>
#using <system.dll>
using namespace System;
using namespace System::IO;
using namespace System::Drawing;
using namespace std;
*/



//free up the allocated memory for the imageIP
void freeImageMem(imageIP *z)
{
	/*      Free the storage associated with the image Z    */

	if (z != 0)
	{
//		for (int i = 0; i<z->iRect->yr; i++)
//			free(z->data[i]);
			free(z->data[0]);
		free(z->iRect);
		free(z->data);
		free(z);
	}
}

void freeImage(imageIP *Img_Ptr)
{
	free(Img_Ptr->iRect);
	free(Img_Ptr->data);
	free(Img_Ptr);

}


void freeSEimage(SElement *z)
{
	/*      Free the storage associated with the image Z    */

	if (z != 0)
	{
		free(z->data[0]);
		free(z->data);
		free(z);
	}
}

// SSElement = (struct SElement  *) malloc(sizeof(struct SSElement));		//allocate memory for SSElement
// Allocate_SElement ( yr,  xc, SElement_Ptr SSElement); //allocate memory for SSElement

SElement *Allocate_SElement (int yr, int xc)
{

	int	i, j;
	
	SElement *q;

// Allocate a structure for the SE //
	q = (SElement *)malloc (sizeof ( SElement));
	if (q == 0)
	{
	  log_err("Can't allocate structuring element.\n");
	  return 0;
	}
	
	//	q->data = (unsigned char **)malloc(sizeof(unsigned char *)*yr); 
		q->data=  (unsigned char **) malloc(sizeof(unsigned char *)*yr );
		if ((q->data==0)) 
		{
			log_err ("Out of storage in SElement.\n");
			return 0;
		}
		/*
	int TempC = xc >> 2;
	if ( xc != (TempC << 2))
		TempC = (TempC + 1) << 2;
	*/
		int TempC = xc;
		//q->data[0] = (unsigned char *)malloc (yr*xc); allocate the whole memory space in one
		q->data[0]=  (unsigned char *) malloc(yr*TempC);
		
		if (q->data[0] == 0)
		{
			log_err("Fail to allocate image memory in SElement.\n");
			return 0;
		}
		
	//copy the starting address of each raw
		for (i=1; i<yr; i++) 
		{
		  q->data[i] = (q->data[i-1] + TempC);		//only for one byte size per pixel
		}

	q->yr = yr;         q->xc = xc;
	q->oyi = 0;      	q->oxj = 0;
	return q;
}

/*struct AlphaB_BlockSet {
	int Char_Num;             	//Number of valid char in the block
	int *VRT_Edges;            	//vertical boundary pairs 
	int *HORZ_Edges;            //Horizontal boundary pairs
	unsigned char VRT_Pairs;   
	unsigned char HORZ_Pairs;   
	unsigned char  Max_Char;    //Max allowed char in the array, the allocated array size is two times of Max_Char +1
	unsigned char  Char_VRT;    //direction of char verical=1 or horizontal=0
};
*/

//due to the source BMP file is rotated by Zebra tool,
//The original point of the image is set at Bottom left for both orignal image and new image
//However, if the sourcre file is in JPG formate, 
//The original point of the image is set at TOP left for both orignal image and new image

//Fetch a block of image from source to Destination
void FetchImageBlock(imageIP *SrcImage, imageIP *DstImage , int NOrgY, int NOrgX)
{
	int	i, j, k, kr;
	unsigned char *SrcPtr, *DstPtr;


		k=0;
		kr = DstImage->iRect->yr + NOrgY;
		for (i = NOrgY; i < kr ; i++)
		{
			SrcPtr =(unsigned char *) (SrcImage->data[i] + NOrgX);
			DstPtr= DstImage->data[k];
			for (j = 0; j< DstImage->iRect->xc; j++)
			{
				*DstPtr = *SrcPtr;
				DstPtr++;
				SrcPtr++;
			}
			k++;
		}


}

//copy BMP file to IMAGEIP data structure
void CopyImageBlock(unsigned char *SrcImage, imageIP *DstImage)
{
	int	i, j, kr, kc, SrcR, SrcC;
	unsigned char *SrcPtr, *DstPtr, *SrcPtrTmp;

		SrcR= DstImage->iRect->yr;
		SrcC= DstImage->iRect->xc;

		SrcPtrTmp = SrcImage;
		
		for (i = 0; i< SrcR; i++)
		{
			SrcPtr=SrcPtrTmp;
			DstPtr= DstImage->data[i];
			for (j = 0; j< SrcC; j++)
			{
				*DstPtr = *SrcPtr;
				DstPtr++;
				SrcPtr++;
			}
			SrcPtrTmp =  SrcPtrTmp + SrcC;
		}
}


//im->data[i][j] = tmp->data[i][j];
//only check the range in the height of SRN_Box
int CountImagePixel(imageIP *SrcImage, ImgRectC *SRN_Box, unsigned char PMaxValue)
{
	int	i, j,PxCount;

	PxCount=0;		
	for (i =SRN_Box->oyi; i < SRN_Box->oyi+SRN_Box->yr; i++)
	{
		for (j = 0; j< SrcImage->iRect->xc; j++)
		{
			if (SrcImage->data[i][j] < PMaxValue)
				PxCount++;
		}
	}
	return PxCount;
}


//this function allocates the memory space for Image data structure but no memory space for image
imageIP *AllocateNewImage(int yr, int xc)
{

	/*		Allocate the image structure	*/
	imageIP *NewImg = ( imageIP  *) malloc(sizeof( imageIP));
	if (NewImg==0)
	{
		log_err("Out of storage in NEWIMAGE.\n");
		return 0;
	}

	/*		Allocate and initialize the header		*/

	NewImg->iRect = ( Imgheader *)malloc(sizeof(Imgheader));
	if ((NewImg->iRect==0))
	{
		log_err("Out of storage in NEWIMAGE.\n");
		return 0;
	}
	NewImg->iRect->yr = yr;
	NewImg->iRect->xc = xc;
	return NewImg;
}

imageIP *AllocateNewImage_2DMem(int yr, int xc, unsigned char *SrcImgData, unsigned char ShareData)
{
	int	i;
	unsigned char *ptr;
	
	/*		Allocate the image structure	*/
	imageIP *NewImg = ( imageIP  *) malloc(sizeof( imageIP));
	if (NewImg==0)
	{
		log_err("Out of Stucture Space in NEWIMAGE_Mem.\n");
		return 0;
	}
	NewImg->iRect = ( Imgheader *) malloc(sizeof( Imgheader));
	if ((NewImg->iRect==0))
	{
		free(NewImg);
		log_err("Out of Info Space in NEWIMAGE_Mem.\n");
		return 0;
	}
	NewImg->data= (unsigned char **) malloc(sizeof(unsigned char *)*yr );
	if ((NewImg->data==0)) 
	{
		free(NewImg->iRect);
		free(NewImg);
		log_err ("Out of storage in NEWIMAGE_Mem.\n");
		return 0;
	}

	if (ShareData==0)
	{		
		NewImg->data[0]=  (unsigned char *) malloc(yr*xc);
		
		if (NewImg->data[0] == 0)
		{
			free(NewImg->iRect);
			free(NewImg);
			log_err("Fail to allocate image memory in NEWIMAGE_Mem.\n");
			return 0;
		}
	}else
		NewImg->data[0]=  SrcImgData;
	
	//copy the starting address of each raw
	for (i=1; i< yr; i++) 
	{
	  NewImg->data[i] = (NewImg->data[i-1] + xc);		//only for one byte size per pixel
	}
	if (ShareData==0)
	{
		memcpy(NewImg->data[0], SrcImgData, yr*xc);
	}
	NewImg->iRect->yr = yr;
	NewImg->iRect->xc = xc;
	return NewImg;
}

//this function allocates the memory space for Image data structure and memory space for image
//the image memory allocation here is always the multiple of 4
imageIP *AllocateNewImage_Mem(int yr, int xc)
{
	int	i;
	unsigned char *ptr;
	
	/*		Allocate the image structure	*/
	imageIP *NewImg = ( imageIP  *) malloc(sizeof( imageIP));
	if (NewImg==0)
	{
		log_err("Out of Stucture Space in NEWIMAGE_Mem.\n");
		return 0;
	}

	/*		Allocate and initialize the header		*/
	NewImg->iRect = ( Imgheader *) malloc(sizeof( Imgheader));
	if ((NewImg->iRect==0))
	{
		free(NewImg);
		log_err("Out of Info Space in NEWIMAGE_Mem.\n");
		return 0;
	}
	NewImg->data=  (unsigned char **) malloc(sizeof(unsigned char *)*yr );
	if ((NewImg->data==0)) 
	{
		free(NewImg->iRect);
		free(NewImg);
		log_err ("Out of storage in NEWIMAGE_Mem.\n");
		return 0;
	}
	
	int TempC = xc >> 2;
	if (xc != (TempC << 2))
		TempC = (TempC + 1) << 2;
	else
		TempC = xc;
	//the memory allocation here is always the multiple of 4


	NewImg->data[0]=  (unsigned char *) malloc(yr*TempC);
	
	if (NewImg->data[0] == 0)
	{
		free(NewImg->iRect);
		free(NewImg->data);
		free(NewImg);
		log_err("Fail to allocate image memory in NEWIMAGE_Mem.\n");
		return 0;
	}
	
//copy the starting address of each raw
	for (i=1; i< yr; i++) 
	{
	  NewImg->data[i] = (NewImg->data[i-1] + TempC);		//only for one byte size per pixel
	}
	NewImg->iRect->yr = yr;
	NewImg->iRect->xc = xc;
	return NewImg;
}

/*
imageIP *ImageFormatP1(t_image_param *img)
{
	imageIP *NewImage;
	int	i;

	NewImage= AllocateNewImage(img->h, img->w);
	
	NewImage->iRect->yr=img->h;
	NewImage->iRect->xc=img->w;
	NewImage->iRect->yr=img->h;
	
	NewImage->data = (unsigned char **)malloc(sizeof(unsigned char *)*img->h);
	NewImage->data[0]= img->data;
	for ( i = 1; i< NewImage->iRect->yr; i++)
	{
		NewImage->data[i] = (NewImage->data[i - 1] + NewImage->iRect->xc);	   //only for one byte size per pixel
	}
	return NewImage;
}
*/
int CopyByte2Int(unsigned char *SrcPtr)
{
return ( *SrcPtr + (*(SrcPtr+1) << 8) + (*(SrcPtr+2) << 16) + (*(SrcPtr+3) <<24));

}

//copy to 4 bytes
void CopyInt2Byte(unsigned char *SrcPtr, int IntData)
{
	*SrcPtr       = IntData;
	*(SrcPtr + 1) = IntData >> 8;
	*(SrcPtr + 2) = IntData >> 16;
	*(SrcPtr + 3) = IntData >> 24;
}
int	GetSizOf_imageIP(void)
{
/*
	int temp, temp2;
	temp = sizeof(t_imageIP);
	temp2 = sizeof(t_Imgheader);

	return(temp + temp2);
*/
	return(sizeof(imageIP) + sizeof(Imgheader));

}


