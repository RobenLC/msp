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
#include "ImageProcessing.h"
//#include "ConsoleBankNote.h"

#define Output_SRNCheckFile 		0
#define Output_ChkP_CheckFile 		0
#define	NTDSRN_MinVPixel		10*25*4		//minimum valid pixel count in one set of SRN
#define	NTDSRN_MaxVPixel		10*25*10	//maxmum valid pixel count in one set of SRN

#define	DebugChkImage				0

/* the t_rjob_cmd attaching the image parameter and image data at *mPtr
	usually it is starting from the first byte of share memory
	. if M4 side send BKCMD_####, the host side send t_ImageParam at BKrjob_cmd->mPtr.
	
	. if M4 side send BKCMD_REQUIRE_AREA, the host side allocate memory space for t_ImageParam and the image data area together
		Please see below for the data sequence.
	///////////////////////////		retrieve image///////////////////////////////////////
		SrcBKJob_Param=(t_ImageParam *)BKrjob_cmd->mPtr;
		JobImgParam_Size=sizeof(t_ImageParam);
		xc=SrcBKJob_Param->ImageRect.xc;		//width
		yr=SrcBKJob_Param->ImageRect.yr;		//Height
		unsigned char *SrcImage= SrcBKJob_Param + JobImgParam_Size;	//the starting address of image data
	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	-t_ImageParam Structure
			Imgheader		BKNoteImageRect;	//the rectangle information of the BKNote, the orginal point (0,0) is the bottom left cornor.
			int 			SeqIdx; 			//the sequence number during proceesing for the current image file
			int 			iJobIdx;			//assigned for the image processing job
			Imgheader		ImageRect;			//the rectagle information for the image using in the assigned iJobIdx, valid if width not equal to zero
			int 			iJobRtnCode;		//The return status code for the iJobIdx
			int 			IpPAMemorySize; 	//The size of the attached memory 
			unsigned char	*AttImgData;		//usually, pointer to the t_imageIP if there is an image data attached
	-imageIP Structure (AttImgData)
			struct Imgheader *info; 			//Pointer to header					
					int yr, xc; 			//Rows and columns in the image 
					int oyi, oxj; 			//Origin  
			unsigned char **data;				// Pixel values 
			----- image Data data[0]
			-//not including 2D array-------unsigned char data;

*/


extern t_image_task_status img_task_status;

