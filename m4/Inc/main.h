

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
//#include "stm32mp1xx_hal.h"
//#include "openamp.h"
//#include "lock_resource.h"

//#include "stm32mp15xx_disco.h"
//#include "log.h"
#include <linux/types.h>
#include <stdio.h>
#include <stdlib.h>


/* Define   ------------------------------------------------------------------*/
#define log_dbg(fmt, ...)  printf("[DBG]" fmt, ##__VA_ARGS__)
#define log_info(fmt, ...)   printf("[INF]" fmt, ##__VA_ARGS__)
#define log_err(fmt, ...)   printf("[ERR]" fmt, ##__VA_ARGS__)
#define log_warn(fmt, ...)   printf("[WRN]" fmt, ##__VA_ARGS__)
#define log_sys(fmt, ...)   printf("[SYS]" fmt, ##__VA_ARGS__)

#define RJOB_MAX_BUFSIZE    (16384)
#define RJOB_TX_OFFSET      (16384)



//*****************************************************************************
//  command code define
//*****************************************************************************
#define BKCMD_ABORT              0x0030
#define BKCMD_IMAGE_IN           0x0031
#define BKCMD_IMAGE_IN_RSP       0x0131
#define BKCMD_REQUIRE_AREA       0x8032      // bit15=1=rjob1
#define BKCMD_REQUIRE_AREA_RSP   0x8132      // bit15=1=rjob1
#define BKCMD_SEND_AREA          0x0033
#define BKCMD_DONE_AREA          0x8034      // bit15=1=rjob1
#define BKCMD_DONE_AREA_RSP      0x8134      // bit15=1=rjob1
#define BKCMD_IMAGE_COMPLETE     0x8035      // bit15=1=rjob1
#define BKCMD_IMAGE_COMPLETE_RSP 0x8135      // bit15=1=rjob1

/* Exported types ------------------------------------------------------------*/
typedef struct
{
    unsigned char rjob0_rx_status;
    unsigned char rjob1_rx_status;
    unsigned char tx_status;
    int rjob0_rx_len;
    int rjob1_rx_len;
    int tx_len;
}   t_rpmsg_status;


typedef struct __attribute__((__packed__))
{
    long            msgtype;    // for msg , don't modify
    unsigned long   tag;        // for user
    unsigned short  cmd;        // for user
    unsigned short  rsp;        // for user
    unsigned short  dSize;      // for user, attached data size/buffer size
    void *          dPtr;       // for user, attached data_ptr
    void *          mPtr;       // Driver internal use
}   t_rjob_cmd;


typedef struct
{
    unsigned short x;
    unsigned short y;
    unsigned short w;
    unsigned short h;
}   t_rect;

typedef struct
{
    unsigned short total_area;
    t_rect         area[0];
}   t_areas;

/*
typedef struct
{
    unsigned short w;
    unsigned short h;
    unsigned short idx;
    unsigned short jobid;
    unsigned short reserved;
    char           filename[32];
    unsigned char  data[0];
}   t_image_param;          // used in BKCMD_SEND_AREA , BKCMD_DONE_AREA
*/

//
//this data structure is used for passs the image parameter in the Jobcmd.
//the host send the image block information with this data structure
/*
	FileImageRect: the rect infor of the input BMPfile, the orginal point (0,0) is the bottom left cornor. 
	SeqIdx:
	iJobIdx:	is uased to refer to the ImageRect, assigned for returnning job id 
	ImageRect:	valid if width not equal to zero
    iJobRtnCode:	The return Status code for the iJobIdx
	IpPAMemorySize: //The allocated memory size
	*AttImgData: the offset address of the image data to the ImageRect or FileImageRect, based on the dPtr or mPtr 
*/
/*
typedef struct		//total 
{
	Imgheader			FileImageRect;	//the rect infor of the input file, the orginal point (0,0) is the bottom left cornor. 
    int			 		SeqIdx;			//the sequence number during proceesing for the current image file
    int			 		iJobIdx;		//assigned for current jimage processing job or the chained next processsing job
	Imgheader			ImageRect;		//valid if width not equal to zero
    int			 		iJobRtnCode;	//The return code for the iJobIdx
    int			 		IpPAMemorySize;	//The allocated memory size
    unsigned char  		*AttImgData;    //usually, t_imageIP	
}   t_ImageParam;          // used in BKCMD_SEND_AREA , BKCMD_DONE_AREA
*/



/* Exported constants --------------------------------------------------------*/

extern unsigned char __RJob_ShareMem[];
extern unsigned char __RJob_ShareMem_end[];
#define RJOB_RX_SIZE (((unsigned int)(__RJob_ShareMem_end-__RJob_ShareMem))/2)
#define RJOB_TX_SIZE (((unsigned int)(__RJob_ShareMem_end-__RJob_ShareMem))/2)

#define RJob_TX_ShareMem &(__RJob_ShareMem[RJOB_RX_SIZE])


/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);
void wait_rjob_cmd();
int send_rjob_rsp( t_rjob_cmd *rsp );
int send_rjob_cmd( t_rjob_cmd *pcmd );
void *wait_rjob1_rsp();
void remove_rjob1_rsp();
void cmd_dispatcher( t_rjob_cmd *p_rjob_cmd );

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
