
#include "../Inc/main.h"
//#include "log.h"
#include <string.h>
#include <sys/types.h>
#include <linux/types.h>

#include "BasicSet.h"
#include "Image_PreProcess.h"
#include "CFilters.h"
#include "BKNote_PreSet.h"
#include "ImageProcessing.h"

/*
typedef struct
{
    short   area_idx;
    short   total_area;
    short   image_idx;
    char    state;
}   t_image_task_status;
*/
	
t_image_task_status img_task_status = {
    .total_area = 2
};

//request two image blocks first for every new page in
void cmd_BKCMD_IMAGE_IN_handler( t_rjob_cmd *pcmd )
{
    t_rjob_cmd Bkjobcmd;
    t_rjob_cmd *prsp;
    t_areas *areaP;
    t_ImageParam *BKimage_param, *DstImageIP_Param;
	int	ReqImgSize;

    Bkjobcmd = *pcmd;
    Bkjobcmd.cmd = BKCMD_IMAGE_IN_RSP;
    Bkjobcmd.dSize = 0;
    Bkjobcmd.dPtr = NULL;
    Bkjobcmd.mPtr = 0;
    send_rjob_cmd( &Bkjobcmd );  //response to the Image_In command
	
//following with the new commnad to request image areas
	
//while(1);
    img_task_status.area_idx = 0;
//	img_task_status.area_idx = 0;
//    img_task_status.image_idx++;

	DstImageIP_Param=(t_ImageParam *)RJob_TX_ShareMem;
    BKimage_param = (t_ImageParam *)pcmd->mPtr;
	Duplicate_imageparam(BKimage_param, DstImageIP_Param);
#if DebugBMPNmae
	log_dbg("[BMP]file_name: %s\n", BKimage_param->BMPfilename);
#endif
    Bkjobcmd.tag 	= 0x11;
    Bkjobcmd.cmd 	= BKCMD_REQUIRE_AREA;
    Bkjobcmd.dPtr 	= 0;
    Bkjobcmd.rsp 	= 0;
    Bkjobcmd.dSize 	= sizeof(t_ImageParam);
    Bkjobcmd.mPtr 	= RJob_TX_ShareMem;
	
/*    log_info("BKCMD_IMAGE_IN pImg=%X w=%d,h=%d TX=%X\r\n",
            (int)BKimage_param,BKimage_param->w, pImg->h ,
            (unsigned int)RJob_TX_ShareMem);
*/
/////////////////////////////////// 1ST image Box
	DstImageIP_Param->iJobIdx=ChkBox1L;
	SetCHKBoxReqArea(DstImageIP_Param);
	//checking NTD for now
    log_dbg("BKCMD_REQUIRE_AREA_1 DstImageIP_Param=%X w=%d h=%d oyi=%d oxj=%d SeqIdx=%d rTX=%X\n",
            (int)DstImageIP_Param,
            DstImageIP_Param->ImageRect.xc, 
            DstImageIP_Param->ImageRect.yr, 
            DstImageIP_Param->ImageRect.oyi, 
            DstImageIP_Param->ImageRect.oxj, 
            DstImageIP_Param->SeqIdx,
            Bkjobcmd.dSize);
	
//	DstImageIP_Param->ImageRect.xc=80;
//	DstImageIP_Param->ImageRect.yr=144;
//	DstImageIP_Param->ImageRect.oyi=186;
//	DstImageIP_Param->ImageRect.oxj=40;
	DstImageIP_Param->ImageLayerInfo.SelLayerNum=1;	//always select one for getting first image box
	
	send_rjob_cmd( &Bkjobcmd );
	prsp = wait_rjob1_rsp();
	log_info("	rsp cmd = %4x\r\n",prsp->cmd);
	remove_rjob1_rsp();
	
/////////////////////////////////// 2nd image Box
/*
	Bkjobcmd.tag = 0x12;
	DstImageIP_Param->iJobIdx=ChkBox2;
	SetCHKBoxReqArea(DstImageIP_Param);
    log_info("BKCMD_REQUIRE_AREA_2 DstImageIP_Param=%X w=%d,h=%d SeqIdx=%d rTX=%X\r\n",
            (int)DstImageIP_Param,
            DstImageIP_Param->ImageRect.nc, 
            DstImageIP_Param->ImageRect.nr, 
            DstImageIP_Param->SeqIdx,
            Bkjobcmd.dSize);
	send_rjob_cmd( &Bkjobcmd );
	prsp = wait_rjob1_rsp();
	log_info("	rsp cmd = %4x\r\n",prsp->cmd);
	remove_rjob1_rsp();
*/
//	ImageJobDispatch(pImg, &cmd);
	
/*
	BKNote_ChkP_Parameter *NTD_CHK_Clk=Get_NTD_CHK_LOC_Param(1);		//get the loaction of check point block on the front face 

    cmd.cmd = BKCMD_REQUIRE_AREA;
    cmd.dPtr = 0;
    cmd.dSize = sizeof(t_areas)+sizeof(t_rect)*2;
    cmd.mPtr = RJob_TX_ShareMem;
    cmd.tag = 0x11;
    cmd.rsp = 0;

	
    areaP = cmd.mPtr;
    img_task_status.total_area = 2;

   log_info(" Request area's SIZE = %x\r\n",NTD_CHK_Clk->Chk_Block_Height * NTD_CHK_Clk->Chk_Block_Width);
	
   areaP->total_area = 2;
   areaP->area[0].x=NTD_CHK_Clk->Chk_Left_Offfet;
   areaP->area[0].y=NTD_CHK_Clk->Chk_BTM_Offset;
   areaP->area[0].w=NTD_CHK_Clk->Chk_Block_Width;
   areaP->area[0].h=NTD_CHK_Clk->Chk_Block_Height;

   areaP->area[1].x=11;
   areaP->area[1].y=22;
   areaP->area[1].w=32;
   areaP->area[1].h=40;
*/
	/*
    areaP->total_area = 2;
    areaP->area[0].x=10;
    areaP->area[0].y=20;
    areaP->area[0].w=pImg->w-20;
    areaP->area[0].h=pImg->h-24;
    areaP->area[1].x=11;
    areaP->area[1].y=22;
    areaP->area[1].w=32;
    areaP->area[1].h=40;
	*/
	///////// Send cmmand and get result
	
}



