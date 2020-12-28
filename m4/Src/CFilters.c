/* Includes ------------------------------------------------------------------*/
#include "../Inc/main.h"
//#include "log.h"
#include "arrayed_queue.h"
#include <string.h>


//////////////////////////////////
/*
#include "stdafx.h"
#include <iostream>
#include <windows.h>
*/
/*
//#include <opencv2/opencv.hpp>
//#include "opencv2/imgproc.hpp"
//#include "opencv2/imgcodecs.hpp"
//#include "opencv2/highgui.hpp"

#include "ImageCV_Main.hpp"
#include "ImageCV_Samples.hpp"
*/
//#include "morph.h"

/*
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
//////////////////////////////////
#include "BasicSet.h"
#include "Image_PreProcess.h"
#include "CFilters.h"
#include "BKNote_PreSet.h"
//#include "ConsoleBankNote.h"



//using namespace std;

//using namespace cv;
/*
#define SElement3x3_Cross			1					
#define SElement3x3_Cross_Erode1	2					
#define SElement3x3_Cross_Erode2	3					
#define SElement3x3_AllBlack		4					
#define SElement3x3_Cross			5						
*/
typedef unsigned char SElement4x4[4][4];

const SElement4x4 SElement4x4_Cross1 =
{
	{ 255, 0, 255, 0},	
	{ 0, 255, 0, 255 }, 	
	{ 255, 0, 255,0 }, 
	{0, 255, 0, 255 } 
};
const SElement4x4 SElement4x4_AllBlack =
{
	{ 0, 255, 255, 0 },	
	{ 255, 0, 0, 255}, 	
	{ 255, 0, 0, 255 }, 
	{ 0, 255, 255, 0 } 
};
	

typedef unsigned char SElement3x3[3][3];

const SElement3x3 SElement3x3_Cross =
{
	{ 255, 0, 255 },	
	{ 0, 0, 0 }, 	
	{ 255, 0, 255 } 
};

const SElement3x3 SElement3x3_Cross_Erode1 =
{
		{ 0, 255, 0 },	
		{ 255, 0, 255 }, 	
		{ 0, 255, 0 } 
};

const SElement3x3 SElement3x3_Cross_Erode2 =
{
	{ 255, 255, 255 },	
	{ 255, 0, 255 }, 	
	{ 255, 255, 255 } 
};
	
const SElement3x3 SElement3x3_AllBlack =
{
	{ 0, 0, 0 },	
	{ 0, 0, 0 }, //cross all black
	{ 0, 0, 0 } 
};
/*
{ 0, 1, 0 },	
{ 1, 1, 1 }, 
{ 0, 1, 0 } 

{ 255, 255, 255 },	
{ 255, 255, 255 }, 
{ 255, 255, 255 } 
*/

typedef unsigned char SElement2x2[2][2];

const SElement2x2 SElement2x2_AllBlack =
{
	{ 0, 0 },	
	{ 0, 0 } 
};

const SElement2x2 SElement2x2_TLBR =
{
	{ 0, 255 },	
	{ 255, 0 } 
};

const SElement2x2 SElement2x2_TRBL =
{
	{ 255, 0 },	
	{ 0, 255} 
};

const SElement2x2 SElement2x2_BKTL =
{
	{ 0, 0 },	
	{ 255, 255} 
};

const SElement2x2 SElement2x2_WHBR =
{
	{ 0, 0 },	
	{ 0, 255} 
};

const  SElement4x4 *SElement4x4_Table[] = {
	0, 												
	&SElement4x4_Cross1, 		  						
	&SElement4x4_Cross1, 					
	&SElement4x4_Cross1,	  				
	&SElement4x4_AllBlack, 		  			
	&SElement4x4_Cross1			  			
};

const  SElement3x3 *SElement3x3_Table[] = {
	0, 												
	&SElement3x3_Cross, 		  						
	&SElement3x3_Cross_Erode1, 					
	&SElement3x3_Cross_Erode2,	  				
	&SElement3x3_AllBlack, 		  			
	&SElement3x3_Cross			  			
};

