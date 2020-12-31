
#include "../Inc/main.h"
//#include "log.h"
#include <stdlib.h> 
#include <stdio.h>
#include <string.h>
#include <unistd.h> 
#include <stddef.h>
#include <poll.h>

#include <sys/types.h>
#include <linux/types.h>

#include "BasicSet.h"
#include "Image_PreProcess.h"
#include "CFilters.h"
#include "BKNote_PreSet.h"
#include "ImageProcessing.h"

const char Rjob_Buffer[RJOB_BUFF_SIZE] = {0};

unsigned int __RJob_ShareMem = (unsigned int)&Rjob_Buffer[0];
unsigned int __RJob_ShareMem_end = (unsigned int)&Rjob_Buffer[RJOB_BUFF_SIZE - 1];
//unsigned int RJob_TX_ShareMem = (unsigned int)&(Rjob_Buffer[RJOB_BUFF_SIZE/2]);
unsigned int RJob_TX_ShareMem = 0;
static char *Rjob_RX_mem = 0;

static int mfourPipEpt1[2] = {0};
static int mfourPipEpt2[2] = {0};

int mfourSetPipEpt1(int *pip)
{
    log_info("%s - 0: %d, 1: %d\n",__func__, pip[0], pip[1]);
    memcpy(mfourPipEpt1, pip, sizeof(int) * 2);

    return 0;
}

int mfourSetPipEpt2(int *pip)
{
    log_info("%s - 0: %d, 1: %d\n",__func__, pip[0], pip[1]);
    memcpy(mfourPipEpt2, pip, sizeof(int) * 2);

    return 0;
}

static int OPENAMP_send(int *pip, t_rjob_cmd *pc, int sizec)
{
    int ret=0;
    char cmd[2]={0};

    log_info("%s - cmd: 0x%.2x, size: %d\n",__func__, pc->cmd, sizec);

    if (sizec > 0xff) return -1;

    cmd[0] = 'r';
    cmd[1] = sizec & 0xff;
    
    ret = write(pip[1], cmd, 2);
    
    ret = write(pip[1], pc, sizec);

    return 0;
}

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

//*****************************************************************************
//  send_rjob_cmd
//  Note :  resmgr_ept.dest_addr is 0xfffffff when begining ,
//          it need to receive a command from A7 to update it to correct value
//          otherwise OPENAMP_send() will fail
//*****************************************************************************
int send_rjob_cmd(t_rjob_cmd *pcmd)
{
    int ret;
    char *cpysrc=0, *cpydst=0;
    
    if( pcmd->mPtr != 0 ) {
        //pcmd->mPtr = pcmd->mPtr - (int)__RJob_ShareMem;

        pcmd->dPtr = Rjob_RX_mem;
        cpysrc = pcmd->mPtr;
        cpydst = pcmd->dPtr;

        memcpy(cpydst, cpysrc, pcmd->dSize);
    }

    ret = OPENAMP_send(mfourPipEpt2, pcmd, sizeof(t_rjob_cmd));
    if (ret < 0) {
        log_err("Failed to send cmdmessage ret=%d \n", ret);
    }
    
    log_info("rjob send cmd %d byte (cmd=%04x tag=%04lx rsp=%04x dsize=%5d dPtr=%08x mPtr=%08x)\r\n",
            sizeof(t_rjob_cmd),
            pcmd->cmd,
            pcmd->tag,
            pcmd->rsp,
            pcmd->dSize,
            (int)pcmd->dPtr,
            (int)pcmd->mPtr);

    return 0;
}

//*****************************************************************************
//  wait_rjob_cmd
//  blocking when cmd not reveived
//*****************************************************************************
void *wait_rjob_cmd(void)
{
    struct pollfd pllfd[2]={0};
    int pipRet=0, len=0, ret=0;
    char ch2[2];
    char *cinfo=0;

    pllfd[0].fd = mfourPipEpt1[0];
    pllfd[0].events = POLLIN;

    while (1) {
        pipRet = poll(pllfd, 1, 500);
        if (pipRet <= 0) {
            continue;
        }

        ret = read(pllfd[0].fd, ch2, 2);
        if (ret != 2) {
            log_err("%s read failed line: %d \n", __func__, __LINE__);
            continue;;
        }

        log_dbg("wait and recv cmd [0x%.2x] [0x%.2x] \n", ch2[0], ch2[1]);

        if (ch2[0] != 's') {
            log_err("%s read char not match ch: 0x%.2x 0x%.2x line: %d \n", __func__, ch2[0], ch2[1], __LINE__);
            continue;;
        }

        len = ch2[1];

        cinfo = malloc(256);
        if (!cinfo) {
            log_err("%s malloc failed line: %d \n", __func__, __LINE__);
            continue;
        }

        pipRet = read(pllfd[0].fd, cinfo, len);
        if (pipRet != len) {
            log_err("%s read failed len: %d line: %d \n", __func__, len, __LINE__);

            free(cinfo);
            continue;
        }

        break;
    }

    return cinfo;
}