//receive image data from A7
void cmd_BKCMD_SEND_AREA_handler( t_rjob_cmd *pcmd )
{
    t_rjob_cmd *prsp;
    t_ImageParam *BkJobImg_Para, *DstImageIP_Param;
    t_rjob_cmd doneCmd, ReqImgCmd;
	unsigned char *SrcImage, *TempPtr;
		

    int i1;
	
	BkJobImg_Para  = pcmd->mPtr;
//	log_info("BkJobImg_Para= %p\n", BkJobImg_Para);
//    char *ptr = (char *)&(pImg->data);

    log_dbg("RCV BKCMD_SEND_AREA BkJobImg_Para=%X w=%X,h=%X oyi=%d, oxj=%d, IpPAMemorySize=%X mPtr=%p\n",
            (int)BkJobImg_Para,
            BkJobImg_Para->ImageRect.xc, 
            BkJobImg_Para->ImageRect.yr, 
            BkJobImg_Para->ImageRect.oyi, 
            BkJobImg_Para->ImageRect.oxj, 
            BkJobImg_Para->IpPAMemorySize,
            pcmd->mPtr);
	

    doneCmd = *pcmd;
	SrcImage=pcmd->mPtr;
//	log_info("SrcImage= %p\n", SrcImage);
//	TempPtr=SrcImage+9700;
//	log_info("TempPtr= %p\n", TempPtr);
/*    log_info("RetrieveImgSetDDD= %x, %x, %x, %x, %x, %x, %x, %x\r\n",
				*TempPtr,
				*(TempPtr+1),
				*(TempPtr+1),
				*(TempPtr+1),
				*(TempPtr+1),
				*(TempPtr+1),
				*(TempPtr+1),
				*(TempPtr+1));
*/
	ImageJobDispatch(&doneCmd);
	
	BkJobImg_Para  = doneCmd.mPtr;
	doneCmd.cmd = BKCMD_DONE_AREA;
	log_dbg("BKCMD_SEND_AREA Return BkJobImg_Para=%X w=%X,h=%X IpPAMemorySize=%X mPtr=%p\n",
		(int)BkJobImg_Para,
		BkJobImg_Para->ImageRect.xc, 
		BkJobImg_Para->ImageRect.yr, 
		BkJobImg_Para->IpPAMemorySize,
		doneCmd.mPtr);
/*	log_info("SrcImage= %p\n", SrcImage);
	TempPtr=SrcImage+9700;
	log_info("TempPtr= %p\n", TempPtr);
	log_info("RetrieveImgSetYYYY= %x, %x, %x, %x, %x, %x, %x, %x\r\n",
			*TempPtr,
			*(TempPtr+1),
			*(TempPtr+1),
			*(TempPtr+1),
			*(TempPtr+1),
			*(TempPtr+1),
			*(TempPtr+1),
			*(TempPtr+1));
*/
	//	doneCmd.dSize = sizeof(t_ImageParam);
	doneCmd.rsp = 0;
	send_rjob_cmd( &doneCmd );
    prsp = wait_rjob1_rsp();
    log_info("  rsp cmd = %4x sent\n",prsp->cmd);
    remove_rjob1_rsp();

	log_info("SendiJobRtnCode = %x \r\n",BkJobImg_Para->iJobRtnCode);
	
///can add new send_rjob_cmd for the new job, then leave here, waiting for response in main while(1)
	if (BkJobImg_Para->iJobRtnCode == iJobImgInquirey)
	{
		if (BkJobImg_Para->iJobIdx >= NewAreaReq) //get new image block
		{
			DstImageIP_Param=(t_ImageParam *)RJob_TX_ShareMem;
			Duplicate_imageparam(BkJobImg_Para, DstImageIP_Param);
			ReqImgCmd=*pcmd;
			ReqImgCmd.tag	= 0x15;
			ReqImgCmd.cmd	= BKCMD_REQUIRE_AREA;
			ReqImgCmd.dPtr	= 0;
			ReqImgCmd.rsp	= 0;
			ReqImgCmd.dSize	= sizeof(t_ImageParam);
			ReqImgCmd.mPtr	= RJob_TX_ShareMem;
					
			send_rjob_cmd( &ReqImgCmd );
			prsp = wait_rjob1_rsp();
			log_info("	rsp cmd = %4x\r\n",prsp->cmd);
			remove_rjob1_rsp();

		}
	}else if ((BkJobImg_Para->iJobRtnCode==iJobImgComplete)
	|| (BkJobImg_Para->iJobRtnCode==iJobImgOCR)
	|| (BkJobImg_Para->iJobRtnCode==iJobImgNoMatch))
    {		/// if all job completed for this image then send BKCMD_IMAGE_COMPLETE
//		log_info("	area_idx = %4x \r\n", img_task_status.area_idx);
        doneCmd.cmd = BKCMD_IMAGE_COMPLETE;
        doneCmd.rsp = 0;
        doneCmd.dSize = 0;
        doneCmd.dPtr = NULL;
        doneCmd.mPtr = 0;
        send_rjob_cmd( &doneCmd );
//		prsp = wait_rjob1_rsp();
//		log_info("	rsp cmd = %4x\r\n",prsp->cmd);
//		remove_rjob1_rsp();
    }
	
}