const  SElement2x2 *SElement2x2_Table[] = {
	0, 											
	&SElement2x2_AllBlack, 		 						
	&SElement2x2_TLBR, 					
	&SElement2x2_TRBL,	  	
	&SElement2x2_BKTL, 		  			
	&SElement2x2_WHBR			   		
};

void Get_4x4SElement(SElement *T_SElement, int SE_Number)
{
unsigned char *Tmp_Se,*T_SEl_data;

		Tmp_Se= (unsigned char *)SElement4x4_Table[SE_Number];
		for (int i=0; i<4; i++)
		{
			T_SEl_data=T_SElement->data[i];
			for (int j=0; j<4; j++)
			{
				*T_SEl_data=*Tmp_Se;
				T_SEl_data++;
				Tmp_Se++;
			}
		}
}

void Get_3x3SElement(SElement *T_SElement, int SE_Number)
{
unsigned char *Tmp_Se,*T_SEl_data;

		Tmp_Se= (unsigned char *)SElement3x3_Table[SE_Number];
		for (int i=0; i<3; i++)
		{
			T_SEl_data=T_SElement->data[i];
			for (int j=0; j<3; j++)
			{
				*T_SEl_data=*Tmp_Se;
				T_SEl_data++;
				Tmp_Se++;
			}
		}
}

void Get_2x2SElement(SElement *T_SElement, int SE_Number)
{
unsigned char *Tmp_Se,*T_SEl_data;

		Tmp_Se= (unsigned char *)SElement2x2_Table[SE_Number];
		for (int i=0; i<2; i++)
		{
			T_SEl_data=T_SElement->data[i];
			for (int j=0; j<2; j++)
			{
				*T_SEl_data=*Tmp_Se;
				T_SEl_data++;
				Tmp_Se++;
			}
		}
}



/*      Check that a pixel index is in range. Return TRUE(1) if so.     */

int range(imageIP *im, int i, int j)
{
	if ((i<0) || (i >= im->iRect->yr)) return 0;
	if ((j<0) || (j >= im->iRect->xc)) return 0;
	return 1;
}


//It needs to expand board of the image before use this function.
//this funcation only do grayscale 8 bits for now.
//original point is at the top left cornor.
//void Convol3x3(Mat *XImg, unsigned char *KBlock)
void Convol3x3(unsigned char *XImg, int yr, int xc, int *KElement)
{
//	int	yr, xc, i, j, k;
	int	 i, j, k;
	unsigned char *dataPtr, *KBlock, *TempPtr;
	int	DA0, DA1, DA2;
	int	DB0, DB1, DB2;
	int	DC0, DC1, DC2;
	
//	yr = XImg->rows;
//	xc = XImg->cols;
//	dataPtr = (unsigned char *)(&XImg->data[0, 0]);
	dataPtr = XImg;
	/*
	KBlock = (unsigned char *)(&KElement->data[0, 0]);
	DA0 = (int)*KBlock;
	DA1 = (int)*(KBlock+1);
	DA2 = (int)*(KBlock+2);
	DB0 = (int)*(KBlock+3);
	DB1 = (int)*(KBlock+4);
	DB2 = (int)*(KBlock+5);
	DC0 = (int)*(KBlock+6);
	DC1 = (int)*(KBlock+7);
	DC2 = (int)*(KBlock+8);
	*/
	for (i = 2; i < yr; i++) {
		for (j = 2; j < xc; j++) {
		 /*
		 k	= (KElement[0] * ((int)*(dataPtr)) + KElement[1] * ((int)*(dataPtr+1)) + KElement[2]  * ((int)*(dataPtr + 2))
						+ KElement[3] * ((int)*(dataPtr+xc)) + KElement[4] * ((int)*(dataPtr+xc+1)) + KElement[5]  * ((int)*(dataPtr+xc + 2))
						+ KElement[6] * ((int)*(dataPtr+2*xc)) + KElement[7] * ((int)*(dataPtr+2*xc+1)) + KElement[8]  * ((int)*(dataPtr+2*xc + 2)) );
		 */
			TempPtr = dataPtr + xc + 1;
			DA0 = (int)KElement[0] *	((int)*(dataPtr));
			DA1 = (int)KElement[1] *	((int)*(dataPtr + 1));
			DA2 = (int)KElement[2] *	((int)*(dataPtr + 2));
			DB0 = (int)KElement[3] *	((int)*(dataPtr + xc));
//			DB1 = (int)KElement[4] *	((int)*(dataPtr + xc + 1));
			DB1 = (int)KElement[4] * ((int)*TempPtr);
			DB2 = (int)KElement[5] *	((int)*(dataPtr + xc + 2));
			DC0 = (int)KElement[6] *	((int)*(dataPtr + 2 * xc));
			DC1 = (int)KElement[7] *	((int)*(dataPtr + 2 * xc + 1));
			DC2 = (int)KElement[8] *	((int)*(dataPtr + 2 * xc + 2));
			k = DA0 + DA1 + DA2 + DB0 + DB1 + DB2 + DC0 + DC1 + DC2;
		if (k < 0)
			 *(TempPtr) = 0;
		else if (k > 255)
			*(TempPtr) = 255;
		else
			*(dataPtr + xc + 1) = (unsigned char) k;

		dataPtr+=1;
			}
		dataPtr+=3;		
	}

}