//not used
imageIP *RetrieveImgSetOrg (t_ImageParam *SrcBKJob_Param)
{
	imageIP *ReqImageIP;
	unsigned char *TempPtr;
	int		i, JobImgParam_Size, imageIP_size;
	
	JobImgParam_Size=sizeof(t_ImageParam);
//	log_info("SrcBKJob_Param2= %p\n", SrcBKJob_Param);
	TempPtr=(unsigned char *)SrcBKJob_Param+JobImgParam_Size;
//	log_info("TempPtr= %p\n", TempPtr);
	ReqImageIP =(imageIP *) TempPtr; 
	ReqImageIP->iRect=(Imgheader *)(TempPtr+ sizeof(imageIP));
	imageIP_size=GetSizOf_imageIP(); //get the required memory size for new imageIP

/*
	log_info("RetrieveImgSet ReqImageIP=%X w=%d,h=%d oyi=%d oxj=%d\r\n",
						ReqImageIP,
						ReqImageIP->iRect->yr, 
						ReqImageIP->iRect->xc, 
						ReqImageIP->iRect->oyi,
						ReqImageIP->iRect->oxj);
    log_info("RetrieveImgSet ReqImageIP=%X w=%d,h=%d SeqIdx=%d rTX=%X\r\n",
						(int)ReqImageIP,
						ReqImageIP->iRect->xc, 
						ReqImageIP->iRect->yr, 
						ReqImageIP->data,
						ReqImageIP->data[0]);
*/	
	TempPtr= (unsigned char *)(TempPtr + imageIP_size);  //start address of image data
//	log_info("TempPtr_Data= %p\n", TempPtr);
	ReqImageIP->data = (unsigned char **)malloc(sizeof(unsigned char *)*ReqImageIP->iRect->yr);
	ReqImageIP->data[0]=TempPtr;
	for ( i = 1; i< ReqImageIP->iRect->yr; i++)
	{
		ReqImageIP->data[i] = (ReqImageIP->data[i - 1] + ReqImageIP->iRect->xc);    //only for one byte size per pixel
	}	
	return (ReqImageIP);
}
/*
imageIP *RetrieveImgSet2PA7M4 (t_ImageParam *SrcBKJob_Param)
{
	imageIP *ReqImageIP;
	unsigned char *TempPtr, *SrcImage;
	int		i, JobImgParam_Size, imageData_size, xc, yr,temp;
	
	JobImgParam_Size=sizeof(t_ImageParam);
	temp=SrcBKJob_Param;
	SrcImage= temp + JobImgParam_Size;
	xc=SrcBKJob_Param->ImageRect.xc;
	yr=SrcBKJob_Param->ImageRect.yr;
	imageData_size=xc*yr;
	memcpy(PA7M4ImgIP->data, SrcImage, imageData_size);
	PA7M4ImgIP->iRect->oyi=SrcBKJob_Param->ImageRect.oyi;
	PA7M4ImgIP->iRect->oxj=SrcBKJob_Param->ImageRect.oxj;
	//memcpy(pcmd, data, len > sizeof(t_rjob_cmd) ? sizeof(t_rjob_cmd) : len);
}
*/
imageIP *RetrieveImgSet (t_ImageParam *SrcBKJob_Param, unsigned char ShareData)
{
	imageIP *ReqImageIP;
	unsigned char *TempPtr, *SrcImage;
	int		i, JobImgParam_Size, imageIP_size, xc, yr,temp;
	
	JobImgParam_Size=sizeof(t_ImageParam);
	temp=SrcBKJob_Param;
	SrcImage= temp + JobImgParam_Size;
//	log_info("SrcBKJob_Param2= %p\n", SrcBKJob_Param);
//	log_info("JobImgParam_Size= %X\n", JobImgParam_Size);
	xc=SrcBKJob_Param->ImageRect.xc;
	yr=SrcBKJob_Param->ImageRect.yr;
//	log_info("tempA= %p\n", temp);
//	log_info("SrcImage char*= %p\n", SrcImage);
//	TempPtr=SrcImage+9600;
//	log_info("TempPtr= %p\n", TempPtr);
/*
	log_info("RetrieveImgSetDDD= %x, %x, %x, %x, %x, %x, %x, %x\r\n",
				*TempPtr,
				*(TempPtr+1),
				*(TempPtr+2),
				*(TempPtr+3),
				*(TempPtr+4),
				*(TempPtr+5),
				*(TempPtr+6),
				*(TempPtr+7));
*/				
	ReqImageIP = AllocateNewImage_2DMem( yr,  xc, SrcImage, ShareData); //including copy image data
	ReqImageIP->iRect->oyi=SrcBKJob_Param->ImageRect.oyi;
	ReqImageIP->iRect->oxj=SrcBKJob_Param->ImageRect.oxj;
/*	
    log_info("RetrieveImgSet ReqImageIP=%X w=%d,h=%d oyi=%d oxj=%d, Data=%p, Data[0]=%p\r\n",
						ReqImageIP,
						ReqImageIP->iRect->xc, 
						ReqImageIP->iRect->yr, 
						ReqImageIP->iRect->oyi,
						ReqImageIP->iRect->oxj,
						ReqImageIP->data,
						ReqImageIP->data[0]);
*/
	return (ReqImageIP);
}
void SetCHKBoxReqArea(t_ImageParam *BKimage_param)
{
	BKNote_ChkP_Parameter	*BillCHKBox;
	int						BoxParamSelect;		
	unsigned short 			BKimgJobIdx;

	BoxParamSelect=0;
	BKimgJobIdx=BKimage_param->iJobIdx;
	switch (BKimgJobIdx)
	{
		case ChkBox1L:	//face side		
		case ChkBox1R:  //face side	 
		 BoxParamSelect=1;		
		break;
		case ChkBox2L:	//Back side
		case ChkBox2R:  //Back side
		 BoxParamSelect=3;		
		break;
	}
/*
	log_info("SetCHKBoxReqArea---NoteW=%d, NoteH=%d, w=%d h=%d oyi=%d oxj=%d \r",
			BKimage_param->BKNoteImageRect.xc,
			BKimage_param->BKNoteImageRect.yr,
			BKimage_param->ImageRect.xc, 
			BKimage_param->ImageRect.yr, 
			BKimage_param->ImageRect.oyi, 
			BKimage_param->ImageRect.oxj);				
*/
	BillCHKBox=Get_NTD_CHK_LOC_Param(BoxParamSelect);		//get the loaction of check point block on the face side 
/*
	log_info("BillCHKBox  w=%d h=%d L_BTM=%d Left_O=%d \r",
			BillCHKBox->Chk_Block_Width, 
			BillCHKBox->Chk_Block_Height, 
			BillCHKBox->Chk_L_BTM_Offset, 
			BillCHKBox->Chk_Left_Offfet);
*/
	if ((BKimgJobIdx==ChkBox1L) ||(BKimgJobIdx==ChkBox2L))
	{
		BKimage_param->ImageRect.oxj=BillCHKBox->Chk_Left_Offfet;			//xc, oxi
		BKimage_param->ImageRect.oyi=BillCHKBox->Chk_L_BTM_Offset;			//yr, oyj
	}else
	{
		BKimage_param->ImageRect.oxj=BKimage_param->BKNoteImageRect.xc - BillCHKBox->Chk_Left_Offfet - BillCHKBox->Chk_Block_Width;			//xc, oxi
//because the height of bill could be truncated, it can not be used to figure out the height value of location.
//however, the NTD bill are all having same height. It will be used to do the calculation and not using the result of cropping.
		//BKimage_param->ImageRect.oyi=BKimage_param->BKNoteImageRect.yr - BillCHKBox->Chk_L_BTM_Offset - BillCHKBox->Chk_Block_Height;			//xc, oxi
		BKimage_param->ImageRect.oyi= NTDBill_Height- BillCHKBox->Chk_L_BTM_Offset - BillCHKBox->Chk_Block_Height;			//xc, oxi
	}
	BKimage_param->ImageRect.xc=BillCHKBox->Chk_Block_Width;
	BKimage_param->ImageRect.yr=BillCHKBox->Chk_Block_Height;
	log_dbg(" RQST_Box Width= %d: Height= %d\r",BillCHKBox->Chk_Block_Width, BillCHKBox->Chk_Block_Height);
	log_dbg(" RQST_Box oxj= %d: oyi= %d\r",BKimage_param->ImageRect.oxj, BKimage_param->ImageRect.oyi);
}
/*
#define	NTD100Img	1
#define	NTD200Img	2
#define	NTD500Img	3
#define	NTD1000Img	4
#define	NTD2000Img	5

#define	NTD100LED	GREENLED
#define	NTD200LED	GREENLED
#define	NTD500LED	BLUELED
#define	NTD1000LED	BLUELED
#define	NTD2000LED	BLUELED


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
*/
unsigned char SelNTDImageLayer(t_ImageLayers *ImgLayerInfo, unsigned char tBillNum)
{
	unsigned char LEDColor, tLEDSel;

	switch (tBillNum)
	{
		case NTD100Img:	LEDColor=NTD100LED;
		break;
		case NTD200Img: LEDColor=NTD200LED;
		break;
		case NTD500Img: LEDColor=NTD500LED;
		break;
		case NTD1000Img: LEDColor=NTD1000LED;
		break;
		case NTD2000Img: LEDColor=NTD2000LED;
		break;
		default: 		LEDColor=NTD100LED;
		break;
	}
	//match layer
		if ((ImgLayerInfo->LEDH2L1 & 0x0F) ==LEDColor)
			tLEDSel=1;
		else if (((ImgLayerInfo->LEDH2L1 & 0xF0)>>4) ==LEDColor)
			tLEDSel=2;
		else if ((ImgLayerInfo->LEDH4L3 & 0x0F) ==LEDColor)
			tLEDSel=3;
		else if (((ImgLayerInfo->LEDH4L3 & 0xF0)>>4) ==LEDColor)
			tLEDSel=4;
		else
		if (ImgLayerInfo->BKNote_Layers > 4)
		{
			if ((ImgLayerInfo->LEDH6L5 & 0x0F) ==LEDColor)
				tLEDSel=5;
			else if (((ImgLayerInfo->LEDH6L5 & 0xF0)>>4) ==LEDColor)
				tLEDSel=6;
			else if ((ImgLayerInfo->LEDH8L7 & 0x0F) ==LEDColor)
				tLEDSel=7;
			else if (((ImgLayerInfo->LEDH8L7 & 0xF0)>>4) ==LEDColor)
				tLEDSel=8;
			else
				tLEDSel=1;
		}else
			tLEDSel=1;
			
	return tLEDSel;

}
void ImageJobDispatch(t_rjob_cmd *BKJobcmd)
{
	imageIP 		*DstImage;
	Pos_XY  		CHKP_Pos;
	t_ImageParam 	*SrcBKJob_Param;
	BKNote_ImgBlk_Param *BKnote_SRN;
	unsigned char *SrcBKJobIMG_Param;
	int 			TempY, OrgAtTF,VPixel;
	unsigned char	ShareData;
	t_ImageParam 	DstImageIP_Param, SaveImageIP_Param;
	int 			t_ImageParam_Size;
	ImgRectC		SRN_Box;

	SrcBKJobIMG_Param=BKJobcmd->mPtr ;
	SrcBKJob_Param=(t_ImageParam *)SrcBKJobIMG_Param; 
	log_dbg("JobDispatch iJobIdx= %p\n", SrcBKJob_Param->iJobIdx);
	OrgAtTF=1;	//set 1 with rotate.c getting image data from low memory of block(upside down)
	switch (SrcBKJob_Param->iJobIdx)
	{
		case ChkBox1L:	//face side
			log_dbg("-ChkBox_1L\n");
//			log_info("SrcBKJob_Param= %p\n", SrcBKJob_Param);
			ShareData=DebugChkImage;
			DstImage = RetrieveImgSet(SrcBKJob_Param, ShareData);
#if	DebugChkImage
			SrcBKJob_Param->SeqIdx=ChkBox1L*10;
			cmd_BKCMD_SVAE_PCAREA(BKJobcmd );
#endif
			Clean_CHKImage_Block(DstImage);
#if	DebugChkImage
			SrcBKJob_Param->SeqIdx=ChkBox1L*10+1;
			cmd_BKCMD_SVAE_PCAREA(BKJobcmd );
#endif
			Get_CHKP_NTD_Pos(DstImage, &CHKP_Pos, (unsigned char) 3); //need to use 3 for NTD
			log_info(" **1L_BillNum= %d\n", CHKP_Pos.BillNum);
			if (CHKP_Pos.BillNum!=0)
			{
				//NTD:
				SrcBKJob_Param->ImageLayerInfo.SelLayerNum=SelNTDImageLayer(&SrcBKJob_Param->ImageLayerInfo, CHKP_Pos.BillNum);
				log_info(" **1L_LED %d\n", SrcBKJob_Param->ImageLayerInfo.SelLayerNum);

				SrcBKJob_Param->SeqIdx=0;		//set zero to not save BMP file
				BKnote_SRN=Get_NTD_BKNote_SNR_Param(CHKP_Pos.BillNum); //SRN	
				//because the orginal point of image from unrotate BMP filr is at top_left cornor, the image data is upside down,
				//it need to convert the CHKP_Pos.posy based on the original rectangle to get new CHKP_Pos.posy 
/*
				log_info("CHKBox1_REQImage= Xo=%d, Yo=%d, Xc=%d, Yr=%d, posy=%d\r\n",
						SrcBKJob_Param->ImageRect.oxj,
						SrcBKJob_Param->ImageRect.oyi,
						SrcBKJob_Param->ImageRect.xc,
						SrcBKJob_Param->ImageRect.yr,
						CHKP_Pos.posy);
*/
				SrcBKJob_Param->iJobIdx=SRNBox1L;
				TempY=SrcBKJob_Param->ImageRect.oyi*2+ SrcBKJob_Param->ImageRect.yr-CHKP_Pos.posy; 
//				log_info("NewCHKP_Pos.posy= %d\n", TempY);
				if (OrgAtTF==0)
					SrcBKJob_Param->ImageRect.oyi = BKnote_SRN->IMGBlk_Left_OrgY+ CHKP_Pos.posy; //from the center of check point
				else
					SrcBKJob_Param->ImageRect.oyi = BKnote_SRN->IMGBlk_Left_OrgY+ TempY; //from the center of check point

//				SrcBKJob_Param->ImageRect.oyi = BKnote_SRN->IMGBlk_Left_OrgY+ CHKP_Pos.posy; //from the center of check point

				SrcBKJob_Param->ImageRect.oxj = BKnote_SRN->IMGBlk_Left_OrgX+ CHKP_Pos.posx;
				SrcBKJob_Param->ImageRect.yr = BKnote_SRN->IMGBlk_Height;
				SrcBKJob_Param->ImageRect.xc = BKnote_SRN->IMGBlk_Width;	
/*
				log_info("CHKBox1_AdjREQ_Image: Xo=%d, Yo=%d, Xc=%d, Yr=%d, posy=%d\r",
						SrcBKJob_Param->ImageRect.oxj,
						SrcBKJob_Param->ImageRect.oyi,
						SrcBKJob_Param->ImageRect.xc,
						SrcBKJob_Param->ImageRect.yr,
						CHKP_Pos.posy);
*/
				SrcBKJob_Param->iJobRtnCode=iJobImgInquirey;
			}else
			{
			/////////////////////////////////// 2nd image Box
				SrcBKJob_Param->iJobIdx=ChkBox1R;
				SrcBKJob_Param->SeqIdx=0;		//set zero to not save BMP file
				SrcBKJob_Param->iJobRtnCode=iJobImgInquirey;			
				SetCHKBoxReqArea(SrcBKJob_Param);
/*
			log_info("ChkBox1R---iJobIdx=%d w=%d h=%d oyi=%d oxj=%d SeqIdx=%d\r",
						SrcBKJob_Param->iJobIdx,
						SrcBKJob_Param->ImageRect.xc, 
						SrcBKJob_Param->ImageRect.yr, 
						SrcBKJob_Param->ImageRect.oyi, 
						SrcBKJob_Param->ImageRect.oxj, 
						SrcBKJob_Param->SeqIdx);				
*/
			}
			log_dbg("--ChkBox_1L_Done\n");
			if (ShareData==0)
				freeImageMem(DstImage);
			else
				freeImage(DstImage);
		break;
			
		case ChkBox1R:	//Back side
			log_dbg("-ChkBox_1R\n");
			//if ShareData==0, cmd_BKCMD_SVAE_PCAREA still have original image
			ShareData= DebugChkImage;		
			DstImage = RetrieveImgSet(SrcBKJob_Param, ShareData);
#if	DebugChkImage
			SrcBKJob_Param->SeqIdx=ChkBox1R*10;
			cmd_BKCMD_SVAE_PCAREA(BKJobcmd );
#endif
			Clean_CHKImage_Block(DstImage);
#if	DebugChkImage
			SrcBKJob_Param->SeqIdx=ChkBox1R*10+1;
			cmd_BKCMD_SVAE_PCAREA(BKJobcmd );
#endif
			Get_CHKP_NTD_Pos(DstImage, &CHKP_Pos, (unsigned char) 3); //need to use 3 for NTD
			log_info(" **1RBillNum= %d\n", CHKP_Pos.BillNum);
//			SrcBKJob_Param->SeqIdx=22;
			if (CHKP_Pos.BillNum!=0)
			{
				//NTD:
				SrcBKJob_Param->ImageLayerInfo.SelLayerNum=SelNTDImageLayer(&SrcBKJob_Param->ImageLayerInfo, CHKP_Pos.BillNum);
				log_info(" **1R_LED %d\n", SrcBKJob_Param->ImageLayerInfo.SelLayerNum);
				
				SrcBKJob_Param->SeqIdx=0;		//set zero to not save BMP file
				BKnote_SRN=Get_NTD_BKNote_SNR_Param(CHKP_Pos.BillNum); //SRN	
				SrcBKJob_Param->iJobIdx=SRNBox1R;

//				log_info("---orgCHKP_Pos.posy= %d\n", CHKP_Pos.posy);
//				TempY=SrcBKJob_Param->ImageRect.oyi*2+ SrcBKJob_Param->ImageRect.yr-CHKP_Pos.posy; 
				if (OrgAtTF==1)
				{
					//CHKP_Pos->posy=DstImage->iRect->oyi+((High_Line+Low_Line)>>1);
	//				log_info("DstImage->iRect->oyi= %d\n", DstImage->iRect->oyi);
	//				log_info("ImageRect.yr= %d\n", SrcBKJob_Param->ImageRect.yr);
					TempY=CHKP_Pos.posy - DstImage->iRect->oyi; //get orginal offset
					TempY=SrcBKJob_Param->ImageRect.yr-TempY;  //flip it
					TempY = TempY + DstImage->iRect->oyi;		//add back to box's coordinate				
	//				TempY=SrcBKJob_Param->ImageRect.yr-CHKP_Pos.posy; 
	//				log_info("---NewCHKP_Pos.posy= %d\n", TempY);
					CHKP_Pos.posy=TempY; //from the center of check point
					SrcBKJob_Param->ImageRect.oyi = CHKP_Pos.posy - BKnote_SRN->IMGBlk_Left_OrgY - BKnote_SRN->IMGBlk_Height;
				}else
					SrcBKJob_Param->ImageRect.oyi = CHKP_Pos.posy - BKnote_SRN->IMGBlk_Left_OrgY - BKnote_SRN->IMGBlk_Height;
	//			log_info("---SrcBKJob_Param->ImageRect.oyi= %d\n", SrcBKJob_Param->ImageRect.oyi);
				
				SrcBKJob_Param->ImageRect.oxj = CHKP_Pos.posx - BKnote_SRN->IMGBlk_Left_OrgX - BKnote_SRN->IMGBlk_Width;
					
//				SrcBKJob_Param->ImageRect.oyi = BKnote_SRN->IMGBlk_Right_OrgY+ CHKP_Pos.posy;
				SrcBKJob_Param->ImageRect.yr = BKnote_SRN->IMGBlk_Height;
				SrcBKJob_Param->ImageRect.xc = BKnote_SRN->IMGBlk_Width;	
				SrcBKJob_Param->iJobRtnCode=iJobImgInquirey;
/*
				log_info("SRNBox1R---iJobIdx=%X w=%d h=%d oyi=%d oxj=%d SeqIdx=%d\r",
						(int)SrcBKJob_Param->iJobIdx,
						SrcBKJob_Param->ImageRect.xc, 
						SrcBKJob_Param->ImageRect.yr, 
						SrcBKJob_Param->ImageRect.oyi, 
						SrcBKJob_Param->ImageRect.oxj, 
						SrcBKJob_Param->SeqIdx);				
*/
			}else
			{
/////////// if no image matched and no more work with current paper //////// 
				SrcBKJob_Param->SeqIdx=0;
				SrcBKJob_Param->iJobIdx=ImageProcDone;
				SrcBKJob_Param->iJobRtnCode=iJobImgNoMatch;
				log_dbg(" --NoMatch-ChkBox1R_Done\n");
			}
			if (ShareData==0)
				freeImageMem(DstImage);
			else
				freeImage(DstImage);
		break;		
		case SRNBox1L:	//no processing for now
			log_dbg("-RCV SRNBox_1L\n");
			//set ShareData=1 for DebugChkImage=1
			//if ShareData==0, cmd_BKCMD_SVAE_PCAREA still have original image
			ShareData = 0;		//ShareData=1:share DstImage for debug purpose. 
			DstImage = RetrieveImgSet(SrcBKJob_Param, ShareData);
#if	DebugChkImage
			SrcBKJob_Param->SeqIdx=SRNBox1L*10;
			cmd_BKCMD_SVAE_PCAREA(BKJobcmd );
#endif
//			Clean_SRNImage_Block(DstImage);
			Clean_SRNImage_BlockTH(DstImage, 2);
			SRN_Box.oxj=0;
			SRN_Box.oyi=0;
			SRN_Box.xc=240;
			SRN_Box.yr=48;
//			Check_SRNBoxH_NTD(DstImage, &SRN_Box, 100);
			VPixel=CountImagePixel(DstImage, &SRN_Box, 10);
			log_dbg(" --VPixel= %d yr=%d, oyi=%d\n", VPixel, SRN_Box.yr, SRN_Box.oyi);
#if	DebugChkImage
			SrcBKJob_Param->SeqIdx=SRNBox1L*10+1;
			cmd_BKCMD_SVAE_PCAREA(BKJobcmd );
#endif
			SrcBKJob_Param->SeqIdx=SRNBox1L;
//			SrcBKJob_Param->iJobIdx=ImageProcDone;
//			if ((VPixel> NTDSRN_MinVPixel) && (SRN_Box.yr < 32))
			if ((VPixel> NTDSRN_MinVPixel) && (VPixel < NTDSRN_MaxVPixel) )
			{
				SrcBKJob_Param->iJobRtnCode=iJobImgOCR;
				log_dbg("-SRNBox_1L_Done\n");
			}
			else
			{
				SrcBKJob_Param->iJobRtnCode=iJobImgNoMatch;
				log_dbg(" --NoMatch_SRNBox_1L_Done\n");
			}
			if (ShareData==0)
				freeImageMem(DstImage);
			else
				freeImage(DstImage);
			break;
		case SRNBox1R:			//no processing for now
			log_dbg(" -RCV SRNBox_1R\n");
			//set ShareData=1 for DebugChkImage=1
			//if ShareData==0, cmd_BKCMD_SVAE_PCAREA still have original image
//			ShareData = DebugChkImage;		
			ShareData = 0;		//ShareData=1:share DstImage for debug purpose. 
			DstImage  = RetrieveImgSet(SrcBKJob_Param, ShareData);
#if	DebugChkImage
			SrcBKJob_Param->SeqIdx=SRNBox1R*10;
			cmd_BKCMD_SVAE_PCAREA(BKJobcmd );
#endif
//			Clean_SRNImage_Block(DstImage);
			Clean_SRNImage_BlockTH(DstImage, 2);
			SRN_Box.oxj=0;
			SRN_Box.oyi=0;
			SRN_Box.xc=240;
			SRN_Box.yr=48;
//			Check_SRNBoxH_NTD(DstImage, &SRN_Box, 100);
			VPixel=CountImagePixel(DstImage, &SRN_Box, 10);
			log_dbg(" --VPixel= %d yr=%d, oyi=%d\n", VPixel, SRN_Box.yr, SRN_Box.oyi);
#if	DebugChkImage
			SrcBKJob_Param->SeqIdx=SRNBox1R*10+1;
			cmd_BKCMD_SVAE_PCAREA(BKJobcmd );
#endif
			SrcBKJob_Param->SeqIdx=SRNBox1R;
		if ((VPixel> NTDSRN_MinVPixel) && (VPixel < NTDSRN_MaxVPixel) )
//			if (VPixel> NTDSRN_MinVPixel)
			{
				SrcBKJob_Param->iJobRtnCode=iJobImgOCR;
				log_dbg("---SRNBox_1R_Done\n");
			}
			else
			{
				SrcBKJob_Param->iJobRtnCode=iJobImgNoMatch;
				log_dbg(" --NoMatch_SRNBox_1R_Done\n");
				log_dbg(" --VPixel count= %d\n", VPixel);
			}
		if (ShareData==0)
			freeImageMem(DstImage);
		else
			freeImage(DstImage);
		break;		
	}
}
//void Clean_SRNImage_Block(TCHAR *folder, TCHAR *fn, BMPFile_Ptr SRN_BMPFile)
void	Get_SRNBlock_Blk(imageIP *TGTImage, Pos_XY *CHKP_Pos)
{
	imageIP *DstImage;
//	Pos_XY  CHKP_Pos;
	BKNote_ChkP_Parameter *BillCHKBox;
	
	imgBlock_Loc_Set *imgBlock_Para= (imgBlock_Loc_Set *)malloc(sizeof(imgBlock_Loc_Set));
	BillCHKBox=Get_NTD_CHK_LOC_Param(1);		//get the loaction of check point block on the face side 
	imgBlock_Para->Block_Height=BillCHKBox->Chk_Block_Height;
	imgBlock_Para->Block_Width=BillCHKBox->Chk_Block_Width;
	imgBlock_Para->posy=BillCHKBox->Chk_L_BTM_Offset;
	imgBlock_Para->posx=BillCHKBox->Chk_Left_Offfet;
	Get_CHKP_NTD_Pos(TGTImage, CHKP_Pos, (unsigned char) 3); //need to use 3 for NTD
//	Pos_XY *CHKP_Pos = (Pos_XY *)malloc(sizeof(Pos_XY));
	free(imgBlock_Para);

}
imageIP *BW_SRNImage_Block(imageIP *SrcImage)
{
//	TCHAR outFolder[300];

	imageIP *DstImage;
	log_dbg("------->Clean_Image_Block\n");
//Get threshold
	unsigned char Thrd = thr_glh(SrcImage, 0);
	DstImage = AllocateNewImage_Mem(SrcImage->iRect->yr, SrcImage->iRect->xc);
	Thrd=Thrd-15;
	APthreshold(SrcImage, DstImage, Thrd, BLACK, WHITE); //THRESH_OTSU, THRESH_BINARY
	return DstImage;
}