//this function can be called after processing the image and need to save the intermediate job for processing
//
void cmd_BKCMD_SVAE_PCAREA( t_rjob_cmd *SavePcmd )
{
    t_rjob_cmd *prsp;
    t_ImageParam *BkJobImg_Para, *DstImageIP_Param;
    t_rjob_cmd 		doneCmd;
	unsigned char *SrcImage, *TempPtr;
			
	BkJobImg_Para  = SavePcmd->mPtr;
//	log_info("BkJobImg_Para= %p\n", BkJobImg_Para);
//    char *ptr = (char *)&(pImg->data);

    log_dbg("SavePCArea=%X w=%X,h=%X SeqIdx=%X mPtr=%p\n",
            (int)BkJobImg_Para,
            BkJobImg_Para->ImageRect.xc, 
            BkJobImg_Para->ImageRect.yr, 
            BkJobImg_Para->SeqIdx,
            SavePcmd->mPtr);
	
    doneCmd = *SavePcmd;
//	SrcImage=SavePcmd->mPtr;
//	log_info("SrcImage= %p\n", SrcImage);
//	TempPtr=SrcImage+9700;
/*	log_info("TempPtr= %p\n", TempPtr);
    log_info("RetrieveImgSetDDD= %x, %x, %x, %x, %x, %x, %x, %x\r\n",
				*TempPtr,
				*(TempPtr+1),
				*(TempPtr+1),
				*(TempPtr+1),
				*(TempPtr+1),
				*(TempPtr+1),
				*(TempPtr+1),
				*(TempPtr+1));
*/
	BkJobImg_Para  = doneCmd.mPtr;
	BkJobImg_Para->iJobRtnCode=iJobSaveIntPM;
	doneCmd.cmd = BKCMD_DONE_AREA;
//	log_info("SrcImage= %p\n", SrcImage);
//	TempPtr=SrcImage+9700;
/*	log_info("TempPtr= %p\n", TempPtr);
log_info("RetrieveImgSetYYYY= %x, %x, %x, %x, %x, %x, %x, %x\r\n",
			*TempPtr,
			*(TempPtr+1),
			*(TempPtr+1),
			*(TempPtr+1),
			*(TempPtr+1),
			*(TempPtr+1),
			*(TempPtr+1),
			*(TempPtr+1));
*///	doneCmd.dSize = sizeof(t_ImageParam);
	doneCmd.rsp = 0;
	send_rjob_cmd( &doneCmd );
    prsp = wait_rjob1_rsp();
    log_info("  rspSaveDonecmd = %4x sent\n",prsp->cmd);
    remove_rjob1_rsp();
	log_info("SaveiJobRtnCode = %x \r\n",BkJobImg_Para->iJobRtnCode);

}


