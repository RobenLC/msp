Project: BKjobtest, 20200623

//20200701: revised
/*
1. modify code to use log_dbg instead of log_info for debug only. 
	.debug: need to set #define LOGLEVEL LOGDBG in log.h file
2. add #define	DebugBMPNmae 1 to show filename of bmp file.
	.need to upgrade this in bkjobtest in A7 
3. improve the checking in Get_CHKP_NTD_Pos() for filtering out the wrong result of checking marking.
4. add pixel counting in the SRN image box to filtering out the wrong result of getting SRN box.
5. use log_err() to show the error when get memory allocation.

*/

summary:

- this project can be used for testing the image transfer between A7 and M4.
- a device node is implemented to pass the command and data between A7 and M4
- 

//*****************************************************************************
//  command code defined
//*****************************************************************************
#define BKCMD_ABORT             0x0030
#define BKCMD_IMAGE_IN          0x0031
#define BKCMD_IMAGE_IN_RSP      0x0131
#define BKCMD_REQUIRE_AREA      0x8032      // bit15=1=rjob1
#define BKCMD_REQUIRE_AREA_RSP  0x8132      // bit15=1=rjob1
#define BKCMD_SEND_AREA         0x0033
#define BKCMD_DONE_AREA         0x8034      // bit15=1=rjob1
#define BKCMD_DONE_AREA_RSP     0x8134      // bit15=1=rjob1
#define BKCMD_IMAGE_COMPLETE    0x8035      // bit15=1=rjob1
#define BKCMD_IMAGE_COMPLETE_RSP 0x8135      // bit15=1=rjob1

//////////////////////////// commands //////////////////////////////

- BKCMD_IMAGE_IN: A7 notify the M4 that a new image of a note is in
- BKCMD_REQUIRE_AREA: M4 requires an image rectangle from A7
- BKCMD_SEND_AREA: A7 send the required image information to M4
- BKCMD_DONE_AREA: M4 notify A7 that the image data is received.
- BKCMD_IMAGE_COMPLETE: M4 notify that all image processing for the current Bank note is completed.
						A7 may discard the current bank note and proceed the next one.


////////////////////////// the data structures //////////////////////////////////////////////

typedef struct __attribute__((__packed__))
{
    long            msgtype;    // for msg , don't modify
    unsigned long   tag;        // for user
    unsigned short  cmd;        // for user
    unsigned short  rsp;        // for user
    unsigned short  dSize;      // for user, attached data size/buffer size
    void *          dPtr;       // for user, attached data_ptr
    void *          mPtr;       // Driver internal use
}   t_bk_cmd;

typedef struct		//total 
{
	t_Imgheader		BKNoteImageRect;	//the rectangle information of the BKNote, the orginal point (0,0) is the bottom left cornor.
    int			 	SeqIdx;				//the sequence number during proceesing for the current image file
    int			 	iJobIdx;			//assigned for the image processing job
	t_Imgheader		ImageRect;			//the rectagle information for the image using in the assigned iJobIdx, valid if width not equal to zero
    int			 	iJobRtnCode;		//The return status code for the iJobIdx
    int			 	IpPAMemorySize;		//The size of the attached memory 
//the following AttImgData is not implemented in current revision 							
    unsigned char  	*AttImgData;    	//usually, pointer to the t_imageIP	if there is an image data attached
}   t_ImageParam;       

typedef struct  
{
	int ry;				//Height,y, 	// Rows and columns in the image 
	int	cx;             //Width,x
	int oxj; 			//x, 		// Origin 
	int	oyi;            //y, 
}	t_Imgheader;


///////////////////////////////////// attached pararmeter and header /////////////////////////////////////////////////////

- the HostA7 side is the application in the A7 process
- The ImageProcess side is the application in the M4.
- for the communication without image data, the *dPtr or *mPtr must attached the t_ImageParam
- for the commnnication with image data, the *dPtr or *mPtr attached the t_ImageParam and the image data
- the image processign job information is updated in the t_ImageParam and to be passed between A7 and M4.
- The original point of the coordinate system in the application is at the bottom left of the image.
- BKNoteImageRect is the rectangle information of the note after deskewing and cropping.
	The function rotateBMPMf() in rotate.c return the width and height in cropinfo[4] and cropinfo[5]
- The SeqIdx is maintained in M4 to keep track of the sequence of the processing image. The program in HostA7 sould not 
	alter it. 
- The iJobIdx is maintained in the ImageProcess to keep track of the job function of the processing image. The 
	HostA7 should not alter it.
- The ImageRect keep the rectangle information for the image using with the assigned iJobIdx.  
- For the BKCMD_REQUIRE_AREA, the HostA7 needs to fetch the image rectangle from the t_ImageParam->BKNoteImageRect based on the 
	t_ImageParam->ImageRect. 
- The t_image_param is aleaus at the first location of t_bk_cmd->dPtr 
	and followed with image data for the rectangle of t_ImageParam->ImageRect, if the image data is requested or 
	returned.