void Clean_SRNImage_BlockTH(imageIP *DstImage, unsigned char THStage)
{
//	TCHAR outFolder[300];
	SElement *SEp2;
	SElement *SEp3;
unsigned char Thrd ;

//	IMAGEIP_Ptr DstImage=SRN_BMPFile->BMPImage;
//	int DstImgFileSize=SRN_BMPFile->HeaderSize + DstImage->iRect->yr *	DstImage->iRect->xc; 	
	log_dbg("------->Clean_SRNImage_Block\n");
//Get threshold
	if (THStage > 0)
	{
		Thrd = thr_glh(DstImage, 0);
//	DstImage = AllocateNewImage_Mem(DstImage->iRect->yr, DstImage->iRect->xc);
		Thrd=Thrd-10;
		APthreshold(DstImage, DstImage, Thrd, BLACK, WHITE); //THRESH_OTSU, THRESH_BINARY
	}
#if Output_SRNCheckFile
	wcscpy_s(outFolder, folder);
	wcscat_s(outFolder, L"SRN_Ths");
	if (isCreateOut1 == false)
	{
		_wmkdir(outFolder);
		isCreateOut1 = true;
	}
	Output_BMP(outFolder, fn, SRN_BMPFile->BMPFile_data, DstImgFileSize);	
#endif //Output_SRNCheckFile

if (THStage > 1)
{
		SEp2=Allocate_SElement(2, 2);
		Get_2x2SElement( SEp2, 1);
		
		if (THStage > 2)
		{
			SEp3 = Allocate_SElement(3, 3);
			Get_3x3SElement( SEp3, 4);
		}
//good for NTD100, SRN, the font is very thin, can only use 2x2 matrix
//		SEp2=Allocate_SElement(2, 2);
		Get_2x2SElement( SEp2, 1);
		bin_erode(DstImage, SEp2); 
		bin_dilate(DstImage, SEp2);

		
#if Output_SRNCheckFile
		wcscpy_s(outFolder, folder);
		wcscat_s(outFolder, L"SRN_Open1");
		if (isCreateOut2 == false)
		{
			_wmkdir(outFolder);
			isCreateOut2 = true;
		}
		Output_BMP(outFolder, fn, SRN_BMPFile->BMPFile_data, DstImgFileSize); 
#endif //Output_SRNCheckFile

//might not need 2nd round for NTD100, SRN, the font is very thin, can only use 2x2 matrix
//		bin_erode(DstImage, SEp2);
//		bin_dilate(DstImage, SEp2); 

#if Output_SRNCheckFile
		wcscpy_s(outFolder, folder);
		wcscat_s(outFolder, L"SRN_Open2");
		if (isCreateOut3 == false)
		{
			_wmkdir(outFolder);
			isCreateOut3 = true;
		}
#else
//		wcscpy_s(outFolder, folder);
#endif //Output_SRNCheckFile
//	Output_BMP(outFolder, fn, SRN_BMPFile->BMPFile_data, DstImgFileSize);

//log_info("------->bin_dilate_Out\n");

//****************************************************************************
// write
//****************************************************************************
	freeSEimage(SEp2);
	if (THStage > 2)
		freeSEimage(SEp3);
}	
	log_dbg("------->Clean_SRNImage_Block_Done\n");
}