/*      Apply a dilation step on one pixel of IM, reult to RES  */

void dil_apply(imageIP *im, SElement *p, int yi, int xj, imageIP *res)
{
	int i, j, is, js, ie, je;
	unsigned char *imAdress, *pAdress;
	unsigned char k;

	/* Find start and end pixel in IM */
	is = yi - p->oyi;        js = xj - p->oxj;
	ie = is + p->yr;        je = js + p->xc;

	/* Place SE over the image from (is,js) to (ie,je). Set pixels in RES
	if the corresponding SE pixel is 1; else do nothing.         */
	for (i = is; i<ie; i++)
		for (j = js; j<je; j++)
		{
			if (range(im, i, j))
			{
				k = (unsigned char) p->data[i - is][j - js];
				//if (k >= 0) res->data[i][j] |= k;
				imAdress = im->data[i];
				pAdress = res->data[i];
//			if (k = (unsigned char)BLACK) res->data[i][j] = (unsigned char)BLACK;
			if (k == (unsigned char)BLACK) 
				res->data[i][j] = (unsigned char)BLACK;
			}
		}
}



int bin_dilate(imageIP *im, SElement *p)
{
	imageIP *tmp;
//	unsigned char *SrcPtr, *DstPtr, *SrcPtrTemp;
	int i,j;
	unsigned char TData;
	
//	log_info(" bin_dilate In\n ");

/* Source image empty? */
	if (im==0)
	{
	  log_dbg ("Bad image in BIN_DILATE\n");
	  return 0;
	}

/* Create a result image */
	tmp = (imageIP *)AllocateNewImage_Mem(im->iRect->yr, im->iRect->xc);
	
	if (tmp == 0)
		log_err ("No memory in BIN_DILATE\n");
//	  max_abort (0, "Out of memory in Dilate.");
	for (i=0; i<tmp->iRect->yr; i++)
	  for (j=0; j<tmp->iRect->xc; j++)
	    tmp->data[i][j] = (unsigned char)WHITE;

//	log_info(" Apply bin_dilate\n ");

/* Apply the SE to each black pixel of the input */
	for (i=0; i<im->iRect->yr; i++)
		for (j = 0; j < im->iRect->xc; j++)
		{
			TData = im->data[i][j];
			if (TData == (unsigned char)BLACK)
				dil_apply(im, p, i, j, tmp);
		}

/* Copy result over the input */
	for (i=0; i<im->iRect->yr; i++)
	  for (j=0; j<im->iRect->xc; j++)
	    im->data[i][j] = tmp->data[i][j];

/* Free the result image - it was a temp */
	freeImageMem(tmp);
	return 1;
}



/*      Apply a erosion step on one pixel of IM, reult to RES   */