////////////////////////////////  Example of preparing the image data 

		SrcBKJobImg_Param= BKJobcmd->dPtr;			//get the previous Parameter data into t_ImageParam
		SrcBKJob_Param=(t_ImageParam *)SrcBKJob_Param;
		
		ImageIP_Param_2M4 = (unsigned char	*)AllocateImgParamImgData(SrcBKJob_Param);	
		//AllocateImgParamImgData(): allocate memory of sizeof(t_ImageParam and size of image data.
		DstImageData=(unsigned char *)(ImageIP_Param_2M4+sizeof(t_ImageParam));
		DstBKJob_Param=(t_ImageParam *)ImageIP_Param_2M4;
		allBufSize=DstBKJob_Param->IpPAMemorySize;
		BKrjob_cmd.dSize = allBufSize;		//if error happpened, the allBufSize=0	
		DstImgHeader=(t_Imgheader *)&DstBKJob_Param->ImageRect;
		DstBKJob_Param->iJobRtnCode=iJobOK;
		
		DstImageData=(unsigned char *)(ImageIP_Param_2M4+sizeof(t_ImageParam));
		rotbuff = DstImageData;			//Destination image data
		.....
		rotateBMPMf(rotbuff, CPED_BMP_header, cropinfo, bmpsrc, pmreal, 0xFF, 0);	//#define V_FLIP_EN (0)
		.....
		BKrjob_cmd.cmd = BKCMD_SEND_AREA;
		BKrjob_cmd.tag = 1234;		//not usd for now
		BKrjob_cmd.rsp = 0;
		BKrjob_cmd.dPtr  = ImageIP_Param_2M4;
		bkjob_send_cmd( chan->mqPostman, &BKrjob_cmd );

//////////////////////////////////////////////////////////////////////////////////////////////////////////


//*****************************************************************************
//	iJobIdx : Image Job idx
//
// iJob_idx is attached for the current job or for the chained next job
//*****************************************************************************
#define ImgProc_None             	0
#define NewImageIn             		1
#define NewAreaReq             		2
#define ImageProcDone             	3
#define ChkBox1L             		4
#define ChkBox1R             		5
#define SRNBox1L             		6
#define SRNBox1R             		7
#define ChkBox2L             		8
#define ChkBox2R             		9
#define SRNBox2L             		10
#define SRNBox2R             		11


//*****************************************************************************
//	iJobRtnCode: Status code
// 
//*****************************************************************************
#define iJobOK						0
#define iJobImgInquirey				1
#define iJobImgComplete				2	//done with current paper
#define ImageOverSize				3
#define ImageZeroSize				4
#define iJobCurDone					5
#define iJobSaveIntPM				6
#define iJobImgOCR					7 //check OCR, //done with current paper
#define iJobImgMatch				8
#define iJobImgNoMatch				9 //done with current paper

iJobIdx: The job ID passed between M4 and A7 nto identify which job is cuurent being working on.
	. NewImageIn is sent by A7 to indicate a new bank note coming
	. NewAreaReq: not used for now
	. ChkBox1L to SRNBox2R: sent by M4 to request a new area, A7 side can not alter it. so the M4 know which image 
		block is returrned.
		
iJobRtnCode: 	The status returned by M4.
	. iJobOK: the current processing is OK.
	. iJobImgInquirey: request new image block. A7 need to get image block based on t_ImageParam->ImageRect. 
	. iJobImgComplete: All image processing is done for this bank note and no further image block required.
	. iJobImgOCR: can process the OCR function from the image in t_ImageParam->ImageRect.
	. others: no process needed.

	* current implementation: the checking is ended by the iJobRtnCode of both iJobImgComplete and iJobImgOCR.
							  one with checking of OCR. and the other doesn't. It could be the back side or other 
							  unknowing situation.
///////////////////////////////////////////////////////////////////////////////////////////////////
	if (BKJob_Param->iJobRtnCode==iJobImgOCR)
	{
		if (BKJob_Param->iJobIdx==iJobImgOCR)
		{
			if (BKJob_Param->iJobIdx==SRNBox1L)		//left side/up : upside down
			{
				log_i( "BKOCR_Check6 !!!!\r\n" );
				BKOCR_Check3( NewFileHeaderPtr, BKFileSize, ocr_result, MaxCount_SRN );
				log_i( " OCR Data = %s\r\n", ocr_result );
				memcpy( CurrentOCRFile, ocr_result, MaxCount_SRN );
				CurrentOCRFile=CurrentOCRFile+strlen(ocr_result);
				CurrentOCRFileCount=CurrentOCRFileCount+strlen(ocr_result);
			}
			if (BKJob_Param->iJobIdx==SRNBox1R)		//right side/Down: mirror
			{
				log_i( "BKOCR_Check7!!!!\r\n" );
				BKOCR_Check7( NewFileHeaderPtr, BKFileSize, ocr_result, MaxCount_SRN );
				log_i( " OCR Data = %s\r\n", ocr_result );
				memcpy( CurrentOCRFile, ocr_result, MaxCount_SRN );
				CurrentOCRFile=CurrentOCRFile+strlen(ocr_result);
				CurrentOCRFileCount=CurrentOCRFileCount+strlen(ocr_result);
			}
		}
	}