void Clean_SRNImage_Block(imageIP *DstImage)
{
//	TCHAR outFolder[300];
	SElement *SEp2;
	SElement *SEp3;

//	IMAGEIP_Ptr DstImage=SRN_BMPFile->BMPImage;
//	int DstImgFileSize=SRN_BMPFile->HeaderSize + DstImage->iRect->yr *	DstImage->iRect->xc; 	

	log_dbg("------->Clean_SRNImage_Block\n");
//Get threshold
	unsigned char Thrd = thr_glh(DstImage, 0);
//	DstImage = AllocateNewImage_Mem(DstImage->iRect->yr, DstImage->iRect->xc);
	Thrd=Thrd-15;
	APthreshold(DstImage, DstImage, Thrd, BLACK, WHITE); //THRESH_OTSU, THRESH_BINARY
	
#if Output_SRNCheckFile
	wcscpy_s(outFolder, folder);
	wcscat_s(outFolder, L"SRN_Ths");
	if (isCreateOut1 == false)
	{
		_wmkdir(outFolder);
		isCreateOut1 = true;
	}
	Output_BMP(outFolder, fn, SRN_BMPFile->BMPFile_data, DstImgFileSize);	
#endif //Output_SRNCheckFile

		SEp2=Allocate_SElement(2, 2);
		Get_2x2SElement( SEp2, 1);
		
		SEp3 = Allocate_SElement(3, 3);
		Get_3x3SElement( SEp3, 4);

//good for NTD100, SRN, the font is very thin, can only use 2x2 matrix
//		SEp2=Allocate_SElement(2, 2);
		Get_2x2SElement( SEp2, 1);
		bin_erode(DstImage, SEp2); 
		bin_dilate(DstImage, SEp2);

		
#if Output_SRNCheckFile
		wcscpy_s(outFolder, folder);
		wcscat_s(outFolder, L"SRN_Open1");
		if (isCreateOut2 == false)
		{
			_wmkdir(outFolder);
			isCreateOut2 = true;
		}
		Output_BMP(outFolder, fn, SRN_BMPFile->BMPFile_data, DstImgFileSize); 
#endif //Output_SRNCheckFile

//might not need 2nd round for NTD100, SRN, the font is very thin, can only use 2x2 matrix
//		bin_erode(DstImage, SEp2);
//		bin_dilate(DstImage, SEp2); 

#if Output_SRNCheckFile
		wcscpy_s(outFolder, folder);
		wcscat_s(outFolder, L"SRN_Open2");
		if (isCreateOut3 == false)
		{
			_wmkdir(outFolder);
			isCreateOut3 = true;
		}
#else
//		wcscpy_s(outFolder, folder);
#endif //Output_SRNCheckFile
//	Output_BMP(outFolder, fn, SRN_BMPFile->BMPFile_data, DstImgFileSize);

//log_info("------->bin_dilate_Out\n");

//****************************************************************************
// write
//****************************************************************************

	freeSEimage(SEp2);
	freeSEimage(SEp3);
	
	log_dbg("------->Clean_SRNImage_Block_Done\n");
}