void cmd10_handler( t_rjob_cmd *pcmd )
{
    t_rjob_cmd rsp;

    log_info("BKCMD_cmd10 dPtr=%X rTX=%X\n",
            (unsigned int)pcmd->dPtr ,
            (unsigned int)pcmd->mPtr);
    rsp = *pcmd;
    rsp.rsp = 0x15;
    rsp.mPtr += RJOB_TX_OFFSET;
    rsp.dSize = pcmd->dSize;
    //memcpy( rsp.dataPtr, pcmd->dataPtr, pcmd->dataSize );
    char *dest = (void*)__RJob_ShareMem + (int)rsp.mPtr;
    char *src  = (void*)__RJob_ShareMem + (int)pcmd->mPtr;
    for( int i1=0;i1<pcmd->dSize;i1++)
        if( i1 >= 0x440 )
            *dest++ = ~(*src++);
        else
            *dest++ = (*src++);

    send_rjob_rsp( &rsp );
}

void cmd00_handler( t_rjob_cmd *pcmd )
{
    t_rjob_cmd rsp;

    log_info("cmd00_handler \n");

//    rsp = *pcmd;
    rsp.cmd = 0;
    rsp.dSize = 0;
    rsp.dPtr = NULL;
    rsp.mPtr = 0;
    rsp.rsp = 0xFF;
    send_rjob_rsp( &rsp );
    send_rjob_rsp( &rsp );
    send_rjob_rsp( &rsp );
    send_rjob_rsp( &rsp );
    send_rjob_rsp( &rsp );
}

int m4_enter(int id) {

    log_info("[R] %s id: %d\n", __LINE__, id);
    return 0;
}

//*****************************************************************************
//  cmd_dispatcher
//  Note :
//      command should be processed one by one in order ,
//      or copy pcmd->dPtr data to another buffer
//      otherwise data will corrupted
//*****************************************************************************
void cmd_dispatcher( t_rjob_cmd *pcmd )
{
	log_info( "rjob cmd=%02X size=%d \n", pcmd->cmd, pcmd->dSize );
    //int len2 = (pcmd->dataSize<16) ? pcmd->dataSize : 16;
    //logDump( (void*)pcmd, sizeof(t_rjob_cmd));
    //logDump( (void*)__RJob_ShareMem, len2);

    unsigned short cmd = pcmd->cmd;
    switch( cmd )
    {
        case  0:
            cmd00_handler( pcmd );
            break;
        case  BKCMD_IMAGE_IN :		//receive when new page in
            cmd_BKCMD_IMAGE_IN_handler( pcmd );
            break;
        case  BKCMD_SEND_AREA : //receive image dat from A7
            cmd_BKCMD_SEND_AREA_handler( pcmd );
            break;
    }
}