//////////////////////////////////////
//*****************************************************************************
//  BKOCR_Check6will load upA_model_hv.txt ( vertical flip )
//*****************************************************************************
extern "C" void *BKOCR_Check6( void * img_buf, int img_size, char *out_buf, int buf_size )
{
    static void *p_model=NULL;

    if( p_model == NULL )
        p_model = bkocr_load_model( 0 );

    if( p_model == NULL )
    {
        ocr_log_i( "Load OCR model fail !!!\n" );
        return 0;
    }
    bkocr( img_buf, img_size, out_buf, buf_size, p_model );
    return 0;
}

//*****************************************************************************
//  BKOCR_Check7 will load downA_model_hv.txt ( horizontal flip )
//*****************************************************************************
extern "C" void *BKOCR_Check7( void * img_buf, int img_size, char *out_buf, int buf_size )
{
    static void *p_model=NULL;

    if( p_model == NULL )
        p_model = bkocr_load_model( 1 );

    if( p_model == NULL )
    {
        ocr_log_i( "Load OCR model fail !!!\n" );
        return 0;
    }
    bkocr( img_buf, img_size, out_buf, buf_size, p_model );
    return 0;

}


////////////////////////////////M4/////////////////////////////////////////////////
void cmd_BKCMD_IMAGE_IN_handler( t_rjob_cmd *pcmd )
{
    Bkjobcmd = *pcmd;
    Bkjobcmd.cmd = BKCMD_IMAGE_IN_RSP;
    Bkjobcmd.dSize = 0;
    Bkjobcmd.dPtr = NULL;
    Bkjobcmd.mPtr = 0;
    send_rjob_cmd( &Bkjobcmd );  //response to the Image_In command


	DstImageIP_Param=(t_ImageParam *)RJob_TX_ShareMem;
    BKimage_param = (t_ImageParam *)pcmd->mPtr;
	Duplicate_imageparam(BKimage_param, DstImageIP_Param);

    Bkjobcmd.tag 	= 0x11;
    Bkjobcmd.cmd 	= BKCMD_REQUIRE_AREA;
    Bkjobcmd.dPtr 	= 0;
    Bkjobcmd.rsp 	= 0;
    Bkjobcmd.dSize 	= sizeof(t_ImageParam);
    Bkjobcmd.mPtr 	= RJob_TX_ShareMem;
	
/////////////////////////////////// 1ST image Box  //////
	DstImageIP_Param->iJobIdx=ChkBox1L;
	SetCHKBoxReqArea(DstImageIP_Param);
//	DstImageIP_Param->ImageRect.xc=80;
//	DstImageIP_Param->ImageRect.yr=144;
//	DstImageIP_Param->ImageRect.oyi=200;
//	DstImageIP_Param->ImageRect.oxj=40;
	send_rjob_cmd( &Bkjobcmd );
	prsp = wait_rjob1_rsp();
	log_info("	rsp cmd = %4x\r\n",prsp->cmd);
	remove_rjob1_rsp();
	
/////////////////////////////////// 2nd image Box   ////////
//	 not implementing/working. temp, implemented afyter 1ST image Box  done , if it needed
/* 
	DstImageIP_Param->iJobIdx=ChkBox1R;
	SetCHKBoxReqArea(DstImageIP_Param);
	send_rjob_cmd( &Bkjobcmd );
	prsp = wait_rjob1_rsp();
	log_info("	rsp cmd = %4x\r\n",prsp->cmd);
	remove_rjob1_rsp();
*/
	
}


////////////////////////////////////cmd_BKCMD_SEND_AREA_handler////////////////////////////////////////
void cmd_BKCMD_SEND_AREA_handler( t_rjob_cmd *pcmd )
{
	BkJobImg_Para  = pcmd->mPtr;
    doneCmd = *pcmd;
	SrcImage=pcmd->mPtr;
	
	ImageJobDispatch(&doneCmd);		//image processing 
	
	BkJobImg_Para  = doneCmd.mPtr;
	doneCmd.cmd = BKCMD_DONE_AREA;
	doneCmd.rsp = 0;
	send_rjob_cmd( &doneCmd );
    prsp = wait_rjob1_rsp();
    log_info("  rsp cmd = %4x sent\r\n",prsp->cmd);
    remove_rjob1_rsp();

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
    {	
        doneCmd.cmd = BKCMD_IMAGE_COMPLETE;
        doneCmd.rsp = 0;
        doneCmd.dSize = 0;
        doneCmd.dPtr = NULL;
        doneCmd.mPtr = 0;
        send_rjob_cmd( &doneCmd );
    }
}

/////////// if no image matched and no more work with current paper ////////
		SrcBKJob_Param->SeqIdx=0;
		SrcBKJob_Param->iJobIdx=ImageProcDone;
		SrcBKJob_Param->iJobRtnCode=iJobImgNoMatch;