/*
void Marking_IndAlpha_ImgBlk(TCHAR *folder, TCHAR *fn, int DstImgFileSize, unsigned char *SNRImagePtr, imageIP *DstImage, int HighLine, int LowLine,  AlphaB_Block_Ptr OCRImg_Blk)
{
	TCHAR outFolder[300];
	int i, j, k, gap_num;
	int *Gap_Loc;
//Draw High line
		 i=HighLine;
		for (j = 0; j < DstImage->iRect->xc; j++)
		{
			DstImage->data[i][j]=128;
		}

//Draw low line
		 i=LowLine;
		for (j = 0; j < DstImage->iRect->xc; j++)
		{
			DstImage->data[i][j]=128;
		}
		
//draw gap line
	gap_num= OCRImg_Blk->VRT_Pairs;
	Gap_Loc=OCRImg_Blk->VRT_Edges;

	for (k = 0; k < gap_num; k++)
	{
		j=*Gap_Loc;
		Gap_Loc++;
		for (i = 0; i < DstImage->iRect->yr; i++)
		{
			DstImage->data[i][j]=128;
		}
		j=*Gap_Loc;
		if (j>=DstImage->iRect->xc)
			j=DstImage->iRect->xc-1;
		Gap_Loc++;
		for (i = 0; i < DstImage->iRect->yr; i++)
		{
			DstImage->data[i][j]=128;
		}
	}
	wcscpy_s(outFolder, folder);
	wcscat_s(outFolder, L"AlphaB_BlkA");
	_wmkdir(outFolder);
	
	Output_BMP(outFolder, fn, SNRImagePtr, DstImgFileSize);	


}
*/