void erode_apply(imageIP *im, SElement *p, int yi, int xj, imageIP *res)
{
	int i, j, is, js, ie, je;
	unsigned char k, r;

	/* Find start and end pixel in IM */
	is = yi - p->oyi;        js = xj - p->oxj;
	ie = is + p->yr;        je = js + p->xc;
	if (ie > im->iRect->yr)
		ie = im->iRect->yr;
	if (je > im->iRect->xc)
		je = im->iRect->xc;
	
	r=(unsigned char)WHITE;
	/*  log_info ("SE is: Origin (%d,%d) size (%d,%d)\n", p->oyi,p->oxj,p->yr,p->xc);
	log_info ("Start at image pixel (%d,%d)\n", ii, jj);          */

	/* Place SE over the image from (is,js) to (ie,je). Set pixels in RES
	if the corresponding pixels in the image agree.      */
	for (i = is; i<ie; i++)
	{
		for (j = js; j<je; j++)
		{
			r = (unsigned char)WHITE;
			k = (unsigned char)p->data[i - is][j - js];
			if (range(im, i, j))
			{
				if ((k == (unsigned char)BLACK) && (im->data[i][j] ==(unsigned char) BLACK)) 
					r = (unsigned char) BLACK;
				/*              log_info ("%3d ", im->data[i-is][j-js]);             */
				else if (k == (unsigned char) WHITE)
					r = (unsigned char) WHITE;
			}
		}
		/*          log_info ("\n");         */
	}
	res->data[yi][xj] = (unsigned char)r;
}


int bin_erode(imageIP *im, SElement *p)
{
	imageIP *tmp;
//	unsigned char *Temp_CharPtr, *Temp_CharPtr1;
	int i, j;
	unsigned char TData;

	/* Source image empty? */
	if (im == 0)
	{
		log_dbg("Bad image in BIN_ERODE\n");
		return 0;
	}

	/* Create a result image */
	tmp = (imageIP *)AllocateNewImage_Mem(im->iRect->yr, im->iRect->xc);
	
	if (tmp == 0)
		log_err ("No memory in bin_erode\n");
///////
	
	for (i = 0; i<tmp->iRect->yr; i++)
		for (j = 0; j<tmp->iRect->xc; j++)
			tmp->data[i][j] = (unsigned char)WHITE;
	
	/* Apply the SE to each black pixel of the input */
	for (i = 0; i<im->iRect->yr; i++)
		for (j = 0; j<im->iRect->xc; j++)
		{
			TData = im->data[i][j];
			if (TData== (unsigned char)BLACK)
				erode_apply(im, p, i, j, tmp);
			}

	/* Copy result over the input */
	for (i = 0; i<im->iRect->yr; i++)
		{
			for (j = 0; j<im->iRect->xc; j++)
				im->data[i][j] = tmp->data[i][j];
		}
	/* Free the result image - it was a temp */
		freeImageMem(tmp);
		return 1;
}

/*
int CopyByte2Int(unsigned char *SrcPtr)
{
return ( *SrcPtr + (*SrcPtr << 8) + (*SrcPtr <<16) + (*SrcPtr <<24));

}
*/
/*
int get_Newse (SElement_Ptr p)
{


}
*/
//not used
SElement *AllocateNewSE(int yr, int xc)
{

	//		Allocate the image structure	*/
	SElement *NewSE = (SElement *) malloc(sizeof( SElement));
	if (NewSE==0)
	{
		log_err("Out of storage in NewSE.\n");
		return 0;
	}

	/*		Allocate and initialize the header		*/

	NewSE->yr = yr;
	NewSE->xc = xc;
	NewSE->data = (unsigned char **)malloc(xc* yr);
	if (!(NewSE->data))
	{
		log_err("Out of storage in New SE.\n");
		return 0;
	}
	return NewSE;
}
//
/* 
int get_se (char *filename, SE *p)
{
	SE q;
	IMAGE_P seim;

	*p = (SE)0;

// Read the PBM format structuring element file //
	if (read_pbm (filename, &seim) == 0) return 0;

//Allocate a structure for the SE //
	q = (SE)malloc (sizeof (struct se_struct));
	if (q == 0)
	{
	  log_info ("Can't allocate structuring element in GET_SE.\n");
	  return 0;
	}
	q->yr = seim->iRect->yr;         q->xc = seim->iRect->xc;
	q->oyi = PBM_SE_ORIGIN_ROW;      q->oxj = PBM_SE_ORIGIN_COL;
	q->data = seim->data;
	free (seim->iRect);
	seim->data = 0;
	free (seim);
	*p = q;
	return 1;
}
*/

