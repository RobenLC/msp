/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "log.h"
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
//#include "BMPFile_Set.h"
#include "Image_PreProcess.h"
#include "CFilters.h"
#include "BKNote_PreSet.h"
//#include "ConsoleBankNote.h"

float w(float *p, int k)
{
	int i;
	float x = 0.0;

	for (i = 1; i <= k; i++) x += p[i];
	return x;
}

float u(float *p, int k)
{
	int i;
	float x = 0.0;

	for (i = 1; i <= k; i++) x += (float)i*p[i];
	return x;
}

float nu(float *p, int k, float ut, float vt)
{
	float x, y;

	y = w(p, k);
	x = ut*y - u(p, k);
	x = x*x;
	y = y*(1.0F - y);
	if (y>0) x = x / y;
	else x = 0.0;
	return x / vt;
}


int thr_glh(imageIP *NewImg, unsigned char Avg2Peak)
{
	/*	Threshold selection using grey level histograms. SMC-9 No 1 Jan 1979
	N. Otsu							*/

	int i, j, k, n, m, h[260], t, tt;
	float y, z, p[260];
	unsigned char *pp;
	float ut, vt;

	n = NewImg->iRect->yr*NewImg->iRect->xc;

//	log_info("Image row is %d\n", NewImg->iRect->yr);
//	log_info("Image col is %d\n", NewImg->iRect->xc);
//	log_info("Image Size is %d\n", n);
	for (i = 0; i<260; i++) {		/* Zero the histograms	*/
		h[i] = 0;
		p[i] = 0.0;
	}
//	unsigned char *NewImgaData_Ptr;
//	NewImgaData_Ptr =( unsigned char *)NewImg->data;
	/* Accumulate a histogram */
	for (i = 0; i<NewImg->iRect->yr; i++)
		for (j = 0; j<NewImg->iRect->xc; j++) {
			k = NewImg->data[i][j];		//pointer to pointer!!!!
//			k = *NewImgaData_Ptr;		//pointer to pointer!!!!
//			NewImgaData_Ptr++;
			h[k + 1] += 1;
		}

	for (i = 1; i <= 256; i++)		/* Normalize into a distribution */
		p[i] = (float)h[i] / (float)n;

	ut = u(p, 256);		/* Global mean */
	vt = 0.0;		/* Global Variance */
	for (i = 1; i <= 256; i++)
		vt += (i - ut)*(i - ut)*p[i];

	j = -1; k = -1;
	for (i = 1; i <= 256; i++) {
		if ((j<0) && (p[i] > 0.0)) j = i;	/* First index */
		if (p[i] > 0.0) k = i;			/* Last index  */
	}
	z = -1.0;
	m = -1;
	for (i = j; i <= k; i++) {
		y = nu(p, i, ut, vt);		/* Compute NU */
		if (y >= z) {			/* Is it the biggest? */
			z = y;			/* Yes. Save value and i */
			m = i;
		}
	}

	t = m;
//	log_info("first peak found is %d\n", t);


//the second peak helps to remove some noise. but it also makes some segments too thin or broken
//second peak
	m = 0;
	tt = t;
	for (i = 0; i < t; i++) {
		//		j = h[i]*((i - t)*(i - t));
//		j = abs(h[i] - h[t]) * ((i - t)*(i - t));
		j = (h[i] - h[t]) * ((i - t)*(i - t));
		//		j = h[i] - h[t];
		if (j > m){
			tt = i;
			m = j;
		}
	}
	
//	log_info("Second peak found is %d\n", tt);
if (Avg2Peak==1)
	t = (t + tt) / 2;

	log_dbg("Threshold found is %d\n", t);
	return t;


	/* Threshold */
	/*
	pp = NewImg->data[0];
	log_info("Image Size is %d\n", n);

	for (i = 0; i<n; i++)
		if (*pp < t)
			*pp++ = 0;
		else
			*pp++ = 255;

	*/
}


//the size of source image and destination image should be equal
//If the pixel's value larger and equal to ThresholdV then the pixel's value become MaxV , otherwise become MinV.
void APthreshold(imageIP *SrcImg, imageIP *DstImg, unsigned char ThresholdV, unsigned char MinV, unsigned char MaxV ) //THRESH_OTSU, THRESH_BINARY
{
	int	i, j;
	unsigned char *SrcPtr, *DstPtr, *SrcPtrTemp;

//	SrcPtr = (unsigned char *)SrcImg->data[0];
//	DstPtr = (unsigned char *)DstImg->data[0];
	SrcPtr = SrcImg->data[0];
	DstPtr = DstImg->data[0];
	for (i = 0; i< SrcImg->iRect->yr; i++)
	{
		for (j = 0; j< SrcImg->iRect->xc; j++)
		{
			if (*SrcPtr >= ThresholdV)
				*DstPtr = MaxV;
			else
				*DstPtr = MinV;
			DstPtr++;
			SrcPtr++;
		}
	}


}