//int Find_HighEdge(imageIP * TGTImage, ImgRectC *ChkBlock, int MinPixels, int MinLines, unsigned char MarkV);
//Pos_XY *Get_CHKP_NTD_Pos(imageIP *DstImage,  imgBlock_Loc_Set *Block_Pos_Param, unsigned char Match_PixelNum)
//Match_PixelNum = 3 for NTD
void Get_CHKP_NTD_Pos(imageIP *DstImage, Pos_XY *CHKP_Pos, int Match_PixelNum)
{
	ImgRectC *Markblock;
	int 		MinPixels, MinLines;
	int 		HighEdge, LowEdge, LeftEdge, RightEdge;
//	Pos_XY *CHKP_Pos = (Pos_XY *)malloc(sizeof(Pos_XY));
	int Low_Line=CHK_LowestLine( DstImage, 0, Match_PixelNum, Match_PixelNum+2,BLACK);
	int High_Line=CHK_HighestLine( DstImage, DstImage->iRect->yr-1, Match_PixelNum, Match_PixelNum+2,BLACK); //i=TGTImage->iRect->yr-1
	int Left_Line=CHK_LeftLine( DstImage, 0,Match_PixelNum, BLACK);
	int Right_Line=CHK_RightLine( DstImage, DstImage->iRect->xc-1, Match_PixelNum, BLACK); //j=TGTImage->iRect->xc-1

	int CHKP_WDiff=Right_Line-Left_Line;
	int CHKP_HDiff=High_Line-Low_Line;

	//the rect. box with marks
	Markblock = (ImgRectC *)malloc(sizeof(ImgRectC));
	Markblock->oxj=Left_Line;
	Markblock->oyi=Low_Line;
	Markblock->xc=CHKP_WDiff;
	Markblock->yr=CHKP_HDiff;
	if (CHKP_WDiff <= (Match_PixelNum << 1)) //1000 can be very thin, 3 pixels wide?
		MinPixels = CHKP_WDiff;
	else
		MinPixels = CHKP_WDiff - Match_PixelNum;
	MinLines  = 10;		//normal, it is 20, at least 10, on the edge?
	HighEdge = Find_HighEdge(DstImage, Markblock, MinPixels, MinLines, WHITE);
	LeftEdge = Find_LeftEdge(DstImage, Markblock, CHKP_HDiff-MinPixels, MinLines, WHITE);
	RightEdge = Find_RightEdge(DstImage, Markblock, CHKP_HDiff-MinPixels, MinLines, WHITE);

    log_dbg(" --Left_Line=%X, Right_Line=%d, High_Line=%d, Low_Line=%d \n",
    		Left_Line, Right_Line, High_Line, Low_Line);
    log_dbg(" --LeftEdge=%X, RightEdge=%d, HighEdge=%d\n",
    		LeftEdge, RightEdge, HighEdge);
	log_dbg(" --CHKP_WDiff= %d \n",CHKP_WDiff);
	log_dbg(" --CHKP_HDiff= %d \n",CHKP_HDiff);
	log_dbg(" --MinPixels= %d\n", MinPixels);

	int MiddleGap =LeftEdge - RightEdge;
	if ((CHKP_HDiff >= 100) || (CHKP_WDiff >= 28))  //105 for 500, 40 for 2000
		CHKP_Pos->BillNum=0;
	else if ((HighEdge==0) && (CHKP_HDiff > 40) && (CHKP_HDiff < 56) && (CHKP_WDiff < 28)) //vertical mark lines, and no gap, 1000 or 2000, need to check if there is a center gap, or just by width
	{
	 	if ((CHKP_WDiff > 15) && (LeftEdge > RightEdge) && (MiddleGap > 7)  
	 		&& ((RightEdge - Left_Line) >= 2) && ((Right_Line- LeftEdge) >= 2))
			CHKP_Pos->BillNum=NTD2000Img;	//124*54, 18*50
	 	else if ((CHKP_WDiff < 12) && ( MiddleGap == 0))
			CHKP_Pos->BillNum=NTD1000Img;	//6*54, 3*50			
		else
			CHKP_Pos->BillNum=0;			
	}else //100, 200, 500
	{
		int Mark1stHi = HighEdge - Low_Line;		//height of 1st mark
		log_dbg(" --High1StM= %d\n", Mark1stHi);
		if ((CHKP_WDiff < 28) && (CHKP_HDiff < 100))
		{
			if ((CHKP_HDiff > 65) && (Mark1stHi < 25) && (HighEdge !=0))
				CHKP_Pos->BillNum=NTD500Img;	//22*95, and HighEdge < 20
			else if ((CHKP_HDiff > 30) && (Mark1stHi < 25) && (HighEdge !=0))
				CHKP_Pos->BillNum=NTD200Img;	//22*62, and HighEdge < 20
			else if ((CHKP_HDiff < 22) && (CHKP_HDiff >  9) && (CHKP_WDiff < 22) && (CHKP_WDiff > 9))//half height of mark, the differenc is very small if the check point is on the edge for NTD100
				CHKP_Pos->BillNum=NTD100Img;	//22*22, 15*15, and HighEdge=0, CHKP_WDiff < 22
			else 
				CHKP_Pos->BillNum=0;	
		}
		else 
			CHKP_Pos->BillNum=0;	
	}

	CHKP_Pos->posx=DstImage->iRect->oxj+((Left_Line+Right_Line)>>1);
	CHKP_Pos->posy=DstImage->iRect->oyi+((High_Line+Low_Line)>>1);

	free(Markblock);

/*	
	imgBlock_LocPtr CHKPT_Box= (imgBlock_Loc_Set *)malloc(sizeof(imgBlock_Loc_Set));
	CHKPT_Box->Block_Width=Left_Line - Right_Line;
	CHKPT_Box->Block_Height=High_Line - Low_Line;
	CHKPT_Box->posx=Block_Pos_Param->posx+Left_Line;
	CHKPT_Box->posy=Block_Pos_Param->posy+Low_Line;
*/
	//int Minimun_Gap_Width

//	CHK_WHT_VRT_Gap(DstImage, AlphaB_Block_Ptr OCRImg_Blk, int High_Line, int Low_Line, int Minimun_Gap_Width);
//	int HGaps=Get_WHT_HOZ_Gap(DstImage, CHKPT_Box, 10, 20);
	
//	free(CHKPT_Box);
//	return (CHKP_Pos);
}