//*****************************************************************************
//  wait_rjob_rsp
//  blocking when cmd not reveived
//*****************************************************************************
void *wait_rjob1_rsp(void)
{
    struct pollfd pllfd[2]={0};
    int pipRet=0, len=0;
    char ch2[2];
    char *cinfo=0;

    pllfd[0].fd = mfourPipEpt1[0];
    pllfd[0].events = POLLIN;

    while (1) {
        pipRet = poll(pllfd, 1, 500);
        if (pipRet <= 0) {
            continue;
        }

        pipRet = read(pllfd[0].fd, ch2, 2);
        if (pipRet != 2) {
            log_err("%s read failed line: %d \n", __func__, __LINE__);
            continue;
        }

        log_dbg("wait and recv rsp [0x%.2x] [0x%.2x] \n", ch2[0], ch2[1]);
        
        if (ch2[0] != 's') {
            log_err("%s read char not match ch: %c line: %d \n", __func__, ch2[0], __LINE__);
            continue;
        }

        len = ch2[1];

        cinfo = malloc(256);
        if (!cinfo) {
            log_err("%s malloc failed line: %d \n", __func__, __LINE__);
            continue;
        }

        pipRet = read(pllfd[0].fd, cinfo, len);
        if (pipRet != len) {
            log_err("%s read failed len: %d line: %d \n", __func__, len, __LINE__);

            free(cinfo);
            continue;
        }

        break;
    }

    return cinfo;
}

void remove_rjob1_rsp(void)
{
    #if 0
    AQ_DropFront(&rjob1_queue);
    #endif
}

//*****************************************************************************
//  send_rjob_rsp
//  send "cmd" response "rsp" to host
//  Note :  resmgr_ept.dest_addr is 0xfffffff when begining ,
//          it need to receive a command from A7 to update it to correct value
//          otherwise OPENAMP_send() will fail
//*****************************************************************************
int send_rjob_rsp( t_rjob_cmd *rsp)
{
    int ret=0;
    char *cpysrc=0, *cpydst=0;
    
    if( rsp->mPtr != 0 ) {
        //rsp->mPtr = rsp->mPtr - (int)__RJob_ShareMem;
        rsp->dPtr = Rjob_RX_mem;
        
        cpysrc = rsp->mPtr;
        cpydst = rsp->dPtr;

        memcpy(cpydst, cpysrc, rsp->dSize);
    }

    ret = OPENAMP_send(mfourPipEpt2, rsp, sizeof(t_rjob_cmd));
    if (ret < 0) {
        log_err("Failed to send rspmessage ret=%d \n", ret);
    }
    
    log_info("rjob  send rsp %d byte (cmd=%04x tag=%04lx rsp=%04x dsize=%5d dPtr=%08x mPtr=%08x)\r\n",
            sizeof(t_rjob_cmd),
            rsp->cmd,
            rsp->tag,
            rsp->rsp,
            rsp->dSize,
            (int)rsp->dPtr,
            (int)rsp->mPtr);
            
    return 0;
}

//request two image blocks first for every new page in
void cmd_BKCMD_IMAGE_IN_handler( t_rjob_cmd *pcmd )
{
    t_rjob_cmd Bkjobcmd;
    t_rjob_cmd *prsp;
    t_areas *areaP;
    t_ImageParam *BKimage_param, *DstImageIP_Param;
	int	ReqImgSize;

    log_dbg("%s imgin enter line %d\n", __func__, __LINE__);

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
    
    log_dbg("%s send RSP line %d dst: 0x%.8x src: 0x%.8x \n", __func__, __LINE__, DstImageIP_Param, BKimage_param);
    
    //Duplicate_imageparam(BKimage_param, DstImageIP_Param);

    memcpy(DstImageIP_Param, BKimage_param, sizeof(t_ImageParam));

    log_dbg("%s dup line %d\n", __func__, __LINE__);
    
#if DebugBMPNmae
	log_dbg("[BMP]file_name: %s\n", BKimage_param->BMPfilename);
#endif
    Bkjobcmd.tag 	= 0x11;
    Bkjobcmd.cmd 	= BKCMD_REQUIRE_AREA;
    Bkjobcmd.dPtr 	= 0;
    Bkjobcmd.rsp 	= 0;
    Bkjobcmd.dSize 	= sizeof(t_ImageParam);
    Bkjobcmd.mPtr 	= (void *)RJob_TX_ShareMem;
	
/*    log_info("BKCMD_IMAGE_IN pImg=%X w=%d,h=%d TX=%X\r\n",
            (int)BKimage_param,BKimage_param->w, pImg->h ,
            (unsigned int)RJob_TX_ShareMem);
*/
/////////////////////////////////// 1ST image Box
	DstImageIP_Param->iJobIdx=ChkBox1L;
	SetCHKBoxReqArea(DstImageIP_Param);

    log_dbg("%s checkbox line %d\n", __func__, __LINE__);
    
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

       log_dbg("%s require line %d\n", __func__, __LINE__);
    
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

    log_dbg("%s area enter line %d\n", __func__, __LINE__);
    
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
			ReqImgCmd.mPtr	= (void *)RJob_TX_ShareMem;
					
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

    log_info("[R] %s line: %d input: %d\n", __func__, __LINE__, id);
    
    //printf("[R] line:%d id: %d\n", __LINE__, id);
    return 0;
}

//*****************************************************************************
//  cmd_dispatcher
//  Note :
//      command should be processed one by one in order ,
//      or copy pcmd->dPtr data to another buffer
//      otherwise data will corrupted
//*****************************************************************************
void cmd_dispatcher(t_rjob_cmd *pcmd)
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

int mfourmaind(void) 
{
    t_rjob_cmd *pcmd=0;

    log_info("%s ENTER line %d [0x%.8x]\n", __func__, __LINE__, RJob_TX_ShareMem);

    if (RJob_TX_ShareMem == 0) {
        RJob_TX_ShareMem = (unsigned int)malloc(64*1024);
    }
    
    pcmd = wait_rjob_cmd();

    if (Rjob_RX_mem == 0) {
        if (pcmd->dPtr) {
            Rjob_RX_mem = pcmd->dPtr;
        }
    }

    cmd_dispatcher(pcmd);

    free(pcmd);
    
    log_info("%s LEAVE line %d \n", __func__, __LINE__);    
    
    return 0;
}