//#define FaceSideNote	0;
//#define BackSideNote	1;
//POS_XY_Ptr Clean_CHKImage_Block(TCHAR *folder, TCHAR *fn, unsigned char *OrgImagePtr, IMAGEIP_Ptr SrcImage, int Image_Header_Size, imgBlock_Loc_Set * Block_Pos_Param, unsigned char Front0_Back1)
//void Clean_CHKImage_Block(TCHAR *folder, TCHAR *fn, int DstImgFileSize, unsigned char *SNRImagePtr, IMAGEIP_Ptr DstImage)
//void Clean_CHKImage_Block(TCHAR *folder, TCHAR *fn, BMPFile_Ptr ChkP_BMPFile)

void Clean_CHKImage_Block(imageIP *DstImage)
{
//	TCHAR outFolder[300];
	SElement *SEp2;
	SElement *SEp3;
	SElement *SEp4;

//	IMAGEIP_Ptr DstImage=ChkP_BMPFile->BMPImage;
//	int DstImgFileSize=ChkP_BMPFile->HeaderSize + DstImage->iRect->yr *  DstImage->iRect->xc; 	
//Get threshold
	//for NTD 100 back chkpoint, can not use Avg2Peak, beause there is only one peak, need to -30, but still not good,
	//perhaps, need to use larger (4,4) Element
	unsigned char Thrd = thr_glh(DstImage, 0);
//	DstImage = AllocateNewImage_Mem(DstImage->iRect->yr, DstImage->iRect->xc);
//	Thrd=Thrd-30; //NTD100Back
	Thrd=Thrd-10;
	APthreshold(DstImage, DstImage, Thrd, BLACK, WHITE); //THRESH_OTSU, THRESH_BINARY

#if Output_ChkP_CheckFile
	wcscpy_s(outFolder, folder);
	wcscat_s(outFolder, L"CHKP_Ths");
	if (isCreateOut5 == false)
	{
		_wmkdir(outFolder);
		isCreateOut5 = true;
	}
	Output_BMP(outFolder, fn, ChkP_BMPFile->BMPFile_data, DstImgFileSize);
#endif	//Output_ChkP_CheckFile

//	log_info("----CHKP--->bin_dilate\n");
/*	
		SEp=Allocate_SElement(3, 3);
		Get_3x3SElement(SEp, 4);
		bin_erode(DstImage, SEp); 
		Get_3x3SElement( SEp, 4);
		bin_dilate(DstImage, SEp); 
*/
		

		SEp4 = Allocate_SElement(4, 4);
		Get_4x4SElement(SEp4, 4);
//		log_info("----SEp4---\n");

		SEp2=Allocate_SElement(2, 2);
		Get_2x2SElement(SEp2, 1);
//		log_info("----SEp2---\n");

		SEp3 = Allocate_SElement(3, 3);
		Get_3x3SElement( SEp3, 4);
//		log_info("----SEp3---\n");


		bin_erode(DstImage, SEp4);
//		log_info("---bin_erode-SEp4---\n");
		bin_erode(DstImage, SEp3);
//		log_info("---bin_erode-SEp3---\n");
		bin_dilate(DstImage, SEp3);
//		log_info("---bin_dilate-SEp3---\n");

//		Output_BMP(outFolder, fn, CHKPImagePtr, DstImgFileSize);
/*
		//for NTD 100 back chkpoint, remove most of noise, keep the rough shape of center
		Get_2x2SElement(SEp2, 1);

		SEp3 = Allocate_SElement(3, 3);
		Get_3x3SElement( SEp3, 4);
		SEp4 = Allocate_SElement(4, 4);
		Get_4x4SElement(SEp4, 4);
		bin_erode(DstImage, SEp4);
		bin_erode(DstImage, SEp3);
		bin_dilate(DstImage, SEp3);
*/
#if Output_ChkP_CheckFile

		log_dbg("----CHKP--->bin_dilate_Out1\n");
		wcscpy_s(outFolder, folder);
		wcscat_s(outFolder, L"CHKP_Open1");
		if (isCreateOut6 == false)
		{
			_wmkdir(outFolder);
			isCreateOut6 = true;
		}
		Output_BMP(outFolder, fn, ChkP_BMPFile->BMPFile_data, DstImgFileSize);
#endif	//Output_ChkP_CheckFile
/*
//for NTD 100 back chkpoint, remove most of noise, keep the rough shape of center
		bin_erode(DstImage, SEp4);
		bin_erode(DstImage, SEp3);
		//		bin_dilate(DstImage, SEp2);
*/
//		bin_erode(DstImage, SEp4);
		bin_erode(DstImage, SEp3);
		bin_dilate(DstImage, SEp2);

#if Output_ChkP_CheckFile
		wcscpy_s(outFolder, folder);
		wcscat_s(outFolder, L"CHKP_Open2");
		if (isCreateOut7 == false)
		{
			_wmkdir(outFolder);
			isCreateOut7 = true;
		}
		log_dbg("----CHKP--->bin_dilate_Out2\n");
#else
//		wcscpy_s(outFolder, folder);
#endif	//Output_ChkP_CheckFile
//		Output_BMP(outFolder, fn, ChkP_BMPFile->BMPFile_data, DstImgFileSize);
//	log_info("----CHKP--->bin_dilate_Out2\n");

	freeSEimage(SEp2);
	freeSEimage(SEp3);
	freeSEimage(SEp4);
  	//****************************************************************************
	//
	//****************************************************************************
	
}

//NTD SRN Box: WxH=200*25, each char:14*25, Chars:10, Pen:2_pixels, Vgap:7,, OneLinePixels: Min=2*10*1.5=30
//SRN_Box:240*48, Continuous_White: (240-30)/2=110 
void Check_SRNBoxH_NTD(imageIP *DstImage, ImgRectC *SRN_Box, int MinMatchPixel)
{
	int 		MinPixels, MinLines, CenterLine, MiniWhite;
	//check from center 
	CenterLine = SRN_Box->yr >> 1;
	int High_Line=CHK_LowestLine( DstImage, CenterLine, MinMatchPixel, 3, WHITE);
	int Low_Line=CHK_HighestLine( DstImage, CenterLine, MinMatchPixel, 3, WHITE); //i=TGTImage->iRect->yr-1
//	int Left_Line=CHK_LeftLine( DstImage, 0,Match_PixelNum, BLACK);
//	int Right_Line=CHK_RightLine( DstImage, DstImage->iRect->xc-1, Match_PixelNum, WHITE); //j=TGTImage->iRect->xc-1
	log_dbg("---High_Line= %d Low_Line=%d, CenterLine=%d\n", High_Line, Low_Line, CenterLine);

	int CHKP_HDiff=High_Line-Low_Line;
	SRN_Box->oyi=Low_Line;
	SRN_Box->yr=CHKP_HDiff;
}

void Duplicate_imageparam(t_ImageParam *SrcImageIP_Param, t_ImageParam *DstImageIP_Param)
{
	//Duplicate JobImg_Param2 
	unsigned char *DstTemp_ptr = (unsigned char *)DstImageIP_Param ;
	unsigned char *SrcTemp_ptr=(unsigned char *)SrcImageIP_Param;
	int t_ImageParam_Size=sizeof (t_ImageParam);
	for( int i = 1; i< t_ImageParam_Size; i++)
	{
		*DstTemp_ptr = *SrcTemp_ptr;	   
		SrcTemp_ptr++;
		DstTemp_ptr ++;
	}

}


