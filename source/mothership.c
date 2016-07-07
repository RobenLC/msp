#include <stdint.h> 
#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
//#include <getopt.h> 
#include <fcntl.h> 
#include <sys/ioctl.h> 
#include <sys/mman.h> 
#include <linux/types.h> 
#include <linux/spi/spidev.h> 
//#include <sys/times.h> 
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <dirent.h>
#include <sys/stat.h>  
#include <netdb.h>
#include <ifaddrs.h>

//main()
#define SPI1_ENABLE (0) 

#if SPI1_ENABLE
#define SPIDEV_SWITCH (0)
#else // spidev switch must be disable
#define SPIDEV_SWITCH (0)
#endif

#define MIN_SECTOR_SIZE (512)

#define PULL_LOW_AFTER_DATA (1)

#define SPI_CPHA  0x01          /* clock phase */
#define SPI_CPOL  0x02          /* clock polarity */
#define SPI_MODE_0      (0|0)
#define SPI_MODE_1      (0|SPI_CPHA)
#define SPI_MODE_2      (SPI_CPOL|0)
#define SPI_MODE_3      (SPI_CPOL|SPI_CPHA)
#if SPI1_ENABLE
static char spidev_1[] = "/dev/spidev32766.0"; 
#else //#if SPI1_ENABLE
static char *spidev_1 = 0;
#endif //#if SPI1_ENABLE
static char spidev_0[] = "/dev/spidev32765.0"; 
static int *totMalloc=0;
static int *totSalloc=0;
//static char netIntfs[16] = "uap0";
    
/* flow operation */
#define OP_PON            0x1                
#define OP_QRY            0x2
#define OP_RDY            0x3                
#define OP_SINGLE       0x4
#define OP_DOUBLE      0x5
#define OP_ACTION       0x6
#define OP_FIH             0x7
#define OP_BACK           0x8

/* SD read write operation */               
#define OP_SDRD            0x20
#define OP_SDWT            0x21
#define OP_STSEC_00     0x22
#define OP_STSEC_01     0x23
#define OP_STSEC_02     0x24
#define OP_STSEC_03     0x25
#define OP_STLEN_00     0x26
#define OP_STLEN_01     0x27
#define OP_STLEN_02     0x28
#define OP_STLEN_03     0x29
#define OP_SDAT             0x2a
#define OP_FREESEC       0x2b
#define OP_USEDSEC       0x2c
#define OP_SDINIT          0x2d
#define OP_SDSTATS       0x2e
#define OP_EG_DECT       0x2f
/* scanner parameters */
#define OP_MSG                0x30       
#define OP_FFORMAT        0x31
#define OP_COLRMOD        0x32
#define OP_COMPRAT        0x33
#define OP_RESOLTN         0x34
#define OP_SCANGAV        0x35
#define OP_MAXWIDH        0x36
#define OP_WIDTHAD_H    0x37
#define OP_WIDTHAD_L    0x38
#define OP_SCANLEN_H     0x39
#define OP_SCANLEN_L     0x3a
#define OP_INTERIMG       0x3b
#define OP_AFEIC             0x3c
#define OP_EXTPULSE       0x3d
#define OP_SUPBACK        0x3e
#define OP_LOG                0x3f

#define OP_RGRD              0x40
#define OP_RGWT              0x41
#define OP_RGDAT            0x42
#define OP_RGADD_H       0x43
#define OP_RGADD_L        0x44

#define OP_CROP_01        0x45
#define OP_CROP_02        0x46
#define OP_CROP_03        0x47
#define OP_CROP_04        0x48
#define OP_CROP_05        0x49
#define OP_CROP_06        0x4a

#define OP_IMG_LEN        0x4b

#define OP_META_DAT     0x4c

#define OP_AP_MODEN     0x50
/*
#define OP_DAT 0x08
#define OP_SCM 0x09
#define OP_DUL 0x0a
*/
#define OP_FUNCTEST_00              0x70
#define OP_FUNCTEST_01              0x71
#define OP_FUNCTEST_02              0x72
#define OP_FUNCTEST_03              0x73
#define OP_FUNCTEST_04              0x74
#define OP_FUNCTEST_05              0x75
#define OP_FUNCTEST_06              0x76
#define OP_FUNCTEST_07              0x77
#define OP_FUNCTEST_08              0x78
#define OP_FUNCTEST_09              0x79
#define OP_FUNCTEST_10              0x7A
#define OP_FUNCTEST_11              0x7B
#define OP_FUNCTEST_12              0x7C
#define OP_FUNCTEST_13              0x7D
#define OP_FUNCTEST_14              0x7E
#define OP_FUNCTEST_15              0x7F

/* debug */

#define OP_SAVE              0x80
#define OP_ERROR            0xe0

#define SPI_MAX_TXSZ  (1024 * 1024)
#define SPI_TRUNK_SZ   (32768)

/* kthread */
#define SPI_KTHREAD_USE    (1) 
#define SPI_UPD_NO_KTHREAD     (0)
#define SPI_KTHREAD_DLY    (0)
#define SPI_TRUNK_FULL_FIX (0)

#define DIR_POOL_SIZE (20480)

#define MAX_PDF_H  (900.0)
#define MAX_PDF_W (1600.0)

#define JPG_FFD9_CUT (1)

#define CROP_USE_META (1)
#define SCANGO_CHECK (1)

#define OUT_BUFF_LEN  (64*1024)

static FILE *mlog = 0;
static struct logPool_s *mlogPool;

typedef int (*func)(int argc, char *argv[]);

typedef enum {
    STINIT = 0,
    WAIT,
    NEXT,
    BREAK,
    BKWRD,
    FWORD,
    EVTMAX,
}event_e;

typedef enum {
    SPY = 0,
    BULLET,      // 1
    LASER,       //  2
    DOUBLEC,   // 3
    DOUBLED,   // 4
    REGE,         // 5
    REGF,
    FATG,
    FATH,
    SUPI,          // 9
    SINJ,          // a
    SAVK,         // b
    SDAL,         // c
    SDAM,        // d
    SDAN,        // e
    SDAO,        // f
    WTBAKP,     // 10
    WTBAKQ,     // 11
    CROPR,     // 11
    VECTORS, // 12
    SAVPARM, // 13
    METAT, // 14
    SMAX,
}state_e;

typedef enum {
    PSSET = 0,
    PSACT,
    PSWT,
    PSRLT,
    PSTSM,
    PSMAX,
}status_e;

typedef enum {
    ASPFS_TYPE_ROOT = 0x1,
    ASPFS_TYPE_DIR,
    ASPFS_TYPE_FILE,
} aspFStp_e;

typedef enum {
    ASPOP_STA_NONE = 0x0,
    ASPOP_STA_WR = 0x01,
    ASPOP_STA_UPD = 0x02,
    ASPOP_STA_APP = 0x04,
    ASPOP_STA_CON = 0x08,
    ASPOP_STA_SCAN = 0x10,
} aspOpSt_e;

typedef enum {
    ASPOP_CODE_NONE = 0,
    ASPOP_FILE_FORMAT,   /* 01 */
    ASPOP_COLOR_MODE,
    ASPOP_COMPRES_RATE,
    ASPOP_SCAN_SINGLE,
    ASPOP_SCAN_DOUBLE,
    ASPOP_ACTION,
    ASPOP_RESOLUTION,
    ASPOP_SCAN_GRAVITY,
    ASPOP_MAX_WIDTH,
    ASPOP_WIDTH_ADJ_H, /* 10 */
    ASPOP_WIDTH_ADJ_L,
    ASPOP_SCAN_LENS_H,
    ASPOP_SCAN_LENS_L,
    ASPOP_INTER_IMG,     
    ASPOP_AFEIC_SEL,     
    ASPOP_EXT_PULSE,     
    ASPOP_SDFAT_RD,      
    ASPOP_SDFAT_WT,
    ASPOP_SDFAT_STR01,
    ASPOP_SDFAT_STR02,  /* 20 */
    ASPOP_SDFAT_STR03,
    ASPOP_SDFAT_STR04,
    ASPOP_SDFAT_LEN01,
    ASPOP_SDFAT_LEN02,
    ASPOP_SDFAT_LEN03,
    ASPOP_SDFAT_LEN04,
    ASPOP_SDFAT_SDAT,
    ASPOP_REG_RD,
    ASPOP_REG_WT,
    ASPOP_REG_ADDRH, /* 30 */
    ASPOP_REG_ADDRL,
    ASPOP_REG_DAT,
    ASPOP_SUP_SAVE,
    ASPOP_SDFREE_FREESEC,
    ASPOP_SDFREE_STR01,
    ASPOP_SDFREE_STR02,
    ASPOP_SDFREE_STR03,
    ASPOP_SDFREE_STR04,
    ASPOP_SDFREE_LEN01,
    ASPOP_SDFREE_LEN02, /* 40 */
    ASPOP_SDFREE_LEN03,
    ASPOP_SDFREE_LEN04,
    ASPOP_SDUSED_USEDSEC,
    ASPOP_SDUSED_STR01,
    ASPOP_SDUSED_STR02,
    ASPOP_SDUSED_STR03,
    ASPOP_SDUSED_STR04,
    ASPOP_SDUSED_LEN01,
    ASPOP_SDUSED_LEN02,
    ASPOP_SDUSED_LEN03, /* 50 */
    ASPOP_SDUSED_LEN04,
    ASPOP_FUNTEST_00,
    ASPOP_FUNTEST_01,
    ASPOP_FUNTEST_02,
    ASPOP_FUNTEST_03,
    ASPOP_FUNTEST_04,
    ASPOP_FUNTEST_05,
    ASPOP_FUNTEST_06,
    ASPOP_FUNTEST_07,
    ASPOP_FUNTEST_08, /* 60 */
    ASPOP_FUNTEST_09,
    ASPOP_FUNTEST_10,
    ASPOP_FUNTEST_11,
    ASPOP_FUNTEST_12,
    ASPOP_FUNTEST_13,
    ASPOP_FUNTEST_14,
    ASPOP_FUNTEST_15,
    ASPOP_CROP_01,
    ASPOP_CROP_02, /* 70 */
    ASPOP_CROP_03,
    ASPOP_CROP_04,
    ASPOP_CROP_05,
    ASPOP_CROP_06,
    ASPOP_IMG_LEN, /* must be here for old design */
    ASPOP_CROP_07,
    ASPOP_CROP_08,
    ASPOP_CROP_09,
    ASPOP_CROP_10,
    ASPOP_CROP_11, /* 80 */
    ASPOP_CROP_12,
    ASPOP_CROP_13,
    ASPOP_CROP_14,
    ASPOP_CROP_15,
    ASPOP_CROP_16,
    ASPOP_CROP_17,
    ASPOP_CROP_18,
    ASPOP_CROP_COOR_XH,
    ASPOP_CROP_COOR_XL,
    ASPOP_CROP_COOR_YH, /* 90 */
    ASPOP_CROP_COOR_YL,
    ASPOP_EG_DECT,
    ASPOP_AP_MODE,
    ASPOP_XCROP_GAT,
    ASPOP_XCROP_LINSTR,
    ASPOP_XCROP_LINREC,
    ASPOP_CODE_MAX, /* 94 */
} aspOpCode_e;

typedef enum {
    ASPOP_MASK_0 = 0x0,
    ASPOP_MASK_1 = 0x1,
    ASPOP_MASK_2 = 0x3,
    ASPOP_MASK_3 = 0x7,
    ASPOP_MASK_4 = 0xf,
    ASPOP_MASK_5 = 0x1f,
    ASPOP_MASK_6 = 0x3f,
    ASPOP_MASK_7 = 0x7f,
    ASPOP_MASK_8 = 0xff,
    ASPOP_MASK_16 = 0xffff,
    ASPOP_MASK_32 = 0xffffffff,
} aspOpMask_e;

typedef enum {
    ASPOP_TYPE_NONE = 0,
    ASPOP_TYPE_SINGLE,
    ASPOP_TYPE_MULTI,
    ASPOP_TYPE_VALUE,
} aspOpType_e;

typedef enum {
    SINSCAN_NONE=0,
    SINSCAN_WIFI_ONLY,
    SINSCAN_SD_ONLY,
    SINSCAN_WIFI_SD,
    SINSCAN_WHIT_BLNC,
    SINSCAN_USB,
    SINSCAN_DUAL_STRM,
    SINSCAN_DUAL_SD,
} singleScan_e;

typedef enum {
    DOUSCAN_NONE=0,
    DOUSCAN_WIFI_ONLY,
    DOUSCAN_WHIT_BLNC,
} doubleScan_e;

typedef enum {
    SUPBACK_NONE=0,
    SUPBACK_SD,
    SUPBACK_RAW,
    SUPBACK_FAT,
} supBack_e;

typedef enum {
    ACTION_NONE=0,
    ACTION_OPTION_01,
    ACTION_OPTION_02,
    ACTION_OPTION_03,
    ACTION_OPTION_04,
    ACTION_OPTION_05,
} actOption_e;

typedef enum {
    FILE_FORMAT_NONE=0,
    FILE_FORMAT_JPG,
    FILE_FORMAT_PDF,
    FILE_FORMAT_RAW,
    FILE_FORMAT_TIFF_I,
    FILE_FORMAT_TIFF_M,
} fileFormat_e;

typedef enum {
    RESOLUTION_NONE=0,
    RESOLUTION_1200,
    RESOLUTION_600,
    RESOLUTION_300, 
    RESOLUTION_200, 
    RESOLUTION_150, 
} resolution_e;

typedef enum {
    SDSTATS_ERROR=0,
    SDSTATS_OK,
} SDStatus_e;

typedef enum {
    APM_NONE=0,
    APM_AP,
    APM_DIRECT,
} APMode_e;

struct aspInfoSplit_s{
    char *infoStr;
    int     infoLen;
    struct aspInfoSplit_s *n;
};

struct aspWaitRlt_s{
    char *wtRlt; /* size == 16bytes */
    int  wtMs;
    int  wtComp;
    int  wtChan;
    struct mainRes_s *wtMrs;
};

struct aspConfig_s{
    uint32_t opStatus;
    uint32_t opCode;
    uint32_t opValue;
    uint32_t opMask;
    uint32_t opType;
    uint32_t opBitlen;
};

typedef enum {
    ASPFS_ATTR_READ_ONLY = 0x01,
    ASPFS_ATTR_HIDDEN = 0x02,
    ASPFS_ATTR_SYSTEM = 0x04,
    ASPFS_ATTR_VOLUME_ID = 0x08,
    ASPFS_ATTR_DIRECTORY = 0x10,
    ASPFS_ATTR_ARCHIVE = 0x20,
} aspFSattribute_e;

typedef enum {
    ASPFS_STATUS_NONE = 0,
    ASPFS_STATUS_ING,
    ASPFS_STATUS_EN,
    ASPFS_STATUS_DIS,
} aspFSstatus_e;

typedef enum {
    ASPMETA_POWON_INIT = 0,
    ASPMETA_SCAN_GO = 1,
    ASPMETA_SCAN_COMPLETE = 2,
    ASPMETA_CROP_300DPI = 3,
    ASPMETA_CROP_600DPI = 4,
} aspMetaParam_e;

typedef enum {
    ASPMETA_INPUT = 0,
    ASPMETA_OUTPUT = 1,
} aspMetaInoutFlag_e;

typedef enum {
    ASPMETA_FUNC_NONE = 0,
    ASPMETA_FUNC_CONF = 0b00000001,
    ASPMETA_FUNC_CROP = 0b00000010,
    ASPMETA_FUNC_IMGLEN = 0b00000100,
    ASPMETA_FUNC_SDFREE = 0b00001000,
    ASPMETA_FUNC_SDUSED = 0b00010000,
    ASPMETA_FUNC_SDRD = 0b00100000,
    ASPMETA_FUNC_SDWT = 0b01000000,
} aspMetaFuncbit_e;

struct apWifiConfig_s{
    char wfssid[36];
    int wfsidLen;
    char wfpsk[64];
    int wfpskLen;
};

struct supdataBack_s{
    struct supdataBack_s   *n;
    int supdataTot;
    int supdataUse;
    char supdataBuff[SPI_TRUNK_SZ];
};

struct directnFile_s{
    uint32_t   dftype;
    uint32_t   dfstats;
    char        dfLFN[256];
    char        dfSFN[12];
    int           dflen;
    uint32_t   dfattrib;
    uint32_t   dfcretime;
    uint32_t   dfcredate;
    uint32_t   dflstacdate;
    uint32_t   dfrecotime;
    uint32_t   dfrecodate;
    uint32_t   dfclstnum;
    uint32_t   dflength;
    struct directnFile_s *pa;
    struct directnFile_s *br;
    struct directnFile_s *ch;   

    struct adFATLinkList_s *fln;
};

struct folderQueue_s{
    struct directnFile_s *fdObj;
    struct folderQueue_s *fdnxt;
};

struct pipe_s{
    int rt[2];
};

typedef enum {
    ASPFAT_STATUS_INIT = 0x1,
    ASPFAT_STATUS_BOOT_SEC = 0x2,
    ASPFAT_STATUS_FS_INFO = 0x4,
    ASPFAT_STATUS_FAT = 0x8,
    ASPFAT_STATUS_ROOT_DIR = 0x10,
    ASPFAT_STATUS_FOLDER = 0x20,
    ASPFAT_STATUS_SDRD = 0x40,
    ASPFAT_STATUS_SDWT = 0x80,
    ASPFAT_STATUS_FATWT = 0x100,
    ASPFAT_STATUS_DFEWT = 0x200,
    ASPFAT_STATUS_BOOT = 0x400,
    ASPFAT_STATUS_DFECHK = 0x800,
    ASPFAT_STATUS_DFERD = 0x1000,
    ASPFAT_STATUS_SDWBK = 0x2000,
} aspFatStatus_e;


struct sdRaw_s{
    char rowBP[512];
};

struct sdbootsec_s{
    int secSt;              // status of boot sector 
    int secJpcmd;      // jump command to the boot program
    char secSysid[8];
    int secSize;          // 512
    int secPrClst;       // 4 8 16 32 64
    int secResv;        // M 
    int secNfat;         // should be 2
    int secTotal;        // total sectors
    int secIDm;         // must be 0xF8
    int secPrfat;         // sectors per FAT
    int secPrtrk;         // sectors per track
    int secNsid;          // number of sides
    int secNhid;          // number of hidden sectors
    int secExtf;           // extension flag, specify the status of FAT mirroring
    int secVers;          // File system version
    int secRtclst;        // indicate the cluster number of root dir
    int secFSif;           // indicate the sector number of FS info, will be 1 normally
    int secBkbt;          // indicate the offset sector number of backup boot sector
    int secPhdk;         // pyhsical disk number, should be 0x80
    int secExtbt;        // extended boot record signature, should be 0x29
    int secVoid;          // volume ID number
    char secVola[12]; // volume label
    char secFtyp[12];   // file system type in ascii
    int secSign;          // shall be 0x55 (BP510) and 0xAA (BP511)
    int secWhfat;        // indicate the sector of fat table
    int secWhroot;      // indicate the sector of root dir
    int secBoffset;
};

struct sdFSinfo_s{
    int finLdsn;            // lead ingnature, shall be 0x52 0x52 0x61 0x41
    int finStsn;             // structure signature, shall be 0x72 0x72 0x41 0x61
    int finFreClst;        // free cluster count
    int finNxtFreClst;   // next free cluster
    int finTrsn;             // shall be 0x00 0x00 0x55 0xaa
};

struct adFATSpaceInfo_s{
    uint32_t  ftfreeClst;
    uint32_t  ftusedClst;
    struct adFATLinkList_s *f;    
};

struct adFATLinkList_s{
    uint32_t ftStart;              // start cluster
    uint32_t ftLen;                // cluster length
    struct adFATLinkList_s *n;    
};

struct sdFATable_s{
    uint8_t *ftbFat1;
    uint32_t ftbLen;
    struct adFATLinkList_s *h;
    struct adFATLinkList_s *c;
    struct adFATSpaceInfo_s ftbMng;
};

struct sdParseBuff_s{
    int dirBuffMax;
    int dirBuffUsed;
    char *dirParseBuff;
};

struct sdDirPool_s{
    struct sdParseBuff_s parBuf;
    int dirMax;
    int dirUsed;
    struct directnFile_s   *dirPool;
};

struct sdFAT_s{
    int fatStatus;
    int fatRetry;
    struct sdbootsec_s   *fatBootsec;
    struct sdFSinfo_s     *fatFSinfo;
    struct sdFATable_s   *fatTable;
    struct directnFile_s   *fatRootdir;
    struct directnFile_s    *fatFileDnld;
    struct directnFile_s    *fatFileUpld;
    struct directnFile_s    *fatCurDir;
    struct sdDirPool_s    *fatDirPool;
    struct supdataBack_s *fatSupdata;
    struct supdataBack_s *fatSupcur;
};

struct psdata_s {
    uint32_t bkofw;
    uint32_t result;
    uint32_t ansp0;
    struct procRes_s *rs;
};

typedef int (*stfunc)(struct psdata_s *data);

struct logPool_s{
    char *pool;
    char *cur;
    int max;
    int len;
};

struct cmd_s{
    int  id;
    char str[16];
    func pfunc;
};

struct ring_s{
    int run;
    int seq;
};

struct ring_p{
    struct ring_s lead;
    struct ring_s dual;
    struct ring_s prelead;
    struct ring_s predual;
    struct ring_s folw;
    struct ring_s psudo;
};

struct socket_s{
    int listenfd;
    int connfd;
    struct sockaddr_in serv_addr; 
    struct sockaddr_in clint_addr; 
    struct addrinfo addr_in;
};

struct shmem_s{
    int totsz;
    int chksz;
    int slotn;
    char **pp;
    int svdist;
    struct ring_p *r;
    int lastflg;
    int lastsz;
    int dualsz; 
};

struct modersp_s{
    int m;
    int r;
    int d;
    int v;  
    int c;
};

struct info16Bit_s{
    uint8_t     inout;
    uint8_t     seqnum;
    uint8_t     opcode;
    uint8_t     data;
    uint32_t   opinfo;
};

struct machineCtrl_s{
    uint32_t seqcnt;
    struct info16Bit_s tmp;
    struct info16Bit_s cur;
    struct info16Bit_s get;
    struct modersp_s mch;
};

struct intMbs_s{
    union {
        uint32_t n;
        uint8_t d[4];
    };
};

struct aspMetaData{
  struct intMbs_s     FUNC_BITS;             // byte[4] 
  unsigned char  ASP_MAGIC[2];            //byte[6] "0x20 0x14"
  
  /* ASPMETA_FUNC_CONF = 0x1 */       /* 0b00000001 */
  unsigned char  FILE_FORMAT;             //0x31
  unsigned char  COLOR_MODE;              //0x32
  unsigned char  COMPRESSION_RATE;        //0x33
  unsigned char  RESOLUTION;              //0x34
  unsigned char  SCAN_GRAVITY;            //0x35
  unsigned char  CIS_MAX_WIDTH;           //0x36
  unsigned char  WIDTH_ADJUST_H;          //0x37
  unsigned char  WIDTH_ADJUST_L;          //0x38
  unsigned char  SCAN_LENGTH_H;           //0x39
  unsigned char  SCAN_LENGTH_L;           //0x3a
  unsigned char  INTERNAL_IMG;            //0x3b
  unsigned char  AFE_IC_SELEC;            //0x3c
  unsigned char  EXTNAL_PULSE;            //0x3d
  unsigned char  SUP_WRITEBK;             //0x3e
  unsigned char  OP_FUNC_00;              //0x70
  unsigned char  OP_FUNC_01;              //0x71
  unsigned char  OP_FUNC_02;              //0x72
  unsigned char  OP_FUNC_03;              //0x73
  unsigned char  OP_FUNC_04;              //0x74
  unsigned char  OP_FUNC_05;              //0x75
  unsigned char  OP_FUNC_06;              //0x76
  unsigned char  OP_FUNC_07;              //0x77
  unsigned char  OP_FUNC_08;              //0x78
  unsigned char  OP_FUNC_09;              //0x79
  unsigned char  OP_FUNC_10;              //0x7A
  unsigned char  OP_FUNC_11;              //0x7B
  unsigned char  OP_FUNC_12;              //0x7C
  unsigned char  OP_FUNC_13;              //0x7D
  unsigned char  OP_FUNC_14;              //0x7E
  unsigned char  OP_FUNC_15;              //0x7F  
  unsigned char  OP_RESERVE[28];          // byte[64]
  
  /* ASPMETA_FUNC_CROP = 0x2 */       /* 0b00000010 */
  struct intMbs_s CROP_POS_1;        //byte[68]
  struct intMbs_s CROP_POS_2;        //byte[72]
  struct intMbs_s CROP_POS_3;        //byte[76]
  struct intMbs_s CROP_POS_4;        //byte[80]
  struct intMbs_s CROP_POS_5;        //byte[84]
  struct intMbs_s CROP_POS_6;        //byte[88]
  struct intMbs_s CROP_POS_7;        //byte[92]
  struct intMbs_s CROP_POS_8;        //byte[96]
  struct intMbs_s CROP_POS_9;        //byte[100]
  struct intMbs_s CROP_POS_10;        //byte[104]
  struct intMbs_s CROP_POS_11;        //byte[108]
  struct intMbs_s CROP_POS_12;        //byte[112]
  struct intMbs_s CROP_POS_13;        //byte[116]
  struct intMbs_s CROP_POS_14;        //byte[120]
  struct intMbs_s CROP_POS_15;        //byte[124]
  struct intMbs_s CROP_POS_16;        //byte[128]
  struct intMbs_s CROP_POS_17;        //byte[132]
  struct intMbs_s CROP_POS_18;        //byte[136]
  unsigned char  Start_Pos_1st;         //byte[137]
  unsigned char  Start_Pos_2nd;        //byte[138]
  unsigned char  End_Pos_All;            //byte[139]
  unsigned char  Start_Pos_RSV;        //byte[140], not using for now
  unsigned char  YLine_Gap;               //byte[141]
  unsigned char  Start_YLine_No;       //byte[142]
  unsigned short YLines_Recorded;     //byte[144] 16bits
  unsigned char CROP_RESERVE[16]; //byte[160]

  /* ASPMETA_FUNC_IMGLEN = 0x4 */     /* 0b00000100 */
  struct intMbs_s SCAN_IMAGE_LEN;     //byte[164]
  
  /* ASPMETA_FUNC_SDFREE = 0x8 */     /* 0b00001000 */
  struct intMbs_s  FREE_SECTOR_ADD;   //byte[168]
  struct intMbs_s  FREE_SECTOR_LEN;   //byte[172]
  
  /* ASPMETA_FUNC_SDUSED = 0x16 */    /* 0b00010000 */
  struct intMbs_s  USED_SECTOR_ADD;   //byte[176]
  struct intMbs_s  USED_SECTOR_LEN;   //byte[180]
  
  /* ASPMETA_FUNC_SDRD = 0x32 */      /* 0b00100000 */
  /* ASPMETA_FUNC_SDWT = 0x64 */      /* 0b01000000 */
  struct intMbs_s  SD_RW_SECTOR_ADD;  //byte[184]
  struct intMbs_s  SD_RW_SECTOR_LEN;  //byte[188]
  
  unsigned char available[324];
};

struct aspMetaMass{
    int massUsed;
    int massMax;
    int massGap;
    int massRecd;
    int massStart;
    char *masspt;
};

struct mainRes_s{
    int sid[9];
    int sfm[2];
    int smode;
    struct psdata_s stdata;
    struct sdFAT_s aspFat;
    struct aspConfig_s configTable[ASPOP_CODE_MAX];
    struct folderQueue_s *folder_dirt;
    struct machineCtrl_s mchine;
    // 3 pipe
    struct pipe_s pipedn[10];
    struct pipe_s pipeup[10];
    // data mode share memory
    struct shmem_s dataRx;
    struct shmem_s dataTx; /* we don't have data mode Tx, so use it as cmdRx for spi1 */
    // command mode share memory
    struct shmem_s cmdRx; /* cmdRx for spi0 */
    struct shmem_s cmdTx;
    struct spi_ioc_transfer *spioc1;
    struct spi_ioc_transfer *spioc2;
    // file save
    FILE *fs;
    // file log
    FILE *flog;
    // time measurement
    struct timespec time[2];
    // log buffer
    char log[1024];
    struct socket_s socket_r;
    struct socket_s socket_t;
    struct socket_s socket_at;
    struct socket_s socket_n;
    struct socket_s socket_v;
    struct logPool_s plog;
    struct aspWaitRlt_s wtg;
    struct apWifiConfig_s wifconf;
    struct aspMetaData *metaout;
    struct aspMetaData *metain;
    struct aspMetaMass metaMass;
    char netIntfs[16];
    char *dbglog;
};

typedef int (*fselec)(struct mainRes_s *mrs, struct modersp_s *modersp);

struct fselec_s{
    int  id;
    fselec pfunc;
};

struct procRes_s{
    // pipe
    int spifd;
    struct psdata_s *pstdata;
    struct sdFAT_s *psFat;
    struct aspConfig_s *pcfgTable;
    struct pipe_s *ppipedn;
    struct pipe_s *ppipeup;
    struct shmem_s *pdataRx;
    struct shmem_s *pdataTx;
    struct shmem_s *pcmdRx;
    struct shmem_s *pcmdTx;
    struct spi_ioc_transfer *rspioc1;
    struct spi_ioc_transfer *rspioc2;

    struct machineCtrl_s *pmch;

    // data mode share memory
    int cdsz_s;
    int mdsz_s;
    char **dmp_s;
    // command mode share memory
    int ccsz_s;
    int mcsz_s;
    char **cmp_s;
    // save file
    FILE *fs_s;
    // save log file
    FILE *flog_s;
    // save data file
    FILE *fdat_s[3];
    // time measurement
    struct timespec *tm[2];
    struct timespec tdf[2];
    char logs[1024];
    struct socket_s *psocket_r;
    struct socket_s *psocket_t;
    struct socket_s *psocket_at;
    struct socket_s *psocket_n;
    struct socket_s *psocket_v;
    struct apWifiConfig_s *pwifconf;
    struct aspMetaData *pmetaout;
    struct aspMetaData *pmetain;
    struct aspMetaMass *pmetaMass;
    struct logPool_s *plogs;
    char *pnetIntfs;
};

//memory alloc. put in/put out
static char **memory_init(int *sz, int tsize, int csize);
//debug printf
static int print_f(struct logPool_s *plog, char *head, char *str);
static int printf_flush(struct logPool_s *plog, FILE *f);
static int print_dbg(struct logPool_s *plog, char *str, int size);
static int printf_dbgflush(struct logPool_s *plog, struct mainRes_s *mrs);
//time measurement, start /stop
static int time_diff(struct timespec *s, struct timespec *e, int unit);
//file rw open, save to file for debug
static int file_save_get(FILE **fp, char *path1);
static FILE *find_save(char *dst, char *tmple);
//res put in
static int res_put_in(struct procRes_s *rs, struct mainRes_s *mrs, int idx);
//p0: control, monitor, and debug
static int p0(struct mainRes_s *mrs);
static int p0_init(struct mainRes_s *mrs);
static int p0_end(struct mainRes_s *mrs);
//p1: spi0 send
static int p1(struct procRes_s *rs, struct procRes_s *rcmd);
static int p1_init(struct procRes_s *rs);
static int p1_end(struct procRes_s *rs);
//p2: spi0 recv
static int p2(struct procRes_s *rs);
static int p2_init(struct procRes_s *rs);
static int p2_end(struct procRes_s *rs);
//p3: spi1 recv
static int p3(struct procRes_s *rs);
static int p3_init(struct procRes_s *rs);
static int p3_end(struct procRes_s *rs);
//p4: socket send
static int p4(struct procRes_s *rs);
static int p4_init(struct procRes_s *rs);
static int p4_end(struct procRes_s *rs);
//p5: socket recv
static int p5(struct procRes_s *rs, struct procRes_s *rcmd);
static int p5_init(struct procRes_s *rs);
static int p5_end(struct procRes_s *rs);
//p6: file list recv/send
static int p6(struct procRes_s *rs);
static int p6_init(struct procRes_s *rs);
static int p6_end(struct procRes_s *rs);
//p7: socket send data for spi1 command mode
static int p7(struct procRes_s *rs);
static int p7_init(struct procRes_s *rs);
static int p7_end(struct procRes_s *rs);
//p8: get UDP broadcast and reply
static int p8(struct procRes_s *rs);
static int p8_init(struct procRes_s *rs);
static int p8_end(struct procRes_s *rs);

static int pn_init(struct procRes_s *rs);
static int pn_end(struct procRes_s *rs);
//IPC wrap
static int rs_ipc_put(struct procRes_s *rs, char *str, int size);
static int rs_ipc_get(struct procRes_s *rs, char *str, int size);
static int mrs_ipc_put(struct mainRes_s *mrs, char *str, int size, int idx);
static int mrs_ipc_get(struct mainRes_s *mrs, char *str, int size, int idx);
static int msp_spi_conf(int dev, int flag, void *bitset);
static int mtx_data(int fd, uint8_t *rx_buff, uint8_t *tx_buff, int pksz, struct spi_ioc_transfer *tr);

static int ring_buf_init(struct shmem_s *pp);
static int ring_buf_get_dual(struct shmem_s *pp, char **addr, int sel);
static int ring_buf_set_last_dual(struct shmem_s *pp, int size, int sel);
static int ring_buf_prod_dual(struct shmem_s *pp, int sel);
static int ring_buf_cons_dual(struct shmem_s *pp, char **addr, int sel);
static int ring_buf_get(struct shmem_s *pp, char **addr);
static int ring_buf_set_last(struct shmem_s *pp, int size);
static int ring_buf_prod(struct shmem_s *pp);
static int ring_buf_cons(struct shmem_s *pp, char **addr);
static int ring_buf_info_len(struct shmem_s *pp);
static int shmem_from_str(char **addr, char *dst, char *sz);
static int shmem_dump(char *src, int size);
static int shmem_pop_send(struct mainRes_s *mrs, char **addr, int seq, int p);
static int shmem_rlt_get(struct mainRes_s *mrs, int seq, int p);
static int stspy_01(struct psdata_s *data);
static int stspy_02(struct psdata_s *data);
static int stspy_03(struct psdata_s *data);
static int stspy_04(struct psdata_s *data);
static int stspy_05(struct psdata_s *data);
static int stbullet_01(struct psdata_s *data);
static int stbullet_02(struct psdata_s *data);
static int stbullet_03(struct psdata_s *data);
static int stbullet_04(struct psdata_s *data);
static int stbullet_05(struct psdata_s *data);
static int stlaser_01(struct psdata_s *data);
static int stlaser_02(struct psdata_s *data);
static int stlaser_03(struct psdata_s *data);
static int stlaser_04(struct psdata_s *data);
static int stlaser_05(struct psdata_s *data);
static int stdob_01(struct psdata_s *data);
static int stdob_02(struct psdata_s *data);
static int stdob_03(struct psdata_s *data);
static int stdob_04(struct psdata_s *data);
static int stdob_05(struct psdata_s *data);
static int stdob_06(struct psdata_s *data);
static int stdob_07(struct psdata_s *data);
static int stdob_08(struct psdata_s *data);
static int stdob_09(struct psdata_s *data);
static int stdob_10(struct psdata_s *data);
static int streg_11(struct psdata_s *data);
static int streg_12(struct psdata_s *data);
static int streg_13(struct psdata_s *data);
static int streg_14(struct psdata_s *data);
static int streg_15(struct psdata_s *data);
static int streg_16(struct psdata_s *data);
static int streg_17(struct psdata_s *data);
static int stfat_18(struct psdata_s *data);
static int stfat_19(struct psdata_s *data);
static int stfat_20(struct psdata_s *data);
static int stfat_21(struct psdata_s *data);
static int stfat_22(struct psdata_s *data);
static int stfat_23(struct psdata_s *data);
static int stfat_24(struct psdata_s *data);
static int stfat_25(struct psdata_s *data);
static int stfat_26(struct psdata_s *data);
static int stfat_27(struct psdata_s *data);
static int stfat_28(struct psdata_s *data);
static int stfat_29(struct psdata_s *data);
static int stfat_30(struct psdata_s *data);
static int stsup_31(struct psdata_s *data);
static int stsup_32(struct psdata_s *data);
static int stsup_33(struct psdata_s *data);
static int stsup_34(struct psdata_s *data);
static int stsup_35(struct psdata_s *data);
static int stsin_36(struct psdata_s *data);
static int stdow_37(struct psdata_s *data);
static int stupd_38(struct psdata_s *data);
static int stupd_39(struct psdata_s *data);
static int stupd_40(struct psdata_s *data);

static int mspFS_createRoot(struct directnFile_s **root, struct sdFAT_s *psFat, char *dir);
static int mspFS_insertChilds(struct sdFAT_s *psFat, struct directnFile_s *root);
static int mspFS_insertChildDir(struct sdFAT_s *psFat, struct directnFile_s *parent, char *dir);
static int mspFS_insertChildFile(struct sdFAT_s *psFat, struct directnFile_s *parent, char *str);
static int mspFS_list(struct directnFile_s *root, int depth);
static int mspFS_FileSearch(struct directnFile_s **dir, struct directnFile_s *root, char *path);
static int mspFS_showFolder(struct directnFile_s *root);
static int mspFS_folderJump(struct directnFile_s **dir, struct directnFile_s *root, char *path);
static int mspSD_parseFAT2LinkList(struct adFATLinkList_s **head, uint32_t idx, uint8_t *fat, uint32_t max);

static int mspFS_allocDir(struct sdFAT_s *psFat, struct directnFile_s **dir);
inline uint32_t mspSD_getNextFreeFAT(uint32_t idx, uint8_t *fat, uint32_t max) ;
static int aspFS_createFATRoot(struct sdFAT_s *pfat);
static int aspFS_insertFATChilds(struct sdFAT_s *pfat, struct directnFile_s *root, char *dir, int max);
static int aspFS_insertFATChild(struct directnFile_s *parent, struct directnFile_s *r);
static uint8_t aspFSchecksum(uint8_t *pch);
static char aspLnameFilter(char ch);
static char aspSnameFilterIn(char ch);
static char aspSnameFilterOut(char ch);
static uint32_t aspFSdateCps(uint32_t val);
static uint32_t aspFStimeCps(uint32_t val);
static int aspNameCpyfromRaw(char *raw, char *dst, int offset, int len, int jump);
static int aspNameCpyfromName(char *raw, char *dst, int offset, int len, int jump);
static int atFindIdx(char *str, char ch);

static int cmdfunc_opchk_single(uint32_t val, uint32_t mask, int len, int type);
static int cfgTableSet(struct aspConfig_s *table, int idx, uint32_t val);

static uint32_t lsb2Msb(struct intMbs_s *msb, uint32_t lsb)
{
    uint32_t org=0;
    int i=4;

    org = lsb;

    while (i) {
        i--;
        msb->d[i] = lsb & 0xff;

        //printf("[%d] :0x%.2x -> 0x%.2x \n", i, lsb & 0xff, msb->d[i]);
        
        lsb = lsb >> 8;
    }

    //printf("lsb2Msb() lsb:0x%.8x -> msb:0x%.8x \n", org, msb->n);

    return msb->n;
}

static uint32_t msb2lsb(struct intMbs_s *msb)
{
    uint32_t lsb=0;
    int i=0;

    while (i < 4) {
        lsb = lsb << 8;
        
        lsb |= msb->d[i];
        
        //printf("[%d] :0x%.2x <- 0x%.2x \n", i, lsb & 0xff, msb->d[i]);
        
        i++;
    }

    //printf("msb2lsb() msb:0x%.8x -> lsb:0x%.8x \n", msb->n, lsb);
    
    return lsb;
}


static int dbgMeta(unsigned int funcbits, struct aspMetaData *pmeta) 
{
    msync(pmeta, sizeof(struct aspMetaData), MS_SYNC);
    printf("********************************************\n");
    printf("[meta] debug print , funcBits: 0x%.8x, magic[0]: 0x%.2x magic[1]: 0x%.2x \n", funcbits, pmeta->ASP_MAGIC[0], pmeta->ASP_MAGIC[1]);

    if ((pmeta->ASP_MAGIC[0] != 0x20) || (pmeta->ASP_MAGIC[1] != 0x14)) {
        printf("[meta] Warning!!! magic[0]: 0x%.2x magic[1]: 0x%.2x \n", pmeta->ASP_MAGIC[0], pmeta->ASP_MAGIC[1]);
        printf("********************************************\n");
        return -2;
    }
    
    if (funcbits == ASPMETA_FUNC_NONE) {
        printf("********************************************\n");
        return -3;
    }

    if (funcbits & ASPMETA_FUNC_CONF) {
        printf("[meta]__ASPMETA_FUNC_CONF__(0x%x & 0x%x = 0x%x)\n", funcbits, ASPMETA_FUNC_CONF, (funcbits & ASPMETA_FUNC_CONF));
        printf("[meta]FILE_FORMAT: 0x%.2x    \n",pmeta->FILE_FORMAT     );          //0x31
        printf("[meta]COLOR_MODE: 0x%.2x      \n",pmeta->COLOR_MODE      );        //0x32
        printf("[meta]COMPRESSION_RATE: 0x%.2x\n",pmeta->COMPRESSION_RATE);   //0x33
        printf("[meta]RESOLUTION: 0x%.2x      \n",pmeta->RESOLUTION      );         //0x34
        printf("[meta]SCAN_GRAVITY: 0x%.2x    \n",pmeta->SCAN_GRAVITY    );       //0x35
        printf("[meta]CIS_MAX_Width: 0x%.2x   \n",pmeta->CIS_MAX_WIDTH   );        //0x36
        printf("[meta]WIDTH_ADJUST_H: 0x%.2x  \n",pmeta->WIDTH_ADJUST_H  );     //0x37
        printf("[meta]WIDTH_ADJUST_L: 0x%.2x  \n",pmeta->WIDTH_ADJUST_L  );      //0x38
        printf("[meta]SCAN_LENGTH_H: 0x%.2x   \n",pmeta->SCAN_LENGTH_H   );      //0x39
        printf("[meta]SCAN_LENGTH_L: 0x%.2x   \n",pmeta->SCAN_LENGTH_L   );       //0x3a
        printf("[meta]INTERNAL_IMG: 0x%.2x    \n",pmeta->INTERNAL_IMG    );         //0x3b
        printf("[meta]AFE_IC_SELEC: 0x%.2x    \n",pmeta->AFE_IC_SELEC    );         //0x3c
        printf("[meta]EXTNAL_PULSE: 0x%.2x    \n",pmeta->EXTNAL_PULSE    );         //0x3d
        printf("[meta]SUP_WRITEBK: 0x%.2x     \n",pmeta->SUP_WRITEBK     );       //0x3e
        printf("[meta]OP_FUNC_00: 0x%.2x      \n",pmeta->OP_FUNC_00      );     //0x70
        printf("[meta]OP_FUNC_01: 0x%.2x      \n",pmeta->OP_FUNC_01      );     //0x71
        printf("[meta]OP_FUNC_02: 0x%.2x      \n",pmeta->OP_FUNC_02      );     //0x72
        printf("[meta]OP_FUNC_03: 0x%.2x      \n",pmeta->OP_FUNC_03      );     //0x73
        printf("[meta]OP_FUNC_04: 0x%.2x      \n",pmeta->OP_FUNC_04      );     //0x74
        printf("[meta]OP_FUNC_05: 0x%.2x      \n",pmeta->OP_FUNC_05      );     //0x75
        printf("[meta]OP_FUNC_06: 0x%.2x      \n",pmeta->OP_FUNC_06      );     //0x76
        printf("[meta]OP_FUNC_07: 0x%.2x      \n",pmeta->OP_FUNC_07      );     //0x77
        printf("[meta]OP_FUNC_08: 0x%.2x      \n",pmeta->OP_FUNC_08      );     //0x78
        printf("[meta]OP_FUNC_09: 0x%.2x      \n",pmeta->OP_FUNC_09      );     //0x79
        printf("[meta]OP_FUNC_10: 0x%.2x      \n",pmeta->OP_FUNC_10      );     //0x7A
        printf("[meta]OP_FUNC_11: 0x%.2x      \n",pmeta->OP_FUNC_11      );     //0x7B
        printf("[meta]OP_FUNC_12: 0x%.2x      \n",pmeta->OP_FUNC_12      );     //0x7C
        printf("[meta]OP_FUNC_13: 0x%.2x      \n",pmeta->OP_FUNC_13      );     //0x7D
        printf("[meta]OP_FUNC_14: 0x%.2x      \n",pmeta->OP_FUNC_14      );     //0x7E
        printf("[meta]OP_FUNC_15: 0x%.2x      \n",pmeta->OP_FUNC_15      );     //0x7F  
    }
    
    if (funcbits & ASPMETA_FUNC_CROP) {
        printf("[meta]__ASPMETA_FUNC_CROP__(0x%x & 0x%x = 0x%x)\n", funcbits, ASPMETA_FUNC_CROP, (funcbits & ASPMETA_FUNC_CROP));
        printf("[meta]CROP_POSX_01: %d, %d\n", msb2lsb(&pmeta->CROP_POS_1) >> 16, msb2lsb(&pmeta->CROP_POS_1) & 0xffff);                      //byte[68]
        printf("[meta]CROP_POSX_02: %d, %d\n", msb2lsb(&pmeta->CROP_POS_2) >> 16, msb2lsb(&pmeta->CROP_POS_2) & 0xffff);                      //byte[72]
        printf("[meta]CROP_POSX_03: %d, %d\n", msb2lsb(&pmeta->CROP_POS_3) >> 16, msb2lsb(&pmeta->CROP_POS_3) & 0xffff);                      //byte[76]
        printf("[meta]CROP_POSX_04: %d, %d\n", msb2lsb(&pmeta->CROP_POS_4) >> 16, msb2lsb(&pmeta->CROP_POS_4) & 0xffff);                      //byte[80]
        printf("[meta]CROP_POSX_05: %d, %d\n", msb2lsb(&pmeta->CROP_POS_5) >> 16, msb2lsb(&pmeta->CROP_POS_5) & 0xffff);                      //byte[84]
        printf("[meta]CROP_POSX_06: %d, %d\n", msb2lsb(&pmeta->CROP_POS_6) >> 16, msb2lsb(&pmeta->CROP_POS_6) & 0xffff);                      //byte[88]
        printf("[meta]CROP_POSX_07: %d, %d\n", msb2lsb(&pmeta->CROP_POS_7) >> 16, msb2lsb(&pmeta->CROP_POS_7) & 0xffff);                      //byte[92]
        printf("[meta]CROP_POSX_08: %d, %d\n", msb2lsb(&pmeta->CROP_POS_8) >> 16, msb2lsb(&pmeta->CROP_POS_8) & 0xffff);                      //byte[96]
        printf("[meta]CROP_POSX_09: %d, %d\n", msb2lsb(&pmeta->CROP_POS_9) >> 16, msb2lsb(&pmeta->CROP_POS_9) & 0xffff);                      //byte[100]
        printf("[meta]CROP_POSX_10: %d, %d\n", msb2lsb(&pmeta->CROP_POS_10) >> 16, msb2lsb(&pmeta->CROP_POS_10) & 0xffff);                      //byte[104]
        printf("[meta]CROP_POSX_11: %d, %d\n", msb2lsb(&pmeta->CROP_POS_11) >> 16, msb2lsb(&pmeta->CROP_POS_11) & 0xffff);                      //byte[108]
        printf("[meta]CROP_POSX_12: %d, %d\n", msb2lsb(&pmeta->CROP_POS_12) >> 16, msb2lsb(&pmeta->CROP_POS_12) & 0xffff);                      //byte[112]
        printf("[meta]CROP_POSX_13: %d, %d\n", msb2lsb(&pmeta->CROP_POS_13) >> 16, msb2lsb(&pmeta->CROP_POS_13) & 0xffff);                      //byte[116]
        printf("[meta]CROP_POSX_14: %d, %d\n", msb2lsb(&pmeta->CROP_POS_14) >> 16, msb2lsb(&pmeta->CROP_POS_14) & 0xffff);                      //byte[120]
        printf("[meta]CROP_POSX_15: %d, %d\n", msb2lsb(&pmeta->CROP_POS_15) >> 16, msb2lsb(&pmeta->CROP_POS_15) & 0xffff);                      //byte[124]
        printf("[meta]CROP_POSX_16: %d, %d\n", msb2lsb(&pmeta->CROP_POS_16) >> 16, msb2lsb(&pmeta->CROP_POS_16) & 0xffff);                      //byte[128]
        printf("[meta]CROP_POSX_17: %d, %d\n", msb2lsb(&pmeta->CROP_POS_17) >> 16, msb2lsb(&pmeta->CROP_POS_17) & 0xffff);                      //byte[132]
        printf("[meta]CROP_POSX_18: %d, %d\n", msb2lsb(&pmeta->CROP_POS_18) >> 16, msb2lsb(&pmeta->CROP_POS_18) & 0xffff);                      //byte[136]
        printf("[meta]YLine_Gap: %.d      \n",pmeta->YLine_Gap); 
        printf("[meta]Start_YLine_No: %d      \n",pmeta->Start_YLine_No); 
        printf("[meta]YLines_Recorded: %d      \n",msb2lsb((struct intMbs_s*)&pmeta->YLines_Recorded) >> 16); 
    }

    if (funcbits & ASPMETA_FUNC_IMGLEN) {
        printf("[meta]__ASPMETA_FUNC_IMGLEN__(0x%x & 0x%x = 0x%x)\n", funcbits, ASPMETA_FUNC_IMGLEN, (funcbits & ASPMETA_FUNC_IMGLEN));
        printf("[meta]SCAN_IMAGE_LEN: %d\n", msb2lsb(&pmeta->SCAN_IMAGE_LEN));                      //byte[124]        
    }

    if (funcbits & ASPMETA_FUNC_SDFREE) {      
        printf("[meta]__ASPMETA_FUNC_SDFREE__(0x%x & 0x%x = 0x%x)\n", funcbits, ASPMETA_FUNC_SDFREE, (funcbits & ASPMETA_FUNC_SDFREE));
        printf("[meta]FREE_SECTOR_ADD: %d\n", msb2lsb(&pmeta->FREE_SECTOR_ADD));                      //byte[128]            
        printf("[meta]FREE_SECTOR_LEN: %d\n", msb2lsb(&pmeta->FREE_SECTOR_LEN));                      //byte[132]        
    }

    if (funcbits & ASPMETA_FUNC_SDUSED) {
        printf("[meta]__ASPMETA_FUNC_SDUSED__(0x%x & 0x%x = 0x%x)\n", funcbits, ASPMETA_FUNC_SDUSED, (funcbits & ASPMETA_FUNC_SDUSED));
        printf("[meta]USED_SECTOR_ADD: %d\n", msb2lsb(&pmeta->USED_SECTOR_ADD));                      //byte[136]            
        printf("[meta]USED_SECTOR_LEN: %d\n", msb2lsb(&pmeta->USED_SECTOR_LEN));                      //byte[140]        
    }

    if (funcbits & ASPMETA_FUNC_SDRD) {
        printf("[meta]__ASPMETA_FUNC_SDRD__(0x%x & 0x%x = 0x%x)\n", funcbits, ASPMETA_FUNC_SDRD, (funcbits & ASPMETA_FUNC_SDRD));
        printf("[meta]SD_RW_SECTOR_ADD: %d\n", msb2lsb(&pmeta->SD_RW_SECTOR_ADD));                      //byte[144]            
        printf("[meta]SD_RW_SECTOR_LEN: %d\n", msb2lsb(&pmeta->SD_RW_SECTOR_LEN));                      //byte[148]        
    }

    if (funcbits & ASPMETA_FUNC_SDWT) {
        printf("[meta]__ASPMETA_FUNC_SDWT__(0x%x & 0x%x = 0x%x)\n", funcbits, ASPMETA_FUNC_SDWT, (funcbits & ASPMETA_FUNC_SDWT));
        printf("[meta]SD_RW_SECTOR_ADD: %d\n", msb2lsb(&pmeta->SD_RW_SECTOR_ADD));                      //byte[136]            
        printf("[meta]SD_RW_SECTOR_LEN: %d\n", msb2lsb(&pmeta->SD_RW_SECTOR_LEN));                      //byte[140]        
    }

    printf("********************************************\n");
    return 0;
}

#if 0
    ASPMETA_FUNC_NONE = 0,
    ASPMETA_FUNC_CONF = 0x1,       /* 0b00000001 */
    ASPMETA_FUNC_CROP = 0x2,       /* 0b00000010 */
    ASPMETA_FUNC_IMGLEN = 0x4,   /* 0b00000100 */
    ASPMETA_FUNC_SDFREE = 0x8,   /* 0b00001000 */
    ASPMETA_FUNC_SDUSED = 0x16, /* 0b00010000 */
    ASPMETA_FUNC_SDRD = 0x32,     /* 0b00100000 */
    ASPMETA_FUNC_SDWT = 0x64,    /* 0b01000000 */

    ASPOP_FILE_FORMAT,
    ASPOP_COLOR_MODE,
    ASPOP_COMPRES_RATE,
    ASPOP_SCAN_SINGLE,
    ASPOP_SCAN_DOUBLE,
    ASPOP_ACTION,
    ASPOP_RESOLUTION,
    ASPOP_SCAN_GRAVITY,
    ASPOP_MAX_WIDTH,
    ASPOP_WIDTH_ADJ_H,
    ASPOP_WIDTH_ADJ_L,
    ASPOP_SCAN_LENS_H,
    ASPOP_SCAN_LENS_L,
    ASPOP_INTER_IMG,     
    ASPOP_AFEIC_SEL,     
    ASPOP_EXT_PULSE,     /* 15 */

#endif

static int aspMetaClear(struct mainRes_s *mrs, struct procRes_s *rs, int out) 
{
    struct aspMetaData *pmeta;
    
    if ((!mrs) && (!rs)) return -1;
    
    if (mrs) {
        if (out) {
            pmeta = mrs->metaout;
        } else {
            pmeta = mrs->metain;        
        }
    } else {
        if (out) {
            pmeta = rs->pmetaout;
        } else {
            pmeta = rs->pmetain;
        }
    }

    memset(pmeta, 0, sizeof(struct aspMetaData));
    msync(pmeta, sizeof(struct aspMetaData), MS_SYNC);
    
    return 0;
}

static int aspMetaBuild(unsigned int funcbits, struct mainRes_s *mrs, struct procRes_s *rs) 
{
    uint32_t tbits=0;
    int opSt=0, opEd=0;
    int istr=0, iend=0, idx=0;
    struct aspMetaData *pmeta;
    struct aspConfig_s *pct=0, *pdt=0;
    char *pvdst=0, *pvend=0;
    
    if ((!mrs) && (!rs)) return -1;
    
    if (mrs) {
        pmeta = mrs->metaout;
        pct = mrs->configTable;
    } else {
        pmeta = rs->pmetaout;
        pct = rs->pcfgTable;
    }

    msync(pct, ASPOP_CODE_MAX * sizeof(struct aspConfig_s), MS_SYNC);

    if (funcbits == ASPMETA_FUNC_NONE) return -2;

    if (funcbits & ASPMETA_FUNC_CONF) {
        opSt = OP_FFORMAT;
        opEd = OP_SUPBACK;
        
        istr = ASPOP_FILE_FORMAT;
        iend = ASPOP_SUP_SAVE;
        
        pvdst = &pmeta->FILE_FORMAT;
        pvend = &pmeta->SUP_WRITEBK;

        for (idx = istr; idx <= iend; idx++) {
            if ((pct[idx].opStatus & ASPOP_STA_CON) && (pct[idx].opCode == opSt)) {
                *pvdst = pct[idx].opValue & 0xff;
                printf("[meta] 0x%.2x = 0x%.2x (0x%.2x)\n", pct[idx].opCode, pct[idx].opValue, pct[idx].opStatus);

                pvdst++;
                opSt++;
            }

            if (pvend < pvdst) {
                 break;
            }
            
            if (opEd < opSt) {
                 break;
            }
        }

        opSt = OP_FUNCTEST_00;
        opEd = OP_FUNCTEST_15;

        istr = ASPOP_FUNTEST_00;
        iend = ASPOP_FUNTEST_15;
        
        pvdst = &pmeta->OP_FUNC_00;
        pvend = &pmeta->OP_FUNC_15;

        for (idx = istr; idx <= iend; idx++) {
            if ((pct[idx].opStatus & ASPOP_STA_CON) && (pct[idx].opCode == opSt)) {
                *pvdst = pct[idx].opValue & 0xff;
                printf("[meta] 0x%.2x = 0x%.2x (0x%.2x)\n", pct[idx].opCode, pct[idx].opValue, pct[idx].opStatus);

                pvdst++;
                opSt++;
            }

            if (pvend < pvdst) {
                 break;
            }
            if (opEd < opSt) {
                 break;
            }
        }
        
    }
    
    if (funcbits & ASPMETA_FUNC_CROP) {
    
    }

    if (funcbits & ASPMETA_FUNC_IMGLEN) {
    
    }

    if (funcbits & ASPMETA_FUNC_SDFREE) {
    
    }

    if (funcbits & ASPMETA_FUNC_SDUSED) {
    
    }

    if (funcbits & ASPMETA_FUNC_SDRD) {
    
    }

    if (funcbits & ASPMETA_FUNC_SDWT) {
    
    }

    pmeta->ASP_MAGIC[0] = 0x20;
    pmeta->ASP_MAGIC[1] = 0x14;
    
    //pmeta->FUNC_BITS |= funcbits;
    tbits = msb2lsb(&pmeta->FUNC_BITS);
    tbits |= funcbits;
    lsb2Msb(&pmeta->FUNC_BITS, tbits);

    msync(pmeta, sizeof(struct aspMetaData), MS_SYNC);
    
    return 0;
}

static int aspMetaRelease(unsigned int funcbits, struct mainRes_s *mrs, struct procRes_s *rs) 
{
    int i=0, act=0;
    struct intMbs_s *pt=0;
    struct aspMetaData *pmeta;
    struct aspMetaMass *pmass;
    struct aspConfig_s *pct=0, *pdt=0;
    unsigned char linGap, linStart;
    unsigned short linRec;
    unsigned int val;

    if ((!mrs) && (!rs)) return -1;
    
    if (mrs) {
        pmeta = mrs->metain;
        pct = mrs->configTable;
        pmass = &mrs->metaMass;
    } else {
        pmeta = rs->pmetain;
        pct = rs->pcfgTable;
        pmass = rs->pmetaMass;
    }
    
    msync(pmeta, sizeof(struct aspMetaData), MS_SYNC);

    if ((pmeta->ASP_MAGIC[0] != 0x20) || (pmeta->ASP_MAGIC[1] != 0x14)) {
        return -2;
    }
    
    if (funcbits == ASPMETA_FUNC_NONE) return -3;

    if (funcbits & ASPMETA_FUNC_CONF) {
    
    }
    
    if (funcbits & ASPMETA_FUNC_CROP) {
        pt = &(pmeta->CROP_POS_1);

        for (i = ASPOP_CROP_01; i <= ASPOP_CROP_06; i++) {
            pct[i].opValue = msb2lsb(pt);
            pct[i].opStatus = ASPOP_STA_UPD;
            pt++;
        }

        for (i = ASPOP_CROP_07; i <= ASPOP_CROP_18; i++) {
            pct[i].opValue = msb2lsb(pt);
            pct[i].opStatus = ASPOP_STA_UPD;
            pt++;
        }

        linGap = pmeta->YLine_Gap;
        linStart = pmeta->Start_YLine_No;
        
        pt = (struct intMbs_s *)&(pmeta->YLines_Recorded);
        val = msb2lsb(pt);
        linRec = val >> 16;

        if (linRec) {
            pmass->massGap = linGap;
            pmass->massStart = linStart;
            pmass->massRecd = linRec;

            cfgTableSet(pct, ASPOP_XCROP_GAT, linGap);
            cfgTableSet(pct, ASPOP_XCROP_LINSTR, linStart);
            cfgTableSet(pct, ASPOP_XCROP_LINREC, linRec);

            act |= ASPMETA_FUNC_CROP;
            
        } else {
            pmass->massGap = 0;
            pmass->massStart = 0;
            pmass->massRecd = 0;
        }
    }

    if (funcbits & ASPMETA_FUNC_IMGLEN) {
        pt = &(pmeta->SCAN_IMAGE_LEN);    

        pct[ASPOP_IMG_LEN].opValue = msb2lsb(pt);
        pct[ASPOP_IMG_LEN].opStatus = ASPOP_STA_UPD;
    }

    if (funcbits & ASPMETA_FUNC_SDFREE) {
    
    }

    if (funcbits & ASPMETA_FUNC_SDUSED) {
    
    }

    if (funcbits & ASPMETA_FUNC_SDRD) {
    
    }

    if (funcbits & ASPMETA_FUNC_SDWT) {
    
    }
    
    msync(pct, ASPOP_CODE_MAX * sizeof(struct aspConfig_s), MS_SYNC);

    return act;
}

static int doSystemCmd(char *sCommand)
{
#define BIGBUFLEN 2048
#define BUFLEN 128

    int ret=0, wct=0, n=0, totlog=0;
    FILE *fpRead = 0;
    char retBuff[BUFLEN], *pch=0, *p0=0;
    char bigBuff[BIGBUFLEN], *pBig=0;

    memset(bigBuff, 0, BIGBUFLEN);
    memset(retBuff, 0, BUFLEN);

    //printf("doSystemCmd() [%s]\n", sCommand);
    fpRead = popen(sCommand, "r");
    //sleep(1);
    
    if (!fpRead) return -1;

    pBig = bigBuff;
    pch = fgets(retBuff, BUFLEN , fpRead);
    while (pch) {
    
        if (pch) {
            //printf("sCommand: \n[%s] - 1\n\n", retBuff);
            //p0 = strstr(pch, "\n");
            //if (p0) *p0 = '\0';

            n = strlen(pch);
            totlog += n;
            if (totlog > BIGBUFLEN) break;

            strncpy(pBig, pch, n);
            pBig += n;
            
            //printf("sCommand: [%s] \n", pch);
            //shmem_dump(retBuff, BUFLEN);
        } else {
            wct++;
            //printf("sCommand: %d...\n", wct);
            if (wct > 3) {
                //break;
            }
        }
        
        //sleep(1);
        //memset(retBuff, 0, BUFLEN);

        pch = fgets(retBuff, BUFLEN , fpRead);
    }

    //printf("scmd: [%s] \n", bigBuff);
            
    pclose(fpRead);

    return 0;
}

static int findEOF(char *p, int max)
{
    int index[2] = {0, 0};
    char mark[2] = {0xff, 0xd9};
    char target = 0;
    int i, cnt=0;

    if (!p) return -1;
    if (max == 0) return -2;

    target = mark[0];
    for (i = 0; i < max; i++) {
        if (cnt > 0) {
            if (p[i] == target) {
                index[1] = i; 
                break;
            } else {
                cnt = 0;
                target = mark[0];
            }
        } else {
            if (p[i] == target) {
                index[0] = i; 
                cnt = 1;
                target = mark[1];
            }
        }    
    }

    if (i < max) {
        return index[0];
    } 

    return -3;
}

static int tiffHead(char *ptiff, int max)
{
    int ret=0;
    if (!ptiff) return -1;
    if (!max) return -2;
    char sample[8] = {0x49, 0x49, 0x2a, 0x00, 0x08, 0xa0, 0x39, 0x00};

    memcpy(ptiff, sample, 8);
    ret = 8;

    return ret;
}

static int tiffTail(char *ptiff, int max)
{
    char patern[170] = {0x0C, 0x00, 0x00, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0xE0, 0x10, 
         0x00, 0x00, 0x01, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x50, 0x1B, 0x00, 0x00, 0x02, 
         0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x01, 0x03, 0x00, 
         0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 
         0x00, 0x01, 0x00, 0x00, 0x00, 0x11, 0x01, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 
         0x00, 0x00, 0x15, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x16, 
         0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x50, 0x1B, 0x00, 0x00, 0x17, 0x01, 0x04, 0x00, 
         0x01, 0x00, 0x00, 0x00, 0xC0, 0x9C, 0x39, 0x00, 0x1A, 0x01, 0x05, 0x00, 0x01, 0x00, 0x00,
         0x00, 0x62, 0x9D, 0x39, 0x00, 0x1B, 0x01, 0x05, 0x00, 0x01, 0x00, 0x00, 0x00, 0x6A, 0x9D, 
         0x39, 0x00, 0x1C, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 
         0x00, 0x00, 0x00, 0xE0, 0x10, 0x00, 0x00, 0x00, 0x00, 0x80, 0x25, 0x00, 0x00, 0x20, 0x00, 
         0x00, 0x00, 0x80, 0x25, 0x00, 0x00, 0x20, 0x00};
    int ret=0;
    if (!ptiff) return -1;
    if (!max) return -2;
    if (max < 170) return -3;
    
    memcpy(ptiff, patern, 170);
    ret = 170;

    return ret;
}

static int pdfParamCalcu(int hi, int wh, int *mh, int *mw)
{
    double jscale=0.0, tscale=0.0, dscale=0.0;;
    double tw=0.0, th=0.0, sw=0.0, sh=0.0;;
    
    if (!mh) return -1;
    if (!mw) return -2;
    if (!hi) return -3;
    if (!wh) return -4;

    sh = hi;
    sw = wh;

    tscale = MAX_PDF_W / MAX_PDF_H;
    jscale = sw / sh;

    if (jscale > tscale) {
        tw = MAX_PDF_W;
        dscale = tw / sw;
        th = sh * dscale;
    } else {
        th = MAX_PDF_H;
        dscale = th / sh;
        tw = sw * dscale;
    }

    *mh = (int) th;
    *mw = (int) tw;

    return 0;
}

static int pdfAppend(char *d, char *s, int tot, int max)
{
    int idx = 0, slen = 0, end = 0;

    if (d == 0) return -1;
    if (s == 0) return -2;
    if (tot > max) return -3;

    slen = strlen(s);
    idx = tot;
    end = tot + slen;
    if (end > max) return -4;
    
    while (idx < end) {
        d[idx] = *s;
        idx ++;
        s++;
    }
    
    return slen;
}

static int pdfHead(char *ppdf, int max, int hi, int wh, int mh, int mw)
{
    double wscale=0.0, hscale=0.0;
    double d1=0, d2=0;
    char tch[128], *dst = 0;
    int tlen = 0, tot = 0, n=0;

    if (ppdf == 0) return -1;
    if (max == 0) return -2;
    if (hi == 0) return -7;
    if (wh == 0) return -4;
    if (mh == 0) return -5;
    if (mw == 0) return -6;

    dst = ppdf;

    d1 = mh;
    d2 = hi;
    hscale = d1 / d2;

    d1 = mw;
    d2 = wh;
    wscale = d1 / d2;
    
    sprintf(tch, "%PDF-1.4\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;
    
    sprintf(tch, "1 0 obj\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "<< /Type /Catalog\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "/Pages 2 0 R\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, ">>\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "endobj\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "3 0 obj\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "<< /Type /Page\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "/Parent 2 0 R\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "/Resources 4 0 R\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "/MediaBox [0 0 %d %d]\n", mw, mh);
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "/Contents 5 0 R\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, ">>\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "endobj\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "4 0 obj\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "<< /ProcSet [/PDF /ImageB]\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "/XObject << /Im1 6 0 R >>\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, ">>\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "endobj\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "5 0 obj\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "<< /Length 107 >>\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "stream\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "   q\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "      %.6f 0 0 %.6f 0 0 cm\n", wscale, hscale);
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "      1 0 0 1 0 0 cm\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "      %d 0 0 %d 0 0 cm\n", wh, hi);
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "      /Im1 Do\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "   Q\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "\nendstream\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "endobj\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "6 0 obj\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "<< /Type /XObject\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "/Subtype /Image\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "/Width %d\n", wh);
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "/Height %d\n", hi);
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "/ColorSpace /DeviceRGB\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "/BitsPerComponent 8\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "/Filter/DCTDecode /Length 7 0 R>>\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "stream\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;


    return tot;
}

static int pdfTail(char *ppdf, int max, int offset, int imgSize)
{
    char tch[128], *dst = 0;
    char end[6] = {0x25, 0x25, 0x45, 0x4f, 0x46, 0x00};
    int tlen = 0, tot = 0, n=0;
    int offset_2=0, offset_7=0, offset_x=0;

    if (ppdf == 0) return -1;
    if (max == 0) return -2;

    dst = ppdf;

    sprintf(tch, "\nendstream\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "endobj\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    offset_7 = offset + tot;
    sprintf(tch, "7 0 obj\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "%d\n", imgSize);
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "endobj\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    offset_2 = offset + tot;
    
    sprintf(tch, "2 0 obj\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "<< /Type /Pages\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "/Kids [3 0 R ]\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "/Count 1\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, ">>\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "endobj\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    offset_x = offset + tot;
    sprintf(tch, "xref\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "0 8\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "0000000000 65535 f\r\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "0000000009 00000 n\r\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "%.10d 00000 n\r\n", offset_2);
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "0000000058 00000 n\r\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "0000000162 00000 n\r\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "0000000234 00000 n\r\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "0000000392 00000 n\r\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "%.10d 00000 n\r\n", offset_7);
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "trailer\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "<< /Size 8\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "/Root 1 0 R\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, ">>\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "startxref\n");
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    sprintf(tch, "%d\n",offset_x);
    n = pdfAppend(dst, tch, tot, max);
    if (n < 0) return -3;
    tot += n;

    n = pdfAppend(dst, end, tot, max);
    if (n < 0) return -3;
    tot += n;

    return tot;
}

static void aspFree(void *p)
{
    //printf("  free 0x%.8x \n", p);
    //free(p);
}

static void* aspMalloc(int mlen)
{
    int tot=0;
    char *p=0;
    
    tot = *totMalloc;
    tot += mlen;
    printf("!!!!!!!!!!!!!!!!!!!  malloc size: %d / %d\n", mlen, tot);
    *totMalloc = tot;
    
    p = malloc(mlen);
    return p;
}

static void* aspSalloc(int slen)
{
    int tot=0;
    char *p=0;
    
    tot = *totSalloc;
    tot += slen;
    printf("*******************  salloc size: %d / %d\n", slen, tot);
    *totSalloc = tot;
    
    p = mmap(NULL, slen, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    return p;
}

static void debugPrintBootSec(struct sdbootsec_s *psec)
{
            /* 0  Jump command */
    printf("[0x%x]: /* 0  Jump command */ \n", psec->secJpcmd);
            /* 3  system id */
    printf("[%s]: /* 3  system id */\n", psec->secSysid);
            /* 11 sector size */ 
    printf("[%d]: /* 11 sector size */ \n", psec->secSize);
            /* 13 sector per cluster */
    printf("[%d]: /* 13 sector per cluster */\n", psec->secPrClst);
            /* 14 reserved sector count*/
    printf("[%d]: /* 14 reserved sector count*/\n", psec->secResv);            
            /* 16 number of FATs */
    printf("[%d]: /* 16 number of FATs */\n", psec->secNfat);
            /* 17 skip, number of root dir entries */
            /* 19 skip, total sectors */
            /* 21 medium id */
    printf("[0x%x]: /* 21 medium id */\n", psec->secIDm);
            /* 22 skip, sector per FAT */
            /* 24 sector per track */
    printf("[%d]: /* 24 sector per track */\n", psec->secPrtrk);
            /* 26 number of sides */
    printf("[%d]: /* 26 number of sides */\n", psec->secNsid);
            /* 28 number of hidded sectors */
    printf("[%d]: /* 28 number of hidded sectors */\n", psec->secNhid);
            /* 32 total sectors */
    printf("[%d]: /* 32 total sectors */\n", psec->secTotal);
            /* 36 sectors per FAT */
    printf("[%d]: /* 36 sectors per FAT */\n", psec->secPrfat);
            /* 40 extension flag */
    printf("[0x%x]: /* 40 extension flag */\n", psec->secExtf);
            /* 42 FS version */
    printf("[0x%x]: /* 42 FS version */\n", psec->secVers); 
            /* 44 root cluster */
    printf("[%d]: /* 44 root cluster */\n", psec->secRtclst); 
            /* 48 FS info */
    printf("[0x%x]: /* 48 FS info */\n", psec->secFSif); 
            /* 50 backup boot sector */
    printf("[%d]: /* 50 backup boot sector */\n", psec->secBkbt); 
            /* 64 physical disk number */
    printf("[%d]: /* 64 physical disk number */\n", psec->secPhdk);
            /* 66 extended boot record signature */
    printf("[0x%x]: /* 66 extended boot record signature */\n", psec->secExtbt);
            /* 67 volume ID number */
    printf("[0x%x]: /* 67 volume ID number */\n", psec->secVoid); 
            /* 71 to 81 volume label */
    printf("[%s]: /* 71 to 81 volume label */\n", psec->secVola);
            /* 82 to 89 file system type */
    printf("[%s]: /* 82 to 89 file system type */\n", psec->secFtyp);
            /* 510 signature word */
    printf("[0x%x]: /* 510 signature word */\n", psec->secSign);
            /* set the boot sector status to 1 */
    printf("[0x%x]: /* boot sector status */\n", psec->secSt);
            /* the start sector of fat table */
    printf("[%d]: /* the start sector of fat table */\n", psec->secWhfat);
            /* the start sector of root dir */
    printf("[%d]: /* the start sector of root dir */\n", psec->secWhroot);

}

static void debugPrintDir(struct directnFile_s *pf)
{
#if 0
struct directnFile_s{
    uint32_t   dftype;
    uint32_t   dfstats;
    char        dfLFN[256];
    char        dfSFN[12];
    int           dflen;
    uint32_t   dfattrib;
    uint32_t   dfcretime;
    uint32_t   dfcredate;
    uint32_t   dflstacdate;
    uint32_t   dfrecotime;
    uint32_t   dfrecodate;
    uint32_t   dfclstnum;
    uint32_t   dflength;
    struct directnFile_s *pa;
    struct directnFile_s *br;
    struct directnFile_s *ch;   
};
#endif
    printf("==========================================\n");
    printf("  [%x] type \n", pf->dftype);
    printf("  [%x] status \n", pf->dfstats);
    printf("  [%s] long file name, len:%d\n", pf->dfLFN, pf->dflen);
    printf("  [%s] short file name \n", pf->dfSFN);
    printf("  [%x] attribute \n", pf->dfattrib);
    printf("  [%.2d:%.2d:%.2d] H:M:S created time \n", (pf->dfcretime >> 16) & 0xff, (pf->dfcretime >> 8) & 0xff, (pf->dfcretime >> 0) & 0xff);
    printf("  [%.2d:%.2d:%.2d] Y:M:D created date \n", ((pf->dfcredate >> 16) & 0xff) + 1980, (pf->dfcredate >> 8) & 0xff, (pf->dfcredate >> 0) & 0xff);
    printf("  [%.2d:%.2d:%.2d] Y:M:D access date \n", ((pf->dflstacdate >> 16) & 0xff) + 1980, (pf->dflstacdate >> 8) & 0xff, (pf->dflstacdate >> 0) & 0xff);
    printf("  [%.2d:%.2d:%.2d] H:M:S recorded time \n", (pf->dfrecotime >> 16) & 0xff, (pf->dfrecotime >> 8) & 0xff, (pf->dfrecotime >> 0) & 0xff);
    printf("  [%.2d:%.2d:%.2d] Y:M:D recorded date \n", ((pf->dfrecodate >> 16) & 0xff) + 1980, (pf->dfrecodate >> 8) & 0xff, (pf->dfrecodate >> 0) & 0xff);
    printf("  [%d] cluster number \n", pf->dfclstnum);
    printf("  [%d] file length \n", pf->dflength);
    printf("==========================================\n");
}

inline int aspCalcSupLen(struct supdataBack_s *sup)
{
    int len=0, cnt=0;
    struct supdataBack_s *scr;
    if (!sup) return -1;
    
    scr = sup;
    while(scr) {
        len += scr->supdataTot - scr->supdataUse;

        //printf("%d. calcu sup, tot / used / len = %d / %d / %d\n", cnt, scr->supdataTot, scr->supdataUse, len);
        scr = scr->n;

        cnt++;
    }
    return len;
}

static int aspPopSupOut(char *dst, struct supdataBack_s *str, int size, struct supdataBack_s **nxt)
{
    int ret = 0, acusz = 0;
    char *src1 = 0, *src2 = 0;
    int tot=0, usd=0, rst=0;
    struct supdataBack_s *scr=0, *snx=0;
    
    if (!str) return -1;
    if (!nxt) return -2;
    if (!dst) return -3;

    scr = str;
    snx = str->n;

    src1 = scr->supdataBuff;
    tot = scr->supdataTot;
    usd = scr->supdataUse;
    rst = tot - usd;

    if (rst < 0) return -4;
    if (rst == 0) return 0;

    if (size == rst) {
        memcpy(dst, src1+usd, size);
        acusz += size;
        *nxt = snx;
        scr->supdataUse += size;
    } else if (size > rst) {
        memcpy(dst, src1+usd, rst);
        acusz += rst;
        ret = aspPopSupOut(dst+rst, snx, size - rst, nxt);
        if (ret > 0) {
            acusz += ret;
        } else {
            *nxt = snx;
        }
        scr->supdataUse += rst;
    } else {
        memcpy(dst, src1+usd, size);    
        acusz += size;
        *nxt = scr;
        scr->supdataUse += size;
    }

    return acusz;
}

static uint32_t aspRawCompose(char * raw, int size)
{
    int sh[4] = {0, 8, 16, 24};
    int i = 0;
    uint32_t val = 0, tmp = 0;

    while(i < size) {
        tmp = raw[i];
        val |= tmp << sh[i];
        i++;
    }
    return val;
}


static uint32_t cfgValueOffset(uint32_t val, int offset)
{
    return (val >> offset) & 0xff;
}

static int cfgTableSet(struct aspConfig_s *table, int idx, uint32_t val)
{
    struct aspConfig_s *p=0;
    int ret=0;

    if (!table) return -1;
    if (idx >= ASPOP_CODE_MAX) return -1;

    p = &table[idx];
    if (!p) return -2;

    ret = cmdfunc_opchk_single(val, p->opMask, p->opBitlen, p->opType);
    if (ret < 0) {
        ret = (ret * 10) -3;
        return ret;
    } else {            
        p->opValue = val;
        p->opStatus = ASPOP_STA_WR;
    }

    return 0;
}

static int cfgTableGet(struct aspConfig_s *table, int idx, uint32_t *rval)
{
    struct aspConfig_s *p=0;
    int ret=0;

    if (!rval) return -1;
    if (!table) return -1;
    if (idx >= ASPOP_CODE_MAX) return -1;

    p = &table[idx];
    if (!p) return -2;

    if (p->opStatus == ASPOP_STA_NONE) return -3;

    *rval = p->opValue;

    return 0;
}

static int cfgTableGetChk(struct aspConfig_s *table, int idx, uint32_t *rval, uint32_t stat)
{
    struct aspConfig_s *p=0;
    int ret=0;

    if (!rval) return -1;
    if (!table) return -1;
    if (idx >= ASPOP_CODE_MAX) return -1;

    p = &table[idx];
    if (!p) return -2;

    if (p->opStatus != stat) return -3;

    *rval = p->opValue;

    return 0;
}

static int asp_idxofch(char *str, char ch, int start, int max) 
{
    int i=0, ret=-1;
    char *p=0;

    if (start >= max) return -2;

    p = str + start;
    
    while (start < max) {

        if (*p == ch) {
            //printf("%c ", *p);
            return start;
        } else {
            //printf("x%c", *p);
        }

        p++;
        start++;
    }
    
    return ret;
}

static int asp_strsplit(struct aspInfoSplit_s **info, char *str, int max)
{
    char log[256];
    int len, cnt;
    int cur, nex, ret;
    struct aspInfoSplit_s *h=0, *p=0, *c=0;

    cur = 0;
    cnt = 0;
    
    while (cur < max) {
        ret = asp_idxofch(str, ',', cur, max);
        if (ret >= 0) {
            nex = ret;
            p = aspMalloc(sizeof(struct aspInfoSplit_s));
            memset(p, 0, sizeof(struct aspInfoSplit_s));            
            cnt++;
        } else {
            p = aspMalloc(sizeof(struct aspInfoSplit_s));
            memset(p, 0, sizeof(struct aspInfoSplit_s));            
            cnt++;
            nex = max;
        }
    
        len = nex - cur;

        if (len == 0) {
            cur = nex+1;            
            aspFree(p);
            continue;
        }
        
        p->infoStr = aspMalloc(len+1);
        p->infoLen = len;
        memcpy(p->infoStr, str+cur,  len);

        p->infoStr[len] = '\0';

        //sprintf(log, "%d, %s\n", p->infoLen, p->infoStr);
        //print_f(mlogPool, "SPLT", log);

        cur = nex+1;

        if (c) {
            c->n = p;
        }
        
        c = p;
        
        if (!h) {
            h = c;
        }

    }

    *info = h;
    return cnt;
}

static struct aspInfoSplit_s *asp_getInfo(struct aspInfoSplit_s *info, int idx) 
{
    int i=0;
    
    while (i < idx) {
        i++;
        if (!info) break;
        info = info->n;
    }

    return info;
}
static struct aspInfoSplit_s *asp_freeInfo(struct aspInfoSplit_s *info) 
{
    struct aspInfoSplit_s *nex=0;
    if (!info) return 0;
    nex = info->n;

    //printf("free[%s]\n", info->infoStr);

    aspFree(info->infoStr);
    aspFree(info);
    return nex;
}

static int aspNameCpyfromName(char *name, char *dst, int offset, int len, int jump)
{
    char ch=0;
    int i=0, cnt=0, idx=0;

    if (!len) return 0;

    cnt = 0;
    for (i = 0; i < len; i++) {
        idx = offset + i*jump;
        
        if (jump == 2) { /* Long file name */
            ch = aspLnameFilter(name[i]);
        } else { /* short file name */
            ch = aspSnameFilterIn(name[i]);
        }

        if (ch == 0xff) return cnt;
        dst[idx] = ch;
        cnt++;
        if (ch == 0) return cnt;
    }

    //printf("cpy cnt:%d \n", cnt);
    return cnt;
}

static int aspCompirseSFN(uint8_t *pc, struct directnFile_s *pf, uint8_t *sfn)
{
    uint32_t tmp32=0;
    struct directnFile_s *fs=0;
    uint8_t *raw=0;

    if (!pc) return -1;
    if (!pf) return -2;

    fs = pf;
    raw = pc;

    memset(raw, 0, 32);

    printf("  Compirse SFN: ");
    printf("  [%x] type \n", pf->dftype);
    printf("  [%x] status \n", pf->dfstats);
    printf("  [%s] long file name, len:%d\n", pf->dfLFN, pf->dflen);
    printf("  [%s] short file name \n", pf->dfSFN);
    printf("  [%s] short file name - 2\n", sfn);
    printf("  [%x] attribute \n", pf->dfattrib);
    printf("  [%.2d:%.2d:%.2d] H:M:S created time \n", (pf->dfcretime >> 16) & 0xff, (pf->dfcretime >> 8) & 0xff, (pf->dfcretime >> 0) & 0xff);
    printf("  [%.2d:%.2d:%.2d] Y:M:D created date \n", ((pf->dfcredate >> 16) & 0xff) + 1980, (pf->dfcredate >> 8) & 0xff, (pf->dfcredate >> 0) & 0xff);
    printf("  [%.2d:%.2d:%.2d] Y:M:D access date \n", ((pf->dflstacdate >> 16) & 0xff) + 1980, (pf->dflstacdate >> 8) & 0xff, (pf->dflstacdate >> 0) & 0xff);
    printf("  [%.2d:%.2d:%.2d] H:M:S recorded time \n", (pf->dfrecotime >> 16) & 0xff, (pf->dfrecotime >> 8) & 0xff, (pf->dfrecotime >> 0) & 0xff);
    printf("  [%.2d:%.2d:%.2d] Y:M:D recorded date \n", ((pf->dfrecodate >> 16) & 0xff) + 1980, (pf->dfrecodate >> 8) & 0xff, (pf->dfrecodate >> 0) & 0xff);
    printf("  [%d] cluster number \n", pf->dfclstnum);
    printf("  [%d] file length \n", pf->dflength);

    if (fs->dfattrib & ASPFS_ATTR_DIRECTORY) {
        raw[11] = ASPFS_ATTR_DIRECTORY;
    } else {
        raw[11] = ASPFS_ATTR_ARCHIVE;
    }

    tmp32 = aspFStimeCps(fs->dfcretime);
    raw[15] = (tmp32 >> 8) & 0xff;
    raw[14] = tmp32 & 0xff;

    tmp32 = aspFSdateCps(fs->dfcredate);
    raw[17] = (tmp32 >> 8) & 0xff;
    raw[16] = tmp32 & 0xff;

    tmp32 = aspFSdateCps(fs->dflstacdate);
    raw[19] = (tmp32 >> 8) & 0xff;
    raw[18] = tmp32 & 0xff;

    tmp32 = aspFStimeCps(fs->dfrecotime);
    raw[23] = (tmp32 >> 8) & 0xff;
    raw[22] = tmp32 & 0xff;

    tmp32 = aspFSdateCps(fs->dfrecodate);
    raw[25] = (tmp32 >> 8) & 0xff;
    raw[24] = tmp32 & 0xff;

    tmp32 = fs->dfclstnum;
    raw[26] = tmp32 & 0xff;
    raw[27] = (tmp32 >> 8) & 0xff;
    raw[20] = (tmp32 >> 16) & 0xff;
    raw[21] = (tmp32 >> 24) & 0xff;

    tmp32 = fs->dflength;
    raw[28] = tmp32 & 0xff;
    raw[29] = (tmp32 >> 8) & 0xff;
    raw[30] = (tmp32 >> 16) & 0xff;
    raw[31] = (tmp32 >> 24) & 0xff;

    /* debug time and date */
    //shmem_dump(raw, 32);
    //printf("  dump end \n ");
    
    aspNameCpyfromName(sfn, raw, 0, 11, 1);
    
    return 32;
}

static int aspCompirseLFN(uint8_t *pc, char *name, uint8_t chksum, int size)
{
    char *c=0;
    uint8_t *p=0;
    int num=0, rst=0, i = 0, cnt=0;
    if (!pc) return -1;
    if (!name) return -2;

    if (size > 255) size = 255;
    
    rst = size % 13;
    num = (rst == 0) ? (size/13) : (size/13+1);

    printf("  LFN[%s] num: %d, rst: %d\n", name, num, rst);

    p = pc;
    c = name;
    for (i = num; i > 0; i--) {
        memset(p, 0, 32);
        p[0] = i & 0x0f;
        p[11] = 0x0f;
        p[12] = 0x00;
        p[13] = chksum;
        p[26] = 0x00;
        p[27] = 0x00;

        c = name + ((i - 1) * 13);
        if (i == num) {
            p[0] |= 0x40;
            if (rst) {
                if (rst > 5) {
                    aspNameCpyfromName(c, p, 1, 5, 2);
                    rst -= 5;
                } else {
                    aspNameCpyfromName(c, p, 1, rst, 2);
                    memset(p+1+rst*2, 0xff, (5-rst)*2);
                    rst = 0;
                }
                c += 5;
                
                if (rst > 6) {
                    aspNameCpyfromName(c, p, 14, 6, 2);
                    rst -= 6;
                } else {
                    aspNameCpyfromName(c, p, 14, rst, 2);
                    memset(p+14+rst*2, 0xff, (6-rst)*2);
                    rst = 0;
                }
                c += 6;
                
                if (rst > 2) {
                    printf("ERROR!! rst should not (> 2) rst: %d \n", rst);
                    aspNameCpyfromName(c, p, 28, 2, 2);
                    rst = 0;
                } else {
                    aspNameCpyfromName(c, p, 28, rst, 2);
                    memset(p+28+rst*2, 0xff, (2-rst)*2);
                    rst = 0;
                }                
            } else {
                aspNameCpyfromName(c, p, 1, 5, 2);
                c += 5;
                aspNameCpyfromName(c, p, 14, 6, 2);
                c += 6;
                aspNameCpyfromName(c, p, 28, 2, 2);
            }
        } else {
            aspNameCpyfromName(c, p, 1, 5, 2);
            c += 5;
            aspNameCpyfromName(c, p, 14, 6, 2);
            c += 6;
            aspNameCpyfromName(c, p, 28, 2, 2);
        }
        p += 32;
    }

    return (p - pc);
}

static int aspFindDot(char *name, int size)
{
    int i=0;
    char *p=0;

    if (!name) return -1;

    p = name;

    i = 0;
    while (i < size) {
        if (*p == '.') {
            return i;
        }
        i++;
        p++;
    }

    return -2;
}

static int aspCompirseDEF(uint8_t *pc, struct directnFile_s *fs)
{
    uint8_t chksum = 0;
    uint8_t *p=0, tmSFN[16];
    int ret=0, len=0;
    if (!pc) return -1;
    if (!fs) return -2;

    memset(tmSFN, 0x20, 16);
    ret = aspFindDot(fs->dfSFN, strlen(fs->dfSFN));
    if (ret < 0) {
        printf("  SFN do not have dot, ret: %d\n", ret);
        aspNameCpyfromName(fs->dfSFN, tmSFN, 0, strlen(fs->dfSFN), 1);
    } else {
        printf("  SFN have dot, at [%d]\n", ret);
        aspNameCpyfromName(fs->dfSFN, tmSFN, 0, ret, 1);
        aspNameCpyfromName(fs->dfSFN+ret+1, tmSFN, 8, 3, 1);
    }

    chksum = aspFSchecksum(tmSFN);
    printf("  tmSFN: [%s], chksum: 0x%.2x\n", tmSFN, chksum);
    
    p = pc;
    if (fs->dflen) {
        len = aspCompirseLFN(p, fs->dfLFN, chksum, fs->dflen+1);
        if (len > 0) {
            printf("  LFN get, len: %d\n", len);
            shmem_dump(p, len);
            p = p + len;
        } else {
            printf("  ERROR!!! LFN get failed, len: %d\n", len);        
        }
    }

    len = aspCompirseSFN(p, fs, tmSFN);
    if (len > 0) {
        printf("  SFN get, len: %d\n", len);
        shmem_dump(p, len);
        //*(p+12) = 0x18; // for windows conpatiable
        p = p + len;
    } else {
        printf("  ERROR!!! SFN get failed, len: %d\n", len);        
    }
    
    return (p - pc);
}

static int aspFindFreeDEF(uint8_t **ppc, uint8_t *pc, int max, int itvl)
{
    int i=0, j=0;
    uint8_t *p=0;

    p = pc;
    while (i < max) {
        if (*p == 0) {
            j = 0;
            while (j < itvl) { 
                if (p[j] != 0) break;
                j++;
            }

            if (j == itvl) {
                *ppc = p;
                return (max - i);
            }
        }
        p += itvl;
        i += itvl;
    }

    *ppc = pc + max;
    
    return -1;
}

static uint8_t aspFSchecksum(uint8_t *pch)
{
    int i=0;
    uint8_t sum=0, ch=0;

    sum = 0;
    for (i=0; i < 11; i++) {
        sum = ((sum & 0x01) ? 0x80:0) | (sum >> 1);
        ch = *pch;
        sum += ch;
        pch++;
    }

    return sum;
}

static char aspLnameFilter(char ch)
{
    char def = '_', *p=0;
    char notAllow[10] = {0x22, 0x2a, 0x2f, 0x3a, 0x3c, 
                                     0x3e, 0x3f, 0x5c, 0x7c, 0x7f};

    if (ch == 0x0)  return ch;
    if (ch < 0x20)  return def;
    if (ch > 0x7f)  return def;

    p = notAllow + 9;
    while (p >= notAllow) {
        if (*p == ch) return def;
        p --;
    }

    return ch;
}

static char aspSnameFilterOut(char ch)
{
    char def = '_', *p=0;
    char notAllow[16] = {0x22, 0x2a, 0x2b, 0x2c, 0x2f, 0x3a, 0x3b, 0x3c, 
                                     0x3d, 0x3e, 0x3f, 0x5b, 0x5c, 0x5d, 0x7c, 0x7f};

    if (ch == 0x0)  return ch;
    if (ch < 0x20)  return def;
    if (ch > 0x7f)  return def;
    if ((ch > 0x40) && ch < (0x5b)) {
        ch += 0x20;
    }

    p = notAllow + 15;
    while (p >= notAllow) {
        if (*p == ch) return def;
        p --;
    }

    return ch;
}

static char aspSnameFilterIn(char ch)
{
    char def = '_', *p=0;
    char notAllow[16] = {0x22, 0x2a, 0x2b, 0x2c, 0x2f, 0x3a, 0x3b, 0x3c, 
                                     0x3d, 0x3e, 0x3f, 0x5b, 0x5c, 0x5d, 0x7c, 0x7f};

    if (ch == 0x0)  return 0x20;
    if (ch < 0x20)  return def;
    if (ch > 0x7f)  return def;
    if ((ch > 0x60) && ch < (0x7b)) {
        ch -= 0x20;
    }

    p = notAllow + 15;
    while (p >= notAllow) {
        if (*p == ch) return def;
        p --;
    }

    return ch;
}

static int aspNameCpyfromRaw(char *raw, char *dst, int offset, int len, int jump)
{
    char ch=0;
    int i=0, cnt=0, idx=0;

    cnt = 0;
    for (i = 0; i < len; i++) {
        idx = offset+i*jump;
        if (idx > 32) return (-1);
        
        if (jump == 2) { /* Long file name */
            ch = aspLnameFilter(raw[idx]);
        } else { /* short file name */
            ch = aspSnameFilterOut(raw[idx]);
        }

        if (ch == 0xff) return cnt;
        *dst = ch;
        dst ++;
        cnt++;
        if (ch == 0) return cnt;
    }

    //printf("cpy cnt:%d \n", cnt);
    return cnt;
}

static int aspFSrmspace(char *str, int len)
{
    int space=0;
    int sc=0;
    char *end=0;
    if (!str) return;
    if (!len) return;
    end = str + len - 1;
    while (end >= str) {
        if (*end == 0x20) {
            space++;
            //*end = 0;
        } else {
            break;
        }
        //if (*end != 0) break;
        end --;
    }
    return space;
}

static void aspFSadddot(char *str, int len)
{
    int sc=0;
    char *end=0;
    if (!str) return;
    if (!len) return;
    end = str + len;
    while (end > str) {
        if (*end == 0x20) {
            *end = '.';
            break;
        }
        end --;
    }
}

static uint32_t aspFSdateAsb(uint32_t fst)
{
    uint32_t val=0, y=0, m=0, d=0;
    d = fst & 0x1f; // 0 -4, 5bits
    m = (fst >> 5) & 0xf; // 5 - 8, 4bits
    y = (fst >> 9) & 0x7f; // 9 - 15, 7bits
    val |= (y << 16) | (m << 8) | d;
    return val;
}

static uint32_t aspFSdateCps(uint32_t val)
{
    uint32_t fst=0, y=0, m=0, d=0;

    d = val & 0x1f; // 0 -4, 5bits
    m = (val >> 8) & 0xf; // 5 - 8, 4bits
    y = (val >> 16) & 0x7f; // 9 - 15, 7bits

    fst = (y << 9) | (m << 5) | d;
    
    return fst;
}

static uint32_t aspFStimeAsb(uint32_t fst)
{
    uint32_t val=0, s=0, m=0, h=0;
    s = (fst & 0x1f) << 1; // 0 -4, 5bits
    m = (fst >> 5) & 0x3f; // 5 - 10, 6bits
    h = (fst >> 11) & 0x1f; // 11 - 15, 5bits
    val |= (h << 16) | (m << 8) | s;
    return val;
}

static uint32_t aspFStimeCps(uint32_t val)
{
    uint32_t fst=0, s=0, m=0, h=0;
    s = (val >> 1) & 0x1f; // 0 -4, 5bits
    m = (val >> 8) & 0x3f; // 5 - 10, 6bits
    h = (val >> 16) & 0x1f; // 11 - 15, 5bits

    fst = (h << 11) | (m << 5) | s;
    
    return fst;
}

static int aspLnameAbs(char *raw, char *dst) 
{
    int cnt=0, ret=0;
    char ch=0;
    if (!raw) return (-1);
    if (!dst) return (-2);

    ret = aspNameCpyfromRaw(raw, dst, 1, 5, 2);
    cnt += ret;
    if (ret != 5) return cnt;

    dst += ret;
    ret = aspNameCpyfromRaw(raw, dst, 14, 6, 2);
    cnt += ret;
    if (ret != 6) return cnt;
    
    dst += ret;
    ret = aspNameCpyfromRaw(raw, dst, 28, 2, 2);
    cnt += ret;

    //printf("name abs cnt:%d\n", cnt);
    return cnt;
}

static int aspRawParseDir(char *raw, struct directnFile_s *fs, int last)
{
    //printf("[%.2x][%d] - [%.6x] \n", *raw, last, fs->dfstats);
    uint32_t tmp32=0;
    uint8_t sum=0;
    int leN=0, cnt=0, idx=0, ret = 0, n = 0;
    char *plnN=0, *pstN=0, *nxraw=0;
    char ld=0, nd=0;
    if (!raw) return (-1);
    if (!fs) return (-2);
    if (last < 32) return (-3); 

    if (fs->dfstats == ASPFS_STATUS_EN) return 0;

    ld = *raw;



    if ((ld == 0xe5) || (ld == 0x05)) {
        ret = -4;
        //memset(fs, 0x00, sizeof(struct directnFile_s));
        goto fsparseEnd;
    } else if (ld == 0x00) {
        //memset(fs, 0x00, sizeof(struct directnFile_s));
        //return 0;
        goto fsparseEnd;
    } else if ((fs->dfstats & 0xff) == ASPFS_STATUS_DIS) {
        pstN = fs->dfSFN;
        ret = aspNameCpyfromRaw(raw, pstN, 0, 11, 1);
        if (ret != 11) {
            memset(fs, 0x00, sizeof(struct directnFile_s));
            printf("\nERROR!!short name [%s] copy error ret:%d \n", pstN, ret);
            goto fsparseEnd;
        }

        idx = (fs->dfstats >> 8) & 0xf;
        if (idx) {
            sum = aspFSchecksum((uint8_t*)raw);
            //printf("LONG file name parsing... last parsing [len:%d]\n", fs->dflen);
            if (sum != (fs->dfstats >> 16) & 0xff) {
                ret = -11;
                memset(fs, 0x00, sizeof(struct directnFile_s));
                printf("WARNING!!! checksum error: 0x%x / 0x%x [%s]\n", sum, (fs->dfstats >> 16) & 0xff, pstN);
                goto fsparseEnd;
            } else {
                printf("CONGING!!! checksum match: 0x%x / 0x%x [%s]\n\n", sum, (fs->dfstats >> 16) & 0xff, pstN);
            }
        } else {
            //printf("\nSHORT file name parsing... [len:%d][%s]\n", fs->dflen, pstN);
        }

        cnt = aspFSrmspace(pstN+8, 3);
        if (cnt < 3) {
            memset(pstN, 0, 16);
            ret = aspNameCpyfromRaw(raw, pstN, 0, 8, 1);
            if (ret != 8) {
                memset(fs, 0x00, sizeof(struct directnFile_s));
                //printf("short name copy error ret:%d \n", ret);
                goto fsparseEnd;
            }
            ret = aspFSrmspace(pstN, 8);
            //printf("[%s]short name space count:%d \n", pstN, ret);
            n = strlen(pstN) - ret;
            pstN += n;
            *pstN = '.';
            pstN += 1;
            //printf("[%s]short name space n:%d \n", pstN, n);
            ret = aspNameCpyfromRaw(raw, pstN, 8, 3, 1);
            if (ret != 3) {
                memset(fs, 0x00, sizeof(struct directnFile_s));
                //printf("short name copy error ret:%d \n", ret);
                goto fsparseEnd;
            }
            //printf("[%s]short name space \n", pstN);
            //aspFSrmspace(pstN, 3);
        }
        
        fs->dfattrib = raw[11];
        tmp32 = raw[14] | (raw[15] << 8);
        fs->dfcretime = aspFStimeAsb(tmp32);
        tmp32 = raw[16] | (raw[17] << 8);
        fs->dfcredate = aspFSdateAsb(tmp32);
        tmp32 = raw[18] | (raw[19] << 8);
        fs->dflstacdate =aspFSdateAsb(tmp32);
        tmp32 = raw[22] | (raw[23] << 8);
        fs->dfrecotime= aspFStimeAsb(tmp32);
        tmp32 = raw[24] | (raw[25] << 8);
        fs->dfrecodate = aspFSdateAsb(tmp32);
        tmp32 = raw[26] | (raw[27] << 8) | (raw[20] << 16) | (raw[21] << 24);
        fs->dfclstnum = tmp32;
        tmp32 = raw[28] | (raw[29] << 8) | (raw[30] << 16) | (raw[31] << 24);
        fs->dflength = tmp32;

        if (fs->dfattrib & ASPFS_ATTR_DIRECTORY) {
            fs->dftype = ASPFS_TYPE_DIR;
        } else {
            fs->dftype = ASPFS_TYPE_FILE;
        }

        fs->dfstats = ASPFS_STATUS_EN;
        ret = 0;

        if (fs->dfattrib > ASPFS_ATTR_ARCHIVE) {
            ret = -12;
        }
        if (fs->dfattrib & ASPFS_ATTR_DIRECTORY) {
            if (fs->dflength > 0) {
                ret = -13;
            }
        } 
        if (!fs->dfclstnum) {
            ret = -14;
        }

        if (ret) {
            memset(fs, 0x00, sizeof(struct directnFile_s));
        }
        
        goto fsparseEnd;
    } else if ((ld & 0xf0) == 0x40) {
        nd = raw[32];
        if (nd != ((ld & 0xf) - 1) || (nd == 0)) {
            //memset(fs, 0x00, sizeof(struct directnFile_s));
            fs->dfstats = ASPFS_STATUS_DIS;
            return aspRawParseDir(raw, fs, last);
        }
        //printf("\n LONG file name parsing...[0x%.2x] \n", ld);
    
        ret = 0;
        if (raw[11] != 0x0f) {
            ret = -5;
        }
        if (raw[12] != 0x00) {
            ret = -6;
        }
        if (ret) {
            //memset(fs, 0x00, sizeof(struct directnFile_s));
            goto fsparseEnd;
        }

        //memset(fs, 0x00, sizeof(struct directnFile_s));

        idx = ld & 0xf;

        if (idx == 0x01) {
            fs->dfstats = ASPFS_STATUS_DIS;
            fs->dfstats |= (ld & 0xf) << 8;
            fs->dfstats |= (raw[13] & 0xff) << 16;
            //printf("WARNING!!! get checksum: 0x%x - 1.0\n", (fs->dfstats >> 16) & 0xff);
        } else {
            fs->dfstats = ASPFS_STATUS_ING;
            fs->dfstats |= (ld & 0xf) << 8;
            fs->dfstats |= (raw[13] & 0xff) << 16;
            //printf("WARNING!!! get checksum: 0x%x - 1.1\n", (fs->dfstats >> 16) & 0xff);
        }

        nxraw = raw+32;
        ret = 32 + aspRawParseDir(nxraw, fs, last-32);

        plnN = fs->dfLFN;
        plnN += fs->dflen;
        cnt = aspLnameAbs(raw, plnN);
        fs->dflen += cnt;
        //printf("LONG file name parsing... go to next ret:%d len:%d cnt:%d\n", ret, fs->dflen, cnt);
        return ret;
    }
    else if ((fs->dfstats & 0xff) == ASPFS_STATUS_ING) {
        //printf("LONG file name parsing... the next \n");
        ret = 0;
        idx = (fs->dfstats >> 8) & 0xf;
        if (ld != (idx - 1)) {
            ret = -7;
        }
        if (raw[11] != 0x0f) {
            ret = -8;
        }
        if (raw[12] != 0x00) {
            ret = -9;
        }
        if (raw[13] != ((fs->dfstats >> 16) & 0xff)) {
            ret = -10;
        }       
        if (ret) {
            printf("\nERROR!!LONG file name parsing... broken here ret:%d\n", ret);
            //memset(fs, 0x00, sizeof(struct directnFile_s));
            goto fsparseEnd;
        }
        
        if ((ld & 0xf) == 0x01) {
            fs->dfstats = ASPFS_STATUS_DIS;
            fs->dfstats |= (ld & 0xf) << 8;
            fs->dfstats |= (raw[13] & 0xff) << 16;
            //printf("WARNING!!! get checksum: 0x%x - 2.0\n", (fs->dfstats >> 16) & 0xff);
        } else {
            fs->dfstats = ASPFS_STATUS_ING;
            fs->dfstats |= (ld & 0xf) << 8;
            fs->dfstats |= (raw[13] & 0xff) << 16;
            //printf("WARNING!!! get checksum: 0x%x - 2.1\n", (fs->dfstats >> 16) & 0xff);
        }

        nxraw = raw+32;
        ret = 32 + aspRawParseDir(nxraw, fs, last-32);

        plnN = fs->dfLFN;
        plnN += fs->dflen;
        cnt = aspLnameAbs(raw, plnN);
        fs->dflen += cnt;
        //printf("LONG file name parsing... go to the next's next ret:%d len:%d cnt:%d\n", ret, fs->dflen, cnt);

        return ret;
    }else {
            //memset(fs, 0x00, sizeof(struct directnFile_s));
            fs->dfstats = ASPFS_STATUS_DIS;
            return aspRawParseDir(raw, fs, last);
    }

fsparseEnd:

    if (last == 32) {
        return 32;
    } else if (last > 32) {
        raw += 32;
        last = last - 32;
        return 32 + aspRawParseDir(raw, fs, last);
    } else {
        return ret;
    }

}

static int aspFS_createFATRoot(struct sdFAT_s *pfat)
{
    char dir[32] = "ROOT";
    struct directnFile_s *r = 0, *c = 0;

    mspFS_allocDir(pfat, &r);
    if (!r) {
        return (-1);
    }
    memset(r, 0, sizeof(struct directnFile_s));

    r->pa = 0;
    r->br = 0;
    r->ch = 0;
    r->dftype = ASPFS_TYPE_ROOT;
    r->dfattrib = 0;
    r->dfstats = ASPFS_STATUS_EN;
    r->dfclstnum = 2;

    strcpy(r->dfSFN, dir);

    /*
    r->dflen = strlen(dir);
    if (r->dflen > 255) r->dflen = 255;
    strncpy(r->dfLFN, dir, r->dflen);
    */
    
    pfat->fatRootdir = r;

    return 0;
}

static int aspFS_insertFATChilds(struct sdFAT_s *pfat, struct directnFile_s *root, char *dir, int max)
{
#define TAB_DEPTH   4
    int ret = 0, cnt = 0;
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    struct directnFile_s *dfs = 0;

    char *dkbuf=0;

    if (!root) {
        printf("[R]root error 0x%x\n", root);
        ret = -1;
        goto insertEnd;
    }

    if (root->dflen) {
        printf("[R]open directory [%s] - %d\n", root->dfLFN, root->dfclstnum);
    } else {
        printf("[R]open directory [%s] - %d\n", root->dfSFN, root->dfclstnum);
    }

    if ((!dir) || (max <=0)) {
        printf("[R]Can`t open directory \n");
        ret = -2;
        goto insertEnd;
    }

    dkbuf = dir;
    mspFS_allocDir(pfat, &dfs);
    if (!dfs) {
        ret = -3;
        goto insertEnd;
    }
    memset(dfs, 0, sizeof(struct directnFile_s));

    cnt = 0;
    ret = aspRawParseDir(dkbuf, dfs, max);
    printf("[R]raw parsing cnt: %d \n", ret);
    while (max > 0) {
        if (dfs->dfstats) {
            //printf("[R]short name: %s \n", dfs->dfSFN);
            if (dfs->dflen > 0) {
                //printf("[R]long name: %s, len:%d \n", dfs->dfLFN, dfs->dflen);
            }

            if (strcmp(dfs->dfSFN, ".") == 0 || 
                strcmp(dfs->dfSFN, "..") == 0 ) {
                
                memset(dfs, 0, sizeof(struct directnFile_s));
                
            } else {
            
                //debugPrintDir(dfs);
                aspFS_insertFATChild(root, dfs);

                mspFS_allocDir(pfat, &dfs);
                if (!dfs) {
                    ret = -3;
                    goto insertEnd;
                }
                memset(dfs, 0, sizeof(struct directnFile_s));
            }
        }

        dkbuf += ret;
        max -= ret;
        cnt++;
        
        ret = aspRawParseDir(dkbuf, dfs, max);
        if (!ret) break;
        //printf("[%d] ret: %d, last:%d \n", cnt, ret, max);
    }

    printf("[R]raw parsing end: %d \n", ret);

insertEnd:

    return ret;
}

static int aspFS_insertFATChild(struct directnFile_s *parent, struct directnFile_s *r)
{
    int ret=0;
    struct directnFile_s *brt = 0;

    r->pa = parent;
    r->br = 0;
    r->ch = 0;

    if (parent->ch == 0) {
        parent->ch = r;
    } else {
        brt = parent->ch;
        if (brt->br == 0) {
            brt->br = r;
        } else {
            r->br = brt->br;
            brt->br = r;
        }
    }

//    ret = aspFS_insertFATChilds(r);
 
    return ret;
}

static int mspFS_insertFATChildDir(struct sdFAT_s *pfat, struct directnFile_s *parent, char * dir, int max)
{
    int ret;
    char mlog[256];

    struct directnFile_s *c = 0;
    struct directnFile_s *brt = 0;

    mspFS_allocDir(pfat, &c);
    if (!c) {
        return (-1);
    }
    memset(c, 0, sizeof(struct directnFile_s));

    c->pa = parent;
    c->br = 0;
    c->ch = 0;
    c->dftype = ASPFS_TYPE_DIR;
    c->dfattrib = 0;
    c->dfstats = ASPFS_STATUS_EN;
    c->dflen = 0;
    strcpy(c->dfSFN, "..");
    
    if (parent->ch == 0) {
        parent->ch = c;
    } else {
        brt = parent->ch;
        if (brt->br == 0) {
            brt->br = c;
        } else {
            c->br = brt->br;
            brt->br = c;
        }
    }

    ret = aspFS_insertFATChilds(pfat, parent, dir, max);
    
    return ret;
}
static int aspFS_list(struct directnFile_s *root, int depth)
{

    struct directnFile_s *fs = 0;
    if (!root) return (-1);

    fs = root->ch;
    while (fs) {
        printf("%*s%s[%d]\n", depth, "", fs->dfLFN, fs->dftype);
        if (fs->dftype == ASPFS_TYPE_DIR) {
            aspFS_list(fs, depth + 4);
        }
        fs = fs->br;
    }

    return 0;
}

static int mspSD_WriteFAT(int idx, int val, uint8_t *fat, uint32_t max) 
{
    uint8_t *uch;
    uint32_t offset=0, i=0;

    if (idx > max) {
        printf(" ERROR!! Get next Free FAT idx: %d > max: %d\n", idx, max);
        return 0xffffffff;
    }

    offset = idx * 4;

    uch = fat + offset;

    i = 0;
    while (i < 4) {
        uch[i] = val >> (i * 8);
        i++;
    }

    //printf("FAT val: %.2x - %.2x - %.2x - %.2x\n", uch[0], uch[1], uch[2], uch[3]);

    return 0;    
}

static int mspSD_writeList2FAT(int start, int length, uint8_t *fat, uint32_t max) 
{
    int i=0, ret=0;
    int cur=0;

    if (!length) return -1;

    cur = start;
    for (i = 0; i < length; i++) {
        if (cur > max) return -2;
        ret = mspSD_WriteFAT(cur, cur+1, fat, max);
        if (ret) return -3;
        cur++;
    }

    return 0;
}

static int mspSD_updLocalFAT(struct adFATLinkList_s *list, uint8_t *fat, uint32_t max) 
{
    int ret=0;
    uint32_t val=0;
    struct adFATLinkList_s *cur, *pre;
    if (!list) return -1;
    if (!fat) return -2;

    cur = list;

    while (cur) {
        val = mspSD_getNextFreeFAT(cur->ftStart, fat, max);
        if (val) {
            printf("  [FS] Warning!!! FAT should be zero val = 0x%x \n", val);
        } else {
            printf("  [FS] start FAT is zero idx: %d len: %d\n", cur->ftStart, cur->ftLen);
        }

        ret = mspSD_writeList2FAT(cur->ftStart, cur->ftLen, fat, max);
        if (ret) return -3;
        pre = cur;
        cur = cur->n;
        if (cur) {
            mspSD_WriteFAT(pre->ftStart+pre->ftLen-1, cur->ftStart, fat, max);
            printf("  [FS] CONT last idx: %d, next start: %d\n", pre->ftStart+pre->ftLen-1, cur->ftStart);
        } else {
            mspSD_WriteFAT(pre->ftStart+pre->ftLen-1, 0x0fffffff, fat, max);
            printf("  [FS] END last idx: %d, next start: %d\n", pre->ftStart+pre->ftLen-1);
        }
    }
    
    return 0;    
}

static int mspSD_rangeFATLinkList(struct adFATLinkList_s *list, int *start, int *last)
{
    int upb=-1, lob=-1;
    int bgn=0, end=0;
    struct adFATLinkList_s *cur;

    if (!list) return -1;
    if (!start) return -2;
    if (!last) return -3;
    
    cur = list;

    while (cur) {
        bgn = cur->ftStart;
        end = bgn + cur->ftLen;

        printf("  range: [%d + %d = %d]\n", bgn, cur->ftLen, end);

        if (lob < 0) {
            lob = bgn;
        } else {
            if (lob > bgn) {
                lob = bgn;
            }
        }

        if (upb < 0) {
            upb = end;
        } else {
            if (upb < end) {
                upb = end;
            }
        }
        
        cur = cur->n;
        printf("  range: [lob:%d, upb:%d]\n", lob, upb);
    }

    if ((upb < 0) || (lob < 0)) {
        return -4;
    } 

    *start = lob;
    *last = upb;
    
    return 0;
}

static int mspSD_createFATLinkList(struct adFATLinkList_s **list)
{
    struct adFATLinkList_s *newList=0;

    newList = (struct adFATLinkList_s *)aspMalloc(sizeof(struct adFATLinkList_s));
    if (!newList) return -1;

    memset(newList, 0, sizeof(struct adFATLinkList_s));

    *list = newList;
    return 0;
}

static uint32_t mspSD_getNextFAT(uint32_t idx, uint8_t *fat, uint32_t max) 
{
    uint8_t *uch;
    uint32_t offset=0, i=0;
    uint32_t val = 0;

    if (idx > max) {
        printf(" ERROR!! Get next FAT idx: %d > max: %d\n", idx, max);
        return 0xffffffff;
    }

    if (!idx) {
        return 0;
    }
    offset = idx * 4;

    uch = fat + offset;
    msync(uch, 16, MS_SYNC);
    
    i = 0;
    while (i < 4) {
        val |= uch[i] << (i * 8);
        i++;
    }

    //printf("   Get next FAT val: %d\n", val);

    return val;    
}

inline int mspSD_parseFAT2LinkList(struct adFATLinkList_s **head, uint32_t idx, uint8_t *fat, uint32_t max)
{
    struct adFATLinkList_s *p=0;
    uint32_t llen=0, lstr=0, nxt=0, cur=0, ret=0;
    struct adFATLinkList_s *ls=0, *nt=0;
    cur = idx;

    ret = mspSD_createFATLinkList(&ls);
    if (ret) return ret;

    *head = ls;

    if (cur < 2) {
        cur = 0;
    }

    lstr = cur; 
    llen = 1;
    printf("  start %d, %d\n", lstr, llen);
    
    nxt = mspSD_getNextFAT(cur, fat, max);
    while (nxt) {
        if (nxt == 0x0ffffff8) {
            printf("  empty %d, %d\n", lstr, llen);
            break;
        } else if (nxt == 0x0fffffff) {
            printf("  end %d, %d\n", lstr, llen);
            break;
        } else if (nxt == 0xffffffff) {
            printf("  error %d, %d\n", lstr, llen);
            return -1;
            break;
        }

        //printf("  compare nxt:%d and cur:%d\n", nxt, cur);    
        
        if (nxt == (cur+1)) {
            cur = nxt;
            nxt = 0;
            llen += 1;
        } else {
            ls->ftStart = lstr;
            ls->ftLen = llen;

            ret = mspSD_createFATLinkList(&ls->n);
            if (ret) return ret;

            //printf("  diff nxt:%d, cur:%d str:%d, len:%d\n", nxt, cur, lstr, llen);

            cur = nxt;
            lstr = cur;
            llen = 1; 
            ls = ls->n;
        }
        nxt = mspSD_getNextFAT(cur, fat, max);
        //printf("  return nxt:%d\n", nxt);
    }

    if (!lstr) {
        llen = 0;
        printf("  empty %d, %d\n", lstr, llen);
    }

    ls->ftStart = lstr;
    ls->ftLen = llen;
    ls->n = 0;

    return 0;
}

inline uint32_t mspSD_getNextFreeFAT(uint32_t idx, uint8_t *fat, uint32_t max) 
{
    uint8_t *uch;
    uint32_t offset=0, i=0;
    uint32_t val = 0;

    if (idx > max) {
        printf(" ERROR!! Get next Free FAT idx: %d > max: %d\n", idx, max);
        return 0xffffffff;
    }

    offset = idx * 4;

    uch = fat + offset;

    i = 0;
    while (i < 4) {
        val |= uch[i] << (i * 8);
        i++;
    }

    //printf("   Get next Free FAT val: %d\n", val);

    return val;    
}

static int mspSD_getFreeFATList(struct adFATLinkList_s **head, uint32_t idx, uint8_t *fat, uint32_t max)
{
    uint32_t llen=0, lstr=0, nxt=0, cur=0, ret=0, ocp=0, fri=0;
    struct adFATLinkList_s *ls=0, *nt=0;
    cur = idx;

    ret = mspSD_createFATLinkList(&ls);
    if (ret) return ret;

    *head = ls;

    lstr = cur; 
    llen = 0;
    printf("  start %d, %d\n", lstr, llen);

    while (cur < max) {
        nxt = mspSD_getNextFreeFAT(cur, fat, max);

        if (nxt == 0x0ffffff8) {
            printf(" search start ...\n");
        } else if (nxt == 0xffffffff) {
            printf(" just start ... \n");
        } else if (nxt == 0x0fffffff) {
            ocp++;
            //printf("  [%d] ocp %d\n", cur, ocp);
        } else if (nxt == 0xffffffff) {
            printf("  [%d] error %d, %d\n", cur, lstr, llen);
            return -1;
        } else {

            if (!nxt) {
                fri ++;
                if (!lstr) {
                    lstr = cur;
                }
                llen += 1;
            } else {
                if (lstr) {
                    ls->ftStart = lstr;
                    ls->ftLen = llen;
                    ret = mspSD_createFATLinkList(&ls->n);
                    if (ret) return ret;

                    printf(" free sector, start:%d len:%d \n", lstr, llen);

                    lstr = 0;
                    llen = 0; 
                    ls = ls->n;
                } else {
                    ocp++;
                    //printf("  [%d] ocp %d\n", cur, ocp);
                }
            }
        }
        cur++;
    }

    if (ls->n) {
        aspFree(ls->n);
        ls->n = 0;
    } else {
        ls->ftStart = lstr;
        ls->ftLen = llen;
    }

    printf(" total free cluster %d, total used cluster %d \n", fri, ocp);
    
    return 0;
}

static int mspSD_getLastFATList(struct adFATLinkList_s **head, struct adFATLinkList_s *f)
{
    int ret=0;
    uint32_t str=0, len=0, acu=0;
    struct adFATLinkList_s *c=0, *t=0;
    struct adFATLinkList_s *ls=0, *nt=0;
    
    if (!f) return -1;
    ret = mspSD_createFATLinkList(&ls);
    if (ret) return ret;

    *head = ls;
    c = f;

    while (c->n) {
        c = c->n;
    }

    ls->ftLen = c->ftLen;
    ls->ftStart = c->ftStart;

    return 0;
}

static int mspSD_allocFreeFATList(struct adFATLinkList_s **head, uint32_t length, struct adFATLinkList_s *f, struct adFATLinkList_s **n)
{
    int ret=0;
    uint32_t str=0, len=0, acu=0;
    struct adFATLinkList_s *c=0, *t=0;
    struct adFATLinkList_s *ls=0, *nt=0;
    
    if (!f) return -1;
    ret = mspSD_createFATLinkList(&ls);
    if (ret) return ret;

    *head = ls;
    c = f;
    
    while (length) {
        if (!c) {
            return -2;
        }

        str = c->ftStart;
        len = c->ftLen;
        if (len > length) {
            ls->ftStart = str;
            ls->ftLen = length;
            
            len = len - length;
            str = str + length;

            c->ftStart = str;
            c->ftLen = len;
            length = 0;
            t = ls;
            break;
        } else {
            length = length - len;
            nt = c;
            
            ret = mspSD_createFATLinkList(&ls->n);
            if (ret) return ret;

            ls->ftStart = str;
            ls->ftLen = len;
            t = ls;
            ls = ls->n;
        }
        
        c = c->n;        
        
        if (nt) {
            aspFree(nt);
            nt = 0;
        }
    }

    if (!ls->ftLen) {
        t->n = 0;
        aspFree(ls);
    }

    *n = c;

    return 0;
}

static int mspFS_allocDir(struct sdFAT_s *psFat, struct directnFile_s **dir)
{
    char mlog[256];
    struct sdDirPool_s *pool;
    
    pool = psFat->fatDirPool;

    if (pool->dirUsed >= pool->dirMax) {
        *dir = 0;
        return -1;
    }

    *dir = &pool->dirPool[pool->dirUsed];
    pool->dirUsed += 1;
    
    //sprintf(mlog, "Pool [%d] used\n", pool->dirUsed);
    //print_f(mlogPool, "FS", mlog);

    return 0;
}

static int mspFS_createRoot(struct directnFile_s **root, struct sdFAT_s *psFat, char *dir)
{
    char mlog[256];
    DIR *dp;
    struct directnFile_s *r = 0, *c = 0;

    sprintf(mlog, "open directory [%s]\n", dir);
    print_f(mlogPool, "FS", mlog);

    if ((dp = opendir(dir)) == NULL) {
        sprintf(mlog, "Can`t open directory [%s]\n", dir);
        print_f(mlogPool, "FS", mlog);
        return (-1);
    }
    sprintf(mlog, "open directory [%s] done\n", dir);
    print_f(mlogPool, "FS", mlog);

    mspFS_allocDir(psFat, &r);
    //r = (struct directnFile_s *) aspMalloc(sizeof(struct directnFile_s));
    if (!r) {
        return (-2);
    }else {
            sprintf(mlog, "alloc root fs done [0x%x]\n", r);
            print_f(mlogPool, "FS", mlog);
    }

    mspFS_allocDir(psFat, &c);
    //c = (struct directnFile_s *) aspMalloc(sizeof(struct directnFile_s));
    if (!c) {
        return (-3);
    }else {
        sprintf(mlog, "alloc root fs first child done [0x%x]\n", c);
        print_f(mlogPool, "FS", mlog);
    }

    c->pa = r;
    c->br = 0;
    c->ch = 0;
    c->dftype = ASPFS_TYPE_DIR;
    c->dfattrib = 0;
    c->dfstats = 0;
    c->dflen = 2;
    strcpy(c->dfLFN, "..");

    r->pa = 0;
    r->br = 0;
    r->ch = c;
    r->dftype = ASPFS_TYPE_ROOT;
    r->dfattrib = 0;
    r->dfstats = 0;

    r->dflen = strlen(dir);
    //sprintf(mlog, "[%s] len: %d\n", dir, r->dflen);
    //print_f(mlogPool, "FS", mlog);
    if (r->dflen > 255) r->dflen = 255;
    strncpy(r->dfLFN, dir, r->dflen);

    *root = r;

    return 0;
}

static int mspFS_insertChilds(struct sdFAT_s *psFat, struct directnFile_s *root)
{
#define TAB_DEPTH   4
    int ret = 0;
    char mlog[256];
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;

    if (!root) {
        sprintf(mlog, "root error 0x%x\n", root);
        print_f(mlogPool, "FS", mlog);
        ret = -1;
        goto insertEnd;
    }

    //sprintf(mlog, "open directory [%s]\n", root->dfLFN);
    //print_f(mlogPool, "FS", mlog);

    if ((dp = opendir(root->dfLFN)) == NULL) {
        printf("Can`t open directory [%s]\n", root->dfLFN);
        ret = -2;
        goto insertEnd;
    }

    //sprintf(mlog, "open directory [%s] done\n", root->dfLFN);
    //print_f(mlogPool, "FS", mlog);
    
    chdir(root->dfLFN);
    while ((entry = readdir(dp)) != NULL) {
        lstat(entry->d_name, &statbuf);
        if (S_ISDIR(statbuf.st_mode)) {
            if (strcmp(entry->d_name, ".") == 0 || 
                strcmp(entry->d_name, "..") == 0 ) {
                continue;   
            }
            //sprintf(mlog, "%*s%s\n", TAB_DEPTH, "", entry->d_name, TAB_DEPTH);
            //print_f(mlogPool, "FS", mlog);
            //printdir(entry->d_name, TAB_DEPTH+4);
            ret = mspFS_insertChildDir(psFat, root, entry->d_name);
            if (ret) goto insertEnd;
        } else {
            //sprintf(mlog, "%*s%s\n", TAB_DEPTH, "", entry->d_name, TAB_DEPTH);
            //print_f(mlogPool, "FS", mlog);
            ret = mspFS_insertChildFile(psFat, root, entry->d_name);
            if (ret) goto insertEnd;
        }
    }
    chdir("..");    

insertEnd:
    closedir(dp);   

    return ret;
}
static int mspFS_insertChildDir(struct sdFAT_s *psFat, struct directnFile_s *parent, char *dir)
{
    int ret;
    char mlog[256];
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    struct directnFile_s *r = 0, *c = 0;
    struct directnFile_s *brt = 0;

    mspFS_allocDir(psFat, &r);
    //r = (struct directnFile_s *) aspMalloc(sizeof(struct directnFile_s));
    if (!r) return (-2);

    mspFS_allocDir(psFat, &c);
    //c = (struct directnFile_s *) aspMalloc(sizeof(struct directnFile_s));
    if (!c) {
        return (-3);
    }else {
        //sprintf(mlog, "alloc root fs first child done [0x%x]\n", c);
        //print_f(mlogPool, "FS", mlog);
    }

    c->pa = r;
    c->br = 0;
    c->ch = 0;
    c->dftype = ASPFS_TYPE_DIR;
    c->dfattrib = 0;
    c->dfstats = 0;
    c->dflen = 2;
    strcpy(c->dfLFN, "..");

    r->pa = parent;
    r->br = 0;
    r->ch = c;
    r->dfattrib = 0;
    r->dfstats = 0;
    r->dftype = ASPFS_TYPE_DIR;

    r->dflen = strlen(dir);
    //sprintf(mlog, "[%d][%s] len: %d \n", r->dftype, dir, r->dflen);
    //print_f(mlogPool, "FS", mlog);
    if (r->dflen > 255) r->dflen = 255;
    strncpy(r->dfLFN, dir, r->dflen+1);

    if (parent->ch == 0) {
        parent->ch = r;
    } else {
        brt = parent->ch;
        if (brt->br == 0) {
            brt->br = r;
        } else {
            r->br = brt->br;
            brt->br = r;
        }
    }

    ret = mspFS_insertChilds(psFat, r);
 
    return ret;
}

static int mspFS_insertChildFile(struct sdFAT_s *psFat, struct directnFile_s *parent, char *str)
{
    char mlog[256];
    struct directnFile_s *r = 0;
    struct directnFile_s *brt = 0;

    mspFS_allocDir(psFat, &r);
    //r = (struct directnFile_s *) aspMalloc(sizeof(struct directnFile_s));
    if (!r) return (-2);

    r->pa = parent;
    r->br = 0;
    r->ch = 0;
    r->dfattrib = 0;
    r->dfstats = 0;
    r->dftype = ASPFS_TYPE_FILE;

    r->dflen = strlen(str);
    //sprintf(mlog, "[%d][%s] len: %d \n", r->dftype, str, r->dflen);
    //print_f(mlogPool, "FS", mlog);
    if (r->dflen > 255) r->dflen = 255;
    strncpy(r->dfLFN, str, r->dflen+1);

    if (parent->ch == 0) {
        parent->ch = r;
    } else {
        brt = parent->ch;
        if (brt->br == 0) {
            brt->br = r;
        } else {
            r->br = brt->br;
            brt->br = r;
        }
    }
 
    return 0;
}

static int mspFS_listDetail(struct directnFile_s *root, int depth)
{
    char mlog[256];
    struct directnFile_s *fs = 0;
    if (!root) return (-1);

    fs = root->ch;
    while (fs) {
        if (fs->dflen) {
            sprintf(mlog, "%*s%s[%d]\n", depth, "", fs->dfLFN, fs->dftype);
            print_f(mlogPool, "FS", mlog);
        } else {
            sprintf(mlog, "%*s%s[%d]\n", depth, "", fs->dfSFN, fs->dftype);
            print_f(mlogPool, "FS", mlog);
        }

        debugPrintDir(fs);

        if (fs->dftype == ASPFS_TYPE_DIR) {
            mspFS_list(fs, depth + 4);
        }
        fs = fs->br;
    }

    return 0;
}

static int mspFS_list(struct directnFile_s *root, int depth)
{
    char mlog[256];
    struct directnFile_s *fs = 0;
    if (!root) return (-1);

    fs = root->ch;
    while (fs) {
        if (fs->dflen) {
            sprintf(mlog, "%*s%s[%d]\n", depth, "", fs->dfLFN, fs->dftype);
            print_f(mlogPool, "FS", mlog);
        } else {
            sprintf(mlog, "%*s%s[%d]\n", depth, "", fs->dfSFN, fs->dftype);
            print_f(mlogPool, "FS", mlog);
        }

        if (fs->dftype == ASPFS_TYPE_DIR) {
            mspFS_list(fs, depth + 4);
        }
        fs = fs->br;
    }

    return 0;
}

static int mspFS_folderList(struct directnFile_s *root, int depth)
{
    char mlog[256];
    struct directnFile_s *fs = 0;
    if (!root) return (-1);

    fs = root->ch;
    while (fs) {
        if (fs->dflen) {
            sprintf(mlog, "%*s%s[%d]\n", depth, "", fs->dfLFN, fs->dftype);
            print_f(mlogPool, "FS", mlog);
        } else {
            sprintf(mlog, "%*s%s[%d]\n", depth, "", fs->dfSFN, fs->dftype);
            print_f(mlogPool, "FS", mlog);
        }

        fs = fs->br;
    }

    return 0;
}

static int mspFS_Search(struct directnFile_s **dir, struct directnFile_s *root, char *path, int type)
{
    char mlog[256];
    int ret = 0;
    char split = '/';
    char *ch;
    char rmp[32][128];
    //char **rmp;
    int a = 0, b = 0, t = 0;
    struct directnFile_s *brt;

    ret = strlen(path);
    sprintf(mlog, "path[%s] root[%s] len:%d\n", path, root->dfSFN, ret);
    print_f(mlogPool, "FSRH2", mlog);

    memset(rmp, 0, 32*128);
    
    ch = path;
    while (ret > 0) {
        if (*ch == split) {
            if (b > 0) {
                b = 0;
                a++;
            }
        } else {
            rmp[a][b] = *ch;
            b++;
            //sprintf(mlog, "%x ", *ch);
            //print_f(mlogPool, "FS", mlog);
        }
        ch++;
        ret --;
    }

    sprintf(mlog, "\n a:%d, b:%d \n", a, b);
    print_f(mlogPool, "FSRH2", mlog);

    for (b = 0; b <= a; b++) {
        sprintf(mlog, "[%d.%d]%s \n", a, b, rmp[b]);
        print_f(mlogPool, "FSRH2", mlog);
    }

    sprintf(mlog, "search prepare: \n");
    print_f(mlogPool, "FSRH2", mlog);


    ret = -1;
    b = 0; t = 0;
    brt = root;
    while (brt) {
        //sprintf(mlog, "b:%d, [%s][0x%x][0x%x] pa[%s]\n", b, brt->dfSFN, brt->dfstats, brt->dftype, (brt->pa == 0)?"NONE":brt->pa->dfSFN);
        //print_f(mlogPool, "FSRH2", mlog);
        if (brt->dfstats != ASPFS_STATUS_EN) {
            //sprintf(mlog, "skip disable file in path [%s][%s][%s] \n", brt->dfLFN, brt->dfSFN, &rmp[b][0]);
            //print_f(mlogPool, "FSRH2", mlog);
        } else if (b == a) {
            if ((brt->dftype == type) || (brt->dftype == ASPFS_TYPE_ROOT)) {
                if ((strcmp(brt->dfLFN, &rmp[b][0]) == 0) || 
                    (strcmp(brt->dfSFN, &rmp[b][0]) == 0)) {
                    *dir = brt;
                    ret = 0;
                    break;
                }
            }
        } else {
            if (brt->dftype == ASPFS_TYPE_FILE) {
                //sprintf(mlog, "skip file in path [%s][%s][%s] \n", brt->dfLFN, brt->dfSFN, &rmp[b][0]);
                //print_f(mlogPool, "FSRH2", mlog);
            } else {
                if (brt->dflen) {
                    if (brt->dflen < 128) {
                        if (strcmp(brt->dfLFN, &rmp[b][0]) == 0) {
                            b++;
                        }
                    } else {
                        if (strcmp(brt->dfSFN, &rmp[b][0]) == 0) {
                            b++;
                        }
                    }
                } else {
                    if (strcmp(brt->dfSFN, &rmp[b][0]) == 0) {
                        b++;
                    }
                }
            }
        }

        if (b > t) {
            t = b;
            brt = brt->ch;
            if (!brt) break;
            //sprintf(mlog, "Next folder[%s] pa[%s]!!!\n", brt->dfSFN, brt->pa->dfSFN);
            //print_f(mlogPool, "FSRH2", mlog);
        } else {
            brt = brt->br;
        }
        
        if (b > a) {
            break;
        }
        
    }

    if (ret) {
        sprintf(mlog, "not found !!\n");
        print_f(mlogPool, "FSRH2", mlog);
    } else {
        sprintf(mlog, "found!! brt[%s] \n", brt->dfSFN);
        print_f(mlogPool, "FSRH2", mlog);
/*
        while((brt) && (b>=0)) {
            sprintf(mlog, "[%d][%s][%s] \n", b, &rmp[b][0], brt->dfLFN);
            print_f(mlogPool, "FSRH2", mlog);
            b--;
            brt = brt->pa;
        }
*/
    }

    aspFree(rmp);
    return ret;
}

static int mspFS_SearchInFolder(struct directnFile_s **dir, struct directnFile_s *folder, char *fname)
{
    char mlog[256];
    int ret = 0;
    struct directnFile_s *brt=0;

    if (!fname) return -1;
    if (!folder) return -2;

    *dir = 0;

    ret = strlen(fname);
    sprintf(mlog, "fname[%s] folder[%s] len:%d\n", fname, (folder->dflen==0)?folder->dfSFN:folder->dfLFN, ret);
    print_f(mlogPool, "FSRH3", mlog);

    ret = -1;
    brt = folder->ch;
    while (brt) {
        if ((brt->dfstats == ASPFS_STATUS_EN) && 
            (brt->dftype == ASPFS_TYPE_FILE)) {
            if ((strcmp(brt->dfLFN, fname) == 0) || 
                (strcmp(brt->dfSFN, fname) == 0)) {
                *dir = brt;
                ret = 0;
                break;
            }
        }
        
        brt = brt->br;        
    }

    if (ret) {
        sprintf(mlog, "[%s] not found !!\n", fname);
        print_f(mlogPool, "FSRH3", mlog);
    } else {
        sprintf(mlog, "found!! brt[%s] \n", brt->dfSFN);
        print_f(mlogPool, "FSRH3", mlog);
    }

    return ret;
}

static int mspFS_FileSearch(struct directnFile_s **dir, struct directnFile_s *root, char *path)
{
    char mlog[256];
    int ret = 0;
    char split = '/';
    char *ch;
    char rmp[32][128];
    //char **rmp;
    int a = 0, b = 0;
    struct directnFile_s *brt;

    ret = strlen(path);
    sprintf(mlog, "path[%s] root[%s] len:%d\n", path, root->dfSFN, ret);
    print_f(mlogPool, "FSRH", mlog);

    memset(rmp, 0, 32*128);
    
    ch = path;
    while (ret > 0) {
        if (*ch == split) {
            if (b > 0) {
                b = 0;
                a++;
            }
        } else {
            rmp[a][b] = *ch;
            b++;
            //sprintf(mlog, "%x ", *ch);
            //print_f(mlogPool, "FS", mlog);
        }
        ch++;
        ret --;
    }

    sprintf(mlog, "\n a:%d, b:%d \n", a, b);
    print_f(mlogPool, "FSRH", mlog);

    for (b = 0; b <= a; b++) {
        sprintf(mlog, "[%d.%d]%s \n", a, b, rmp[b]);
        print_f(mlogPool, "FSRH", mlog);
    }

    ret = -1;
    b = 0;
    brt = root;
    while (brt) {
        sprintf(mlog, "%d/ %s\n", b, brt->dfSFN);
        print_f(mlogPool, "FSRH", mlog);
        if (brt->dfstats != ASPFS_STATUS_EN) {
            sprintf(mlog, "skip disable file in path [%s][%s][%s] \n", brt->dfLFN, brt->dfSFN, &rmp[b][0]);
            print_f(mlogPool, "FSRH", mlog);
            brt = brt->br;
        } else if ((b < a) && (brt->dftype == ASPFS_TYPE_FILE)) {
            sprintf(mlog, "skip file in path [%s][%s][%s] \n", brt->dfLFN, brt->dfSFN, &rmp[b][0]);
            print_f(mlogPool, "FSRH", mlog);
            brt = brt->br;
        }
        else if ((b == a) && (brt->dftype == ASPFS_TYPE_DIR)) {
            sprintf(mlog, "skip folder in path [%s][%s][%s][0x%x] \n", brt->dfLFN, brt->dfSFN, &rmp[b][0], brt->dftype);
            print_f(mlogPool, "FSRH", mlog);
            brt = brt->br;
        }
        else if (strcmp("..", &rmp[b][0]) == 0) {
            b++;
            brt = brt->pa;
        } else if (strcmp(".", &rmp[b][0]) == 0) {
            b++;
            brt = brt->br;
        } else if (strcmp(brt->dfLFN, &rmp[b][0]) == 0) {
            b++;
            if (b > a) {
               *dir = brt;
                ret = 0;
                break;
            }
            brt = brt->ch;
        } else if ((!brt->dflen) && (strcmp(brt->dfSFN, &rmp[b][0]) == 0)) {
            b++;
            if (b > a) {
               *dir = brt;
                ret = 0;
                break;
            }
            brt = brt->ch;
        } else {
            brt = brt->br;
        }
    }

    sprintf(mlog, "path len: %d, match num: %d, brt:0x%x \n", a, b, brt);
    print_f(mlogPool, "FSRH", mlog);
/*
    while((brt) && (b>=0)) {
        //sprintf(mlog, "[%d][%s][%s] \n", b, &rmp[b][0], brt->dfLFN);
        //print_f(mlogPool, "FS", mlog);
        b--;
        brt = brt->pa;
    }
*/
    return ret;
}

static int mspFS_FolderSearch(struct directnFile_s **dir, struct directnFile_s *root, char *path)
{
    char mlog[256];
    int ret = 0;
    char split = '/';
    char *ch;
    char rmp[32][128];
    //char **rmp;
    int a = 0, b = 0;
    struct directnFile_s *brt;

    ret = strlen(path);
    sprintf(mlog, "path[%s] root[%s] len:%d\n", path, root->dfSFN, ret);
    print_f(mlogPool, "DSRH", mlog);

    //rmp = aspMalloc(256*256);
    memset(rmp, 0, 32*128);
    
    ch = path;
    while (ret > 0) {
        if (*ch == split) {
            if (b > 0) {
                b = 0;
                a++;
            }
        } else {
            rmp[a][b] = *ch;
            b++;
            //sprintf(mlog, "%x ", *ch);
            //print_f(mlogPool, "FS", mlog);
        }
        ch++;
        ret --;
    }

    /* TODO: fix the hanging bug */
    sprintf(mlog, "\n a:%d, b:%d \n", a, b);
    print_f(mlogPool, "DSRH", mlog);

    for (b = 0; b <= a; b++) {
        sprintf(mlog, "[%d.%d]%s \n", a, b, rmp[b]);
        print_f(mlogPool, "DSRH", mlog);
    }

    ret = -1;
    b = 0;
    brt = root;
    while (brt) {
        sprintf(mlog, "%d. %s\n", b, brt->dfSFN);
        print_f(mlogPool, "DSRH", mlog);
        if (brt->dfstats != ASPFS_STATUS_EN) {
            sprintf(mlog, "skip disable file in path [%s][%s][%s] \n", brt->dfLFN, brt->dfSFN, &rmp[b][0]);
            print_f(mlogPool, "FSRH", mlog);
            brt = brt->br;
        }
        else if ((b < a) && (brt->dftype == ASPFS_TYPE_FILE)) {
            sprintf(mlog, "skip file in path [%s][%s][%s] \n", brt->dfLFN, brt->dfSFN, &rmp[b][0]);
            print_f(mlogPool, "DSRH", mlog);
            brt = brt->br;
        }
        else if ((b == a) && (brt->dftype == ASPFS_TYPE_FILE)) {
            sprintf(mlog, "skip folder in path [%s][%s][%s] \n", brt->dfLFN, brt->dfSFN, &rmp[b][0]);
            print_f(mlogPool, "DSRH", mlog);
            brt = brt->br;
        }
        else if (strcmp("..", &rmp[b][0]) == 0) {
            b++;
            brt = brt->pa->pa;
            if (b > a) {
               *dir = brt;
                ret = 0;
                break;
            }
        } else if (strcmp(".", &rmp[b][0]) == 0) {
            b++;
            if (b > a) {
               *dir = brt;
                ret = 0;
                break;
            }            
            brt = brt->br;
        } else if (strcmp(brt->dfLFN, &rmp[b][0]) == 0) {
            b++;
            if (b > a) {
               *dir = brt;
                ret = 0;
                break;
            }
            brt = brt->ch;
        } else if ((!brt->dflen) && (strcmp(brt->dfSFN, &rmp[b][0]) == 0)) {
            b++;
            if (b > a) {
               *dir = brt;
                ret = 0;
                break;
            }
            brt = brt->ch;
        } else {
            brt = brt->br;
        }
    }

    sprintf(mlog, "path len: %d, match num: %d, brt:%s \n", a, b, brt->dfSFN);
    print_f(mlogPool, "DSRH", mlog);

/*
    while((brt) && (b>=0)) {
        //sprintf(mlog, "[%d][%s][%s] \n", b, &rmp[b][0], brt->dfLFN);
        //print_f(mlogPool, "FS", mlog);
        b--;
        brt = brt->pa;
    }
*/

    return ret;
}
static int mspFS_showFolder(struct directnFile_s *root)
{
    char mlog[256];
    struct directnFile_s *brt = 0;
    if (!root) return (-1);
    if (root->dftype == ASPFS_TYPE_FILE) return (-2);
    
    sprintf(mlog, "%s \n", root->dfLFN);
    print_f(mlogPool, "FS", mlog);

    brt = root->ch;
    while (brt) {
        sprintf(mlog, "|-[%c] %s\n", brt->dftype == ASPFS_TYPE_DIR?'D':'F', brt->dflen==0?brt->dfSFN:brt->dfLFN);
        print_f(mlogPool, "FS", mlog);
        brt = brt->br;
    }
    return 0;
}

static int mspFS_folderJump(struct directnFile_s **dir, struct directnFile_s *root, char *path)
{
    char mlog[256];
    int retval = 0;
    struct directnFile_s *brt;

    if ((!path) || (!root) || (!dir)) return (-1);
    if (root->dftype == ASPFS_TYPE_FILE) return (-2);

    if (strcmp("..", path) == 0) {
        if (root->dftype == ASPFS_TYPE_ROOT) {
            *dir = root;
            retval = 1;
        } else {
            *dir = root->pa;
            retval = 2;
        }
    } else if (strcmp(".", path) == 0) {
        *dir = root;
        retval = 3;
    } else {
        brt = root->ch;

        while (brt) {
            if (brt->dftype != ASPFS_TYPE_FILE) {
                if (brt->dflen) {
                    if (strcmp(brt->dfLFN, path) == 0) {
                        *dir = brt;
                        retval = 4;
                        break;
                    }
                } else {
                    if (strcmp(brt->dfSFN, path) == 0) {
                        *dir = brt;
                        retval = 5;
                        break;
                    }
                }
            }
            brt = brt->br;
        }

        if (!brt) retval = (-3);
    }

    return retval;
}

static int error_handle(char *log, int line)
{
#define MAX_LEN 256
#define TOT_LEN (MAX_LEN + 64)

    char str[TOT_LEN];
    int len=0;

    len = strlen(log);   
    if (len > 0) {
        if (len >= MAX_LEN) {
            len = MAX_LEN;
            log[len-1] = '\0';
        }

        sprintf(str, "warning: %s - line: %d\n", log, line); 
        print_f(mlogPool, "error", str); 

    } else {
        sprintf(str, "warning: read log failed - line: %d\n", line); 
        print_f(mlogPool, "error", str); 
    }

    //while(1);

    return 0;

}
inline uint16_t abs_info(struct info16Bit_s *p, uint16_t info)
{
    char str[128];

    p->data = info & 0xff;
    p->opcode = (info >> 8) & 0xff;
    //p->seqnum = (info >> 12) & 0x7;
    //p->inout = (info >> 15) & 0x1;

    //sprintf(str, "info: 0x%.4x \n", info); 
    //print_f(mlogPool, "abs_info", str); 

    return info;
}

inline uint16_t pkg_info(struct info16Bit_s *p)
{
    char str[128];
    uint16_t info = 0;
    info |= p->data & 0xff;
    info |= (p->opcode & 0xff) << 8;
    //info |= (p->seqnum & 0x7) << 12;
    //info |= (p->inout & 0x1) << 15;

    //sprintf(str, "info: 0x%.4x \n", info); 
    //print_f(mlogPool, "pkg_info", str); 

    return info;
}

inline uint32_t clr_bk(uint32_t bkf) 
{
    bkf &= ~0xffff0000;
    return bkf;
}

inline uint32_t emb_bk(uint32_t bkf, uint8_t evt, uint8_t ste) 
{
    bkf &= ~0xffff0000;
    bkf |= ((evt << 8) | ste) << 16;
    return bkf;
}

inline uint32_t clr_fw(uint32_t bkf) 
{
    bkf &= ~0x0000ffff;
    return bkf;
}

inline uint32_t emb_fw(uint32_t bkf, uint8_t evt, uint8_t ste) 
{
    bkf &= ~0x0000ffff;
    bkf |= (evt << 8) | ste;
    return bkf;
}

inline uint32_t abs_result(uint32_t result)
{
    result = result >> 16;
    return (result & 0xff);
}
inline uint32_t emb_result(uint32_t result, uint32_t flag) 
{
    result &= ~0xff0000;
    result |= (flag << 16) & 0xff0000;
    return result;
}

inline uint32_t emb_stanPro(uint32_t result, uint32_t rlt, uint32_t sta, uint32_t pro) 
{
    result &= ~0xffffff;
    result |= ((rlt & 0xff) << 16) | ((sta & 0xff) << 8) | (pro & 0xff);
    return result;
}

inline uint32_t emb_event(uint32_t result, uint32_t flag) 
{
    result &= ~0xff000000;
    result |= (flag & 0xff) << 24;
    return result;
}

inline uint32_t emb_state(uint32_t result, uint32_t flag) 
{
    char str[32];
    static int pre=0;
    pre = result;
    result &= ~0xff00;
    result |= (flag & 0xff) << 8;

    if (pre != result) {
        sprintf(str, "0x%.8x -> 0x%.8x\n", pre, result); 
        print_f(mlogPool, "state", str); 
    }

    return result;
}

inline uint32_t emb_process(uint32_t result, uint32_t flag) 
{
    result &= ~0xff;
    result |= flag & 0xff;
    return result;
}

static uint32_t next_METAT(struct psdata_s *data)
{
    int pro, rlt, next = 0;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    uint32_t bkf;
    bkf = data->bkofw;
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "bullet", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET:
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSACT;
                evt = SAVPARM; 
                break;
            case PSACT:
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSMAX;                
                break;
            case PSWT:
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSMAX;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSMAX;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "bullet", str);
                next = PSMAX;
                break;
            default:
                //sprintf(str, "default\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSSET;
                break;
        }
    }
    else if (rlt == BREAK) {
        tmpRlt = emb_result(tmpRlt, WAIT);
        next = pro;
    } else if (rlt == BKWRD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = (bkf >> 16) & 0xff;
            evt = (bkf >> 24) & 0xff;
            data->bkofw = clr_bk(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else if (rlt == FWORD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = bkf & 0xff;
            evt = (bkf >> 8) & 0xff;
            data->bkofw = clr_fw(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else {
        next = PSMAX;
    }
    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static uint32_t next_SAVPARM(struct psdata_s *data)
{
    int pro, rlt, next = 0;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    uint32_t bkf;
    bkf = data->bkofw;
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "bullet", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET:
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSMAX;
                break;
            case PSACT:
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSWT;                
                break;
            case PSWT:
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSACT;
                evt = METAT; 
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSACT;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "bullet", str);
                next = PSACT;
                break;
            default:
                //sprintf(str, "default\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSSET;
                break;
        }
    }
    else if (rlt == BREAK) {
        tmpRlt = emb_result(tmpRlt, WAIT);
        next = pro;
    } else if (rlt == BKWRD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = (bkf >> 16) & 0xff;
            evt = (bkf >> 24) & 0xff;
            data->bkofw = clr_bk(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else if (rlt == FWORD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = bkf & 0xff;
            evt = (bkf >> 8) & 0xff;
            data->bkofw = clr_fw(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else {
        next = PSMAX;
    }
    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static uint32_t next_VECTORS(struct psdata_s *data)
{
    int pro, rlt, next = 0;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    uint32_t bkf;
    bkf = data->bkofw;
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "bullet", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET:
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSMAX;
                break;
            case PSACT:
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSSET;
                evt = CROPR; 
                break;
            case PSWT:
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "bullet", str); 
                if (tmpAns == 1) {
                    next = PSRLT;
                } else if (tmpAns == 2) {
                    next = PSTSM;
                } else {
                    next = PSMAX;
                }
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSMAX;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "bullet", str);
                next = PSMAX;
                break;
            default:
                //sprintf(str, "default\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSSET;
                break;
        }
    }
    else if (rlt == BREAK) {
        tmpRlt = emb_result(tmpRlt, WAIT);
        next = pro;
    } else if (rlt == BKWRD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = (bkf >> 16) & 0xff;
            evt = (bkf >> 24) & 0xff;
            data->bkofw = clr_bk(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else if (rlt == FWORD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = bkf & 0xff;
            evt = (bkf >> 8) & 0xff;
            data->bkofw = clr_fw(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else {
        next = PSMAX;
    }
    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static uint32_t next_CROPR(struct psdata_s *data)
{
    int pro, rlt, next = 0;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    uint32_t bkf;
    bkf = data->bkofw;
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "bullet", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET:
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSACT;
                break;
            case PSACT:
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSWT;
                break;
            case PSWT:
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSRLT;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSTSM;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "bullet", str);
                next = PSSET;
                break;
            default:
                //sprintf(str, "default\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSSET;
                break;
        }
    }
    else if (rlt == BREAK) {
        tmpRlt = emb_result(tmpRlt, WAIT);
        next = pro;
    } else if (rlt == BKWRD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = (bkf >> 16) & 0xff;
            evt = (bkf >> 24) & 0xff;
            data->bkofw = clr_bk(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else if (rlt == FWORD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = bkf & 0xff;
            evt = (bkf >> 8) & 0xff;
            data->bkofw = clr_fw(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else {
        next = PSMAX;
    }
    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static uint32_t next_WTBAKQ(struct psdata_s *data)
{
    int pro, rlt, next = 0;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    uint32_t bkf;
    bkf = data->bkofw;
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "bullet", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET:
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "bullet", str); 
                if (tmpAns == 1) {
                    next = PSWT;
                } else if (tmpAns == 2) {
                    next = PSMAX;
                } else if (tmpAns == 3) {
                    next = PSSET;
                    evt = SDAM;
                } else {
                    next = PSMAX;
                }
                break;
            case PSACT:
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "bullet", str); 
                if (tmpAns == 1) {
                    next = PSWT;
                    evt = SDAO;
                } else if (tmpAns == 2) {
                    next = PSMAX;
                } else if (tmpAns == 3) {
                    next = PSSET;
                    evt = SDAO;
                } else {
                    next = PSMAX;
                }
                break;
            case PSWT:
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "bullet", str); 
                if (tmpAns == SINSCAN_WIFI_SD) {
                    next = PSWT;
                    evt = SUPI;
                } else if (tmpAns == SINSCAN_SD_ONLY) {
                    next = PSWT;
                    evt = WTBAKP;
                } else if (tmpAns == SINSCAN_DUAL_SD) {
                    next = PSSET;
                    evt = BULLET;
                } else {
                    next = PSMAX;
                }
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "bullet", str); 
                if (tmpAns == 1) {
                    next = PSMAX;
                } else if (tmpAns == 2) {
                    next = PSMAX;
                } else if (tmpAns == 3) {
                    next = PSMAX;
                } else {
                    next = PSMAX;
                }

                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "bullet", str);
                next = PSSET;
                evt = CROPR; 
                break;
            default:
                //sprintf(str, "default\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSSET;
                break;
        }
    }
    else if (rlt == BREAK) {
        tmpRlt = emb_result(tmpRlt, WAIT);
        next = pro;
    } else if (rlt == BKWRD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = (bkf >> 16) & 0xff;
            evt = (bkf >> 24) & 0xff;
            data->bkofw = clr_bk(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else if (rlt == FWORD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = bkf & 0xff;
            evt = (bkf >> 8) & 0xff;
            data->bkofw = clr_fw(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else {
        next = PSMAX;
    }
    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static uint32_t next_WTBAKP(struct psdata_s *data)
{
    int pro, rlt, next = 0;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    uint32_t bkf;
    bkf = data->bkofw;
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "bullet", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET:
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "bullet", str); 
                if (tmpAns == 1) {
                    next = PSWT;
                    evt = SUPI;
                } else if (tmpAns == 2) {
                    next = PSWT;
                    evt = WTBAKP;
                } else if (tmpAns == 3) {
                    next = PSSET;
                    evt = BULLET;
                } else {
                    next = PSMAX;
                }
                break;
            case PSACT:
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "bullet", str); 
                if (tmpAns == 1) {
                    next = PSMAX;
                } else if (tmpAns == 2) {
                    next = PSMAX;
                } else if (tmpAns == 3) {
                    next = PSMAX;;
                } else {
                    next = PSMAX;
                }
                break;
            case PSWT:
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSRLT;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSTSM;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "bullet", str);
                next = PSACT;
                evt = WTBAKQ; 
                break;
            default:
                //sprintf(str, "default\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSSET;
                break;
        }
    }
    else if (rlt == BREAK) {
        tmpRlt = emb_result(tmpRlt, WAIT);
        next = pro;
    } else if (rlt == BKWRD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = (bkf >> 16) & 0xff;
            evt = (bkf >> 24) & 0xff;
            data->bkofw = clr_bk(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else if (rlt == FWORD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = bkf & 0xff;
            evt = (bkf >> 8) & 0xff;
            data->bkofw = clr_fw(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else {
        next = PSMAX;
    }
    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static uint32_t next_SDAO(struct psdata_s *data)
{
    int pro, rlt, next = 0;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    uint32_t bkf;
    bkf = data->bkofw;
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "bullet", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET:
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSACT;
                evt = SDAM; 
                break;
            case PSACT:
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSTSM;
                evt = FATH; 
                break;
            case PSWT:
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSTSM;
                evt = FATH; 
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSTSM;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "bullet", str);
                next = PSTSM;
                evt = FATH; 
                break;
            default:
                //sprintf(str, "default\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSSET;
                break;
        }
    }
    else if (rlt == BREAK) {
        tmpRlt = emb_result(tmpRlt, WAIT);
        next = pro;
    } else if (rlt == BKWRD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = (bkf >> 16) & 0xff;
            evt = (bkf >> 24) & 0xff;
            data->bkofw = clr_bk(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else if (rlt == FWORD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = bkf & 0xff;
            evt = (bkf >> 8) & 0xff;
            data->bkofw = clr_fw(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else {
        next = PSMAX;
    }
    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static uint32_t next_SDAN(struct psdata_s *data)
{
    int pro, rlt, next = 0;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    uint32_t bkf;
    bkf = data->bkofw;
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "bullet", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET:
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSACT;
                break;
            case PSACT:
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSWT;
                break;
            case PSWT:
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSRLT;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSTSM;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "bullet", str);
                next = PSACT;
                evt = SDAO; 
                break;
            default:
                //sprintf(str, "default\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSSET;
                break;
        }
    }
    else if (rlt == BREAK) {
        tmpRlt = emb_result(tmpRlt, WAIT);
        next = pro;
    } else if (rlt == BKWRD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = (bkf >> 16) & 0xff;
            evt = (bkf >> 24) & 0xff;
            data->bkofw = clr_bk(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else if (rlt == FWORD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = bkf & 0xff;
            evt = (bkf >> 8) & 0xff;
            data->bkofw = clr_fw(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else {
        next = PSMAX;
    }
    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static uint32_t next_SDAM(struct psdata_s *data)
{
    int pro, rlt, next = 0;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    uint32_t bkf;
    bkf = data->bkofw;
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "bullet", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET:
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSACT;
                evt = SAVK; 
                break;
            case PSACT:
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSWT;
                break;
            case PSWT:
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSRLT;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSTSM;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "bullet", str);
                next = PSSET;
                evt = SDAN; 
                break;
            default:
                //sprintf(str, "default\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSSET;
                break;
        }
    }
    else if (rlt == BREAK) {
        tmpRlt = emb_result(tmpRlt, WAIT);
        next = pro;
    } else if (rlt == BKWRD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = (bkf >> 16) & 0xff;
            evt = (bkf >> 24) & 0xff;
            data->bkofw = clr_bk(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else if (rlt == FWORD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = bkf & 0xff;
            evt = (bkf >> 8) & 0xff;
            data->bkofw = clr_fw(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else {
        next = PSMAX;
    }
    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static uint32_t next_SDAL(struct psdata_s *data)
{
    int pro, rlt, next = 0;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    uint32_t bkf;
    bkf = data->bkofw;
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "bullet", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET:
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSACT;
                break;
            case PSACT:
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSWT;
                break;
            case PSWT:
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSRLT;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSTSM;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "bullet", str);
                next = PSMAX;
                break;
            default:
                //sprintf(str, "default\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSSET;
                break;
        }
    }
    else if (rlt == BREAK) {
        tmpRlt = emb_result(tmpRlt, WAIT);
        next = pro;
    } else if (rlt == BKWRD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = (bkf >> 16) & 0xff;
            evt = (bkf >> 24) & 0xff;
            data->bkofw = clr_bk(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else if (rlt == FWORD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = bkf & 0xff;
            evt = (bkf >> 8) & 0xff;
            data->bkofw = clr_fw(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else {
        next = PSMAX;
    }
    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static uint32_t next_SAVK(struct psdata_s *data)
{
    int pro, rlt, next = 0;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    uint32_t bkf;
    bkf = data->bkofw;
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "bullet", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET:
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSMAX;
                break;
            case PSACT:
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSWT;
                break;
            case PSWT:
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSRLT;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSTSM;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "bullet", str);
                next = PSSET;
                evt = SDAL; 
                break;
            default:
                //sprintf(str, "default\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSSET;
                break;
        }
    }
    else if (rlt == BREAK) {
        tmpRlt = emb_result(tmpRlt, WAIT);
        next = pro;
    } else if (rlt == BKWRD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = (bkf >> 16) & 0xff;
            evt = (bkf >> 24) & 0xff;
            data->bkofw = clr_bk(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else if (rlt == FWORD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = bkf & 0xff;
            evt = (bkf >> 8) & 0xff;
            data->bkofw = clr_fw(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else {
        next = PSMAX;
    }
    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static uint32_t next_SINJ(struct psdata_s *data)
{
    int pro, rlt, next = 0;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    uint32_t bkf;
    bkf = data->bkofw;
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "bullet", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET:
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSMAX; 
                break;
            case PSACT:
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSTSM;
                evt = FATH; 
                break;
            case PSWT:
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSTSM;
                evt = FATH; 
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSMAX;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "bullet", str);
                next = PSMAX;
                evt = SINJ; 
                break;
            default:
                //sprintf(str, "default\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSSET;
                break;
        }
    }
    else if (rlt == BREAK) {
        tmpRlt = emb_result(tmpRlt, WAIT);
        next = pro;
    } else if (rlt == BKWRD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = (bkf >> 16) & 0xff;
            evt = (bkf >> 24) & 0xff;
            data->bkofw = clr_bk(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else if (rlt == FWORD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = bkf & 0xff;
            evt = (bkf >> 8) & 0xff;
            data->bkofw = clr_fw(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else {
        next = PSMAX;
    }
    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static uint32_t next_SUPI(struct psdata_s *data)
{
    int pro, rlt, next = 0;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    uint32_t bkf;
    bkf = data->bkofw;
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "bullet", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET:
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSACT;
                break;
            case PSACT:
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSMAX;
                break;
            case PSWT:
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSRLT;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSTSM;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "bullet", str);
                next = PSSET;
                evt = SINJ; 
                break;
            default:
                //sprintf(str, "default\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSSET;
                break;
        }
    }
    else if (rlt == BREAK) {
        tmpRlt = emb_result(tmpRlt, WAIT);
        next = pro;
    } else if (rlt == BKWRD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = (bkf >> 16) & 0xff;
            evt = (bkf >> 24) & 0xff;
            data->bkofw = clr_bk(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else if (rlt == FWORD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = bkf & 0xff;
            evt = (bkf >> 8) & 0xff;
            data->bkofw = clr_fw(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else {
        next = PSMAX;
    }
    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static uint32_t next_FAT32H(struct psdata_s *data)
{
    int pro, rlt, next = 0;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    uint32_t bkf;
    bkf = data->bkofw;
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "bullet", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET:
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSACT;
                break;
            case PSACT:
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSWT;
                break;
            case PSWT:
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSRLT;                                /* end double side scan */
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSTSM;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "bullet", str); 
                if (tmpAns == 1) {
                    evt = FATH; 
                    next = PSTSM;
                } else if (tmpAns == 2) {
                    evt = REGF; 
                    next = PSWT;
                } else if (tmpAns == 3) {
                    evt = REGF; 
                    next = PSRLT;
                } else {
                    next = PSMAX;
                }
                break;
            default:
                //sprintf(str, "default\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSSET;
                break;
        }
    }
    else if (rlt == BREAK) {
        tmpRlt = emb_result(tmpRlt, WAIT);
        next = pro;
    } else if (rlt == BKWRD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = (bkf >> 16) & 0xff;
            evt = (bkf >> 24) & 0xff;
            data->bkofw = clr_bk(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else if (rlt == FWORD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = bkf & 0xff;
            evt = (bkf >> 8) & 0xff;
            data->bkofw = clr_fw(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else {
        next = PSMAX;
    }
    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static uint32_t next_FAT32G(struct psdata_s *data)
{
    int pro, rlt, next = 0;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    uint32_t bkf;
    bkf = data->bkofw;
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "bullet", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET:
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSACT;
                break;
            case PSACT:
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSWT;
                break;
            case PSWT:
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSRLT;                                /* end double side scan */
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSTSM;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "bullet", str); 
                evt = FATH; /* jump to next stage */
                next = PSSET;
                break;
            default:
                //sprintf(str, "default\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSSET;
                break;
        }
    }
    else if (rlt == BREAK) {
        tmpRlt = emb_result(tmpRlt, WAIT);
        next = pro;
    } else if (rlt == BKWRD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = (bkf >> 16) & 0xff;
            evt = (bkf >> 24) & 0xff;
            data->bkofw = clr_bk(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else if (rlt == FWORD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = bkf & 0xff;
            evt = (bkf >> 8) & 0xff;
            data->bkofw = clr_fw(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else {
        next = PSMAX;
    }
    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static uint32_t next_registerE(struct psdata_s *data)
{
    int pro, rlt, next = 0;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    uint32_t bkf;
    bkf = data->bkofw;
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "bullet", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET:
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSACT;
                break;
            case PSACT:
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSWT;
                break;
            case PSWT:
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSMAX;                                /* end double side scan */
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSTSM;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "bullet", str); 
                evt = REGF; /* jump to next stage */
                next = PSSET;
                break;
            default:
                //sprintf(str, "default\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSSET;
                break;
        }
    }
    else if (rlt == BREAK) {
        tmpRlt = emb_result(tmpRlt, WAIT);
        next = pro;
    } else if (rlt == BKWRD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = (bkf >> 16) & 0xff;
            evt = (bkf >> 24) & 0xff;
            data->bkofw = clr_bk(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else if (rlt == FWORD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = bkf & 0xff;
            evt = (bkf >> 8) & 0xff;
            data->bkofw = clr_fw(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else {
        next = PSMAX;
    }
    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static uint32_t next_registerF(struct psdata_s *data)
{
    int pro, rlt, next = 0;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    uint32_t bkf;
    bkf = data->bkofw;
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "bullet", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET:
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSACT;
                break;
            case PSACT:
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSMAX;
                break;
            case PSWT:
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSTSM;                                /* end double side scan */
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSTSM;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "bullet", str); 
                evt = FATG; /* jump to next stage */
                next = PSSET;
                break;
            default:
                //sprintf(str, "default\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSSET;
                break;
        }
    }
    else if (rlt == BREAK) {
        tmpRlt = emb_result(tmpRlt, WAIT);
        next = pro;
    } else if (rlt == BKWRD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = (bkf >> 16) & 0xff;
            evt = (bkf >> 24) & 0xff;
            data->bkofw = clr_bk(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else if (rlt == FWORD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = bkf & 0xff;
            evt = (bkf >> 8) & 0xff;
            data->bkofw = clr_fw(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else {
        next = PSMAX;
    }
    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static uint32_t next_doubleC(struct psdata_s *data)
{
    int pro, rlt, next = 0;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    uint32_t bkf;
    bkf = data->bkofw;
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "bullet", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET:
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSACT;
                break;
            case PSACT:
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSWT;
                break;
            case PSWT:
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSRLT;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSTSM;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSACT;
                evt = DOUBLED; /* jump to next stage */
                //next = PSMAX;
                break;
            default:
                //sprintf(str, "default\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSSET;
                break;
        }
    }
    else if (rlt == BREAK) {
        tmpRlt = emb_result(tmpRlt, WAIT);
        next = pro;
    } else if (rlt == BKWRD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = (bkf >> 16) & 0xff;
            evt = (bkf >> 24) & 0xff;
            data->bkofw = clr_bk(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else if (rlt == FWORD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = bkf & 0xff;
            evt = (bkf >> 8) & 0xff;
            data->bkofw = clr_fw(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else {
        next = PSMAX;
    }
    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static uint32_t next_doubleD(struct psdata_s *data)
{
    int pro, rlt, next = 0;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    uint32_t bkf;
    bkf = data->bkofw;
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "bullet", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET:
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSACT;
                break;
            case PSACT:
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSWT;
                break;
            case PSWT:
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSMAX;                                /* end double side scan */
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSMAX;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "bullet", str); 
                evt = REGE; /* jump to next stage */
                next = PSSET;
                break;
            default:
                //sprintf(str, "default\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSSET;
                break;
        }
    }
    else if (rlt == BREAK) {
        tmpRlt = emb_result(tmpRlt, WAIT);
        next = pro;
    } else if (rlt == BKWRD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = (bkf >> 16) & 0xff;
            evt = (bkf >> 24) & 0xff;
            data->bkofw = clr_bk(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else if (rlt == FWORD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = bkf & 0xff;
            evt = (bkf >> 8) & 0xff;
            data->bkofw = clr_fw(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else {
        next = PSMAX;
    }
    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static int next_spy(struct psdata_s *data)
{
    int pro, rlt, next = 0;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    uint32_t bkf;
    bkf = data->bkofw;
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "spy", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {

        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET:
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "spy", str); 
                next = PSMAX;
                break;
            case PSACT:
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "spy", str); 
                //next = PSWT;
                next = PSMAX; /* skip power on sequence */              
                break;
            case PSWT:
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "spy", str); 
                //next = PSRLT;
                next = PSMAX; /* skip power on sequence */
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "spy", str); 
                //next = PSTSM;
                next = PSMAX;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "spy", str); 
                next = PSSET; /* jump to next stage */
                evt = BULLET; /* jump to next stage */
                break;
            default:
                //sprintf(str, "default\n"); 
                //print_f(mlogPool, "spy", str); 
                next = PSSET;
                break;
        }
    }
    else if (rlt == BREAK) {
        tmpRlt = emb_result(tmpRlt, WAIT);
        next = pro;
    } else if (rlt == BKWRD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = (bkf >> 16) & 0xff;
            evt = (bkf >> 24) & 0xff;
            data->bkofw = clr_bk(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else if (rlt == FWORD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = bkf & 0xff;
            evt = (bkf >> 8) & 0xff;
            data->bkofw = clr_fw(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else {
        next = PSMAX;
    }
    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static uint32_t next_bullet(struct psdata_s *data)
{
    int pro, rlt, next = 0;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    uint32_t bkf;
    bkf = data->bkofw;
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "bullet", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET:
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSACT;
                break;
            case PSACT:
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSWT;
                break;
            case PSWT:
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSRLT;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSTSM;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSSET;
                evt = LASER; /* jump to next stage */
                //next = PSMAX;
                break;
            default:
                //sprintf(str, "default\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSSET;
                break;
        }
    }
    else if (rlt == BREAK) {
        tmpRlt = emb_result(tmpRlt, WAIT);
        next = pro;
    } else if (rlt == BKWRD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = (bkf >> 16) & 0xff;
            evt = (bkf >> 24) & 0xff;
            data->bkofw = clr_bk(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else if (rlt == FWORD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = bkf & 0xff;
            evt = (bkf >> 8) & 0xff;
            data->bkofw = clr_fw(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else {
        next = PSMAX;
    }
    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static int next_laser(struct psdata_s *data)
{
    int pro, rlt, next = 0;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    uint32_t bkf;
    bkf = data->bkofw;
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "laser", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET:
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "laser", str); 
                next = PSACT;
                break;
            case PSACT:
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "laser", str); 
                next = PSMAX; 
                break;
            case PSWT:
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "laser", str); 
                next = PSRLT;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "laser", str); 
                next = PSTSM;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "laser", str); 
                next = PSSET;
                evt = SPY; 
                break;
            default:
                //sprintf(str, "default\n"); 
                //print_f(mlogPool, "laser", str); 
                next = PSSET;
                break;
        }
    }
    else if (rlt == BREAK) {
        tmpRlt = emb_result(tmpRlt, WAIT);
        next = pro;
    } else if (rlt == BKWRD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = (bkf >> 16) & 0xff;
            evt = (bkf >> 24) & 0xff;
            data->bkofw = clr_bk(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else if (rlt == FWORD) {
        if (bkf) {
            tmpRlt = emb_result(tmpRlt, STINIT);
            next = bkf & 0xff;
            evt = (bkf >> 8) & 0xff;
            data->bkofw = clr_fw(data->bkofw);
        } else {
            next = PSMAX;
        }
    } else {
        next = PSMAX;
    }
    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static int next_error(struct psdata_s *data)
{
    int pro, rlt, next;
    char str[256];
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "error", str); 
                    
    if (rlt == NEXT) {
        switch (pro) {
            case PSSET:
                sprintf(str, "PSSET\n"); 
                print_f(mlogPool, "error", str); 
                next = PSACT;
                break;
            case PSACT:
                sprintf(str, "PSACT\n"); 
                print_f(mlogPool, "error", str); 
                next = PSWT;
                break;
            case PSWT:
                sprintf(str, "PSWT\n"); 
                print_f(mlogPool, "error", str); 
                next = PSRLT;
                break;
            case PSRLT:
                sprintf(str, "PSRLT\n"); 
                print_f(mlogPool, "error", str); 
                next = PSTSM;
                break;
            case PSTSM:
                sprintf(str, "PSTSM\n"); 
                print_f(mlogPool, "error", str); 
                next = PSSET;
                break;
            default:
                sprintf(str, "default\n"); 
                print_f(mlogPool, "error", str); 
                next = PSSET;
                break;
        }
    }
    next = 0; /* error handle, return to 0 */

    return emb_process(data->result, next);
}
static int ps_next(struct psdata_s *data)
{
    int sta, ret, evt, nxtst = -1, nxtrlt = 0;
    char str[256];

    sta = (data->result >> 8) & 0xff;
    nxtst = sta;

    //sprintf(str, "sta: 0x%x\n", sta); 
    //print_f(mlogPool, "ps_next", str); 

    switch (sta) {
        case SPY:
            ret = next_spy(data);
            evt = (ret >> 24) & 0xff;
            if (evt) nxtst = evt; /* long jump */
            break;
        case BULLET:
            ret = next_bullet(data);
            evt = (ret >> 24) & 0xff;
            if (evt) nxtst = evt; /* long jump */
            break;
        case LASER:
            ret = next_laser(data);
            evt = (ret >> 24) & 0xff;
            if (evt) nxtst = evt; /* long jump */
            //if (evt == 0x1) nxtst = SPY; /* end the test loop */
            break;
        case DOUBLEC:
            ret = next_doubleC(data);
            evt = (ret >> 24) & 0xff;
            if (evt) nxtst = evt; /* long jump */
            break;
        case DOUBLED:
            ret = next_doubleD(data);
            evt = (ret >> 24) & 0xff;
            if (evt) nxtst = evt; /* long jump */
            break;
        case REGE:
            ret = next_registerE(data);
            evt = (ret >> 24) & 0xff;
            if (evt) nxtst = evt; /* long jump */
            break;
        case REGF:
            ret = next_registerF(data);
            evt = (ret >> 24) & 0xff;
            if (evt) nxtst = evt; /* long jump */
            break;
        case FATG:
            ret = next_FAT32G(data);
            evt = (ret >> 24) & 0xff;
            if (evt) nxtst = evt; /* long jump */
            break;
        case FATH:
            ret = next_FAT32H(data);
            evt = (ret >> 24) & 0xff;
            if (evt) nxtst = evt; /* long jump */
            break;
        case SUPI:
            ret = next_SUPI(data);
            evt = (ret >> 24) & 0xff;
            if (evt) nxtst = evt; /* long jump */
            break;
        case SINJ:
            ret = next_SINJ(data);
            evt = (ret >> 24) & 0xff;
            if (evt) nxtst = evt; /* long jump */
            break;
        case SAVK:
            ret = next_SAVK(data);
            evt = (ret >> 24) & 0xff;
            if (evt) nxtst = evt; /* long jump */
            break;
        case SDAL:
            ret = next_SDAL(data);
            evt = (ret >> 24) & 0xff;
            if (evt) nxtst = evt; /* long jump */
            break;
        case SDAM:
            ret = next_SDAM(data);
            evt = (ret >> 24) & 0xff;
            if (evt) nxtst = evt; /* long jump */
            break;
        case SDAN:
            ret = next_SDAN(data);
            evt = (ret >> 24) & 0xff;
            if (evt) nxtst = evt; /* long jump */
            break;
        case SDAO:
            ret = next_SDAO(data);
            evt = (ret >> 24) & 0xff;
            if (evt) nxtst = evt; /* long jump */
            break;
        case WTBAKP:
            ret = next_WTBAKP(data);
            evt = (ret >> 24) & 0xff;
            if (evt) nxtst = evt; /* long jump */
            break;
        case WTBAKQ:
            ret = next_WTBAKQ(data);
            evt = (ret >> 24) & 0xff;
            if (evt) nxtst = evt; /* long jump */
            break;
        case CROPR:
            ret = next_CROPR(data);
            evt = (ret >> 24) & 0xff;
            if (evt) nxtst = evt; /* long jump */
            break;
        case VECTORS:
            ret = next_VECTORS(data);
            evt = (ret >> 24) & 0xff;
            if (evt) nxtst = evt; /* long jump */
            break;
        case SAVPARM:
            ret = next_SAVPARM(data);
            evt = (ret >> 24) & 0xff;
            if (evt) nxtst = evt; /* long jump */
            break;
        case METAT:
            ret = next_METAT(data);
            evt = (ret >> 24) & 0xff;
            if (evt) nxtst = evt; /* long jump */
            break;
        default:
            ret = next_error(data);
            evt = (ret >> 24) & 0xff;
            //if (evt == 0x1) nxtst = SPY;
            break;
    }

    nxtrlt = emb_state(ret, nxtst);

    //sprintf(str, "ret: 0x%.4x nxtst: 0x%x nxtrlt: 0x%.4x\n", ret, nxtst, nxtrlt); 
    //print_f(mlogPool, "ps_next", str); 
    
#if 0
    data->result += 1;
    if ((data->result & 0xf) == PSMAX) {
        data->result = (data->result & 0xf0) + 0x10;
    }

    if (((data->result & 0xf0) >> 4)== SMAX) {
        data->result = -1;
    }
#endif

    return nxtrlt;

}
static int stdob_01(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;

    struct info16Bit_s *p;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;

    rlt = abs_result(data->result); 

    //sprintf(str, "01 - rlt:0x%x \n", rlt); 
    //print_f(mlogPool, "dob", str); 

    switch (rlt) {
        case STINIT:
            ch = 25; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(str, "op_01: result: %x\n", data->result); 
            //print_f(mlogPool, "spy", str);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                //sprintf(str, "op_01: %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
                //print_f(mlogPool, "bullet", str);  
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stdob_02(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct info16Bit_s *p;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;

    rlt = abs_result(data->result); 

    //sprintf(str, "op_01 - rlt:0x%x \n", rlt); 
    //print_f(mlogPool, "spy", str); 

    switch (rlt) {
        case STINIT:
            ch = 27; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(str, "op_01: result: %x\n", data->result); 
            //print_f(mlogPool, "spy", str);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                //sprintf(str, "op_01: %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
                //print_f(mlogPool, "bullet", str);  
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stdob_03(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    rlt = abs_result(data->result); 

    //sprintf(str, "op_01 - rlt:0x%x \n", rlt); 
    //print_f(mlogPool, "spy", str); 

    switch (rlt) {
        case STINIT:
            ch = 29; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(str, "op_01: result: %x\n", data->result); 
            //print_f(mlogPool, "spy", str);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stdob_04(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    rlt = abs_result(data->result); 

    //sprintf(str, "op_01 - rlt:0x%x \n", rlt); 
    //print_f(mlogPool, "spy", str); 

    switch (rlt) {
        case STINIT:
            ch = 31; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(str, "op_01: result: %x\n", data->result); 
            //print_f(mlogPool, "spy", str);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stdob_05(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    rlt = abs_result(data->result); 

    //sprintf(str, "op_01 - rlt:0x%x \n", rlt); 
    //print_f(mlogPool, "spy", str); 

    switch (rlt) {
        case STINIT:
            ch = 34; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(str, "op_01: result: %x\n", data->result); 
            //print_f(mlogPool, "spy", str);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stdob_06(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    rlt = abs_result(data->result); 

    //sprintf(str, "op_01 - rlt:0x%x \n", rlt); 
    //print_f(mlogPool, "spy", str); 

    switch (rlt) {
        case STINIT:
            ch = 1; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(str, "op_01: result: %x\n", data->result); 
            //print_f(mlogPool, "spy", str);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stdob_07(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    rlt = abs_result(data->result); 

    //sprintf(str, "op_01 - rlt:0x%x \n", rlt); 
    //print_f(mlogPool, "spy", str); 

    switch (rlt) {
        case STINIT:
            ch = 38; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(str, "op_01: result: %x\n", data->result); 
            //print_f(mlogPool, "spy", str);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stdob_08(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    rlt = abs_result(data->result); 
    struct aspConfig_s *pct=0, *pdt=0;

    pct = data->rs->pcfgTable;

    //sprintf(str, "op_01 - rlt:0x%x \n", rlt); 
    //print_f(mlogPool, "spy", str); 

    switch (rlt) {
        case STINIT:
            ch = 6; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(str, "op_01: result: %x\n", data->result); 
            //print_f(mlogPool, "spy", str);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_EG_DECT];
                if ((pdt->opStatus == ASPOP_STA_UPD) && (pdt->opValue == 1)) {
#if CROP_USE_META
                    data->bkofw = emb_bk(data->bkofw, SAVPARM, PSRLT);
#else
                    data->bkofw = emb_bk(data->bkofw, WTBAKQ, PSTSM);
#endif
                    data->result = emb_result(data->result, BKWRD);
                } else {
                    data->result = emb_result(data->result, NEXT);
                }
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stdob_09(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    
    rlt = abs_result(data->result); 

    //sprintf(str, "wt_01 - rlt:0x%.8x \n", data->result); 
    //print_f(mlogPool, "wt", str); 

    switch (rlt) {
        case STINIT:
            ch = 101; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(str, "wt_01: result: 0x%.8x\n", data->result); 
            //print_f(mlogPool, "wt", str);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            //sprintf(str, "wt_01 - ans:0x%x \n", data->ansp0); 
            //print_f(mlogPool, "wt", str); 
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

#if test
    ASPOP_REG_RD,
    ASPOP_REG_WT,
    ASPOP_REG_ADDRH,
    ASPOP_REG_ADDRL,
    ASPOP_REG_DAT,
#endif
static int stdob_10(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;
    pct = rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op10 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "reg", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_REG_RD];
            if (pdt->opCode != OP_RGRD) {
                sprintf(rs->logs, "op10, REG_RD opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "reg", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (!(pdt->opStatus & ASPOP_STA_CON)) {
                sprintf(rs->logs, "op10, REG_RD status is wrong op:%x\n", pdt->opStatus); 
                print_f(rs->plogs, "reg", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_REG_RD];
                //pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int streg_11(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op11 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "reg", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_REG_ADDRH];
            if (pdt->opCode != OP_RGADD_H) {
                sprintf(rs->logs, "op11, OP_RGADD_H opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "reg", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (!(pdt->opStatus & ASPOP_STA_CON)) {
                sprintf(rs->logs, "op11, OP_RGADD_H status is wrong op:%x\n", pdt->opStatus); 
                print_f(rs->plogs, "reg", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_REG_ADDRH];
                //pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int streg_12(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op12 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "reg", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_REG_ADDRL];
            if (pdt->opCode != OP_RGADD_L) {
                sprintf(rs->logs, "op12, OP_RGADD_L opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "reg", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (!(pdt->opStatus & ASPOP_STA_CON)) {
                sprintf(rs->logs, "op12, OP_RGADD_L status is wrong op:%x\n", pdt->opStatus); 
                print_f(rs->plogs, "reg", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_REG_ADDRL];
                //pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int streg_13(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op13 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "reg", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_REG_DAT];
            if (pdt->opCode != OP_RGDAT) {
                sprintf(rs->logs, "op13, OP_RGDAT opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "reg", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (!(pdt->opStatus & ASPOP_STA_CON)) {
                sprintf(rs->logs, "op13, OP_RGDAT status is wrong op:%x\n", pdt->opStatus); 
                print_f(rs->plogs, "reg", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_REG_DAT];
                //pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, STINIT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int streg_14(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;
    pct = rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op14 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "reg", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_REG_WT];
            if (pdt->opCode != OP_RGWT) {
                sprintf(rs->logs, "op14, REG_WT opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "reg", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (!(pdt->opStatus & ASPOP_STA_CON)) {
                sprintf(rs->logs, "op14, REG_WT status is wrong op:%x\n", pdt->opStatus); 
                print_f(rs->plogs, "reg", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_REG_WT];
                //pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int streg_15(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op15 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "reg", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_REG_ADDRH];
            if (pdt->opCode != OP_RGADD_H) {
                sprintf(rs->logs, "op15, OP_RGADD_H opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "reg", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (!(pdt->opStatus & ASPOP_STA_CON)) {
                sprintf(rs->logs, "op15, OP_RGADD_H status is wrong op:%x\n", pdt->opStatus); 
                print_f(rs->plogs, "reg", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_REG_ADDRH];
                //pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int streg_16(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op16 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "reg", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_REG_ADDRL];
            if (pdt->opCode != OP_RGADD_L) {
                sprintf(rs->logs, "op16, OP_RGADD_L opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "reg", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (!(pdt->opStatus & ASPOP_STA_CON)) {
                sprintf(rs->logs, "op16, OP_RGADD_L status is wrong op:%x\n", pdt->opStatus); 
                print_f(rs->plogs, "reg", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_REG_ADDRL];
                //pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int streg_17(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op17 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "reg", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_REG_DAT];
            if (pdt->opCode != OP_RGDAT) {
                sprintf(rs->logs, "op17, OP_RGDAT opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "reg", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (!(pdt->opStatus & ASPOP_STA_CON)) {
                sprintf(rs->logs, "op17, OP_RGDAT status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "reg", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_REG_DAT];
                //pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stfat_18(struct psdata_s *data)
{
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_18 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "FAT", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDFAT_RD];
            if (pdt->opCode != OP_SDRD) {
                sprintf(rs->logs, "op18, OP_SDRD opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "FAT", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op18, OP_SDRD status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "FAT", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDFAT_RD];
                //pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stfat_19(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_19 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "FAT", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDFAT_WT];
            if (pdt->opCode != OP_SDWT) {
                sprintf(rs->logs, "op19, OP_SDWT opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "FAT", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op19, OP_SDWT status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "FAT", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDFAT_WT];
                //pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stfat_20(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_20 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "FAT", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDFAT_STR01];
            if (pdt->opCode != OP_STSEC_00) {
                sprintf(rs->logs, "op20, OP_STSEC_0 opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "FAT", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op20, OP_STSEC_0 status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "FAT", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
            
                //sprintf(rs->logs, "!!!! op20, ASPOP_SDFAT_STR01: %d !!!!\n", pdt->opValue); 
                //print_f(rs->plogs, "FAT", rs->logs);  

                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDFAT_STR01];
                //pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stfat_21(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_21 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "FAT", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDFAT_STR02];
            if (pdt->opCode != OP_STSEC_01) {
                sprintf(rs->logs, "op21, OP_STSEC_1 opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "FAT", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op21, OP_STSEC_1 status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "FAT", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
            
                //sprintf(rs->logs, "!!!! op21, ASPOP_SDFAT_STR02: %d !!!!\n", pdt->opValue); 
                //print_f(rs->plogs, "FAT", rs->logs);  

                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDFAT_STR02];
                //pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stfat_22(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_22 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "FAT", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDFAT_STR03];
            if (pdt->opCode != OP_STSEC_02) {
                sprintf(rs->logs, "op22, OP_STSEC_2 opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "FAT", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op22, OP_STSEC_2 status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "FAT", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
            
                //sprintf(rs->logs, "!!!! op22, ASPOP_SDFAT_STR03: %d !!!!\n", pdt->opValue); 
                //print_f(rs->plogs, "FAT", rs->logs);  

                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDFAT_STR03];
                //pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stfat_23(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_23 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "FAT", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDFAT_STR04];
            if (pdt->opCode != OP_STSEC_03) {
                sprintf(rs->logs, "op23, OP_STSEC_3 opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "FAT", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op23, OP_STSEC_3 status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "FAT", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
            
                //sprintf(rs->logs, "!!!! op23, ASPOP_SDFAT_STR04: %d !!!!\n", pdt->opValue); 
                //print_f(rs->plogs, "FAT", rs->logs);  

                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDFAT_STR04];
                //pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stfat_24(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_24 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "FAT", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDFAT_LEN01];
            if (pdt->opCode != OP_STLEN_00) {
                sprintf(rs->logs, "op24, OP_STLEN_0 opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "FAT", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op24, OP_STLEN_0 status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "FAT", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDFAT_LEN01];
                //pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stfat_25(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_25 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "FAT", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDFAT_LEN02];
            if (pdt->opCode != OP_STLEN_01) {
                sprintf(rs->logs, "op25, OP_STLEN_1 opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "FAT", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op25, OP_STLEN_1 status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "FAT", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDFAT_LEN02];
                //pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stfat_26(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_26 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "FAT", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDFAT_LEN03];
            if (pdt->opCode != OP_STLEN_02) {
                sprintf(rs->logs, "op26, OP_STLEN_2 opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "FAT", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op26, OP_STLEN_2 status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "FAT", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDFAT_LEN03];
                //pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stfat_27(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_27 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "FAT", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDFAT_LEN04];
            if (pdt->opCode != OP_STLEN_03) {
                sprintf(rs->logs, "op27, OP_STLEN_3 opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "FAT", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op27, OP_STLEN_3 status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "FAT", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDFAT_LEN04];
                //pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stfat_28(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_28 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "FAT", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDFAT_SDAT];
            if (pdt->opCode != OP_SDAT) {
                sprintf(rs->logs, "op28, OP_SDAT opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "FAT", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op28, OP_SDAT status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "FAT", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDFAT_SDAT];
                //pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stfat_29(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    struct sdFAT_s *pFat=0;
    struct info16Bit_s *p=0, *c=0;

    pFat = data->rs->psFat;
    
    rs = data->rs;
    rlt = abs_result(data->result); 
    
    p = &rs->pmch->tmp;
    c = &rs->pmch->cur;

    //sprintf(rs->logs, "op_29 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "FAT", rs->logs);  

    switch (rlt) {
        case STINIT:

            if (!(pFat->fatStatus & ASPFAT_STATUS_BOOT_SEC)) {
                ch = 45;             
            } else {
                if (pFat->fatStatus & ASPFAT_STATUS_SDRD) {
                    ch = 72;
                } else if (pFat->fatStatus & ASPFAT_STATUS_SDWT) {
                    ch = 77;
                } else if (pFat->fatStatus & ASPFAT_STATUS_SDWBK) {
                    ch = 95;
                } else if (pFat->fatStatus & ASPFAT_STATUS_FATWT) {
                    ch = 82;
                } else if (pFat->fatStatus & ASPFAT_STATUS_DFECHK) {
                    ch = 45;
                } else if (pFat->fatStatus & ASPFAT_STATUS_DFERD) {
                    ch = 45;
                } else if (pFat->fatStatus & ASPFAT_STATUS_DFEWT) {
                    ch = 89;
                } else if ((c->opinfo == pFat->fatBootsec->secWhfat) && 
                    (p->opinfo == pFat->fatBootsec->secPrfat)) {
                    ch = 54; 
                } else {
                    ch = 45; 
                }
            }

            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(rs->logs, "op_29: result: %x, goto %d, fatStatus: %x\n", data->result, ch, pFat->fatStatus); 
            print_f(rs->plogs, "FAT", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stfat_30(struct psdata_s *data)
{ 
    int ret=0, offset=0, val=0;
    struct sdFAT_s *pFat=0;
    uint32_t secStr=0, secLen=0;
    char str[128], ch = 0; 
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->tmp;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    uint32_t rlt;
    rlt = abs_result(data->result); 
    pFat = data->rs->psFat;

    sprintf(rs->logs, "op_30 rlt:0x%x fat:0x%.8x\n", rlt, pFat->fatStatus); 
    print_f(rs->plogs, "FAT", rs->logs);  

    switch (rlt) {
        case STINIT:
        
            if (!(pFat->fatStatus & ASPFAT_STATUS_BOOT_SEC)) {
                secStr = 0 + pFat->fatBootsec->secBoffset;
                secLen = 16;
                c->opinfo = secStr;
                p->opinfo = secLen;

                sprintf(rs->logs, "BOOT_SEC secStr:%d, secLen:%d, fat status:0x%.8x \n", secStr, secLen, pFat->fatStatus); 
                print_f(rs->plogs, "FAT", rs->logs);  

                ch = 50;
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            } else if (!(pFat->fatStatus & ASPFAT_STATUS_FAT)) {
                secStr = pFat->fatBootsec->secWhfat;
                secLen = pFat->fatBootsec->secPrfat;

                c->opinfo = secStr;
                p->opinfo = secLen;
                
                ch = 53;
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            } else if (!(pFat->fatStatus & ASPFAT_STATUS_ROOT_DIR)) {
                secStr = pFat->fatBootsec->secWhroot;
                secLen = pFat->fatBootsec->secPrClst;

                c->opinfo = secStr;
                p->opinfo = secLen;

                if (secLen < 16) secLen = 16;

                sprintf(rs->logs, "ROOT_DIR secStr:%d, secLen:%d, fat status:0x%.8x \n", secStr, secLen, pFat->fatStatus); 
                print_f(rs->plogs, "FAT", rs->logs);  
                
                ch = 51;
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            } else if (!(pFat->fatStatus & ASPFAT_STATUS_FOLDER)) {
                sprintf(rs->logs, "FOLDER fat status:0x%.8x \n", pFat->fatStatus); 
                print_f(rs->plogs, "FAT", rs->logs);  

                ch = 52;
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            } else if ((pFat->fatStatus & ASPFAT_STATUS_SDRD)) {
                sprintf(rs->logs, "SD Read to APP status:0x%.8x \n", pFat->fatStatus); 
                print_f(rs->plogs, "FAT", rs->logs);  

                ch = 71; /* LOV->MSP->APP */
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            } else if ((pFat->fatStatus & ASPFAT_STATUS_SDWT)) {
                sprintf(rs->logs, "APP write data to SD status:0x%.8x \n", pFat->fatStatus); 
                print_f(rs->plogs, "FAT", rs->logs);  

                ch = 76; /* APP->MSP->LOV */
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            } else if ((pFat->fatStatus & ASPFAT_STATUS_SDWBK)) {
                sprintf(rs->logs, "APP write SD data BACK to SD status:0x%.8x \n", pFat->fatStatus); 
                print_f(rs->plogs, "FAT", rs->logs);  

                ch = 94; /* MSP->LOV */
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            } else if ((pFat->fatStatus & ASPFAT_STATUS_FATWT)) {
                sprintf(rs->logs, "APP write FAT to SD status:0x%.8x \n", pFat->fatStatus); 
                print_f(rs->plogs, "FAT", rs->logs);  

                ch = 80; /* APP->MSP->LOV */
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            } else if ((pFat->fatStatus & ASPFAT_STATUS_DFECHK)) {
                sprintf(rs->logs, "APP read DFE from SD status:0x%.8x \n", pFat->fatStatus); 
                print_f(rs->plogs, "FAT", rs->logs);  

                ch = 81; /* APP->MSP->LOV */
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            } else if ((pFat->fatStatus & ASPFAT_STATUS_DFEWT)) {
                sprintf(rs->logs, "APP write DFE to SD status:0x%.8x \n", pFat->fatStatus); 
                print_f(rs->plogs, "FAT", rs->logs);  

                ch = 88; /* APP->MSP->LOV */
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            } else {
                ch = 56; /* show the folder tree */
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }
            
            //sprintf(str, "op_30: result: %x\n", data->result); 
            //print_f(mlogPool, "FAT", str);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 3) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 4) {
                data->result = emb_result(data->result, FWORD);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            } else {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsup_31(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    struct sdFAT_s *pFat=0;
    struct info16Bit_s *p=0, *c=0;

    pFat = data->rs->psFat;
    
    rs = data->rs;
    rlt = abs_result(data->result); 
    
    p = &rs->pmch->tmp;
    c = &rs->pmch->cur;

    //sprintf(rs->logs, "op_31 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SUP", rs->logs);  

    switch (rlt) {
        case STINIT:

            ch = 60;

            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(rs->logs, "op_31: result: %x, goto %d \n", data->result, ch); 
            print_f(rs->plogs, "SUP", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsup_32(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    struct sdFAT_s *pFat=0;
    struct info16Bit_s *p=0, *c=0;

    pFat = data->rs->psFat;
    
    rs = data->rs;
    rlt = abs_result(data->result); 
    
    p = &rs->pmch->tmp;
    c = &rs->pmch->cur;

    //sprintf(rs->logs, "op_32 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SUP", rs->logs);  

    switch (rlt) {
        case STINIT:
            ch = 48; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(rs->logs, "op_32: result: %x, goto %d\n", data->result, ch); 
            print_f(rs->plogs, "SUP", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsup_33(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    
    rs = data->rs;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_33 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SIG", rs->logs);  

    switch (rlt) {
        case STINIT:
            ch = 25; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(rs->logs, "op_33: result: %x, goto %d\n", data->result, ch); 
            //print_f(rs->plogs, "SIG", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsup_34(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    struct info16Bit_s *p=0, *c=0;
    struct aspConfig_s *pct=0, *pdt=0;

    pct = data->rs->pcfgTable;
    rs = data->rs;
    rlt = abs_result(data->result); 
    
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    //sprintf(rs->logs, "op_34 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SIG", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SCAN_SINGLE];
            if (pdt->opCode != OP_SINGLE) {
                sprintf(rs->logs, "op34, OP_SINGLE opcode is wrong val:%x\n", pdt->opCode); 
                print_f(rs->plogs, "SIG", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (!(pdt->opStatus & ASPOP_STA_WR)) {
                sprintf(rs->logs, "op34, OP_SINGLE status is wrong val:%x\n", pdt->opStatus); 
                print_f(rs->plogs, "SIG", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                switch(pdt->opValue) {
                    case SINSCAN_WIFI_ONLY:
                    //case SINSCAN_SD_ONLY:
                    case SINSCAN_WIFI_SD:
                    //case SINSCAN_WHIT_BLNC:
                    //case SINSCAN_USB:
                    //case SINSCAN_DUAL_STRM: /*not going here*/
                    //case SINSCAN_DUAL_SD:
                        ch = 41; 
                        c->opcode =  pdt->opCode;
                        c->data = pdt->opValue;
                        memset(p, 0, sizeof(struct info16Bit_s));

                        rs_ipc_put(data->rs, &ch, 1);
                        data->result = emb_result(data->result, WAIT);
                        //sprintf(rs->logs, "op_34: result: %x, goto %d\n", data->result, ch); 
                        //print_f(rs->plogs, "SIG", rs->logs);  
                        break;
                    default:
                        sprintf(rs->logs, "WARNING!!! op34, opValue is unexpected val:%x\n", pdt->opValue);
                        print_f(rs->plogs, "SIG", rs->logs);  
                        data->result = emb_result(data->result, EVTMAX);
                        break;
                }
            }        
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                if (p->data == c->data) {
                    data->result = emb_result(data->result, NEXT);
                } else {
                    data->result = emb_result(data->result, EVTMAX);
                }
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsup_35(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;

    rs = data->rs;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_35 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SIG", rs->logs);  

    switch (rlt) {
        case STINIT:
            ch = 67; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(rs->logs, "op_35: result: %x, goto %d\n", data->result, ch); 
            //print_f(rs->plogs, "SIG", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsin_36(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    struct aspConfig_s *pct=0, *pdt=0;

    pct = data->rs->pcfgTable;
    rs = data->rs;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_36 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SIN", rs->logs);  

    switch (rlt) {
        case STINIT:
            ch = 48; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(rs->logs, "op_36: result: %x, goto %d\n", data->result, ch); 
            //print_f(rs->plogs, "SIN", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_EG_DECT];
                if ((pdt->opStatus == ASPOP_STA_UPD) && (pdt->opValue == 1)) {
#if CROP_USE_META
                    data->bkofw = emb_bk(data->bkofw, SAVPARM, PSRLT);
#else
                    data->bkofw = emb_bk(data->bkofw, WTBAKQ, PSTSM);
#endif
                    data->result = emb_result(data->result, BKWRD);
                } else {
                    data->result = emb_result(data->result, FWORD);
                }
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stdow_37(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    
    rs = data->rs;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_37 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "DOW", rs->logs);  

    switch (rlt) {
        case STINIT:
            ch = 70; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(rs->logs, "op_37: result: 0x%x, goto %d\n", data->result, ch); 
            //print_f(rs->plogs, "DOW", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stupd_38(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    
    rs = data->rs;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_38 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "UPD", rs->logs);  

    switch (rlt) {
        case STINIT:
            ch = 75; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(rs->logs, "op_38: result: %x, goto %d\n", data->result, ch); 
            //print_f(rs->plogs, "UPD", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stupd_39(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    
    rs = data->rs;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_39 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "UPD", rs->logs);  

    switch (rlt) {
        case STINIT:
            ch = 0; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(rs->logs, "op_39: result: %x, goto %d\n", data->result, ch); 
            //print_f(rs->plogs, "UPD", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stupd_40(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    
    rs = data->rs;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_40 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "UPD", rs->logs);  

    switch (rlt) {
        case STINIT:
            ch = 0; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(rs->logs, "op_40: result: %x, goto %d\n", data->result, ch); 
            //print_f(rs->plogs, "UPD", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsav_41(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    
    rs = data->rs;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_41 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SAV", rs->logs);  

    switch (rlt) {
        case STINIT:
            ch = 84; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(rs->logs, "op_41: result: %x, goto %d\n", data->result, ch); 
            //print_f(rs->plogs, "SAV", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsda_42(struct psdata_s *data)
{
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_42 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SDA", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDFREE_FREESEC];
            if (pdt->opCode != OP_FREESEC) {
                sprintf(rs->logs, "op42, OP_FREESEC opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op42, OP_FREESEC status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDFREE_FREESEC];
                pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsda_43(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_43 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SDA", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDFREE_STR01];
            if (pdt->opCode != OP_STSEC_00) {
                sprintf(rs->logs, "op43, OP_STSEC_0 opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op43, OP_STSEC_0 status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDFREE_STR01];
                pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsda_44(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_44 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SDA", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDFREE_STR02];
            if (pdt->opCode != OP_STSEC_01) {
                sprintf(rs->logs, "op44, OP_STSEC_1 opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op44, OP_STSEC_1 status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDFREE_STR02];
                pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsda_45(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_45 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SDA", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDFREE_STR03];
            if (pdt->opCode != OP_STSEC_02) {
                sprintf(rs->logs, "op45, OP_STSEC_2 opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op45, OP_STSEC_2 status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDFREE_STR03];
                pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsda_46(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_46 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SDA", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDFREE_STR04];
            if (pdt->opCode != OP_STSEC_03) {
                sprintf(rs->logs, "op46, OP_STSEC_3 opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op46, OP_STSEC_3 status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDFREE_STR04];
                pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsda_47(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_47 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SDA", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDFREE_LEN01];
            if (pdt->opCode != OP_STLEN_00) {
                sprintf(rs->logs, "op47, OP_STLEN_0 opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op47, OP_STLEN_0 status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDFREE_LEN01];
                pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsda_48(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_48 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SDA", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDFREE_LEN02];
            if (pdt->opCode != OP_STLEN_01) {
                sprintf(rs->logs, "op48, OP_STLEN_1 opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op48, OP_STLEN_1 status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDFREE_LEN02];
                pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsda_49(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_49 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SDA", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDFREE_LEN03];
            if (pdt->opCode != OP_STLEN_02) {
                sprintf(rs->logs, "op49, OP_STLEN_2 opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op49, OP_STLEN_2 status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDFREE_LEN03];
                pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsda_50(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_50 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SDA", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDFREE_LEN04];
            if (pdt->opCode != OP_STLEN_03) {
                sprintf(rs->logs, "op50, OP_STLEN_3 opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op50, OP_STLEN_3 status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDFREE_LEN04];
                pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, FWORD);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsda_51(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    struct sdFAT_s *pFat=0;
    struct info16Bit_s *p=0, *c=0;

    pFat = data->rs->psFat;
    
    rs = data->rs;
    rlt = abs_result(data->result); 
    
    p = &rs->pmch->tmp;
    c = &rs->pmch->cur;

    //sprintf(rs->logs, "op_51 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SDA", rs->logs);  

    switch (rlt) {
        case STINIT:
            /* TODO */
            if (!(pFat->fatStatus & ASPFAT_STATUS_FAT)) {
                data->result = emb_result(data->result, EVTMAX);
            } else {
                ch = 91;
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }

            sprintf(rs->logs, "op_51: result: %x, goto %d, fatStatus: %x\n", data->result, ch, pFat->fatStatus); 
            print_f(rs->plogs, "SDA", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsda_52(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_52 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SDA", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDUSED_USEDSEC];
            if (pdt->opCode != OP_USEDSEC) {
                sprintf(rs->logs, "op52, OP_USEDSEC opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op52, OP_USEDSEC status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDUSED_USEDSEC];
                pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}


static int stsda_53(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_53 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SDA", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDUSED_STR01];
            if (pdt->opCode != OP_STSEC_00) {
                sprintf(rs->logs, "op53, OP_STSEC_0 opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op53, OP_STSEC_0 status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDUSED_STR01];
                pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsda_54(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_54 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SDA", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDUSED_STR02];
            if (pdt->opCode != OP_STSEC_01) {
                sprintf(rs->logs, "op54, OP_STSEC_1 opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op54, OP_STSEC_1 status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDUSED_STR02];
                pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsda_55(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_55 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SDA", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDUSED_STR03];
            if (pdt->opCode != OP_STSEC_02) {
                sprintf(rs->logs, "op55, OP_STSEC_2 opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op55, OP_STSEC_2 status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDUSED_STR03];
                pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsda_56(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_56 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SDA", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDUSED_STR04];
            if (pdt->opCode != OP_STSEC_03) {
                sprintf(rs->logs, "op56, OP_STSEC_3 opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op56, OP_STSEC_3 status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDUSED_STR04];
                pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsda_57(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_57 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SDA", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDUSED_LEN01];
            if (pdt->opCode != OP_STLEN_00) {
                sprintf(rs->logs, "op57, OP_STLEN_0 opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op57, OP_STLEN_0 status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDUSED_LEN01];
                pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsda_58(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_58 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SDA", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDUSED_LEN02];
            if (pdt->opCode != OP_STLEN_01) {
                sprintf(rs->logs, "op58, OP_STLEN_1 opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op58, OP_STLEN_1 status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDUSED_LEN02];
                pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsda_59(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_59 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SDA", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDUSED_LEN03];
            if (pdt->opCode != OP_STLEN_02) {
                sprintf(rs->logs, "op59, OP_STLEN_2 opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op59, OP_STLEN_2 status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDUSED_LEN03];
                pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsda_60(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_60 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SDA", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SDUSED_LEN04];
            if (pdt->opCode != OP_STLEN_03) {
                sprintf(rs->logs, "op60, OP_STLEN_3 opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op60, OP_STLEN_3 status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "SDA", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SDUSED_LEN04];
                pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsda_61(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    struct sdFAT_s *pFat=0;
    struct info16Bit_s *p=0, *c=0;

    pFat = data->rs->psFat;
    
    rs = data->rs;
    rlt = abs_result(data->result); 
    
    p = &rs->pmch->tmp;
    c = &rs->pmch->cur;

    //sprintf(rs->logs, "op_61 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SDA", rs->logs);  

    switch (rlt) {
        case STINIT:
            /* TODO */
            if (!(pFat->fatStatus & ASPFAT_STATUS_FAT)) {
                data->result = emb_result(data->result, EVTMAX);
            } else {
                ch = 92;
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }
            
            //sprintf(rs->logs, "op_61: result: %x, goto %d, fatStatus: %x\n", data->result, ch, pFat->fatStatus); 
            //print_f(rs->plogs, "SDA", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsda_62(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    
    rs = data->rs;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_62 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SDA", rs->logs);  

    switch (rlt) {
        case STINIT:
            ch = 93; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(rs->logs, "op_62: result: %x, goto %d\n", data->result, ch); 
            //print_f(rs->plogs, "SDA", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stwbk_63(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    
    rs = data->rs;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_63 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SDWBK", rs->logs);  

    switch (rlt) {
        case STINIT:
            ch = 98; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(rs->logs, "op_63: result: %x, goto %d\n", data->result, ch); 
            //print_f(rs->plogs, "SDWBK", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsdinit_64(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;


    rs = data->rs;
    rlt = abs_result(data->result); 
    
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    //sprintf(rs->logs, "op_64 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SDINIT", rs->logs);  

    switch (rlt) {
        case STINIT:
            c->opcode = OP_SDINIT;
            c->data = 0;
            memset(p, 0, sizeof(struct info16Bit_s));

            ch = 41; 

            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(rs->logs, "op_64: result: %x, goto %d\n", data->result, ch); 
            //print_f(rs->plogs, "SDINIT", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsdinit_65(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;


    rs = data->rs;
    rlt = abs_result(data->result); 
    
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    //sprintf(rs->logs, "op_65 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SDINIT", rs->logs);  

    switch (rlt) {
        case STINIT:
            c->opcode = OP_SDSTATS;
            c->data = 0;
            memset(p, 0, sizeof(struct info16Bit_s));

            ch = 41; 

            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(rs->logs, "op_65: result: %x, goto %d\n", data->result, ch); 
            //print_f(rs->plogs, "SDINIT", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                if (p->data == SDSTATS_OK) {
                    data->result = emb_result(data->result, NEXT);
                } else {
                    data->result = emb_result(data->result, EVTMAX);
                }
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);

}

static int stwtbak_66(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;
    struct aspConfig_s *pct=0, *pdt=0;

    pct = data->rs->pcfgTable;
    rs = data->rs;
    rlt = abs_result(data->result); 
    
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    //sprintf(rs->logs, "op_66 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "WTBAK", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SCAN_SINGLE];
            if (pdt->opCode != OP_SINGLE) {
                sprintf(rs->logs, "op66, OP_SINGLE opcode is wrong val:%x\n", pdt->opCode); 
                print_f(rs->plogs, "WTBAK", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (!(pdt->opStatus & ASPOP_STA_WR)) {
                sprintf(rs->logs, "op66, OP_SINGLE status is wrong val:%x\n", pdt->opStatus); 
                print_f(rs->plogs, "WTBAK", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                if (pdt->opValue == SINSCAN_WIFI_SD) {
                    data->ansp0 = 1;
                    data->result = emb_result(data->result, NEXT);
                    data->bkofw = emb_fw(data->bkofw, WTBAKQ, PSACT);
                    sprintf(rs->logs, "op_66: SINSCAN_WIFI_SD go to next!!\n"); 
                    print_f(rs->plogs, "WTBAK", rs->logs);  
                } else if (pdt->opValue == SINSCAN_SD_ONLY) {
                    data->ansp0 = 2;
                    data->result = emb_result(data->result, NEXT);
                    data->bkofw = emb_fw(data->bkofw, WTBAKQ, PSACT);
                    sprintf(rs->logs, "op_66: SINSCAN_SD_ONLY go to next!!\n"); 
                    print_f(rs->plogs, "WTBAK", rs->logs);  
                } else if (pdt->opValue == SINSCAN_DUAL_SD) {
                    data->ansp0 = 3;
                    data->result = emb_result(data->result, NEXT);
                    data->bkofw = emb_fw(data->bkofw, WTBAKQ, PSACT);
                    sprintf(rs->logs, "op_66: SINSCAN_DUAL_SD go to next!!\n"); 
                    print_f(rs->plogs, "WTBAK", rs->logs);  
                } else {
                    sprintf(rs->logs, "WARNING!!! op66, opValue is unexpected val:%x\n", pdt->opValue);
                    print_f(rs->plogs, "WTBAK", rs->logs);  
                    data->result = emb_result(data->result, EVTMAX);
                }
            }
        
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stwtbak_67(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;
    struct aspConfig_s *pct=0, *pdt=0;

    pct = data->rs->pcfgTable;
    rs = data->rs;
    rlt = abs_result(data->result); 
    
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    //sprintf(rs->logs, "op_67 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "WTBAK", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SCAN_SINGLE];
            if (pdt->opCode != OP_SINGLE) {
                sprintf(rs->logs, "op67, OP_SINGLE opcode is wrong val:%x\n", pdt->opCode); 
                print_f(rs->plogs, "WTBAK", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (!(pdt->opStatus & ASPOP_STA_WR)) {
                sprintf(rs->logs, "op67, OP_SINGLE status is wrong val:%x\n", pdt->opStatus); 
                print_f(rs->plogs, "WTBAK", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                if (pdt->opValue == SINSCAN_WIFI_SD) {
                    pdt->opStatus = ASPOP_STA_UPD;
                    data->ansp0 = 1;
                    data->result = emb_result(data->result, NEXT);
                    sprintf(rs->logs, "op_67: SINSCAN_WIFI_SD go to next!!\n"); 
                    print_f(rs->plogs, "WTBAK", rs->logs);  
                } else if (pdt->opValue == SINSCAN_SD_ONLY) {
                    pdt->opStatus = ASPOP_STA_UPD;
                    data->ansp0 = 2;
                    data->result = emb_result(data->result, NEXT);
                    sprintf(rs->logs, "op_67: SINSCAN_SD_ONLY go to next!!\n"); 
                    print_f(rs->plogs, "WTBAK", rs->logs);  
                } else if (pdt->opValue == SINSCAN_DUAL_SD) {
                    pdt->opStatus = ASPOP_STA_UPD;
                    data->ansp0 = 3;
                    data->result = emb_result(data->result, NEXT);
                    sprintf(rs->logs, "op_67: SINSCAN_DUAL_SD go to next!!\n"); 
                    print_f(rs->plogs, "WTBAK", rs->logs);  
                } else {
                    sprintf(rs->logs, "WARNING!!! op67, opValue is unexpected val:%x\n", pdt->opValue);
                    print_f(rs->plogs, "WTBAK", rs->logs);  
                    data->result = emb_result(data->result, EVTMAX);
                }
            }
        
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stwtbak_68(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;


    rs = data->rs;
    rlt = abs_result(data->result); 
    
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    //sprintf(rs->logs, "op_68 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "WTBAK", rs->logs);  

    switch (rlt) {
        case STINIT:
            c->opcode = OP_SINGLE;
            c->data = SINSCAN_SD_ONLY;
            memset(p, 0, sizeof(struct info16Bit_s));

            ch = 41; 

            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(rs->logs, "op_68: result: %x, goto %d\n", data->result, ch); 
            //print_f(rs->plogs, "WTBAK", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                if (p->data == c->data) {
                    data->result = emb_result(data->result, NEXT);
                } else {
                    data->result = emb_result(data->result, EVTMAX);    
                }
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stwtbak_69(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;

    rs = data->rs;
    rlt = abs_result(data->result); 
    
    //sprintf(rs->logs, "op_69 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "WTBAK", rs->logs);  

    switch (rlt) {
        case STINIT:
            ch = 99; 

            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(rs->logs, "op_69: result: %x, goto %d\n", data->result, ch); 
            //print_f(rs->plogs, "WTBAK", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stwtbak_70(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    struct aspConfig_s *pct=0, *pdt=0;

    pct = data->rs->pcfgTable;
    rs = data->rs;
    rlt = abs_result(data->result); 
    
    //sprintf(rs->logs, "op_70 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "WTBAK", rs->logs);  

    switch (rlt) {
        case STINIT:
            ch = 48; 

            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(rs->logs, "op_70: result: %x, goto %d\n", data->result, ch); 
            //print_f(rs->plogs, "WTBAK", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_EG_DECT];
                if ((pdt->opStatus == ASPOP_STA_UPD) && (pdt->opValue == 1)) {
#if CROP_USE_META
                    data->bkofw = emb_bk(data->bkofw, SAVPARM, PSRLT);
#else
                    data->bkofw = emb_bk(data->bkofw, WTBAKQ, PSTSM);
#endif
                    data->result = emb_result(data->result, BKWRD);
                } else {
                    data->result = emb_result(data->result, FWORD);
                }
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stwtbak_71(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;
    struct aspConfig_s *pct=0, *pdt=0;

    pct = data->rs->pcfgTable;
    rs = data->rs;
    rlt = abs_result(data->result); 
    
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    //sprintf(rs->logs, "op_71 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "WTBAK", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SUP_SAVE];
            if (pdt->opCode != OP_SUPBACK) {
                sprintf(rs->logs, "op_71, ASPOP_SUP_SAVE opcode is wrong val:%x\n", pdt->opCode); 
                print_f(rs->plogs, "WTBAK", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (!(pdt->opStatus & ASPOP_STA_CON)) {
                sprintf(rs->logs, "op_71, ASPOP_SUP_SAVE status is wrong val:%x\n", pdt->opStatus); 
                print_f(rs->plogs, "WTBAK", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                if (pdt->opValue == SUPBACK_SD) {
                    data->ansp0 = 1;
                    data->result = emb_result(data->result, NEXT);
                    data->bkofw = emb_fw(data->bkofw, WTBAKQ, PSWT);
                    sprintf(rs->logs, "op_71: SUPBACK_SD go to next!!\n"); 
                    print_f(rs->plogs, "WTBAK", rs->logs);  
                } else if (pdt->opValue == SUPBACK_RAW) {
                    data->ansp0 = 2;
                    data->result = emb_result(data->result, NEXT);
                    sprintf(rs->logs, "op_71: SUPBACK_RAW go to next!!\n"); 
                    print_f(rs->plogs, "WTBAK", rs->logs);  
                } else if (pdt->opValue == SUPBACK_FAT) {
                    data->ansp0 = 3;
                    data->result = emb_result(data->result, NEXT);
                    data->bkofw = emb_fw(data->bkofw, WTBAKP, PSSET);
                    sprintf(rs->logs, "op_71: SUPBACK_FAT go to next!!\n"); 
                    print_f(rs->plogs, "WTBAK", rs->logs);  
                } else {
                    sprintf(rs->logs, "WARNING!!! op_71, opValue is unexpected val:%x\n", pdt->opValue);
                    print_f(rs->plogs, "WTBAK", rs->logs);  
                    data->result = emb_result(data->result, EVTMAX);
                }
            }
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stwtbak_72(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;
    struct aspConfig_s *pct=0, *pdt=0;

    pct = data->rs->pcfgTable;
    rs = data->rs;
    rlt = abs_result(data->result); 
    
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    //sprintf(rs->logs, "op_72 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "WTBAK", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SUP_SAVE];
            if (pdt->opCode != OP_SUPBACK) {
                sprintf(rs->logs, "op_72, OP_SINGLE opcode is wrong val:%x\n", pdt->opCode); 
                print_f(rs->plogs, "WTBAK", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (!(pdt->opStatus & ASPOP_STA_CON)) {
                sprintf(rs->logs, "op_72, OP_SINGLE status is wrong val:%x\n", pdt->opStatus); 
                print_f(rs->plogs, "WTBAK", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                if (pdt->opValue == SUPBACK_SD) {
                    data->ansp0 = 1;
                    data->result = emb_result(data->result, NEXT);
                    data->bkofw = emb_fw(data->bkofw, WTBAKQ, PSRLT);
                    sprintf(rs->logs, "op_72: SINSCAN_WIFI_SD go to next!!\n"); 
                    print_f(rs->plogs, "WTBAK", rs->logs);  
                } else if (pdt->opValue == SUPBACK_RAW) {
                    data->ansp0 = 2;
                    data->result = emb_result(data->result, NEXT);
                    sprintf(rs->logs, "op_72: SINSCAN_SD_ONLY go to next!!\n"); 
                    print_f(rs->plogs, "WTBAK", rs->logs);  
                } else if (pdt->opValue == SUPBACK_FAT) {
                    data->ansp0 = 3;
                    data->result = emb_result(data->result, NEXT);
                    data->bkofw = emb_fw(data->bkofw, WTBAKP, PSACT);
                    sprintf(rs->logs, "op_72: SINSCAN_DUAL_SD go to next!!\n"); 
                    print_f(rs->plogs, "WTBAK", rs->logs);  
                } else {
                    sprintf(rs->logs, "WARNING!!! op_72, opValue is unexpected val:%x\n", pdt->opValue);
                    print_f(rs->plogs, "WTBAK", rs->logs);  
                    data->result = emb_result(data->result, EVTMAX);
                }
            }
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stwtbak_73(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;
    struct aspConfig_s *pct=0, *pdt=0;

    pct = data->rs->pcfgTable;
    rs = data->rs;
    rlt = abs_result(data->result); 
    
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    //sprintf(rs->logs, "op_73 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "WTBAK", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SCAN_SINGLE];
            if (pdt->opCode != OP_SINGLE) {
                sprintf(rs->logs, "op_73, OP_SINGLE opcode is wrong val:%x\n", pdt->opCode); 
                print_f(rs->plogs, "WTBAK", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (!(pdt->opStatus & ASPOP_STA_WR)) {
                sprintf(rs->logs, "op_73, OP_SINGLE status is wrong val:%x\n", pdt->opStatus); 
                print_f(rs->plogs, "WTBAK", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                if (pdt->opValue == SINSCAN_WIFI_SD) {

                    ch = 59; 
                    rs_ipc_put(data->rs, &ch, 1);
                    data->result = emb_result(data->result, WAIT);

                    sprintf(rs->logs, "op_73: SINSCAN_WIFI_SD go to next!!\n"); 
                    print_f(rs->plogs, "WTBAK", rs->logs);  
                } else if (pdt->opValue == SINSCAN_SD_ONLY) {
                
                    ch = 59; 
                    rs_ipc_put(data->rs, &ch, 1);
                    data->result = emb_result(data->result, WAIT);

                    sprintf(rs->logs, "op_73: SINSCAN_SD_ONLY go to next!!\n"); 
                    print_f(rs->plogs, "WTBAK", rs->logs);  
                } else if (pdt->opValue == SINSCAN_DUAL_SD) {

                    ch = 59; 
                    rs_ipc_put(data->rs, &ch, 1);
                    data->result = emb_result(data->result, WAIT);

                    sprintf(rs->logs, "op_73: SINSCAN_DUAL_SD go to next!!\n"); 
                    print_f(rs->plogs, "WTBAK", rs->logs);  
                } else {
                    sprintf(rs->logs, "WARNING!!! op_73, opValue is unexpected val:%x\n", pdt->opValue);
                    print_f(rs->plogs, "WTBAK", rs->logs);  
                    data->result = emb_result(data->result, EVTMAX);
                }
            }
        
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_SCAN_SINGLE];
                data->ansp0 = pdt->opValue;
                data->result = emb_result(data->result, NEXT);
                data->bkofw = emb_fw(data->bkofw, WTBAKQ, PSACT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stwtbak_74(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;
    struct aspConfig_s *pct=0, *pdt=0;

    pct = data->rs->pcfgTable;
    rs = data->rs;
    rlt = abs_result(data->result); 
    
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    //sprintf(rs->logs, "op_74 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "WTBAK", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SCAN_SINGLE];
            if (pdt->opCode != OP_SINGLE) {
                sprintf(rs->logs, "op_74, OP_SINGLE opcode is wrong val:%x\n", pdt->opCode); 
                print_f(rs->plogs, "WTBAK", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (!(pdt->opStatus & ASPOP_STA_WR)) {
                sprintf(rs->logs, "op_74, OP_SINGLE status is wrong val:%x\n", pdt->opStatus); 
                print_f(rs->plogs, "WTBAK", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                if (pdt->opValue == SINSCAN_WIFI_SD) {
                    pdt->opStatus = ASPOP_STA_UPD;
                    data->ansp0 = 1;
                    data->result = emb_result(data->result, NEXT);
                    sprintf(rs->logs, "op_74: SINSCAN_WIFI_SD go to next!!\n"); 
                    print_f(rs->plogs, "WTBAK", rs->logs);  
                } else if (pdt->opValue == SINSCAN_SD_ONLY) {
                    pdt->opStatus = ASPOP_STA_UPD;
                    data->ansp0 = 2;
                    data->result = emb_result(data->result, NEXT);
                    sprintf(rs->logs, "op_74: SINSCAN_SD_ONLY go to next!!\n"); 
                    print_f(rs->plogs, "WTBAK", rs->logs);  
                } else if (pdt->opValue == SINSCAN_DUAL_SD) {
                    pdt->opStatus = ASPOP_STA_UPD;
                    data->ansp0 = 3;
                    data->result = emb_result(data->result, NEXT);
                    sprintf(rs->logs, "op_74: SINSCAN_DUAL_SD go to next!!\n"); 
                    print_f(rs->plogs, "WTBAK", rs->logs);  
                } else {
                    sprintf(rs->logs, "WARNING!!! op_74, opValue is unexpected val:%x\n", pdt->opValue);
                    print_f(rs->plogs, "WTBAK", rs->logs);  
                    data->result = emb_result(data->result, EVTMAX);
                }
            }
        
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stcrop_75(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;
    struct aspConfig_s *pct=0, *pdt=0;

    pct = data->rs->pcfgTable;
    rs = data->rs;
    rlt = abs_result(data->result); 
    
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    //sprintf(rs->logs, "op_75 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "CROP", rs->logs);  

    switch (rlt) {
        case STINIT:
            
            pct[ASPOP_CROP_01].opValue = 0;
            pct[ASPOP_CROP_01].opStatus = ASPOP_STA_APP;

            pct[ASPOP_CROP_02].opValue = 0;
            pct[ASPOP_CROP_02].opStatus = ASPOP_STA_APP;

            pct[ASPOP_CROP_03].opValue = 0;
            pct[ASPOP_CROP_03].opStatus = ASPOP_STA_APP;

            pct[ASPOP_CROP_04].opValue = 0;
            pct[ASPOP_CROP_04].opStatus = ASPOP_STA_APP;

            pct[ASPOP_CROP_05].opValue = 0;
            pct[ASPOP_CROP_05].opStatus = ASPOP_STA_APP;

            pct[ASPOP_CROP_06].opValue = 0;
            pct[ASPOP_CROP_06].opStatus = ASPOP_STA_APP;

            pct[ASPOP_IMG_LEN].opValue = 0;
            pct[ASPOP_IMG_LEN].opStatus = ASPOP_STA_APP;

            sprintf(rs->logs, "op_75, reset CROP value"); 
            print_f(rs->plogs, "CROP", rs->logs);  
            data->result = emb_result(data->result, NEXT);
        
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stcrop_76(struct psdata_s *data)
{ 
    uint8_t uch=0;
    int xyAr[6];
    int id=0, err=0;
    uint32_t coord[2];
    char ch = 0; 
    uint32_t rlt;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;
    struct aspConfig_s *pct=0, *pdt=0;

    pct = data->rs->pcfgTable;
    rs = data->rs;
    rlt = abs_result(data->result); 
    
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    //sprintf(rs->logs, "op_76 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "CROP", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = 0;
            for (id=0; id < 7; id++) {
                if ((!pdt) && (pct[ASPOP_CROP_01 + id].opStatus == ASPOP_STA_WR)) {
                    pdt = &pct[ASPOP_CROP_01 + id];
                }

                xyAr[id] = -1;
            }
            if (pdt) {
                for (id=0; id < 4; id++) {            
                    if (pct[ASPOP_CROP_COOR_XH + id].opStatus == ASPOP_STA_UPD) {
                        xyAr[id] = pct[ASPOP_CROP_COOR_XH + id].opValue;
                    }
                }

                pdt->opValue = 0;
                err = 0;
                for (id=0; id < 4; id++) {
                    if (xyAr[id] < 0) {
                        sprintf(rs->logs, "ERROR!! wrong xy value - %d\n", xyAr[id]); 
                        print_f(rs->plogs, "CROP", rs->logs);  
                        err++;
                    } else {
                        uch = xyAr[id] & 0xff;
                        pdt->opValue |= uch << (8 * id); // low byte first or high byte first
                    }
                }

                if (!err) {
                    pdt->opStatus = ASPOP_STA_UPD;
                    //sprintf(rs->logs, "DONE!! crop value - 0x%.8x\n", pdt->opValue); 
                    //print_f(rs->plogs, "CROP", rs->logs);  
                } else {
                    pdt->opStatus = ASPOP_STA_APP;
                }
            }
            
            pdt = 0;
            for (id=0; id < 7; id++) {
                if ((!pdt) && (pct[ASPOP_CROP_01 + id].opStatus == ASPOP_STA_APP)) {
                    pdt = &pct[ASPOP_CROP_01 + id];
                }
            }

            if (!pdt) {
                for (id=0; id < 7; id++) {
                    if (id == 6) {
                        pdt = &pct[ASPOP_CROP_01 + id];
                        sprintf(rs->logs, "%d. %x (%d) [0x%.8x]\n", id, pdt->opStatus, pdt->opValue, pdt->opValue); 
                        print_f(rs->plogs, "CROP", rs->logs);  
                    } else {
                        pdt = &pct[ASPOP_CROP_01 + id];
                        coord[0] = pdt->opValue >> 16;
                        coord[1] = pdt->opValue & 0xffff;
                        sprintf(rs->logs, "%d. %x (%.4d, %.4d) / 2 = (%.4d, %.4d)\n", id, pdt->opStatus, coord[0], coord[1], (coord[0] / 2), (coord[1] / 2));
                        print_f(rs->plogs, "CROP", rs->logs);  
                    }
                }

                sprintf(rs->logs, "op_76, update CROP coordinates DONE!!\n"); 
                print_f(rs->plogs, "CROP", rs->logs);  
                data->result = emb_result(data->result, FWORD);
                //data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 

                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
                //sprintf(rs->logs, "op_76, send 0x%.2x, 0x%.2x\n", c->opcode, c->data); 
                //print_f(rs->plogs, "CROP", rs->logs);  
            }
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = 0;
                for (id=0; id < 7; id++) {
                    if ((!pdt) && (pct[ASPOP_CROP_01 + id].opStatus == ASPOP_STA_APP)) {
                        pdt = &pct[ASPOP_CROP_01 + id];
                    }

                    if (id < 4) {
                        pct[ASPOP_CROP_COOR_XH + id].opStatus = ASPOP_STA_WR;
                        pct[ASPOP_CROP_COOR_XH + id].opValue = 0;
                    }
                }

                //pdt->opValue = p->data;
                if (pdt) {
                    pdt->opStatus = ASPOP_STA_WR;
                    data->result = emb_result(data->result, NEXT);
                } else {
                    data->result = emb_result(data->result, FWORD);
                    //data->result = emb_result(data->result, EVTMAX);                
                    sprintf(rs->logs, "op_76, can't find target table \n"); 
                    print_f(rs->plogs, "CROP", rs->logs);  
                }

            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stcrop_77(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_77 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "CROP", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_CROP_COOR_XH];
            if (pdt->opCode != OP_STLEN_00) {
                sprintf(rs->logs, "op77, ASPOP_CROP_COOR_XH opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "CROP", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op77, ASPOP_CROP_COOR_XH status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "CROP", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_CROP_COOR_XH];
                pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stcrop_78(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_78 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "CROP", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_CROP_COOR_XL];
            if (pdt->opCode != OP_STLEN_01) {
                sprintf(rs->logs, "op78, ASPOP_CROP_COOR_XL opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "CROP", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op78, ASPOP_CROP_COOR_XL status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "CROP", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_CROP_COOR_XL];
                pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stcrop_79(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_79 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "CROP", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_CROP_COOR_YH];
            if (pdt->opCode != OP_STLEN_02) {
                sprintf(rs->logs, "op79, ASPOP_CROP_COOR_YH opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "CROP", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op79, ASPOP_CROP_COOR_YH status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "CROP", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_CROP_COOR_YH];
                pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stcrop_80(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_80 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "CROP", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_CROP_COOR_YL];
            if (pdt->opCode != OP_STLEN_03) {
                sprintf(rs->logs, "op80, ASPOP_CROP_COOR_YL opcode is wrong op:%x\n", pdt->opCode); 
                print_f(rs->plogs, "CROP", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (pdt->opStatus != ASPOP_STA_WR) {
                sprintf(rs->logs, "op80, ASPOP_CROP_COOR_YL status is wrong, %x\n", pdt->opStatus); 
                print_f(rs->plogs, "CROP", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                c->opcode = pdt->opCode;
                c->data = pdt->opValue;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            }            
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_CROP_COOR_YL];
                pdt->opStatus = ASPOP_STA_UPD;
                pdt->opValue = p->data;
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stvector_81(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;

    rs = data->rs;
    rlt = abs_result(data->result); 
    
    sprintf(rs->logs, "op_81 rlt:0x%x \n", rlt); 
    print_f(rs->plogs, "VECTOR", rs->logs);  

    switch (rlt) {
        case STINIT:

            ch = 105; 

            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(rs->logs, "op_81: result: %x, goto %d\n", data->result, ch); 
            print_f(rs->plogs, "VECTOR", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stvector_82(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct info16Bit_s *p=0, *c=0;
    struct procRes_s *rs;
    struct aspConfig_s *pct=0, *pdt=0;

    pct = data->rs->pcfgTable;
    rs = data->rs;
    rlt = abs_result(data->result); 
    
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    sprintf(rs->logs, "op_82 rlt:0x%x \n", rlt); 
    print_f(rs->plogs, "VECTOR", rs->logs);  

    switch (rlt) {
        case STINIT:
            pct[ASPOP_IMG_LEN].opValue = 0;
            pct[ASPOP_IMG_LEN].opStatus = ASPOP_STA_APP;

            sprintf(rs->logs, "op_82, reset value"); 
            print_f(rs->plogs, "VECTOR", rs->logs);  
            data->result = emb_result(data->result, NEXT);
        
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stapm_83(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    struct aspConfig_s *pct=0, *pdt=0;

    pct = data->rs->pcfgTable;
    rs = data->rs;
    rlt = abs_result(data->result); 
    
    sprintf(rs->logs, "op_83 rlt:0x%x \n", rlt); 
    print_f(rs->plogs, "APM", rs->logs);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_AP_MODE];
            if (pdt->opCode != OP_AP_MODEN) {
                sprintf(rs->logs, "op83, OP_AP_MODEN opcode is wrong val:%x\n", pdt->opCode); 
                print_f(rs->plogs, "APM", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (!(pdt->opStatus & ASPOP_STA_APP)) {
                sprintf(rs->logs, "op83, OP_AP_MODEN status is wrong val:%x\n", pdt->opStatus); 
                print_f(rs->plogs, "APM", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else {
                switch(pdt->opValue) {
                    case APM_DIRECT:

                        ch = 106; 
                        
                        rs_ipc_put(data->rs, &ch, 1);
                        data->result = emb_result(data->result, WAIT);
                        sprintf(rs->logs, "op_83: result: %x, goto %d\n", data->result, ch); 
                        print_f(rs->plogs, "APM", rs->logs);  
                        
                        data->bkofw = emb_fw(data->bkofw, VECTORS, PSRLT);
                        
                        //pdt->opValue = APM_AP; /* for debug */
                        break;
                    case APM_AP:

                        ch = 106; 
                        
                        rs_ipc_put(data->rs, &ch, 1);
                        data->result = emb_result(data->result, WAIT);
                        sprintf(rs->logs, "op_83: result: %x, goto %d\n", data->result, ch); 
                        print_f(rs->plogs, "APM", rs->logs);  

                        data->bkofw = emb_fw(data->bkofw, VECTORS, PSTSM);
                        
                        //pdt->opValue = APM_DIRECT; /* for debug */
                        break;
                    default:
                        sprintf(rs->logs, "WARNING!!! op83, opValue is unexpected val:%x\n", pdt->opValue);
                        print_f(rs->plogs, "APM", rs->logs);  
                        data->result = emb_result(data->result, EVTMAX);
                        break;
                }
            }        
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, FWORD);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stapm_84(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;

    rs = data->rs;
    rlt = abs_result(data->result); 
    
    sprintf(rs->logs, "op_84 rlt:0x%x \n", rlt); 
    print_f(rs->plogs, "APM", rs->logs);  

    switch (rlt) {
        case STINIT:

            ch = 107; 

            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(rs->logs, "op_84: result: %x, goto %d\n", data->result, ch); 
            print_f(rs->plogs, "APM", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stapm_85(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;

    rs = data->rs;
    rlt = abs_result(data->result); 
    
    sprintf(rs->logs, "op_85 rlt:0x%x \n", rlt); 
    print_f(rs->plogs, "APM", rs->logs);  

    switch (rlt) {
        case STINIT:

            ch = 108; 

            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(rs->logs, "op_85: result: %x, goto %d\n", data->result, ch); 
            print_f(rs->plogs, "APM", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsparam_86(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;

    rs = data->rs;
    rlt = abs_result(data->result); 
    
    sprintf(rs->logs, "op_86 rlt:0x%x \n", rlt); 
    print_f(rs->plogs, "SPM", rs->logs);  

    switch (rlt) {
        case STINIT:

            ch = 109; 

            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(rs->logs, "op_86: result: %x, goto %d\n", data->result, ch); 
            print_f(rs->plogs, "SPM", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stsparam_87(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct info16Bit_s *p=0, *c=0, *t=0;
    struct procRes_s *rs;


    rs = data->rs;
    rlt = abs_result(data->result); 
    
    p = &rs->pmch->get;
    c = &rs->pmch->cur;
    t = &rs->pmch->tmp;
    
    //sprintf(rs->logs, "op_87 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "META", rs->logs);  

    switch (rlt) {
        case STINIT:
            if (t->opcode == OP_META_DAT) {
                c->opcode = OP_META_DAT;
                c->data = t->data;
                memset(p, 0, sizeof(struct info16Bit_s));

                ch = 41; 

                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
                sprintf(rs->logs, "op_87: result: %x, goto %d\n", data->result, ch); 
                print_f(rs->plogs, "META", rs->logs);  
            } else {
                sprintf(rs->logs, "op_87 error!! tmp opcode: %x break!!\n", t->opcode); 
                print_f(rs->plogs, "META", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);                
            }
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                if (p->data == c->data) {
                    data->result = emb_result(data->result, NEXT);
                } else {
                    data->result = emb_result(data->result, EVTMAX);    
                }
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

#define SAVE_CROP_MASS (1)
static int stsparam_88(struct psdata_s *data)
{ 
    int act=0;
    char ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    struct aspMetaData *pmetaIn, *pmetaOut;
    struct aspMetaMass *pmass;
#if SAVE_CROP_MASS
    int ret=0;
    struct aspConfig_s *pct=0, *pdt=0;

    FILE *f=0;
    char supPath[128] = "/mnt/mmc2/crop/g%d_s%d_c%d_%.4d%.2d%.2d-%.2d%.2d%.2d.bin";
    char tail[32] = "_%d.bin\0";
    char supPathCp1[512];
    char supDst[128];
    int slen=0, dlen=0;

    pct = data->rs->pcfgTable;
#endif
    rs = data->rs;
    rlt = abs_result(data->result); 
    pmetaIn = rs->pmetain;
    pmetaOut= rs->pmetaout;
    pmass = rs->pmetaMass;
    //sprintf(rs->logs, "op_88 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SPM", rs->logs);  

    switch (rlt) {
        case STINIT:
            aspMetaClear(0, rs, ASPMETA_INPUT);
            
            sprintf(rs->logs, "dump meta output\n"); 
            print_f(rs->plogs, "SPM", rs->logs);  
            //shmem_dump((char *)rs->pmetaout, sizeof(struct aspMetaData));            
            dbgMeta(msb2lsb(&pmetaOut->FUNC_BITS), pmetaOut);
            
            ch = 110; 

            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(rs->logs, "op_88: result: %x, goto %d\n", data->result, ch); 
            print_f(rs->plogs, "SPM", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                sprintf(rs->logs, "dump meta input (used:%d) \n", pmass->massUsed); 
                print_f(rs->plogs, "SPM", rs->logs);  

                if (pmass->massRecd > 0) {
                    sprintf(rs->logs, "dump meta mass: (gap:%d, linStart:%d, linRecd:%d)\n", pmass->massGap, pmass->massStart, pmass->massRecd); 
                    print_f(rs->plogs, "SPM", rs->logs);  
#if SAVE_CROP_MASS
                    char syscmd[128] = "mkdir -p /mnt/mmc2/crop";
                    char *wday[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"}; 
                    struct tm *p; 
                    time_t timep;
    
                    time(&timep);
                    p=localtime(&timep);
                    //sprintf(rs->logs, "%.4d%.2d%.2d \n", (1900+p->tm_year),( 1+p-> tm_mon), p->tm_mday); 
                    //print_f(rs->plogs, "SPM", rs->logs);
                    //sprintf(rs->logs, "%s,%.2d:%.2d:%.2d\n", wday[p->tm_wday],p->tm_hour, p->tm_min, p->tm_sec); 
                    //print_f(rs->plogs, "SPM", rs->logs);       

                    pdt = &pct[ASPOP_CROP_01];
                    sprintf(supPathCp1, supPath, pmass->massGap, pmass->massStart, pmass->massRecd, (1900+p->tm_year),( 1+p-> tm_mon), p->tm_mday,  p->tm_hour, p->tm_min, p->tm_sec);
/*
                    dlen = strlen(supPathCp1) - 1;
                    slen = strlen(tail);
                    memcpy(&supPathCp1[dlen], tail, slen);
*/
                    //supPathCp1[dlen+slen] = '\0';

                    sprintf(rs->logs, "plan to save crop mass to [%s] size: %d\n", supPathCp1, pmass->massUsed);
                    print_f(rs->plogs, "SPM", rs->logs);  
                    
                    f = find_save(supDst, supPathCp1);
                    if (f) {
                        fwrite((char *)pmass->masspt, 1, pmass->massUsed, f);
                        sprintf(rs->logs, "save crop mass to [%s] size: %d\n", supDst, pmass->massUsed);
                        print_f(rs->plogs, "SPM", rs->logs);  

                        fflush(f);
                        fclose(f);

                        sync();
                    } else {
                        ret = doSystemCmd(syscmd);

                        f = find_save(supDst, supPathCp1);
                        if (f) {
                            fwrite((char *)pmass->masspt, 1, pmass->massUsed, f);
                            sprintf(rs->logs, "save crop mass to [%s] size: %d\n", supDst, pmass->massUsed);
                            print_f(rs->plogs, "SPM", rs->logs);  

                            fflush(f);
                            fclose(f);

                            sync();
                        } else {
                            sprintf(rs->logs, "Error!!! failed to save crop mass to [%s] size: %d\n", supPathCp1, pmass->massUsed);
                            print_f(rs->plogs, "SPM", rs->logs);  
                        }
                    }
                    
#endif
                    //mem_dump((char *)pmass->masspt, pmass->massUsed);
                    

                    //data->result = emb_result(data->result, FWORD);                
                    data->result = emb_result(data->result, NEXT);                
                    //pmass->massRecd = 0;
                    //pmass->massUsed = 0;
                } else {
                    //shmem_dump((char *)rs->pmetain, sizeof(struct aspMetaData));
                    dbgMeta(msb2lsb(&pmetaIn->FUNC_BITS), pmetaIn);
                    act = aspMetaRelease(msb2lsb(&pmetaIn->FUNC_BITS), 0, rs);

                    if ((act > 0) && (act & ASPMETA_FUNC_CROP)) {
                        data->bkofw = emb_bk(data->bkofw, METAT, PSSET);
                        data->result = emb_result(data->result, BKWRD);

                        sprintf(rs->logs, "act:0x%x, metamass gap:%d, start:%d, record:%d\n", act, pmass->massGap, pmass->massStart, pmass->massRecd); 
                        print_f(rs->plogs, "SPM", rs->logs);  
                    } else {
                        //data->result = emb_result(data->result, NEXT);
                        data->result = emb_result(data->result, FWORD);
                    }
                }

            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stcropmeta_89(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    struct aspMetaData * pmeta;
    struct info16Bit_s *p=0, *c=0, *t=0;

    rs = data->rs;
    rlt = abs_result(data->result); 
    pmeta = rs->pmetaout;
    t = &rs->pmch->tmp;
    sprintf(rs->logs, "op_89 rlt:0x%x \n", rlt); 
    print_f(rs->plogs, "CPM", rs->logs);  

    switch (rlt) {
        case STINIT:
            t->opcode = OP_META_DAT;
            t->data = ASPMETA_SCAN_COMPLETE;
            aspMetaClear(0, rs, ASPMETA_OUTPUT);
            //aspMetaBuild(ASPMETA_FUNC_CONF, 0, rs);
            //dbgMeta(pmeta->FUNC_BITS, pmeta);

            data->result = emb_result(data->result, NEXT);
            sprintf(rs->logs, "op_89: result: %x, goto %d\n", data->result, ch); 
            print_f(rs->plogs, "CPM", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stcropmeta_90(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    struct aspMetaData * pmeta;
    struct info16Bit_s *p=0, *c=0, *t=0;

    rs = data->rs;
    rlt = abs_result(data->result); 
    pmeta = rs->pmetaout;
    t = &rs->pmch->tmp;
    sprintf(rs->logs, "op_90 rlt:0x%x \n", rlt); 
    print_f(rs->plogs, "CPM", rs->logs);  

    switch (rlt) {
        case STINIT:
            t->opcode = OP_META_DAT;
            t->data = ASPMETA_POWON_INIT;
            aspMetaClear(0, rs, ASPMETA_OUTPUT);
            aspMetaBuild(ASPMETA_FUNC_CONF, 0, rs);
            dbgMeta(msb2lsb(&pmeta->FUNC_BITS), pmeta);

            data->result = emb_result(data->result, NEXT);
            sprintf(rs->logs, "op_90: result: %x, goto %d\n", data->result, ch); 
            print_f(rs->plogs, "CPM", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stcropmeta_91(struct psdata_s *data)
{ 
    int ret=0;
    char ch = 0; 
    uint32_t rlt=0;
    uint32_t rval=0;
    struct procRes_s *rs;
    struct aspMetaData * pmeta;
    struct info16Bit_s *p=0, *c=0, *t=0;
    struct aspConfig_s *pct=0, *pdt=0;

    pct = data->rs->pcfgTable;
    rs = data->rs;
    rlt = abs_result(data->result); 
    pmeta = rs->pmetaout;
    t = &rs->pmch->tmp;
    sprintf(rs->logs, "op_91 rlt:0x%x \n", rlt); 
    print_f(rs->plogs, "CPM", rs->logs);  

    switch (rlt) {
        case STINIT:
            t->opcode = OP_META_DAT;
            ret = cfgTableGet(pct, ASPOP_RESOLUTION, &rval);
            if (ret < 0) {
                data->result = emb_result(data->result, EVTMAX);
                sprintf(rs->logs, "Error!!! op_91 get ASPOP_RESOLUTION failed!!!ret = %d \n", ret); 
                print_f(rs->plogs, "CPM", rs->logs);  
            } else {
                switch (rval) {
                case RESOLUTION_1200:
                case RESOLUTION_600:
                    t->data = ASPMETA_CROP_600DPI;
                    data->result = emb_result(data->result, NEXT);
                    sprintf(rs->logs, "op_91: result: %x, goto %d\n", data->result, ch); 
                    print_f(rs->plogs, "CPM", rs->logs);  
                    rs->pmetaMass->massUsed = 0;
                    break;
                case RESOLUTION_300:
                case RESOLUTION_200:
                case RESOLUTION_150:
                    t->data = ASPMETA_CROP_300DPI;
                    data->result = emb_result(data->result, NEXT);
                    sprintf(rs->logs, "op_91: result: %x, goto %d\n", data->result, ch); 
                    print_f(rs->plogs, "CPM", rs->logs);  
                    rs->pmetaMass->massUsed = 0;
                    break;
                default:
                    data->result = emb_result(data->result, EVTMAX);
                    sprintf(rs->logs, "Error!!! op_91 get ASPOP_RESOLUTION failed!!!rval = %d \n", rval); 
                    print_f(rs->plogs, "CPM", rs->logs);  
                    break;
                }
            }
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stcropmeta_92(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    struct aspMetaData * pmeta;
    struct info16Bit_s *p=0, *c=0, *t=0;

    rs = data->rs;
    rlt = abs_result(data->result); 
    pmeta = rs->pmetaout;
    t = &rs->pmch->tmp;
    //sprintf(rs->logs, "op_92 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "CPM", rs->logs);  

    switch (rlt) {
        case STINIT:
            ch = 112;

            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(rs->logs, "op_92: result: %x, goto %d\n", data->result, ch); 
            print_f(rs->plogs, "CPM", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, FWORD);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stcropmeta_93(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    struct aspMetaData * pmeta;
    struct info16Bit_s *p=0, *c=0, *t=0;

    rs = data->rs;
    rlt = abs_result(data->result); 
    pmeta = rs->pmetaout;
    t = &rs->pmch->tmp;
    sprintf(rs->logs, "op_93 rlt:0x%x \n", rlt); 
    print_f(rs->plogs, "CPM", rs->logs);  

    switch (rlt) {
        case STINIT:
            t->opcode = OP_META_DAT;
            t->data = ASPMETA_CROP_300DPI;
            aspMetaClear(0, rs, ASPMETA_OUTPUT);
            aspMetaBuild(ASPMETA_FUNC_CONF, 0, rs);
            dbgMeta(msb2lsb(&pmeta->FUNC_BITS), pmeta);

            data->result = emb_result(data->result, NEXT);
            sprintf(rs->logs, "op_93: result: %x, goto %d\n", data->result, ch); 
            print_f(rs->plogs, "CPM", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stcropmeta_94(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    struct aspMetaData * pmeta;
    struct info16Bit_s *p=0, *c=0, *t=0;

    rs = data->rs;
    rlt = abs_result(data->result); 
    pmeta = rs->pmetaout;
    t = &rs->pmch->tmp;
    sprintf(rs->logs, "op_94 rlt:0x%x \n", rlt); 
    print_f(rs->plogs, "CPM", rs->logs);  

    switch (rlt) {
        case STINIT:
            t->opcode = OP_META_DAT;
            t->data = ASPMETA_CROP_300DPI;
            aspMetaClear(0, rs, ASPMETA_OUTPUT);
            aspMetaBuild(ASPMETA_FUNC_CONF, 0, rs);
            dbgMeta(msb2lsb(&pmeta->FUNC_BITS), pmeta);

            data->result = emb_result(data->result, NEXT);
            sprintf(rs->logs, "op_94: result: %x, goto %d\n", data->result, ch); 
            print_f(rs->plogs, "CPM", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stcropmeta_95(struct psdata_s *data)
{ 
    char ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    struct aspMetaData * pmeta;
    struct info16Bit_s *p=0, *c=0, *t=0;

    rs = data->rs;
    rlt = abs_result(data->result); 
    pmeta = rs->pmetaout;
    t = &rs->pmch->tmp;
    sprintf(rs->logs, "op_95 rlt:0x%x \n", rlt); 
    print_f(rs->plogs, "CPM", rs->logs);  

    switch (rlt) {
        case STINIT:
            t->opcode = OP_META_DAT;
            t->data = ASPMETA_CROP_300DPI;
            aspMetaClear(0, rs, ASPMETA_OUTPUT);
            aspMetaBuild(ASPMETA_FUNC_CONF, 0, rs);
            dbgMeta(msb2lsb(&pmeta->FUNC_BITS), pmeta);

            data->result = emb_result(data->result, NEXT);
            sprintf(rs->logs, "op_95: result: %x, goto %d\n", data->result, ch); 
            print_f(rs->plogs, "CPM", rs->logs);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stspy_01(struct psdata_s *data)
{ 
    // keep polling, kind of idle mode
    // jump to next status if receive any op code

    char str[128], ch = 0; 
    uint32_t rlt;
    rlt = abs_result(data->result); 

    //sprintf(str, "op_01 - rlt:0x%x \n", rlt); 
    //print_f(mlogPool, "spy", str); 

    switch (rlt) {
        case STINIT:
            ch = 1; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(str, "op_01: result: %x\n", data->result); 
            //print_f(mlogPool, "spy", str);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, EVTMAX);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}
static int stspy_02(struct psdata_s *data) 
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct info16Bit_s *p;
    struct procRes_s *rs;
    rs = data->rs;
    p = &rs->pmch->get;

    rlt = abs_result(data->result); 

    //sprintf(str, "op_02 - rlt:0x%.8x \n", rlt); 
    //print_f(mlogPool, "spy", str); 

    switch (rlt) {
        case STINIT:
            ch = 5; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //printf(str, "op_02: result: %x\n", data->result); 
            //print_f(mlogPool, "spy", str);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                sprintf(str, "op_02: %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
                print_f(mlogPool, "spy", str);  
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }
    return ps_next(data);
}
static int stspy_03(struct psdata_s *data) 
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    rlt = abs_result(data->result); 

    //sprintf(str, "op_03 - rlt:0x%x \n", rlt); 
    //print_f(mlogPool, "spy", str); 

    switch (rlt) {
        case STINIT:
            ch = 5; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(str, "op_03: result: %x\n", data->result); 
            //print_f(mlogPool, "spy", str);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}
static int stspy_04(struct psdata_s *data) 
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    rlt = abs_result(data->result); 

    //sprintf(str, "op_04 - rlt:0x%.8x \n", rlt); 
    //print_f(mlogPool, "spy", str); 

    switch (rlt) {
        case STINIT:
            ch = 6; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(str, "op_04: result: %x\n", data->result); 
            //print_f(mlogPool, "spy", str);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }
    return ps_next(data);
}
static int stspy_05(struct psdata_s *data) 
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    rlt = abs_result(data->result); 

    //sprintf(str, "op_05 - rlt:0x%.8x \n", rlt); 
    //print_f(mlogPool, "spy", str); 

    switch (rlt) {
        case STINIT:
            ch = 8; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(str, "op_05: result: %x\n", data->result); 
            //print_f(mlogPool, "spy", str);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }
    return ps_next(data);
}
static int stbullet_01(struct psdata_s *data) 
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct info16Bit_s *p;
    struct procRes_s *rs;
    struct modersp_s *mch;
    rs = data->rs;
    p = &rs->pmch->get;
    mch = &rs->pmch->mch;
    rlt = abs_result(data->result); 
    //sprintf(str, "op_01: rlt: %.8x result: %.8x ans:%d\n", rlt, data->result, data->ansp0); 
    //print_f(mlogPool, "bullet", str);  

    switch (rlt) {
        case STINIT:
            ch = 10; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(str, "op_01: result: %x\n", data->result); 
            //print_f(mlogPool, "bullet", str);  

            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                sprintf(str, "op_01: %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
                print_f(mlogPool, "bullet", str);  
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}
static int stbullet_02(struct psdata_s *data) 
{ 
    uint32_t rlt;
    char str[128], ch = 0; 
    struct aspConfig_s *pct=0, *pdt=0;
    struct info16Bit_s *p=0, *t=0;
    struct procRes_s *rs;

    rs = data->rs;
    p = &rs->pmch->get;
    t = &rs->pmch->tmp;
    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 
    //sprintf(str, "op_02: rlt: %.8x result: %.8x ans:%d\n", rlt, data->result, data->ansp0); 
    //print_f(mlogPool, "bullet", str);  

    switch (rlt) {
        case STINIT:
            pdt = &pct[ASPOP_SCAN_SINGLE];
            if (pdt->opCode != OP_SINGLE) {
                sprintf(rs->logs, "op02, OP_SINGLE opcode is wrong val:%x\n", pdt->opCode); 
                print_f(rs->plogs, "SIG", rs->logs);  
                data->result = emb_result(data->result, EVTMAX);
            } else if (!(pdt->opStatus & ASPOP_STA_WR)) {
                sprintf(rs->logs, "op02, OP_SINGLE status is wrong val:%x\n", pdt->opStatus); 
                print_f(rs->plogs, "SIG", rs->logs);  

                        ch = 12; 
                        t->opcode =  OP_SINGLE;;
                        t->data = SINSCAN_DUAL_STRM;
                        memset(p, 0, sizeof(struct info16Bit_s));

                        rs_ipc_put(data->rs, &ch, 1);
                        data->result = emb_result(data->result, WAIT);
                        sprintf(rs->logs, "op_02: result: %x, goto %d\n", data->result, ch); 
                        print_f(rs->plogs, "SIG", rs->logs);  

                //data->result = emb_result(data->result, EVTMAX);
            } else {
                switch(pdt->opValue) {
                    //case SINSCAN_WIFI_ONLY:
                    //case SINSCAN_SD_ONLY:
                    //case SINSCAN_WIFI_SD:
                    //case SINSCAN_WHIT_BLNC:
                    //case SINSCAN_USB:
                    case SINSCAN_DUAL_STRM:
                    case SINSCAN_DUAL_SD:
                        ch = 12; 
                        t->opcode =  pdt->opCode;
                        t->data = pdt->opValue;
                        memset(p, 0, sizeof(struct info16Bit_s));

                        rs_ipc_put(data->rs, &ch, 1);
                        data->result = emb_result(data->result, WAIT);
                        sprintf(rs->logs, "op_02: result: %x, goto %d\n", data->result, ch); 
                        print_f(rs->plogs, "SIG", rs->logs);  
                        break;
                    default:
                        sprintf(rs->logs, "WARNING!!! op02, opValue is unexpected val:%x\n", pdt->opValue);
                        print_f(rs->plogs, "SIG", rs->logs);  
                        data->result = emb_result(data->result, EVTMAX);
                        break;
                }
            }        
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }
    return ps_next(data);
}
static int stbullet_03(struct psdata_s *data) 
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    
    rlt = abs_result(data->result); 
    //sprintf(str, "op_03: rlt: %.8x result: %.8x ans:%d\n", rlt, data->result, data->ansp0); 
    //print_f(mlogPool, "bullet", str);  

    switch (rlt) {
        case STINIT:
            ch = 14; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }
    return ps_next(data);
}
static int stbullet_04(struct psdata_s *data) 
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    rlt = abs_result(data->result); 
    //sprintf(str, "op_04: rlt: %x result: %x ans:%d\n", rlt, data->result, data->ansp0); 
    //print_f(mlogPool, "bullet", str);  

    switch (rlt) {
        case STINIT:
            ch = 17; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }
    return ps_next(data);
}
static int stbullet_05(struct psdata_s *data) 
{ 
    char str[128], ch = 0;
    uint32_t rlt;
    rlt = abs_result(data->result); 
    //sprintf(str, "op_05: rlt: %x result: %x ans:%d\n", rlt, data->result, data->ansp0);  
    //print_f(mlogPool, "bullet", str);  

    switch (rlt) {
        case STINIT:
            ch = 20; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }
    return ps_next(data);
}
static int stlaser_01(struct psdata_s *data) 
{ 
    char str[128], ch = 0;
    uint32_t rlt;

    rlt = abs_result(data->result); 
    //sprintf(str, "op_01: rlt: %x result: %x ans:%d\n", rlt, data->result, data->ansp0);  
    //print_f(mlogPool, "laser", str);  

    switch (rlt) {
        case STINIT:
            ch = 21; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }
    return ps_next(data);
}
static int stlaser_02(struct psdata_s *data) 
{ 
    int ret=0;
    char str[128], ch = 0;
    uint32_t rlt, val=0;
    struct aspConfig_s *pct=0, *pdt=0;

    pct = data->rs->pcfgTable;
    rlt = abs_result(data->result); 
    //sprintf(str, "op_02: rlt: %.8x result: %.8x ans:%d\n", rlt, data->result, data->ansp0);  
    //print_f(mlogPool, "laser", str);  

    switch (rlt) {
        case STINIT:
            ch = 6; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                pdt = &pct[ASPOP_EG_DECT];
                if ((pdt->opStatus == ASPOP_STA_UPD) && (pdt->opValue == 1)) {
#if CROP_USE_META
                    data->bkofw = emb_bk(data->bkofw, SAVPARM, PSRLT);
#else
                    data->bkofw = emb_bk(data->bkofw, WTBAKQ, PSTSM);
#endif
                    data->result = emb_result(data->result, BKWRD);
                } else {
                    data->result = emb_result(data->result, FWORD);
                }
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }
    return ps_next(data);
}
static int stlaser_03(struct psdata_s *data) 
{ 
    char str[128], ch = 0;
    uint32_t rlt;
    rlt = abs_result(data->result); 
    //sprintf(str, "op_03: rlt: %x result: %x ans:%d\n", rlt, data->result, data->ansp0);  
    //print_f(mlogPool, "laser", str);  

    switch (rlt) {
        case STINIT:
            ch = 17; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stlaser_04(struct psdata_s *data) 
{ 
    char str[128], ch = 0;
    uint32_t rlt;
    rlt = abs_result(data->result); 
    //sprintf(str, "op_04: rlt: %x result: %x ans:%d\n", rlt, data->result, data->ansp0);  
    //print_f(mlogPool, "laser", str);  

    switch (rlt) {
        case STINIT:
            ch = 5; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stlaser_05(struct psdata_s *data) 
{ 
    char str[128], ch = 0;
    uint32_t rlt;
    rlt = abs_result(data->result); 
    //sprintf(str, "op_05: rlt: %.8x result: %.8x ans:%d\n", rlt, data->result, data->ansp0);  
    //print_f(mlogPool, "laser", str);  

    switch (rlt) {
        case STINIT:
            ch = 7; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 0xed) {
                data->result = emb_result(data->result, EVTMAX);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0x7f;
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int shmem_rlt_get(struct mainRes_s *mrs, int seq, int p)
{
    int ret, sz, len;
    char ch, *stop_at, dst[16];;

    ret = mrs_ipc_get(mrs, &ch, 1, p);
    if (ret > 0) {
        len = mrs_ipc_get(mrs, &ch, 1, p);
        if ((len > 0) && (ch == 'l')) {
            len = mrs_ipc_get(mrs, dst, 8, p);
            if (len == 8) {
                sz = strtoul(dst, &stop_at, 10);
            }
        }
    
        if ((ch == 'l') && (sz == 0)) {
        } else {
            ring_buf_prod_dual(&mrs->dataRx, seq);
        }
        //sprintf(mlog, "[dback] ch:%c rt:%d idx:%d \n", ch,  len, seq);
        //shmem_dump(addr[0], sz);
        if (ch == 'l') {
            ring_buf_set_last_dual(&mrs->dataRx, sz, seq);
        }

    }

    return ret;
}

static int shmem_pop_send(struct mainRes_s *mrs, char **addr, int seq, int p)
{
    char str[128];
    int sz = 0;
    sz = ring_buf_get_dual(&mrs->dataRx, addr, seq);
    //sprintf(mlog, "shmem pop:0x%.8x, seq:%d sz:%d\n", *addr, seq, sz);
    if (sz < 0) return (-1);
    sprintf(str, "d%.8xl%.8d\n", *addr, sz);
    print_f(&mrs->plog, "pop", str);
    //sprintf(mlog, "[%s]\n", str);
    mrs_ipc_put(mrs, str, 18, p);

    return sz;
}

static int shmem_dump(char *src, int size)
{
    char str[128];
    int inc;
    if (!src) return -1;

    inc = 0;
    sprintf(str, "memdump[0x%.8x] sz%d: \n", src, size);
    print_f(mlogPool, "dump", str);
    while (inc < size) {
        sprintf(str, "%.2x ", *src);
        print_f(mlogPool, NULL, str);

        if (!((inc+1) % 16)) {
            sprintf(str, "\n");
            print_f(mlogPool, NULL, str);
        }
        inc++;
        src++;
    }

    return inc;
}
static int shmem_from_str(char **addr, char *dst, char *sz)
{
    char *stop_at;
    int size;
    if ((!addr) || (!dst) || (!sz)) return -1;

    *addr = (char *)strtoul(dst, &stop_at, 16);
    size = strtoul(sz, &stop_at, 10);

    return size;
}

static int ring_buf_info_len(struct shmem_s *pp)
{
    int dualn = 0;
    int leadn = 0;
    int folwn = 0;
    int dist;

    folwn = pp->r->folw.run * pp->slotn + pp->r->folw.seq;
    dualn = pp->r->dual.run * pp->slotn + pp->r->dual.seq;
    leadn = pp->r->lead.run * pp->slotn + pp->r->lead.seq;

    if (dualn > leadn) {
        dist = dualn - folwn;
    } else {
        dist = leadn - folwn;
    }

    return dist;
}

#define DUAL_STREAM_RING_LOG (0)
static int ring_buf_init(struct shmem_s *pp)
{
    pp->r->lead.run = 0;
    pp->r->lead.seq = 0;
    pp->r->dual.run = 0;
    pp->r->dual.seq = 1;
    pp->r->prelead.run = 0;
    pp->r->prelead.seq = 0;
    pp->r->predual.run = 0;
    pp->r->predual.seq = 1;
    pp->r->folw.run = 0;
    pp->r->folw.seq = 0;
    pp->r->psudo.run = 0;
    pp->r->psudo.seq = 0;
    pp->lastflg = 0;
    pp->lastsz = 0;
    pp->dualsz = 0;

    msync(pp, sizeof(struct shmem_s), MS_SYNC);
    return 0;
}

static int ring_buf_get_dual(struct shmem_s *pp, char **addr, int sel)
{
    char str[128];
    int dualn = 0;
    int folwn = 0;
    int dist;
    int tmps;

    sel = sel % 2;

    folwn = pp->r->folw.run * pp->slotn + pp->r->folw.seq;

    if (sel) {
        dualn = pp->r->predual.run * pp->slotn + pp->r->predual.seq;
    } else {
        dualn = pp->r->prelead.run * pp->slotn + pp->r->prelead.seq;
    }

    dist = dualn - folwn;
    //sprintf(str, "get d:%d, %d /%d \n", dist, dualn, folwn);
    //print_f(mlogPool, "ring", str);

    if (dist > (pp->slotn - 3))  return -1;

    if (sel) {
        if ((pp->r->predual.seq + 2) < pp->slotn) {
            *addr = pp->pp[pp->r->predual.seq+2];
        } else {
            tmps = (pp->r->predual.seq+2) % pp->slotn;
            *addr = pp->pp[tmps];
        }
    } else {
        if ((pp->r->prelead.seq + 2) < pp->slotn) {
            *addr = pp->pp[pp->r->prelead.seq+2];
        } else {
            tmps = (pp->r->prelead.seq+2) % pp->slotn;
            *addr = pp->pp[tmps];
        }
    }

    if (sel) {
        if ((pp->r->predual.seq + 2) < pp->slotn) {
            pp->r->predual.seq += 2;
        } else {
            pp->r->predual.seq = 1;
            pp->r->predual.run += 1;
        }
    } else {
        if ((pp->r->prelead.seq + 2) < pp->slotn) {
            pp->r->prelead.seq += 2;
        } else {
            pp->r->prelead.seq = 0;
            pp->r->prelead.run += 1;
        }
    }

    return pp->chksz;   
}

static int ring_buf_get(struct shmem_s *pp, char **addr)
{
    char str[128];
    int leadn = 0;
    int folwn = 0;
    int dist;

    folwn = pp->r->folw.run * pp->slotn + pp->r->folw.seq;
    leadn = pp->r->lead.run * pp->slotn + pp->r->lead.seq;

    dist = leadn - folwn;
    //sprintf(str, "get d:%d, %d \n", dist, leadn, folwn);
    //print_f(mlogPool, "ring", str);

    if (dist > (pp->slotn - 2))  return -1;

    if ((pp->r->lead.seq + 1) < pp->slotn) {
        *addr = pp->pp[pp->r->lead.seq+1];
    } else {
        *addr = pp->pp[0];
    }

    return pp->chksz;   
}

static int ring_buf_set_last_dual(struct shmem_s *pp, int size, int sel)
{
    int tlen=0;
    char str[128];
    sel = sel % 2;

    tlen = size % MIN_SECTOR_SIZE;
    if (tlen) {
        size = size + MIN_SECTOR_SIZE - tlen;
    }

    if (sel) {
        pp->dualsz = size;
    } else {
        pp->lastsz = size;
    }
    pp->lastflg += 1;

    //sprintf(str, "set last d:%d l:%d f:%d \n", pp->dualsz, pp->lastsz, pp->lastflg);
    //print_f(mlogPool, "ring", str);

    msync(pp, sizeof(struct shmem_s), MS_SYNC);
    return pp->lastflg;
}

static int ring_buf_set_last(struct shmem_s *pp, int size)
{
    char str[128];
    int tlen=0;
    tlen = size % MIN_SECTOR_SIZE;
    if (tlen) {
        size = size + MIN_SECTOR_SIZE - tlen;
    }

    pp->lastsz = size;
    pp->lastflg = 1;

    sprintf(str, "set last l:%d f:%d \n", pp->lastsz, pp->lastflg);
    print_f(mlogPool, "ring", str);

    msync(pp, sizeof(struct shmem_s), MS_SYNC);
    return pp->lastflg;
}
static int ring_buf_prod_dual(struct shmem_s *pp, int sel)
{
    char str[128];
    sel = sel % 2;
    if (sel) {
        if ((pp->r->dual.seq + 2) < pp->slotn) {
            pp->r->dual.seq += 2;
        } else {
            pp->r->dual.seq = 1;
            pp->r->dual.run += 1;
        }
    } else {
        if ((pp->r->lead.seq + 2) < pp->slotn) {
            pp->r->lead.seq += 2;
        } else {
            pp->r->lead.seq = 0;
            pp->r->lead.run += 1;
        }
    }
    msync(pp, sizeof(struct shmem_s), MS_SYNC);
#if DUAL_STREAM_RING_LOG
    sprintf(str, "prod %d %d, %d %d\n", pp->r->lead.run, pp->r->lead.seq, pp->r->dual.run, pp->r->dual.seq);
    print_f(mlogPool, "ring", str);
#endif
    return 0;
}

static int ring_buf_prod(struct shmem_s *pp)
{
    char str[128];
    if ((pp->r->lead.seq + 1) < pp->slotn) {
        pp->r->lead.seq += 1;
    } else {
        pp->r->lead.seq = 0;
        pp->r->lead.run += 1;
    }

    msync(pp, sizeof(struct shmem_s), MS_SYNC);
    
    //sprintf(str, "prod %d %d/\n", pp->r->lead.run, pp->r->lead.seq);
    //print_f(mlogPool, "ring", str);

    return 0;
}

static int ring_buf_cons_dual(struct shmem_s *pp, char **addr, int sel)
{
    int ret=-1;
    char str[128];
    int dualn = 0;
    int leadn = 0;
    int folwn = 0;
    int dist;

    folwn = pp->r->folw.run * pp->slotn + pp->r->folw.seq;
    dualn = pp->r->dual.run * pp->slotn + pp->r->dual.seq;
    leadn = pp->r->lead.run * pp->slotn + pp->r->lead.seq;
    if (dualn > leadn) {
        dist = dualn - folwn;
    } else {
        dist = leadn - folwn;
    }
#if DUAL_STREAM_RING_LOG
    sprintf(str, "cons d: %d %d/%d/%d \n", dist, leadn, dualn, folwn);
    print_f(mlogPool, "ring", str);
#endif
    if ((pp->lastflg) && (dist < 1)) return (-1);
    if (dist < 1)  return (-2);

    if ((pp->r->folw.seq + 1) < pp->slotn) {
        *addr = pp->pp[pp->r->folw.seq + 1];
        pp->r->folw.seq += 1;
    } else {
        *addr = pp->pp[0];
        pp->r->folw.seq = 0;
        pp->r->folw.run += 1;
    }

    if ((pp->lastflg) && (dist == 1)) {
        sprintf(str, "[clast] f:%d %d, d:%d %d l: %d %d \n", pp->r->folw.run, pp->r->folw.seq, 
            pp->r->dual.run, pp->r->dual.seq, pp->r->lead.run, pp->r->lead.seq);
        print_f(mlogPool, "ring", str);
        if (dualn > leadn) {
            if ((pp->r->folw.run == pp->r->dual.run) &&
             (pp->r->folw.seq == pp->r->dual.seq)) {
                //return pp->dualsz;
                ret = pp->dualsz;
            }
        } else {
            if ((pp->r->folw.run == pp->r->lead.run) &&
             (pp->r->folw.seq == pp->r->lead.seq)) {
                //return pp->lastsz;
                ret = pp->lastsz;
            }
        }
    }

    if (ret < 0) {
        ret = pp->chksz;
    }
    msync(pp, sizeof(struct shmem_s), MS_SYNC);
    
    //sprintf(str, "cons dual len: %d \n", ret);
    //print_f(mlogPool, "ring", str);

    return ret;
}

static int ring_buf_cons_dual_psudo(struct shmem_s *pp, char **addr, int sel)
{
    int ret=-1;
    char str[128];
    int dualn = 0;
    int leadn = 0;
    int folwn = 0;
    int dist;

    folwn = pp->r->psudo.run * pp->slotn + pp->r->psudo.seq;
    dualn = pp->r->dual.run * pp->slotn + pp->r->dual.seq;
    leadn = pp->r->lead.run * pp->slotn + pp->r->lead.seq;
    if (dualn > leadn) {
        dist = dualn - folwn;
    } else {
        dist = leadn - folwn;
    }

    //sprintf(str, "[cons], d: %d %d/%d/%d \n", dist, leadn, dualn, folwn);
    //print_f(mlogPool, "ring", str);

    if ((pp->lastflg) && (dist < 1)) return (-1);
    if (dist < 1)  return (-2);

    if ((pp->r->psudo.seq + 1) < pp->slotn) {
        *addr = pp->pp[pp->r->psudo.seq + 1];
        pp->r->psudo.seq += 1;
    } else {
        *addr = pp->pp[0];
        pp->r->psudo.seq = 0;
        pp->r->psudo.run += 1;
    }

    if ((pp->lastflg) && (dist == 1)) {
        sprintf(str, "[psudo] f:%d %d, d:%d %d l: %d %d \n", pp->r->psudo.run, pp->r->psudo.seq, 
            pp->r->dual.run, pp->r->dual.seq, pp->r->lead.run, pp->r->lead.seq);
        print_f(mlogPool, "ring", str);
        if (dualn > leadn) {
            if ((pp->r->psudo.run == pp->r->dual.run) &&
             (pp->r->psudo.seq == pp->r->dual.seq)) {
                //return pp->dualsz;
                ret = pp->dualsz;
            }
        } else {
            if ((pp->r->psudo.run == pp->r->lead.run) &&
             (pp->r->psudo.seq == pp->r->lead.seq)) {
                //return pp->lastsz;
                ret = pp->lastsz;
            }
        }
    }

    if (ret < 0) {
        ret = pp->chksz;
    }
    msync(pp, sizeof(struct shmem_s), MS_SYNC);
    //sprintf(str, "psudo len %d \n", ret);
    //print_f(mlogPool, "ring", str);

    return ret;
}

static int ring_buf_cons(struct shmem_s *pp, char **addr)
{
    char str[128];
    int leadn = 0;
    int folwn = 0;
    int dist;

    folwn = pp->r->folw.run * pp->slotn + pp->r->folw.seq;
    leadn = pp->r->lead.run * pp->slotn + pp->r->lead.seq;
    dist = leadn - folwn;

    //sprintf(str, "cons, d: %d %d/%d - %d\n", dist, leadn, folwn,pp->lastflg);
    //print_f(mlogPool, "ring", str);

    if (dist < 1)  return -1;

    if ((pp->r->folw.seq + 1) < pp->slotn) {
        *addr = pp->pp[pp->r->folw.seq + 1];
        pp->r->folw.seq += 1;
    } else {
        *addr = pp->pp[0];
        pp->r->folw.seq = 0;
        pp->r->folw.run += 1;
    }

    if ((pp->lastflg) && (dist == 1)) {
        sprintf(str, "last, f: %d %d/l: %d %d\n", pp->r->folw.run, pp->r->folw.seq, pp->r->lead.run, pp->r->lead.seq);
        print_f(mlogPool, "ring", str);

        if ((pp->r->folw.run == pp->r->lead.run) &&
            (pp->r->folw.seq == pp->r->lead.seq)) {
            return pp->lastsz;
        }
    }

    msync(pp, sizeof(struct shmem_s), MS_SYNC);

    return pp->chksz;
}

static int ring_buf_cons_psudo(struct shmem_s *pp, char **addr)
{
    char str[128];
    int leadn = 0;
    int folwn = 0;
    int dist;

    folwn = pp->r->psudo.run * pp->slotn + pp->r->psudo.seq;
    leadn = pp->r->lead.run * pp->slotn + pp->r->lead.seq;
    dist = leadn - folwn;

    //sprintf(str, "psudo cons, d: %d %d/%d - %d\n", dist, leadn, folwn,pp->lastflg);
    //print_f(mlogPool, "ring", str);

    if (dist < 1)  return -1;

    if ((pp->r->psudo.seq + 1) < pp->slotn) {
        *addr = pp->pp[pp->r->psudo.seq + 1];
        pp->r->psudo.seq += 1;
    } else {
        *addr = pp->pp[0];
        pp->r->psudo.seq = 0;
        pp->r->psudo.run += 1;
    }

    if ((pp->lastflg) && (dist == 1)) {
        sprintf(str, "psudo last, f: %d %d/l: %d %d\n", pp->r->psudo.run, pp->r->psudo.seq, pp->r->lead.run, pp->r->lead.seq);
        print_f(mlogPool, "ring", str);

        if ((pp->r->psudo.run == pp->r->lead.run) &&
            (pp->r->psudo.seq == pp->r->lead.seq)) {
            return pp->lastsz;
        }
    }

    msync(pp, sizeof(struct shmem_s), MS_SYNC);

    return pp->chksz;
}

static int msp_spi_conf(int dev, int flag, void *bitset)
{
    int ret;
    
    if (!dev) {
        printf("spi-config, spi device id == 0\n");
        return -1;
    }
    ret = ioctl(dev, flag, bitset);

    //printf("tx/rx len: %d\n", ret);
    
    return ret;
}

static int mtx_data(int fd, uint8_t *rx_buff, uint8_t *tx_buff, int pksz, struct spi_ioc_transfer *tr)
{
    char mlog[256];
    int ret;

    memset(tr, 0, sizeof(struct spi_ioc_transfer));
    msync(tr, sizeof(struct spi_ioc_transfer), MS_SYNC);

    if (pksz > SPI_MAX_TXSZ) return (-3);

    tr->tx_buf = (unsigned long)tx_buff;
    tr->rx_buf = (unsigned long)rx_buff;
    tr->len = pksz;
    tr->delay_usecs = 0;
    tr->speed_hz = 1000000;
    tr->bits_per_word = 8;
//    tr->rx_nbits = 0;
//    tr->tx_nbits = 0;

    ret = msp_spi_conf(fd, SPI_IOC_MESSAGE(1), tr);
    if (ret < 0) {
        printf("spi error code: %d \n", ret);
    }
    
    return ret;
}

static int mrs_ipc_get(struct mainRes_s *mrs, char *str, int size, int idx)
{
    int ret;
    ret = read(mrs->pipeup[idx].rt[0], str, size);
    return ret;
}

static int mrs_ipc_put(struct mainRes_s *mrs, char *str, int size, int idx)
{
    int ret;
    ret = write(mrs->pipedn[idx].rt[1], str, size);
    return ret;
}

static int rs_ipc_put(struct procRes_s *rs, char *str, int size)
{
    int ret;
    ret = write(rs->ppipeup->rt[1], str, size);
    return ret;
}

static int rs_ipc_get(struct procRes_s *rs, char *str, int size)
{
    int ret;
    ret = read(rs->ppipedn->rt[0], str, size);
    return ret;
}

static int pn_init(struct procRes_s *rs)
{
    close(rs->ppipedn->rt[1]);
    close(rs->ppipeup->rt[0]);
    return 0;
}

static int pn_end(struct procRes_s *rs)
{
    close(rs->ppipedn->rt[0]); 
    close(rs->ppipeup->rt[1]);
    return 0;
}

static int p6_init(struct procRes_s *rs)
{
    int ret;
    ret = pn_init(rs);
    return ret;
}

static int p6_end(struct procRes_s *rs)
{
    int ret;
    ret = pn_end(rs);
    return ret;
}

static int p7_init(struct procRes_s *rs)
{
    int ret;
    ret = pn_init(rs);
    return ret;
}

static int p7_end(struct procRes_s *rs)
{
    int ret;
    ret = pn_end(rs);
    return ret;
}

static int p8_init(struct procRes_s *rs)
{
    int ret;
    ret = pn_init(rs);
    return ret;
}

static int p8_end(struct procRes_s *rs)
{
    int ret;
    ret = pn_end(rs);
    return ret;
}

static int p5_init(struct procRes_s *rs)
{
    int ret;
    ret = pn_init(rs);
    return ret;
}

static int p5_end(struct procRes_s *rs)
{
    int ret;
    ret = pn_end(rs);
    return ret;
}

static int p4_init(struct procRes_s *rs)
{
    int ret;
    ret = pn_init(rs);
    return ret;
}

static int p4_end(struct procRes_s *rs)
{
    int ret;
    ret = pn_end(rs);
    return ret;
}

static int p3_init(struct procRes_s *rs)
{
    int ret;
    ret = pn_init(rs);
    return ret;
}

static int p3_end(struct procRes_s *rs)
{
    int ret;
    ret = pn_end(rs);
    return ret;
}

static int p2_init(struct procRes_s *rs)
{
    int ret;
    ret = pn_init(rs);
    return ret;
}

static int p2_end(struct procRes_s *rs)
{
    int ret;
    ret = pn_end(rs);
    return ret;
}

static int p1_init(struct procRes_s *rs)
{
    int ret;
    ret = pn_init(rs);
    return ret;
}

static int p1_end(struct procRes_s *rs)
{
    int ret;
    ret = pn_end(rs);
    return ret;
}

static int p0_init(struct mainRes_s *mrs) 
{
    close(mrs->pipedn[0].rt[0]);
    close(mrs->pipedn[1].rt[0]);
    close(mrs->pipedn[2].rt[0]);
    close(mrs->pipedn[3].rt[0]);
    close(mrs->pipedn[4].rt[0]);
    
    close(mrs->pipeup[0].rt[1]);
    close(mrs->pipeup[1].rt[1]);
    close(mrs->pipeup[2].rt[1]);
    close(mrs->pipeup[3].rt[1]);
    close(mrs->pipeup[4].rt[1]);

    return 0;
}

static int p0_end(struct mainRes_s *mrs)
{
    close(mrs->pipeup[0].rt[0]);
    close(mrs->pipeup[1].rt[0]);
    close(mrs->pipeup[2].rt[0]);
    close(mrs->pipeup[3].rt[0]);
    close(mrs->pipeup[4].rt[0]);

    close(mrs->pipedn[0].rt[1]);
    close(mrs->pipedn[1].rt[1]);
    close(mrs->pipedn[2].rt[1]);
    close(mrs->pipedn[3].rt[1]);
    close(mrs->pipedn[4].rt[1]);

    kill(mrs->sid[0]);
    kill(mrs->sid[1]);
    kill(mrs->sid[2]);
    kill(mrs->sid[3]);
    kill(mrs->sid[4]);

    fclose(mrs->fs);
    munmap(mrs->dataRx.pp[0], 1024*SPI_TRUNK_SZ);
    munmap(mrs->dataTx.pp[0], 256*SPI_TRUNK_SZ);
    munmap(mrs->cmdRx.pp[0], 256*SPI_TRUNK_SZ);
    munmap(mrs->cmdTx.pp[0], 256*SPI_TRUNK_SZ);
    aspFree(mrs->dataRx.pp);
    aspFree(mrs->cmdRx.pp);
    aspFree(mrs->dataTx.pp);
    aspFree(mrs->cmdTx.pp);
    return 0;
}

static int aspWaitResult(struct aspWaitRlt_s *tg)
{
    struct mainRes_s *mrs;
    int n, ms, c, pr, ret=0, wcnt=0;
    char ch=0, *rlt=0, dt;

    rlt = tg->wtRlt;
    if (!rlt) return -1;
    mrs = tg->wtMrs;
    if (!mrs) return -2;
    if (tg->wtComp) dt = *rlt;
    pr = tg->wtComp;
    c = tg->wtChan;
    ms = tg->wtMs;
    if (!ms) ms = 3000;

    while (1) {
        n=0; ch=0;
        n = mrs_ipc_get(mrs, &ch, 1, c);
        if (n >0) {
            if (pr) {
                if (ch == dt) {
                    *rlt = ch;
                     goto end;
                }
            } else {
                *rlt = ch;
                 goto end;
            }
        }
/*      
        usleep(ms);
        wcnt++;
        if (wcnt > 1000) {
            return -3;
        }
*/
    }
end:

    //sprintf(mrs->log, "wait rlt: %c 0x%.2x\n", ch, ch); 
    //print_f(&mrs->plog, "DBG", mrs->log);

    return ret;
}

#define FUNC_PROCEDURE_LOG (0)
static int cmdfunc_upd2host(struct mainRes_s *mrs, char cmd, char *rsp)
{
    char ch=0, param=0, *rlt=0;
    int ret=0, wcnt=0, n=0;
    uint16_t dt16;
    struct info16Bit_s *pkt;
    struct aspWaitRlt_s *pwt;
    if (!mrs) {ret = -1; goto end;}
    pwt = &mrs->wtg;
    pkt = &mrs->mchine.tmp;
    if ((!pwt) || (!pwt->wtRlt) || (!pwt->wtMrs)) {ret = -2; goto end;}
    if (!pkt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}
    if (!rsp) {ret = -5; goto end;}
    
#if FUNC_PROCEDURE_LOG
    sprintf(mrs->log, "cmdfunc_upd2host opc:0x%x, dat:0x%x\n", pkt->opcode, pkt->data); 
    print_f(&mrs->plog, "DBG", mrs->log);
#endif
    dt16 = pkg_info(pkt);
    abs_info(&mrs->mchine.cur, dt16);

    ch = cmd;
    mrs_ipc_put(mrs, &ch, 1, 6);

    ch = cmd;
    pwt->wtComp = 1;
    *rlt = ch;
    n = aspWaitResult(pwt);
    if (n) {
        ret = (n * 10) -2; /* -32 */
        goto end;
    }
#if FUNC_PROCEDURE_LOG
    sprintf(mrs->log, "1.wt get %c\n", *rlt); 
    print_f(&mrs->plog, "DBG", mrs->log);
#endif
    pwt->wtComp = 0;
    n = aspWaitResult(pwt);
    if (n) {
        ret = (n * 10) -3;
        goto end; /* -33 */
    }    
    *rsp = *rlt;

/*
    if (*rlt == 0x1) {
        sprintf(mrs->log, "succeed:"); 
        print_dbg(&mrs->plog, mrs->log, n);
    } else {
        sprintf(mrs->log, "failed:"); 
        print_dbg(&mrs->plog, mrs->log, n);
    }
*/
    sprintf(mrs->log, "result:"); 
    print_dbg(&mrs->plog, mrs->log, n);

#if FUNC_PROCEDURE_LOG
    sprintf(mrs->log, "2.wt get 0x%x\n", *rlt); 
    print_f(&mrs->plog, "DBG", mrs->log);
#endif
    n = mrs_ipc_get(mrs, mrs->log, 256, pwt->wtChan);
    while (n > 0) {
        print_dbg(&mrs->plog, mrs->log, n);
        n = mrs_ipc_get(mrs, mrs->log, 256, pwt->wtChan);
    }

    dt16 = pkg_info(&mrs->mchine.get);
    abs_info(pkt, dt16);
#if FUNC_PROCEDURE_LOG
    sprintf(mrs->log, "3.wt get pkt op:0x%x, data:0x%x\n", pkt->opcode, pkt->data); 
    print_f(&mrs->plog, "DBG", mrs->log);
#endif
end:

    return ret;
}

static int cmdfunc_wt_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0;
    int ret=0, ix=0, n=0, err=0, upd=0, brk=0, pass=0;
    struct aspWaitRlt_s *pwt;
    struct info16Bit_s *pkt;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_wt_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);

    pkt = &mrs->mchine.tmp;
    pwt = &mrs->wtg;
    if (!pkt) {ret = -2; goto end;}
    if (!pwt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}

    /* set wait result mechanism */
    pwt->wtChan = 6;
    pwt->wtMs = 300;

    n = cmdfunc_upd2host(mrs, 'r', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }

    struct aspConfig_s* ctb = 0;
    for (ix = 0; ix < ASPOP_CODE_MAX; ix++) {
        ctb = &mrs->configTable[ix];
        if (!ctb) {ret = -5; goto end;}
        if (ctb->opStatus & ASPOP_STA_CON) {
            n = 0; rsp = 0;
            /* set data for update to scanner */
            pkt->opcode = ctb->opCode;
            pkt->data = ctb->opValue;
            n = cmdfunc_upd2host(mrs, 't', &rsp);
            if ((n == -32) || (n == -33)) {
                brk = 1;
                goto end;
            }
            
            if ((!n) && (rsp == 0x1) && (pkt->opcode == ctb->opCode) && (pkt->data == ctb->opValue)) {
            #if 0 /* remove for FPGA developing need */
                ctb->opStatus = ASPOP_STA_UPD; 
            #endif
                pass++;
            } else {
                /*
                sprintf(mrs->log, "<err++, n=%d rsp=%d opc:0x%x dat:0x%x>", n, rsp, pkt->opcode, pkt->data); 
                print_dbg(&mrs->plog, mrs->log, 0);
                */
                sprintf(mrs->log, "err++, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
                print_f(&mrs->plog, "DBG", mrs->log);

                err++;
            }
            upd++;
        }
        ctb = 0;
    }

    sprintf(mrs->log, "cmdfunc_wt_opcode total do:%d error:%d \n", upd, err); 
    print_f(&mrs->plog, "DBG", mrs->log);
end:

    if (brk | ret | err) {
        sprintf(mrs->log, "wt_NG,error:%d,pass:%d,break:%d,ret:%d", err, pass, brk,ret);
    } else {
        sprintf(mrs->log, "wt_PASS,error:%d,pass:%d,break:%d,ret:%d", err, pass, brk,ret);
    }
/*
    if (brk) {
        n = 0; rsp = 0;
        n = cmdfunc_upd2host(mrs, 'r', &rsp);
        sprintf(mrs->log, "failed:do=%d, error=%d ret=%d brk=%d n=%d rsp=0x%x", upd, err, ret, brk, n, rsp);
    } else if ((ret) || (err)) {
        sprintf(mrs->log, "failed:do=%d, error=%d ret=%d", upd, err, ret);         
    } else {
        sprintf(mrs->log, "succeed:do=%d, error=%d ret=%d", upd, err, ret);         
    }
 */

    n = strlen(mrs->log);
    print_dbg(&mrs->plog, mrs->log, n);
    printf_dbgflush(&mrs->plog, mrs);

    return ret;
}

static int cmdfunc_lh_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0;
    int ret=0, ix=0, n=0, brk=0;
    struct aspWaitRlt_s *pwt;
    struct info16Bit_s *pkt;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_lh_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);

    pkt = &mrs->mchine.tmp;
    pwt = &mrs->wtg;
    if (!pkt) {ret = -2; goto end;}
    if (!pwt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}

    /* set wait result mechanism */
    pwt->wtChan = 6;
    pwt->wtMs = 300;

    n = 0; rsp = 0;
    /* set data for update to scanner */
    pkt->opcode = OP_SINGLE;
    pkt->data = SINSCAN_DUAL_STRM;
    n = cmdfunc_upd2host(mrs, 'd', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }
        
    if ((n) && (rsp != 0x1)) {
         sprintf(mrs->log, "ERROR!!, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
         print_f(&mrs->plog, "DBG", mrs->log);
    }

    sprintf(mrs->log, "cmdfunc_lh_opcode n = %d, rsp = %d\n", n, rsp); 
    print_f(&mrs->plog, "DBG", mrs->log);
end:

    if (brk | ret) {
        sprintf(mrs->log, "E,%d,%d", ret, brk);
    } else {
        sprintf(mrs->log, "D,%d,%d", ret, brk);
    }
/*
    if (brk) {
        n = 0; rsp = 0;
        n = cmdfunc_upd2host(mrs, 'r', &rsp);
        sprintf(mrs->log, "failed:do=%d, error=%d ret=%d brk=%d n=%d rsp=0x%x", upd, err, ret, brk, n, rsp);
    } else if ((ret) || (err)) {
        sprintf(mrs->log, "failed:do=%d, error=%d ret=%d", upd, err, ret);         
    } else {
        sprintf(mrs->log, "succeed:do=%d, error=%d ret=%d", upd, err, ret);         
    }
 */

    n = strlen(mrs->log);
    print_dbg(&mrs->plog, mrs->log, n);
    
    printf_dbgflush(&mrs->plog, mrs);

    printf_flush(&mrs->plog, mrs->flog);
    
    return ret;
}

static int cmdfunc_go_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0;
    int ret=0, ix=0, n=0, brk=0;
    struct aspWaitRlt_s *pwt;
    struct info16Bit_s *pkt;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_act_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);

    pkt = &mrs->mchine.tmp;
    pwt = &mrs->wtg;
    if (!pkt) {ret = -2; goto end;}
    if (!pwt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}

    /* set wait result mechanism */
    pwt->wtChan = 6;
    pwt->wtMs = 300;

    n = 0; rsp = 0;
    /* set data for update to scanner */
    pkt->opcode = OP_SINGLE;
    pkt->data = SINSCAN_DUAL_STRM;
    n = cmdfunc_upd2host(mrs, 'n', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }
        
    if ((n) && (rsp != 0x1)) {
         sprintf(mrs->log, "ERROR!!, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
         print_f(&mrs->plog, "DBG", mrs->log);
    }

    sprintf(mrs->log, "cmdfunc_act_opcode n = %d, rsp = %d\n", n, rsp); 
    print_f(&mrs->plog, "DBG", mrs->log);
end:

    if (brk | ret) {
        sprintf(mrs->log, "E,%d,%d", ret, brk);
    } else {
        sprintf(mrs->log, "D,%d,%d", ret, brk);
    }

    n = strlen(mrs->log);
    print_dbg(&mrs->plog, mrs->log, n);
    printf_dbgflush(&mrs->plog, mrs);

    return ret;
}

static int cmdfunc_act_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0;
    int ret=0, ix=0, n=0, brk=0;
    struct aspWaitRlt_s *pwt;
    struct info16Bit_s *pkt;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_go_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);

    pkt = &mrs->mchine.tmp;
    pwt = &mrs->wtg;
    if (!pkt) {ret = -2; goto end;}
    if (!pwt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}

    /* set wait result mechanism */
    pwt->wtChan = 6;
    pwt->wtMs = 300;

    n = 0; rsp = 0;
    /* set data for update to scanner */
    pkt->opcode = OP_ACTION;
    pkt->data = ACTION_OPTION_01;
    n = cmdfunc_upd2host(mrs, 't', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }
        
    if ((n) && (rsp != 0x1)) {
         sprintf(mrs->log, "ERROR!!, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
         print_f(&mrs->plog, "DBG", mrs->log);
    }

    sprintf(mrs->log, "cmdfunc_act_opcode n = %d, rsp = %d\n", n, rsp); 
    print_f(&mrs->plog, "DBG", mrs->log);
end:

    if (brk | ret) {
        sprintf(mrs->log, "E,%d,%d", ret, brk);
    } else {
        sprintf(mrs->log, "D,%d,%d", ret, brk);
    }

    n = strlen(mrs->log);
    print_dbg(&mrs->plog, mrs->log, n);
    printf_dbgflush(&mrs->plog, mrs);

    return ret;
}

static int cmdfunc_op1_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0;
    int ret=0, ix=0, n=0, brk=0;
    struct aspWaitRlt_s *pwt;
    struct info16Bit_s *pkt;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_go_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);

    pkt = &mrs->mchine.tmp;
    pwt = &mrs->wtg;
    if (!pkt) {ret = -2; goto end;}
    if (!pwt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}

    /* set wait result mechanism */
    pwt->wtChan = 6;
    pwt->wtMs = 300;

    n = 0; rsp = 0;
    /* set data for update to scanner */
    pkt->opcode = OP_ACTION;
    pkt->data = ACTION_OPTION_01;
    n = cmdfunc_upd2host(mrs, 't', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }
        
    if ((n) && (rsp != 0x1)) {
         sprintf(mrs->log, "ERROR!!, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
         print_f(&mrs->plog, "DBG", mrs->log);
    }

    sprintf(mrs->log, "cmdfunc_act_opcode n = %d, rsp = %d\n", n, rsp); 
    print_f(&mrs->plog, "DBG", mrs->log);
end:

    if (brk | ret) {
        sprintf(mrs->log, "E,%d,%d", ret, brk);
    } else {
        sprintf(mrs->log, "D,%d,%d", ret, brk);
    }

    n = strlen(mrs->log);
    print_dbg(&mrs->plog, mrs->log, n);
    printf_dbgflush(&mrs->plog, mrs);

    return ret;
}

static int cmdfunc_op2_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0;
    int ret=0, ix=0, n=0, brk=0;
    struct aspWaitRlt_s *pwt;
    struct info16Bit_s *pkt;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_go_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);

    pkt = &mrs->mchine.tmp;
    pwt = &mrs->wtg;
    if (!pkt) {ret = -2; goto end;}
    if (!pwt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}

    /* set wait result mechanism */
    pwt->wtChan = 6;
    pwt->wtMs = 300;

    n = 0; rsp = 0;
    /* set data for update to scanner */
    pkt->opcode = OP_ACTION;
    pkt->data = ACTION_OPTION_02;
    n = cmdfunc_upd2host(mrs, 't', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }
        
    if ((n) && (rsp != 0x1)) {
         sprintf(mrs->log, "ERROR!!, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
         print_f(&mrs->plog, "DBG", mrs->log);
    }

    sprintf(mrs->log, "cmdfunc_act_opcode n = %d, rsp = %d\n", n, rsp); 
    print_f(&mrs->plog, "DBG", mrs->log);
end:

    if (brk | ret) {
        sprintf(mrs->log, "E,%d,%d", ret, brk);
    } else {
        sprintf(mrs->log, "D,%d,%d", ret, brk);
    }

    n = strlen(mrs->log);
    print_dbg(&mrs->plog, mrs->log, n);
    printf_dbgflush(&mrs->plog, mrs);

    return ret;
}

static int cmdfunc_op3_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0;
    int ret=0, ix=0, n=0, brk=0;
    struct aspWaitRlt_s *pwt;
    struct info16Bit_s *pkt;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_go_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);

    pkt = &mrs->mchine.tmp;
    pwt = &mrs->wtg;
    if (!pkt) {ret = -2; goto end;}
    if (!pwt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}

    /* set wait result mechanism */
    pwt->wtChan = 6;
    pwt->wtMs = 300;

    n = 0; rsp = 0;
    /* set data for update to scanner */
    pkt->opcode = OP_ACTION;
    pkt->data = ACTION_OPTION_03;
    n = cmdfunc_upd2host(mrs, 't', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }
        
    if ((n) && (rsp != 0x1)) {
         sprintf(mrs->log, "ERROR!!, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
         print_f(&mrs->plog, "DBG", mrs->log);
    }

    sprintf(mrs->log, "cmdfunc_act_opcode n = %d, rsp = %d\n", n, rsp); 
    print_f(&mrs->plog, "DBG", mrs->log);
end:

    if (brk | ret) {
        sprintf(mrs->log, "E,%d,%d", ret, brk);
    } else {
        sprintf(mrs->log, "D,%d,%d", ret, brk);
    }

    n = strlen(mrs->log);
    print_dbg(&mrs->plog, mrs->log, n);
    printf_dbgflush(&mrs->plog, mrs);

    return ret;
}

static int cmdfunc_op4_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0;
    int ret=0, ix=0, n=0, brk=0;
    struct aspWaitRlt_s *pwt;
    struct info16Bit_s *pkt;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_go_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);

    pkt = &mrs->mchine.tmp;
    pwt = &mrs->wtg;
    if (!pkt) {ret = -2; goto end;}
    if (!pwt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}

    /* set wait result mechanism */
    pwt->wtChan = 6;
    pwt->wtMs = 300;

    n = 0; rsp = 0;
    /* set data for update to scanner */
    pkt->opcode = OP_ACTION;
    pkt->data = ACTION_OPTION_04;
    n = cmdfunc_upd2host(mrs, 't', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }
        
    if ((n) && (rsp != 0x1)) {
         sprintf(mrs->log, "ERROR!!, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
         print_f(&mrs->plog, "DBG", mrs->log);
    }

    sprintf(mrs->log, "cmdfunc_act_opcode n = %d, rsp = %d\n", n, rsp); 
    print_f(&mrs->plog, "DBG", mrs->log);
end:

    if (brk | ret) {
        sprintf(mrs->log, "E,%d,%d", ret, brk);
    } else {
        sprintf(mrs->log, "D,%d,%d", ret, brk);
    }

    n = strlen(mrs->log);
    print_dbg(&mrs->plog, mrs->log, n);
    printf_dbgflush(&mrs->plog, mrs);

    return ret;
}

static int cmdfunc_op5_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0;
    int ret=0, ix=0, n=0, brk=0;
    struct aspWaitRlt_s *pwt;
    struct info16Bit_s *pkt;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_go_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);

    pkt = &mrs->mchine.tmp;
    pwt = &mrs->wtg;
    if (!pkt) {ret = -2; goto end;}
    if (!pwt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}

    /* set wait result mechanism */
    pwt->wtChan = 6;
    pwt->wtMs = 300;

    n = 0; rsp = 0;
    /* set data for update to scanner */
    pkt->opcode = OP_ACTION;
    pkt->data = ACTION_OPTION_05;
    n = cmdfunc_upd2host(mrs, 't', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }
        
    if ((n) && (rsp != 0x1)) {
         sprintf(mrs->log, "ERROR!!, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
         print_f(&mrs->plog, "DBG", mrs->log);
    }

    sprintf(mrs->log, "cmdfunc_act_opcode n = %d, rsp = %d\n", n, rsp); 
    print_f(&mrs->plog, "DBG", mrs->log);
end:

    if (brk | ret) {
        sprintf(mrs->log, "E,%d,%d", ret, brk);
    } else {
        sprintf(mrs->log, "D,%d,%d", ret, brk);
    }

    n = strlen(mrs->log);
    print_dbg(&mrs->plog, mrs->log, n);
    printf_dbgflush(&mrs->plog, mrs);

    return ret;
}

static int cmdfunc_sdon_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0;
    int ret=0, ix=0, n=0, brk=0;
    struct aspWaitRlt_s *pwt;
    struct info16Bit_s *pkt;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_sdon_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);

    pkt = &mrs->mchine.tmp;
    pwt = &mrs->wtg;
    if (!pkt) {ret = -2; goto end;}
    if (!pwt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}

    /* set wait result mechanism */
    pwt->wtChan = 6;
    pwt->wtMs = 300;

    n = 0; rsp = 0;
    /* set data for update to scanner */
    pkt->opcode = OP_SINGLE;
    pkt->data = SINSCAN_SD_ONLY;
    n = cmdfunc_upd2host(mrs, 'g', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }
        
    if ((n) && (rsp != 0x1)) {
         sprintf(mrs->log, "ERROR!!, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
         print_f(&mrs->plog, "DBG", mrs->log);
    }

    sprintf(mrs->log, "cmdfunc_sdon_opcode n = %d, rsp = %d\n", n, rsp); 
    print_f(&mrs->plog, "DBG", mrs->log);
end:

    if (brk | ret) {
        sprintf(mrs->log, "E,%d,%d", ret, brk);
    } else {
        sprintf(mrs->log, "D,%d,%d", ret, brk);
    }

    n = strlen(mrs->log);
    print_dbg(&mrs->plog, mrs->log, n);
    printf_dbgflush(&mrs->plog, mrs);

    return ret;
}

static int cmdfunc_wfisd_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0;
    int ret=0, ix=0, n=0, brk=0;
    struct aspWaitRlt_s *pwt;
    struct info16Bit_s *pkt;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_wfisd_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);

    pkt = &mrs->mchine.tmp;
    pwt = &mrs->wtg;
    if (!pkt) {ret = -2; goto end;}
    if (!pwt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}

    /* set wait result mechanism */
    pwt->wtChan = 6;
    pwt->wtMs = 300;

    n = 0; rsp = 0;
    /* set data for update to scanner */
    pkt->opcode = OP_SINGLE;
    pkt->data = SINSCAN_WIFI_SD;
    n = cmdfunc_upd2host(mrs, 'i', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }
        
    if ((n) && (rsp != 0x1)) {
         sprintf(mrs->log, "ERROR!!, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
         print_f(&mrs->plog, "DBG", mrs->log);
    }

    sprintf(mrs->log, "cmdfunc_wfisd_opcode n = %d, rsp = %d\n", n, rsp); 
    print_f(&mrs->plog, "DBG", mrs->log);
end:

    if (brk | ret) {
        sprintf(mrs->log, "E,%d,%d", ret, brk);
    } else {
        sprintf(mrs->log, "D,%d,%d", ret, brk);
    }

    n = strlen(mrs->log);
    print_dbg(&mrs->plog, mrs->log, n);
    printf_dbgflush(&mrs->plog, mrs);

    return ret;
}

static int cmdfunc_dulsd_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0;
    int ret=0, ix=0, n=0, brk=0;
    struct aspWaitRlt_s *pwt;
    struct info16Bit_s *pkt;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_dulsd_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);

    pkt = &mrs->mchine.tmp;
    pwt = &mrs->wtg;
    if (!pkt) {ret = -2; goto end;}
    if (!pwt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}

    /* set wait result mechanism */
    pwt->wtChan = 6;
    pwt->wtMs = 300;

    n = 0; rsp = 0;
    /* set data for update to scanner */
    pkt->opcode = OP_SINGLE;
    pkt->data = SINSCAN_DUAL_SD;
    n = cmdfunc_upd2host(mrs, 'j', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }
        
    if ((n) && (rsp != 0x1)) {
         sprintf(mrs->log, "ERROR!!, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
         print_f(&mrs->plog, "DBG", mrs->log);
    }

    sprintf(mrs->log, "cmdfunc_dulsd_opcode n = %d, rsp = %d\n", n, rsp); 
    print_f(&mrs->plog, "DBG", mrs->log);
end:

    if (brk | ret) {
        sprintf(mrs->log, "E,%d,%d", ret, brk);
    } else {
        sprintf(mrs->log, "D,%d,%d", ret, brk);
    }

    n = strlen(mrs->log);
    print_dbg(&mrs->plog, mrs->log, n);
    printf_dbgflush(&mrs->plog, mrs);

    return ret;
}

static int cmdfunc_tgr_opcode(int argc, char *argv[])
{
    int val=0;
    int n=0, ix=0, ret=0;
    char ch=0, opcode[5], param=0;
    uint32_t tg=0, cd=0;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_tgr_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);
    /* get opcode and parameter */
    ch = 'o';
    mrs_ipc_put(mrs, &ch, 1, 5);

    n = -1;
    while (n == -1) {
        n = mrs_ipc_get(mrs, opcode, 5, 5);
        //sprintf(mrs->log, "n:%d\n", n); 
        //print_f(&mrs->plog, "DBG", mrs->log);
    }

    /* debug print */ 
    /*
    for (ix = 0; ix < n; ix++) {
        sprintf(mrs->log, "%d.%.2x\n", ix, opcode[ix]); 
        print_f(&mrs->plog, "DBG", mrs->log);
    }
    */

    if (n != 5) {ret = -2; goto end;}
    if ((opcode[0] != 0xaa) && (opcode[0] != 0xad)) {ret = -3; goto end;}
    if (opcode[4] != 0xa5) {ret = -4; goto end;}
    if (opcode[2] != '/') {ret = -5; goto end;}

    tg = opcode[1];
    struct aspConfig_s* ctb = 0;
    for (ix = 0; ix < ASPOP_CODE_MAX; ix++) {
        ctb = &mrs->configTable[ix];
        if (tg == ctb->opCode) {
            break;
        }
        ctb = 0;
    }

    if (!ctb) {
        sprintf(mrs->log, "cmdfunc_tgr_opcode - 3\n"); 
        print_f(&mrs->plog, "DBG", mrs->log);
        ret = -7;
        goto end;
    }

    if (opcode[0] == 0xaa) {
        cd = opcode[3];

        if (cd == ctb->opValue) {
            ctb->opStatus |= ASPOP_STA_WR;
            param = ctb->opValue;
            goto end;
        }

        ret = cmdfunc_opchk_single(cd, ctb->opMask, ctb->opBitlen, ctb->opType);
        if (ret < 0) {
            ret = (ret * 10) -6;
        } else {            
            ctb->opValue = cd;
            ctb->opStatus |= ASPOP_STA_WR;
        }
        
        sprintf(mrs->log, "WT opcode 0x%.2x/0x%.2x, input value: 0x%.2x, ret:%d\n", ctb->opCode, ctb->opValue, cd, ret); 
        print_f(&mrs->plog, "DBG", mrs->log);
    } else {
        sprintf(mrs->log, "RD opcode 0x%.2x/0x%.2x, input value: 0x%.2x, ret:%d\n", ctb->opCode, ctb->opValue, cd, ret); 
        print_f(&mrs->plog, "DBG", mrs->log);
    }

    param = ctb->opValue;

end:

    if (ret < 0) {
        ch = 'x';
        mrs_ipc_put(mrs, &ch, 1, 5);
        ch = 0xf0;
        mrs_ipc_put(mrs, &ch, 1, 5);

        if (!ctb) {
            sprintf(mrs->log, "failed: input 0x%x/0x%x, ret:%d", opcode[1], opcode[3], ret); 
        } else {
            sprintf(mrs->log, "failed: input 0x%x/0x%x, mask:0x%x, bitlen:%d, ret:%d", opcode[1], opcode[3], ctb->opMask, ctb->opBitlen, ret); 
        }

    } else {
        if (ret == 0) {
            ch = 'p';
            mrs_ipc_put(mrs, &ch, 1, 5);

            sprintf(mrs->log, "same: result 0x%x/0x%x, ret: %d", ctb->opCode, ctb->opValue, ret);   
        } else {
            ch = 'p';
            mrs_ipc_put(mrs, &ch, 1, 5);

            sprintf(mrs->log, "succeed: result 0x%x/0x%x, ret: %d", ctb->opCode, ctb->opValue, ret);    
        } 
        mrs_ipc_put(mrs, &param, 1, 5);     
    }
    n = strlen(mrs->log);
    mrs_ipc_put(mrs, mrs->log, n, 5);
    
    sprintf(mrs->log, "opcode op end, log n =%d, ret=%d\n", n, ret); 
    print_f(&mrs->plog, "DBG", mrs->log);
    
    return 0;
}

static int cmdfunc_scango_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0, ch=0;
    int ret=0, ix=0, n=0, brk=0;
    struct aspWaitRlt_s *pwt;
    struct info16Bit_s *pkt;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_scango_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);

    pkt = &mrs->mchine.tmp;
    pwt = &mrs->wtg;
    if (!pkt) {ret = -2; goto end;}
    if (!pwt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}

    /* set wait result mechanism */
    pwt->wtChan = 6;
    pwt->wtMs = 300;

    n = 0; rsp = 0;
    /* set data for update to scanner */
    n = cmdfunc_upd2host(mrs, 'z', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }
        
    if (rsp != 0x1) {
         sprintf(mrs->log, "ERROR!!, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
         print_f(&mrs->plog, "DBG", mrs->log);
         ret = -1;
    }

    sprintf(mrs->log, "cmdfunc_scango_opcode n = %d, rsp = %d\n", n, rsp); 
    print_f(&mrs->plog, "DBG", mrs->log);
end:

    if (brk | ret) {
        sprintf(mrs->log, "SCANGO_NG,%d,%d", ret, brk);
        ch = 'z';
    } else {
        sprintf(mrs->log, "SCANGO_OK,%d,%d", ret, brk);
        ch = 'Z';
    }
    
    mrs_ipc_put(mrs, &ch, 1, 5);

    n = strlen(mrs->log);
    print_dbg(&mrs->plog, mrs->log, n);
    printf_dbgflush(&mrs->plog, mrs);

    return ret;
}

static int cmdfunc_meta_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0;
    int ret=0, ix=0, n=0, brk=0;
    struct aspWaitRlt_s *pwt;
    struct info16Bit_s *pkt;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_meta_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);

    pkt = &mrs->mchine.tmp;
    pwt = &mrs->wtg;
    if (!pkt) {ret = -2; goto end;}
    if (!pwt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}

    /* set wait result mechanism */
    pwt->wtChan = 6;
    pwt->wtMs = 300;

    n = 0; rsp = 0;
    /* set data for update to scanner */
    pkt->opcode = OP_META_DAT;
    pkt->data = ASPMETA_POWON_INIT;
    n = cmdfunc_upd2host(mrs, 'y', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }
        
    if ((n) && (rsp != 0x1)) {
         sprintf(mrs->log, "ERROR!!, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
         print_f(&mrs->plog, "DBG", mrs->log);
    }

    sprintf(mrs->log, "cmdfunc_meta_opcode n = %d, rsp = %d\n", n, rsp); 
    print_f(&mrs->plog, "DBG", mrs->log);
end:

    if (brk | ret) {
        sprintf(mrs->log, "META_NG,%d,%d", ret, brk);
        //print_f(&mrs->plog, "DBG", mrs->log);
    } else {
        sprintf(mrs->log, "META_OK,%d,%d", ret, brk);
        //print_f(&mrs->plog, "DBG", mrs->log);
    }

    n = strlen(mrs->log);
    print_dbg(&mrs->plog, mrs->log, n);
    printf_dbgflush(&mrs->plog, mrs);

    return ret;
}

static int cmdfunc_apm_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0;
    int ret=0, ix=0, n=0, brk=0;
    struct aspWaitRlt_s *pwt;
    struct info16Bit_s *pkt;
    struct mainRes_s *mrs=0;
    struct aspConfig_s* ctb = 0;

    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_ap_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);

    ctb = &mrs->configTable[ASPOP_AP_MODE];
    
    pkt = &mrs->mchine.tmp;
    pwt = &mrs->wtg;
    if (!pkt) {ret = -2; goto end;}
    if (!pwt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}


    /* set wait result mechanism */
    pwt->wtChan = 6;
    pwt->wtMs = 300;

    n = 0; rsp = 0;
    /* set data for update to scanner */
    pkt->opcode = OP_SINGLE;
    pkt->data = SINSCAN_DUAL_SD;

    ctb->opValue = APM_AP;
    ctb->opStatus = ASPOP_STA_APP;
    
    n = cmdfunc_upd2host(mrs, 'r', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }
    
    n = cmdfunc_upd2host(mrs, 'q', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }
        
    if ((n) && (rsp != 0x1)) {
         sprintf(mrs->log, "ERROR!!, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
         print_f(&mrs->plog, "DBG", mrs->log);
    }

    sprintf(mrs->log, "cmdfunc_ap_opcode n = %d, rsp = %d\n", n, rsp); 
    print_f(&mrs->plog, "DBG", mrs->log);
end:

    if (brk | ret) {
        sprintf(mrs->log, "E,%d,%d", ret, brk);
    } else {
        sprintf(mrs->log, "D,%d,%d", ret, brk);
    }

    n = strlen(mrs->log);
    print_dbg(&mrs->plog, mrs->log, n);
    printf_dbgflush(&mrs->plog, mrs);

    return ret;
}

static int cmdfunc_vector_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0;
    int ret=0, ix=0, n=0, brk=0;
    struct aspWaitRlt_s *pwt;
    struct info16Bit_s *pkt;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_vector_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);

    pkt = &mrs->mchine.tmp;
    pwt = &mrs->wtg;
    if (!pkt) {ret = -2; goto end;}
    if (!pwt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}

    /* set wait result mechanism */
    pwt->wtChan = 6;
    pwt->wtMs = 300;

    n = 0; rsp = 0;
    /* set data for update to scanner */
    pkt->opcode = OP_SINGLE;
    pkt->data = SINSCAN_DUAL_SD;
    n = cmdfunc_upd2host(mrs, 'o', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }
        
    if ((n) && (rsp != 0x1)) {
         sprintf(mrs->log, "ERROR!!, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
         print_f(&mrs->plog, "DBG", mrs->log);
    }

    sprintf(mrs->log, "cmdfunc_vector_opcode n = %d, rsp = %d\n", n, rsp); 
    print_f(&mrs->plog, "DBG", mrs->log);
end:

    if (brk | ret) {
        sprintf(mrs->log, "E,%d,%d", ret, brk);
    } else {
        sprintf(mrs->log, "D,%d,%d", ret, brk);
    }

    n = strlen(mrs->log);
    print_dbg(&mrs->plog, mrs->log, n);
    printf_dbgflush(&mrs->plog, mrs);

    return ret;
}

static int cmdfunc_crop_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0;
    int ret=0, ix=0, n=0, brk=0;
    struct aspWaitRlt_s *pwt;
    struct info16Bit_s *pkt;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_crop_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);

    pkt = &mrs->mchine.tmp;
    pwt = &mrs->wtg;
    if (!pkt) {ret = -2; goto end;}
    if (!pwt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}

    /* set wait result mechanism */
    pwt->wtChan = 6;
    pwt->wtMs = 300;

    n = 0; rsp = 0;
    /* set data for update to scanner */
    pkt->opcode = OP_SINGLE;
    pkt->data = SINSCAN_DUAL_SD;
    n = cmdfunc_upd2host(mrs, 'm', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }
        
    if ((n) && (rsp != 0x1)) {
         sprintf(mrs->log, "ERROR!!, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
         print_f(&mrs->plog, "DBG", mrs->log);
    }

    sprintf(mrs->log, "cmdfunc_dulsd_opcode n = %d, rsp = %d\n", n, rsp); 
    print_f(&mrs->plog, "DBG", mrs->log);
end:

    if (brk | ret) {
        sprintf(mrs->log, "E,%d,%d", ret, brk);
    } else {
        sprintf(mrs->log, "D,%d,%d", ret, brk);
    }

    n = strlen(mrs->log);
    print_dbg(&mrs->plog, mrs->log, n);
    printf_dbgflush(&mrs->plog, mrs);

    return ret;
}

static int cmdfunc_regw_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0;
    int ret=0, ix=0, n=0, brk=0;
    struct aspWaitRlt_s *pwt;
    struct info16Bit_s *pkt;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_go_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);

    pkt = &mrs->mchine.tmp;
    pwt = &mrs->wtg;
    if (!pkt) {ret = -2; goto end;}
    if (!pwt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}

    /* set wait result mechanism */
    pwt->wtChan = 6;
    pwt->wtMs = 300;

    n = 0; rsp = 0;
    /* set data for update to scanner */
    pkt->opcode = OP_RGWT;
    pkt->data = 0;
    n = cmdfunc_upd2host(mrs, 'e', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }
        
    if ((n) && (rsp != 0x1)) {
         sprintf(mrs->log, "ERROR!!, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
         print_f(&mrs->plog, "DBG", mrs->log);
    }

    sprintf(mrs->log, "cmdfunc_act_opcode n = %d, rsp = %d\n", n, rsp); 
    print_f(&mrs->plog, "DBG", mrs->log);
end:

    if (brk | ret) {
        sprintf(mrs->log, "E,%d,%d", ret, brk);
    } else {
        sprintf(mrs->log, "D,%d,%d", ret, brk);
    }

    n = strlen(mrs->log);
    print_dbg(&mrs->plog, mrs->log, n);
    printf_dbgflush(&mrs->plog, mrs);

    return ret;
}

static int cmdfunc_regr_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0;
    int ret=0, ix=0, n=0, brk=0;
    struct aspWaitRlt_s *pwt;
    struct info16Bit_s *pkt;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_go_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);

    pkt = &mrs->mchine.tmp;
    pwt = &mrs->wtg;
    if (!pkt) {ret = -2; goto end;}
    if (!pwt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}

    /* set wait result mechanism */
    pwt->wtChan = 6;
    pwt->wtMs = 300;

    n = 0; rsp = 0;
    /* set data for update to scanner */
    pkt->opcode = OP_RGRD;
    pkt->data = 0;
    n = cmdfunc_upd2host(mrs, 'f', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }
        
    if ((n) && (rsp != 0x1)) {
         sprintf(mrs->log, "ERROR!!, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
         print_f(&mrs->plog, "DBG", mrs->log);
    }

    sprintf(mrs->log, "cmdfunc_act_opcode n = %d, rsp = %d\n", n, rsp); 
    print_f(&mrs->plog, "DBG", mrs->log);
end:

    if (brk | ret) {
        sprintf(mrs->log, "E,%d,%d", ret, brk);
    } else {
        sprintf(mrs->log, "D,%d,%d", ret, brk);
    }

    n = strlen(mrs->log);
    print_dbg(&mrs->plog, mrs->log, n);
    printf_dbgflush(&mrs->plog, mrs);

    return ret;
}

static int cmdfunc_boot_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0;
    int ret=0, ix=0, n=0, brk=0;
    struct aspWaitRlt_s *pwt;
    struct info16Bit_s *pkt;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_boot_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);

    pkt = &mrs->mchine.tmp;
    pwt = &mrs->wtg;
    if (!pkt) {ret = -2; goto end;}
    if (!pwt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}

    /* set wait result mechanism */
    pwt->wtChan = 6;
    pwt->wtMs = 300;

    n = 0; rsp = 0;
    /* set data for update to scanner */
    pkt->opcode = OP_RGRD;
    pkt->data = 0;
    n = cmdfunc_upd2host(mrs, 'b', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }
        
    if ((n) || (rsp != 0x4)) {
         sprintf(mrs->log, "ERROR!!, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
         print_f(&mrs->plog, "DBG", mrs->log);
         ret = -5;
    }

    sprintf(mrs->log, "cmdfunc_boot_opcode n = %d, rsp = %d\n", n, rsp); 
    print_f(&mrs->plog, "DBG", mrs->log);
    
end:

    if (brk | ret) {
        sprintf(mrs->log, "BOOT_NG,%d,%d", ret, brk);
    } else {
        sprintf(mrs->log, "BOOT_OK,%d,%d", ret, brk);
    }

    n = strlen(mrs->log);
    print_dbg(&mrs->plog, mrs->log, n);
    printf_dbgflush(&mrs->plog, mrs);

    return ret;
}

static int cmdfunc_single_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0;
    int ret=0, ix=0, n=0, brk=0;
    struct aspWaitRlt_s *pwt;
    struct info16Bit_s *pkt;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_single_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);

    pkt = &mrs->mchine.tmp;
    pwt = &mrs->wtg;
    if (!pkt) {ret = -2; goto end;}
    if (!pwt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}

    /* set wait result mechanism */
    pwt->wtChan = 6;
    pwt->wtMs = 300;

    n = 0; rsp = 0;
    /* set data for update to scanner */
    pkt->opcode = OP_SINGLE;
    pkt->data = SINSCAN_WIFI_ONLY;
    n = cmdfunc_upd2host(mrs, 's', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }
        
    if ((n) && (rsp != 0x1)) {
         sprintf(mrs->log, "ERROR!!, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
         print_f(&mrs->plog, "DBG", mrs->log);
    }

    sprintf(mrs->log, "cmdfunc_single_opcode n = %d, rsp = %d\n", n, rsp); 
    print_f(&mrs->plog, "DBG", mrs->log);
    
end:

    if (brk | ret) {
        sprintf(mrs->log, "E,%d,%d", ret, brk);
    } else {
        sprintf(mrs->log, "D,%d,%d", ret, brk);
    }

    n = strlen(mrs->log);
    print_dbg(&mrs->plog, mrs->log, n);

    printf_dbgflush(&mrs->plog, mrs);

    printf_flush(&mrs->plog, mrs->flog);    
    
    return ret;
}

static int cmdfunc_dnld_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0;
    int ret=0, ix=0, n=0, brk=0;
    struct aspWaitRlt_s *pwt;
    struct info16Bit_s *pkt;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_dnld_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);

    pkt = &mrs->mchine.tmp;
    pwt = &mrs->wtg;
    if (!pkt) {ret = -2; goto end;}
    if (!pwt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}

    /* set wait result mechanism */
    pwt->wtChan = 6;
    pwt->wtMs = 300;

    n = 0; rsp = 0;
    /* set data for update to scanner */
    pkt->opcode = OP_SINGLE;
    pkt->data = SINSCAN_WIFI_ONLY;
    n = cmdfunc_upd2host(mrs, 'h', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }
        
    if ((n) && (rsp != 0x1)) {
         sprintf(mrs->log, "ERROR!!, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
         print_f(&mrs->plog, "DBG", mrs->log);
    }

    sprintf(mrs->log, "cmdfunc_dnld_opcode n = %d, rsp = %d\n", n, rsp); 
    print_f(&mrs->plog, "DBG", mrs->log);
    
end:

    if (brk | ret) {
        sprintf(mrs->log, "E,%d,%d", ret, brk);
    } else {
        sprintf(mrs->log, "D,%d,%d", ret, brk);
    }

    n = strlen(mrs->log);
    print_dbg(&mrs->plog, mrs->log, n);
    printf_dbgflush(&mrs->plog, mrs);

    return ret;
}

static int cmdfunc_upld_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0;
    int ret=0, ix=0, n=0, brk=0;
    struct aspWaitRlt_s *pwt;
    struct info16Bit_s *pkt;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_upld_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);

    pkt = &mrs->mchine.tmp;
    pwt = &mrs->wtg;
    if (!pkt) {ret = -2; goto end;}
    if (!pwt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}

    /* set wait result mechanism */
    pwt->wtChan = 6;
    pwt->wtMs = 300;

    n = 0; rsp = 0;
    /* set data for update to scanner */
    pkt->opcode = OP_SINGLE;
    pkt->data = SINSCAN_WIFI_ONLY;
    n = cmdfunc_upd2host(mrs, 'u', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }
        
    if ((n) && (rsp != 0x1)) {
         sprintf(mrs->log, "ERROR!!, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
         print_f(&mrs->plog, "DBG", mrs->log);
    }

    sprintf(mrs->log, "cmdfunc_upld_opcode n = %d, rsp = %d\n", n, rsp); 
    print_f(&mrs->plog, "DBG", mrs->log);
    
end:

    if (brk | ret) {
        sprintf(mrs->log, "E,%d,%d", ret, brk);
    } else {
        sprintf(mrs->log, "D,%d,%d", ret, brk);
    }

    n = strlen(mrs->log);
    print_dbg(&mrs->plog, mrs->log, n);
    printf_dbgflush(&mrs->plog, mrs);

    return ret;
}

static int cmdfunc_save_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0;
    int ret=0, ix=0, n=0, brk=0;
    struct aspWaitRlt_s *pwt;
    struct info16Bit_s *pkt;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_upld_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);

    pkt = &mrs->mchine.tmp;
    pwt = &mrs->wtg;
    if (!pkt) {ret = -2; goto end;}
    if (!pwt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}

    /* set wait result mechanism */
    pwt->wtChan = 6;
    pwt->wtMs = 300;

    n = 0; rsp = 0;
    /* set data for update to scanner */
    pkt->opcode = OP_SINGLE;
    pkt->data = SINSCAN_WIFI_ONLY;
    n = cmdfunc_upd2host(mrs, 'v', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }
        
    if ((n) && (rsp != 0x1)) {
         sprintf(mrs->log, "ERROR!!, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
         print_f(&mrs->plog, "DBG", mrs->log);
    }

    sprintf(mrs->log, "cmdfunc_upld_opcode n = %d, rsp = %d\n", n, rsp); 
    print_f(&mrs->plog, "DBG", mrs->log);
    
end:

    if (brk | ret) {
        sprintf(mrs->log, "E,%d,%d", ret, brk);
    } else {
        sprintf(mrs->log, "D,%d,%d", ret, brk);
    }

    n = strlen(mrs->log);
    print_dbg(&mrs->plog, mrs->log, n);
    printf_dbgflush(&mrs->plog, mrs);

    return ret;
}

static int cmdfunc_free_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0;
    int ret=0, ix=0, n=0, brk=0;
    struct aspWaitRlt_s *pwt;
    struct info16Bit_s *pkt;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_free_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);

    pkt = &mrs->mchine.tmp;
    pwt = &mrs->wtg;
    if (!pkt) {ret = -2; goto end;}
    if (!pwt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}

    /* set wait result mechanism */
    pwt->wtChan = 6;
    pwt->wtMs = 300;

    n = 0; rsp = 0;
    /* set data for update to scanner */
    pkt->opcode = OP_SINGLE;
    pkt->data = SINSCAN_WIFI_ONLY;
    n = cmdfunc_upd2host(mrs, 'c', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }
        
    if ((n) && (rsp != 0x1)) {
         sprintf(mrs->log, "ERROR!!, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
         print_f(&mrs->plog, "DBG", mrs->log);
    }

    sprintf(mrs->log, "cmdfunc_upld_opcode n = %d, rsp = %d\n", n, rsp); 
    print_f(&mrs->plog, "DBG", mrs->log);
    
end:

    if (brk | ret) {
        sprintf(mrs->log, "E,%d,%d", ret, brk);
    } else {
        sprintf(mrs->log, "D,%d,%d", ret, brk);
    }

    n = strlen(mrs->log);
    print_dbg(&mrs->plog, mrs->log, n);
    printf_dbgflush(&mrs->plog, mrs);

    return ret;
}

static int cmdfunc_used_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0;
    int ret=0, ix=0, n=0, brk=0;
    struct aspWaitRlt_s *pwt;
    struct info16Bit_s *pkt;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_used_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);

    pkt = &mrs->mchine.tmp;
    pwt = &mrs->wtg;
    if (!pkt) {ret = -2; goto end;}
    if (!pwt) {ret = -3; goto end;}
    rlt = pwt->wtRlt;
    if (!rlt) {ret = -4; goto end;}

    /* set wait result mechanism */
    pwt->wtChan = 6;
    pwt->wtMs = 300;

    n = 0; rsp = 0;
    /* set data for update to scanner */
    pkt->opcode = OP_SINGLE;
    pkt->data = SINSCAN_WIFI_ONLY;
    n = cmdfunc_upd2host(mrs, 'k', &rsp);
    if ((n == -32) || (n == -33)) {
        brk = 1;
        goto end;
    }
        
    if ((n) && (rsp != 0x1)) {
         sprintf(mrs->log, "ERROR!!, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
         print_f(&mrs->plog, "DBG", mrs->log);
    }

    sprintf(mrs->log, "cmdfunc_upld_opcode n = %d, rsp = %d\n", n, rsp); 
    print_f(&mrs->plog, "DBG", mrs->log);
    
end:

    if (brk | ret) {
        sprintf(mrs->log, "E,%d,%d", ret, brk);
    } else {
        sprintf(mrs->log, "D,%d,%d", ret, brk);
    }

    n = strlen(mrs->log);
    print_dbg(&mrs->plog, mrs->log, n);
    printf_dbgflush(&mrs->plog, mrs);

    return ret;
}

static int cmdfunc_opchk_single(uint32_t val, uint32_t mask, int len, int type)
{
    int cnt=0, s=0;
    if (val > mask) return -1;
    if (len > 32) return -2;
    if (!len) return -3;

    if (type != ASPOP_TYPE_SINGLE) {
        if (val > mask) {
            return -4;
        }
        return 1;
    }
    
    s = 0;
    while(s < len) {
        if (val & (0x1 << s)) cnt++;
        s++;
    }

    if (cnt == 0) return -5;
    if (cnt > 1) return -6;

    return 2;
}
static int cmdfunc_opcode(int argc, char *argv[])
{
    int val=0;
    int n=0, ix=0, ret=0;
    char ch=0, opcode[5], param=0;
    uint32_t tg=0, cd=0;
    struct mainRes_s *mrs=0;
    mrs = (struct mainRes_s *)argv[0];
    if (!mrs) {ret = -1; goto end;}
    sprintf(mrs->log, "cmdfunc_opcode argc:%d\n", argc); 
    print_f(&mrs->plog, "DBG", mrs->log);
    /* get opcode and parameter */
    ch = 'o';
    mrs_ipc_put(mrs, &ch, 1, 5);

    n = -1;
    while (n == -1) {
        n = mrs_ipc_get(mrs, opcode, 5, 5);
        //sprintf(mrs->log, "n:%d\n", n); 
        //print_f(&mrs->plog, "DBG", mrs->log);
    }

    /* debug print */ 
    /*
    for (ix = 0; ix < n; ix++) {
        sprintf(mrs->log, "%d.%.2x\n", ix, opcode[ix]); 
        print_f(&mrs->plog, "DBG", mrs->log);
    }
    */

    if (n != 5) {ret = -2; goto end;}
    if ((opcode[0] != 0xaa) && (opcode[0] != 0xad)) {ret = -3; goto end;}
    if (opcode[4] != 0xa5) {ret = -4; goto end;}
    if (opcode[2] != '/') {ret = -5; goto end;}

    tg = opcode[1];
    struct aspConfig_s* ctb = 0;
    for (ix = 0; ix < ASPOP_CODE_MAX; ix++) {
        ctb = &mrs->configTable[ix];
        if (tg == ctb->opCode) {
        
            //sprintf(mrs->log, "found!!! [%d] [0x%x] [0x%x] [0x%x] \n", ix, ctb->opCode, ctb->opValue, ctb->opStatus); 
            //print_f(&mrs->plog, "DBG", mrs->log);

            break;
        }
        ctb = 0;
    }

    if (!ctb) {
        sprintf(mrs->log, "Error!!! cmdfunc_opcode - 3\n"); 
        print_f(&mrs->plog, "DBG", mrs->log);
        ret = -7;
        goto end;
    }

    if (opcode[0] == 0xaa) {
        cd = opcode[3];

        if (cd == ctb->opValue) {
            ctb->opStatus |= ASPOP_STA_CON;
            param = ctb->opValue;
            goto end;
        }

        ret = cmdfunc_opchk_single(cd, ctb->opMask, ctb->opBitlen, ctb->opType);
        if (ret < 0) {
            ret = (ret * 10) -6;
        } else {            
            ctb->opValue = cd;
            ctb->opStatus |= ASPOP_STA_CON;
        }
        
        sprintf(mrs->log, "WT opcode 0x%.2x/0x%.2x, input value: 0x%.2x, ret:%d\n", ctb->opCode, ctb->opValue, cd, ret); 
        print_f(&mrs->plog, "DBG", mrs->log);
    } else {
        sprintf(mrs->log, "RD opcode 0x%.2x/0x%.2x, input value: 0x%.2x, ret:%d\n", ctb->opCode, ctb->opValue, cd, ret); 
        print_f(&mrs->plog, "DBG", mrs->log);
    }

    param = ctb->opValue;

end:

    if (ret < 0) {
        ch = 'x';
        mrs_ipc_put(mrs, &ch, 1, 5);
        ch = 0xf0;
        mrs_ipc_put(mrs, &ch, 1, 5);

        if (!ctb) {
            sprintf(mrs->log, "failed: input 0x%x/0x%x, ret:%d", opcode[1], opcode[3], ret); 
        } else {
            sprintf(mrs->log, "failed: input 0x%x/0x%x, mask:0x%x, bitlen:%d, ret:%d", opcode[1], opcode[3], ctb->opMask, ctb->opBitlen, ret); 
        }

    } else {
        if (ret == 0) {
            ch = 'p';
            mrs_ipc_put(mrs, &ch, 1, 5);

            sprintf(mrs->log, "same: result 0x%x/0x%x, ret: %d", ctb->opCode, ctb->opValue, ret);   
        } else {
            ch = 'p';
            mrs_ipc_put(mrs, &ch, 1, 5);

            sprintf(mrs->log, "succeed: result 0x%x/0x%x, ret: %d", ctb->opCode, ctb->opValue, ret);    
        } 
        mrs_ipc_put(mrs, &param, 1, 5);     
    }
    n = strlen(mrs->log);
    mrs_ipc_put(mrs, mrs->log, n, 5);
    
    sprintf(mrs->log, "opcode op end, log n =%d, ret=%d\n", n, ret); 
    print_f(&mrs->plog, "DBG", mrs->log);
    
    return 0;
}

static int cmdfunc_01(int argc, char *argv[])
{
    struct mainRes_s *mrs;
    char str[256], ch;
    if (!argv) return -1;

    mrs = (struct mainRes_s *)argv[0];

    ch = '=';

    if (argc == 7) {
        ch = 'd';
    }

    else if (argc == 6) {
        ch = 'r';
    }
    
    else if (argc == 0) {
        ch = 'p';
    }

    else if (argc == 5) {
        ch = 'n';
    }

    sprintf(str, "cmdfunc_01 argc:%d ch:%c\n", argc, ch); 
    print_f(mlogPool, "DBG", str);

    mrs_ipc_put(mrs, &ch, 1, 6);
    return 1;
}

static int dbg(struct mainRes_s *mrs)
{
#define CMD_SIZE 29

    int ci, pi, ret, idle=0, wait=-1, loglen=0;
    char cmd[256], *addr[3], rsp[256], ch, *plog;
    char poll[32] = "poll";

    struct cmd_s cmdtab[CMD_SIZE] = {{0, "poll", cmdfunc_01}, {1, "action", cmdfunc_act_opcode}, {2, "rgw", cmdfunc_regw_opcode}, {3, "op", cmdfunc_opcode}, 
                                {4, "wt", cmdfunc_wt_opcode}, {5, "go", cmdfunc_go_opcode}, {6, "rgr", cmdfunc_regr_opcode}, {7, "launch", cmdfunc_lh_opcode},
                                {8, "boot", cmdfunc_boot_opcode}, {9, "single", cmdfunc_single_opcode}, {10, "dnld", cmdfunc_dnld_opcode}, {11, "upld", cmdfunc_upld_opcode},
                                {12, "save", cmdfunc_save_opcode}, {13, "free", cmdfunc_free_opcode}, {14, "used", cmdfunc_used_opcode}, {15, "op1", cmdfunc_op1_opcode}
                                , {16, "op2", cmdfunc_op2_opcode}, {17, "op3", cmdfunc_op3_opcode}, {18, "op4", cmdfunc_op4_opcode}, {19, "op5", cmdfunc_op5_opcode}
                                , {20, "sdon", cmdfunc_sdon_opcode}, {21, "wfisd", cmdfunc_wfisd_opcode}, {22, "dulsd", cmdfunc_dulsd_opcode}, {23, "tgr", cmdfunc_tgr_opcode}
                                , {24, "crop", cmdfunc_crop_opcode}, {25, "vec", cmdfunc_vector_opcode}, {26, "apm", cmdfunc_apm_opcode}, {27, "meta", cmdfunc_meta_opcode}
                                , {28, "scango", cmdfunc_scango_opcode}};

    p0_init(mrs);

    plog = aspMalloc(2048);
    if (!plog) {
        sprintf(mrs->log, "DBG plog alloc failed! \n");
        print_f(&mrs->plog, "DBG", mrs->log);
        return (-1);
    }

    while (1) {
        /* command parsing */
        ci = 0;    
        ci = mrs_ipc_get(mrs, cmd, 256, 5);

        if (ci > 0) {
            cmd[ci] = '\0';
            //sprintf(mrs->log, "get [%s] size:%d \n", cmd, ci);
            //print_f(&mrs->plog, "DBG", mrs->log);
        } else {
            if (idle > 100) {
                idle = 0;
                //strcpy(cmd, poll);
            } else {
                idle ++;
                printf_flush(&mrs->plog, mrs->flog);
                usleep(1000);
                continue;
            }
        }

        pi = 0;
        while (pi < CMD_SIZE) {
            if ((strlen(cmd) == strlen(cmdtab[pi].str))) {
                if (!strncmp(cmd, cmdtab[pi].str, strlen(cmdtab[pi].str))) {
                    break;
                }
            }
            pi++;
        }
#if 0 /* no longer dumy log */
        /* clear previous log */
        ret = 0;
        ret = mrs_ipc_get(mrs, rsp, 256, 6);
        while (ret > 0) {
            sprintf(mrs->log, "ret:%d, rsp:%s\n", ret, rsp);
            print_f(&mrs->plog, "DBG", mrs->log);
/*
            mrs_ipc_put(mrs, rsp, ret, 5);
*/
            ret = 0;
            ret = mrs_ipc_get(mrs, rsp, 256, 6);
        }
#endif

        /* command execution */
        if (ci > 0) {
            if (pi < CMD_SIZE) {
                addr[0] = (char *)mrs;
                sprintf(mrs->log, "input [%d]%s\n", pi, cmdtab[pi].str);
                print_f(&mrs->plog, "DBG", mrs->log);
                ret = cmdtab[pi].pfunc(cmdtab[pi].id, addr);
                wait = 1;
                memset(plog, 0, 2048);
                loglen = 0;
                memset(cmd, 0, 256);
            } else {
                mrs_ipc_put(mrs, "?", 1, 5); 
                continue;
            }
        } else {
            if (wait < 0) continue;
        }

        ch = 0;
        ret = mrs_ipc_get(mrs, &ch, 1, 6);
        while (ret > 0) {

            if (loglen > 0) {
                plog[loglen] = ch;
                loglen++;
                if ((ch == '>') || (loglen == 2048)) {

                    mrs_ipc_put(mrs, plog, loglen, 5);
                    sprintf(mrs->log, "\"%s\"", plog);
                    print_f(&mrs->plog, "DBG", mrs->log);

                    wait = -1;
                }
            } else {
                if (ch == '<') {
                    plog[loglen] = ch;
                    loglen++;
                }
            }
            ch = 0;
            ret = mrs_ipc_get(mrs, &ch, 1, 6);
        }

        wait ++;
        if (wait > 1000000) {
            sprintf(mrs->log, "command time out :%d, loglen: %d\n", wait, loglen);
            print_f(&mrs->plog, "DBG", mrs->log);
            if (loglen > 0) {

                mrs_ipc_put(mrs, plog, loglen, 5);

            }
            wait = -1;
        }

        printf_flush(&mrs->plog, mrs->flog);
    }

    p0_end(mrs);
}

static int hd00(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd01(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd02(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd03(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd04(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd05(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd06(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd07(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd08(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd09(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd10(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd11(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd12(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd13(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd14(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd15(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd16(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd17(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd18(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd19(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd20(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd21(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd22(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd23(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd24(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd25(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd26(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd27(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd28(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd29(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd30(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd31(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd32(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd33(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd34(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd35(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd36(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd37(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd38(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd39(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd40(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd41(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd42(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int ret=0, bitset;
    bitset = 0;
    ret = msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "set sp0 ctl pin=%d ret=%d\n", bitset, ret);
    print_f(&mrs->plog, "ERROR42", mrs->log);

    return 0;
}
static int hd43(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd44(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int ret=0, bitset;
    bitset = 0;
    ret = msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "set sp0 ctl pin=%d ret=%d\n", bitset, ret);
    print_f(&mrs->plog, "ERROR44", mrs->log);

    return 0;
}
static int hd45(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd46(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd47(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd48(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd49(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd50(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd51(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd52(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd53(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd54(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd55(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd56(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd57(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd58(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd59(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd60(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd61(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd62(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd63(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd64(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd65(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd66(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd67(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd68(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd69(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd70(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd71(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd72(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd73(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd74(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd75(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd76(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd77(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd78(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd79(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd80(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd81(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd82(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd83(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd84(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd85(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd86(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd87(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd88(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd89(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd90(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd91(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd92(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd93(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd94(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd95(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd96(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd97(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd98(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd99(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd100(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd101(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd102(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd103(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd104(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd105(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd106(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd107(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd108(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd109(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd110(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd111(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd112(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd113(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int hd114(struct mainRes_s *mrs, struct modersp_s *modersp){return 0;}
static int fs00(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p;
    p = &mrs->mchine.cur;

    sprintf(mrs->log, "usleep(%d) %d -> %d \n", modersp->v, modersp->m, modersp->d);
    print_f(&mrs->plog, "fs00", mrs->log);

    usleep(modersp->v);
    modersp->m = modersp->d;
    modersp->d = 0;

    if (modersp->m == -1) {
        modersp->r = 1;
        return 1;
    }
    
    return 0; 
}
static int fs01(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    sprintf(mrs->log, "check RDY high \n");
    print_f(&mrs->plog, "fs01", mrs->log);
    mrs_ipc_put(mrs, "b", 1, 1);
    modersp->c = 0;
    modersp->m = modersp->m + 1;
    return 0; 
}
static int fs02(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    char ch;
    int len;
    //sprintf(mrs->log, "check RDY high d:%d, c:%d\n", modersp->d, modersp->c);
    //print_f(&mrs->plog, "fs02", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if (len > 0){
        if (ch == 'B') {
        //modersp->m = modersp->m + 1;
            if (modersp->d) {
                modersp->m = modersp->d;
                modersp->d = 0;
                return 2;
            } else {
                modersp->r = 1;
                return 1;
            }
        } else {
            if (modersp->c < 1) { 
                mrs_ipc_put(mrs, "b", 1, 1);
                modersp->c += 1;
            } else {
                modersp->c = 0;
                modersp->r = 2;
                return 1;
            }
        }
    }
    return 0; 
}
static int fs03(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p;
    p = &mrs->mchine.cur;
    sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    print_f(&mrs->plog, "fs03", mrs->log);

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = OP_PON;
    p->inout = 0;
    p->seqnum = mrs->mchine.seqcnt;
    
    mrs_ipc_put(mrs, "c", 1, 1);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs04(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0, bitset=0;
    char ch=0;
    struct info16Bit_s *p;

    //sprintf(mrs->log, "enter \n");
    //print_f(&mrs->plog, "fs04", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        //print_f(&mrs->plog, "fs04", mrs->log);

        if (p->opcode == OP_PON) {
            //modersp->m = modersp->m + 1;   
            modersp->r = 1;
            return 1;
        } else {
            modersp->m = 0;
            modersp->d = 4;
            modersp->v = 1000000;
            return 2;
        }
    }
    return 0; 
}

static int fs05(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p;
    p = &mrs->mchine.cur;
    int bitset;
    sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    print_f(&mrs->plog, "fs05", mrs->log);
    bitset = 1;
    msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 0 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs05", mrs->log);
    bitset = 1;
    msp_spi_conf(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 1 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs05",mrs->log);

    bitset = 0;
    msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",0, bitset, modersp->d);
    print_f(&mrs->plog, "fs05", mrs->log);

    bitset = 0;
    msp_spi_conf(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",1, bitset, modersp->d);
    print_f(&mrs->plog, "fs05", mrs->log);

    modersp->r = 1;
    return 1; 
}
static int fs06(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    struct info16Bit_s *p;
    p = &mrs->mchine.cur;
    sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    print_f(&mrs->plog, "fs06", mrs->log);

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = OP_RDY;
    p->inout = 0;
    p->seqnum = mrs->mchine.seqcnt;
    
    mrs_ipc_put(mrs, "c", 1, 1);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs07(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        //print_f(&mrs->plog, "fs07", mrs->log);

        if (p->opcode == OP_RDY) {
            modersp->r = 1;
            return 1;
        } else {
            modersp->m = modersp->m - 1;        
            return 2;
        }
    }
    
    if ((len > 0) && (ch == 'X')) {
            sprintf(mrs->log, "FAIL!!send command again!\n");
            print_f(&mrs->plog, "fs07", mrs->log);
            modersp->m = modersp->m - 1;        
            return 2;
    }
    return 0; 
}

static int fs08(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    struct info16Bit_s *p;
    p = &mrs->mchine.cur;
    sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    print_f(&mrs->plog, "fs08", mrs->log);

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = OP_RDY;
    p->inout = 0;
    p->seqnum = mrs->mchine.seqcnt;
    
    mrs_ipc_put(mrs, "c", 1, 1);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs09(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        //print_f(&mrs->plog, "fs09", mrs->log);

        if (p->opcode == OP_QRY) {
            modersp->d = modersp->m - 1;        
            modersp->m = 0;
            modersp->v = 1000000;
            return 2;
        } else {
            /* must be something wrong */
            sprintf(mrs->log, "fs09: did not receive QRY but 0x%x", p->opcode);
            error_handle(mrs->log, 1967);
        }
    }

    if ((len > 0) && (ch == 'X')) {
            sprintf(mrs->log, "FAIL!!send command again!\n");
            print_f(&mrs->plog, "fs09", mrs->log);
            modersp->m = modersp->m - 1;        
            return 2;
    }

    return 0; 
}

static int fs10(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    sprintf(mrs->log, "check socket status\n");
    print_f(&mrs->plog, "fs10", mrs->log);
    
    mrs_ipc_put(mrs, "r", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs11(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    //sprintf(mrs->log, "wait socket status\n");
    //print_f(&mrs->plog, "fs11", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if (len > 0) {

        sprintf(mrs->log, "wait socket status ch: %c - end\n", ch);
        print_f(&mrs->plog, "fs11", mrs->log);

        if (ch == 'R') {
            modersp->r = 1;
            return 1;
        } else if (ch == 'r') {
            error_handle("socket error", 3007);
            modersp->r = 2;
            return 1;
        } else  {
            modersp->r = 3;
         return 0;
        }
    }
    return 0; 
}

static int fs12(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    struct info16Bit_s *p;
    struct info16Bit_s *t;
    p = &mrs->mchine.cur;
    t = &mrs->mchine.tmp;

    p->opcode = t->opcode;
    p->data = t->data;
    
    //sprintf(mrs->log, "set %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    //print_f(&mrs->plog, "fs12", mrs->log);
    
    mrs_ipc_put(mrs, "c", 1, 1);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs13(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        //print_f(&mrs->plog, "fs13", mrs->log);

        if (p->opcode == OP_QRY) {
            modersp->r = 1;
            return 1;
        } else {
            modersp->m = modersp->m - 1;        
            return 2;
        }
    }
    return 0; 
}

static int fs14(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    struct info16Bit_s *p;
    struct info16Bit_s *t;
    p = &mrs->mchine.cur;
    t = &mrs->mchine.tmp;

    p->opcode = t->opcode;
    p->data = t->data;

    //sprintf(mrs->log, "set %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    //print_f(&mrs->plog, "fs14", mrs->log);
    
    mrs_ipc_put(mrs, "c", 1, 1);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs15(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0, val=0, ret=0;
    char ch=0;
    struct info16Bit_s *p;
    struct aspConfig_s *pct=0;
    pct = mrs->configTable;

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        //print_f(&mrs->plog, "fs15", mrs->log);

        if ((p->opcode == OP_SINGLE) && ((p->data == SINSCAN_DUAL_STRM) 
            ||(p->data == SINSCAN_DUAL_SD))) {
            modersp->m = modersp->m + 1;
            return 2;
        }
        #if 0 /* not a good flow */
        else if ((p->opcode == OP_SINGLE) && (p->data == SINSCAN_DUAL_SD)) {
            modersp->m = modersp->m + 1;
        
            ret = cfgTableGet(pct, ASPOP_SUP_SAVE, &val);
            if (ret) {
                sprintf(mrs->log, "ASPOP_SUP_SAVE not available, ret:%d\n", ret);  
                print_f(&mrs->plog, "fs15", mrs->log);
            } else {
                sprintf(mrs->log, "ASPOP_SUP_SAVE value: 0x%x\n", val);  
                print_f(&mrs->plog, "fs15", mrs->log);

                switch (val) {
                    case SUPBACK_RAW:
                        break;
                    case SUPBACK_FAT:
                        break;
                    case SUPBACK_SD:
                        modersp->d = modersp->m + 1;
                        modersp->m = 59;
                        break;
                    default:
                        sprintf(mrs->log, "WARNING!!! unexpected OP_SUPBACK value: 0x%x \n", val);  
                        print_f(&mrs->plog, "fs15", mrs->log);
                        break;
                }
            }

            return 2;
        } 
        #endif
        else if (p->opcode == OP_SUPBACK) { // flow changed, will be removed in future
            modersp->d = modersp->m + 1;
            modersp->m = 57;
            return 2;
        } else {
            modersp->m = modersp->m - 1;        
            return 2;
        }
    }
    return 0; 
}

static int fs16(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset=0, ret;
    bitset = 0;
    msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi0 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs16", mrs->log);
    bitset = 0;
    msp_spi_conf(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi1 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs16", mrs->log);

/* bullet don't need kthread mode */
#if SPI_KTHREAD_USE 
    bitset = 0;
    ret = msp_spi_conf(mrs->sfm[0], _IOR(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_START_THREAD
    sprintf(mrs->log, "Start spi0 spidev thread, ret: 0x%x\n", ret);
    print_f(&mrs->plog, "fs16", mrs->log);

    bitset = 0;
    ret = msp_spi_conf(mrs->sfm[1], _IOR(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_START_THREAD
    sprintf(mrs->log, "Start spi1 spidev thread, ret: 0x%x\n", ret);
    print_f(&mrs->plog, "fs16", mrs->log);
#endif

    modersp->r = 1;
    return 1;
}

static int fs17(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    sprintf(mrs->log, "trigger spi0 spi1 \n");
    print_f(&mrs->plog, "fs17", mrs->log);


    mrs_ipc_put(mrs, "d", 1, 2);
    clock_gettime(CLOCK_REALTIME, &mrs->time[1]);
    usleep(100);
    mrs_ipc_put(mrs, "d", 1, 1);
    clock_gettime(CLOCK_REALTIME, &mrs->time[0]);

    modersp->m = modersp->m + 1;
    modersp->v = 0;
    modersp->c = 0;
    return 2;
}

static int fs18(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct sdFAT_s *pfat=0;
    struct supdataBack_s *s=0, *sc=0;

    int len=0;
    char *addr = 0, *dst=0;
    int ret, bitset;
    char ch;

    //sprintf(mrs->log, "%d\n", modersp->v);
    //print_f(&mrs->plog, "fs18", mrs->log);
    pfat = &mrs->aspFat;
    sc = pfat->fatSupcur;

    while (1) {
    if (!(modersp->c % 2)) {
    ret = mrs_ipc_get(mrs, &ch, 1, 1);
    if (ret > 0) {
        if (ch == 'p') {
            if (sc) {
                len = ring_buf_cons_dual_psudo(&mrs->dataRx, &addr, modersp->v);
                //sprintf(mrs->log, "1. get psudo len:%d, cnt:%d\n", len, modersp->v);
                //print_f(&mrs->plog, "fs18", mrs->log);

                if (len >= 0) {
                    dst = sc->supdataBuff;
                    memcpy(dst, addr, len);
                    sc->supdataTot = len;

                    s = aspMalloc(sizeof(struct supdataBack_s));
                    memset(s, 0, sizeof(struct supdataBack_s));
                    sc->n = s;
                    sc = sc->n;

                    modersp->v += 1;                    
                    pfat->fatSupcur = sc;
                }
            }
            
            mrs_ipc_put(mrs, "d", 1, 3);
        }

        if (ch == 'd') {
            sprintf(mrs->log, "0 %d end\n", modersp->v);
            print_f(&mrs->plog, "fs18", mrs->log);

            mrs_ipc_put(mrs, "d", 1, 3);
            modersp->r |= 0x1;
            //mrs_ipc_put(mrs, "e", 1, 3);
        }
        //ret = mrs_ipc_get(mrs, &ch, 1, 1);
        modersp->c += 1;
    }else {
        break;
    }
    }

    if (modersp->c % 2) {    
    ret = mrs_ipc_get(mrs, &ch, 1, 2);
    if (ret > 0) {
        if (ch == 'p') {
            if (sc) {
                len = ring_buf_cons_dual_psudo(&mrs->dataRx, &addr, modersp->v);
                //sprintf(mrs->log, "2. get psudo len:%d, cnt:%d\n", len, modersp->v);
                //print_f(&mrs->plog, "fs18", mrs->log);

                if (len >= 0) {
                    dst = sc->supdataBuff;
                    memcpy(dst, addr, len);
                    sc->supdataTot = len;

                    s = aspMalloc(sizeof(struct supdataBack_s));
                    memset(s, 0, sizeof(struct supdataBack_s));
                    sc->n = s;
                    sc = sc->n;

                    modersp->v += 1;  
                    pfat->fatSupcur = sc;
                }
            }
            mrs_ipc_put(mrs, "d", 1, 3);
        }

        if (ch == 'd') {
            sprintf(mrs->log, "1 %d end\n", modersp->v);
            print_f(&mrs->plog, "fs18", mrs->log);
 
            mrs_ipc_put(mrs, "d", 1, 3);
            modersp->r |= 0x2;
            //mrs_ipc_put(mrs, "e", 1, 3);
        }
        //ret = mrs_ipc_get(mrs, &ch, 1, 2);
        modersp->c += 1;
    }else {
        break;
    }
    }
    }

    if (modersp->r == 0x3) {
        if (sc) {
            len = ring_buf_cons_dual_psudo(&mrs->dataRx, &addr, modersp->v);
            while (len >= 0) {
                sprintf(mrs->log, "3. get psudo len:%d, cnt:%d\n", len, modersp->v);
                print_f(&mrs->plog, "fs18", mrs->log);

                dst = sc->supdataBuff;
                memcpy(dst, addr, len);
                sc->supdataTot = len;
                
                s = aspMalloc(sizeof(struct supdataBack_s));
                memset(s, 0, sizeof(struct supdataBack_s));
                sc->n = s;
                sc = sc->n;

                pfat->fatSupcur = sc;
                modersp->v += 1;  
                len = ring_buf_cons_dual_psudo(&mrs->dataRx, &addr, modersp->v);
            }

            len = 0;
            s = pfat->fatSupdata;
            while (s) {
                if (s->supdataTot == 0) {
                    break;
                } else {
                    len += s->supdataTot;
                }
                sc = s;
                s = s->n;
            }

            if (sc) {
                sc->n = 0;
            }

            while (s) {
                sc = s;
                s = s->n;
                
                memset(sc, 0, sizeof(struct supdataBack_s));
                aspFree(sc);
            }
            pfat->fatSupcur = 0;
        }
        mrs_ipc_put(mrs, "D", 1, 3);
        sprintf(mrs->log, "%d end, len: %d, cnt:%d\n", modersp->v, len,modersp->c);
        print_f(&mrs->plog, "fs18", mrs->log);

        mrs->mchine.cur.opinfo = modersp->v;
        
        modersp->m = modersp->m + 1;
        return 2;
    }

    return 0;
}

static int fs19(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int len=0;
    char ch=0;
    //sprintf(mrs->log, "wait socket finish \n");
    //print_f(&mrs->plog, "fs19", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'D')) {
        ring_buf_init(&mrs->dataRx);
        mrs->dataRx.r->folw.seq = 1;
        mrs->dataRx.r->psudo.seq = 1;

        modersp->r = 1;;
        return 1;
    }

    return 0;
}

static int fs20(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int bitset, ret;

    bitset = 0;
    msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",0, bitset, modersp->d);
    print_f(&mrs->plog, "fs20", mrs->log);

    bitset = 0;
    msp_spi_conf(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",1, bitset, modersp->d);
    print_f(&mrs->plog, "fs20", mrs->log);

    usleep(100000);
            
    bitset = 0;
    msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi0 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs20", mrs->log);

    bitset = 0;
    msp_spi_conf(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi1 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs20", mrs->log);

    bitset = 1;
    msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 0 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs20", mrs->log);

    bitset = 1;
    msp_spi_conf(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 1 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs20", mrs->log);
    
#if SPI_KTHREAD_USE
    bitset = 0;
    ret = msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
    sprintf(mrs->log, "Stop spi0 spidev thread, ret: 0x%x\n", ret);
    print_f(&mrs->plog, "fs20", mrs->log);

    bitset = 0;
    ret = msp_spi_conf(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
    sprintf(mrs->log, "Stop spi1 spidev thread, ret: 0x%x\n", ret);
    print_f(&mrs->plog, "fs20", mrs->log);
#endif

    modersp->r = 1;
    return 1;
}
static int fs21(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    struct info16Bit_s *p;

    sprintf(mrs->log, "get OP_FIH \n");
    print_f(&mrs->plog, "fs21", mrs->log);

    p = &mrs->mchine.cur;

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = OP_FIH;
    p->inout = 0;
    p->seqnum = mrs->mchine.seqcnt;
    
    mrs_ipc_put(mrs, "c", 1, 1);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs22(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        //print_f(&mrs->plog, "fs22", mrs->log);

        if (p->opcode == OP_FIH) {
            modersp->m = modersp->m + 1; 
        } else {
            error_handle("get FIH failed", 2238);
        }
    }
    
    if ((len > 0) && (ch == 'X')) {
            sprintf(mrs->log, "FAIL!!send command again!\n");
            print_f(&mrs->plog, "fs22", mrs->log);
            modersp->m = modersp->m - 1;        
            return 2;
    }

    return 0; 
}

static int fs23(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset;
    bitset = 1;
    msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY

    sprintf(mrs->log, "Set spi 0 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs23", mrs->log);

    bitset = 1;
    msp_spi_conf(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY

    sprintf(mrs->log, "Set spi 1 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs23", mrs->log);

    modersp->r = 1;
    return 1;
}

static int fs24(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset;
    usleep(1);

    bitset = 1;
    msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",0, bitset, modersp->d);
    print_f(&mrs->plog, "fs24", mrs->log);

    bitset = 1;
    msp_spi_conf(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",1, bitset, modersp->d);
    print_f(&mrs->plog, "fs24", mrs->log);

    usleep(1000);

    bitset = 0;
    msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",0, bitset, modersp->d);
    print_f(&mrs->plog, "fs24", mrs->log);

    bitset = 0;
    msp_spi_conf(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",1, bitset, modersp->d);
    print_f(&mrs->plog, "fs24", mrs->log);

    sleep(1);

    bitset = 1;
    msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",0, bitset, modersp->d);
    print_f(&mrs->plog, "fs24", mrs->log);

    bitset = 1;
    msp_spi_conf(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",1, bitset, modersp->d);
    print_f(&mrs->plog, "fs24", mrs->log);

    usleep(1000);

    bitset = 0;
    msp_spi_conf(mrs->sfm[0], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
    sprintf(mrs->log, "[%d]Get RDY pin %d, cnt:%d\n",0, bitset, modersp->d);
    print_f(&mrs->plog, "fs24", mrs->log);

    bitset = 0;
    msp_spi_conf(mrs->sfm[1], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
    sprintf(mrs->log, "[%d]Get RDY pin %d, cnt:%d\n",1, bitset, modersp->d);
    print_f(&mrs->plog, "fs24", mrs->log);

    bitset = 0;
    msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 0 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs24", mrs->log);

    bitset = 0;
    msp_spi_conf(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 1 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs24", mrs->log);

    return 1;
}

static int fs25(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    sprintf(mrs->log, "check 01 socket status\n");
    print_f(&mrs->plog, "fs25", mrs->log);
    
    mrs_ipc_put(mrs, "r", 1, 3);

    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs26(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    //sprintf(mrs->log, "wait socket status\n");
    //print_f(&mrs->plog, "fs26", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if (len > 0) {

        sprintf(mrs->log, "wait 01 socket status ch: %c - end\n", ch);
        print_f(&mrs->plog, "fs26", mrs->log);

        if (ch == 'R') {
            modersp->r = 1;
            return 1;
        } else if (ch == 'r') {
            error_handle("socket error", 3421);
            modersp->r = 2;
            return 1;
        } else  {
            modersp->r = 3;
         return 0;
        }
    }
    return 0; 
}

static int fs27(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    sprintf(mrs->log, "check 02 socket status\n");
    print_f(&mrs->plog, "fs27", mrs->log);
    
    mrs_ipc_put(mrs, "r", 1, 8);

    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs28(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    //sprintf(mrs->log, "wait socket status\n");
    //print_f(&mrs->plog, "fs28", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 8);
    if (len > 0) {

        sprintf(mrs->log, "wait 02 socket status ch: %c - end\n", ch);
        print_f(&mrs->plog, "fs28", mrs->log);

        if (ch == 'R') {
            modersp->r = 1;
            return 1;
        } else if (ch == 'r') {
            error_handle("socket error", 3463);
            modersp->r = 2;
            return 1;
        } else  {
            modersp->r = 3;
         return 0;
        }
    }
    return 0; 
}

static int fs29(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    struct info16Bit_s *p;
    p = &mrs->mchine.cur;

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = OP_DOUBLE;
    p->data = DOUSCAN_WIFI_ONLY;
    p->inout = 0;
    p->seqnum = mrs->mchine.seqcnt;

    //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    //print_f(&mrs->plog, "fs29", mrs->log);
    
    mrs_ipc_put(mrs, "c", 1, 1);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs30(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        //print_f(&mrs->plog, "fs30", mrs->log);

        if (p->opcode == OP_QRY) {
            modersp->r = 1;
            return 1;
        } else {
            modersp->m = modersp->m - 1;        
            return 2;
        }
    }
    return 0; 
}

static int fs31(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    struct info16Bit_s *p;
    p = &mrs->mchine.cur;

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = OP_DOUBLE;
    p->data = DOUSCAN_WIFI_ONLY;
    p->inout = 0;
    p->seqnum = mrs->mchine.seqcnt;

    //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    //print_f(&mrs->plog, "fs31", mrs->log);
    
    mrs_ipc_put(mrs, "c", 1, 1);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs32(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        //print_f(&mrs->plog, "fs32", mrs->log);

        if ((p->opcode == OP_DOUBLE) && (p->data == DOUSCAN_WIFI_ONLY)) {
            modersp->m = modersp->m + 1;
            return 2;
        } else {
            modersp->m = modersp->m - 1;        
            return 2;
        }
    }
    return 0; 
}

static int fs33(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset=0, ret;
    bitset = 0;
    msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi0 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs33", mrs->log);
    bitset = 0;
    msp_spi_conf(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi1 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs33", mrs->log);

#if SPI_KTHREAD_USE
    bitset = 0;
    ret = msp_spi_conf(mrs->sfm[0], _IOR(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_START_THREAD
    sprintf(mrs->log, "Start spi0 spidev thread, ret: 0x%x\n", ret);
    print_f(&mrs->plog, "fs33", mrs->log);

    bitset = 0;
    ret = msp_spi_conf(mrs->sfm[1], _IOR(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_START_THREAD
    sprintf(mrs->log, "Start spi1 spidev thread, ret: 0x%x\n", ret);
    print_f(&mrs->plog, "fs33", mrs->log);
#endif

    modersp->r = 1;
    return 1;
}

static int fs34(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    sprintf(mrs->log, "trigger spi0 spi1 \n");
    print_f(&mrs->plog, "fs34", mrs->log);


    mrs_ipc_put(mrs, "n", 1, 2);
    clock_gettime(CLOCK_REALTIME, &mrs->time[1]);
    usleep(100);
    mrs_ipc_put(mrs, "n", 1, 1);
    clock_gettime(CLOCK_REALTIME, &mrs->time[0]);

    modersp->m = modersp->m + 1;
    modersp->v = 0;
    modersp->c = 0;
    return 2;
}

static int fs35(struct mainRes_s *mrs, struct modersp_s *modersp)
{
#define QSIZE 5
#define QDELAY 0
    int ret, bitset, tmc;
    char ch;

    //sprintf(mrs->log, "%d\n", modersp->v);
    //print_f(&mrs->plog, "fs35", mrs->log);

    if (modersp->r & 0x4) {

        if (modersp->r & 0x1) {
            modersp->r &= ~(0x04);
        } 
        else if (modersp->c) {
            ret = mrs_ipc_get(mrs, &ch, 1, 3);
            while (ret > 0) {
                if (ch == 'n') {
                    modersp->c -= 1;
                    sprintf(mrs->log, "WiFi _0_ rest - %d\n", modersp->c);
                    print_f(&mrs->plog, "fs35", mrs->log);
                }
                if (modersp->c == 0) {
                    break;
                }
                ret = mrs_ipc_get(mrs, &ch, 1, 3);
            }
            if (modersp->c == 0) {
                if (modersp->v < 0) {
                    modersp->r |= 0x1;
                }
                modersp->v = 0;
                modersp->r &= ~(0x04);
            }
        }
        else {
            ret = mrs_ipc_get(mrs, &ch, 1, 1);
            while (ret > 0) {
                if (ch == 'p') {
                    modersp->v += 1;
                    if (modersp->v > QSIZE) break;
                }
                if (ch == 'd') {
                    sprintf(mrs->log, "spi __0__ END!!!\n");
                    print_f(&mrs->plog, "fs35", mrs->log);
                    modersp->v += 1;
                    modersp->v = 0 - modersp->v;
                    //mrs_ipc_put(mrs, "n", 1, 3);
                    //modersp->r |= 0x1;
                    //mrs_ipc_put(mrs, "e", 1, 3);
#if PULL_LOW_AFTER_DATA
                    bitset = 0;
                    msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
                    sprintf(mrs->log, "set spi0 RDY pin %d\n",bitset);
                    print_f(&mrs->plog, "fs35", mrs->log);
                    usleep(210000);
#endif
                }
                ret = mrs_ipc_get(mrs, &ch, 1, 1);
            }

            if (modersp->v < 0) {
                modersp->c = 0 - modersp->v;
            } else if (modersp->v > QSIZE) {
                modersp->c = modersp->v;
            }

            if (modersp->c) {
                tmc = modersp->c;
                while (tmc) {
                    mrs_ipc_put(mrs, "n", 1, 3);
                    #if QDELAY
                    usleep(QDELAY);
                    #endif
                    tmc --;
                }
            }
        }
    }

    if (!(modersp->r & 0x4)) {

        if (modersp->r & 0x2) {
            modersp->r |= 0x04;
        } 
        else if (modersp->c) {
            ret = mrs_ipc_get(mrs, &ch, 1, 8);
            while (ret > 0) {
                if (ch == 'n') {
                    modersp->c -= 1;
                    sprintf(mrs->log, "WiFi _1_ rest - %d\n", modersp->c);
                    print_f(&mrs->plog, "fs35", mrs->log);
                }
                if (modersp->c == 0) {
                    break;
                }
                ret = mrs_ipc_get(mrs, &ch, 1, 8);
            }
            if (modersp->c == 0) {
                if (modersp->v < 0) {
                    modersp->r |= 0x2;
                }
                modersp->v = 0;
                modersp->r |= 0x04;
            }
        }
        else {
            ret = mrs_ipc_get(mrs, &ch, 1, 2);
            while (ret > 0) {
                if (ch == 'p') {
                    modersp->v += 1;
                    if (modersp->v > QSIZE) break;
                }
                if (ch == 'd') {
                    sprintf(mrs->log, "spi __1__ END!!!\n");
                    print_f(&mrs->plog, "fs35", mrs->log);
                    modersp->v += 1;
                    modersp->v = 0 - modersp->v;
                    //mrs_ipc_put(mrs, "n", 1, 8);
                    //modersp->r |= 0x1;
                    //mrs_ipc_put(mrs, "e", 1, 8);
#if PULL_LOW_AFTER_DATA
                    bitset = 0;
                    msp_spi_conf(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
                    sprintf(mrs->log, "set spi1 RDY pin %d\n",bitset);
                    print_f(&mrs->plog, "fs35", mrs->log);
                    usleep(210000);
#endif
                }
                ret = mrs_ipc_get(mrs, &ch, 1, 2);
            }

            if (modersp->v < 0) {
                modersp->c = 0 - modersp->v;
            } else if (modersp->v > QSIZE) {
                modersp->c = modersp->v;
            }  

            if (modersp->c) {
                tmc = modersp->c;
                while (tmc) {
                    mrs_ipc_put(mrs, "n", 1, 8);
                    #if QDELAY
                    usleep(QDELAY);
                    #endif
                    tmc --;
                }
            }

        }
    }

    if (modersp->r & 0x3) {
        mrs_ipc_put(mrs, "N", 1, 3);
        mrs_ipc_put(mrs, "N", 1, 8);

        sprintf(mrs->log, "%d end\n", modersp->v);
        print_f(&mrs->plog, "fs35", mrs->log);
        modersp->m = modersp->m + 1;
        return 2;
    }

    return 0; 
}

static int fs36(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset;
    int len=0;
    char ch=0;
    //sprintf(mrs->log, "wait socket finish \n");
    //print_f(&mrs->plog, "fs36", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'N')) {            
        modersp->m = modersp->m + 1;
        return 2;
    }

    return 0;
}

static int fs37(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int bitset, ret;
    int len=0;
    char ch=0;

    len = mrs_ipc_get(mrs, &ch, 1, 8);
    if ((len > 0) && (ch == 'N')) {
        ring_buf_init(&mrs->cmdRx);
        ring_buf_init(&mrs->cmdTx);
        
        bitset = 0;
        msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        sprintf(mrs->log, "spi0 Set data mode: %d\n", bitset);
        print_f(&mrs->plog, "fs37", mrs->log);

        bitset = 0;
        msp_spi_conf(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        sprintf(mrs->log, "spi1 Set data mode: %d\n", bitset);
        print_f(&mrs->plog, "fs37", mrs->log);

        bitset = 1;
        msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
        sprintf(mrs->log, "Set spi 0 slave ready: %d\n", bitset);
        print_f(&mrs->plog, "fs37", mrs->log);

        bitset = 1;
        msp_spi_conf(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
        sprintf(mrs->log, "Set spi 1 slave ready: %d\n", bitset);
        print_f(&mrs->plog, "fs37", mrs->log);

#if SPI_KTHREAD_USE
        bitset = 0;
        ret = msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
        sprintf(mrs->log, "Stop spi0 spidev thread, ret: 0x%x\n", ret);
        print_f(&mrs->plog, "fs37", mrs->log);
        
        bitset = 0;
        ret = msp_spi_conf(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
        sprintf(mrs->log, "Stop spi1 spidev thread, ret: 0x%x\n", ret);
        print_f(&mrs->plog, "fs37", mrs->log);
#endif

        modersp->r = 1;
        return 1;
    }

    return 0;
}
static int fs38(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    struct info16Bit_s *p;

    sprintf(mrs->log, "get OP_FIH \n");
    print_f(&mrs->plog, "fs38", mrs->log);

    p = &mrs->mchine.cur;

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = OP_FIH;
    p->inout = 0;
    p->seqnum = mrs->mchine.seqcnt;
    
    mrs_ipc_put(mrs, "c", 1, 1);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs39(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        //print_f(&mrs->plog, "fs39", mrs->log);

        if (p->opcode == OP_FIH) {
            modersp->m = modersp->m + 1; 
        } else {
            error_handle("get FIH failed", 3741);
        }
    }
    
    if ((len > 0) && (ch == 'X')) {
            sprintf(mrs->log, "FAIL!!send command again!\n");
            print_f(&mrs->plog, "fs39", mrs->log);
            modersp->m = modersp->m - 1;        
            return 2;
    }

    return 0; 
}

static int fs40(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset;
    bitset = 1;
    msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY

    sprintf(mrs->log, "Set spi 0 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs40", mrs->log);

    bitset = 1;
    msp_spi_conf(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY

    sprintf(mrs->log, "Set spi 1 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs40", mrs->log);

    modersp->r = 1;
    return 1;
}

static int fs41(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    struct info16Bit_s *p;
    p = &mrs->mchine.cur;
    //sprintf(mrs->log, "set %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    //print_f(&mrs->plog, "fs41", mrs->log);

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }
    
    mrs_ipc_put(mrs, "c", 1, 1);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs42(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        //print_f(&mrs->plog, "fs42", mrs->log);

        if (p->opcode == OP_QRY) {
            modersp->m = modersp->m + 1;
        } else {
            modersp->r = 2;
            return 1;
        }
    }
    return 0; 
}

static int fs43(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    struct info16Bit_s *p;
    p = &mrs->mchine.cur;
    //sprintf(mrs->log, "set %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    //print_f(&mrs->plog, "fs43", mrs->log);

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }
    
    mrs_ipc_put(mrs, "c", 1, 1);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs44(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int len=0;
    char ch=0;
    struct info16Bit_s *p, *c;

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        c = &mrs->mchine.cur;
        p = &mrs->mchine.get;
        //sprintf(mrs->log, "get 0x%.2x/0x%.2x 0x%.2x/0x%.2x\n", p->opcode, c->opcode, p->data, c->data);
        //print_f(&mrs->plog, "fs44", mrs->log);

        if (p->opcode == c->opcode){
            modersp->r = 1;
            return 1;
        } else {
            modersp->r = 2;
            return 1;
        }
    }
    return 0; 
}

static int fs45(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset=0, ret;
    bitset = 0;
    msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi0 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs45", mrs->log);

#if SPI_KTHREAD_USE
    bitset = 0;
    ret = msp_spi_conf(mrs->sfm[0], _IOR(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_START_THREAD
    sprintf(mrs->log, "Start spi0 spidev thread, ret: 0x%x\n", ret);
    print_f(&mrs->plog, "fs16", mrs->log);
#endif

    modersp->m = modersp->m + 1;
    return 2;
}

static int fs46(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    sprintf(mrs->log, "trigger spi0 \n");
    print_f(&mrs->plog, "fs46", mrs->log);

    mrs_ipc_put(mrs, "s", 1, 1);
    clock_gettime(CLOCK_REALTIME, &mrs->time[0]);

    modersp->m = modersp->m + 1;
    return 2;
}

static int fs47(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    struct sdParseBuff_s *pabuf=0;
    int ret, bitset;
    char ch;

    //sprintf(mrs->log, "%d\n", modersp->v++);
    //print_f(&mrs->plog, "fs47", mrs->log);

    ret = mrs_ipc_get(mrs, &ch, 1, 1);
    if ((ret > 0) && (ch == 'S')){
        clock_gettime(CLOCK_REALTIME, &mrs->time[1]);
        pabuf = &mrs->aspFat.fatDirPool->parBuf;
        sprintf(mrs->log, "spi 0 end, buff used: %d\n", pabuf->dirBuffUsed);
        print_f(&mrs->plog, "fs47", mrs->log);
        
        modersp->m = modersp->m + 1;

#if SPI_KTHREAD_USE
         bitset = 0;
         ret = msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
         sprintf(mrs->log, "Stop spi0 spidev thread, ret: 0x%x\n", ret);
         print_f(&mrs->plog, "fs47", mrs->log);
#endif
#if PULL_LOW_AFTER_DATA
        bitset = 0;
        msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        sprintf(mrs->log, "set RDY pin %d\n",bitset);
        print_f(&mrs->plog, "fs47", mrs->log);
#endif
        usleep(210000);

        return 2;
    }

    return 0; 
}

static int fs48(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    struct info16Bit_s *p;

    //sprintf(mrs->log, "get OP_FIH \n");
    //print_f(&mrs->plog, "fs48", mrs->log);

    p = &mrs->mchine.cur;

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = OP_FIH;
    p->inout = 0;
    p->seqnum = mrs->mchine.seqcnt;
    
    mrs_ipc_put(mrs, "c", 1, 1);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs49(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int len=0;
    char ch=0;
    struct info16Bit_s *p;
    
    //sprintf(mrs->log, "get OP_FIH \n");
    //print_f(&mrs->plog, "fs49", mrs->log);
    
    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        //print_f(&mrs->plog, "fs49", mrs->log);

        if (p->opcode == OP_FIH) {
            modersp->m = 6; 
            return 2;
        } else {
            modersp->r = 2;
            return 1;
        }
    }
    
    if ((len > 0) && (ch == 'X')) {
            sprintf(mrs->log, "FAIL!!send command again!\n");
            print_f(&mrs->plog, "fs49", mrs->log);
            modersp->m = modersp->m - 1;        
            return 2;
    }

    return 0; 
}

static int fs50(struct mainRes_s *mrs, struct modersp_s *modersp)
{
#define SF0 (8 * 0)
#define SF1 (8 * 1)
#define SF2 (8 * 2)
#define SF3 (8 * 3)

    uint32_t val=0, i=0;
    char *pr=0;
    uint32_t secStr=0, secLen=0;
    struct aspConfig_s *pct=0;
    struct sdbootsec_s   *psec=0;
    struct sdFAT_s *pfat=0;
    struct sdParseBuff_s *pParBuf=0;
    struct info16Bit_s *p=0, *c=0;

    sprintf(mrs->log, "parsing boot sector \n");
    print_f(&mrs->plog, "fs50", mrs->log);

    c = &mrs->mchine.cur;
    p = &mrs->mchine.tmp;
    
    pct = mrs->configTable;
    pfat = &mrs->aspFat;
    pParBuf = &pfat->fatDirPool->parBuf;

    if (pParBuf->dirBuffUsed) {
        sprintf(mrs->log, "parsing, buff  size:%d\n", pParBuf->dirBuffUsed);
        print_f(&mrs->plog, "fs50", mrs->log);

        pr = pParBuf->dirParseBuff;
        
        //shmem_dump(pr, 512);
        
        psec = pfat->fatBootsec;
        /* 0  Jump command */
        psec->secJpcmd = pr[0] | (pr[1] << SF1) | (pr[2] << SF2) | (pr[3] << SF3);
        /* 3  system id */
        for (i = 0; i < 8; i++) {
            psec->secSysid[i] = pr[3+i];
        }
        psec->secSysid[8] = '\0';
        /* 11 sector size */ 
        psec->secSize = aspRawCompose(&pr[11], 2);
        /* 13 sector per cluster */
        psec->secPrClst = pr[13];
        /* 14 reserved sector count*/
        psec->secResv = aspRawCompose(&pr[14], 2);
        /* 16 number of FATs */
        psec->secNfat = pr[16];
        /* 17 skip, number of root dir entries */
        /* 19 skip, total sectors */
        /* 21 medium id */
        psec->secIDm = pr[21];
        /* 22 skip, sector per FAT */
        /* 24 sector per track */
        psec->secPrtrk = aspRawCompose(&pr[24], 2);
        /* 26 number of sides */
        psec->secNsid = aspRawCompose(&pr[26], 2);
        /* 28 number of hidded sectors */
        psec->secNhid = aspRawCompose(&pr[28], 4);
        /* 32 total sectors */
        psec->secTotal = aspRawCompose(&pr[32], 4);
        /* 36 sectors per FAT */
        psec->secPrfat = aspRawCompose(&pr[36], 4);
        /* 40 extension flag */
        psec->secExtf = aspRawCompose(&pr[40], 2);
        /* 42 FS version */
        psec->secVers = aspRawCompose(&pr[42], 2); 
        /* 44 root cluster */
        psec->secRtclst = aspRawCompose(&pr[44], 4); 
        /* 48 FS info */
        psec->secFSif = aspRawCompose(&pr[48], 2); 
        /* 50 backup boot sector */
        psec->secBkbt = aspRawCompose(&pr[50], 2); 
        /* 64 physical disk number */
        psec->secPhdk = pr[64];
        /* 66 extended boot record signature */
        psec->secExtbt = pr[66];
        /* 67 volume ID number */
        psec->secVoid = aspRawCompose(&pr[67], 4); 
        /* 71 to 81 volume label */
        for (i = 0; i < 11; i++) {
            psec->secVola[i] = pr[71+i];
        }
        psec->secVola[11] = '\0';
        /* 82 to 89 file system type */
        for (i = 0; i < 8; i++) {
            psec->secFtyp[i] = pr[82+i];
        }
        psec->secFtyp[8] = '\0';
        /* 510 signature word */
        psec->secSign = aspRawCompose(&pr[510], 2); 

        /* set the boot sector status to 1 */
        psec->secSt = 1;

        psec->secWhfat = psec->secResv;
        psec->secWhroot = psec->secWhfat + psec->secPrfat * 2;

        pParBuf->dirBuffUsed = 0;

        if (psec->secSize == 512) {
            pfat->fatStatus |= ASPFAT_STATUS_BOOT_SEC;
            psec->secWhfat += psec->secBoffset;
            psec->secWhroot += psec->secBoffset;

            debugPrintBootSec(psec);
        } else {
            pfat->fatRetry += 1;
#if 0 /* test if boot failed */
            if (pfat->fatRetry == 2) {
                modersp->r = 0xed;
                return 1;
            }

            else if (pfat->fatRetry == 4) {
                modersp->r = 0xed;
                return 1;
            }

            else if (pfat->fatRetry == 6) {
                modersp->r = 0xed;
                return 1;
            }

            else if (pfat->fatRetry == 8) {
                modersp->r = 0xed;
                return 1;
            }

            else if (pfat->fatRetry == 10) {
                modersp->r = 0xed;
                return 1;
            }

            else if (pfat->fatRetry == 12) {
                modersp->r = 0xed;
                return 1;
            }

            else if (pfat->fatRetry > 14) {
                psec->secBoffset = 8192;
            }
#else
            if (pfat->fatRetry > 2) {
                psec->secBoffset = 8192;
            }
#endif
        }

        sprintf(mrs->log, "!!!! boot retry :%d offset: %d  !!!!\n", pfat->fatRetry, psec->secBoffset);
        print_f(&mrs->plog, "fs50", mrs->log);

        modersp->r = 1;
    }else {
        secStr = c->opinfo;
        secLen = p->opinfo;

        sprintf(mrs->log, "buff empty, set str:%d(0x%x), len:%d \n", secStr, secStr, secLen);
        print_f(&mrs->plog, "fs50", mrs->log);

        cfgTableSet(pct, ASPOP_SDFAT_RD, 1);

        val = cfgValueOffset(secStr, 24);
        cfgTableSet(pct, ASPOP_SDFAT_STR01, val);
        val = cfgValueOffset(secStr, 16);
        cfgTableSet(pct, ASPOP_SDFAT_STR02, val);
        val = cfgValueOffset(secStr, 8);
        cfgTableSet(pct, ASPOP_SDFAT_STR03, val);
        val = cfgValueOffset(secStr, 0);
        cfgTableSet(pct, ASPOP_SDFAT_STR04, val);

        val = cfgValueOffset(secLen, 24);
        cfgTableSet(pct, ASPOP_SDFAT_LEN01, val);
        val = cfgValueOffset(secLen, 16);
        cfgTableSet(pct, ASPOP_SDFAT_LEN02, val);
        val = cfgValueOffset(secLen, 8);
        cfgTableSet(pct, ASPOP_SDFAT_LEN03, val);
        val = cfgValueOffset(secLen, 0);
        cfgTableSet(pct, ASPOP_SDFAT_LEN04, val);

        cfgTableSet(pct, ASPOP_SDFAT_SDAT, 1);

        modersp->r = 2;
    }


    return 1;
}
static int fs51(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int val=0, i=0, ret=0;
    char *pr=0;
    uint32_t secStr=0, secLen=0;
    struct aspConfig_s *pct=0;
    struct sdbootsec_s   *psec=0;
    struct sdFAT_s *pfat=0;
    struct sdParseBuff_s *pParBuf=0;
    struct info16Bit_s *p=0, *c=0;
    struct directnFile_s *curDir=0, *ch=0, *br=0;
    struct folderQueue_s *pfhead=0, *pfdirt=0, *pfnext=0;
    struct adFATLinkList_s *pflsh=0, *pflnt=0;
    struct sdFATable_s   *pftb=0;
    
    c = &mrs->mchine.cur;
    p = &mrs->mchine.tmp;
    
    pct = mrs->configTable;
    pfat = &mrs->aspFat;
    pParBuf = &pfat->fatDirPool->parBuf;
    psec = pfat->fatBootsec;
    pftb = pfat->fatTable;

    if (pftb->c) {
        pflnt = pftb->c;
        pftb->c = pflnt->n;
        aspFree(pflnt);
    }

    if ((!pftb->c) && (pParBuf->dirBuffUsed)) {
        sprintf(mrs->log, "parsing, buff  size:%d\n", pParBuf->dirBuffUsed);
        print_f(&mrs->plog, "fs51", mrs->log);

        pftb->h = 0;
        
        pr = pParBuf->dirParseBuff;

        ret = aspFS_createFATRoot(pfat);
        if (ret == 0) {
            aspFS_insertFATChilds(pfat, pfat->fatRootdir, pr, pParBuf->dirBuffUsed);
        }

        if (pfat->fatRootdir->ch->dfSFN) {
            pfat->fatStatus |= ASPFAT_STATUS_ROOT_DIR;
        }
        

        curDir = pfat->fatRootdir;
        br= curDir->ch;

#if 0 /* do folder parsing anyway */
        if (!br) {
            pfat->fatStatus |= ASPFAT_STATUS_FOLDER;
            modersp->r = 1;
            return 1;
        }
#endif

        while (br) {
            if (br->dftype == ASPFS_TYPE_DIR) {
                if ((strcmp(br->dfSFN, "..") != 0) && (strcmp(br->dfSFN, ".") != 0)) {
                    sprintf(mrs->log, "ADD folder [%s] to parsing queue\n", br->dfSFN);
                    print_f(&mrs->plog, "fs51", mrs->log);
                    pfdirt = aspMalloc(sizeof(struct folderQueue_s));
                    pfdirt->fdObj = br;
                    pfdirt->fdnxt = 0;
                    
                    if (!pfhead) {
                        pfhead = pfdirt;
                    } else {
                        pfnext = pfhead;
                        
                        while (pfnext->fdnxt) {
                            pfnext = pfnext->fdnxt;
                        }

                        pfnext->fdnxt = pfdirt;
                    }
                }
            }

            br = br->br;            
        }

        pParBuf->dirBuffUsed = 0;
        mrs->folder_dirt = pfhead;
        
        modersp->r = 1;
    }else {

        if (!pftb->h) {
            pflsh = 0;
            ret = mspSD_parseFAT2LinkList(&pflsh, psec->secRtclst, pftb->ftbFat1, (psec->secTotal - psec->secWhroot) / psec->secPrClst);
            if (ret) {
                sprintf(mrs->log, "FAT table parsing for root dictionary FAIL!!ret:%d \n", ret);
                print_f(&mrs->plog, "fs51", mrs->log);
                modersp->r = 3;
                return 1;
            }
            sprintf(mrs->log, "show root FAT link str:\n");
            print_f(&mrs->plog, "fs51", mrs->log);

            pflnt = pflsh;
            while (pflnt) {
                sprintf(mrs->log, "    str:%d len:%d\n", pflnt->ftStart, pflnt->ftLen);
                print_f(&mrs->plog, "fs51", mrs->log);
                pflnt = pflnt->n;
            }
            pftb->h = pflsh;
            pftb->c = pftb->h;
        }

        pflnt = pftb->c;
                 
        secStr = (pflnt->ftStart - 2) * psec->secPrClst + psec->secWhroot;
        secLen = pflnt->ftLen * psec->secPrClst;

        c->opinfo = secStr;
        p->opinfo = secLen;

        if (secLen < 16) secLen = 16;

        sprintf(mrs->log, "buff empty, set str:%d, len:%d \n", secStr, secLen);
        print_f(&mrs->plog, "fs51", mrs->log);

        cfgTableSet(pct, ASPOP_SDFAT_RD, 1);

        val = cfgValueOffset(secStr, 24);
        cfgTableSet(pct, ASPOP_SDFAT_STR01, val);
        val = cfgValueOffset(secStr, 16);
        cfgTableSet(pct, ASPOP_SDFAT_STR02, val);
        val = cfgValueOffset(secStr, 8);
        cfgTableSet(pct, ASPOP_SDFAT_STR03, val);
        val = cfgValueOffset(secStr, 0);
        cfgTableSet(pct, ASPOP_SDFAT_STR04, val);
        val = cfgValueOffset(secLen, 24);
        cfgTableSet(pct, ASPOP_SDFAT_LEN01, val);
        val = cfgValueOffset(secLen, 16);
        cfgTableSet(pct, ASPOP_SDFAT_LEN02, val);
        val = cfgValueOffset(secLen, 8);
        cfgTableSet(pct, ASPOP_SDFAT_LEN03, val);
        val = cfgValueOffset(secLen, 0);
        cfgTableSet(pct, ASPOP_SDFAT_LEN04, val);

        cfgTableSet(pct, ASPOP_SDFAT_SDAT, 1);

        modersp->r = 2;
    }

    return 1;
}
static int fs52(struct mainRes_s *mrs, struct modersp_s *modersp) 
{
    char strFullPath[544];
    char strPath[32][16];
    int val=0, i=0, ret=0, last=0, offset=0;
    char *pr=0;
    uint32_t secStr=0, secLen=0;
    struct aspConfig_s *pct=0;
    struct sdbootsec_s   *psec=0;
    struct sdFAT_s *pfat=0;
    struct sdParseBuff_s *pParBuf=0;
    struct info16Bit_s *p=0, *c=0;
    struct directnFile_s *curDir=0, *ch=0, *br=0, *pa=0;
    struct folderQueue_s *pfhead=0, *pfdirt=0, *pfnext=0;
    struct adFATLinkList_s *pflsh=0, *pflnt=0;
    struct sdFATable_s   *pftb=0;
    
    c = &mrs->mchine.cur;
    p = &mrs->mchine.tmp;
    
    pct = mrs->configTable;
    pfat = &mrs->aspFat;
    pParBuf = &pfat->fatDirPool->parBuf;
    psec = pfat->fatBootsec;
    pftb = pfat->fatTable;
    
    if (mrs->folder_dirt) {
        pfhead = mrs->folder_dirt;
        curDir = pfhead->fdObj;

        if (pftb->c) {
            pflnt = pftb->c;
            pftb->c = pflnt->n;
            aspFree(pflnt);
        }

        if ((!pftb->c) && (pParBuf->dirBuffUsed)) {
            sprintf(mrs->log, "parsing, buff  size:%d\n", pParBuf->dirBuffUsed);
            print_f(&mrs->plog, "fs52", mrs->log);

            pftb->h = 0;

            pr = pParBuf->dirParseBuff;
            last = pParBuf->dirBuffUsed;

            mspFS_insertFATChildDir(pfat, curDir, pr, last);

            ch = curDir->ch;
            if (!ch) {
                sprintf(mrs->log, "ERROR!! folder [%s] should have child\n", curDir->dfSFN);
                print_f(&mrs->plog, "fs52", mrs->log);

                modersp->r = 0xed;
                return 1;
            }
            
            br = ch->br;
            while (br) {
                if (br->dftype == ASPFS_TYPE_DIR) {
                    if ((strcmp(br->dfSFN, "..") != 0) && (strcmp(br->dfSFN, ".") != 0)) {
                        sprintf(mrs->log, "ADD folder [%s]\n", br->dfSFN);
                        print_f(&mrs->plog, "fs52", mrs->log);

                        pfdirt = aspMalloc(sizeof(struct folderQueue_s));
                        pfdirt->fdObj = br;
                        pfdirt->fdnxt = 0;
                    
                        if (!pfhead) {
                            pfhead = pfdirt;
                        } else {
                            pfnext = pfhead;
                        
                            while (pfnext->fdnxt) {
                                pfnext = pfnext->fdnxt;
                            }

                            pfnext->fdnxt = pfdirt;
                        }
                    }
                }
/*
                pflsh = 0;
                ret = mspSD_parseFAT2LinkList(&pflsh, br->dfclstnum, pftb->ftbFat1, pftb->ftbLen/4);
                if (ret) {
                    sprintf(mrs->log, "FAT table parsing for root dictionary FAIL!!ret:%d \n", ret);
                    print_f(&mrs->plog, "fs52", mrs->log);
                } else {
                    sprintf(mrs->log, "show FAT for /root/%s\n", br->dfSFN);
                    print_f(&mrs->plog, "fs52", mrs->log);
                }
                
                pflnt = pflsh;
                while (pflnt) {
                    sprintf(mrs->log, "check list str:%d len:%d\n", pflnt->ftStart, pflnt->ftLen);
                    print_f(&mrs->plog, "fs52", mrs->log);
                    pflnt = pflnt->n;
                }
*/
                br = br->br;
            }

            pParBuf->dirBuffUsed = 0;
            
            mrs->folder_dirt = pfhead->fdnxt;
            aspFree(pfhead);
            
        }
    }
    
    if (mrs->folder_dirt) {
        pfhead = mrs->folder_dirt;
        curDir = pfhead->fdObj;

        if (!pftb->h) {
            pflsh = 0;

            ret = mspSD_parseFAT2LinkList(&pflsh, curDir->dfclstnum, pftb->ftbFat1, (psec->secTotal - psec->secWhroot) / psec->secPrClst);
            if (ret) {
                sprintf(mrs->log, "FAT table parsing for root dictionary FAIL!!ret:%d (%s)\n", ret, curDir->dfSFN);
                print_f(&mrs->plog, "fs52", mrs->log);
                modersp->r = 0xed;
                return 1;
            }
            /* debug */
            sprintf(mrs->log, "show FAT link for [%s]:\n", curDir->dfSFN);
            print_f(&mrs->plog, "fs52", mrs->log);

            pflnt = pflsh;
            while (pflnt) {
                sprintf(mrs->log, "    str:%d len:%d\n", pflnt->ftStart, pflnt->ftLen);
                print_f(&mrs->plog, "fs52", mrs->log);
                pflnt = pflnt->n;
            }
            pftb->h = pflsh;
            pftb->c = pftb->h;
        }

        pflnt = pftb->c;

        sprintf(mrs->log, "[%d x %d + %d] \n",pflnt->ftStart - 2, psec->secPrClst, psec->secWhroot);
        print_f(&mrs->plog, "fs52", mrs->log);
                 
        secStr = (pflnt->ftStart - 2) * (uint32_t)psec->secPrClst + (uint32_t)psec->secWhroot;
        secLen = pflnt->ftLen * (uint32_t)psec->secPrClst;
        //secStr = (curDir->dfclstnum - 2) * psec->secPrClst + psec->secWhroot;
        //secLen = psec->secPrClst;

        c->opinfo = secStr;
        p->opinfo = secLen;

        if (secLen < 16) secLen = 16;
        
        memset(strPath[0], 0, 512);

        pa = curDir;
        i = 0;
        while(pa) {

            strcpy(strPath[i], pa->dfSFN);
            pa = pa->pa;
            i++;
            if (i >= 16) break;
        }

        memset(strFullPath, 0, 544);
        
        pr = strFullPath;
        while (i) {
            i --;
            *pr = '/';
            pr += 1;
            
            ret = strlen(strPath[i]);
            strncpy(pr, strPath[i], ret);
            pr += ret;
            
        }

        sprintf(mrs->log, "NEXT parsing dir[%s], set str:%d, len:%d(%d) \n", strFullPath, secStr, secLen, pflnt->ftStart);
        print_f(&mrs->plog, "fs52", mrs->log);

        cfgTableSet(pct, ASPOP_SDFAT_RD, 1);

        val = cfgValueOffset(secStr, 24);
        cfgTableSet(pct, ASPOP_SDFAT_STR01, val);
        val = cfgValueOffset(secStr, 16);
        cfgTableSet(pct, ASPOP_SDFAT_STR02, val);
        val = cfgValueOffset(secStr, 8);
        cfgTableSet(pct, ASPOP_SDFAT_STR03, val);
        val = cfgValueOffset(secStr, 0);
        cfgTableSet(pct, ASPOP_SDFAT_STR04, val);
        val = cfgValueOffset(secLen, 24);
        cfgTableSet(pct, ASPOP_SDFAT_LEN01, val);
        val = cfgValueOffset(secLen, 16);
        cfgTableSet(pct, ASPOP_SDFAT_LEN02, val);
        val = cfgValueOffset(secLen, 8);
        cfgTableSet(pct, ASPOP_SDFAT_LEN03, val);
        val = cfgValueOffset(secLen, 0);
        cfgTableSet(pct, ASPOP_SDFAT_LEN04, val);

        cfgTableSet(pct, ASPOP_SDFAT_SDAT, 1);

        modersp->r = 2;
    }
    else {
        pfat->fatStatus |= ASPFAT_STATUS_FOLDER;
        modersp->r = 1;
    }

    return 1;
}
static int fs53(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
#define FAT_FILE (1)

#if FAT_FILE
    FILE *f=0;
    char fatPath[128] = "/mnt/mmc2/fatTab.bin";
    char fatDst[128];
#endif

    int val=0, i=0, ret = 0;
    char *pr=0;
    uint32_t secStr=0, secLen=0, freeClst=0, usedClst=0, totClst=0;
    struct aspConfig_s *pct=0;
    struct sdbootsec_s   *psec=0;
    struct sdFAT_s *pfat=0;
    struct sdParseBuff_s *pParBuf=0;
    struct info16Bit_s *p=0, *c=0;
    struct sdFATable_s   *pftb=0;
    struct adFATLinkList_s *pfatfree=0, *pflnt=0;
    
    sprintf(mrs->log, "read FAT  \n");
    print_f(&mrs->plog, "fs53", mrs->log);

    c = &mrs->mchine.cur;
    p = &mrs->mchine.tmp;
    
    pct = mrs->configTable;
    pfat = &mrs->aspFat;
    pftb = pfat->fatTable;
    psec = pfat->fatBootsec;
    pParBuf = &pfat->fatDirPool->parBuf;

    if (pftb->ftbFat1) {
        sprintf(mrs->log, "get FAT table, addr:0x%.8x, len:%d\n", pftb->ftbFat1, pftb->ftbLen);
        print_f(&mrs->plog, "fs53", mrs->log);
        
#if FAT_FILE
        //ret = file_save_get(&f, fatPath);
        //f = find_save(fatDst, fatPath);
        f = fopen(fatPath, "w+");

        if (f) {
            fwrite(pftb->ftbFat1, 1, pftb->ftbLen, f);
            fflush(f);
            fclose(f);
            sprintf(mrs->log, "FAT table save to [%s] size:%d\n", fatPath, pftb->ftbLen);
            print_f(&mrs->plog, "fs53", mrs->log);
        } else {
            sprintf(mrs->log, "FAT table find save to [%s] failed !!!\n", fatPath);
            print_f(&mrs->plog, "fs53", mrs->log);
        }
/*
        aspFree(pftb->ftbFat1);
        pftb->ftbFat1 = 0;
        pftb->ftbLen = 0;
*/
#endif
        sprintf(mrs->log, "total sector: %d, root sector: %d, free cluster: %d vs secPerfat: %d\n", psec->secTotal, psec->secWhroot, (psec->secTotal - psec->secWhroot) / psec->secPrClst, psec->secPrfat);
        print_f(&mrs->plog, "fs53", mrs->log);

        totClst = (psec->secTotal - psec->secWhroot) / psec->secPrClst;
        ret = mspSD_getFreeFATList(&pfatfree, 0, pftb->ftbFat1, totClst);
        if (!ret) {
            sprintf(mrs->log, "show FAT free space \n");
            print_f(&mrs->plog, "fs53", mrs->log);

            freeClst = 0;
            pflnt = pfatfree;
            while (pflnt) {
                freeClst += pflnt->ftLen;
                sprintf(mrs->log, "start: %d len:%d \n", pflnt->ftStart, pflnt->ftLen);
                print_f(&mrs->plog, "fs53", mrs->log);
                pflnt = pflnt->n;
            }
            sprintf(mrs->log, "total free cluster: %d, free sector: %d (%d) \n", freeClst, freeClst * psec->secPrClst, freeClst * psec->secPrClst * psec->secSize);
            print_f(&mrs->plog, "fs53", mrs->log);     
            usedClst = totClst - freeClst;
            
            pftb->ftbMng.ftfreeClst = freeClst;
            pftb->ftbMng.ftusedClst = usedClst;
            pftb->ftbMng.f = pfatfree;
            
            pfat->fatStatus |= ASPFAT_STATUS_FAT;
            msync(pftb->ftbFat1, pftb->ftbLen, MS_SYNC);
            
            modersp->r = 1;
        } else {
            sprintf(mrs->log, "parse FAT free space failed, ret:0x%x \n", ret);
            print_f(&mrs->plog, "fs53", mrs->log);
            modersp->r = 0xed;
        }
    }else {
        secStr = c->opinfo;
        secLen = p->opinfo;

        sprintf(mrs->log, "buff empty, set str:%d, len:%d \n", secStr, secLen);
        print_f(&mrs->plog, "fs53", mrs->log);

        cfgTableSet(pct, ASPOP_SDFAT_RD, 1);

        val = cfgValueOffset(secStr, 24);
        cfgTableSet(pct, ASPOP_SDFAT_STR01, val);
        val = cfgValueOffset(secStr, 16);
        cfgTableSet(pct, ASPOP_SDFAT_STR02, val);
        val = cfgValueOffset(secStr, 8);
        cfgTableSet(pct, ASPOP_SDFAT_STR03, val);
        val = cfgValueOffset(secStr, 0);
        cfgTableSet(pct, ASPOP_SDFAT_STR04, val);
        val = cfgValueOffset(secLen, 24);
        cfgTableSet(pct, ASPOP_SDFAT_LEN01, val);
        val = cfgValueOffset(secLen, 16);
        cfgTableSet(pct, ASPOP_SDFAT_LEN02, val);
        val = cfgValueOffset(secLen, 8);
        cfgTableSet(pct, ASPOP_SDFAT_LEN03, val);
        val = cfgValueOffset(secLen, 0);
        cfgTableSet(pct, ASPOP_SDFAT_LEN04, val);

        cfgTableSet(pct, ASPOP_SDFAT_SDAT, 1);

        modersp->r = 2;
    }

    return 1;
}

static int fs54(struct mainRes_s *mrs, struct modersp_s *modersp) 
{
    int bitset=0, ret=0;
    
    bitset = 0;
    msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi0 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs54", mrs->log);

#if SPI_KTHREAD_USE
    bitset = 0;
    ret = msp_spi_conf(mrs->sfm[0], _IOR(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_START_THREAD
    sprintf(mrs->log, "Start spi0 spidev thread, ret: 0x%x\n", ret);
    print_f(&mrs->plog, "fs54", mrs->log);
#endif

    sprintf(mrs->log, "trigger spi0 \n");
    print_f(&mrs->plog, "fs54", mrs->log);

    ring_buf_init(&mrs->cmdRx);

    mrs_ipc_put(mrs, "n", 1, 1);
    clock_gettime(CLOCK_REALTIME, &mrs->time[0]);

    modersp->m = modersp->m + 1;
    return 0;
}
static int fs55(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int val=0, pi=0, ret=0, len=0, bitset=0;
    char *pr=0, ch=0, *addr=0;
    uint32_t secStr=0, secLen=0;
    
    struct sdbootsec_s   *psec=0;
    struct sdFAT_s *pfat=0;
    struct info16Bit_s *p=0, *c=0;
    struct sdFATable_s   *pftb=0;

    //sprintf(mrs->log, "read FAT  \n");
    //print_f(&mrs->plog, "fs55", mrs->log);

    c = &mrs->mchine.cur;
    p = &mrs->mchine.tmp;
    
    pfat = &mrs->aspFat;
    pftb = pfat->fatTable;

    ret = mrs_ipc_get(mrs, &ch, 1, 1);
    while (ret > 0) {
        if ((ch == 'p') || (ch == 'd')){
            if (!pftb->ftbFat1) {
                secLen = p->opinfo;
                val = secLen * 512;
                //pftb->ftbFat1 = aspMalloc(val);
                pftb->ftbFat1 = aspSalloc(val);
                if (!pftb->ftbFat1) {
                    sprintf(mrs->log, "aspMalloc for FAT table FAIL!! \n");
                    print_f(&mrs->plog, "fs55", mrs->log);

                    modersp->r = 2;
                    return 1;
                }
                sprintf(mrs->log, "FAT table size: %d\n", val);
                print_f(&mrs->plog, "fs55", mrs->log);

                pftb->ftbLen = 0;
            }
        
            modersp->v += 1;
            len = ring_buf_cons(&mrs->cmdRx, &addr);
            if (len >= 0) {
                pi++;
                msync(addr, len, MS_SYNC);
                /* send data to wifi socket */
                if (len != 0) {
                    pr = pftb->ftbFat1 + pftb->ftbLen;
                    memcpy(pr, addr, len);
                    pftb->ftbLen += len;
                    //sprintf(mrs->log, "%d get fat len:%d, total:%d\n", pi, len, pftb->ftbLen);
                    //print_f(&mrs->plog, "fs55", mrs->log);
                }
            } else {
                sprintf(mrs->log, "end, len:%d\n", len);
                print_f(&mrs->plog, "fs55", mrs->log);
            }

            if (ch == 'd') {
                sprintf(mrs->log, "spi0 %d end\n", modersp->v);
                print_f(&mrs->plog, "fs55", mrs->log);
#if PULL_LOW_AFTER_DATA
                bitset = 0;
                msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
                sprintf(mrs->log, "set RDY pin %d\n",bitset);
                print_f(&mrs->plog, "fs55", mrs->log);
#endif
#if SPI_KTHREAD_USE
                bitset = 0;
                ret = msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
                sprintf(mrs->log, "Stop spi0 spidev thread, ret: 0x%x\n", ret);
                print_f(&mrs->plog, "fs55", mrs->log);
#endif
                usleep(210000);

                modersp->m = 48;
                return 2;
            }
        }

        ret = mrs_ipc_get(mrs, &ch, 1, 1);
    }
    
    return 0;
}

static int fs56(struct mainRes_s *mrs, struct modersp_s *modersp) 
{ 
    struct sdFAT_s *pfat=0;
    struct directnFile_s *curDir=0;

    sprintf(mrs->log, "show the tree!!!  \n");
    print_f(&mrs->plog, "fs56", mrs->log);

    pfat = &mrs->aspFat;
    curDir = pfat->fatRootdir;

    if (!(pfat->fatStatus & ASPFAT_STATUS_BOOT)) {
        mspFS_listDetail(curDir, 4);
        //mspFS_list(curDir, 4);
        pfat->fatStatus |= ASPFAT_STATUS_BOOT;
        
        pfat->fatCurDir = pfat->fatRootdir;
    } else {
        if(pfat->fatCurDir) {
            curDir = pfat->fatCurDir;
            mspFS_folderList(curDir, 4);            
        } else {
            pfat->fatCurDir = pfat->fatRootdir;
            curDir = pfat->fatCurDir;
            mspFS_folderList(curDir, 4);            
        }
    }

    modersp->r = 4;    
    return 1;
}

static int fs57(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    struct info16Bit_s *p;
    p = &mrs->mchine.cur;

    p->opcode = OP_SUPBACK;
    p->data = 0;

    //sprintf(mrs->log, "set opcode OP_SUPBACK: 0x%.2x 0x%.2x \n", p->opcode, p->data);
    //print_f(&mrs->plog, "fs57", mrs->log);
    
    mrs_ipc_put(mrs, "c", 1, 1);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs58(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        //sprintf(mrs->log, "get opcode 0x%.2x 0x%.2x \n", p->opcode, p->data);
        //print_f(&mrs->plog, "fs58", mrs->log);

        if (p->opcode == OP_SUPBACK) {
            modersp->m = modersp->m + 1;
            return 2;
        } else {
            modersp->r = 2;
            return 1;
        }
    }
    return 0; 
}

static int fs59(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int ret=0;
    uint32_t val=0, fformat=0;
    struct supdataBack_s *s=0;
    struct aspConfig_s *pct=0;
    struct sdFAT_s *pfat=0;

    pct = mrs->configTable;
    pfat = &mrs->aspFat;

    sprintf(mrs->log, "initial the fatSupdata !!!  \n");
    print_f(&mrs->plog, "fs59", mrs->log);
    pfat->fatSupdata = 0;

    ret = cfgTableGetChk(pct, ASPOP_FILE_FORMAT, &fformat, ASPOP_STA_CON);    
    if (ret) {
        fformat = 0;
    }
    
    //cfgTableSet(pct, ASPOP_SUP_SAVE, (uint32_t)s);
    s = 0;
    s = aspMalloc(sizeof(struct supdataBack_s));
    if (!s) {
        sprintf(mrs->log, "FAIL to initial the second fatSupdata !!! \n");
        print_f(&mrs->plog, "fs59", mrs->log);

        modersp->r = 2;
        return 1;
    }

    //cfgTableSet(pct, ASPOP_SUP_SAVE, (uint32_t)s);
    memset(s, 0, sizeof(struct supdataBack_s));
    pfat->fatSupdata = s;   
    pfat->fatSupcur = pfat->fatSupdata;

    if ((fformat == FILE_FORMAT_PDF) || (fformat == FILE_FORMAT_TIFF_I)) {
        sprintf(mrs->log, "file format (%d) 2:PDF 4:tiff_i, allocate one more trunk at the begin\n", fformat);
        print_f(&mrs->plog, "fs59", mrs->log);

        s = aspMalloc(sizeof(struct supdataBack_s));
        if (!s) {
            sprintf(mrs->log, "FAIL to initial the head fatSupdata !!! \n");
            print_f(&mrs->plog, "fs59", mrs->log);

            modersp->r = 2;
            return 1;
        }

        memset(s, 0, sizeof(struct supdataBack_s));
        pfat->fatSupcur->supdataUse = SPI_TRUNK_SZ - 512;
        pfat->fatSupcur->supdataTot = SPI_TRUNK_SZ;
        pfat->fatSupcur->n = s;
        pfat->fatSupcur = s;
    }

    sprintf(mrs->log, "fatSupdata = 0x%.8x, fatSupcur = 0x%.8x!!!  \n", pfat->fatSupdata, pfat->fatSupcur);
    print_f(&mrs->plog, "fs59", mrs->log);

    if (modersp->d) {
        modersp->m = modersp->d;
        modersp->d = 0;
        return 2;
    } else {
        modersp->r = 1;
        return 1;
    }
}

static int fs60(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    struct info16Bit_s *p;
    p = &mrs->mchine.cur;

    p->opcode = OP_SUPBACK;
    p->data = 0;

    //sprintf(mrs->log, "set opcode OP_SUPBACK: 0x%.2x 0x%.2x \n", p->opcode, p->data);
    //print_f(&mrs->plog, "fs60", mrs->log);
    
    mrs_ipc_put(mrs, "c", 1, 1);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs61(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        //sprintf(mrs->log, "get opcode 0x%.2x 0x%.2x \n", p->opcode, p->data);
        //print_f(&mrs->plog, "fs61", mrs->log);

        if (p->opcode == OP_QRY) {
            modersp->m = modersp->m + 1;            
            return 2;
        } else {
            modersp->r = 2;
            return 1;
        }
    }
    return 0; 
}

static int fs62(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    struct info16Bit_s *p;
    p = &mrs->mchine.cur;

    p->opcode = OP_SUPBACK;
    p->data = 0;

    //sprintf(mrs->log, "set opcode OP_SUPBACK: 0x%.2x 0x%.2x \n", p->opcode, p->data);
    //print_f(&mrs->plog, "fs62", mrs->log);
    
    mrs_ipc_put(mrs, "c", 1, 1);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs63(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        //sprintf(mrs->log, "get opcode 0x%.2x 0x%.2x \n", p->opcode, p->data);
        //print_f(&mrs->plog, "fs63", mrs->log);

        if (p->opcode == OP_SUPBACK) {
            modersp->m = modersp->m + 1;            
            return 2;
        } else {
            modersp->r = 2;
            return 1;
        }
    }
    return 0; 
}

#define SUP_FILE (0)
static int fs64(struct mainRes_s *mrs, struct modersp_s *modersp)
{

#if SUP_FILE
    FILE *f=0;
    char supPath[128] = "/mnt/mmc2/tx/supBack_%d.bin";
    char supDst[128];
#endif

    sprintf(mrs->log, "trigger spi0 \n");
    print_f(&mrs->plog, "fs64", mrs->log);

    ring_buf_init(&mrs->dataTx);
    
#if SUP_FILE
    f = find_save(supDst, supPath);
    if (f) {
        sprintf(mrs->log, "save sup back to [%s] \n", supDst);
        print_f(&mrs->plog, "fs64", mrs->log);

        mrs->mchine.cur.opinfo = (uint32_t)f;
    }
#else    
    mrs->mchine.cur.opinfo = 0;
#endif
    
    mrs_ipc_put(mrs, "k", 1, 1);
    clock_gettime(CLOCK_REALTIME, &mrs->time[0]);

    modersp->m = modersp->m + 1;
    modersp->c = 0;
    modersp->v = 0;
    return 2;
}

static int fs65(struct mainRes_s *mrs, struct modersp_s *modersp) 
{ 
#if SUP_FILE
    FILE *f=0;
#endif
    struct sdFAT_s *pfat=0;
    struct supdataBack_s *s=0, *sc=0, *sh=0;

    int len=0, cnt=0;;
    char *addr = 0;

    sprintf(mrs->log, "start \n");
    print_f(&mrs->plog, "fs65", mrs->log);
    pfat = &mrs->aspFat;
    sh = pfat->fatSupdata;
    sc = pfat->fatSupcur;
#if SUP_FILE
    f = (FILE *)mrs->mchine.cur.opinfo;
#endif        
    if (sc) {
        sprintf(mrs->log, "the current should not here!!! sc:0x%.8x\n", (uint32_t)sc);
        print_f(&mrs->plog, "fs65", mrs->log);
    }
    
    if (!sh) {
        sprintf(mrs->log, "the head should not here!!! sh:0x%.8x\n", (uint32_t)sh);
        print_f(&mrs->plog, "fs65", mrs->log);
    }

    while (sh) {
    
        len = ring_buf_get(&mrs->dataTx, &addr);
        if (len <= 0) {
            //sprintf(mrs->log, "WARNING, len:%d \n", len);
            //print_f(&mrs->plog, "fs65", mrs->log);
            break;
        } else if (len != SPI_TRUNK_SZ) {
            sprintf(mrs->log, "WARNING, buff len not equal to %d, len:%d \n", SPI_TRUNK_SZ, len);
            print_f(&mrs->plog, "fs65", mrs->log);
        } 

        //sprintf(mrs->log, "cnt:%d\n", modersp->c);
        //print_f(&mrs->plog, "fs65", mrs->log);
        
        if (sh->supdataTot < len) {
            len = sh->supdataTot;
        }

        if (len > 0) {

#if SUP_FILE
            if (f) {
                fwrite(sh->supdataBuff, 1, len, f);
                sprintf(mrs->log, "sup save len:%d\n", len);
                print_f(&mrs->plog, "fs65", mrs->log);
            } else {
                sprintf(mrs->log, "sup back save NONE \n");
                print_f(&mrs->plog, "fs65", mrs->log);
            }
#endif

            memcpy(addr, sh->supdataBuff, len);
            ring_buf_prod(&mrs->dataTx);
            mrs_ipc_put(mrs, "k", 1, 1);
            modersp->c += 1;
            modersp->v += len;
        }

        s = sh;
        sh = sh->n;

        pfat->fatSupdata = sh;
        
        memset(s, 0, sizeof(struct supdataBack_s));
        aspFree(s);
    }

    if (sh) {
        //sprintf(mrs->log, "not yet, cnt:%d \n", modersp->c);
        //print_f(&mrs->plog, "fs65", mrs->log);

        return 0;
    } else {
        ring_buf_set_last(&mrs->dataTx, len);
        
        mrs_ipc_put(mrs, "K", 1, 1);    
        sprintf(mrs->log, "tx done:%d total:%d last:%d\n", modersp->c, modersp->v, len);
        print_f(&mrs->plog, "fs65", mrs->log);
        modersp->m = modersp->m + 1;
#if SUP_FILE
        if (f) {
            fclose(f);
            mrs->mchine.cur.opinfo = 0;
        }
#endif        
        return 2;
    }
    
}

static int fs66(struct mainRes_s *mrs, struct modersp_s *modersp) 
{ 
    int len=0, bitset=0, ret=0;
    char ch=0;
    struct info16Bit_s *p;

    //sprintf(mrs->log, "wait spi0 tx end\n");
    //print_f(&mrs->plog, "fs66", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if (len > 0) {

        sprintf(mrs->log, "ch: %c - end\n", ch);
        print_f(&mrs->plog, "fs66", mrs->log);

        if (ch == 'K') {

#if PULL_LOW_AFTER_DATA
            bitset = 0;
            msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
            sprintf(mrs->log, "set RDY pin %d\n",bitset);
            print_f(&mrs->plog, "fs66", mrs->log);
#endif
#if SPI_KTHREAD_USE
            bitset = 0;
            ret = msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
            sprintf(mrs->log, "Stop spi0 spidev thread, ret: 0x%x\n", ret);
            print_f(&mrs->plog, "fs66", mrs->log);
#endif
            usleep(210000);

            modersp->r = 1;            
            return 1;
        } else  {
            modersp->r = 2;
            return 1;
        }
    }
    return 0; 
}
static int fs67(struct mainRes_s *mrs, struct modersp_s *modersp) 
{ 
    int bitset, ret;
    sprintf(mrs->log, "trigger spi0\n");
    print_f(&mrs->plog, "fs67", mrs->log);

#if SPI_KTHREAD_USE
    bitset = 0;
    ret = msp_spi_conf(mrs->sfm[0], _IOR(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_START_THREAD
    sprintf(mrs->log, "Start spi0 spidev thread, ret: 0x%x\n", ret);
    print_f(&mrs->plog, "fs67", mrs->log);
#endif

    ring_buf_init(&mrs->cmdRx);

    mrs_ipc_put(mrs, "n", 1, 1);
    clock_gettime(CLOCK_REALTIME, &mrs->time[0]);

    modersp->v = 0;
    modersp->m = modersp->m + 1;
    return 2;
}
static int fs68(struct mainRes_s *mrs, struct modersp_s *modersp) 
{ 
    int ret, bitset, len;
    char ch, *addr=0, *dst=0;
    struct sdFAT_s *pfat=0;
    struct supdataBack_s *s=0, *sc=0;

    //sprintf(mrs->log, "%d\n", modersp->v);
    //print_f(&mrs->plog, "fs68", mrs->log);
    pfat = &mrs->aspFat;
    sc = pfat->fatSupcur;

    ret = mrs_ipc_get(mrs, &ch, 1, 1);
    while (ret > 0) {
        if (ch == 'p') {
            modersp->v += 1;
            
            if (sc) {
                len = ring_buf_cons_psudo(&mrs->cmdRx, &addr);
                //sprintf(mrs->log, "1. get psudo len:%d, cnt:%d\n", len, modersp->v);
                //print_f(&mrs->plog, "fs68", mrs->log);

                if (len >= 0) {
                    dst = sc->supdataBuff;
                    memcpy(dst, addr, len);
                    sc->supdataTot = len;

                    s = aspMalloc(sizeof(struct supdataBack_s));
                    memset(s, 0, sizeof(struct supdataBack_s));
                    sc->n = s;
                    sc = sc->n;

                    pfat->fatSupcur = sc;
                }
            }
            
            mrs_ipc_put(mrs, "n", 1, 3);
        }

        if (ch == 'd') {
            sprintf(mrs->log, "0 %d end\n", modersp->v);
            print_f(&mrs->plog, "fs68", mrs->log);

            mrs_ipc_put(mrs, "n", 1, 3);
            modersp->r |= 0x1;
            //mrs_ipc_put(mrs, "e", 1, 3);
        }
        ret = mrs_ipc_get(mrs, &ch, 1, 1);
    }

    if (modersp->r & 0x1) {
        if (sc) {
            len = ring_buf_cons_psudo(&mrs->cmdRx, &addr);
            while (len >= 0) {
                //sprintf(mrs->log, "2. get psudo len:%d, cnt:%d\n", len, modersp->v);
                //print_f(&mrs->plog, "fs68", mrs->log);

                dst = sc->supdataBuff;
                memcpy(dst, addr, len);
                sc->supdataTot = len;
                
                s = aspMalloc(sizeof(struct supdataBack_s));
                memset(s, 0, sizeof(struct supdataBack_s));
                sc->n = s;
                sc = sc->n;

                pfat->fatSupcur = sc;
                modersp->v += 1;  
                len = ring_buf_cons_psudo(&mrs->cmdRx, &addr);
            }

            len = 0;
            s = pfat->fatSupdata;
            while (s) {
                if (s->supdataTot == 0) {
                    break;
                } else {
                    len += s->supdataTot;
                    //sprintf(mrs->log, "tot/len: %d/%d\n", s->supdataTot, len);
                    //print_f(&mrs->plog, "fs68", mrs->log);
                }
                sc = s;
                s = s->n;
            }

            if (sc) {
                sc->n = 0;
            }

            while (s) {
                sc = s;
                s = s->n;
                
                memset(sc, 0, sizeof(struct supdataBack_s));
                aspFree(sc);
            }
            pfat->fatSupcur = 0;

            ret = aspCalcSupLen(pfat->fatSupdata);
        }

        modersp->c = 0;
        mrs_ipc_put(mrs, "N", 1, 3);
        sprintf(mrs->log, "%d end, len: %d, calcu: %d\n", modersp->v, len, ret);
        print_f(&mrs->plog, "fs68", mrs->log);
        modersp->m = modersp->m + 1;

        mrs->mchine.cur.opinfo = modersp->v;
        
        return 2;
    }

    return 0; 
}
static int fs69(struct mainRes_s *mrs, struct modersp_s *modersp) 
{ 
    int len=0, bitset=0, ret=0;
    char ch=0;
    struct info16Bit_s *p;

    sprintf(mrs->log, "wait wifi tx end\n");
    print_f(&mrs->plog, "fs69", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    while (len > 0) {

        //sprintf(mrs->log, "ch: %c - end\n", ch);
        //print_f(&mrs->plog, "fs69", mrs->log);
        modersp->c ++;
        
        if (ch == 'N') {
            sprintf(mrs->log, "ch: %c - end, count: %d\n", ch, modersp->c);
            print_f(&mrs->plog, "fs69", mrs->log);

            ring_buf_init(&mrs->cmdRx);

#if SPI_KTHREAD_USE
            bitset = 0;
            ret = msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
            sprintf(mrs->log, "Stop spi0 spidev thread, ret: 0x%x\n", ret);
            print_f(&mrs->plog, "fs69", mrs->log);
#endif

#if PULL_LOW_AFTER_DATA
            bitset = 0;
            msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
            sprintf(mrs->log, "set RDY pin %d\n",bitset);
            print_f(&mrs->plog, "fs69", mrs->log);
#endif
            usleep(210000);

            modersp->r = 1;            
            return 1;
        }
        len = mrs_ipc_get(mrs, &ch, 1, 3);
    }
    return 0; 
}

static int fs70(struct mainRes_s *mrs, struct modersp_s *modersp) 
{
    int val=0, i=0, ret=0;
    char *pr=0;
    uint32_t secStr=0, secLen=0;
    struct aspConfig_s *pct=0;
    struct sdbootsec_s   *psec=0;
    struct sdFAT_s *pfat=0;
    struct sdParseBuff_s *pParBuf=0;
    struct info16Bit_s *p=0, *c=0;
    struct directnFile_s *curDir=0, *ch=0, *br=0;
    struct folderQueue_s *pfhead=0, *pfdirt=0, *pfnext=0;
    struct adFATLinkList_s *pflsh=0, *pflnt=0;
    struct sdFATable_s   *pftb=0;
    
    c = &mrs->mchine.cur;
    p = &mrs->mchine.tmp;
    
    pct = mrs->configTable;
    pfat = &mrs->aspFat;
    pParBuf = &pfat->fatDirPool->parBuf;
    psec = pfat->fatBootsec;
    pftb = pfat->fatTable;

    sprintf(mrs->log, "download file: %s \n", pfat->fatFileDnld->dfSFN);
    print_f(&mrs->plog, "fs70", mrs->log);

    if (!pfat->fatFileDnld) {
        modersp->r = 2;
        return 1;
    }
   
    curDir = pfat->fatFileDnld;
    if (!pftb->h) {
        pflsh = 0;

        ret = mspSD_parseFAT2LinkList(&pflsh, curDir->dfclstnum, pftb->ftbFat1, (psec->secTotal - psec->secWhroot) / psec->secPrClst);
        if (ret) {
            sprintf(mrs->log, "FAT table parsing for root dictionary FAIL!!ret:%d (%s)\n", ret, curDir->dfSFN);
            print_f(&mrs->plog, "fs70", mrs->log);
            modersp->r = 3;
            return 1;
        }
        /* debug */
        sprintf(mrs->log, "show FAT link for [%s]:\n", curDir->dfSFN);
        print_f(&mrs->plog, "fs70", mrs->log);

        pflnt = pflsh;
        while (pflnt) {
            sprintf(mrs->log, "    str:%d len:%d\n", pflnt->ftStart, pflnt->ftLen);
            print_f(&mrs->plog, "fs70", mrs->log);
            pflnt = pflnt->n;
        }
        pftb->h = pflsh;
        pftb->c = pftb->h;

        pfat->fatStatus |= ASPFAT_STATUS_SDRD;
        modersp->r = 1;
    } else {
        sprintf(mrs->log, "FAT table parsing for root dictionary FAIL!!ret:%d \n", ret);
        print_f(&mrs->plog, "fs70", mrs->log);
        modersp->r = 2;
    }

    return 1;
}

static int fs71(struct mainRes_s *mrs, struct modersp_s *modersp) 
{
    int val=0, i=0, ret=0;
    char *pr=0;
    uint32_t secStr=0, secLen=0, fstsec=0, lstsec;
    struct aspConfig_s *pct=0;
    struct sdbootsec_s   *psec=0;
    struct sdFAT_s *pfat=0;
    struct sdParseBuff_s *pParBuf=0;
    struct info16Bit_s *p=0, *c=0;
    struct directnFile_s *curDir=0, *ch=0, *br=0;
    struct folderQueue_s *pfhead=0, *pfdirt=0, *pfnext=0;
    struct adFATLinkList_s *pflsh=0, *pflnt=0;
    struct sdFATable_s   *pftb=0;


    c = &mrs->mchine.cur;
    p = &mrs->mchine.tmp;
    
    pct = mrs->configTable;
    pfat = &mrs->aspFat;
    pParBuf = &pfat->fatDirPool->parBuf;
    psec = pfat->fatBootsec;
    pftb = pfat->fatTable;

    curDir = pfat->fatFileDnld;
    if (!curDir) {
        sprintf(mrs->log, "get SD cur failed\n");
        print_f(&mrs->plog, "fs71", mrs->log);

        modersp->r = 0xed;
        return 1;
    }

    sprintf(mrs->log, "get SD cur:0x%.8x filename:[%s]length[%d]\n", pftb->c, (curDir->dflen==0)?curDir->dfSFN:curDir->dfLFN, curDir->dflength);
    print_f(&mrs->plog, "fs71", mrs->log);

    if (pftb->c) {
        pflnt = pftb->c;
                 
        secStr = (pflnt->ftStart - 2) * psec->secPrClst + psec->secWhroot;

        if (!pflnt->n) {
            if (!(curDir->dflength % 512)) {
                fstsec = curDir->dflength / 512;
            } else {
                fstsec = (curDir->dflength / 512) + 1;
            }
            sprintf(mrs->log, "fstsec: %d\n", fstsec);
            print_f(&mrs->plog, "fs71", mrs->log);

            if (!(fstsec % psec->secPrClst) ) {
                lstsec = psec->secPrClst;
            } else {
                lstsec = fstsec % psec->secPrClst;
            }
            sprintf(mrs->log, "lstsec: %d\n", lstsec);
            print_f(&mrs->plog, "fs71", mrs->log);
            
            secLen = (pflnt->ftLen - 1) * psec->secPrClst + lstsec;
        } else {
            secLen = pflnt->ftLen * psec->secPrClst;
        }

        c->opinfo = secStr;
        p->opinfo = secLen;

        if (secLen < 16) secLen = 16;

        sprintf(mrs->log, "set secStart:%d, secLen:%d \n", secStr, secLen);
        print_f(&mrs->plog, "fs71", mrs->log);

        cfgTableSet(pct, ASPOP_SDFAT_RD, 1);

        val = cfgValueOffset(secStr, 24);
        cfgTableSet(pct, ASPOP_SDFAT_STR01, val);
        val = cfgValueOffset(secStr, 16);
        cfgTableSet(pct, ASPOP_SDFAT_STR02, val);
        val = cfgValueOffset(secStr, 8);
        cfgTableSet(pct, ASPOP_SDFAT_STR03, val);
        val = cfgValueOffset(secStr, 0);
        cfgTableSet(pct, ASPOP_SDFAT_STR04, val);
        val = cfgValueOffset(secLen, 24);
        cfgTableSet(pct, ASPOP_SDFAT_LEN01, val);
        val = cfgValueOffset(secLen, 16);
        cfgTableSet(pct, ASPOP_SDFAT_LEN02, val);
        val = cfgValueOffset(secLen, 8);
        cfgTableSet(pct, ASPOP_SDFAT_LEN03, val);
        val = cfgValueOffset(secLen, 0);
        cfgTableSet(pct, ASPOP_SDFAT_LEN04, val);

        cfgTableSet(pct, ASPOP_SDFAT_SDAT, 1);
        
        modersp->r = 2;

        pftb->c = pflnt->n;
        aspFree(pflnt);

    }else {
        pftb->h = 0;
        pfat->fatStatus &= ~ASPFAT_STATUS_SDRD;    
        pfat->fatFileDnld = 0;
        modersp->r = 1;
    }

    return 1;
}

static int fs72(struct mainRes_s *mrs, struct modersp_s *modersp) 
{ 
    int bitset, ret;
    sprintf(mrs->log, "trigger spi0\n");
    print_f(&mrs->plog, "fs72", mrs->log);

#if SPI_KTHREAD_USE
    bitset = 0;
    ret = msp_spi_conf(mrs->sfm[0], _IOR(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_START_THREAD
    sprintf(mrs->log, "Start spi0 spidev thread, ret: 0x%x\n", ret);
    print_f(&mrs->plog, "fs72", mrs->log);
#endif

    ring_buf_init(&mrs->cmdRx);

    mrs_ipc_put(mrs, "n", 1, 1);
    clock_gettime(CLOCK_REALTIME, &mrs->time[0]);

    modersp->m = modersp->m + 1;
    return 2;
}
static int fs73(struct mainRes_s *mrs, struct modersp_s *modersp) 
{ 
    int ret, bitset;
    char ch;

    //sprintf(mrs->log, "%d\n", modersp->v);
    //print_f(&mrs->plog, "fs73", mrs->log);

    ret = mrs_ipc_get(mrs, &ch, 1, 1);
    while (ret > 0) {
        if (ch == 'p') {
            modersp->v += 1;
            mrs_ipc_put(mrs, "n", 1, 3);
        }

        if (ch == 'd') {
            sprintf(mrs->log, "0 %d end\n", modersp->v);
            print_f(&mrs->plog, "fs73", mrs->log);

            mrs_ipc_put(mrs, "n", 1, 3);
            modersp->r |= 0x1;
            //mrs_ipc_put(mrs, "e", 1, 3);
        }
        ret = mrs_ipc_get(mrs, &ch, 1, 1);
    }

    if (modersp->r & 0x1) {
        mrs_ipc_put(mrs, "N", 1, 3);
        sprintf(mrs->log, "%d end\n", modersp->v);
        print_f(&mrs->plog, "fs73", mrs->log);
        modersp->m = modersp->m + 1;
        return 2;
    }

    return 0; 
}
static int fs74(struct mainRes_s *mrs, struct modersp_s *modersp) 
{ 
    int len=0, bitset=0, ret=0, count=0;
    char ch=0;
    struct info16Bit_s *p;

    sprintf(mrs->log, "wait spi0 tx end\n");
    print_f(&mrs->plog, "fs74", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    while (len > 0) {

        count++;
        if (ch == 'N') {
            sprintf(mrs->log, "ch: %c - end, count: %d\n", ch, count);
            print_f(&mrs->plog, "fs74", mrs->log);

            ring_buf_init(&mrs->cmdRx);

#if SPI_KTHREAD_USE
            bitset = 0;
            ret = msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
            sprintf(mrs->log, "Stop spi0 spidev thread, ret: 0x%x\n", ret);
            print_f(&mrs->plog, "fs74", mrs->log);
#endif
#if PULL_LOW_AFTER_DATA
            bitset = 0;
            msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
            sprintf(mrs->log, "set RDY pin %d\n",bitset);
            print_f(&mrs->plog, "fs74", mrs->log);
#endif
            usleep(210000);

            modersp->m = 48;            
            return 2;
        }

        len = mrs_ipc_get(mrs, &ch, 1, 3);
        
    }
    return 0; 
}

static int fs75(struct mainRes_s *mrs, struct modersp_s *modersp) 
{
    int val=0, i=0, ret=0;
    char *pr=0;
    uint32_t secStr=0, secLen=0, clstByte=0, clstLen=0, freeClst=0, usedClst=0, totClst=0;
    struct aspConfig_s *pct=0;
    struct sdbootsec_s   *psec=0;
    struct sdFAT_s *pfat=0;
    struct sdParseBuff_s *pParBuf=0;
    struct info16Bit_s *p=0, *c=0;
    struct directnFile_s *curDir=0, *ch=0, *br=0;
    struct folderQueue_s *pfhead=0, *pfdirt=0, *pfnext=0;
    struct adFATLinkList_s *pflsh=0, *pflnt=0;
    struct adFATLinkList_s *pfre=0, *pnxf=0, *pclst=0;
    struct sdFATable_s   *pftb=0;
    
    c = &mrs->mchine.cur;
    p = &mrs->mchine.tmp;
    
    pct = mrs->configTable;
    pfat = &mrs->aspFat;
    pParBuf = &pfat->fatDirPool->parBuf;
    psec = pfat->fatBootsec;
    pftb = pfat->fatTable;
    clstByte = psec->secSize * psec->secPrClst;
    if (!clstByte) {
        sprintf(mrs->log, "ERROR!! bytes number of cluster is zero \n");
        print_f(&mrs->plog, "fs75", mrs->log);

        modersp->r = 3;
        return 1;
    }

    pfre = pftb->ftbMng.f;
    if (!pfre) {
        sprintf(mrs->log, "Error!! free space link list is empty \n");
        print_f(&mrs->plog, "fs75", mrs->log);
        modersp->r = 0xed;
        return 1;
    }
    
    if (!pfat->fatFileUpld) {
        modersp->r = 0xed;
        return 1;
    }    

    sprintf(mrs->log, "upload file: %s \n", pfat->fatFileUpld->dfSFN);
    print_f(&mrs->plog, "fs75", mrs->log);
   
    curDir = pfat->fatFileUpld;
    if (!pftb->h) {
        pflsh = 0;

        if (curDir->dflength % clstByte) {
            clstLen = (curDir->dflength / clstByte) + 1;
        } else {
            clstLen = (curDir->dflength / clstByte);        
        }

        sprintf(mrs->log, "needed cluster length: %d \n", clstLen);
        print_f(&mrs->plog, "fs75", mrs->log);
        
        if (clstLen) {
            ret = mspSD_allocFreeFATList(&pflsh, clstLen, pfre, &pnxf);
            if (ret) {
                sprintf(mrs->log, "free FAT table parsing for file upload FAIL!!ret:%d (%s)\n", ret, curDir->dfSFN);
                print_f(&mrs->plog, "fs75", mrs->log);
                modersp->r = 0xed;
                return 1;
            } 
            else {
                freeClst = 0;
                if ((pfre != pnxf) && (pnxf)) {
                    totClst = (psec->secTotal - psec->secWhroot) / psec->secPrClst;

                    while (pfre != pnxf) {
                        pclst = pfre;

                        pfre = pfre->n;

                        sprintf(mrs->log, "free used FREE FAT linklist, 0x%.8x start: %d, length: %d \n", pclst, pclst->ftStart, pclst->ftLen);
                        print_f(&mrs->plog, "fs75", mrs->log);

                        aspFree(pclst);
                        pclst = 0;
                    }
                }

                pflnt = pnxf;
                while (pflnt) {
                    freeClst += pflnt->ftLen;
                    sprintf(mrs->log, "cal start: %d len:%d \n", pflnt->ftStart, pflnt->ftLen);
                    print_f(&mrs->plog, "fs75", mrs->log);
                    pflnt = pflnt->n;
                }

                sprintf(mrs->log, " re-calculate total free cluster: %d \n free sector: %d (size: %d) \n", freeClst, freeClst * psec->secPrClst, freeClst * psec->secPrClst * psec->secSize);
                print_f(&mrs->plog, "fs75", mrs->log);     
                usedClst = totClst - freeClst;

                pftb->ftbMng.ftfreeClst = freeClst;
                pftb->ftbMng.ftusedClst = usedClst;
                pftb->ftbMng.f = pnxf;
            }

            /* debug */
            sprintf(mrs->log, "show allocated FAT list: \n");
            print_f(&mrs->plog, "fs75", mrs->log);

            val = 0;
            pflnt = pflsh;
            while (pflnt) {
                val += pflnt->ftLen;
                sprintf(mrs->log, "    str:%d len:%d - %d\n", pflnt->ftStart, pflnt->ftLen, val);
                print_f(&mrs->plog, "fs75", mrs->log);
                pflnt = pflnt->n;
            }
            sprintf(mrs->log, "total allocated cluster is %d!! \n", val);
            print_f(&mrs->plog, "fs75", mrs->log);

        
            pftb->h = pflsh;
            pftb->c = pftb->h;

            pfat->fatStatus |= ASPFAT_STATUS_SDWT;
            pfat->fatStatus |= ASPFAT_STATUS_FATWT;
        }else {
            pftb->h = 0;
            pftb->c = 0;
        }

        pfat->fatStatus |= ASPFAT_STATUS_DFECHK;
        pfat->fatStatus |= ASPFAT_STATUS_DFEWT;
        modersp->r = 1;
    } else {
        sprintf(mrs->log, "ERROR!!! header of FAT link list is not empty!! \n");
        print_f(&mrs->plog, "fs75", mrs->log);
        modersp->r = 0xed;
    }

    return 1;
}

static int fs76(struct mainRes_s *mrs, struct modersp_s *modersp) 
{
    int val=0, i=0, ret=0;
    char *pr=0;
    uint32_t secStr=0, secLen=0, fstsec=0, lstsec;
    struct aspConfig_s *pct=0;
    struct sdbootsec_s   *psec=0;
    struct sdFAT_s *pfat=0;
    struct sdParseBuff_s *pParBuf=0;
    struct info16Bit_s *p=0, *c=0;
    struct directnFile_s *curDir=0, *ch=0, *br=0;
    struct folderQueue_s *pfhead=0, *pfdirt=0, *pfnext=0;
    struct adFATLinkList_s *pflsh=0, *pflnt=0;
    struct sdFATable_s   *pftb=0;


    c = &mrs->mchine.cur;
    p = &mrs->mchine.tmp;
    
    pct = mrs->configTable;
    pfat = &mrs->aspFat;
    pParBuf = &pfat->fatDirPool->parBuf;
    psec = pfat->fatBootsec;
    pftb = pfat->fatTable;

    curDir = pfat->fatFileUpld;
    if (!curDir) {
        sprintf(mrs->log, "get SD cur failed\n");
        print_f(&mrs->plog, "fs76", mrs->log);

        modersp->r = 0xed;
        return 1;
    }

    sprintf(mrs->log, "get SD cur:0x%.8x filename:[%s]length[%d]\n", pftb->c, curDir->dfSFN, curDir->dflength);
    print_f(&mrs->plog, "fs76", mrs->log);

    if (pftb->c) {
        pflnt = pftb->c;
                 
        secStr = (pflnt->ftStart - 2) * psec->secPrClst + psec->secWhroot;

        if (!pflnt->n) {
            if (!(curDir->dflength % 512)) {
                fstsec = curDir->dflength / 512;
            } else {
                fstsec = (curDir->dflength / 512) + 1;
            }
            sprintf(mrs->log, "fstsec: %d\n", fstsec);
            print_f(&mrs->plog, "fs76", mrs->log);

            if (!(fstsec % psec->secPrClst) ) {
                lstsec = psec->secPrClst;
            } else {
                lstsec = fstsec % psec->secPrClst;
            }
            sprintf(mrs->log, "lstsec: %d\n", lstsec);
            print_f(&mrs->plog, "fs76", mrs->log);
            
            secLen = (pflnt->ftLen - 1) * psec->secPrClst + lstsec;
        } else {
            secLen = pflnt->ftLen * psec->secPrClst;
        }

        c->opinfo = secStr;
        p->opinfo = secLen;

        if (secLen < 16) secLen = 16;

        sprintf(mrs->log, "set secStart:%d, secLen:%d \n", secStr, secLen);
        print_f(&mrs->plog, "fs76", mrs->log);

        cfgTableSet(pct, ASPOP_SDFAT_WT, 1);

        val = cfgValueOffset(secStr, 24);
        cfgTableSet(pct, ASPOP_SDFAT_STR01, val);
        val = cfgValueOffset(secStr, 16);
        cfgTableSet(pct, ASPOP_SDFAT_STR02, val);
        val = cfgValueOffset(secStr, 8);
        cfgTableSet(pct, ASPOP_SDFAT_STR03, val);
        val = cfgValueOffset(secStr, 0);
        cfgTableSet(pct, ASPOP_SDFAT_STR04, val);
        val = cfgValueOffset(secLen, 24);
        cfgTableSet(pct, ASPOP_SDFAT_LEN01, val);
        val = cfgValueOffset(secLen, 16);
        cfgTableSet(pct, ASPOP_SDFAT_LEN02, val);
        val = cfgValueOffset(secLen, 8);
        cfgTableSet(pct, ASPOP_SDFAT_LEN03, val);
        val = cfgValueOffset(secLen, 0);
        cfgTableSet(pct, ASPOP_SDFAT_LEN04, val);

        cfgTableSet(pct, ASPOP_SDFAT_SDAT, 1);
        
        modersp->r = 3; /*3 is for SDWT*/

        pftb->c = pflnt->n;
        //aspFree(pflnt);

    }else {
        pfat->fatStatus &= ~ASPFAT_STATUS_SDWT;    
        pftb->c = pftb->h;
        //pfat->fatFileUpld = 0;
        //pftb->h = 0;
        modersp->r = 1;
    }

    return 1;
}

static int fs77(struct mainRes_s *mrs, struct modersp_s *modersp) 
{
    int bitset, ret;
    sprintf(mrs->log, "data flow upload to SD\n");
    print_f(&mrs->plog, "fs77", mrs->log);

    sprintf(mrs->log, "trigger spi0\n");
    print_f(&mrs->plog, "fs77", mrs->log);

#if SPI_KTHREAD_USE & SPI_UPD_NO_KTHREAD
    bitset = 0;
    ret = msp_spi_conf(mrs->sfm[0], _IOR(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_START_THREAD
    sprintf(mrs->log, "Start spi0 spidev thread, ret: 0x%x\n", ret);
    print_f(&mrs->plog, "fs77", mrs->log);
#endif

    ring_buf_init(&mrs->cmdTx);

    mrs_ipc_put(mrs, "u", 1, 3);
    clock_gettime(CLOCK_REALTIME, &mrs->time[0]);
    mrs_ipc_put(mrs, "u", 1, 8);
            
    modersp->m = modersp->m + 1;
    return 2;
}

static int fs78(struct mainRes_s *mrs, struct modersp_s *modersp) 
{ 
    int ret, bitset;
    char ch;

    //sprintf(mrs->log, "%d\n", modersp->v);
    //print_f(&mrs->plog, "fs78", mrs->log);

    ret = mrs_ipc_get(mrs, &ch, 1, 3);
    while (ret > 0) {
        if (ch == 'u') {
            modersp->v += 1;
            mrs_ipc_put(mrs, "u", 1, 1);
        } else if (ch == 'h'){
            mrs_ipc_put(mrs, "u", 1, 8);
        }

        if (ch == 'U') {
            sprintf(mrs->log, "0 %d end\n", modersp->v);
            print_f(&mrs->plog, "fs78", mrs->log);

            mrs_ipc_put(mrs, "U", 1, 1);

            mrs_ipc_put(mrs, "U", 1, 8);
            
            modersp->r |= 0x1;
        }
        ret = mrs_ipc_get(mrs, &ch, 1, 3);
    }

    if (modersp->r & 0x1) {
        sprintf(mrs->log, "%d end\n", modersp->v);
        print_f(&mrs->plog, "fs78", mrs->log);
        modersp->m = modersp->m + 1;
        return 2;
    }

    return 0; 
}

static int fs79(struct mainRes_s *mrs, struct modersp_s *modersp) 
{ 
    int len=0, bitset=0, ret=0;
    char ch=0;
    struct info16Bit_s *p;

    //sprintf(mrs->log, "wait spi0 tx end\n");
    //print_f(&mrs->plog, "fs79", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if (len > 0) {

        sprintf(mrs->log, "ch: %c - end\n", ch);
        print_f(&mrs->plog, "fs79", mrs->log);

        if (ch == 'U') {

            ring_buf_init(&mrs->cmdTx);

#if SPI_KTHREAD_USE & SPI_UPD_NO_KTHREAD
            bitset = 0;
            ret = msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
            sprintf(mrs->log, "Stop spi0 spidev thread, ret: 0x%x\n", ret);
            print_f(&mrs->plog, "fs79", mrs->log);
#endif
#if PULL_LOW_AFTER_DATA
            bitset = 0;
            msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
            sprintf(mrs->log, "set RDY pin %d\n",bitset);
            print_f(&mrs->plog, "fs79", mrs->log);
#endif
            usleep(210000);

            modersp->m = 48;            
            return 2;
        } else  {
            //modersp->r = 2;
            //modersp->c += 1;
            return 0;
        }
    }
    return 0; 
}

static int fs80(struct mainRes_s *mrs, struct modersp_s *modersp) 
{
    FILE *f=0;
    char fatPath[128] = "/mnt/mmc2/fatTab.bin";

    int val=0, i=0, ret=0;
    char *pr=0;
    uint32_t secStr=0, secLen=0, fstsec=0, lstsec;
    struct aspConfig_s *pct=0;
    struct sdbootsec_s   *psec=0;
    struct sdFAT_s *pfat=0;
    struct sdParseBuff_s *pParBuf=0;
    struct info16Bit_s *p=0, *c=0;
    struct directnFile_s *curDir=0, *ch=0, *br=0;
    struct folderQueue_s *pfhead=0, *pfdirt=0, *pfnext=0;
    struct adFATLinkList_s *pflsh=0, *pflnt=0;
    struct sdFATable_s   *pftb=0;

    c = &mrs->mchine.cur;
    p = &mrs->mchine.tmp;
    
    pct = mrs->configTable;
    pfat = &mrs->aspFat;
    pParBuf = &pfat->fatDirPool->parBuf;
    psec = pfat->fatBootsec;
    pftb = pfat->fatTable;
    sprintf(mrs->log, "FAT table upload to SD\n");
    print_f(&mrs->plog, "fs80", mrs->log);

    curDir = pfat->fatFileUpld;
    if (pftb->c) {
        pflnt = pftb->c;

        ret = mspSD_updLocalFAT(pflnt, pftb->ftbFat1, pftb->ftbLen);
        if (ret) {
            sprintf(mrs->log, "update local FAT failed!!! ret: %d \n", ret);
            print_f(&mrs->plog, "fs80", mrs->log);
        }

        f = fopen(fatPath, "w+");

        if (f) {
            msync(pftb->ftbFat1, pftb->ftbLen, MS_SYNC);
            fwrite(pftb->ftbFat1, 1, pftb->ftbLen, f);
            fflush(f);
            fclose(f);
            sprintf(mrs->log, "FAT table save to [%s] size:%d\n", fatPath, pftb->ftbLen);
            print_f(&mrs->plog, "fs80", mrs->log);
        } else {
            sprintf(mrs->log, "FAT table find save to [%s] failed !!!\n", fatPath);
            print_f(&mrs->plog, "fs80", mrs->log);
        }

        secStr = 0; secLen = 0;
        ret = mspSD_rangeFATLinkList(pflnt, &fstsec, &lstsec);
        if (ret) {
            sprintf(mrs->log, "find range of FAT table failed ret:%d, secStr: %d, secLen: %d\n", ret, fstsec, lstsec);
            print_f(&mrs->plog, "fs80", mrs->log);    
            fstsec = 2;
            lstsec = psec->secPrfat;
        }

        sprintf(mrs->log, "FAT table upload to SD, fstsec: %d, lstsec: %d (FAT offset)\n", fstsec, lstsec);
        print_f(&mrs->plog, "fs80", mrs->log);

        fstsec = (fstsec * 4) / 512;
        lstsec = ((lstsec * 4) / 512) + 1;
        
        sprintf(mrs->log, "FAT table upload to SD, fstsec: %d, lstsec: %d (sector)\n", fstsec, lstsec);
        print_f(&mrs->plog, "fs80", mrs->log);
/*
        val = SPI_TRUNK_SZ / 512;
        if ((lstsec % val) < 16) {
            lstsec = lstsec + (16 - (lstsec % val));
        }
        
        sprintf(mrs->log, "FAT table upload to SD, fstsec: %d, lstsec: %d - 3\n", fstsec, lstsec);
        print_f(&mrs->plog, "fs80", mrs->log);
*/
        secStr = psec->secWhfat + fstsec;
        secLen = lstsec - fstsec;

        c->opinfo = secStr;
        p->opinfo = secLen;
        
        sprintf(mrs->log, "set secStart:%d, secLen:%d \n", secStr, secLen);
        print_f(&mrs->plog, "fs80", mrs->log);

        if (secLen < 16) secLen = 16;

        cfgTableSet(pct, ASPOP_SDFAT_WT, 1);

        val = cfgValueOffset(secStr, 24);
        cfgTableSet(pct, ASPOP_SDFAT_STR01, val);
        val = cfgValueOffset(secStr, 16);
        cfgTableSet(pct, ASPOP_SDFAT_STR02, val);
        val = cfgValueOffset(secStr, 8);
        cfgTableSet(pct, ASPOP_SDFAT_STR03, val);
        val = cfgValueOffset(secStr, 0);
        cfgTableSet(pct, ASPOP_SDFAT_STR04, val);
        val = cfgValueOffset(secLen, 24);
        cfgTableSet(pct, ASPOP_SDFAT_LEN01, val);
        val = cfgValueOffset(secLen, 16);
        cfgTableSet(pct, ASPOP_SDFAT_LEN02, val);
        val = cfgValueOffset(secLen, 8);
        cfgTableSet(pct, ASPOP_SDFAT_LEN03, val);
        val = cfgValueOffset(secLen, 0);
        cfgTableSet(pct, ASPOP_SDFAT_LEN04, val);

        cfgTableSet(pct, ASPOP_SDFAT_SDAT, 1);
        
        modersp->r = 3; /*3 is for SDWT*/

        pftb->c = 0;
    }else {
        pfat->fatStatus &= ~ASPFAT_STATUS_FATWT;
        //curDir->dfstats = ASPFS_STATUS_EN;
        //pfat->fatFileUpld = 0;
        modersp->r = 1;
    }

    return 1;
}

static int fs81(struct mainRes_s *mrs, struct modersp_s *modersp) 
{
    FILE *f=0;
    char clstPath[128] = "/mnt/mmc2/clstNew.bin";
    int val=0, i=0, ret=0, fLen=0, len=0;
    uint8_t *pdef=0;
    uint32_t secStr=0, secLen=0, fstsec=0, lstsec, freeClst=0, usedClst=0, totClst=0;
    struct aspConfig_s *pct=0;
    struct sdbootsec_s   *psec=0;
    struct sdFAT_s *pfat=0;
    struct sdParseBuff_s *pParBuf=0;
    struct info16Bit_s *p=0, *c=0;
    struct directnFile_s *curDir=0, *ch=0, *br=0, *pa=0;
    struct folderQueue_s *pfhead=0, *pfdirt=0, *pfnext=0;
    struct adFATLinkList_s *pflsh=0, *pflnt=0;
    struct adFATLinkList_s *padd=0;
    struct sdFATable_s   *pftb=0;
    struct adFATLinkList_s *pfre=0, *pnxf=0, *pclst;    

    c = &mrs->mchine.cur;
    p = &mrs->mchine.tmp;
    
    pct = mrs->configTable;
    pfat = &mrs->aspFat;
    pParBuf = &pfat->fatDirPool->parBuf;
    psec = pfat->fatBootsec;
    pftb = pfat->fatTable;
    
    sprintf(mrs->log, "DFE read from SD (%s)\n", pfat->fatFileUpld->dfSFN);
    print_f(&mrs->plog, "fs81", mrs->log);

    curDir = pfat->fatFileUpld;
    if (!curDir) {
        sprintf(mrs->log, "DFE read from SD\n");
        print_f(&mrs->plog, "fs81", mrs->log);
        modersp->r = 0xed;
        return 1;
    }
    
    if (mrs->folder_dirt) {
        pfdirt = mrs->folder_dirt;
        pa = pfdirt->fdObj;
        
        if (pParBuf->dirBuffUsed) {
            sprintf(mrs->log, "get parse buffer used size: %d]\n", pParBuf->dirBuffUsed);
            print_f(&mrs->plog, "fs81", mrs->log);

            msync(pParBuf->dirParseBuff, pParBuf->dirBuffUsed, MS_SYNC);
            
            /* fill the DEF */
            
            /* find the free space, slot unit is 32 bytes */
            fLen = aspFindFreeDEF(&pdef, pParBuf->dirParseBuff, pParBuf->dirBuffUsed, 32);

            /* debug */
            if (fLen > 0) {
                //shmem_dump(pParBuf->dirParseBuff + (((pParBuf->dirBuffUsed - fLen) > 512)?(pParBuf->dirBuffUsed - fLen - 512):(pParBuf->dirBuffUsed - fLen)), fLen+512);
            }
            
            /* calculate the DEF */                 
            len = aspCompirseDEF(pdef, curDir);
            if (len > 0) {
                sprintf(mrs->log, "compirse DEF len:%d(free:%d)\n", len, fLen);
                print_f(&mrs->plog, "fs81", mrs->log);

                //shmem_dump(pdef, len);
            } else {
                sprintf(mrs->log, "ERROR!!! compirse DEF failed, ret len:%d(free:%d)\n", len, fLen);
                print_f(&mrs->plog, "fs81", mrs->log);
            }

            f = fopen(clstPath, "w+");
            if (f) {
                msync(pdef, len, MS_SYNC);
                fwrite(pdef, 1, len, f);
                fflush(f);
                fclose(f);
                sprintf(mrs->log, "DFE save to [%s] size:%d\n", clstPath, len);
                print_f(&mrs->plog, "fs81", mrs->log);
                shmem_dump(pdef, len);
            } else {
                sprintf(mrs->log, "FAT table find save to [%s] failed !!!\n", clstPath);
                print_f(&mrs->plog, "fs81", mrs->log);
            }
            
            pflsh = 0;
            ret = mspSD_parseFAT2LinkList(&pflsh, pa->dfclstnum, pftb->ftbFat1, (psec->secTotal - psec->secWhroot) / psec->secPrClst);
            if (ret) {
                sprintf(mrs->log, "FAT table parsing for root dictionary FAIL!!ret:%d (%s)\n", ret, curDir->dfSFN);
                print_f(&mrs->plog, "fs81", mrs->log);
                modersp->r = 0xed;
                return 1;
            }

            /* debug */
            sprintf(mrs->log, "show FAT link for [%s]:\n", pa->dfSFN);
            print_f(&mrs->plog, "fs81", mrs->log);

            pclst = pflsh;
            while (1) {
                sprintf(mrs->log, "    str:%d len:%d for \n", pclst->ftStart, pclst->ftLen, pa->dfSFN);
                print_f(&mrs->plog, "fs81", mrs->log);
                if (!pclst->n) break;
                pnxf = pclst;
                pclst = pclst->n;
                aspFree(pnxf);
            }       
                    
            if ((fLen == -1) || ((fLen > 0) && (len > fLen))) {

                sprintf(mrs->log, "  free:%d, need:%d\n", fLen, len);
                print_f(&mrs->plog, "fs81", mrs->log);
                
                /* no space */
                /* allocate FAT to folder */
                pfre = pftb->ftbMng.f;
                if (!pfre) {
                    sprintf(mrs->log, "Error!! free space link list is empty \n");
                    print_f(&mrs->plog, "fs81", mrs->log);
                    modersp->r = 0xed;
                    return 1;
                }

                sprintf(mrs->log, "folder allocate new cluster for file: [%s] \n", curDir->dfSFN);
                print_f(&mrs->plog, "fs81", mrs->log);

                if (!pftb->h) {
                    padd = 0;

                    pnxf = 0;
                    ret = mspSD_allocFreeFATList(&padd, 1, pfre, &pnxf);
                    if (ret) {
                        sprintf(mrs->log, "ERROR!!! free FAT table parsing for file upload FAIL!!ret:%d (%s)\n", ret, curDir->dfSFN);
                        print_f(&mrs->plog, "fs81", mrs->log);
                        modersp->r = 0xed;
                        return 1;
                    } 

                    else {

                        freeClst = 0;
                        if ((pfre != pnxf) && (pnxf)) {
                            totClst = (psec->secTotal - psec->secWhroot) / psec->secPrClst;

                            while (pfre != pnxf) {
                                pclst = pfre;

                                pfre = pfre->n;

                                sprintf(mrs->log, "free used FAT linklist, 0x%.8x start: %d, length: %d \n", pclst, pclst->ftStart, pclst->ftLen);
                                print_f(&mrs->plog, "fs81", mrs->log);

                                aspFree(pclst);
                                pclst = 0;
                            }
                        }

                        pflnt = pnxf;
                        while (pflnt) {
                            freeClst += pflnt->ftLen;
                            sprintf(mrs->log, "cal start: %d len:%d \n", pflnt->ftStart, pflnt->ftLen);
                            print_f(&mrs->plog, "fs81", mrs->log);
                            pflnt = pflnt->n;
                        }

                        sprintf(mrs->log, " re-calculate total free cluster: %d \n free sector: %d (size: %d) \n", freeClst, freeClst * psec->secPrClst, freeClst * psec->secPrClst * psec->secSize);
                        print_f(&mrs->plog, "fs81", mrs->log);     
                        usedClst = totClst - freeClst;

                        pftb->ftbMng.ftfreeClst = freeClst;
                        pftb->ftbMng.ftusedClst = usedClst;
                        pftb->ftbMng.f = pnxf;

                    }

                    /* debug */
                    sprintf(mrs->log, "show allocated free FAT list: \n");
                    print_f(&mrs->plog, "fs81", mrs->log);

                    val = 0;
                    pflnt = padd;
                    while (pflnt) {
                        val += pflnt->ftLen;
                        sprintf(mrs->log, "    str:%d len:%d - %d\n", pflnt->ftStart, pflnt->ftLen, val);
                        print_f(&mrs->plog, "fs81", mrs->log);
                        pflnt = pflnt->n;
                    }
                    sprintf(mrs->log, "total allocated cluster is %d!! \n", val);
                    print_f(&mrs->plog, "fs81", mrs->log);
                    
                    pclst->n = padd; 
                }else {
                    aspFree(pclst);
                    sprintf(mrs->log, "ERROR!!! pftb->h != 0, 0x%x\n", pftb->h);
                    print_f(&mrs->plog, "fs81", mrs->log);
                    modersp->r = 0xed;
                    return 1;
                }
                
                /* enable FAT update flag */
                pfat->fatStatus |= ASPFAT_STATUS_FATWT;   
            }

            /* for cluster DEF update */
            pftb->h = pclst;
            pftb->c = pftb->h;

            memset(pParBuf->dirParseBuff, 0, pParBuf->dirBuffMax);            
            pParBuf->dirBuffUsed = 0;
            pfat->fatStatus &= ~ASPFAT_STATUS_DFECHK;   
            aspFree(pfdirt);         
            mrs->folder_dirt = 0;
            modersp->r = 1;
        } else {
            sprintf(mrs->log, "Size of used parse buffer should not be zero, folder[%s]\n", pa->dfSFN);
            print_f(&mrs->plog, "fs81", mrs->log);
            
            aspFree(pfdirt);            
            mrs->folder_dirt = 0;
            modersp->r = 0xed;
        }
    } else {

        if (pftb->h) {
            pflnt = pftb->h;
            curDir->dfclstnum = pflnt->ftStart;
            sprintf(mrs->log, "set upload file [%s] start cluster: %d, size:%d\n", curDir->dfSFN, curDir->dfclstnum, curDir->dflength);
            print_f(&mrs->plog, "fs81", mrs->log);

            while (pflnt) {
                pflsh = pflnt;
                pflnt = pflnt->n;
                aspFree(pflsh);
            }
            pftb->h = 0;
        }

        pa = curDir->pa;
        pfdirt = aspMalloc(sizeof(struct folderQueue_s));
        pfdirt->fdObj = pa;
        pfdirt->fdnxt = 0;

        if (!pftb->h) {
            pflsh = 0;

            ret = mspSD_parseFAT2LinkList(&pflsh, pa->dfclstnum, pftb->ftbFat1, (psec->secTotal - psec->secWhroot) / psec->secPrClst);
            if (ret) {
                sprintf(mrs->log, "ERROR!!! FAT table parsing for root dictionary FAIL!!ret:%d (%s)\n", ret, pa->dfSFN);
                print_f(&mrs->plog, "fs81", mrs->log);
                modersp->r = 0xed;
                return 1;
            }
            /* debug */
            sprintf(mrs->log, "show FAT link for [%s]:\n", pa->dfSFN);
            print_f(&mrs->plog, "fs81", mrs->log);

            pflnt = pflsh;
            while (pflnt) {
                sprintf(mrs->log, "    str:%d len:%d\n", pflnt->ftStart, pflnt->ftLen);
                print_f(&mrs->plog, "fs81", mrs->log);
                pflnt = pflnt->n;
            }
            
            pftb->h = pflsh;
            pftb->c = pftb->h;
        }else {
            pflnt = pftb->h;
            sprintf(mrs->log, "ERROR!!! FAT link list head should be zero, str:%d len:%d\n", pflnt->ftStart, pflnt->ftLen);
            print_f(&mrs->plog, "fs81", mrs->log);

            modersp->r = 0xed;
            aspFree(pfdirt);            
            return 1;
        }

        /* goto the last cluster */
        pflnt = pftb->h;
        while (pflnt->n) {
            pflnt = pflnt->n;
            sprintf(mrs->log, "goto next FAT list str:%d len:%d\n", pflnt->ftStart, pflnt->ftLen);
            print_f(&mrs->plog, "fs81", mrs->log);
        }

        sprintf(mrs->log, "last FAT list str:%d len:%d\n", pflnt->ftStart, pflnt->ftLen);
        print_f(&mrs->plog, "fs81", mrs->log);

        sprintf(mrs->log, "[%d x %d + %d] \n",pflnt->ftStart - 2, psec->secPrClst, psec->secWhroot);
        print_f(&mrs->plog, "fs81", mrs->log);
                 
        secStr = (pflnt->ftStart - 2) * (uint32_t)psec->secPrClst + (uint32_t)psec->secWhroot;
        secLen = pflnt->ftLen * (uint32_t)psec->secPrClst;
        //secStr = (curDir->dfclstnum - 2) * psec->secPrClst + psec->secWhroot;
        //secLen = psec->secPrClst;

        c->opinfo = secStr;
        p->opinfo = secLen;

        if (secLen < 16) secLen = 16;

        cfgTableSet(pct, ASPOP_SDFAT_RD, 1);

        val = cfgValueOffset(secStr, 24);
        cfgTableSet(pct, ASPOP_SDFAT_STR01, val);
        val = cfgValueOffset(secStr, 16);
        cfgTableSet(pct, ASPOP_SDFAT_STR02, val);
        val = cfgValueOffset(secStr, 8);
        cfgTableSet(pct, ASPOP_SDFAT_STR03, val);
        val = cfgValueOffset(secStr, 0);
        cfgTableSet(pct, ASPOP_SDFAT_STR04, val);
        val = cfgValueOffset(secLen, 24);
        cfgTableSet(pct, ASPOP_SDFAT_LEN01, val);
        val = cfgValueOffset(secLen, 16);
        cfgTableSet(pct, ASPOP_SDFAT_LEN02, val);
        val = cfgValueOffset(secLen, 8);
        cfgTableSet(pct, ASPOP_SDFAT_LEN03, val);
        val = cfgValueOffset(secLen, 0);
        cfgTableSet(pct, ASPOP_SDFAT_LEN04, val);

        cfgTableSet(pct, ASPOP_SDFAT_SDAT, 1);

        modersp->r = 2;

        /* goto the last cluster */
        sprintf(mrs->log, "free FAT link for [%s]:\n", curDir->dfSFN);
        print_f(&mrs->plog, "fs81", mrs->log);

        pflnt = pftb->h;
        while (pflnt) {
            sprintf(mrs->log, "    str:%d len:%d\n", pflnt->ftStart, pflnt->ftLen);
            print_f(&mrs->plog, "fs81", mrs->log);
            pflsh = pflnt;
            pflnt = pflnt->n;
            aspFree(pflsh);
        }
        pftb->h = 0;
        
        mrs->folder_dirt = pfdirt;
        pParBuf->dirBuffUsed = 0;
        memset(pParBuf->dirParseBuff, 0, pParBuf->dirBuffMax);
    }

    return 1;
}

static int fs82(struct mainRes_s *mrs, struct modersp_s *modersp) 
{ 
    int bitset, ret;
    sprintf(mrs->log, "trigger spi0\n");
    print_f(&mrs->plog, "fs82", mrs->log);

#if SPI_KTHREAD_USE & SPI_UPD_NO_KTHREAD
    bitset = 0;
    ret = msp_spi_conf(mrs->sfm[0], _IOR(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_START_THREAD
    sprintf(mrs->log, "Start spi0 spidev thread, ret: 0x%x\n", ret);
    print_f(&mrs->plog, "fs82", mrs->log);
#endif

    mrs_ipc_put(mrs, "f", 1, 1);

    modersp->m = modersp->m + 1;
    return 2;
}

static int fs83(struct mainRes_s *mrs, struct modersp_s *modersp) 
{ 
    int len=0, bitset=0, ret=0;
    char ch=0;
    struct info16Bit_s *p;

    //sprintf(mrs->log, "wait spi0 tx end\n");
    //print_f(&mrs->plog, "fs83", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if (len > 0) {

        sprintf(mrs->log, "ch: %c - end\n", ch);
        print_f(&mrs->plog, "fs83", mrs->log);

        if (ch == 'F') {

#if SPI_KTHREAD_USE & SPI_UPD_NO_KTHREAD
            bitset = 0;
            ret = msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
            sprintf(mrs->log, "Stop spi0 spidev thread, ret: 0x%x\n", ret);
            print_f(&mrs->plog, "fs83", mrs->log);
#endif
#if PULL_LOW_AFTER_DATA
            bitset = 0;
            msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
            sprintf(mrs->log, "set RDY pin %d\n",bitset);
            print_f(&mrs->plog, "fs83", mrs->log);
#endif
            usleep(210000);

            modersp->m = 48;            
            return 2;
        } else {
            modersp->r = 2;
            return 1;
        }
    }
    return 0; 
}

static int fs84(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    struct info16Bit_s *p;
    p = &mrs->mchine.cur;

    p->opcode = OP_SAVE;
    p->data = 0;

    //sprintf(mrs->log, "set opcode OP_SAVE: 0x%.2x 0x%.2x \n", p->opcode, p->data);
    //print_f(&mrs->plog, "fs84", mrs->log);
    
    mrs_ipc_put(mrs, "c", 1, 1);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs85(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        //sprintf(mrs->log, "get opcode 0x%.2x 0x%.2x \n", p->opcode, p->data);
        //print_f(&mrs->plog, "fs85", mrs->log);

        if (p->opcode == OP_QRY) {
            modersp->m = modersp->m + 1;
            return 2;
        } else {
            modersp->r = 2;
            return 1;
        }
    }
    return 0; 
}

static int fs86(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    struct info16Bit_s *p;
    p = &mrs->mchine.cur;

    p->opcode = OP_SAVE;
    p->data = 0;

    //sprintf(mrs->log, "set opcode OP_SAVE: 0x%.2x 0x%.2x \n", p->opcode, p->data);
    //print_f(&mrs->plog, "fs86", mrs->log);
    
    mrs_ipc_put(mrs, "c", 1, 1);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs87(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        //sprintf(mrs->log, "get opcode 0x%.2x 0x%.2x \n", p->opcode, p->data);
        //print_f(&mrs->plog, "fs87", mrs->log);

        if (p->opcode == OP_SAVE) {
            modersp->r = 1;
            return 1;
        } else {
            modersp->r = 2;
            return 1;
        }
    }
    return 0; 
}

static int fs88(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    FILE *f=0;
    char clstPath[128] = "/mnt/mmc2/clstNew.bin";
    
    uint8_t *pdef=0;
    int val=0, i=0, ret=0, fLen=0, len=0;
    char *pr=0, *addr=0;
    uint32_t secStr=0, secLen=0, fstsec=0, lstsec;
    struct aspConfig_s *pct=0;
    struct sdbootsec_s   *psec=0;
    struct sdFAT_s *pfat=0;
    struct sdParseBuff_s *pParBuf=0;
    struct info16Bit_s *p=0, *c=0;
    struct directnFile_s *curDir=0, *ch=0, *br=0, *pa=0;
    struct folderQueue_s *pfhead=0, *pfdirt=0, *pfnext=0;
    struct adFATLinkList_s *pflsh=0, *pflnt=0;
    struct sdFATable_s   *pftb=0;

    c = &mrs->mchine.cur;
    p = &mrs->mchine.tmp;
    
    pct = mrs->configTable;
    pfat = &mrs->aspFat;
    pParBuf = &pfat->fatDirPool->parBuf;
    psec = pfat->fatBootsec;
    pftb = pfat->fatTable;
    
    sprintf(mrs->log, "DFE upload to SD\n");
    print_f(&mrs->plog, "fs88", mrs->log);

    curDir = pfat->fatFileUpld;

    if (pParBuf->dirBuffUsed) {
        pfat->fatStatus &= ~ASPFAT_STATUS_DFERD;
        if (!pftb->c) {
            sprintf(mrs->log, "  pftb->c should not be NULL \n");
            print_f(&mrs->plog, "fs88", mrs->log);

            modersp->r = 0xed;
            return 1;
        }
        pflnt = pftb->c;
        
        msync(pParBuf->dirParseBuff, pParBuf->dirBuffUsed, MS_SYNC);
        //shmem_dump(pParBuf->dirParseBuff, pParBuf->dirBuffUsed);
        /* find the free space, slot unit is 32 bytes */
        fLen = aspFindFreeDEF(&pdef, pParBuf->dirParseBuff, pParBuf->dirBuffUsed, 32);

        /* debug */
        if (fLen > 0) {
            //shmem_dump(pParBuf->dirParseBuff + (((pParBuf->dirBuffUsed - fLen) > 512)?(pParBuf->dirBuffUsed - fLen - 512):(pParBuf->dirBuffUsed - fLen)), fLen+512);
        } else {
            sprintf(mrs->log, "  ERROR!!! cluster has no space! ret:%d \n", fLen);
            print_f(&mrs->plog, "fs88", mrs->log);
            modersp->r = 0xed;
            return 1;
        }

        f = fopen(clstPath, "r");

        ret = fseek(f, 0, SEEK_END);
        if (ret) {
            sprintf(mrs->log, " file seek failed!! ret:%d \n", ret);
            print_f(&mrs->plog, "fs88", mrs->log);
            modersp->r = 0xed;
            return 1;
        } 
                
        len = ftell(f);
        sprintf(mrs->log, " file [%s] size: %d \n", clstPath, len);
        print_f(&mrs->plog, "fs88", mrs->log);

        ret = fseek(f, 0, SEEK_SET);
        if (ret) {
            sprintf(mrs->log, " file seek failed!! ret:%d \n", ret);
            print_f(&mrs->plog, "fs88", mrs->log);
            modersp->r = 0xed;
            return 1;
        }

        pr = aspMalloc(len);
        if (!pr) {
            sprintf(mrs->log, " malloc failed ret: %d \n", pr);
            print_f(&mrs->plog, "fs88", mrs->log);
            modersp->r = 0xed;
            return 1;
        }
        
        ret = fread(pr, 1, len, f);
        fclose(f);

        sprintf(mrs->log, "FAT file read size: %d/%d free:%d\n", ret, len, fLen);
        print_f(&mrs->plog, "fs88", mrs->log);
        shmem_dump(pr, len);
        
        //addr = pParBuf->dirParseBuff + (pParBuf->dirBuffUsed - fLen);

        if (len > fLen) {
            //shmem_dump(pdef, fLen);
            memcpy(pdef, pr,  fLen);
            //shmem_dump(pdef, fLen);

            pr += fLen;
            len -= fLen;
        } else {
            //shmem_dump(pdef, len);
            memcpy(pdef, pr,  len);
            //shmem_dump(pdef, len);
            len = 0;
        }

        if (len) {
            f = fopen(clstPath, "w+");
            if (f) {
                msync(pr, len, MS_SYNC);
                //shmem_dump(pr, len);
                fwrite(pr, 1, len, f);
                fflush(f);
                fclose(f);
                sprintf(mrs->log, "FAT table save to [%s] size:%d\n", clstPath, len);
                print_f(&mrs->plog, "fs81", mrs->log);
            } else {
                sprintf(mrs->log, "FAT table find save to [%s] failed !!!\n", clstPath);
                print_f(&mrs->plog, "fs81", mrs->log);
            }
        } else {
            f = fopen(clstPath, "w+");
            if (f) {
                fflush(f);
                fclose(f);
                sprintf(mrs->log, "FAT table save to [%s] size:%d\n", clstPath, len);
                print_f(&mrs->plog, "fs81", mrs->log);
            } else {
                sprintf(mrs->log, "FAT table find save to [%s] failed !!!\n", clstPath);
                print_f(&mrs->plog, "fs81", mrs->log);
            }
        }

        secStr = (pflnt->ftStart - 2) * (uint32_t)psec->secPrClst + (uint32_t)psec->secWhroot;
        secLen = pflnt->ftLen * (uint32_t)psec->secPrClst;
        
        c->opinfo = secStr;
        p->opinfo = secLen;
        
        sprintf(mrs->log, "set secStart:%d, secLen:%d \n", secStr, secLen);
        print_f(&mrs->plog, "fs88", mrs->log);

        if (secLen < 16) secLen = 16;

        cfgTableSet(pct, ASPOP_SDFAT_WT, 1);

        val = cfgValueOffset(secStr, 24);
        cfgTableSet(pct, ASPOP_SDFAT_STR01, val);
        val = cfgValueOffset(secStr, 16);
        cfgTableSet(pct, ASPOP_SDFAT_STR02, val);
        val = cfgValueOffset(secStr, 8);
        cfgTableSet(pct, ASPOP_SDFAT_STR03, val);
        val = cfgValueOffset(secStr, 0);
        cfgTableSet(pct, ASPOP_SDFAT_STR04, val);
        val = cfgValueOffset(secLen, 24);
        cfgTableSet(pct, ASPOP_SDFAT_LEN01, val);
        val = cfgValueOffset(secLen, 16);
        cfgTableSet(pct, ASPOP_SDFAT_LEN02, val);
        val = cfgValueOffset(secLen, 8);
        cfgTableSet(pct, ASPOP_SDFAT_LEN03, val);
        val = cfgValueOffset(secLen, 0);
        cfgTableSet(pct, ASPOP_SDFAT_LEN04, val);

        cfgTableSet(pct, ASPOP_SDFAT_SDAT, 1);
        
        modersp->r = 3; /*2 is for FAT_WT*/

        pftb->c = pflnt->n;
        pftb->h = 0;
        aspFree(pflnt);
        pParBuf->dirBuffUsed = 0;
    }
    else if (pftb->c) {
        pflnt = pftb->c;

        secStr = (pflnt->ftStart - 2) * (uint32_t)psec->secPrClst + (uint32_t)psec->secWhroot;
        secLen = pflnt->ftLen * (uint32_t)psec->secPrClst;

        if ((!pftb->h) && (!pflnt->n)) {
            pParBuf->dirBuffUsed = secLen * 512;
            memset(pParBuf->dirParseBuff, 0, pParBuf->dirBuffUsed);    
            modersp->r = 1;
        } else {
        
            c->opinfo = secStr;
            p->opinfo = secLen;

            sprintf(mrs->log, "set secStart:%d, secLen:%d \n", secStr, secLen);
            print_f(&mrs->plog, "fs88", mrs->log);

            if (secLen < 16) secLen = 16;

            cfgTableSet(pct, ASPOP_SDFAT_RD, 1);

            val = cfgValueOffset(secStr, 24);
            cfgTableSet(pct, ASPOP_SDFAT_STR01, val);
            val = cfgValueOffset(secStr, 16);
            cfgTableSet(pct, ASPOP_SDFAT_STR02, val);
            val = cfgValueOffset(secStr, 8);
            cfgTableSet(pct, ASPOP_SDFAT_STR03, val);
            val = cfgValueOffset(secStr, 0);
            cfgTableSet(pct, ASPOP_SDFAT_STR04, val);
            val = cfgValueOffset(secLen, 24);
            cfgTableSet(pct, ASPOP_SDFAT_LEN01, val);
            val = cfgValueOffset(secLen, 16);
            cfgTableSet(pct, ASPOP_SDFAT_LEN02, val);
            val = cfgValueOffset(secLen, 8);
            cfgTableSet(pct, ASPOP_SDFAT_LEN03, val);
            val = cfgValueOffset(secLen, 0);
            cfgTableSet(pct, ASPOP_SDFAT_LEN04, val);

            cfgTableSet(pct, ASPOP_SDFAT_SDAT, 1);
        
            modersp->r = 2; /*2 is for FAT_RD*/

            pfat->fatStatus |= ASPFAT_STATUS_DFERD;
            pParBuf->dirBuffUsed = 0;
            memset(pParBuf->dirParseBuff, 0, secLen * 512);
        }
    }else {
        if (pftb->h) {
            sprintf(mrs->log, " BEGIN... \n");
            print_f(&mrs->plog, "fs88", mrs->log);

            pftb->c = pftb->h;
        } else {
            sprintf(mrs->log, " END... \n");
            print_f(&mrs->plog, "fs88", mrs->log);

            pfat->fatStatus &= ~ASPFAT_STATUS_DFEWT;    
            pfat->fatFileUpld = 0;
            curDir->dfstats = ASPFS_STATUS_EN;
            mrs->folder_dirt = 0;
            //curDir->dfstats = ASPFS_STATUS_EN;
            //pfat->fatFileUpld = 0;
        }
        modersp->r = 1;
    }

    return 1;
}
static int fs89(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset, ret;
    sprintf(mrs->log, "trigger spi0\n");
    print_f(&mrs->plog, "fs89", mrs->log);

#if SPI_KTHREAD_USE & SPI_UPD_NO_KTHREAD
    bitset = 0;
    ret = msp_spi_conf(mrs->sfm[0], _IOR(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_START_THREAD
    sprintf(mrs->log, "Start spi0 spidev thread, ret: 0x%x\n", ret);
    print_f(&mrs->plog, "fs89", mrs->log);
#endif

    mrs_ipc_put(mrs, "w", 1, 1);

    modersp->m = modersp->m + 1;
    return 2;
}

static int fs90(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int len=0, bitset=0, ret=0;
    char ch=0;
    struct info16Bit_s *p;

    //sprintf(mrs->log, "wait spi0 tx end\n");
    //print_f(&mrs->plog, "fs90", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if (len > 0) {

        sprintf(mrs->log, "ch: %c - end\n", ch);
        print_f(&mrs->plog, "fs90", mrs->log);

        if (ch == 'W') {

#if SPI_KTHREAD_USE & SPI_UPD_NO_KTHREAD
            bitset = 0;
            ret = msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
            sprintf(mrs->log, "Stop spi0 spidev thread, ret: 0x%x\n", ret);
            print_f(&mrs->plog, "fs90", mrs->log);
#endif
#if PULL_LOW_AFTER_DATA
            bitset = 0;
            msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
            sprintf(mrs->log, "set RDY pin %d\n",bitset);
            print_f(&mrs->plog, "fs90", mrs->log);
#endif
            usleep(210000);

            modersp->m = 48;            
            return 2;
        } else {
            modersp->r = 2;
            return 1;
        }
    }
    return 0; 
}

static int fs91(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int val=0, i=0, ret=0;
    char *pr=0;
    uint32_t secStr=0, secLen=0, clstByte=0, clstLen=0, freeClst=0, usedClst=0, totClst=0;
    struct aspConfig_s *pct=0;
    struct sdbootsec_s   *psec=0;
    struct sdFAT_s *pfat=0;
    struct info16Bit_s *p=0, *c=0;
    struct directnFile_s *curDir=0, *ch=0, *br=0;
    struct adFATLinkList_s *pflsh=0, *pflnt=0;
    struct adFATLinkList_s *pfre=0, *pnxf=0, *pclst=0;
    struct sdFATable_s   *pftb=0;
    
    pct = mrs->configTable;
    pfat = &mrs->aspFat;
    psec = pfat->fatBootsec;
    pftb = pfat->fatTable;
    clstByte = psec->secSize * psec->secPrClst;
    if (!clstByte) {
        sprintf(mrs->log, "ERROR!! bytes number of cluster is zero \n");
        print_f(&mrs->plog, "fs91", mrs->log);

        modersp->r = 2;
        return 1;
    }

    pfre = pftb->ftbMng.f;
    if (!pfre) {
        sprintf(mrs->log, "Error!! free space link list is empty \n");
        print_f(&mrs->plog, "fs91", mrs->log);
        modersp->r = 0xed;
        return 1;
    }

    ret = mspSD_getLastFATList(&pflsh, pfre);
    if (ret) {
        sprintf(mrs->log, "Error!! get last FAT linklist failed ret: %d\n", ret);
        print_f(&mrs->plog, "fs91", mrs->log);
        modersp->r = 2;
        return 1;
    }

    sprintf(mrs->log, "Get last FAT linklist start: %d, length: %d\n", pflsh->ftStart, pflsh->ftLen);
    print_f(&mrs->plog, "fs91", mrs->log);

    pflnt = pflsh;

    secStr = (pflnt->ftStart - 2) * (uint32_t)psec->secPrClst + (uint32_t)psec->secWhroot;
    secLen = pflnt->ftLen * (uint32_t)psec->secPrClst;
        
    sprintf(mrs->log, "set secStart:%d, secLen:%d \n", secStr, secLen);
    print_f(&mrs->plog, "fs91", mrs->log);

    cfgTableSet(pct, ASPOP_SDFREE_FREESEC, psec->secPrClst);

    val = cfgValueOffset(secStr, 24);
    cfgTableSet(pct, ASPOP_SDFREE_STR01, val);
    val = cfgValueOffset(secStr, 16);
    cfgTableSet(pct, ASPOP_SDFREE_STR02, val);
    val = cfgValueOffset(secStr, 8);
    cfgTableSet(pct, ASPOP_SDFREE_STR03, val);
    val = cfgValueOffset(secStr, 0);
    cfgTableSet(pct, ASPOP_SDFREE_STR04, val);
    val = cfgValueOffset(secLen, 24);
    cfgTableSet(pct, ASPOP_SDFREE_LEN01, val);
    val = cfgValueOffset(secLen, 16);
    cfgTableSet(pct, ASPOP_SDFREE_LEN02, val);
    val = cfgValueOffset(secLen, 8);
    cfgTableSet(pct, ASPOP_SDFREE_LEN03, val);
    val = cfgValueOffset(secLen, 0);
    cfgTableSet(pct, ASPOP_SDFREE_LEN04, val);

    modersp->r = 1;
    return 1;
}

static int fs92(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    uint32_t secStr=0, secLen=0, val=0;
    struct aspConfig_s *pct=0;
    struct sdbootsec_s   *psec=0;
    struct sdFAT_s *pfat=0;

    pfat = &mrs->aspFat;
    pct = mrs->configTable;
    psec = pfat->fatBootsec;

    secStr = 0;
    secLen = 0;
        
    sprintf(mrs->log, "set secStart:%d, secLen:%d secPrClst: %d \n", secStr, secLen, psec->secPrClst);
    print_f(&mrs->plog, "fs92", mrs->log);

    cfgTableSet(pct, ASPOP_SDUSED_USEDSEC, psec->secPrClst);

    val = cfgValueOffset(secStr, 24);
    cfgTableSet(pct, ASPOP_SDUSED_STR01, val);
    val = cfgValueOffset(secStr, 16);
    cfgTableSet(pct, ASPOP_SDUSED_STR02, val);
    val = cfgValueOffset(secStr, 8);
    cfgTableSet(pct, ASPOP_SDUSED_STR03, val);
    val = cfgValueOffset(secStr, 0);
    cfgTableSet(pct, ASPOP_SDUSED_STR04, val);
    val = cfgValueOffset(secLen, 24);
    cfgTableSet(pct, ASPOP_SDUSED_LEN01, val);
    val = cfgValueOffset(secLen, 16);
    cfgTableSet(pct, ASPOP_SDUSED_LEN02, val);
    val = cfgValueOffset(secLen, 8);
    cfgTableSet(pct, ASPOP_SDUSED_LEN03, val);
    val = cfgValueOffset(secLen, 0);
    cfgTableSet(pct, ASPOP_SDUSED_LEN04, val);

    modersp->r = 1;

    return 1;
}

static int fs93(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    char fnameSave[16] = "asp%.5d.jpg";
    char srhName[16];
    int ret=0, cnt=0;
    uint32_t secStr=0, secLen=0, clstLen=0, clstStr=0;;
    uint32_t freeClst=0, usedClst=0, totClst=0, val=0, b32=0;
    struct aspConfig_s *pct=0;
    struct sdbootsec_s   *psec=0;
    struct sdFAT_s *pfat=0;
    struct adFATLinkList_s *pflsh=0, *pflnt=0;
    struct adFATLinkList_s *pfre=0, *pnxf=0, *pclst=0;
    struct sdFATable_s   *pftb=0;
    struct directnFile_s *upld=0, *fscur=0, *fssrh=0;

    uint32_t adata[3], atime[3];
    char *wday[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"}; 
    struct tm *p=0;
    time_t timep;
                
    pct = mrs->configTable;
    pfat = &mrs->aspFat;
    psec = pfat->fatBootsec;
    pftb = pfat->fatTable;

    pfre = pftb->ftbMng.f;
    if (!pfre) {
        sprintf(mrs->log, "Error!! free space link list is empty \n");
        print_f(&mrs->plog, "fs93", mrs->log);
        modersp->r = 0xed;
        return 1;
    }

    if (!pfat->fatCurDir) {
        sprintf(mrs->log, "Error!! current folder is null \n");
        print_f(&mrs->plog, "fs93", mrs->log);
        modersp->r = 0xed;
        return 1;
    }

    fscur = pfat->fatCurDir;

    secStr = 0;
    secLen = 0;
            
    cfgTableGet(pct, ASPOP_SDUSED_USEDSEC, &val);
    if (val != psec->secPrClst) {
        sprintf(mrs->log, "ERROR!!! get secPrClst: %d (should be:%d) \n", val, psec->secPrClst);
        print_f(&mrs->plog, "fs93", mrs->log);   
    }

    ret = 0;

    b32 = 0;
    ret += cfgTableGet(pct, ASPOP_SDUSED_STR01, &val);
    b32 |= val << 24;

    ret += cfgTableGet(pct, ASPOP_SDUSED_STR02, &val);
    b32 |= val << 16;
    
    ret += cfgTableGet(pct, ASPOP_SDUSED_STR03, &val);
    b32 |= val << 8;
    
    ret += cfgTableGet(pct, ASPOP_SDUSED_STR04, &val);
    b32 |= val ;
    secStr = b32;

    b32 = 0;
    ret += cfgTableGet(pct, ASPOP_SDUSED_LEN01, &val);
    b32 |= val << 24;
    
    ret += cfgTableGet(pct, ASPOP_SDUSED_LEN02, &val);
    b32 |= val << 16;
    
    ret += cfgTableGet(pct, ASPOP_SDUSED_LEN03, &val);
    b32 |= val << 8;
    
    ret += cfgTableGet(pct, ASPOP_SDUSED_LEN04, &val);
    b32 |= val;
    secLen = b32;

    pflnt = pfre;

    while (pflnt->n) {
        pflnt = pflnt->n;
    }

    clstStr = ((secStr - (uint32_t)psec->secWhroot) / (uint32_t)psec->secPrClst) + 2;

    if ((secLen % (uint32_t)psec->secPrClst) == 0) {
        clstLen = secLen / (uint32_t)psec->secPrClst;
    } else {
        clstLen = (secLen / (uint32_t)psec->secPrClst) + 1;
    }

    sprintf(mrs->log, "Get secStart:%d, secLen:%d, clstStr: %d, clstLen: %d \n", secStr, secLen, clstStr, clstLen);
    print_f(&mrs->plog, "fs93", mrs->log);

    if ((pflnt->ftStart != clstStr) || (clstLen > pflnt->ftLen)) {
        sprintf(mrs->log, "ERROR!!! get clstStart: %d(%d) clstLen: %d(%d) \n", clstStr, pflnt->ftStart, clstLen, pflnt->ftLen);
        print_f(&mrs->plog, "fs93", mrs->log);   
        modersp->r = 2;
        return 1;
    } else {
        pflnt->ftLen -= clstLen;
        pflnt->ftStart += clstLen;
        modersp->r = 1;

        pflnt = pfre;
        while (pflnt) {
            freeClst += pflnt->ftLen;
            sprintf(mrs->log, "cal start: %d len:%d \n", pflnt->ftStart, pflnt->ftLen);
            print_f(&mrs->plog, "fs93", mrs->log);
            pflnt = pflnt->n;
        }

        sprintf(mrs->log, " re-calculate total free cluster: %d \n free sector: %d (size: %d) \n", freeClst, freeClst * psec->secPrClst, freeClst * psec->secPrClst * psec->secSize);
        print_f(&mrs->plog, "fs98", mrs->log);     
        usedClst = totClst - freeClst;

        pftb->ftbMng.ftfreeClst = freeClst;
        pftb->ftbMng.ftusedClst = usedClst;
        pftb->ftbMng.f = pfre;
    }

    ret = mspFS_allocDir(pfat, &upld);
    if (ret) {
         sprintf(mrs->log, "Error!! get new file entry failed ret: %d \n", ret);
        print_f(&mrs->plog, "fs93", mrs->log);
        modersp->r = 0xed;
        return 1;
    }

    memset(upld, 0, sizeof(struct directnFile_s));
    upld->dftype = ASPFS_TYPE_FILE;
    upld->dfstats = ASPFS_STATUS_DIS;
    //upld->dfstats = ASPFS_STATUS_EN;
    upld->dfattrib = ASPFS_ATTR_ARCHIVE;
    upld->dfclstnum = clstStr; /* start cluster */

    time(&timep);
    p=localtime(&timep); /*取得當地時間*/ 
    sprintf(mrs->log, "%.4d%.2d%.2d \n", (1900+p->tm_year),( 1+p-> tm_mon), p->tm_mday); 
    print_f(&mrs->plog, "fs93", mrs->log);
    sprintf(mrs->log, "%s,%.2d:%.2d:%.2d\n", wday[p->tm_wday],p->tm_hour, p->tm_min, p->tm_sec); 
    print_f(&mrs->plog, "fs93", mrs->log);

    adata[0] = p->tm_year+1900;
    adata[1] = p->tm_mon + 1;
    adata[2] = p->tm_mday;
    
    atime[0] = p->tm_hour;
    atime[1] = p->tm_min;
    atime[2] = p->tm_sec;
    
    upld->dfcredate = ((((adata[0] - 1980) & 0xff) << 16) | ((adata[1] & 0xff) << 8) | (adata[2] & 0xff));
    upld->dfcretime = (((atime[0]&0xff) << 16) | ((atime[1]&0xff) << 8) | (atime[2]&0xff));
    upld->dflstacdate = ((((adata[0] - 1980)&0xff) << 16) | ((adata[1]&0xff) << 8) | (adata[2]&0xff));
    upld->dfrecodate = ((((adata[0] - 1980)&0xff) << 16) | ((adata[1]&0xff) << 8) | (adata[2]&0xff));
    upld->dfrecotime = (((atime[0]&0xff) << 16) | ((atime[1]&0xff) << 8) | (atime[2]&0xff));

    upld->dflength = secLen * 512; /* file length */                                                

    /* assign a name with sequence number */
    for (cnt=0; cnt < 10000; cnt++) {
        sprintf(srhName, fnameSave, cnt);
        sprintf(mrs->log, "search name: [%s]\n", srhName);
        print_f(&mrs->plog, "fs93", mrs->log);

        ret = mspFS_SearchInFolder(&fssrh, fscur, srhName);
        if (ret) break;
    }

    strncpy(upld->dfSFN, srhName, 12);
    upld->dfSFN[13] = '\0';

    upld->dflen = 0;
    upld->dfLFN[0] = '\0';

    sprintf(mrs->log, "SFN[%s] LFS[%s] len:%d\n", upld->dfSFN, upld->dfLFN, upld->dflen);
    print_f(&mrs->plog, "fs93", mrs->log);

    ret = mspSD_createFATLinkList(&pclst);
    if (ret) {
        modersp->r = 0xed;
        return 1;
    }
    pclst->ftStart = clstStr;
    pclst->ftLen = clstLen;
    pftb->h = pclst;
    pftb->c = pftb->h;

    aspFS_insertFATChild(fscur, upld);
    pfat->fatFileUpld = upld;
    debugPrintDir(upld);
    mspFS_folderList(upld->pa, 4);

    pfat->fatStatus |= ASPFAT_STATUS_FATWT;
    pfat->fatStatus |= ASPFAT_STATUS_DFECHK;
    pfat->fatStatus |= ASPFAT_STATUS_DFEWT;

    modersp->r = 1;
    return 1;
}

static int fs94(struct mainRes_s *mrs, struct modersp_s *modersp) 
{
    int val=0, i=0, ret=0;
    char *pr=0;
    uint32_t secStr=0, secLen=0, fstsec=0, lstsec;
    struct aspConfig_s *pct=0;
    struct sdbootsec_s   *psec=0;
    struct sdFAT_s *pfat=0;
    struct sdParseBuff_s *pParBuf=0;
    struct info16Bit_s *p=0, *c=0;
    struct directnFile_s *curDir=0, *ch=0, *br=0;
    struct folderQueue_s *pfhead=0, *pfdirt=0, *pfnext=0;
    struct adFATLinkList_s *pflsh=0, *pflnt=0;
    struct sdFATable_s   *pftb=0;


    c = &mrs->mchine.cur;
    p = &mrs->mchine.tmp;
    
    pct = mrs->configTable;
    pfat = &mrs->aspFat;
    pParBuf = &pfat->fatDirPool->parBuf;
    psec = pfat->fatBootsec;
    pftb = pfat->fatTable;

    curDir = pfat->fatFileUpld;
    if (!curDir) {
        sprintf(mrs->log, "get SD cur failed\n");
        print_f(&mrs->plog, "fs94", mrs->log);

        modersp->r = 0xed;
        return 1;
    }

    sprintf(mrs->log, "get SD cur:0x%.8x filename:[%s]length[%d]\n", pftb->c, curDir->dfSFN, curDir->dflength);
    print_f(&mrs->plog, "fs94", mrs->log);

    if (pftb->c) {
        pflnt = pftb->c;
                 
        secStr = (pflnt->ftStart - 2) * psec->secPrClst + psec->secWhroot;

        if (!pflnt->n) {
            if (!(curDir->dflength % 512)) {
                fstsec = curDir->dflength / 512;
            } else {
                fstsec = (curDir->dflength / 512) + 1;
            }
            sprintf(mrs->log, "fstsec: %d\n", fstsec);
            print_f(&mrs->plog, "fs94", mrs->log);

            if (!(fstsec % psec->secPrClst) ) {
                lstsec = psec->secPrClst;
            } else {
                lstsec = fstsec % psec->secPrClst;
            }
            sprintf(mrs->log, "lstsec: %d\n", lstsec);
            print_f(&mrs->plog, "fs94", mrs->log);
            
            secLen = (pflnt->ftLen - 1) * psec->secPrClst + lstsec;
        } else {
            secLen = pflnt->ftLen * psec->secPrClst;
        }

        c->opinfo = secStr;
        p->opinfo = secLen;

        if (secLen < 16) secLen = 16;

        sprintf(mrs->log, "set secStart:%d, secLen:%d \n", secStr, secLen);
        print_f(&mrs->plog, "fs94", mrs->log);

        cfgTableSet(pct, ASPOP_SDFAT_WT, 1);

        val = cfgValueOffset(secStr, 24);
        cfgTableSet(pct, ASPOP_SDFAT_STR01, val);
        val = cfgValueOffset(secStr, 16);
        cfgTableSet(pct, ASPOP_SDFAT_STR02, val);
        val = cfgValueOffset(secStr, 8);
        cfgTableSet(pct, ASPOP_SDFAT_STR03, val);
        val = cfgValueOffset(secStr, 0);
        cfgTableSet(pct, ASPOP_SDFAT_STR04, val);
        val = cfgValueOffset(secLen, 24);
        cfgTableSet(pct, ASPOP_SDFAT_LEN01, val);
        val = cfgValueOffset(secLen, 16);
        cfgTableSet(pct, ASPOP_SDFAT_LEN02, val);
        val = cfgValueOffset(secLen, 8);
        cfgTableSet(pct, ASPOP_SDFAT_LEN03, val);
        val = cfgValueOffset(secLen, 0);
        cfgTableSet(pct, ASPOP_SDFAT_LEN04, val);

        cfgTableSet(pct, ASPOP_SDFAT_SDAT, 1);
        
        modersp->r = 3; /*3 is for SDWT*/

        pftb->c = pflnt->n;
        //aspFree(pflnt);

    }else {
        pfat->fatStatus &= ~ASPFAT_STATUS_SDWBK;    
        pftb->c = pftb->h;
        //pfat->fatFileUpld = 0;
        //pftb->h = 0;
        modersp->r = 1;
    }

    return 1;
}

static int fs95(struct mainRes_s *mrs, struct modersp_s *modersp) 
{
    int bitset, ret;

    sprintf(mrs->log, "trigger spi_0_\n");
    print_f(&mrs->plog, "fs95", mrs->log);

#if SPI_KTHREAD_USE & SPI_UPD_NO_KTHREAD
    bitset = 0;
    ret = msp_spi_conf(mrs->sfm[0], _IOR(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_START_THREAD
    sprintf(mrs->log, "Start spi0 spidev thread, ret: 0x%x\n", ret);
    print_f(&mrs->plog, "fs95", mrs->log);
#else
    sprintf(mrs->log, "NOT start spi0 spidev thread, ret: 0x%x\n", ret);
    print_f(&mrs->plog, "fs95", mrs->log);
#endif

    ring_buf_init(&mrs->cmdTx);
    modersp->v = 0;
 
    modersp->m = modersp->m + 1;
    return 2;
}

static int findJpgScale(uint8_t *data, int *hi, int *wh, int max)
{
    int ret = -1, ix = 0, staf = 0;
    uint8_t marker[2] = {0, 0};
    uint32_t imgLen[2] = {0, 0};
    uint32_t imgWid[2] = {0, 0};
    uint8_t ch = 0;
    int len = 0, width = 0;
    int scale[5][3], si=0;

    memset(scale, 0, sizeof(int) * 15);

    if (!data) return -2;
    if (!max) return -3;
    if (!hi) return -4;
    if (!wh) return -5;
    
    msync(data, max, MS_SYNC);
    
    for (ix=0; ix < max; ix++) {
        ch = data[ix];
    
        if (ch == 0xff) {
            marker[0] = ch;
            staf = 1;
        } 
        else if (staf == 1) {
            if (((ch >> 4) == 0xc) && ((ch & 0xf) != 4)) {
                marker[1] = ch;
                
                imgLen[1] = data[ix + 4];
                imgLen[0] = data[ix + 5];

                imgWid[1] = data[ix + 6];
                imgWid[0] = data[ix + 7];
                                
                len = (imgLen[1] << 8) + imgLen[0];   
                width = (imgWid[1] << 8) + imgWid[0];   

                scale[si][0] = len;
                scale[si][1] = width;
                scale[si][2] = 1;
                si ++;
                
                printf("!!!!!!!![findJpgScale] height = %d, width = %d, m[0]:0x%.2x, m[1]:0x%.2x\n", len, width, marker[0], marker[1]);
                
            }
            staf = 0;
        }
        
    }

    for (ix=0; ix < si; ix++) {
        printf("[findJpgScale] %d. hight: %d width: %d flag: %d \n", ix, scale[ix][0], scale[ix][1], scale[ix][2]);

        if (scale[ix][2] != 0) {
            len = scale[ix][0];
            width = scale[ix][1];
            ret = 0;
        }
    }
    
    *hi = len;
    *wh = width;

    return ret;
}

static int changeJpgLen(uint8_t *data, uint32_t tlen, int max)
{
    int ret = -1, ix = 0, staf = 0;
    uint8_t marker[2] = {0, 0};
    uint32_t imgLen[2] = {0, 0};
    uint8_t ch = 0;
    uint32_t len = 0;

    if (!data) return -2;
    if (!max) return -3;
    
    msync(data, max, MS_SYNC);
    
    for (ix=0; ix < max; ix++) {
        ch = data[ix];
    
        if (ch == 0xff) {
            marker[0] = ch;
            staf = 1;
        } 
        else if (staf == 1) {
            if (((ch >> 4) == 0xc) && ((ch & 0xf) != 4)) {
                marker[1] = ch;
                
                imgLen[1] = data[ix + 4];
                imgLen[0] = data[ix + 5];
                                
                len = (imgLen[1] << 8) + imgLen[0];   

                printf("[changeImgLen] Length = %d -> %d\n", len, tlen);
                
                data[ix + 4] = tlen >> 8;
                data[ix + 5] = tlen & 0xff;;

                ret = 0;
                break;
            }
            staf = 0;
        }
        
    }

    return ret;
}

static int fs96(struct mainRes_s *mrs, struct modersp_s *modersp) 
{ 
    char *addr=0;
    uint32_t val=0;
    int ret, totsz=0, len=0, secLen, max=0, mdo=0;
    int hi = 0, wh = 0, n = 0;
    struct sdFAT_s *pfat=0;
    struct supdataBack_s *rs = 0, *s=0, *sc=0, *sh=0, *se=0;
    struct sdbootsec_s   *psec=0;
    struct info16Bit_s *p=0, *c=0;
    struct aspConfig_s *pct=0;
    
    pct = mrs->configTable;
    pfat = &mrs->aspFat;
    sh = pfat->fatSupdata;
    sc = pfat->fatSupcur;
    psec = pfat->fatBootsec;

    rs = aspMalloc(sizeof(struct supdataBack_s));
    memset(rs, 0, sizeof(struct supdataBack_s));
    s = rs;
    
    sprintf(mrs->log, "deal with sup back head buff!! - 1\n");
    print_f(&mrs->plog, "fs96", mrs->log);

    if (!sh) {
        sprintf(mrs->log, "ERROR!!! sup back head buff is empty! \n");
        print_f(&mrs->plog, "fs96", mrs->log);
        modersp->r = 0xed;
        aspFree(rs);
        return 1;
    }

    if (!sc) {
        sprintf(mrs->log, "WARNING!!! sup back current buff is empty! \n");
        print_f(&mrs->plog, "fs96", mrs->log);
        pfat->fatSupcur = sh;
        sc = sh;
    }

    p = &mrs->mchine.tmp;
       
    /* calculate the total size */
    secLen = p->opinfo;
    totsz = secLen * psec->secSize;

    sprintf(mrs->log, "deal with sup back head buff!! - 2\n");
    print_f(&mrs->plog, "fs96", mrs->log);

    max = aspCalcSupLen(sc);
    if (totsz > max) {
        sprintf(mrs->log, "WARNING!!! totsz is larger than max rest size of sup buff, %d/%d \n", totsz, max);
        print_f(&mrs->plog, "fs96", mrs->log);
        totsz = max;
    }

    sprintf(mrs->log, "totsz/max = %d/%d \n", totsz, max);
    print_f(&mrs->plog, "fs96", mrs->log);

    sprintf(mrs->log, "deal with sup back head buff!! - 3\n");
    print_f(&mrs->plog, "fs96", mrs->log);

#if 0 // move to fs98
    ret = cfgTableGetChk(pct, ASPOP_IMG_LEN, &val, ASPOP_STA_UPD);    
    if (ret) {
        val = 0;
    }

    sprintf(mrs->log, "deal with sup back head buff!! - 4\n");
    
    print_f(&mrs->plog, "fs96", mrs->log);

    pct[ASPOP_IMG_LEN].opStatus = ASPOP_STA_APP;
#endif

    sprintf(mrs->log, "deal with sup back head buff!! - 5\n");
    print_f(&mrs->plog, "fs96", mrs->log);

    mdo = 1;
    while (totsz >= 0) {
    
        //sprintf(mrs->log, "deal with sup back head buff!! - 6\n");
        //print_f(&mrs->plog, "fs96", mrs->log);

        /* pup and push data here */
        len = ring_buf_get(&mrs->cmdTx, &addr);
        while (len <= 0) {
            sleep(2);
            len = ring_buf_get(&mrs->cmdTx, &addr);
        }

        //sprintf(mrs->log, "deal with sup back head buff!! - 7 len:%d \n", len);
        //print_f(&mrs->plog, "fs96", mrs->log);

        if (totsz > len) {
            ret = aspPopSupOut(addr, sc, len, &s);
        } else {
            ret = aspPopSupOut(addr, sc, totsz, &s);
        }

        //sprintf(mrs->log, "list buff pop resutl, ret: %d/%d\n", ret, totsz);
        //print_f(&mrs->plog, "fs96", mrs->log);

#if 0 // move to fs98
        if ((mdo) && (val)) {
            mdo = changeJpgLen(addr, val, ret);
        }

        n = findJpgScale(addr, &hi, &wh, ret);
        if (!n) {
            sprintf(mrs->log, "jpg scale = (%d, %d)\n", hi, wh);
            print_f(&mrs->plog, "fs96", mrs->log);
        }
#endif
        
        ring_buf_prod(&mrs->cmdTx);
        
        mrs_ipc_put(mrs, "u", 1, 1);
        totsz -= ret;

        if (totsz <= 0) break;
        
        sc = s;
    }

    ring_buf_set_last(&mrs->cmdTx, ret);
    mrs_ipc_put(mrs, "u", 1, 1);
    
    mrs_ipc_put(mrs, "U", 1, 1);

    pfat->fatSupcur = sc;
    sprintf(mrs->log, "END buff pop, resutl %d/%d\n", ret, totsz);
    print_f(&mrs->plog, "fs96", mrs->log);

    modersp->m = modersp->m + 1;

    aspFree(rs);
    return 2; 
}

static int fs97(struct mainRes_s *mrs, struct modersp_s *modersp) 
{ 
    int len=0, bitset=0, ret=0;
    char ch=0;
    struct info16Bit_s *p;

    //sprintf(mrs->log, "wait spi0 tx end\n");
    //print_f(&mrs->plog, "fs97", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if (len > 0) {

        sprintf(mrs->log, "ch: %c - end\n", ch);
        print_f(&mrs->plog, "fs97", mrs->log);
        if (ch == 'u') {
            modersp->v += 1;
        }
        
        if (ch == 'U') {

            ring_buf_init(&mrs->cmdTx);

#if SPI_KTHREAD_USE & SPI_UPD_NO_KTHREAD
            bitset = 0;
            ret = msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
            sprintf(mrs->log, "Stop spi0 spidev thread, ret: 0x%x\n", ret);
            print_f(&mrs->plog, "fs97", mrs->log);
#endif
#if PULL_LOW_AFTER_DATA
            bitset = 0;
            msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
            sprintf(mrs->log, "set RDY pin %d\n",bitset);
            print_f(&mrs->plog, "fs97", mrs->log);
#endif
            usleep(210000);

            modersp->m = 48;            
            return 2;
        } else  {
            //modersp->r = 2;
            //modersp->c += 1;
            return 0;
        }
    }
    return 0; 
}

static int fs98(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    char *fnameSave = 0;
    char fnameSave_jpg[16] = "asp%.5d.jpg";
    char fnameSave_pdf[16] = "asp%.5d.pdf";
    char fnameSave_tif[16] = "asp%.5d.tif";
    char srhName[16];
    int ret=0, cnt=0, hi=0, wh=0, mh=0, mw=0;
    uint32_t secStr=0, secLen=0, clstByte, clstLen=0, clstStr=0;
    uint32_t freeClst=0, usedClst=0, totClst=0, val=0;
    int datLen=0, imgLen=0;
    uint32_t fformat=0;
    struct sdbootsec_s   *psec=0;
    struct sdFAT_s *pfat=0;
    struct adFATLinkList_s *pflsh=0, *pflnt=0;
    struct adFATLinkList_s *pfre=0, *pnxf=0, *pclst=0;
    struct sdFATable_s   *pftb=0;
    struct directnFile_s *upld=0, *fscur=0, *fssrh=0;
    struct supdataBack_s *s=0, *sc=0, *sh=0, *se=0, *sb=0;
    struct aspConfig_s *pct=0;
    
    uint32_t adata[3], atime[3];
    char *wday[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"}; 
    struct tm *p=0;
    time_t timep;
                
    pct = mrs->configTable;                
    pfat = &mrs->aspFat;
    psec = pfat->fatBootsec;
    pftb = pfat->fatTable;
    clstByte = psec->secSize * psec->secPrClst;
    if (!clstByte) {
        sprintf(mrs->log, "ERROR!! bytes number of cluster is zero \n");
        print_f(&mrs->plog, "fs98", mrs->log);

        modersp->r = 0xed;
        return 1;
    }

    pfre = pftb->ftbMng.f;
    if (!pfre) {
        sprintf(mrs->log, "Error!! free space link list is empty \n");
        print_f(&mrs->plog, "fs98", mrs->log);
        modersp->r = 0xed;
        return 1;
    }

    if (!pfat->fatCurDir) {
        sprintf(mrs->log, "Error!! current folder is null \n");
        print_f(&mrs->plog, "fs98", mrs->log);
        modersp->r = 0xed;
        return 1;
    }
    fscur = pfat->fatCurDir;

    sh = pfat->fatSupdata;
    if (!sh) {
        sprintf(mrs->log, "ERROR!!! buffered link list is NULL \n");
        print_f(&mrs->plog, "fs98", mrs->log);
        modersp->r = 0xed;
        return 1;
    }

    sc = pfat->fatSupcur;
    if (!sc) {
        sprintf(mrs->log, "WARNING!!! current buffered link list is NULL \n");
        print_f(&mrs->plog, "fs98", mrs->log);

        pfat->fatSupcur = sh;
        sc = sh;
    }

    ret = mspFS_allocDir(pfat, &upld);
    if (ret) {
         sprintf(mrs->log, "Error!! get new file entry failed ret: %d \n", ret);
        print_f(&mrs->plog, "fs98", mrs->log);
        modersp->r = 0xed;
        return 1;
    }

    ret = cfgTableGetChk(pct, ASPOP_FILE_FORMAT, &fformat, ASPOP_STA_CON);    
    if (ret) {
        fformat = 0;
    }

    if (fformat == FILE_FORMAT_JPG) {
        fnameSave = fnameSave_jpg;
        sprintf(mrs->log, "file format : JPG(%d) name type:[%s]\n", fformat, fnameSave);
        print_f(&mrs->plog, "fs98", mrs->log);

    } else if (fformat == FILE_FORMAT_PDF) {
        fnameSave = fnameSave_pdf;
        sprintf(mrs->log, "file format : PDF(%d) name type:[%s]\n", fformat, fnameSave);
        print_f(&mrs->plog, "fs98", mrs->log);

    } else if (fformat == FILE_FORMAT_TIFF_I) {
        fnameSave = fnameSave_tif;
        sprintf(mrs->log, "file format : TIFF_I(%d) name type:[%s]\n", fformat, fnameSave);
        print_f(&mrs->plog, "fs98", mrs->log);

    } else if (fformat == FILE_FORMAT_TIFF_M) {
        fnameSave = fnameSave_tif;
        sprintf(mrs->log, "file format : TIFF_M(%d) name type:[%s]\n", fformat, fnameSave);
        print_f(&mrs->plog, "fs98", mrs->log);

    } else {
        fnameSave = fnameSave_jpg;    
        sprintf(mrs->log, "file format : others(%d) name type:[%s]\n", fformat, fnameSave);
        print_f(&mrs->plog, "fs98", mrs->log);

    }

    memset(upld, 0, sizeof(struct directnFile_s));
    upld->dftype = ASPFS_TYPE_FILE;
    upld->dfstats = ASPFS_STATUS_DIS;
    //upld->dfstats = ASPFS_STATUS_EN;
    upld->dfattrib = ASPFS_ATTR_ARCHIVE;

    time(&timep);
    p=localtime(&timep); /*取得當地時間*/ 
    sprintf(mrs->log, "%.4d%.2d%.2d \n", (1900+p->tm_year),( 1+p-> tm_mon), p->tm_mday); 
    print_f(&mrs->plog, "fs98", mrs->log);
    sprintf(mrs->log, "%s,%.2d:%.2d:%.2d\n", wday[p->tm_wday],p->tm_hour, p->tm_min, p->tm_sec); 
    print_f(&mrs->plog, "fs98", mrs->log);

    adata[0] = p->tm_year+1900;
    adata[1] = p->tm_mon + 1;
    adata[2] = p->tm_mday;
    
    atime[0] = p->tm_hour;
    atime[1] = p->tm_min;
    atime[2] = p->tm_sec;
    
    upld->dfcredate = ((((adata[0] - 1980) & 0xff) << 16) | ((adata[1] & 0xff) << 8) | (adata[2] & 0xff));
    upld->dfcretime = (((atime[0]&0xff) << 16) | ((atime[1]&0xff) << 8) | (atime[2]&0xff));
    upld->dflstacdate = ((((adata[0] - 1980)&0xff) << 16) | ((adata[1]&0xff) << 8) | (adata[2]&0xff));
    upld->dfrecodate = ((((adata[0] - 1980)&0xff) << 16) | ((adata[1]&0xff) << 8) | (adata[2]&0xff));
    upld->dfrecotime = (((atime[0]&0xff) << 16) | ((atime[1]&0xff) << 8) | (atime[2]&0xff));

    /* assign a name with sequence number */
    for (cnt=0; cnt < 10000; cnt++) {
        sprintf(srhName, fnameSave, cnt);
        sprintf(mrs->log, "search name: [%s]\n", srhName);
        print_f(&mrs->plog, "fs98", mrs->log);

        ret = mspFS_SearchInFolder(&fssrh, fscur, srhName);
        if (ret) break;
    }

    strncpy(upld->dfSFN, srhName, 12);
    upld->dfSFN[13] = '\0';

    upld->dflen = 0;
    upld->dfLFN[0] = '\0';

    sprintf(mrs->log, "SFN[%s] LFS[%s] len:%d\n", upld->dfSFN, upld->dfLFN, upld->dflen);
    print_f(&mrs->plog, "fs98", mrs->log);

    if (fformat == FILE_FORMAT_PDF) {
        s = sh->n;
        if (!s) {
            sprintf(mrs->log, "Error!!! the first trunk is not exist!!!");
            print_f(&mrs->plog, "fs98", mrs->log);
            modersp->r = 2;
            return 1;
        }
        
        ret = cfgTableGetChk(pct, ASPOP_IMG_LEN, &val, ASPOP_STA_UPD);    
        if (ret) {
            val = 0;
        }
        sprintf(mrs->log, "user defined jpg length: %d, ret:%d \n", val, ret);
        print_f(&mrs->plog, "fs98", mrs->log);
        
        pct[ASPOP_IMG_LEN].opStatus = ASPOP_STA_APP;
        
        if (val) {
            ret = changeJpgLen(s->supdataBuff, val, s->supdataTot);
            if (ret) {
                sprintf(mrs->log, "Error!!! can NOT find jpg length in first trunk !!!");
                print_f(&mrs->plog, "fs98", mrs->log);
                modersp->r = 0xed;
                return 1;
            }
        }
        
        ret = findJpgScale(s->supdataBuff, &hi, &wh, s->supdataTot);
        if (ret) {
            sprintf(mrs->log, "Error!!! can NOT find height and width in first trunk !!!");
            print_f(&mrs->plog, "fs98", mrs->log);
            modersp->r = 0xed;
            return 1;
        } else {
            /* caluclate the pdf parameter by height and width */
            pdfParamCalcu(hi, wh, &mh, &mw);
        }
        
        /* pdf head */
        sb = sh;
        sprintf(mrs->log, "PDF Head get!!! tot: %d, use:%d \n", sb->supdataTot, sb->supdataUse);
        print_f(&mrs->plog, "fs98", mrs->log);
        if (sb->supdataTot != sb->supdataUse) {
            //shmem_dump(sb->supdataBuff + sb->supdataUse, sb->supdataTot - sb->supdataUse);
            //sprintf(mrs->log, "dump - end\n");
            //print_f(&mrs->plog, "fs98", mrs->log);
        }
        
        ret = pdfHead(sb->supdataBuff, SPI_TRUNK_SZ, hi, wh, mh, mw);
        
        sb->supdataUse = 0;
        sb->supdataTot = ret;

        /*
        shmem_dump(sb->supdataBuff, sb->supdataTot);
        sprintf(mrs->log, "dump PDF head - end\n");
        print_f(&mrs->plog, "fs98", mrs->log);
        */

        /* calculate sector start and sector length of file */            
        datLen = aspCalcSupLen(sc);
        if (datLen < 0) {
            sprintf(mrs->log, "Error!!! calculate support buffer length failed !!!");
            print_f(&mrs->plog, "fs98", mrs->log);
            modersp->r = 0xed;
            return 1;
        }

        imgLen = aspCalcSupLen(sc->n); 
        if (imgLen < 0) {
            sprintf(mrs->log, "Error!!! calculate support buffer length failed !!!");
            print_f(&mrs->plog, "fs98", mrs->log);
            modersp->r = 0xed;
            return 1;
        }
    
        /* pdf tail */
        se = sc;
        while (se->n) {
            se = se->n;
        }
        sprintf(mrs->log, "PDF Tail get!!! tot: %d, use:%d \n", se->supdataTot, se->supdataUse);
        print_f(&mrs->plog, "fs98", mrs->log);
        /*
        if (se->supdataTot != 0) {
            shmem_dump(se->supdataBuff, se->supdataTot);
            sprintf(mrs->log, "dump - end\n");
            print_f(&mrs->plog, "fs98", mrs->log);
        }
        */
        ret = pdfTail(se->supdataBuff + se->supdataTot, SPI_TRUNK_SZ-se->supdataTot, datLen, imgLen);
        if (ret == -3) {
            s = 0;
            s = aspMalloc(sizeof(struct supdataBack_s));
            if (!s) {
                sprintf(mrs->log, "FAIL to allcate memory for the pdf tail !!! \n");
                print_f(&mrs->plog, "fs98", mrs->log);
                modersp->r = 0xed;
                return 1;
            }

            memset(s, 0, sizeof(struct supdataBack_s));
            se->n = s;
            se = s;
            ret = pdfTail(se->supdataBuff, SPI_TRUNK_SZ, datLen, imgLen);
        }
        
        if (ret < 0) {
            sprintf(mrs->log, "Error!!! can NOT append pdf tail ret: %d !!!", ret);
            print_f(&mrs->plog, "fs98", mrs->log);
            modersp->r = 0xed;
            return 1;
        }

        /*
        shmem_dump(se->supdataBuff+se->supdataTot, ret);
        sprintf(mrs->log, "dump PDF tail - end\n");
        print_f(&mrs->plog, "fs98", mrs->log);
        */
        se->supdataTot += ret;
        datLen += ret;
    }
    else if (fformat == FILE_FORMAT_TIFF_I) {
        s = sh->n;
        if (!s) {
            sprintf(mrs->log, "Error!!! TIFF_I the first trunk is not exist!!!");
            print_f(&mrs->plog, "fs98", mrs->log);
            modersp->r = 2;
            return 1;
        }
         
        /* tiff head */
        sb = sh;
        sprintf(mrs->log, "TIFF_I Head get!!! tot: %d, use:%d \n", sb->supdataTot, sb->supdataUse);
        print_f(&mrs->plog, "fs98", mrs->log);
        if (sb->supdataTot != sb->supdataUse) {
            shmem_dump(sb->supdataBuff + sb->supdataUse, sb->supdataTot - sb->supdataUse);
            sprintf(mrs->log, "dump - end\n");
            print_f(&mrs->plog, "fs98", mrs->log);
        }
        
        ret = tiffHead(sb->supdataBuff, SPI_TRUNK_SZ);
        
        sb->supdataUse = 0;
        sb->supdataTot = ret;

        
        shmem_dump(sb->supdataBuff, sb->supdataTot);
        sprintf(mrs->log, "dump TIFF_I head - end\n");
        print_f(&mrs->plog, "fs98", mrs->log);
        

        /* calculate sector start and sector length of file */            
        datLen = aspCalcSupLen(sc);
        if (datLen < 0) {
            sprintf(mrs->log, "Error!!! calculate support buffer length failed !!!");
            print_f(&mrs->plog, "fs98", mrs->log);
            modersp->r = 0xed;
            return 1;
        }
    
        /* pdf tail */
        se = sc;
        while (se->n) {
            se = se->n;
        }
        sprintf(mrs->log, "TIFF_I Tail get!!! tot: %d, use:%d \n", se->supdataTot, se->supdataUse);
        print_f(&mrs->plog, "fs98", mrs->log);
        
        if (se->supdataTot != 0) {
            shmem_dump(se->supdataBuff, se->supdataTot);
            sprintf(mrs->log, "dump - end\n");
            print_f(&mrs->plog, "fs98", mrs->log);
        }
        
        
        ret = tiffTail(se->supdataBuff + se->supdataTot, SPI_TRUNK_SZ-se->supdataTot);
        if (ret == -3) {
            s = 0;
            s = aspMalloc(sizeof(struct supdataBack_s));
            if (!s) {
                sprintf(mrs->log, "FAIL to allcate memory for the TIFF_I tail !!! \n");
                print_f(&mrs->plog, "fs98", mrs->log);
                modersp->r = 0xed;
                return 1;
            }

            memset(s, 0, sizeof(struct supdataBack_s));
            se->n = s;
            se = s;
            ret = tiffTail(se->supdataBuff, SPI_TRUNK_SZ);
        }
        
        if (ret < 0) {
            sprintf(mrs->log, "Error!!! can NOT append TIFF_I tail ret: %d !!!", ret);
            print_f(&mrs->plog, "fs98", mrs->log);
            modersp->r = 0xed;
            return 1;
        }

        
        shmem_dump(se->supdataBuff+se->supdataTot, ret);
        sprintf(mrs->log, "dump TIFF_I tail - end\n");
        print_f(&mrs->plog, "fs98", mrs->log);
        
        se->supdataTot += ret;
        datLen += ret;
    }
    else {
        /* calculate sector start and sector length of file */            
        datLen = aspCalcSupLen(sc);
        if (datLen < 0) {
            sprintf(mrs->log, "Error!!! calculate support buffer length failed !!!");
            print_f(&mrs->plog, "fs98", mrs->log);
            modersp->r = 0xed;
            return 1;
        }
    }

    if (datLen % clstByte) {
        clstLen = (datLen / clstByte) + 1;
    } else {
        clstLen = (datLen / clstByte);        
    }

    sprintf(mrs->log, "Calculate sup back data len: %d\n", datLen);
    print_f(&mrs->plog, "fs98", mrs->log);
    
    if (clstLen) {
        ret = mspSD_allocFreeFATList(&pflsh, clstLen, pfre, &pnxf);
        if (ret) {
            sprintf(mrs->log, "free FAT table parsing for file upload FAIL!!ret:%d (%s)\n", ret, upld->dfSFN);
            print_f(&mrs->plog, "fs98", mrs->log);
            modersp->r = 0xed;
            return 1;
        } 
        else {
            freeClst = 0;
            if ((pfre != pnxf) && (pnxf)) {
                totClst = (psec->secTotal - psec->secWhroot) / psec->secPrClst;

                while (pfre != pnxf) {
                    pclst = pfre;

                    pfre = pfre->n;

                    sprintf(mrs->log, "free used FREE FAT linklist, 0x%.8x start: %d, length: %d \n", pclst, pclst->ftStart, pclst->ftLen);
                    print_f(&mrs->plog, "fs98", mrs->log);

                    aspFree(pclst);
                    pclst = 0;
                }
            }

            pflnt = pnxf;
            while (pflnt) {
                freeClst += pflnt->ftLen;
                sprintf(mrs->log, "cal start: %d len:%d \n", pflnt->ftStart, pflnt->ftLen);
                print_f(&mrs->plog, "fs98", mrs->log);
                pflnt = pflnt->n;
            }

            sprintf(mrs->log, " re-calculate total free cluster: %d free sector: %d (size: %d) \n", freeClst, freeClst * psec->secPrClst, freeClst * psec->secPrClst * psec->secSize);
            print_f(&mrs->plog, "fs98", mrs->log);     
            usedClst = totClst - freeClst;

            pftb->ftbMng.ftfreeClst = freeClst;
            pftb->ftbMng.ftusedClst = usedClst;
            pftb->ftbMng.f = pnxf;
        }

        /* debug */
        sprintf(mrs->log, "show allocated FAT list: \n");
        print_f(&mrs->plog, "fs98", mrs->log);

        val = 0;
        pflnt = pflsh;
        while (pflnt) {
            val += pflnt->ftLen;
            sprintf(mrs->log, "    str:%d len:%d - %d\n", pflnt->ftStart, pflnt->ftLen, val);
            print_f(&mrs->plog, "fs98", mrs->log);
            pflnt = pflnt->n;
        }
        sprintf(mrs->log, "total allocated cluster is %d!! \n", val);
        print_f(&mrs->plog, "fs98", mrs->log);
    
        pftb->h = pflsh;
        pftb->c = pftb->h;

        pfat->fatStatus |= ASPFAT_STATUS_SDWBK;
        pfat->fatStatus |= ASPFAT_STATUS_FATWT;
    }else {
        pftb->h = 0;
        pftb->c = 0;
    }

    upld->dfclstnum = pflsh->ftStart; /* start cluster */
    upld->dflength = datLen;

    aspFS_insertFATChild(fscur, upld);
    pfat->fatFileUpld = upld;
    debugPrintDir(upld);
    mspFS_folderList(upld->pa, 4);

    pfat->fatStatus |= ASPFAT_STATUS_DFECHK;
    pfat->fatStatus |= ASPFAT_STATUS_DFEWT;

    modersp->r = 1;
    return 1;
}

static int fs99(struct mainRes_s *mrs, struct modersp_s *modersp) 
{ 
    int bitset, ret;
    sprintf(mrs->log, "trigger spi0\n");
    print_f(&mrs->plog, "fs99", mrs->log);

#if SPI_KTHREAD_USE
    bitset = 0;
    ret = msp_spi_conf(mrs->sfm[0], _IOR(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_START_THREAD
    sprintf(mrs->log, "Start spi0 spidev thread, ret: 0x%x\n", ret);
    print_f(&mrs->plog, "fs99", mrs->log);
#endif

    ring_buf_init(&mrs->cmdRx);

    mrs_ipc_put(mrs, "x", 1, 1);

    modersp->v = 0;
    modersp->m = modersp->m + 1;
    return 2;
}

static int fs100(struct mainRes_s *mrs, struct modersp_s *modersp) 
{ 
    int ret, bitset, len;
    char ch, *addr=0, *dst=0;
    struct sdFAT_s *pfat=0;
    struct supdataBack_s *s=0, *sc=0;

    pfat = &mrs->aspFat;
    sc = pfat->fatSupcur;
    //sprintf(mrs->log, "%d\n", modersp->v);
    //print_f(&mrs->plog, "fs100", mrs->log);

    ret = mrs_ipc_get(mrs, &ch, 1, 1);
    while (ret > 0) {
        if (ch == 'x') {
            if (sc) {
                len = ring_buf_cons(&mrs->cmdRx, &addr);
                sprintf(mrs->log, "1. get psudo len:%d\n", len);
                print_f(&mrs->plog, "fs100", mrs->log);

                if (len >= 0) {
                    dst = sc->supdataBuff;
                    memcpy(dst, addr, len);
                    sc->supdataTot = len;

                    s = aspMalloc(sizeof(struct supdataBack_s));
                    memset(s, 0, sizeof(struct supdataBack_s));
                    sc->n = s;
                    sc = sc->n;

                    pfat->fatSupcur = sc;
                }
            } else {
                len = ring_buf_cons(&mrs->cmdRx, &addr);
                sprintf(mrs->log, "1. cons len:%d\n", len);
                print_f(&mrs->plog, "fs100", mrs->log);
            }
            
        }
    
        if (ch == 'X') {
            sprintf(mrs->log, "get %c \n", ch);
            print_f(&mrs->plog, "fs100", mrs->log);

            modersp->v |= 0x1;
        }
        ret = mrs_ipc_get(mrs, &ch, 1, 1);
    }

    if (modersp->v & 0x1) {
        if (sc) {
            len = ring_buf_cons(&mrs->cmdRx, &addr);
            while (len >= 0) {
                sprintf(mrs->log, "2. get psudo len:%d END\n", len);
                print_f(&mrs->plog, "fs100", mrs->log);

                dst = sc->supdataBuff;
                memcpy(dst, addr, len);
                sc->supdataTot = len;
                
                s = aspMalloc(sizeof(struct supdataBack_s));
                memset(s, 0, sizeof(struct supdataBack_s));
                sc->n = s;
                sc = sc->n;

                pfat->fatSupcur = sc;
                modersp->v += 1;  
                len = ring_buf_cons(&mrs->cmdRx, &addr);
            }

            s = pfat->fatSupdata;
            while (s) {
                if (s->supdataTot == 0) {
                    break;
                }
                sc = s;
                s = s->n;
            }

            if (sc) {
                sc->n = 0;
            }

            while (s) {
                sc = s;
                s = s->n;

                memset(sc, 0, sizeof(struct supdataBack_s));
                aspFree(sc);
            }
            pfat->fatSupcur = 0;
        }
        sprintf(mrs->log, "SD only end\n");
        print_f(&mrs->plog, "fs100", mrs->log);

        ring_buf_init(&mrs->cmdRx);

#if SPI_KTHREAD_USE
        bitset = 0;
        ret = msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
        sprintf(mrs->log, "Stop spi0 spidev thread, ret: 0x%x\n", ret);
        print_f(&mrs->plog, "fs100", mrs->log);
#endif
#if PULL_LOW_AFTER_DATA
        bitset = 0;
        msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        sprintf(mrs->log, "set RDY pin %d\n",bitset);
        print_f(&mrs->plog, "fs100", mrs->log);
#endif
        usleep(210000);

        modersp->r = 1;            
        return 1;
    }

    return 0; 
}

static int fs101(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    struct info16Bit_s *p;
    p = &mrs->mchine.cur;
    //sprintf(mrs->log, "set %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    //print_f(&mrs->plog, "fs101", mrs->log);

    //printf("[fs101] \n");
    
    mrs_ipc_put(mrs, "c", 1, 1);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs102(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    p = &mrs->mchine.get;

    //printf("[fs102] \n");
    
    //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x - 1\n", p->inout, p->seqnum, p->opcode, p->data);
    //print_f(&mrs->plog, "fs102", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x - 2\n", p->inout, p->seqnum, p->opcode, p->data);
        //print_f(&mrs->plog, "fs102", mrs->log);

        if (p->opcode == OP_QRY) {
            modersp->m = modersp->m + 1;
        } else {
            modersp->r = 2;
            return 1;
        }
    }
    return 0; 
}

static int fs103(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    struct info16Bit_s *p;
    p = &mrs->mchine.cur;

    //printf("[fs103] \n");
    
    //sprintf(mrs->log, "set %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    //print_f(&mrs->plog, "fs103", mrs->log);
    
    mrs_ipc_put(mrs, "c", 1, 1);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs104(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int len=0;
    char ch=0;
    struct info16Bit_s *p, *c;
    c = &mrs->mchine.cur;
    p = &mrs->mchine.get;

    //printf("[fs104] \n");

    //sprintf(mrs->log, "get 0x%.2x/0x%.2x 0x%.2x/0x%.2x - 1\n", p->opcode, c->opcode, p->data, c->data);
    //print_f(&mrs->plog, "fs104", mrs->log);
        
    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        c = &mrs->mchine.cur;
        p = &mrs->mchine.get;
        //sprintf(mrs->log, "get 0x%.2x/0x%.2x 0x%.2x/0x%.2x - 2\n", p->opcode, c->opcode, p->data, c->data);
        //print_f(&mrs->plog, "fs104", mrs->log);

        if (p->opcode == c->opcode) {
            if (p->data == c->data) {
                modersp->r = 1;
            } else {
                modersp->r = 2;
            }
            return 1;
        } else {
            modersp->r = 2;
            return 1;
        }
    }
    return 0; 
}

#define CROP_SCALE 10

#define CROP_COOD_01 {20  * CROP_SCALE, 80  * CROP_SCALE}
#define CROP_COOD_02 {75  * CROP_SCALE, 135 * CROP_SCALE}
#define CROP_COOD_03 {85  * CROP_SCALE, 135 * CROP_SCALE}
#define CROP_COOD_04 {140 * CROP_SCALE, 80  * CROP_SCALE}
#define CROP_COOD_05 {85   * CROP_SCALE, 25  * CROP_SCALE}
#define CROP_COOD_06 {75   * CROP_SCALE, 25  * CROP_SCALE}

static int getVector(double *vec, double *p1, double *p2)
{
    double a1, b, a2;
    double x1, y1, x2, y2;

    if (!vec) return -1;
    if (!p1) return -2;
    if (!p2) return -3;

    x1 = p1[0];
    y1 = p1[1];

    x2 = p2[0];
    y2 = p2[1];

    printf("getVector() input - p1 = (%lf, %lf), p2 = (%lf, %lf)\n", x1, y1, x2, y2);
    
    b = ((x2 * y1) - (x1 * y2)) / (x2 - x1);

    a1 = ((x1 * y2) - (x1 * y1)) / ((x1 * x2) - (x1 * x1));
    a2 = ((x2 * y2) - (x2 * y1)) / ((x2 * x2) - (x2 * x1));

    printf("getVector() output - a = %lf/%lf, b = %lf\n", a1, a2, b);

    vec[0] = a1;
    vec[1] = b;

    return 0;

}

static int getCross(double *v1, double *v2, double *pt)
{
    double a1, a2, b1, b2;
    double x, y;

    if (!v1) return -1;
    if (!v2) return -2;
    if (!pt) return -3;

    a1 = v1[0];
    b1 = v1[1];

    a2 = v2[0];
    b2 = v2[1];

    y = ((a1 * b2) - (a2 * b1)) / (a1 - a2);
    x = (b2 - b1) / (a1 - a2);

    printf("getCross() output - pt = (%lf, %lf)\n", x, y);

    pt[0] = x;
    pt[1] = y;
    
    return 0;
}

static int calcuDistance(double *dist, double *p1, double *p2) 
{
    double x1, y1, x2, y2;
    double dx, dy, ds;

    if (!dist) return -1;
    if (!p1) return -2;
    if (!p2) return -3;

    x1 = p1[0];
    y1 = p1[1];

    x2 = p2[0];
    y2 = p2[1];

    dx = x1 - x2;
    dy = y1 - y2;

    ds = dx * dx + dy * dy;
    
    printf("calcuDistance() output - ds = %lf \n", ds);    

    *dist = ds;
    return 0;
}

static int fs105(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int ret=0, id=0, id1=0, id2=0;
    double c01[2] = CROP_COOD_01;
    double c02[2] = CROP_COOD_02;
    double c03[2] = CROP_COOD_03;
    double c04[2] = CROP_COOD_04;
    double c05[2] = CROP_COOD_05;
    double c06[2] = CROP_COOD_06;

    double vect01[2] = {0, 0};
    double vect02[2] = {0, 0};
    double vect03[2] = {0, 0};
    double vect04[2] = {0, 0};
    double vect05[2] = {0, 0};
    double vect06[2] = {0, 0};

    double cross[6][2];
    
    double cros01[2] = {0, 0};
    double cros02[2] = {0, 0};
    double cros03[2] = {0, 0};
    double cros04[2] = {0, 0};
    double cros05[2] = {0, 0};
    double cros06[2] = {0, 0};

    double ds, dt1, dt2, dmin[6][2];
    int idm[6][2];
    
    sprintf(mrs->log, "calculate ...\n");
    print_f(&mrs->plog, "fs105", mrs->log);

    ret = getVector(vect01, c01, c02);
    ret = getVector(vect02, c02, c03);
    ret = getVector(vect03, c03, c04);
    ret = getVector(vect04, c04, c05);
    ret = getVector(vect05, c05, c06);
    ret = getVector(vect06, c06, c01);

    ret = getCross(vect01, vect03, cross[0]);
    ret = getCross(vect02, vect04, cross[1]);
    ret = getCross(vect03, vect05, cross[2]);
    ret = getCross(vect04, vect06, cross[3]);
    ret = getCross(vect05, vect01, cross[4]);
    ret = getCross(vect06, vect02, cross[5]);

    for (id = 0; id < 6; id++) {
        id1 = -1;
        id2 = -1;
        ret = calcuDistance(&ds, cross[id], c01);
        id1 = 1;
        dt1 = ds;
        ret = calcuDistance(&ds, cross[id], c02);
        id2 = 2;
        dt2 = ds;
        ret = calcuDistance(&ds, cross[id], c03);
        if ((dt1 > ds) || (dt2 > ds)) {
            if (dt1 > dt2) {
                dt1 = ds;     
                id1 = 3;
            } else {
                dt2 = ds;
                id2 = 3;
            }
        }
        ret = calcuDistance(&ds, cross[id], c04);
        if ((dt1 > ds) || (dt2 > ds)) {
            if (dt1 > dt2) {
                dt1 = ds;     
                id1 = 4;
            } else {
                dt2 = ds;
                id2 = 4;
            }
        }
        ret = calcuDistance(&ds, cross[id], c05);
        if ((dt1 > ds) || (dt2 > ds)) {
            if (dt1 > dt2) {
                dt1 = ds;     
                id1 = 5;
            } else {
                dt2 = ds;
                id2 = 5;
            }
        }
        ret = calcuDistance(&ds, cross[id], c06);
        if ((dt1 > ds) || (dt2 > ds)) {
            if (dt1 > dt2) {
                dt1 = ds;     
                id1 = 6;
            } else {
                dt2 = ds;
                id2 = 6;
            }
        }
        dmin[id][0] = dt1;
        dmin[id][1] = dt2;
        idm[id][0] = id1;
        idm[id][1] = id2;
    }

    dt1 = -1;
    dt2 = -1;
    
    for (id = 0; id < 6; id++) {
        if ((dt1 < 0) || (dt2 < 0)) {
            if (dt1 < 0) {
                dt1 = dmin[id][0] + dmin[id][1];
                id1 = id;
            } else {
                dt2 = dmin[id][0] + dmin[id][1];
                id2 = id;
            }
        } else {
            ds = dmin[id][0] + dmin[id][1];
            if ((dt1 > ds) || (dt2 > ds)) {
                if (dt1 > dt2) {
                    dt1 = ds;
                    id1 = id;
                } else {
                    dt2 = ds;
                    id2 = id;
                }
            }
        }
        printf("pt: (%lf, %lf) dmin: (%lf)(%lf)/(%d)(%d)\n", cross[id][0], cross[id][1], dmin[id][0], dmin[id][1], idm[id][0], idm[id][1]);
    }

    printf("replace (%d, %d) with %d, and (%d, %d) with %d, distance: (%lf, %lf)(%lf, %lf)\n", idm[id1][0], idm[id1][1], id1, idm[id2][0], idm[id2][1], id2, dmin[id1][0], dmin[id1][1], dmin[id2][0], dmin[id2][1]);
    
    modersp->r = 1; 
    return 1;
}

static int fs106(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int ret;
    char syscmd[256] = "ls -al";
    
    sprintf(mrs->log, "clear status ...\n");
    print_f(&mrs->plog, "fs106", mrs->log);

// launchAP or directAccess
    /* clear status */
    sprintf(syscmd, "kill -9 $(ps aux | grep 'uap0' | awk '{print $1}')");
    ret = doSystemCmd(syscmd);

    sprintf(mrs->log, "exec [%s]...\n", syscmd);
    print_f(&mrs->plog, "fs106", mrs->log);

    //sprintf(syscmd, "kill -9 $(ps aux | grep 'mothership' | awk '{print $1}')");
    //ret = doSystemCmd(syscmd);

    sprintf(syscmd, "kill -9 $(ps aux | grep 'hostapd' | awk '{print $1}')");
    ret = doSystemCmd(syscmd);

    sprintf(mrs->log, "exec [%s]...\n", syscmd);
    print_f(&mrs->plog, "fs106", mrs->log);

    sprintf(syscmd, "ifconfig uap0 down");
    ret = doSystemCmd(syscmd);

    sprintf(mrs->log, "exec [%s]...\n", syscmd);
    print_f(&mrs->plog, "fs106", mrs->log);

    sprintf(syscmd, "kill -9 $(ps aux | grep 'mlan0' | awk '{print $1}')");
    ret = doSystemCmd(syscmd);

    sprintf(mrs->log, "exec [%s]...\n", syscmd);
    print_f(&mrs->plog, "fs106", mrs->log);

    sprintf(syscmd, "kill -9 $(ps aux | grep 'wpa_supplicant' | awk '{print $1}')");
    ret = doSystemCmd(syscmd);

    sprintf(mrs->log, "exec [%s]...\n", syscmd);
    print_f(&mrs->plog, "fs106", mrs->log);

    sprintf(syscmd, "ifconfig mlan0 down");
    ret = doSystemCmd(syscmd);

    sprintf(mrs->log, "exec [%s]...\n", syscmd);
    print_f(&mrs->plog, "fs106", mrs->log);

    memset(mrs->netIntfs, 0, 16);
    sprintf(mrs->netIntfs, "%s", "mlan0");
    msync(mrs->netIntfs, 16, MS_SYNC);

    modersp->r = 1; 
    return 1;
}

static int fs107(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int ret;
    char syscmd[256] = "ls -al";
    
    sprintf(mrs->log, "launch Direct mode ...\n");
    print_f(&mrs->plog, "fs107", mrs->log);

    sprintf(syscmd, "/root/script/launchAP_88w8787.sh");
    ret = doSystemCmd(syscmd);

    sprintf(mrs->log, "exec [%s]...\n", syscmd);
    print_f(&mrs->plog, "fs107", mrs->log);
    
    memset(mrs->netIntfs, 0, 16);
    sprintf(mrs->netIntfs, "%s", "uap0");
    msync(mrs->netIntfs, 16, MS_SYNC);
    
    modersp->r = 1; 
    return 1;
}

static int fs108(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int ret;
    char syscmd[256] = "ls -al";
    struct apWifiConfig_s *pwfc=0;

    pwfc = &mrs->wifconf;
    if ((pwfc->wfpskLen > 0) && (pwfc->wfsidLen > 0)) {
        sprintf(mrs->log, "launch AP mode ... ssid: \"%s\", psk: \"%s\"\n", pwfc ->wfssid, pwfc->wfpsk);
        print_f(&mrs->plog, "fs108", mrs->log);
    } else {
        sprintf(mrs->log, "failed to launch AP mode, no ssid and psk ...\n");
        print_f(&mrs->plog, "fs108", mrs->log);
        modersp->r = 2; 
        return 1;
    }

    /* launch wpa connect */
    //sprintf(syscmd, "/root/script/iw_con.sh");
    sprintf(syscmd, "/root/script/wpa_conf.sh \\\"%s\\\" \\\"%s\\\" /etc/wpa_supplicant.conf ", pwfc ->wfssid, pwfc->wfpsk);
    ret = doSystemCmd(syscmd);

    sprintf(syscmd, "wpa_supplicant -B -c /etc/wpa_supplicant.conf -imlan0 -Dnl80211 -dd");
    ret = doSystemCmd(syscmd);

    sleep(3);

    sprintf(syscmd, "udhcpc -i mlan0 -t 3 -n");
    ret = doSystemCmd(syscmd);

    sprintf(mrs->log, "exec [%s]...\n", syscmd);
    print_f(&mrs->plog, "fs108", mrs->log);

    sync();
    
    modersp->r = 1; 
    return 1;
}

static int fs109(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int ret=0, len=0;
    char paramFilePath[128] = "/root/scaner/scannerParam.bin";
    FILE *f;
    struct aspConfig_s *pct=0;
    
    pct = mrs->configTable;
    len = ASPOP_CODE_MAX*sizeof(struct aspConfig_s);

    msync(pct, len, MS_SYNC);
    
    f = fopen(paramFilePath, "w+");
    if (f) {
        fwrite(pct, 1, len, f);
        fflush(f);
        fclose(f);
        sprintf(mrs->log, "Scanner parameter table save to [%s] size:%d\n", paramFilePath, len);
        print_f(&mrs->plog, "fs109", mrs->log);
    } else {
        sprintf(mrs->log, "Scanner parameter table save to [%s] failed !!!\n", paramFilePath);
        print_f(&mrs->plog, "fs109", mrs->log);
    }

    sync();
    
    modersp->r = 1; 
    return 1;
}

static int fs110(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    sprintf(mrs->log, "trigger spi0 \n");
    print_f(&mrs->plog, "fs110", mrs->log);

    mrs_ipc_put(mrs, "y", 1, 1);
    clock_gettime(CLOCK_REALTIME, &mrs->time[0]);

    modersp->m = modersp->m + 1;
    return 2;
}

static int fs111(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int ret, bitset;
    char ch;

    //sprintf(mrs->log, "%d\n", modersp->v++);
    //print_f(&mrs->plog, "fs111", mrs->log);

    //sleep(5);

    ret = mrs_ipc_get(mrs, &ch, 1, 1);
    if ((ret > 0) && (ch == 'Y')){

        sprintf(mrs->log, "spi 0 end, metaout get!\n");
        print_f(&mrs->plog, "fs111", mrs->log);
        
        modersp->m = 48;

#if PULL_LOW_AFTER_DATA
        bitset = 0;
        msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        sprintf(mrs->log, "set RDY pin %d\n",bitset);
        print_f(&mrs->plog, "fs111", mrs->log);
#endif
        usleep(210000);

        return 2;
    }

    return 0; 
}

static int fs112(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    //sprintf(mrs->log, "send notice to P6 for meta mass ready\n");
    //print_f(&mrs->plog, "fs112", mrs->log);

    mrs_ipc_put(mrs, "c", 1, 7);
    modersp->m = modersp->m + 1;
    return 0;
}

static int fs113(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int len=0;
    char ch=0;
    //sprintf(mrs->log, "check P6 getting the notice\n");
    //print_f(&mrs->plog, "fs113", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 7);
    if ((len > 0) && (ch == 'C')) {
        modersp->r = 1;
        return 1;
    }
    
    if ((len > 0) && (ch != 'C')) {
            sprintf(mrs->log, "FAIL!!send notice to P6 again!\n");
            print_f(&mrs->plog, "fs113", mrs->log);
            modersp->m = modersp->m - 1;        
            return 2;
    }

    return 0; 
}

static int fs114(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    return 1;
}

#define P0_LOG (0)
static int p0(struct mainRes_s *mrs)
{
#define PS_NUM 115

    int ret=0, len=0, tmp=0;
    char ch=0;

    struct modersp_s *modesw = aspMalloc(sizeof(struct modersp_s));
    if (modesw == 0) {
        sprintf(mrs->log, "modesw memory allocation fail \n");
        print_f(&mrs->plog, "P0", mrs->log);
    }

    struct fselec_s afselec[PS_NUM] = {{ 0, fs00},{ 1, fs01},{ 2, fs02},{ 3, fs03},{ 4, fs04},
                                 { 5, fs05},{ 6, fs06},{ 7, fs07},{ 8, fs08},{ 9, fs09},
                                 {10, fs10},{11, fs11},{12, fs12},{13, fs13},{14, fs14},
                                 {15, fs15},{16, fs16},{17, fs17},{18, fs18},{19, fs19},
                                 {20, fs20},{21, fs21},{22, fs22},{23, fs23},{24, fs24},
                                 {25, fs25},{26, fs26},{27, fs27},{28, fs28},{29, fs29},
                                 {30, fs30},{31, fs31},{32, fs32},{33, fs33},{34, fs34},
                                 {35, fs35},{36, fs36},{37, fs37},{38, fs38},{39, fs39},
                                 {40, fs40},{41, fs41},{42, fs42},{43, fs43},{44, fs44},
                                 {45, fs45},{46, fs46},{47, fs47},{48, fs48},{49, fs49},
                                 {50, fs50},{51, fs51},{52, fs52},{53, fs53},{54, fs54},
                                 {55, fs55},{56, fs56},{57, fs57},{58, fs58},{59, fs59},
                                 {60, fs60},{61, fs61},{62, fs62},{63, fs63},{64, fs64},
                                 {65, fs65},{66, fs66},{67, fs67},{68, fs68},{69, fs69},
                                 {70, fs70},{71, fs71},{72, fs72},{73, fs73},{74, fs74},
                                 {75, fs75},{76, fs76},{77, fs77},{78, fs78},{79, fs79},
                                 {80, fs80},{81, fs81},{82, fs82},{83, fs83},{84, fs84},
                                 {85, fs85},{86, fs86},{87, fs87},{88, fs88},{89, fs89},
                                 {90, fs90},{91, fs91},{92, fs92},{93, fs93},{94, fs94},
                                 {95, fs95},{96, fs96},{97, fs97},{98, fs98},{99, fs99},
                                 {100, fs100},{101, fs101},{102, fs102},{103, fs103},{104, fs104},
                                 {105, fs105},{106, fs106},{107, fs107},{108, fs108},{109, fs109},
                                 {110, fs110},{111, fs111},{112, fs112},{113, fs113},{114, fs114}};
                                 
    struct fselec_s errHdle[PS_NUM] = {{ 0, hd00},{ 1, hd01},{ 2, hd02},{ 3, hd03},{ 4, hd04},
                                 { 5, hd05},{ 6, hd06},{ 7, hd07},{ 8, hd08},{ 9, hd09},
                                 {10, hd10},{11, hd11},{12, hd12},{13, hd13},{14, hd14},
                                 {15, hd15},{16, hd16},{17, hd17},{18, hd18},{19, hd19},
                                 {20, hd20},{21, hd21},{22, hd22},{23, hd23},{24, hd24},
                                 {25, hd25},{26, hd26},{27, hd27},{28, hd28},{29, hd29},
                                 {30, hd30},{31, hd31},{32, hd32},{33, hd33},{34, hd34},
                                 {35, hd35},{36, hd36},{37, hd37},{38, hd38},{39, hd39},
                                 {40, hd40},{41, hd41},{42, hd42},{43, hd43},{44, hd44},
                                 {45, hd45},{46, hd46},{47, hd47},{48, hd48},{49, hd49},
                                 {50, hd50},{51, hd51},{52, hd52},{53, hd53},{54, hd54},
                                 {55, hd55},{56, hd56},{57, hd57},{58, hd58},{59, hd59},
                                 {60, hd60},{61, hd61},{62, hd62},{63, hd63},{64, hd64},
                                 {65, hd65},{66, hd66},{67, hd67},{68, hd68},{69, hd69},
                                 {70, hd70},{71, hd71},{72, hd72},{73, hd73},{74, hd74},
                                 {75, hd75},{76, hd76},{77, hd77},{78, hd78},{79, hd79},
                                 {80, hd80},{81, hd81},{82, hd82},{83, hd83},{84, hd84},
                                 {85, hd85},{86, hd86},{87, hd87},{88, hd88},{89, hd89},
                                 {90, hd90},{91, hd91},{92, hd92},{93, hd93},{94, hd94},
                                 {95, hd95},{96, hd96},{97, hd97},{98, hd98},{99, hd99},
                                 {100, hd100},{101, hd101},{102, hd102},{103, hd103},{104, hd104},
                                 {105, hd105},{106, hd106},{107, hd107},{108, hd108},{109, hd109},
                                 {110, hd110},{111, hd111},{112, hd112},{113, hd113},{114, hd114}};
    p0_init(mrs);

    modesw->m = -2;
    modesw->r = 0;
    modesw->d = 0;

    while (1) {
        //sprintf(mrs->log, ".\n");
        //print_f(&mrs->plog, "P0", mrs->log);
        len = mrs_ipc_get(mrs, &ch, 1, 0);
        if (len > 0) {
#if P0_LOG
            sprintf(mrs->log, "modesw.m:%d ch:%d\n", modesw->m, ch);
            print_f(&mrs->plog, "P0", mrs->log);
#endif
            if (modesw->m == -2) {
                if ((ch >=0) && (ch < PS_NUM)) {
                    modesw->m = ch;
                }
            } else {
                /* todo: interrupt state machine here */
                if (ch == 0x7f) {
                    ret = 0;
                    ret = (*errHdle[modesw->m].pfunc)(mrs, modesw);
                    if (!ret) {
                        modesw->m = -1;
                        modesw->d = -1;
                        modesw->r = 0xed;
                    }
                    sprintf(mrs->log, "!! Error handle !! m:%d d:%d ret:%d\n", modesw->m, modesw->d, ret);
                    print_f(&mrs->plog, "P0", mrs->log);
                }
            }
        }

        if ((modesw->m >= 0) && (modesw->m < PS_NUM)) {
            msync(modesw, sizeof(struct modersp_s), MS_SYNC);
            //sprintf(mrs->log, "pmode:%d rsp:%d - 1\n", modesw->m, modesw->r);
            //print_f(&mrs->plog, "P0", mrs->log);
            
            ret = (*afselec[modesw->m].pfunc)(mrs, modesw);

            msync(modesw, sizeof(struct modersp_s), MS_SYNC);
            
            //sprintf(mrs->log, "pmode:%d rsp:%d - 2, ret: %d\n", modesw->m, modesw->r, ret);
            //print_f(&mrs->plog, "P0", mrs->log);
            if (ret == 1) {
                tmp = modesw->m;
                modesw->m = -1;
            }
        }

        if (modesw->m == -1) {
#if P0_LOG
            sprintf(mrs->log, "pmode:%d rsp:%d - end\n", tmp, modesw->r);
            print_f(&mrs->plog, "P0", mrs->log);
#endif

            ch = modesw->r; /* response */
            modesw->r = 0;
            modesw->v = 0;
            modesw->d = 0;
            modesw->m = -2;
            tmp = -1;

            mrs_ipc_put(mrs, &ch, 1, 0);
        } else {
            mrs_ipc_put(mrs, "$", 1, 0);
        }

        usleep(1000);
    }

    p0_end(mrs);
    return 0;
}

#define P1_LOG (0)
static int p1(struct procRes_s *rs, struct procRes_s *rcmd)
{
    uint32_t px, pi;
    int ret = 0, ci, len, logcnt=0;
    char ch, cmd, cmdt, str[128] = "good", er=0;
    char *addr;
    uint32_t evt;

    sprintf(rs->logs, "p1\n");
    print_f(rs->plogs, "P1", rs->logs);
    struct psdata_s *stdata;
    stfunc pf[SMAX][PSMAX] = {{stspy_01, stspy_02, stspy_03, stspy_04, stspy_05}, //SPY
                            {stbullet_01, stbullet_02, stbullet_03, stbullet_04, stbullet_05}, //BULLET
                            {stlaser_01, stlaser_02, stlaser_03, stlaser_04, stlaser_05}, // LASER
                            {stdob_01, stdob_02, stdob_03, stdob_04, stdob_05}, // DOUBLEC
                            {stdob_06, stdob_07, stdob_08, stdob_09, stdob_10}, // DOUBLED
                            {streg_11, streg_12, streg_13, streg_14, streg_15}, // REGE
                            {streg_16, streg_17, stfat_18, stfat_19, stfat_20}, // REGF
                            {stfat_21, stfat_22, stfat_23, stfat_24, stfat_25}, // FATG
                            {stfat_26, stfat_27, stfat_28, stfat_29, stfat_30}, // FATH
                            {stsup_31, stsup_32, stsup_33, stsup_34, stsup_35}, // SUPI
                            {stsin_36, stdow_37, stupd_38, stupd_39, stupd_40}, // SINJ
                            {stsav_41, stsda_42, stsda_43, stsda_44, stsda_45}, // SAVK
                            {stsda_46, stsda_47, stsda_48, stsda_49, stsda_50}, // SDAL
                            {stsda_51, stsda_52, stsda_53, stsda_54, stsda_55}, // SDAM
                            {stsda_56, stsda_57, stsda_58, stsda_59, stsda_60}, // SDAN
                            {stsda_61, stsda_62, stwbk_63, stsdinit_64, stsdinit_65}, // SDAO
                            {stwtbak_66, stwtbak_67, stwtbak_68, stwtbak_69, stwtbak_70}, // WTBAKP
                            {stwtbak_71, stwtbak_72, stwtbak_73, stwtbak_74, stcrop_75}, // WTBAKQ
                            {stcrop_76, stcrop_77, stcrop_78, stcrop_79, stcrop_80}, // CROPR
                            {stvector_81, stvector_82, stapm_83, stapm_84, stapm_85}, // VECTORS
                            {stsparam_86, stsparam_87, stsparam_88, stcropmeta_89, stcropmeta_90}, //SAVPARM
                            {stcropmeta_91, stcropmeta_92, stcropmeta_93, stcropmeta_94, stcropmeta_95}}; //METAT

    p1_init(rs);
    stdata = rs->pstdata;
    // wait for ch from p0
    // state machine control
    stdata->rs = rs;
    pi = 0;    stdata->result = 0;    cmd = '\0';   cmdt = '\0';
    while (1) {
        //sprintf(rs->logs, "+\n");
        //print_f(rs->plogs, "P1", rs->logs);

//     {'d', 'p', '=', 'n', 't', 'a', 'e', 'f', 'b', 's', 'h', 'u', 'v', 'c', 'k', 'g', 'i', 'j', 'm', 'o', 'q', 'r', 'y'};
        cmd = '\0';
        ci = 0; 
        ci = rs_ipc_get(rcmd, &cmd, 1);
        while (ci > 0) {
            //sprintf(rs->logs, "%c\n", cmd);
            //print_f(rs->plogs, "P1", rs->logs);

            if (cmdt == '\0') {
                if (cmd == 'd') {
                    cmdt = cmd;
                    stdata->result = emb_stanPro(0, STINIT, BULLET, PSSET);
                } else if (cmd == 'p') {
                    cmdt = cmd;
                    stdata->result = emb_stanPro(0, STINIT, SPY, PSTSM);
                } else if (cmd == '=') {
                    cmdt = cmd;
                    stdata->result = emb_stanPro(0, STINIT, SMAX, PSMAX);
                } else if (cmd == 'n') {
                    cmdt = cmd;
                    stdata->result = emb_stanPro(0, STINIT, DOUBLEC, PSSET);
                } else if (cmd == 't') {
                    cmdt = cmd;
                    stdata->result = emb_stanPro(0, STINIT, DOUBLED, PSRLT);
                } else if (cmd == 'a') {
                    cmdt = cmd;
                    stdata->result = emb_stanPro(0, STINIT, DOUBLED, PSRLT);
                } else if (cmd == 'e') {
                    cmdt = cmd;
                    stdata->result = emb_stanPro(0, STINIT, REGE, PSRLT);
                } else if (cmd == 'f') {
                    cmdt = cmd;
                    stdata->result = emb_stanPro(0, STINIT, DOUBLED, PSTSM);
                } else if (cmd == 'b') {
                    cmdt = cmd;
                    #if 0 /* 1: boot with OP_SDINIT and OP_SDSTATUS, 0: nope */
                    stdata->result = emb_stanPro(0, STINIT, SDAO, PSRLT);
                    #else
                    stdata->result = emb_stanPro(0, STINIT, FATH, PSTSM);
                    #endif
                } else if (cmd == 's') {
                    cmdt = cmd;
                    stdata->result = emb_stanPro(0, STINIT, SUPI, PSWT);
                } else if (cmd == 'h') {
                    cmdt = cmd;
                    stdata->result = emb_stanPro(0, STINIT, SINJ, PSACT);
                } else if (cmd == 'u') {
                    cmdt = cmd;
                    stdata->result = emb_stanPro(0, STINIT, SINJ, PSWT);
                } else if (cmd == 'v') {
                    cmdt = cmd;
                    stdata->result = emb_stanPro(0, STINIT, SAVK, PSSET);
                } else if (cmd == 'c') {
                    cmdt = cmd;
                    stdata->result = emb_stanPro(0, STINIT, SDAM, PSSET);
                } else if (cmd == 'k') {
                    cmdt = cmd;
                    stdata->result = emb_stanPro(0, STINIT, SDAO, PSSET);
                } else if (cmd == 'g') {
                    cmdt = cmd;
                    stdata->result = emb_stanPro(0, STINIT, WTBAKQ, PSSET);
                } else if (cmd == 'i') {
                    cmdt = cmd;
                    stdata->result = emb_stanPro(0, STINIT, WTBAKQ, PSSET);
                } else if (cmd == 'j') {
                    cmdt = cmd;
                    stdata->result = emb_stanPro(0, STINIT, WTBAKQ, PSSET);
                } else if (cmd == 'm') {
                    cmdt = cmd;
                    stdata->result = emb_stanPro(0, STINIT, WTBAKQ, PSTSM);
                } else if (cmd == 'o') {
                    cmdt = cmd;
                    stdata->result = emb_stanPro(0, STINIT, VECTORS, PSSET);
                } else if (cmd == 'q') {
                    cmdt = cmd;
                    stdata->result = emb_stanPro(0, STINIT, VECTORS, PSWT);
                } else if (cmd == 'r') {
                    cmdt = cmd;
                    stdata->result = emb_stanPro(0, STINIT, SAVPARM, PSSET);
                } else if (cmd == 'y') {
                    cmdt = cmd;
                    stdata->result = emb_stanPro(0, STINIT, SAVPARM, PSTSM);
                } else if (cmd == 'z') {
                    cmdt = cmd;
                    stdata->result = emb_stanPro(0, STINIT, SPY, PSSET);
                }


                if (cmdt != '\0') {
                    evt = stdata->result;
                    pi = (evt >> 8) & 0xff;
                    px = (evt & 0xff);

                    sprintf(rs->logs, "cmdt:%c, [%d,%d] - 0\nSTART\n", cmdt, pi, px);
                    print_f(rs->plogs, "P1", rs->logs);

                    logcnt = 0;
                    break;
                }
            } else { /* command to interrupt state machine here */
                if (cmd == 'r') {
                    cmdt = cmd;
                    stdata->result = emb_result(stdata->result, BREAK);

                    evt = stdata->result;
                    pi = (evt >> 8) & 0xff;
                    px = (evt & 0xff);

                    sprintf(rs->logs, "cmdt:%c, [%d,%d] 0x%.8x - 0\nBREAK\n", cmdt, pi, px, stdata->result);
                    print_f(rs->plogs, "P1", rs->logs);
                }
            }
            ci = 0;
            ci = rs_ipc_get(rcmd, &cmd, 1);            
        }

        ret = 0; ch = '\0';
        ret = rs_ipc_get(rs, &ch, 1);
        
        if ((ret > 0) && (ch != '$')) {
#if P1_LOG
            sprintf(rs->logs, "rsp, ret:%d ch:0x%.2x\n", ret, ch);
            print_f(rs->plogs, "P1", rs->logs);
#endif
        }
        

        if (((ret > 0) && (ch != '$')) || (cmdt != '\0')){
            stdata->ansp0 = ch;
            msync(stdata, sizeof(struct psdata_s), MS_SYNC);
            evt = stdata->result;
            pi = (evt >> 8) & 0xff;
            px = (evt & 0xff);
/*
            sprintf(rs->logs, "[%d,%d] %c 0x%.2x 0x%.8x - 1\n", pi, px, cmdt, ch, stdata->result);
            print_f(rs->plogs, "P1", rs->logs);
*/
            if ((pi >= SMAX) && (px >= PSMAX)) {
                sprintf(rs->logs, "cmdt:%c do nothing\n", cmdt, ch);
                print_f(rs->plogs, "P1", rs->logs);
            }
            else if ((pi >= SMAX) || (px >= PSMAX)) {
                sprintf(rs->logs, "ERROR [%d,%d] %c 0x%.2x 0x%.8x - 2\n", pi, px, cmdt, ch, stdata->result);
                print_f(rs->plogs, "P1", rs->logs);

                er = cmdt;
                rs_ipc_put(rcmd, &er, 1);
                er = ch;
                rs_ipc_put(rcmd, &er, 1);

                sprintf(str, "<%c,0x%x,unexpectedFailed>", cmdt, ch);
                len = strlen(str);
                if (len >= 256) len = 255;
                str[len] = '\0';
                
                rs_ipc_put(rcmd, str, len+1);
                logcnt = 0;

                sprintf(rs->logs, "result:0x%.8x\nBROKEN\n", stdata->result);
                print_f(rs->plogs, "P1", rs->logs);

                cmdt = '\0'; 
                stdata->result = emb_stanPro(0, STINIT, SPY, PSSET);
                continue;
            } else {
                stdata->result = (*pf[pi][px])(stdata);
            }

            msync(stdata, sizeof(struct psdata_s), MS_SYNC);
            evt = stdata->result;
            pi = (evt >> 8) & 0xff;
            px = (evt & 0xff);
/*
            sprintf(rs->logs, "[%d,%d] %c 0x%.2x 0x%.8x - 3\n", pi, px, cmdt, ch, stdata->result);
            print_f(rs->plogs, "P1", rs->logs);
*/
            if ((pi >= SMAX) || (px >= PSMAX)) {
#if P1_LOG
                sprintf(rs->logs, "<%c,0x%x,done>\n", cmdt, ch);
                print_f(rs->plogs, "P1", rs->logs);
#endif
                if (((cmdt != '\0') && (cmdt != 'w')) && (px == PSMAX)) {
                    er = cmdt;
                    rs_ipc_put(rcmd, &er, 1);
                    er = ch;
                    rs_ipc_put(rcmd, &er, 1);

                    sprintf(str, "<%c,0x%x,done>", cmdt, ch);
                    len = strlen(str);
                    if (len >= 256) len = 255;
                    str[len] = '\0';
                    rs_ipc_put(rcmd, str, len+1);
                    logcnt = 0;

                    sprintf(rs->logs, "result:0x%.8x\nEND\n", stdata->result);
                    print_f(rs->plogs, "P1", rs->logs);
                }

                cmdt = '\0'; 
                //stdata.result = emb_stanPro(0, STINIT, SPY, PSSET);
                continue;
            }

            //sprintf(rs->logs, "comdt:%c ch:0x%x evt:0x%.8x\n", cmdt, ch, stdata.result);
            //print_f(rs->plogs, "P1", rs->logs);
        }
        else {
            //sprintf(rs->logs, ";\n");
            //print_f(rs->plogs, "P1", rs->logs);

            if (logcnt > 100) {
                //rs_ipc_put(rcmd, ";", 1);
                logcnt = 0;
            }
        }

        logcnt++;
        //rs_ipc_put(rs, &ch, 1);
        //usleep(100000);
    }

    p1_end(rs);
    return 0;
}

#define MSP_P4_SAVE_DAT (0)
#define MSP_P2_SAVE_DAT (1)
#define IN_SAVE (0)
#define TIME_MEASURE (0)
#define P2_TX_LOG (1)
#define P2_CMD_LOG (1)
#define P2_SIMPLE_LOG (1)
static int p2(struct procRes_s *rs)
{
    FILE *fp=0;
    char fatPath[128] = "/mnt/mmc2/fatTab.bin";
#if IN_SAVE
    char filename[128] = "/mnt/mmc2/tx/input_x3.bin";
    FILE *fin = NULL;
#endif

    //struct spi_ioc_transfer *tr = aspMalloc(sizeof(struct spi_ioc_transfer));
    struct spi_ioc_transfer *tr = rs->rspioc1;
    struct timespec tnow;
    struct sdParseBuff_s *pabuf=0;
    int px, pi=0, ret, len=0, opsz, cmode=0, tdiff, tlast, twait, tlen=0, maxsz=0;
    int bitset, totsz=0;
    uint16_t send16, recv16;
    uint32_t secStr=0, secLen=0, datLen=0, minLen=0;
    uint32_t fformat=0;
    struct info16Bit_s *p=0, *c=0;
    struct sdFAT_s *pfat=0;
    struct sdFATable_s   *pftb=0;
    struct sdbootsec_s   *psec=0;
    struct aspConfig_s *pct=0, *pdt=0;
    struct aspMetaData *pmeta;
    
    pmeta = rs->pmetain;

    char ch, str[128], rx8[4], tx8[4];
    char *addr, *laddr, *rx_buff;
    sprintf(rs->logs, "p2\n");
    print_f(rs->plogs, "P2", rs->logs);

    pct = rs->pcfgTable;
    pfat = rs->psFat;
    pftb = pfat->fatTable;
    psec = pfat->fatBootsec;
    
    c = &rs->pmch->cur;
    p = &rs->pmch->tmp;
    
    p2_init(rs);
    // wait for ch from p0
    // in charge of spi0 data mode
    // 'd': data mode, store the incomming infom into share memory
    // send 'd' to notice the p0 that we have incomming data chunk

    rx_buff = aspMalloc(SPI_TRUNK_SZ);
    ch = '2';

    while (1) {
        //sprintf(rs->logs, "!\n");
        //print_f(rs->plogs, "P2", rs->logs);

        ret = rs_ipc_get(rs, &ch, 1);
        if (ret > 0) {

            //sprintf(rs->logs, "recv ch: %c\n", ch);
            //print_f(rs->plogs, "P2", rs->logs);

            switch (ch) {
                case 'g':
                    cmode = 1;
                    break;
                case 'c':
                case 'C':
                    cmode = 2;
                    //sprintf(rs->logs, "cmode: %d - 1\n", cmode);
                    //print_f(rs->plogs, "P2", rs->logs);
                    break;
                case 'e':
                    pi = 0;
                case 'r':
                    cmode = 3;
                    break;
                case 'b':
                    cmode = 4;
                    break;
                case 'd':
                    cmode = 5;
                    break;
                case 'n':
                    cmode = 6;
                    break;
                case 's':
                    cmode = 7;
                    break;
                case 'k':
                    cmode = 9;
                    break;
                case 'u':
                    cmode = 10;
                    break;
                case 'f':
                    cmode = 11;
                    break;
                case 'w':
                    cmode = 12;
                    break;
                case 'x':
                    cmode = 13;
                    break;
                case 'y':
                    cmode = 14;
                    break;
                default:
                    break;
            }

            //sprintf(rs->logs, "cmode: %d - 2\n", cmode);
            //print_f(rs->plogs, "P2", rs->logs);
            
            if (cmode == 1) {
                //sprintf(rs->logs, "cmode: %d - 3\n", cmode);
                //print_f(rs->plogs, "P2", rs->logs);

                bitset = 1;
                msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
                //sprintf(rs->logs, "Check if RDY pin == 0 (pin:%d)\n", bitset);
                //print_f(rs->plogs, "P2", rs->logs);

                while (1) {
                    bitset = 1;
                    msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
                    //sprintf(rs->logs, "Get RDY pin: %d\n", bitset);
                    //print_f(rs->plogs, "P2", rs->logs);
                    if (bitset == 0) break;
                }
                if (!bitset) rs_ipc_put(rs, "G", 1);
            } else if (cmode == 2) {
                //sprintf(rs->logs, "cmode: %d - 4\n", cmode);
                //print_f(rs->plogs, "P2", rs->logs);

                len = 0;
                msync(rs->pmch, sizeof(struct machineCtrl_s), MS_SYNC);            
                send16 = pkg_info(&rs->pmch->cur);
                tx8[1] = send16 & 0xff;
                tx8[0] = (send16 >> 8) & 0xff;      
#if P2_CMD_LOG
                sprintf(rs->logs, "send 0x%.2x 0x%.2x \n", tx8[0], tx8[1]);
                print_f(rs->plogs, "P2", rs->logs);
#endif
                len = mtx_data(rs->spifd, rx8, tx8, 2, tr);

                msync(rx8, 4, MS_SYNC);                    
                
                if (len > 0) {
                    recv16 = rx8[1] | (rx8[0] << 8);
                    abs_info(&rs->pmch->get, recv16);
                    rs_ipc_put(rs, "C", 1);
                } else {

                    sprintf(rs->logs, "ch = X, len = %d\n", len);
                    print_f(rs->plogs, "P2", rs->logs);
                    
                    bitset = 0;
                    msp_spi_conf(rs->spifd, _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN

                    rs_ipc_put(rs, "X", 1);                
                }
#if P2_CMD_LOG
                sprintf(rs->logs, "recv 0x%.2x 0x%.2x len=%d\n", rx8[0], rx8[1], len);
                print_f(rs->plogs, "P2", rs->logs);
#endif

#if P2_SIMPLE_LOG
                sprintf(rs->logs, "tx 0x%.2x 0x%.2x rx 0x%.2x 0x%.2x \n",  tx8[0], tx8[1], rx8[0], rx8[1]);
                print_f(rs->plogs, "P2", rs->logs);
#endif
            } else if (cmode == 4) {
                //sprintf(rs->logs, "cmode: %d - 5\n", cmode);
                //print_f(rs->plogs, "P2", rs->logs);

                bitset = 0; 
                px = 0;
                //msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
                //sprintf(rs->logs, "Check if RDY pin == 1 (pin:%d)\n", bitset);
                //print_f(rs->plogs, "P2", rs->logs);
            
                while (1) {
                    bitset = 0;
                    msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
                    //sprintf(rs->logs, "Get RDY pin: %d\n", bitset);
                    //print_f(rs->plogs, "P2", rs->logs);
                    
                    if (bitset == 1) break;

                    px ++;
                    usleep(1000);

                    if (px > 50) {
                        break;
                    }
                }
                
                if (bitset) {
                    rs_ipc_put(rs, "B", 1);
                } else {
                    rs_ipc_put(rs, "b", 1);
                }
            }else if (cmode == 5) {
#if MSP_P2_SAVE_DAT
                ret = file_save_get(&rs->fdat_s[1], "/mnt/mmc2/tx/p2%d.dat");
                if (ret) {
                    sprintf(rs->logs, "get tx log data file error - %d, hold here\n", ret);
                    print_f(rs->plogs, "P2", rs->logs);         
                    while(1);
                } else {
                    sprintf(rs->logs, "get tx log data file ok - %d, f: %d\n", ret, rs->fdat_s[1]);
                    print_f(rs->plogs, "P2", rs->logs);         
                }
#endif
                //sprintf(rs->logs, "cmode: %d\n", cmode);
                //print_f(rs->plogs, "P2", rs->logs);

                pi = 0;  

                while (1) {
                    len = ring_buf_get_dual(rs->pdataRx, &addr, pi);
                    memset(addr, 0x55, len);
                    msync(addr, len, MS_SYNC);                    
#if  TIME_MEASURE
                    clock_gettime(CLOCK_REALTIME, rs->tm[0]);
#endif
#if SPI_KTHREAD_USE
                    opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr);  //SPI_IOC_PROBE_THREAD
                    while (opsz == 0) {
#if SPI_KTHREAD_DLY
                        usleep(1000);
                        sprintf(rs->logs, "kth opsz:%d\n", opsz);
                        print_f(rs->plogs, "P2", rs->logs);  
#endif
                        opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr); //kthread 
                    }
                    
#else
                    opsz = mtx_data(rs->spifd, addr, NULL, len, tr);

#endif        

                    if ((opsz > 0) && (opsz < SPI_TRUNK_SZ)) { // workaround to fit original design
                        opsz = 0 - opsz;
                    }
                    
#if SPI_TRUNK_FULL_FIX
                    if (opsz < 0) {
                        opsz = 0 - opsz;                    
                        if (opsz == SPI_TRUNK_SZ) {
                            opsz = 0 - opsz;    
                        } else {
                            sprintf(rs->logs, "Error!!! spi send tx failed, return %d \n", opsz);
                            print_f(rs->plogs, "P2", rs->logs);
                        }
                    }
#endif

#if MSP_P2_SAVE_DAT
                    msync(addr, len, MS_SYNC);                    
                    fwrite(addr, 1, len, rs->fdat_s[1]);
                    fflush(rs->fdat_s[1]);
#endif
                    //printf("0 spi %d\n", opsz);
#if P2_TX_LOG
                    sprintf(rs->logs, "r %d / %d\n", opsz, len);
                    print_f(rs->plogs, "P2", rs->logs);
#endif
                    msync(rs->tm, sizeof(struct timespec) * 2, MS_SYNC);                    
#if TIME_MEASURE 
                    clock_gettime(CLOCK_REALTIME, &tnow);

                    tdiff = time_diff(rs->tm[0], rs->tm[1], 1000);
                    if (tdiff == -1) {
                         tdiff = 0 - time_diff(rs->tm[1], rs->tm[0], 1000);
                    }

                    //sprintf(rs->logs, "t %d us\n", tdiff);
                    //print_f(rs->plogs, "P2", rs->logs);                 

                    if (tdiff < 0) {
                        sprintf(rs->logs, "!!t %d - %d!!\n", tdiff, len);
                        print_f(rs->plogs, "P2", rs->logs);
                    }
#endif
                    if (opsz < 0) break;
                    
                    //msync(addr, len, MS_SYNC);
                    ring_buf_prod_dual(rs->pdataRx, pi);
                    //shmem_dump(addr, 32);

                    rs_ipc_put(rs, "p", 1);
                    pi += 2;
                }

                opsz = 0 - opsz;

                if (opsz == 1) opsz = 0;

#if JPG_FFD9_CUT /* find 0xcffd9 in jpg */
                ret = cfgTableGetChk(pct, ASPOP_FILE_FORMAT, &fformat, ASPOP_STA_CON);    
                if (ret) {
                    fformat = 0;
                }
                
                if ((fformat == 0) || (fformat == FILE_FORMAT_JPG) || (fformat == FILE_FORMAT_PDF)) {
                    sprintf(rs->logs, "fformat: 0x%x !!!\n", fformat);
                    print_f(rs->plogs, "P2", rs->logs);    

                    /* search the offset of 0xffd9 */
                    ret = findEOF(addr, opsz);
                    if (ret > 0) {
                        memset(addr + ret + 2, 0xff, opsz - ret - 2);
                    }

                    sprintf(rs->logs, "fformat: 0x%x, ret: %d \n", fformat, ret);
                    print_f(rs->plogs, "P2", rs->logs);    
                }
#endif

                if (opsz > 0) {
                    ring_buf_prod_dual(rs->pdataRx, pi);
                }

                ring_buf_set_last_dual(rs->pdataRx, opsz, pi);
                rs_ipc_put(rs, "d", 1);
                sprintf(rs->logs, "spi0 recv end, the last sector size: %d\n", opsz);
                print_f(rs->plogs, "P2", rs->logs);

#if MSP_P2_SAVE_DAT
                fclose(rs->fdat_s[1]);
#endif
            }else if (cmode == 6) {
                sprintf(rs->logs, "cmode: %d\n", cmode);
                print_f(rs->plogs, "P2", rs->logs);

                pi = 0;  
                while (1) {
                    len = 0;
                    len = ring_buf_get(rs->pcmdRx, &addr);
                    if (len > 0) {
                        msync(addr, len, MS_SYNC);

                        opsz = 0;
                        while (opsz == 0) {
#if SPI_KTHREAD_USE                    
                            opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr);  //SPI_IOC_PROBE_THREAD
                            if (opsz == 0) {
#if SPI_KTHREAD_DLY
                                usleep(100000);
                                sprintf(rs->logs, "kth opsz:%d\n", opsz);
                                print_f(rs->plogs, "P2", rs->logs);  
#endif
                                continue;
                            }
#else
                            opsz = mtx_data(rs->spifd, addr, NULL, len, tr);
#endif
                            if ((opsz > 0) && (opsz < SPI_TRUNK_SZ)) { // workaround to fit original design
                                opsz = 0 - opsz;
                            }

#if SPI_TRUNK_FULL_FIX
                            if (opsz < 0) {
                                opsz = 0 - opsz;                    
                                if (opsz == SPI_TRUNK_SZ) {
                                    opsz = 0 - opsz;    
                                } else {
                                    sprintf(rs->logs, "Error!!! spi send tx failed, return %d \n", opsz);
                                    print_f(rs->plogs, "P2", rs->logs);
                                }
                            }
#endif

                            //usleep(10000);
#if P2_TX_LOG
                            sprintf(rs->logs, "r %d - %d\n", opsz, pi);
                            print_f(rs->plogs, "P2", rs->logs);
#endif
                        }

                        //msync(addr, len, MS_SYNC);
                        ring_buf_prod(rs->pcmdRx);    
                        if (opsz < 0) {
                            //sprintf(rs->logs, "opsz:%d break!\n", opsz);
                            //print_f(rs->plogs, "P2", rs->logs);    
                            break;
                        }
                        rs_ipc_put(rs, "p", 1);
                        pi += 1;
                    }
                }

                sprintf(rs->logs, "len:%d opsz:%d ret:%d, break!\n", len, opsz, ret);
                print_f(rs->plogs, "P2", rs->logs);    

                opsz = 0 - opsz;

#if JPG_FFD9_CUT /* find 0xcffd9 in jpg */
                ret = cfgTableGetChk(pct, ASPOP_FILE_FORMAT, &fformat, ASPOP_STA_CON);    
                if (ret) {
                    fformat = 0;
                }
                
                if ((fformat == 0) || (fformat == FILE_FORMAT_JPG) || (fformat == FILE_FORMAT_PDF)) {
                    sprintf(rs->logs, "fformat: 0x%x !!!\n", fformat);
                    print_f(rs->plogs, "P2", rs->logs);    

                    /* search the offset of 0xffd9 */
                    ret = findEOF(addr, opsz);
                    if (ret > 0) {
                        memset(addr + ret + 2, 0xff, opsz - ret - 2);
                    }

                    sprintf(rs->logs, "fformat: 0x%x, ret: %d \n", fformat, ret);
                    print_f(rs->plogs, "P2", rs->logs);    
                }
#endif
                
                ring_buf_set_last(rs->pcmdRx, opsz);
                rs_ipc_put(rs, "d", 1);
                sprintf(rs->logs, "spi0 recv end\n");
                print_f(rs->plogs, "P2", rs->logs);

            }
            else if (cmode == 7) {
                secStr = c->opinfo;
                secLen = p->opinfo;
                datLen = secLen * 512;
                minLen = 16 * 512;

                pabuf = &rs->psFat->fatDirPool->parBuf;
                sprintf(rs->logs, "cmode: %d, ready for rx %d/%d, secStr:%d, secLen:%d\n", cmode, datLen, minLen, secStr, secLen);
                print_f(rs->plogs, "P2", rs->logs);
/*
                if (datLen < minLen) {
                    datLen = minLen;
                }
*/
                addr = pabuf->dirParseBuff + pabuf->dirBuffUsed;
                len = 0;
                pi = 0;  
                while (1) {
#if SPI_KTHREAD_USE
                    opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr);  //SPI_IOC_PROBE_THREAD
                    while (opsz == 0) {
#if SPI_KTHREAD_DLY
                        usleep(1000);
                        sprintf(rs->logs, "kth opsz:%d\n", opsz);
                        print_f(rs->plogs, "P2", rs->logs);  
#endif
                        opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr);  //SPI_IOC_PROBE_THREAD
                    }
#else
                    opsz = mtx_data(rs->spifd, addr, NULL, SPI_TRUNK_SZ, tr);
#endif

                    if ((opsz > 0) && (opsz < SPI_TRUNK_SZ)) { // workaround to fit original design
                        opsz = 0 - opsz;
                    }
                    
#if SPI_TRUNK_FULL_FIX
                    if (opsz < 0) {
                        opsz = 0 - opsz;                    
                        if (opsz == SPI_TRUNK_SZ) {
                            opsz = 0 - opsz;    
                        } else {
                            sprintf(rs->logs, "Error!!! spi send tx failed, return %d \n", opsz);
                            print_f(rs->plogs, "P2", rs->logs);
                        }
                    }
#endif

#if P2_TX_LOG
                    sprintf(rs->logs, "r %d\n", opsz);
                    print_f(rs->plogs, "P2", rs->logs);
#endif

                    if (opsz == 0) {
                        sprintf(rs->logs, "opsz:%d\n", opsz);
                        print_f(rs->plogs, "P2", rs->logs);    
                        continue;
                    }

                    if (opsz < 0) break;
                    
                    addr += opsz;
                    len += opsz;
                    pi += 1;
                }

                opsz = 0 - opsz;
                if (opsz == 1) opsz = 0;

                len += opsz;

                if (len != datLen) {
                    sprintf(rs->logs, "total len: %d, actual len: %d\n", len, datLen);
                    print_f(rs->plogs, "P2", rs->logs);
                    len = datLen; 
                }

                pabuf->dirBuffUsed += datLen;
                msync(pabuf->dirParseBuff, len, MS_SYNC);
                rs_ipc_put(rs, "S", 1);

                sprintf(rs->logs, "spi0 recv end - total len: %d\n", len);
                print_f(rs->plogs, "P2", rs->logs);
            }
            else if (cmode == 8) {
                pi = 0;
                while (1) {
                    len = ring_buf_cons(rs->pdataTx, &addr);
                    if (len >= 0) {
                        msync(addr, len, MS_SYNC);
                        if (len != 0) {
                            pi++;
                            #if 1
                            opsz = mtx_data(rs->spifd, addr, NULL, SPI_TRUNK_SZ, tr);
                            #else
                            opsz = len;
                            #endif
                            //sprintf(rs->logs, "t %d c %d \n", opsz, pi);
                            //print_f(rs->plogs, "P2", rs->logs);         

                            if (opsz < 0) break;
                        }
                    } else {
                        sprintf(rs->logs, "tx len:%d cnt:%d- end\n", opsz, pi);
                        print_f(rs->plogs, "P2", rs->logs);         
                        break;
                    }

                    if (ch != 'K') {
                        ch = 0;
                        rs_ipc_get(rs, &ch, 1);
                        sprintf(rs->logs, "ch:%c\n", ch);
                        print_f(rs->plogs, "P2", rs->logs);         
                    }
                }

                while (ch != 'K') {
                    sprintf(rs->logs, "%c clr\n", ch);
                    print_f(rs->plogs, "P2", rs->logs);         
                    ch = 0;
                    rs_ipc_get(rs, &ch, 1);
                }
                rs_ipc_put(rs, "K", 1);
                sprintf(rs->logs, "tx cnt:%d - end\n", pi);
                print_f(rs->plogs, "P2", rs->logs);         
            }
            else if (cmode == 9) {
                totsz = 0;
                pi = 0;
                sprintf(rs->logs, "cmode: %d \n", cmode);
                print_f(rs->plogs, "P2", rs->logs);
                
                while (1) {
                    if (ch != 'K') {
                        rs_ipc_get(rs, &ch, 1);
                    }


                    len = ring_buf_cons(rs->pdataTx, &addr);

                    //sprintf(rs->logs, "t %d\n", len);
                    //print_f(rs->plogs, "P2", rs->logs);          

                    if (len > 0) {
                        if (len < SPI_TRUNK_SZ) len = SPI_TRUNK_SZ;
#if SPI_KTHREAD_USE
                    opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr);  //SPI_IOC_PROBE_THREAD
                    while (opsz == 0) {
#if SPI_KTHREAD_DLY
                        usleep(1000);
                        sprintf(rs->logs, "kth opsz:%d\n", opsz);
                        print_f(rs->plogs, "P2", rs->logs);  
#endif
                        opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr); //kthread 
                    }
#else
                        opsz = mtx_data(rs->spifd, addr, addr, len, tr);
#endif
                        if ((opsz > 0) && (opsz < SPI_TRUNK_SZ)) { // workaround to fit original design
                            opsz = 0 - opsz;
                        }

#if SPI_TRUNK_FULL_FIX
                        if (opsz < 0) {
                            opsz = 0 - opsz;                    
                            if (opsz == SPI_TRUNK_SZ) {
                                opsz = 0 - opsz;    
                            } else {
                                sprintf(rs->logs, "Error!!! spi send tx failed, return %d \n", opsz);
                                print_f(rs->plogs, "P2", rs->logs);
                            }
                        }
#endif

                        pi++;
                        if (opsz < 0) break;
                        totsz += opsz;
                    }
                    if ((len < 0) && (ch == 'K')) break;
                }

                opsz = 0 - opsz;
                if (opsz > 0) {
                    totsz += opsz;
                }

                while (ch != 'K') {
                    rs_ipc_get(rs, &ch, 1);
                }

                rs_ipc_put(rs, "K", 1);
                sprintf(rs->logs, "tx cnt:%d - end\n", pi);
                print_f(rs->plogs, "P2", rs->logs);         
            }

            else if (cmode == 10) {
#if MSP_P2_SAVE_DAT
                ret = file_save_get(&rs->fdat_s[1], "/mnt/mmc2/tx/p2%d.dat");
                if (ret) {
                    sprintf(rs->logs, "get tx log data file error - %d, hold here\n", ret);
                    print_f(rs->plogs, "P2", rs->logs);         
                    while(1);
                } else {
                    sprintf(rs->logs, "get tx log data file ok - %d, f: %d\n", ret, rs->fdat_s[1]);
                    print_f(rs->plogs, "P2", rs->logs);         
                }
#endif

                totsz = 0;
                pi = 0;
                sprintf(rs->logs, "cmode: %d \n", cmode);
                print_f(rs->plogs, "P2", rs->logs);
                
                while (1) {
                    if (ch != 'U') {
                        rs_ipc_get(rs, &ch, 1);
                    }

                    len = ring_buf_cons(rs->pcmdTx, &addr);
                    
                    /* debug */
                    if (pi == 0) {
                        sprintf(rs->logs, "dump the first 128 byte of SD data: \n");
                        print_f(rs->plogs, "P2", rs->logs);  
                        shmem_dump(addr, 128);
                    }

#if P2_TX_LOG
                    sprintf(rs->logs, "u t %d\n", len);
                    print_f(rs->plogs, "P2", rs->logs);          
#endif

                    if (len > 0) {
                    msync(addr, len, MS_SYNC); 
#if MSP_P2_SAVE_DAT
                    fwrite(addr, 1, len, rs->fdat_s[1]);
                    fflush(rs->fdat_s[1]);
#endif

                        if (len < SPI_TRUNK_SZ) len = SPI_TRUNK_SZ;

#if SPI_KTHREAD_USE & SPI_UPD_NO_KTHREAD
                    opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr);  //SPI_IOC_PROBE_THREAD
                    while (opsz == 0) {
#if SPI_KTHREAD_DLY
                        usleep(1000);
                        sprintf(rs->logs, "kth opsz:%d\n", opsz);
                        print_f(rs->plogs, "P2", rs->logs);  
#endif
                        opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr); //kthread 
                    }
#else
                        opsz = mtx_data(rs->spifd, addr, addr, len, tr);
#endif

                         sprintf(rs->logs, "opsz: %d \n", opsz);
                         print_f(rs->plogs, "P2", rs->logs);

                        if ((opsz > 0) && (opsz < SPI_TRUNK_SZ)) { // workaround to fit original design
                            opsz = 0 - opsz;
                        }
                        
#if 1 /* SPI_TRUNK_FULL_FIX, the full trunk in non-kthread data mode need this */
                        if (opsz < 0) {
                            opsz = 0 - opsz;                    
                            if (opsz == SPI_TRUNK_SZ) {
                                //opsz = 0 - opsz;    
                            } else {
                                sprintf(rs->logs, "Error!!! spi send tx failed, return %d \n", opsz);
                                print_f(rs->plogs, "P2", rs->logs);
                            }
                        }
#endif

                        pi++;
                        if (opsz < 0) break;
                        totsz += opsz;
                    }
                    if ((len < 0) && (ch == 'U')) break;
                }

                opsz = 0 - opsz;
                if (opsz > 0) {
                    totsz += opsz;
                }

                while (ch != 'U') {
                    rs_ipc_get(rs, &ch, 1);
                }

                rs_ipc_put(rs, "U", 1);
                sprintf(rs->logs, "tx cnt:%d - end\n", pi);
                print_f(rs->plogs, "P2", rs->logs);            

#if MSP_P2_SAVE_DAT
                fclose(rs->fdat_s[1]);
#endif
            }

            else if (cmode == 11) {
                fp = fopen(fatPath, "r");

                ret = fseek(fp, 0, SEEK_END);
                if (ret) {
                    sprintf(rs->logs, " file seek failed!! ret:%d \n", ret);
                    print_f(rs->plogs, "P2", rs->logs);
                } 
                
                maxsz = ftell(fp);
                sprintf(rs->logs, " file [%s] size: %d \n", fatPath, maxsz);
                print_f(rs->plogs, "P2", rs->logs);

                ret = fseek(fp, 0, SEEK_SET);
                if (ret) {
                    sprintf(rs->logs, " file seek failed!! ret:%d \n", ret);
                    print_f(rs->plogs, "P2", rs->logs);
                }

                addr = aspMalloc(maxsz);

                ret = fread(addr, 1, maxsz, fp);

                fclose(fp);

                sprintf(rs->logs, "FAT file read size: %d/%d \n", ret, maxsz);
                print_f(rs->plogs, "P2", rs->logs);
                
                secStr = c->opinfo;
                secLen = p->opinfo;
                datLen = secLen * 512;
                minLen = 16 * 512;

                sprintf(rs->logs, "ready for rx %d/%d, %d/%d\n", secStr, psec->secWhfat, datLen, minLen);
                print_f(rs->plogs, "P2", rs->logs);
/*
                if (datLen < minLen) {
                    datLen = minLen;
                }
*/
                laddr = addr;
                addr = addr + (secStr - psec->secWhfat) * 512;

                sprintf(rs->logs, "laddr:0x%.8x, addr:0x%.8x\n", laddr, addr);
                print_f(rs->plogs, "P2", rs->logs);

                totsz = datLen;
                len = 0;
                pi = 0;  
                
                while (1) {
                    tlen = SPI_TRUNK_SZ;
                    
#if SPI_KTHREAD_USE & SPI_UPD_NO_KTHREAD
                    opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr);  //SPI_IOC_PROBE_THREAD
                    while (opsz == 0) {
#if SPI_KTHREAD_DLY
                        usleep(1000);
                        sprintf(rs->logs, "kth opsz:%d\n", opsz);
                        print_f(rs->plogs, "P2", rs->logs);  
#endif
                        opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr);  //SPI_IOC_PROBE_THREAD
                    }
#else
                    msync(addr, tlen, MS_SYNC);                    
                    //shmem_dump(addr, datLen);
                    opsz = mtx_data(rs->spifd, addr, addr, tlen, tr);
#endif

                    if ((opsz > 0) && (opsz < SPI_TRUNK_SZ)) { // workaround to fit original design
                        opsz = 0 - opsz;
                    }

#if SPI_TRUNK_FULL_FIX
                    if (opsz < 0) {
                        opsz = 0 - opsz;                    
                        if (opsz == SPI_TRUNK_SZ) {
                            opsz = 0 - opsz;    
                        } else {
                            sprintf(rs->logs, "Error!!! spi send tx failed, return %d \n", opsz);
                            print_f(rs->plogs, "P2", rs->logs);
                        }
                    }
#endif

#if P2_TX_LOG
                    sprintf(rs->logs, "r (%d)\n", opsz);
                    print_f(rs->plogs, "P2", rs->logs);
#endif
                    if (opsz == 0) {
                        sprintf(rs->logs, "opsz:%d\n", opsz);
                        print_f(rs->plogs, "P2", rs->logs);    
                        continue;
                    }

                    if (opsz < 0) break;

                    len += opsz;
                    addr += opsz;
                    
                    pi += 1;
                }

                opsz = 0 - opsz;
                if (opsz == 1) opsz = 0;

                len += opsz;

                if (len != datLen) {
                    sprintf(rs->logs, "total len: %d, actual len: %d\n", len, datLen);
                    print_f(rs->plogs, "P2", rs->logs);
                    len = datLen; 
                }

                aspFree(laddr);
                rs_ipc_put(rs, "F", 1);

                sprintf(rs->logs, "spi0 recv end - total len: %d\n", len);
                print_f(rs->plogs, "P2", rs->logs);
            }
            else if (cmode == 12) {
                secStr = c->opinfo;
                secLen = p->opinfo;
                datLen = secLen * 512;
                minLen = 16 * 512;

                pabuf = &rs->psFat->fatDirPool->parBuf;
                sprintf(rs->logs, "cmode: %d, ready for rx %d/%d\n", cmode, datLen, minLen);
                print_f(rs->plogs, "P2", rs->logs);
/*
                if (datLen < minLen) {
                    datLen = minLen;
                }
*/

                addr = pabuf->dirParseBuff;
                msync(addr, psec->secPrClst*512, MS_SYNC);
                //shmem_dump(addr, psec->secPrClst*512);
                
                len = 0;
                pi = 0;  
                while (1) {
#if SPI_KTHREAD_USE & SPI_UPD_NO_KTHREAD
                    opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr);  //SPI_IOC_PROBE_THREAD
                    while (opsz == 0) {
#if SPI_KTHREAD_DLY
                        usleep(1000);
                        sprintf(rs->logs, "kth opsz:%d\n", opsz);
                        print_f(rs->plogs, "P2", rs->logs);  
#endif
                        opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr);  //SPI_IOC_PROBE_THREAD
                    }
#else
                    msync(addr, SPI_TRUNK_SZ, MS_SYNC);
                    opsz = mtx_data(rs->spifd, rx_buff, addr, SPI_TRUNK_SZ, tr);
#endif

                    if ((opsz > 0) && (opsz < SPI_TRUNK_SZ)) { // workaround to fit original design
                        opsz = 0 - opsz;
                    }

#if SPI_TRUNK_FULL_FIX
                    if (opsz < 0) {
                        opsz = 0 - opsz;                    
                        if (opsz == SPI_TRUNK_SZ) {
                            opsz = 0 - opsz;    
                        } else {
                            sprintf(rs->logs, "Error!!! spi send tx failed, return %d \n", opsz);
                            print_f(rs->plogs, "P2", rs->logs);
                        }
                    }
#endif

#if P2_TX_LOG
                    sprintf(rs->logs, "r %d\n", opsz);
                    print_f(rs->plogs, "P2", rs->logs);
#endif
                    if (opsz == 0) {
                        sprintf(rs->logs, "opsz:%d\n", opsz);
                        print_f(rs->plogs, "P2", rs->logs);    
                        continue;
                    }

                    if (opsz < 0) break;
                    
                    addr += opsz;
                    len += opsz;
                    pi += 1;
                }

                opsz = 0 - opsz;
                if (opsz == 1) opsz = 0;

                len += opsz;

                if (len != datLen) {
                    sprintf(rs->logs, "total len: %d, actual len: %d\n", len, datLen);
                    print_f(rs->plogs, "P2", rs->logs);
                    len = datLen; 
                }

                rs_ipc_put(rs, "W", 1);

                sprintf(rs->logs, "spi0 recv end - total len: %d\n", len);
                print_f(rs->plogs, "P2", rs->logs);
            }
            else if (cmode == 13) {
                totsz = 0;
                len = 0;
                pi = 0;  
                while (1) {
                    len = 0;
                    len = ring_buf_get(rs->pcmdRx, &addr);
                    if (len > 0) {
                        msync(addr, len, MS_SYNC);

                        opsz = 0;
                        while (opsz == 0) {
#if SPI_KTHREAD_USE                    
                            opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr);  //SPI_IOC_PROBE_THREAD
                            if (opsz == 0) {
#if SPI_KTHREAD_DLY
                                usleep(1000);
                                sprintf(rs->logs, "kth opsz:%d\n", opsz);
                                print_f(rs->plogs, "P2", rs->logs);  
#endif
                                continue;
                            }
#else
                            opsz = mtx_data(rs->spifd, addr, NULL, len, tr);
#endif
                            if ((opsz > 0) && (opsz < SPI_TRUNK_SZ)) { // workaround to fit original design
                                opsz = 0 - opsz;
                            }

#if SPI_TRUNK_FULL_FIX
                            if (opsz < 0) {
                                opsz = 0 - opsz;                    
                                if (opsz == SPI_TRUNK_SZ) {
                                    opsz = 0 - opsz;    
                                } else {
                                    sprintf(rs->logs, "Error!!! spi send tx failed, return %d \n", opsz);
                                    print_f(rs->plogs, "P2", rs->logs);
                                }

                            }
#endif

                            //usleep(10000);
#if P2_TX_LOG
                            sprintf(rs->logs, "spi0 recv %d\n", opsz);
                            print_f(rs->plogs, "P2", rs->logs);
#endif
                        }

                        //msync(addr, len, MS_SYNC);
                        ring_buf_prod(rs->pcmdRx);    
                        if (opsz < 0) {
                            sprintf(rs->logs, "opsz:%d break!\n", opsz);
                            print_f(rs->plogs, "P2", rs->logs);    
                            break;
                        }
                        rs_ipc_put(rs, "x", 1);
                        pi += 1;
                        totsz += opsz;
                    }
                }

                sprintf(rs->logs, "len:%d opsz:%d ret:%d, break!\n", len, opsz, ret);
                print_f(rs->plogs, "P2", rs->logs);    

                opsz = 0 - opsz;
                if (opsz == 1) opsz = 0;
                
                ring_buf_set_last(rs->pcmdRx, opsz);
                totsz += opsz;
                
                rs_ipc_put(rs, "X", 1);
                sprintf(rs->logs, "spi0 recv end, total: %d\n", totsz);
                print_f(rs->plogs, "P2", rs->logs);

            }
            else if (cmode == 14) {
                totsz = 0;
                len = 0;
                pi = 0;  
                len = 0;

                //len = 512;
                len = SPI_TRUNK_SZ;
                addr = (char *)rs->pmetaout;
                //laddr = (char *)rs->pmetain;
                laddr = (char *)rs->pmetaMass->masspt;
                
                memcpy(laddr, addr, 512);
                msync(laddr, 512, MS_SYNC);

                opsz = 0;
                while (opsz == 0) {
                    opsz = mtx_data(rs->spifd, laddr, laddr, len, tr);

                    if ((opsz > 0) && (opsz < SPI_TRUNK_SZ)) { // workaround to fit original design
                        opsz = 0 - opsz;
                    }

                    //usleep(10000);
#if P2_TX_LOG
                    sprintf(rs->logs, "spi0 recv %d\n", opsz);
                    print_f(rs->plogs, "P2", rs->logs);
#endif
                    //shmem_dump(addr, 512);
                    
                    sprintf(rs->logs, "meta get magic number: 0x%.2x 0x%.2x \n", pmeta->ASP_MAGIC[0], pmeta->ASP_MAGIC[1]);
                    print_f(rs->plogs, "P2", rs->logs);
                    
                    if (opsz < 0) {
                        //sprintf(rs->logs, "opsz:%d break!\n", opsz);
                        //print_f(rs->plogs, "P2", rs->logs);    
                        break;
                    }

                }
                
                rs_ipc_put(rs, "Y", 1);

                pi += 1;
                
                opsz = 0 - opsz;
                if (opsz == 1) opsz = 0;
                totsz += opsz;

                memcpy(rs->pmetain, laddr, 512);
                msync(rs->pmetain, 512, MS_SYNC);

                if (totsz > rs->pmetaMass->massMax) {
                    rs->pmetaMass->massUsed = rs->pmetaMass->massMax;
                } else {
                    rs->pmetaMass->massUsed = totsz;
                }
                
                sprintf(rs->logs, "totsz: %d, len:%d opsz:%d ret:%d, break!\n", totsz, len, opsz, ret);
                print_f(rs->plogs, "P2", rs->logs);
            }
            else {
                sprintf(rs->logs, "cmode: %d \n", cmode);
                print_f(rs->plogs, "P2", rs->logs);
            }

        }
    }

    p2_end(rs);
    return 0;
}

#define P3_TX_LOG  (0)
static int p3(struct procRes_s *rs)
{
#if IN_SAVE
    char filename[128] = "/mnt/mmc2/tx/input_x2.bin";
    FILE *fin = NULL;
#endif

    //struct spi_ioc_transfer *tr = aspMalloc(sizeof(struct spi_ioc_transfer));
    struct spi_ioc_transfer *tr = rs->rspioc2;
    struct timespec tnow;
    struct aspConfig_s *pct=0, *pdt=0;
    
    int pi, ret, len, opsz, cmode, bitset, tdiff, tlast, twait;
    uint16_t send16, recv16;
    char ch, str[128], rx8[4], tx8[4];
    char *addr, *laddr;
    uint32_t fformat=0;

    pct = rs->pcfgTable;
    
    sprintf(rs->logs, "p3, spidev:%d \n", rs->spifd);
    print_f(rs->plogs, "P3", rs->logs);

    while (!rs->spifd) {
        ch = 0;
        ret = rs_ipc_get(rs, &ch, 1);
        if (ret > 0) {
            sprintf(rs->logs, "get ch:%c\n", ch);
            print_f(rs->plogs, "P3", rs->logs);
            rs_ipc_put(rs, "x", 1);
        }
    }

    p3_init(rs);
    // wait for ch from p0
    // in charge of spi1 data mode
    // 'd': data mode, forward the socket incoming inform into share memory

    pi = 0;
    while (1) {
        //sprintf(rs->logs, "/\n");
        //print_f(rs->plogs, "P3", rs->logs);

        ret = rs_ipc_get(rs, &ch, 1);
        if (ret > 0) {
            //sprintf(rs->logs, "recv ch: %c\n", ch);
            //print_f(rs->plogs, "P3", rs->logs);

            switch (ch) {
                case 'g':
                    cmode = 1;
                    break;
                case 'c':
                    cmode = 2;
                    break;
                case 'e':
                    pi = 0;
                case 'r':
                    cmode = 3;
                    break;
                case 'b':
                    cmode = 4;
                    break;
                case 'd':
                    cmode = 5;
                    break;
                case 'n':
                    cmode = 6;
                    break;
                default:
                    break;
            }
            if (cmode == 1) {
                bitset = 1;
                msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
                //sprintf(rs->logs, "Check if RDY pin == 0 (pin:%d)\n", bitset);
                //print_f(rs->plogs, "P3", rs->logs);

                while (1) {
                    bitset = 1;
                    msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
                    //sprintf(rs->logs, "Get RDY pin: %d\n", bitset);
                    //print_f(rs->plogs, "P3", rs->logs);
                    if (bitset == 0) break;
                }
                if (!bitset) rs_ipc_put(rs, "G", 1);
            } else if (cmode == 2) {
                len = 0;
                msync(rs->pmch, sizeof(struct machineCtrl_s), MS_SYNC);            
                send16 = pkg_info(&rs->pmch->cur);
                tx8[0] = send16 & 0xff;
                tx8[1] = (send16 >> 8) & 0xff;      
                len = mtx_data(rs->spifd, rx8, tx8, 2, tr);
                if (len > 0) {
                    recv16 = rx8[0] | (rx8[1] << 8);
                    abs_info(&rs->pmch->get, recv16);
                    rs_ipc_put(rs, "C", 1);
                }
            } else if (cmode == 4) {
                bitset = 0;
                msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
                //sprintf(rs->logs, "Check if RDY pin == 1 (pin:%d)\n", bitset);
                //print_f(rs->plogs, "P3", rs->logs);
            
                while (1) {
                    bitset = 0;
                    msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
                    //sprintf(rs->logs, "Get RDY pin: %d\n", bitset);
                    //print_f(rs->plogs, "P4", rs->logs);
            
                    if (bitset == 1) break;
                }
                if (bitset) rs_ipc_put(rs, "B", 1);
            } else if (cmode == 5) {
                //sprintf(rs->logs, "cmode: %d\n", cmode);
                //print_f(rs->plogs, "P3", rs->logs);

                pi = 1;  

                while (1) {
                    len = ring_buf_get_dual(rs->pdataRx, &addr, pi);
                    memset(addr, 0xaa, len);
                    msync(addr, len, MS_SYNC);
#if TIME_MEASURE
                    clock_gettime(CLOCK_REALTIME, rs->tm[1]);
#endif
#if SPI1_ENABLE
#if SPI_KTHREAD_USE
                    opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr);  //SPI_IOC_PROBE_THREAD
                    while (opsz == 0) {
#if SPI_KTHREAD_DLY
                        usleep(1000);
                        sprintf(rs->logs, "kth opsz:%d\n", opsz);
                        print_f(rs->plogs, "P2", rs->logs);  
#endif
                        opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr); //kthread 
                    }
#else // #if SPI_KTHREAD_USE
                    opsz = mtx_data(rs->spifd, addr, NULL, len, tr);
#endif // #if SPI_KTHREAD_USE

                    if ((opsz > 0) && (opsz < SPI_TRUNK_SZ)) { // workaround to fit original design
                        opsz = 0 - opsz;
                    }

#if SPI_TRUNK_FULL_FIX
                    if (opsz < 0) {
                        opsz = 0 - opsz;                    
                        if (opsz == SPI_TRUNK_SZ) {
                            opsz = 0 - opsz;    
                        } else {
                            sprintf(rs->logs, "Error!!! spi send tx failed, return %d \n", opsz);
                            print_f(rs->plogs, "P2", rs->logs);
                        }
                    }
#endif

#else // #if SPI1_ENABLE
                    opsz = SPI_TRUNK_SZ;
#endif // #if SPI1_ENABLE

#if SPI1_ENABLE
#if IN_SAVE
                   msync(addr, len, MS_SYNC);
                   fin = fopen(filename, "a+");
                    if (!fin) {
                        sprintf(rs->logs, "file read [%s] failed \n", filename);
                        print_f(rs->plogs, "P4", rs->logs);
                    } else {
                        sprintf(rs->logs, "file read [%s] ok \n", filename);
                        print_f(rs->plogs, "P4", rs->logs);

                        ret = fwrite(addr, 1, len, fin);

                        fflush(fin);
                        fclose(fin);
                        fin = NULL;
                }
#endif
#if P3_TX_LOG
                    sprintf(rs->logs, "r %d / %d\n", opsz, len);
                    print_f(rs->plogs, "P3", rs->logs);
#endif
                    msync(rs->tm, sizeof(struct timespec) * 2, MS_SYNC);

#if TIME_MEASURE
                    clock_gettime(CLOCK_REALTIME, &tnow);
                    if (opsz == 0) {
                        //sprintf(rs->logs, "opsz:%d\n", opsz);
                        //print_f(rs->plogs, "P3", rs->logs);  
                        //usleep(1000);
                        continue;
                    }

                    tdiff = time_diff(rs->tm[1], rs->tm[0], 1000);
                    if (tdiff == -1) {
                         tdiff = 0 - time_diff(rs->tm[0], rs->tm[1], 1000);
                    }

                    //sprintf(rs->logs, "t %d us\n", tdiff);
                    //print_f(rs->plogs, "P3", rs->logs);
                    if (tdiff < 0) {                    
                        sprintf(rs->logs, "!!t %d - %d!!\n", tdiff, len);
                        print_f(rs->plogs, "P3", rs->logs);
                    }
#endif
                    //shmem_dump(addr, 32);
                    //msync(addr, len, MS_SYNC);
                    if (opsz < 0) break;
#else // #if SPI1_ENABLE
                    bitset = 1;
                    msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
                    //sprintf(rs->logs, "Get RDY pin: %d\n", bitset);
                    //print_f(rs->plogs, "P3", rs->logs);
                    if (bitset == 0) {
                        opsz = -1;
                        break;
                    }
#endif // #if SPI1_ENABLE
                    ring_buf_prod_dual(rs->pdataRx, pi);

                    rs_ipc_put(rs, "p", 1);
                    pi += 2;
                }

                opsz = 0 - opsz;

                if (opsz == 1) opsz = 0;
#if JPG_FFD9_CUT /* find 0xcffd9 in jpg */
                ret = cfgTableGetChk(pct, ASPOP_FILE_FORMAT, &fformat, ASPOP_STA_CON);    
                if (ret) {
                    fformat = 0;
                }
                
                if ((fformat == 0) || (fformat == FILE_FORMAT_JPG) || (fformat == FILE_FORMAT_PDF)) {
                    sprintf(rs->logs, "fformat: 0x%x !!!\n", fformat);
                    print_f(rs->plogs, "P3", rs->logs);    

                    /* search the offset of 0xffd9 */
                    ret = findEOF(addr, opsz);
                    if (ret > 0) {
                        memset(addr + ret + 2, 0xff, opsz - ret - 2);
                    }

                    sprintf(rs->logs, "fformat: 0x%x, ret: %d \n", fformat, ret);
                    print_f(rs->plogs, "P3", rs->logs);    
                }
#endif
                if (opsz > 0) {
                    ring_buf_prod_dual(rs->pdataRx, pi);
                }
 
                ring_buf_set_last_dual(rs->pdataRx, opsz, pi);
                rs_ipc_put(rs, "d", 1);

                sprintf(rs->logs, "spi1 recv end, the last sector size: %d\n", opsz);
                print_f(rs->plogs, "P3", rs->logs);
            }else  if (cmode == 6) {
                sprintf(rs->logs, "cmode: %d\n", cmode);
                print_f(rs->plogs, "P3", rs->logs);

                pi = 0;  

                while (1) {
                    len = 0;
                    len = ring_buf_get(rs->pcmdTx, &addr);
                    if (len > 0) {

                        opsz = 0;
                        while (opsz == 0) {
#if SPI_KTHREAD_USE
                    opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr);  //SPI_IOC_PROBE_THREAD
                    while (opsz == 0) {
#if SPI_KTHREAD_DLY
                        usleep(100000);
                        sprintf(rs->logs, "kth opsz:%d\n", opsz);
                        print_f(rs->plogs, "P3", rs->logs);  
#endif
                        opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr); //kthread 
                    }

#else // #if SPI_KTHREAD_USE
                            opsz = mtx_data(rs->spifd, addr, NULL, len, tr);
#endif

                            if ((opsz > 0) && (opsz < SPI_TRUNK_SZ)) { // workaround to fit original design
                                opsz = 0 - opsz;
                            }

#if SPI_TRUNK_FULL_FIX
                            if (opsz < 0) {
                                opsz = 0 - opsz;                    
                                if (opsz == SPI_TRUNK_SZ) {
                                    opsz = 0 - opsz;    
                                } else {
                                    sprintf(rs->logs, "Error!!! spi send tx failed, return %d \n", opsz);
                                    print_f(rs->plogs, "P2", rs->logs);
                                }
                            }
#endif

#if P3_TX_LOG
                            sprintf(rs->logs, "r %d - %d\n", opsz, pi);
                            print_f(rs->plogs, "P3", rs->logs);
#endif
                        }

                        //msync(addr, len, MS_SYNC);
                        ring_buf_prod(rs->pcmdTx);    
                        if (opsz < 0) break;
                        rs_ipc_put(rs, "p", 1);
                        pi += 1;
                    }
                }
                opsz = 0 - opsz;

#if JPG_FFD9_CUT /* find 0xcffd9 in jpg */
                ret = cfgTableGetChk(pct, ASPOP_FILE_FORMAT, &fformat, ASPOP_STA_CON);    
                if (ret) {
                    fformat = 0;
                }
                
                if ((fformat == 0) || (fformat == FILE_FORMAT_JPG) || (fformat == FILE_FORMAT_PDF)) {
                    sprintf(rs->logs, "fformat: 0x%x !!!\n", fformat);
                    print_f(rs->plogs, "P3", rs->logs);    

                    /* search the offset of 0xffd9 */
                    ret = findEOF(addr, opsz);
                    if (ret > 0) {
                        memset(addr + ret + 2, 0xff, opsz - ret - 2);
                    }

                    sprintf(rs->logs, "fformat: 0x%x, ret: %d \n", fformat, ret);
                    print_f(rs->plogs, "P3", rs->logs);    
                }
#endif

                ring_buf_set_last(rs->pcmdTx, opsz);
                rs_ipc_put(rs, "d", 1);
                sprintf(rs->logs, "spi0 recv end\n");
                print_f(rs->plogs, "P3", rs->logs);
            } else {
                sprintf(rs->logs, "cmode: %d - 7\n", cmode);
                print_f(rs->plogs, "P3", rs->logs);
            }
        }
    }

    p3_end(rs);
    return 0;
}

#define P4_TX_LOG  (1)
static int p4(struct procRes_s *rs)
{
    float flsize, fltime;
    int px, pi, ret=0, len=0, opsz, totsz, tdiff;
    int cmode, acuhk, errtor=0, cltport=0;
    char ch, str[128];
    char *addr, *pre, *cltaddr;
    struct info16Bit_s *p=0, *c=0;
    uint32_t secStr=0, secLen=0, datLen=0, minLen=0;
    struct sdFAT_s *pfat=0;
    struct sdFATable_s   *pftb=0;

    pfat = rs->psFat;
    pftb = pfat->fatTable;
    sprintf(rs->logs, "p4\n");
    print_f(rs->plogs, "P4", rs->logs);

    c = &rs->pmch->cur;
    p = &rs->pmch->tmp;

    p4_init(rs);
    // wait for ch from p0
    // in charge of socket send

    char *recvbuf, *tmp;

    recvbuf = aspMalloc(1024);
    if (!recvbuf) {
        sprintf(rs->logs, "p4 recvbuf alloc failed! \n");
        print_f(rs->plogs, "P4", rs->logs);
        return (-1);
    } else {
        sprintf(rs->logs, "p4 recvbuf alloc success! 0x%x\n", recvbuf);
        print_f(rs->plogs, "P4", rs->logs);
    }

    rs->psocket_t->listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (rs->psocket_t->listenfd < 0) { 
        sprintf(rs->logs, "p4 get socket ret: %d", rs->psocket_t->listenfd);
        error_handle(rs->logs, 3128);
    }

    memset(&rs->psocket_t->serv_addr, '0', sizeof(struct sockaddr_in));

    rs->psocket_t->serv_addr.sin_family = AF_INET;
    rs->psocket_t->serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    rs->psocket_t->serv_addr.sin_port = htons(6000); 

    ret = bind(rs->psocket_t->listenfd, (struct sockaddr*)&rs->psocket_t->serv_addr, sizeof(struct sockaddr_in)); 
    if (ret < 0) {
        //perror("bind:");
        
        ret = -1;
        if (setsockopt(rs->psocket_t->listenfd, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof(int)) == -1) {
            perror("setsockopt");    
            
            sprintf(rs->logs, "p4 get bind ret: %d", ret);
            error_handle(rs->logs, 22465);
        }
    }

    ret = listen(rs->psocket_t->listenfd, 10); 
    if (ret < 0) {
        sprintf(rs->logs, "p4 get listen ret: %d", ret);
        error_handle(rs->logs, 3146);
    }

    while (1) {
        //printf("^");
        //sprintf(rs->logs, "^\n");
        //print_f(rs->plogs, "P4", rs->logs);
#if 1 /* remote for test */
        len = sizeof(struct sockaddr_in);
        memset(&rs->psocket_t->clint_addr, 0, len);
        rs->psocket_t->connfd = accept(rs->psocket_t->listenfd, (struct sockaddr*)&rs->psocket_t->clint_addr, &len); \
        if (rs->psocket_t->connfd < 0) {
            sprintf(rs->logs, "P4 get connect failed ret:%d", rs->psocket_t->connfd);
            error_handle(rs->logs, 3157);
            continue;
        } else {
            //cltaddr = rs->psocket_t->clint_addr.sa_data;
            cltaddr = inet_ntoa(rs->psocket_t->clint_addr.sin_addr);
            cltport = ntohs(rs->psocket_t->clint_addr.sin_port);
            sprintf(rs->logs, "get connection id: %d [%s:%d]\n", rs->psocket_t->connfd, cltaddr, cltport);
            print_f(rs->plogs, "P4", rs->logs);
        }
#else
        rs->psocket_t->connfd = 4;
#endif
//        opsz = read(rs->psocket_t->connfd, recvbuf, 1024);
        opsz = 0;
        recvbuf[0] = '\0';
        //sprintf(rs->logs, "socket connected %d\n", rs->psocket_t->connfd);
        //print_f(rs->plogs, "P4", rs->logs);

        pi = 0;
        ret = 1;

        while (ret > 0) {

            ret = 0;
            ret = rs_ipc_get(rs, &ch, 1);               
            
            //sprintf(rs->logs, "%c ret:%d \n", ch, ret);
            //print_f(rs->plogs, "P4", rs->logs);

            switch (ch) {
                case 'E':
                    goto socketEnd;
                    break;
                case 'd':
                    cmode = 1;
                    break;
                case 'c':
                    cmode = 2;
                    break;
                case 'r':
                    cmode = 3;
                    break;
                case 'n':
                    cmode = 4;
                    break;
                case 'u':
                    cmode = 5;
                    break;
                default:
                    break;
            }

            if (cmode == 3) {
                if (rs->psocket_t->connfd > 0) {
                    rs_ipc_put(rs, "R", 1);
                    continue;
                } else {
                    rs_ipc_put(rs, "r", 1);
                    break;
                }
            } 
            else if (cmode == 1) {
#if MSP_P4_SAVE_DAT
                ret = file_save_get(&rs->fdat_s[0], "/mnt/mmc2/tx/p4%d.dat");
                if (ret) {
                    sprintf(rs->logs, "get tx log data file error - %d, hold here\n", ret);
                    print_f(rs->plogs, "P4", rs->logs);         
                    while(1);
                } else {
                    sprintf(rs->logs, "get tx log data file ok - %d, f: %d\n", ret, rs->fdat_s[0]);
                    print_f(rs->plogs, "P4", rs->logs);         
                }
#endif
                totsz = 0;
                clock_gettime(CLOCK_REALTIME, &rs->tdf[0]);
                sprintf(rs->logs, "start at %ds\n", rs->tdf[0].tv_sec);
                print_f(rs->plogs, "P4", rs->logs);                         
                
                while (1) {
                    len = ring_buf_cons_dual(rs->pdataRx, &addr, pi);
                    if (len >= 0) {
                        //printf("cons 0x%x %d %d \n", addr, len, pi);
                        pi++;
                    
                        msync(addr, len, MS_SYNC);
                        /* send data to wifi socket */
                        //sprintf(rs->logs, " %d -%d \n", len, pi);
                        //print_f(rs->plogs, "P4", rs->logs);         
                        if (len != 0) {
#if 1 /* debug */
                            opsz = write(rs->psocket_t->connfd, addr, len);
#else
                            opsz = len;
#endif
                            totsz += len;
                            //printf("socket tx %d %d\n", rs->psocket_r->connfd, opsz);
#if P4_TX_LOG
                            sprintf(rs->logs, "t %d -%d \n", opsz, pi);
                            print_f(rs->plogs, "P4", rs->logs);         
#endif
#if MSP_P4_SAVE_DAT
                            fwrite(addr, 1, len, rs->fdat_s[0]);
                            fflush(rs->fdat_s[0]);
#endif
                        } else {
                            sprintf(rs->logs, "len:%d \n", len);
                            print_f(rs->plogs, "P4", rs->logs);         
                        }
                    } else {
                        sprintf(rs->logs, "socket tx %d %d- wait\n", opsz, pi);
                        print_f(rs->plogs, "P4", rs->logs);         
                        //break;
                    }

                    if (ch != 'D') {
                        ch = 0;
                        rs_ipc_get(rs, &ch, 1);
                    } else {
                        if (len < 0) {
                           sprintf(rs->logs, "socket tx %d %d- END\n", opsz, pi);
                           print_f(rs->plogs, "P4", rs->logs);         
                            break;
                        }
                    }
                }
                
                clock_gettime(CLOCK_REALTIME, &rs->tdf[1]);
                sprintf(rs->logs, "end at %ds\n", rs->tdf[1].tv_sec);
                print_f(rs->plogs, "P4", rs->logs);                         
                
                while (ch != 'D') {
                    sprintf(rs->logs, "%c clr\n", ch);
                    print_f(rs->plogs, "P4", rs->logs);         
                    ch = 0;
                    rs_ipc_get(rs, &ch, 1);
                }

                tdiff = time_diff(&rs->tdf[0], &rs->tdf[1], 1000);

                flsize = totsz;
                fltime = tdiff;
                sprintf(rs->logs, "time:%d us, totsz:%d bytes, thoutghput: %f MBits\n", tdiff, totsz, (flsize*8)/fltime);
                print_f(rs->plogs, "P4", rs->logs);

                rs_ipc_put(rs, "D", 1);
                sprintf(rs->logs, "%c socket tx %d - end\n", ch, pi);
                print_f(rs->plogs, "P4", rs->logs);         
#if MSP_P4_SAVE_DAT
                fclose(rs->fdat_s[0]);
#endif
                break;
            }
            else if (cmode == 2) {
                secStr = c->opinfo;
                secLen = p->opinfo;
                datLen = secLen * 512;
                minLen = 16 * 512;

                if (datLen < minLen) {
                    datLen = minLen;
                }

                px = 0;

                len = 0;
                len = ring_buf_get(rs->pcmdTx, &addr);
                while (len <= 0) {
                    usleep(10000000);
                    len = ring_buf_get(rs->pcmdTx, &addr);
                }
                acuhk = 0;
                opsz = read(rs->psocket_t->connfd, addr, len);
                while (opsz <= 0) {
                    //sprintf(rs->logs, "[wait] socket receive %d/%d bytes from %d\n", px, opsz, len, rs->psocket_t->connfd);
                    //print_f(rs->plogs, "P4", rs->logs);
                    opsz = read(rs->psocket_t->connfd, addr, len);
                }

                addr += opsz;
                acuhk += opsz;
                while ((opsz < len) && (opsz > 0)) {
                    len = len - opsz;

                    errtor = 0;
                    opsz = read(rs->psocket_t->connfd, addr, len);
                    while (opsz <= 0) {
                        if (errtor > 3) break;
                        opsz = read(rs->psocket_t->connfd, addr, len);
                        errtor ++;
                    }

                    addr += opsz;
                    acuhk += opsz;
                }
                
                //sprintf(rs->logs, "[%d] socket receive %d/%d bytes from %d, n:%d\n", px, acuhk, len, rs->psocket_t->connfd, n);
                //print_f(rs->plogs, "P4", rs->logs);
                px++;
                ring_buf_prod(rs->pcmdTx);
                rs_ipc_put(rs, "c", 1);

                if (opsz >= 0) {
                while (1) {
                    len = 0;
                    len = ring_buf_get(rs->pcmdTx, &addr);
                    while (len <= 0) {
                        usleep(10000000);
                        len = ring_buf_get(rs->pcmdTx, &addr);
                    }

                    acuhk = 0;

                    errtor = 0;
                    opsz = read(rs->psocket_t->connfd, addr, len);
                    while (opsz <= 0) {
                        if (errtor > 3) break;
                        opsz = read(rs->psocket_t->connfd, addr, len);
                        errtor ++;
                    }

                    addr += opsz;
                    acuhk += opsz;
                    while ((opsz < len) && (opsz > 0)) {
                        len = len - opsz;

                        errtor = 0;
                        opsz = read(rs->psocket_t->connfd, addr, len);
                        while (opsz <= 0) {
                            if (errtor > 3) break;
                            opsz = read(rs->psocket_t->connfd, addr, len);
                            errtor ++;
                        }

                        addr += opsz;
                        acuhk += opsz;
                    }
                    
                    //sprintf(rs->logs, "[%d] socket receive %d/%d bytes from %d n:%d\n", px, acuhk, len, rs->psocket_t->connfd, opsz);
                    //print_f(rs->plogs, "P4", rs->logs);
                    px++;

                    ring_buf_prod(rs->pcmdTx);
                    if (opsz <= 0) break;
                    //shmem_dump(addr, 32);

                    rs_ipc_put(rs, "c", 1);
                }
                }
                ring_buf_set_last(rs->pcmdTx, acuhk);

                sprintf(rs->logs, "[%d] socket receive %d/%d bytes end\n", px, opsz, len);
                print_f(rs->plogs, "P4", rs->logs);

                rs_ipc_put(rs, "C", 1);
                if (!pftb->c) {
                    break;
                }
            }
            else if (cmode == 4) {
#if MSP_P4_SAVE_DAT
                ret = file_save_get(&rs->fdat_s[0], "/mnt/mmc2/tx/p4%d.dat");
                if (ret) {
                    sprintf(rs->logs, "get tx log data file error - %d, hold here\n", ret);
                    print_f(rs->plogs, "P4", rs->logs);         
                    while(1);
                } else {
                    sprintf(rs->logs, "get tx log data file ok - %d, f: %d\n", ret, rs->fdat_s[0]);
                    print_f(rs->plogs, "P4", rs->logs);         
                }
#endif
                clock_gettime(CLOCK_REALTIME, &rs->tdf[0]);
                totsz = 0;
                pi = 0;
                while (1) {
                    len = ring_buf_cons(rs->pcmdRx, &addr);
                    if (len >= 0) {
                        pi++;
                    
                        msync(addr, len, MS_SYNC);
                        /* send data to wifi socket */
#if P4_TX_LOG
                        sprintf(rs->logs, " %d -%d \n", len, pi);
                        print_f(rs->plogs, "P4", rs->logs);         
#endif
                        if (len != 0) {
                            #if 1 /*debug*/
                            opsz = write(rs->psocket_t->connfd, addr, len);
                            #else
                            opsz = len;
                            #endif
                            //sprintf(rs->logs, "tx %d -%d \n", opsz, pi);
                            //print_f(rs->plogs, "P4", rs->logs);      
#if MSP_P4_SAVE_DAT
                            fwrite(addr, 1, len, rs->fdat_s[0]);
                            fflush(rs->fdat_s[0]);
#endif
                            totsz += opsz;
                        } else {
                            sprintf(rs->logs, "len:%d \n", len);
                            print_f(rs->plogs, "P4", rs->logs);         
                        }
                        
                        rs_ipc_put(rs, "n", 1);
                    } else {
                        sprintf(rs->logs, "%c socket tx %d %d %d- end\n", ch, rs->psocket_t->connfd, opsz, pi);
                        print_f(rs->plogs, "P4", rs->logs);         
                        break;
                    }

                    if (ch != 'N') {
                        ch = 0;
                        rs_ipc_get(rs, &ch, 1);
                    }
                }

                clock_gettime(CLOCK_REALTIME, &rs->tdf[1]);

                tdiff = time_diff(&rs->tdf[0], &rs->tdf[1], 1000);

                flsize = totsz;
                fltime = tdiff;
                sprintf(rs->logs, "time:%d us, totsz:%d bytes, thoutghput: %f MBits\n", tdiff, totsz, (flsize*8)/fltime);
                print_f(rs->plogs, "P4", rs->logs);

                while (ch != 'N') {
                    sprintf(rs->logs, "%c clr\n", ch);
                    print_f(rs->plogs, "P4", rs->logs);         
                    ch = 0;
                    rs_ipc_get(rs, &ch, 1);
                }

                rs_ipc_put(rs, "N", 1);
                sprintf(rs->logs, "%c socket tx %d - end\n", ch, pi);
                print_f(rs->plogs, "P4", rs->logs);         
#if MSP_P4_SAVE_DAT
                fclose(rs->fdat_s[0]);
#endif
                if (!pftb->c) {
                    break;
                }
            }
            else if (cmode == 5) {
#if MSP_P4_SAVE_DAT
                ret = file_save_get(&rs->fdat_s[0], "/mnt/mmc2/tx/p4%d.dat");
                if (ret) {
                    sprintf(rs->logs, "get tx log data file error - %d, hold here\n", ret);
                    print_f(rs->plogs, "P4", rs->logs);         
                    while(1);
                } else {
                    sprintf(rs->logs, "get tx log data file ok - %d, f: %d\n", ret, rs->fdat_s[0]);
                    print_f(rs->plogs, "P4", rs->logs);         
                }
#endif
                secStr = c->opinfo;
                secLen = p->opinfo;
                datLen = secLen * 512;
                minLen = 16 * 512;

                sprintf(rs->logs, "ready for tx len: %d/%d\n", datLen, minLen);
                print_f(rs->plogs, "P4", rs->logs);         

                if (datLen < minLen) {
                    datLen = minLen;
                }

                px = 0;

                len = 0;
                len = ring_buf_get(rs->pcmdTx, &addr);
                while (len <= 0) {
                    usleep(1000000);
                    len = ring_buf_get(rs->pcmdTx, &addr);
                }
                
                if (len > datLen) {
                    len = datLen;
                    datLen = 0;
                } else {
                    datLen = datLen - len;
                }

                pre = addr;
                acuhk = 0;

                rs_ipc_put(rs, "h", 1);
                opsz = read(rs->psocket_t->connfd, addr, len);
                while (opsz <= 0) {
                    rs_ipc_put(rs, "h", 1);
                    sprintf(rs->logs, "[wait] %d\n", opsz);
                    print_f(rs->plogs, "P4", rs->logs);
                    opsz = read(rs->psocket_t->connfd, addr, len);
                }

                addr += opsz;
                acuhk += opsz;
                while ((opsz < len) && (opsz > 0)) {
                    len = len - opsz;

                    errtor = 0;
                    opsz = read(rs->psocket_t->connfd, addr, len);
                    while (opsz <= 0) {
                        if (errtor > 3) break;
                        opsz = read(rs->psocket_t->connfd, addr, len);
                        errtor ++;
                    }

                    addr += opsz;
                    acuhk += opsz;
                }

                sprintf(rs->logs, "socket rx %d\n", acuhk);
                print_f(rs->plogs, "P4", rs->logs);

                
                if (datLen == 0) {
#if MSP_P4_SAVE_DAT 
                    fwrite(pre, 1, acuhk, rs->fdat_s[0]);
                    fflush(rs->fdat_s[0]);
#endif
                    ring_buf_prod(rs->pcmdTx);
                    ring_buf_set_last(rs->pcmdTx, acuhk);
                } else {
                    px++;
                    ring_buf_prod(rs->pcmdTx);
#if MSP_P4_SAVE_DAT 
                    fwrite(pre, 1, acuhk, rs->fdat_s[0]);
                    fflush(rs->fdat_s[0]);
#endif
                }
                rs_ipc_put(rs, "u", 1);
                
                if (datLen > 0) {
                while (1) {
                    len = 0;
                    len = ring_buf_get(rs->pcmdTx, &addr);
                    while (len <= 0) {
                        rs_ipc_put(rs, "h", 1);
                        usleep(1000000);
                        len = ring_buf_get(rs->pcmdTx, &addr);
                    }

                    if (len > datLen) {
                        len = datLen;
                        datLen = 0;
                    } else {
                        datLen = datLen - len;
                    }

                    acuhk = 0;
                    pre = addr;

                    errtor = 0;
                    opsz = read(rs->psocket_t->connfd, addr, len);
                    while (opsz <= 0) {
                        rs_ipc_put(rs, "h", 1);
                        sprintf(rs->logs, "[wait] %d\n", opsz);
                        print_f(rs->plogs, "P4", rs->logs);

                        if (errtor > 3) break;
                        opsz = read(rs->psocket_t->connfd, addr, len);
                        errtor ++;
                    }

                    addr += opsz;
                    acuhk += opsz;
                    while ((opsz < len) && (opsz > 0)) {
                        len = len - opsz;

                        errtor = 0;
                        opsz = read(rs->psocket_t->connfd, addr, len);
                        while (opsz <= 0) {
                            rs_ipc_put(rs, "h", 1);
                            sprintf(rs->logs, "[wait] %d\n", opsz);
                            print_f(rs->plogs, "P4", rs->logs);

                            if (errtor > 3) break;
                            opsz = read(rs->psocket_t->connfd, addr, len);
                            errtor ++;
                        }

                        addr += opsz;
                        acuhk += opsz;
                    }
                    
                    sprintf(rs->logs, "socket rx %d\n", acuhk);
                    print_f(rs->plogs, "P4", rs->logs);
                    px++;
#if MSP_P4_SAVE_DAT 
                    fwrite(pre, 1, acuhk, rs->fdat_s[0]);
                    fflush(rs->fdat_s[0]);
#endif
                    
                    ring_buf_prod(rs->pcmdTx);

                    if (datLen == 0) break;
                    //if (opsz <= 0) break;
                    //shmem_dump(addr, 32);

                    rs_ipc_put(rs, "u", 1);
                }
                }

#if MSP_P4_SAVE_DAT
                fclose(rs->fdat_s[0]);
#endif
                ring_buf_set_last(rs->pcmdTx, acuhk);

                sprintf(rs->logs, "[%d] socket receive %d/%d bytes end\n", px, opsz, len);
                print_f(rs->plogs, "P4", rs->logs);

                rs_ipc_put(rs, "U", 1);
                if (!pftb->c) {
                    sprintf(rs->logs, "break connection!! \n");
                    print_f(rs->plogs, "P4", rs->logs);

                    break;
                }
            }
            else {
                sprintf(rs->logs, "cmode: %d - 7\n", cmode);
                print_f(rs->plogs, "P4", rs->logs);
                error_handle(rs->logs, 4702);
            }
        }

        socketEnd:
        close(rs->psocket_t->connfd);
        rs->psocket_t->connfd = 0;
    }

    p4_end(rs);
    return 0;
}

static int p5(struct procRes_s *rs, struct procRes_s *rcmd)
{
    int px, pi, size, opsz, acusz, len, acu;
    int ret, n, num, hd, be, ed, ln, fg, cltport=0;
    char ch, *recvbuf, *addr, *sendbuf, *cltaddr;
    char opcode=0, param=0, flag = 0;
    char msg[256] = "boot";
    sprintf(rs->logs, "p5\n");
    print_f(rs->plogs, "P5", rs->logs);

    p5_init(rs);
    // wait for ch from p0
    // in charge of socket recv

    addr = aspMalloc(1024);
    if (!addr) {
        sprintf(rs->logs, "p5 addr alloc failed! \n");
        print_f(rs->plogs, "P5", rs->logs);
        return (-1);
    }
    
    sendbuf = aspMalloc(OUT_BUFF_LEN);
    if (!sendbuf) {
        sprintf(rs->logs, "p5 sendbuf alloc failed! \n");
        print_f(rs->plogs, "P5", rs->logs);
        return (-1);
    }

    recvbuf = aspMalloc(2048);
    if (!recvbuf) {
        sprintf(rs->logs, "p5 recvbuf alloc failed! \n");
        print_f(rs->plogs, "P5", rs->logs);
        return (-1);
    } else {
        sprintf(rs->logs, "p5 recvbuf alloc success! 0x%x\n", recvbuf);
        print_f(rs->plogs, "P5", rs->logs);
    }

    rs->psocket_r->listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (rs->psocket_r->listenfd < 0) { 
        sprintf(rs->logs, "p5 get socket ret: %d", rs->psocket_r->listenfd);
        error_handle(rs->logs, 3302);
    }

    memset(&rs->psocket_r->serv_addr, '0', sizeof(struct sockaddr_in));

    rs->psocket_r->serv_addr.sin_family = AF_INET;
    rs->psocket_r->serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    rs->psocket_r->serv_addr.sin_port = htons(5000); 

    ret = bind(rs->psocket_r->listenfd, (struct sockaddr*)&rs->psocket_r->serv_addr, sizeof(struct sockaddr_in)); 
    if (ret < 0) {
        //perror("bind:");
        
        ret = -1;
        if (setsockopt(rs->psocket_r->listenfd, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof(int)) == -1) {
            perror("setsockopt");    
            
            sprintf(rs->logs, "p5 get bind ret: %d", ret);
            error_handle(rs->logs, 23071);
        }
    }

    ret = listen(rs->psocket_r->listenfd, 10); 
    if (ret < 0) {
        sprintf(rs->logs, "p5 get listen ret: %d", ret);
        error_handle(rs->logs, 3320);
    }
    
#if 0 /* disable auto boot for testing */
    sprintf(rs->logs, "send the very first command [%s] \n", msg);
    print_f(rs->plogs, "P5", rs->logs);
    rs_ipc_put(rcmd, msg, 4);

    n = rs_ipc_get(rcmd, sendbuf, 2048);
    sprintf(rs->logs, "get boot result: [%s], ret: %d\n", sendbuf, n);
    print_f(rs->plogs, "P5", rs->logs);
#endif

    while (1) {
        //printf("#");
        //sprintf(rs->logs, "#\n");
        //print_f(rs->plogs, "P5", rs->logs);
        ret = -1;
        
        len = sizeof(struct sockaddr_in);
        memset(&rs->psocket_r->clint_addr, 0, len);
        rs->psocket_r->connfd = accept(rs->psocket_r->listenfd, (struct sockaddr*)&rs->psocket_r->clint_addr, &len);

        if (rs->psocket_r->connfd < 0) {
            sprintf(rs->logs, "P5 get connect failed ret:%d", rs->psocket_r->connfd);
            error_handle(rs->logs, 3331);
            continue;
        }
        else {
            //cltaddr = rs->psocket_r->clint_addr.sa_data;
            cltaddr = inet_ntoa(rs->psocket_r->clint_addr.sin_addr);
            cltport = ntohs(rs->psocket_r->clint_addr.sin_port);
            sprintf(rs->logs, "get connection id: %d [%s:%d]\n", rs->psocket_r->connfd, cltaddr, cltport);
            print_f(rs->plogs, "P5", rs->logs);
        }

        memset(recvbuf, 0x0, 1024);
        memset(sendbuf, 0x0, OUT_BUFF_LEN);
        memset(msg, 0x0, 256);

        n = read(rs->psocket_r->connfd, recvbuf, 1024);
        
        recvbuf[n-1] = '\0';
        //sprintf(rs->logs, "receive len[%d]content[%s]\n", n, recvbuf);
        //print_f(rs->plogs, "P5", rs->logs);
        memset(sendbuf, 0, OUT_BUFF_LEN);
        sprintf(sendbuf, "Leo heard [%s]\n", recvbuf);
        //strcpy(sendbuf, recvbuf);
        n = strlen(sendbuf);
        
        if (n <= 0) goto socketEnd;
        hd = atFindIdx(recvbuf, 0xfe);
        if (hd < 0) goto socketEnd;
        fg = atFindIdx(recvbuf, 0xfd);
        if (fg < 0) goto socketEnd;     
        be = atFindIdx(&recvbuf[hd], 0xfc);
        if (be < 0) goto socketEnd;
        ed = atFindIdx(&recvbuf[hd], 0xfb);
        if (ed < 0) goto socketEnd;
        ln = atFindIdx(&recvbuf[hd], '\0');
        //if (ln < 0) goto socketEnd;
        
        num = strlen(&recvbuf[hd]);
        if (num <= 0) {
            goto socketEnd;
        }

        //sprintf(rs->logs, "receive len[%d]content[%s]hd[%d]be[%d]ed[%d]ln[%d]fg[%d]\n", n, &recvbuf[hd], hd, be, ed, ln, fg);
        //print_f(rs->plogs, "P5", rs->logs);

        opcode = recvbuf[hd+1]; param = recvbuf[be-1]; flag = recvbuf[fg+1];
        sprintf(rs->logs, "opcode:[0x%x]arg[0x%x]flg[0x%x]\n", opcode, param, flag);
        print_f(rs->plogs, "P5", rs->logs);

        /* android socket api can't send data with '\0' */
        if ((fg+1) != (be-1)) {
            if (param == 0xff) {
                if (flag & 0x2) {
                    param = 0;
                }
            }
        }

        num = ed - be - 1;
        if ((num < 255) && (num > 0)) {
            memcpy(msg, &recvbuf[be+1], num);
            msg[num] = '\0';

            sprintf(rs->logs, "get msg from app [%s] size:%d\n", msg, num);
            print_f(rs->plogs, "P5", rs->logs);
        } else {
            goto socketEnd;
        }

#if SCANGO_CHECK 
        rs_ipc_put(rcmd, "scango", 6);
        ch = 0; ret = 0;
        ret = rs_ipc_get(rcmd, &ch, 1024);
        if ((ret == 1) && (ch == 'Z')) {
            ret = rs_ipc_get(rcmd, addr, 1024);
            addr[ret] = '\0';
            sprintf(rs->logs, "get response [%s] size:%d\n", addr, ret);
            print_f(rs->plogs, "P5", rs->logs);
        } else {
            opcode = OP_ERROR; 
        }
#endif
        
        if (opcode == OP_ERROR) {
            num = -1;
        }
        else if (opcode != OP_MSG) {
            num = 0;
        }

        //sprintf(rs->logs, "num [%d] ch [%c] \n",num, ch);
        //print_f(rs->plogs, "P5", rs->logs);

        if (num < 0) {
            param = ch;
        } else if (num > 0) {
            //rs_ipc_put(rs, "s", 1);
            //rs_ipc_put(rs, msg, n);
            //sprintf(rs->logs, "send to p0 [%s]\n", recvbuf);
            //print_f(rs->plogs, "P5", rs->logs);
            
            //ret = write(rs->psocket_r->connfd, msg, n);
            //sprintf(rs->logs, "get msg from app [%s] size:%d\n", msg, num);
            //print_f(rs->plogs, "P5", rs->logs);
            rs_ipc_put(rcmd, msg, num);
        } else {
            if ((opcode == OP_SINGLE) || (opcode == OP_DOUBLE)) {
                msg[0] = 't';
                msg[1] = 'g';
                msg[2] = 'r';
                rs_ipc_put(rcmd, msg, 3);
            } else {
                msg[0] = 'o';
                msg[1] = 'p';
                rs_ipc_put(rcmd, msg, 2);
            }

            ch = 0; num = 0;
            num = rs_ipc_get(rcmd, &ch, 1);

            if (ch == 'o') {
                if (flag & 0x04) { /* 0xaa = write, 0xad = read */
                    msg[0] = 0xad; 
                } else {
                    msg[0] = 0xaa; 
                }
                msg[1] = opcode;
                msg[2] = '/';
                msg[3] = param;
                msg[4] = 0xa5;
                rs_ipc_put(rcmd, msg, 5);
            }

            ch = 0; num = 0;
            num = rs_ipc_get(rcmd, &ch, 1);

            if (ch != 'p') {
                opcode = OP_ERROR; 
                num = rs_ipc_get(rcmd, &ch, 1);
                param = ch;
            } else {
                num = rs_ipc_get(rcmd, &ch, 1);
                param = ch;
            }
        }

        //usleep(100000);
        memset(sendbuf, 0, OUT_BUFF_LEN);

        sendbuf[0] = 0xfe;
        sendbuf[1] = ((opcode & 0x80) ? 1:0) + 1;
        sendbuf[2] = opcode & 0x7f;
        sendbuf[3] = 0xfd;
        //sendbuf[3] = 'P';//0x0;
        sendbuf[6] = 0xfc;

        n = rs_ipc_get(rcmd, &sendbuf[7], OUT_BUFF_LEN);
        sendbuf[4] = ((param & 0x80) ? 1:0) + 1;
        sendbuf[5] = param & 0x7f;
        
        sendbuf[7+n] = '\n';
        sendbuf[7+n+1] = 0xfb;
        sendbuf[7+n+2] = '\0';

        //sprintf(rs->logs, "socket send, len:%d content[%s] from %d, ret:%d, opcode:%d, [%x][%x][%x][%x]\n", 7+n+3, &sendbuf[7], rs->psocket_r->connfd, ret, opcode, sendbuf[1], sendbuf[2], sendbuf[4], sendbuf[5]);
        //print_f(rs->plogs, "P5", rs->logs);
        //printf("[p5]:%s\n", &sendbuf[7]);

        socketEnd:
        ret = write(rs->psocket_r->connfd, sendbuf, 7+n+3);

        sendbuf[7+n] = '\0';
        printf("[P5] END send len[%d]content[\n\n%s\n\n]len[%d]\n", n, &sendbuf[7], n);
        //sprintf(rs->logs, "END send len[%d]hd[%d]be[%d]ed[%d]ln[%d]fg[%d]\n", n, hd, be, ed, ln, fg);
        //print_f(rs->plogs, "P5", rs->logs);

        close(rs->psocket_r->connfd);
        rs->psocket_r->connfd = 0;
    }

    p5_end(rs);
    return 0;
}

static int atFindIdx(char *str, char ch)
{
    int i, len;
    if (!str) return (-1);
    len = strlen(str);
    if (len > 1024) return (-2);

    i = 0;
    while (i < len) {
        if (*str == ch) {
            return i;
        }
        str++;
        i++;
    }
    return (-3);
}

#define P6_RX_LOG    (1)
#define P6_UTC_LOG  (0)
#define P6_PARA_LOG  (0)
#define P6_SEND_BUFF_SIZE (4096)
static int p6(struct procRes_s *rs)
{
    char ssidPath[128] = "/root/scaner/ssid.bin";
    char pskPath[128] = "/root/scaner/psk.bin";
    FILE *fssid=0, *fpsk=0;
    char strFullPath[1024];
    char strPath[32][128];
    //char **strPath; 
    char *recvbuf, *sendbuf, *pr, *cltaddr;
    int ret, n, num, hd, be, ed, ln, cnt=0, i, len=0, cltport=0;
    char opc=0;
    char opcode=0, param=0, flag = 0;
    uint32_t clstSize=0;
    uint32_t adata[3], atime[3];
    uint32_t *scanParam=0, op=0, cd=0, fg=0;
    int ix=0, idx=0;
    char curTime[16];
    char syscmd[256] = "ls -al";

    struct directnFile_s *fscur = 0, *nxtf = 0;
    struct directnFile_s *brt, *pa;
    struct directnFile_s *dnld=0, *upld=0;

    struct aspConfig_s *pct=0, *pdt=0;
    struct adFATLinkList_s *pflsh=0, *pflnt=0;
    struct sdFAT_s *pfat=0;
    struct sdFATable_s   *pftb=0;
    uint32_t secStr=0, secLen=0;

    struct aspMetaMass *pmass=0;
    int masUsed=0, masRecd=0, masStart=0;
    int gap=0, cy=0, cxm=0, cxn=0;
    unsigned short *ptBuf;

    struct aspInfoSplit_s *strinfo=0, *nexinfo=0;

    struct apWifiConfig_s *pwfc=0;
    char ch=0;

    pct = rs->pcfgTable;
    pfat = rs->psFat;
    pftb = pfat->fatTable;
    pwfc = rs->pwifconf;
    pmass = rs->pmetaMass;

    char dir[64] = "/mnt/mmc2";
    //char dir[256] = "/root";
    char folder[1024];

    sprintf(rs->logs, "p6\n");
    print_f(rs->plogs, "P6", rs->logs);

    p6_init(rs);
/*
    while (!rs->psFat->fatRootdir);    
    fscur = rs->psFat->fatRootdir;
*/

    recvbuf = aspMalloc(1024);
    if (!recvbuf) {
        sprintf(rs->logs, "recvbuf alloc failed! \n");
        print_f(rs->plogs, "P6", rs->logs);
        return (-1);
    } else {
        sprintf(rs->logs, "recvbuf alloc success! 0x%x\n", recvbuf);
        print_f(rs->plogs, "P6", rs->logs);
    }

    sendbuf = aspMalloc(P6_SEND_BUFF_SIZE);
    if (!sendbuf) {
        sprintf(rs->logs, "sendbuf alloc failed! \n");
        print_f(rs->plogs, "P6", rs->logs);
        return (-1);
    } else {
        sprintf(rs->logs, "sendbuf alloc success! 0x%x\n", sendbuf);
        print_f(rs->plogs, "P6", rs->logs);
    }

    rs->psocket_at->listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (rs->psocket_at->listenfd < 0) { 
        sprintf(rs->logs, "p6 get socket ret: %d", rs->psocket_at->listenfd);
        error_handle(rs->logs, 3302);
    }

    memset(&rs->psocket_at->serv_addr, '0', sizeof(struct sockaddr_in));

    rs->psocket_at->serv_addr.sin_family = AF_INET;
    rs->psocket_at->serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    rs->psocket_at->serv_addr.sin_port = htons(4000); 

    ret = bind(rs->psocket_at->listenfd, (struct sockaddr*)&rs->psocket_at->serv_addr, sizeof(struct sockaddr_in)); 
    if (ret < 0) {
        //perror("bind:");
        ret = -1;
        if (setsockopt(rs->psocket_at->listenfd, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof(int)) == -1) {
            perror("setsockopt");    
            
            sprintf(rs->logs, "p6 get bind ret: %d", ret);
            error_handle(rs->logs, 23360);
        }
    }

    ret = listen(rs->psocket_at->listenfd, 10); 
    if (ret < 0) {
        sprintf(rs->logs, "p6 get listen ret: %d", ret);
        error_handle(rs->logs, 3801);
    }

    while (1) {
        //sprintf(rs->logs, "@\n");
        //print_f(rs->plogs, "P6", rs->logs);
        cnt = 0;

        len = sizeof(struct sockaddr_in);
        memset(&rs->psocket_at->clint_addr, 0, len);
        rs->psocket_at->connfd = accept(rs->psocket_at->listenfd, (struct sockaddr*)&rs->psocket_at->clint_addr, &len); 
        if (rs->psocket_at->connfd < 0) {
            sprintf(rs->logs, "P6 get connect failed ret:%d", rs->psocket_at->connfd);
            error_handle(rs->logs, 3812);
            goto socketEnd;
        }
        else {
            //cltaddr = rs->psocket_at->clint_addr.sa_data;
            cltaddr = inet_ntoa(rs->psocket_at->clint_addr.sin_addr);
            cltport = ntohs(rs->psocket_at->clint_addr.sin_port);
            sprintf(rs->logs, "get connection id: %d [%s:%d]\n", rs->psocket_at->connfd, cltaddr, cltport);
            print_f(rs->plogs, "P6", rs->logs);
        }

        memset(recvbuf, 0x0, 1024);
        memset(sendbuf, 0x0, P6_SEND_BUFF_SIZE);
        
        n = read(rs->psocket_at->connfd, recvbuf, 1024);
        if (n <= 0) goto socketEnd;
        hd = atFindIdx(recvbuf, 0xfe);
        if (hd < 0) goto socketEnd;
        be = atFindIdx(&recvbuf[hd], 0xfc);
        if (be < 0) goto socketEnd;
        ed = atFindIdx(&recvbuf[hd], 0xfb);
        if (ed < 0) goto socketEnd;
        ln = atFindIdx(&recvbuf[hd], '\0');
        
        opcode = recvbuf[hd+1]; param = recvbuf[be-1];
#if P6_RX_LOG
        sprintf(rs->logs, "opcode:[0x%x]arg[0x%x]\n", opcode, param);
        print_f(rs->plogs, "P6", rs->logs);
#endif
        n = strlen(&recvbuf[hd]);
        if (n <= 0) {
            goto socketEnd;
        }

#if P6_RX_LOG
        sprintf(rs->logs, "[P6] receive len[%d]contentStr[%s]hd[%d]be[%d]ed[%d]ln[%d]\n", n, &recvbuf[be], hd, be, ed, ln);
        print_f(rs->plogs, "P6", rs->logs);

        //sprintf(rs->logs, "opcode:[0x%x]arg[0x%x]\n", recvbuf[hd+1], recvbuf[be-1]);
        //print_f(rs->plogs, "P6", rs->logs);
#endif
        n = ed - be - 1;
        sprintf(rs->logs, "n = %d \n", n);
        print_f(rs->plogs, "P6", rs->logs);
        if ((n < 1024) && (n > 0)) {
            memcpy(folder, &recvbuf[be+1], n);
            folder[n] = '\0';
        } else {
            goto socketEnd;
        }

        sprintf(rs->logs, "get folder:[%s]\n", folder);
        print_f(rs->plogs, "P6", rs->logs);

        sendbuf[0] = 0xfe;
        sendbuf[1] = opcode;
        sendbuf[2] = 0xfd;
        sendbuf[3] = 0x01; /*??*/
        sendbuf[4] = 0xfc;
        
        sendbuf[3] = 'F';
        if (opcode == 0x19) { /* send CROP info (new)*/
            #define CROP_MAX_NUM_META (18)
            sprintf(rs->logs, "handle opcode: 0x%x(CROP new)\n", opcode);
            print_f(rs->plogs, "P6", rs->logs);

            cnt = 0;
            while (1) {
                num = 0;
                for (i = 0; i < (CROP_MAX_NUM_META+1); i++) {
                    idx = ASPOP_CROP_01 + i;
                    
                    switch(idx) {
                        case ASPOP_CROP_01:
                        case ASPOP_CROP_02:
                        case ASPOP_CROP_03:
                        case ASPOP_CROP_04:
                        case ASPOP_CROP_05:
                        case ASPOP_CROP_06:
                        case ASPOP_CROP_07:
                        case ASPOP_CROP_08:
                        case ASPOP_CROP_09:
                        case ASPOP_CROP_10:
                        case ASPOP_CROP_11:
                        case ASPOP_CROP_12:
                        case ASPOP_CROP_13:
                        case ASPOP_CROP_14:
                        case ASPOP_CROP_15:
                        case ASPOP_CROP_16:
                        case ASPOP_CROP_17:
                        case ASPOP_CROP_18:
                            pdt = &pct[idx];
                            if (pdt->opStatus == ASPOP_STA_UPD) {
                                num++;
                            }

                            break;
                        default:
                            break;
                    }
                    
                }

                if (num == CROP_MAX_NUM_META) {
                    break;
                }

                sprintf(rs->logs, "wait crop num:%d, %d s\n", num, cnt/2);
                print_f(rs->plogs, "P6", rs->logs);
/*
                if (cnt > 10) {
                    break;
                }
*/
                usleep(500000);
                cnt ++;
            }

            
#if 1  /* skip for debug, anable later */
            if (pmass->massRecd) {
            
                ch = 0; ret = 0;
                
                while (ch != 'c') {
                    ret = rs_ipc_get(rs, &ch, 1);
                    if (ret > 0) {
                        if (ch == 'c') {
                            sprintf(rs->logs, "succeed to get ch == %c\n", ch);
                            print_f(rs->plogs, "P6", rs->logs);    
                        } else {
                            sprintf(rs->logs, "wrong!! ch == %c \n", ch);
                            print_f(rs->plogs, "P6", rs->logs);    
                        }
                    } else {
                        sprintf(rs->logs, "failed to get ch ret: \n", ret);
                        print_f(rs->plogs, "P6", rs->logs);    
                    }
                }

                cnt = 0;

                masUsed = pmass->massUsed;
                masStart = pmass->massStart;
                
                sprintf(rs->logs, "wait meta mass (used:%d, start:%d) %d\n", masUsed, masStart, cnt); 
                print_f(rs->plogs, "P6", rs->logs);

                while (!masUsed) {
                    usleep(500000);
                    msync(pmass, sizeof(struct aspMetaMass), MS_SYNC);
                    masUsed = pmass->massUsed;
                    sprintf(rs->logs, "wait meta mass (used:%d) %d\n", masUsed, cnt); 
                    print_f(rs->plogs, "P6", rs->logs);
                }

                masRecd = pmass->massRecd;

                msync(pmass->masspt, masUsed, MS_SYNC);
                ptBuf = (unsigned short *)pmass->masspt;
                
                cy = masStart;
                gap = pmass->massGap;

                sendbuf[3] = 'T';
                sprintf(rs->logs, "%d \n\r", masRecd); 
                print_f(rs->plogs, "P6", rs->logs);

                n = strlen(rs->logs);
                memcpy(&sendbuf[5], rs->logs, n);

                sendbuf[5+n] = 0xfb;
                sendbuf[5+n+1] = '\n';
                sendbuf[5+n+2] = '\0';

                ret = write(rs->psocket_at->connfd, sendbuf, 5+n+3);
                sprintf(rs->logs, "socket send, len:%d content[%s] from %d, ret:%d\n", 5+n+3, sendbuf, rs->psocket_at->connfd, ret);

                for (i = 0; i < masRecd; i++){
                    cy += gap;
                    cxm = *ptBuf;
                    ptBuf++;
                    cxn = *ptBuf;
                    ptBuf++;
                    sprintf(rs->logs, "%d,%d,%d,\n\r", cy, cxm, cxn); 
                    print_f(rs->plogs, "P6", rs->logs);

                    sendbuf[3] = 'M';
                    n = strlen(rs->logs);
                    memcpy(&sendbuf[5], rs->logs, n);

                    sendbuf[5+n] = 0xfb;
                    sendbuf[5+n+1] = '\n';
                    sendbuf[5+n+2] = '\0';

                    ret = write(rs->psocket_at->connfd, sendbuf, 5+n+3);
                    //sprintf(rs->logs, "socket send, len:%d from %d, ret:%d\n", 5+n+3, rs->psocket_at->connfd, ret);
                    //print_f(rs->plogs, "P6", rs->logs);
                }

                //shmem_dump(pmass->masspt, pmass->massUsed);

#if 0                    
                masStart = pmass->massStart;
                while (masStart != 0) {
                    masStart = pmass->massStart;
                    usleep(100000);
                }
#endif                
                pmass->massRecd = 0;
                pmass->massUsed = 0;

            } 
#endif
            for (i = 0; i < (CROP_MAX_NUM_META+1); i++) {
                idx = ASPOP_CROP_01 + i;
                pdt = &pct[idx];
                switch(idx) {
                    case ASPOP_CROP_01:
                    case ASPOP_CROP_02:
                    case ASPOP_CROP_03:
                    case ASPOP_CROP_04:
                    case ASPOP_CROP_05:
                    case ASPOP_CROP_06:
                    case ASPOP_CROP_07:
                    case ASPOP_CROP_08:
                    case ASPOP_CROP_09:
                    case ASPOP_CROP_10:
                    case ASPOP_CROP_11:
                    case ASPOP_CROP_12:
                    case ASPOP_CROP_13:
                    case ASPOP_CROP_14:
                    case ASPOP_CROP_15:
                    case ASPOP_CROP_16:
                    case ASPOP_CROP_17:
                    case ASPOP_CROP_18:
                        pdt = &pct[idx];
                        if (pdt->opStatus == ASPOP_STA_UPD) {
                            sprintf(rs->logs, "CROP%.2d. [0x%.8x]     {%d,  %d}  \n", i, pdt->opValue, pdt->opValue >> 16, pdt->opValue & 0xffff); 
                            print_f(rs->plogs, "P6", rs->logs);  
                            sendbuf[3] = 'C';

                            sprintf(rs->logs, "%d,%d,", pdt->opValue >> 16, pdt->opValue & 0xffff);
                            n = strlen(rs->logs);

                            pdt->opStatus = ASPOP_STA_APP;
                        }

                        break;
                    case ASPOP_IMG_LEN:
                        pdt = &pct[idx];
                        
                        if (pdt->opStatus == ASPOP_STA_UPD) {
                            sendbuf[3] = 'L';
                            sprintf(rs->logs, "%d,\n\r", pdt->opValue & 0xffff);
                            n = strlen(rs->logs);
                        }
                        break;
                    default:
                        break;
                }
                
                if (n > (P6_SEND_BUFF_SIZE - 16)) n = (P6_SEND_BUFF_SIZE - 16);
                
                memcpy(&sendbuf[5], rs->logs, n);

                sendbuf[5+n] = 0xfb;
                sendbuf[5+n+1] = '\n';
                sendbuf[5+n+2] = '\0';
                ret = write(rs->psocket_at->connfd, sendbuf, 5+n+3);
                
                //sendbuf[5+n] = '\0';                
                //sprintf(rs->logs, "socket send CROP%.2d [ %s \n], len:%d \n", i, &sendbuf[5], 5+n+3);
                //print_f(rs->plogs, "P6", rs->logs);
            }

            rs_ipc_put(rs, "C", 1);
            
            goto socketEnd;
        }
        
        if (opcode == 0x18) { /* update parameter */
            sprintf(rs->logs, "handle opcode: 0x%x(PARA)\n", opcode);
            print_f(rs->plogs, "P6", rs->logs);

            ret = asp_strsplit(&strinfo, folder, n);
            n = 0;
            if (ret > 0) {
                sprintf(rs->logs, "split str found, ret:%d\n", ret);
                print_f(rs->plogs, "P6", rs->logs);
                len = ret;
                memset(recvbuf, 0, 1024);
                
                scanParam = aspMalloc(len * sizeof(uint32_t));
                memset(scanParam, 0, len * sizeof(uint32_t));
                
                if (scanParam) {
                    for (i = 0; i < len; i++) {
                        nexinfo = asp_getInfo(strinfo, i);
                        if (nexinfo) {
#if P6_PARA_LOG
                            sprintf(rs->logs, "%d.[%s]\n", i, nexinfo->infoStr);
                            print_f(rs->plogs, "P6", rs->logs);
#endif
                            scanParam[i] = atoi(nexinfo->infoStr);
                            
                        } else {
                            sprintf(rs->logs, "split info error!!!\n");
                            print_f(rs->plogs, "P6", rs->logs);
                        }
                    }                            


                    for (i = 0; i < (len/3); i++) {
                        for (ix = 0; ix < ASPOP_CODE_MAX; ix++) {
                            pdt = &pct[ix];
                            op = scanParam[i*3+1];
                            cd = scanParam[i*3+2] & 0xff;
                            fg = scanParam[i*3+2] >> 8;

                            if (op == pdt->opCode) {
                                if ((cd == 0xff) && ((fg & 0x02) != 0)) {
                                    cd = 0;
                                }

                                ret = cmdfunc_opchk_single(cd, pdt->opMask, pdt->opBitlen, pdt->opType);
                                if (ret > 0) {
                                    pdt->opValue = cd;
                                    pdt->opStatus |= ASPOP_STA_CON;

#if P6_PARA_LOG
                                    sprintf(rs->logs, "get 0x%.2x = 0x%.2x (%d)\n", op, cd, fg);
                                    print_f(rs->plogs, "P6", rs->logs);
#endif
                                    
                                    sprintf(rs->logs, "%d,", scanParam[i*3]);
                                    strncpy(&recvbuf[n], rs->logs, strlen(rs->logs));
                                    n += strlen(rs->logs);
                                }
                            }
                        }
                    }

                    nexinfo = asp_getInfo(strinfo, (len - 1));
                    if (nexinfo) {
                        sprintf(rs->logs, "%d.%s\n", (len - 1), nexinfo->infoStr);
                        print_f(rs->plogs, "P6", rs->logs);

                        if (strcmp("ASP", nexinfo->infoStr) != 0) {
                            sendbuf[3] = 'F';
                        } else {
                            sendbuf[3] = 'D';
                            strncpy(&sendbuf[5], recvbuf, n);
                        }
                    }
                }                   
            }
            else {
                sprintf(rs->logs, "split str not found, ret:%d\n", ret);
                print_f(rs->plogs, "P6", rs->logs);
            }

            sendbuf[5+n] = 0xfb;
            sendbuf[5+n+1] = '\n';
            sendbuf[5+n+2] = '\0';
            ret = write(rs->psocket_at->connfd, sendbuf, 5+n+3);
            sprintf(rs->logs, "socket send, len:%d content[%s] from %d, ret:%d\n", 5+n+3, sendbuf, rs->psocket_at->connfd, ret);
            print_f(rs->plogs, "P6", rs->logs);

            /* free memory */
            nexinfo = strinfo;
            while(nexinfo) {
                nexinfo = asp_freeInfo(nexinfo);
            }
            
            if (scanParam) {
                aspFree(scanParam);
                scanParam = 0;
            }
            
            goto socketEnd;
        }
        
        if (opcode == 0x17) { /* send WIFI info */
            sprintf(rs->logs, "handle opcode: 0x%x(WIFI_PSK)\n", opcode);
            print_f(rs->plogs, "P6", rs->logs);
            n = 0;

            len = strlen(folder);
            sprintf(rs->logs, "get psk[%s] size: %d\n", folder, len);
            print_f(rs->plogs, "P6", rs->logs);

            fpsk = fopen(pskPath, "w+");
            if (fpsk) {
                fwrite(folder, 1, len, fpsk);
                fflush(fpsk);
                fclose(fpsk);

                if ((len >=8) && (len <= 63)) {
                    memset(pwfc->wfpsk, 0, 64);
                    memcpy(pwfc->wfpsk, folder, len);
                    pwfc->wfpskLen = len;
                    msync(pwfc->wfpsk, len, MS_SYNC);
                }
                
                sprintf(rs->logs, "PSK_OK");
            } else {
                sprintf(rs->logs, "PSK_NG");
            }

            n = strlen(rs->logs);
            strncpy(&sendbuf[5], rs->logs, n);
            
            sendbuf[5+n] = 0xfb;
            sendbuf[5+n+1] = '\n';
            sendbuf[5+n+2] = '\0';
            ret = write(rs->psocket_at->connfd, sendbuf, 5+n+3);
            sprintf(rs->logs, "socket send, len:%d content[%s] from %d, ret:%d\n", 5+n+3, sendbuf, rs->psocket_at->connfd, ret);
            print_f(rs->plogs, "P6", rs->logs);
            goto socketEnd;
        }
        
        if (opcode == 0x16) { /* send WIFI info */
            sprintf(rs->logs, "handle opcode: 0x%x(WIFI_SSID)\n", opcode);
            print_f(rs->plogs, "P6", rs->logs);
            n = 0;
            len = strlen(folder);
            sprintf(rs->logs, "get ssid[%s] size: %d\n", folder, len);
            print_f(rs->plogs, "P6", rs->logs);

            fssid = fopen(ssidPath, "w+");
            if (fssid) {
                fwrite(folder, 1, len, fssid);
                fflush(fssid);
                fclose(fssid);

                if (len <= 32) {
                    memset(pwfc->wfssid, 0, 36);
                    memcpy(pwfc->wfssid, folder, len);
                    pwfc->wfsidLen = len;
                    msync(pwfc->wfssid, len, MS_SYNC);
                }

                sprintf(rs->logs, "SSID_OK");
            } else {
                sprintf(rs->logs, "SSID_NG");
            }

            n = strlen(rs->logs);
            strncpy(&sendbuf[5], rs->logs, n);
            
            sendbuf[5+n] = 0xfb;
            sendbuf[5+n+1] = '\n';
            sendbuf[5+n+2] = '\0';
            ret = write(rs->psocket_at->connfd, sendbuf, 5+n+3);
            sprintf(rs->logs, "socket send, len:%d content[%s] from %d, ret:%d\n", 5+n+3, sendbuf, rs->psocket_at->connfd, ret);
            print_f(rs->plogs, "P6", rs->logs);

            goto socketEnd;
        }
        
        if (opcode == 0x15) { /* send CROP info (old)*/
            #define CROP_MAX_NUM (6)
            sprintf(rs->logs, "handle opcode: 0x%x(CROP)\n", opcode);
            print_f(rs->plogs, "P6", rs->logs);

            cnt = 0;
            while (1) {
                num = 0;
                for (i = 0; i < (CROP_MAX_NUM+1); i++) {
                    idx = ASPOP_CROP_01 + i;
                    
                    switch(idx) {
                        case ASPOP_CROP_01:
                        case ASPOP_CROP_02:
                        case ASPOP_CROP_03:
                        case ASPOP_CROP_04:
                        case ASPOP_CROP_05:
                        case ASPOP_CROP_06:
                            pdt = &pct[idx];
                            if (pdt->opStatus == ASPOP_STA_UPD) {
                                num++;
                            }

                            break;
                        default:
                            break;
                    }
                    
                }

                if (num == CROP_MAX_NUM) {
                    break;
                }

                sprintf(rs->logs, "wait crop num:%d, %d s\n", num, cnt/2);
                print_f(rs->plogs, "P6", rs->logs);
/*
                if (cnt > 10) {
                    break;
                }
*/
                usleep(500000);
                cnt ++;
            }

            for (i = 0; i < (CROP_MAX_NUM+1); i++) {
                idx = ASPOP_CROP_01 + i;
                pdt = &pct[idx];
                switch(idx) {
                    case ASPOP_CROP_01:
                    case ASPOP_CROP_02:
                    case ASPOP_CROP_03:
                    case ASPOP_CROP_04:
                    case ASPOP_CROP_05:
                    case ASPOP_CROP_06:
                        pdt = &pct[idx];
                        if (pdt->opStatus == ASPOP_STA_UPD) {
                            sprintf(rs->logs, "%d. %x (%d, %d) [0x%.8x]\n", i, pdt->opStatus, pdt->opValue >> 16, pdt->opValue & 0xffff, pdt->opValue); 
                            print_f(rs->plogs, "P6", rs->logs);  
                            sendbuf[3] = 'C';

                            sprintf(rs->logs, "%d,%d,\n\r", pdt->opValue >> 16, pdt->opValue & 0xffff);
                            n = strlen(rs->logs);

                            pdt->opStatus = ASPOP_STA_APP;
                        }

                        break;
                    case ASPOP_IMG_LEN:
                        pdt = &pct[idx];
                        
                        if (pdt->opStatus == ASPOP_STA_UPD) {
                            sendbuf[3] = 'L';
                            sprintf(rs->logs, "%d,\n\r", pdt->opValue & 0xffff);
                            n = strlen(rs->logs);
                        }
                        break;
                    default:
                        break;
                }
                
                if (n > 256) n = 256;
                
                memcpy(&sendbuf[5], rs->logs, n);

                sendbuf[5+n] = 0xfb;
                sendbuf[5+n+1] = '\n';
                sendbuf[5+n+2] = '\0';
                ret = write(rs->psocket_at->connfd, sendbuf, 5+n+3);
                sprintf(rs->logs, "socket send, len:%d content[%s] from %d, ret:%d\n", 5+n+3, sendbuf, rs->psocket_at->connfd, ret);
                print_f(rs->plogs, "P6", rs->logs);
            }
            
            goto socketEnd;
        }

        if (opcode == 0x14) { /* update UTC time */
            sprintf(rs->logs, "handle opcode: 0x%x(UTC)\n", opcode);
            print_f(rs->plogs, "P6", rs->logs);

            ret = asp_strsplit(&strinfo, folder, n);
            n = 0;
            if (ret > 0) {
#if P6_UTC_LOG
                sprintf(rs->logs, "split str found, ret:%d\n", ret);
                print_f(rs->plogs, "P6", rs->logs);
#endif
                for (i = 0; i < ret; i++) {
                    nexinfo = asp_getInfo(strinfo, i);
                    if (nexinfo) {
#if P6_UTC_LOG
                        sprintf(rs->logs, "%d.[%s]\n", i, nexinfo->infoStr);
                        print_f(rs->plogs, "P6", rs->logs);
#endif
                    } else {
                        sprintf(rs->logs, "split info error!!!\n");
                        print_f(rs->plogs, "P6", rs->logs);
                    }
                }                

                nexinfo = asp_getInfo(strinfo, 0);
                if (nexinfo) {
                    sprintf(rs->logs, "%d.%s\n", 0, nexinfo->infoStr);
                    print_f(rs->plogs, "P6", rs->logs);
                }
        
                nexinfo = asp_getInfo(strinfo, 6);
                if (nexinfo) {
                    sprintf(rs->logs, "%d.%s\n", 6, nexinfo->infoStr);
                    print_f(rs->plogs, "P6", rs->logs);

                    if (strcmp("ASP", nexinfo->infoStr) != 0) {
                        sendbuf[3] = 'F';
                    }
                    else {
                        sendbuf[3] = 'D';
                    }
                }

                for (i = 0 ; i < 3; i++) {
                    nexinfo = asp_getInfo(strinfo, i);
                    if (nexinfo) {
                        adata[i] = atoi(nexinfo->infoStr);
#if P6_RX_LOG
                        sprintf(rs->logs, "%d.%s = %d\n", 2+i, nexinfo->infoStr, adata[i]);
                        print_f(rs->plogs, "P6", rs->logs);
#endif
                    } else {
                        break;
                    } 
                }

                for (i = 0 ; i < 3; i++) {
                    nexinfo = asp_getInfo(strinfo, i+3);
                    if (nexinfo) {
                        atime[i] = atoi(nexinfo->infoStr);
#if P6_RX_LOG
                        sprintf(rs->logs, "%d.%s = %d\n", 5+i, nexinfo->infoStr, atime[i]);
                        print_f(rs->plogs, "P6", rs->logs);
#endif
                    } else {
                        break;
                    } 
                }

                memset(curTime, 0, 16);
                sprintf(curTime, "%.4d%.2d%.2d%.2d%.2d", adata[0], adata[1], adata[2], atime[0], atime[1]);

                sprintf(syscmd, "date -s %s", curTime);
                ret = system(syscmd);
#if P6_UTC_LOG
                sprintf(rs->logs, "system command:[%s] ret:%d \n", syscmd, ret);
                print_f(rs->plogs, "P6", rs->logs);
#endif
                sprintf(syscmd, "hwclock -w");
                ret = system(syscmd);
#if P6_UTC_LOG
                sprintf(rs->logs, "system command:[%s] ret:%d \n", syscmd, ret);
                print_f(rs->plogs, "P6", rs->logs);
#endif
                sprintf(syscmd, "date");
                ret = system(syscmd);
#if P6_UTC_LOG
                sprintf(rs->logs, "system command:[%s] ret:%d \n", syscmd, ret);
                print_f(rs->plogs, "P6", rs->logs);

                sprintf(rs->logs, "system time update: %s \n", curTime);
                print_f(rs->plogs, "P6", rs->logs);
#endif                
                char *wday[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"}; 
                struct tm *p; 
                time_t timep;

                time(&timep);
#if P6_UTC_LOG
                sprintf(rs->logs, "get current %s in C \n", ctime(&timep));
                print_f(rs->plogs, "P6", rs->logs);
#endif
                p=localtime(&timep); /*取得當地時間*/ 
                sprintf(rs->logs, "%.4d%.2d%.2d \n", (1900+p->tm_year),( 1+p-> tm_mon), p->tm_mday); 
                print_f(rs->plogs, "P6", rs->logs);
                sprintf(rs->logs, "%s,%.2d:%.2d:%.2d\n", wday[p->tm_wday],p->tm_hour, p->tm_min, p->tm_sec); 
                print_f(rs->plogs, "P6", rs->logs);
            
            }
            else {
                sendbuf[3] = 'F';
                sprintf(rs->logs, "split str not found, ret:%d\n", ret);
                print_f(rs->plogs, "P6", rs->logs);
            }

            sendbuf[5+n] = 0xfb;
            sendbuf[5+n+1] = '\n';
            sendbuf[5+n+2] = '\0';
            ret = write(rs->psocket_at->connfd, sendbuf, 5+n+3);
            sprintf(rs->logs, "socket send, len:%d content[%s] from %d, ret:%d\n", 5+n+3, sendbuf, rs->psocket_at->connfd, ret);
            print_f(rs->plogs, "P6", rs->logs);

            /* free memory */
            nexinfo = strinfo;
            while(nexinfo) {
                nexinfo = asp_freeInfo(nexinfo);
            }
            goto socketEnd;
        }

        if  (!rs->psFat->fatRootdir) {
            sendbuf[3] = 'E';
            sendbuf[5+1] = '\n';
            sendbuf[5+2] = '\0';
            ret = write(rs->psocket_at->connfd, sendbuf, 5+3);
            sprintf(rs->logs, "socket send, len:%d content[%s] from %d, ret:%d\n", 5+3, sendbuf, rs->psocket_at->connfd, ret);
            print_f(rs->plogs, "P6", rs->logs);    

            sprintf(rs->logs, "error!! get opcode = 0x%x\n", opcode);
            print_f(rs->plogs, "P6", rs->logs);
            goto socketEnd;
        }


        if (!fscur) {
            fscur = rs->psFat->fatRootdir;
            pfat->fatCurDir = fscur;
        }

        if (opcode == 0x10) { /* get current path */
        
            //ret = mspFS_folderJump(&nxtf, fscur, folder);
            //ret = mspFS_FolderSearch(&nxtf, rs->psFat->fatRootdir, folder);
            ret = mspFS_Search(&nxtf, rs->psFat->fatRootdir, folder, ASPFS_TYPE_DIR);
            if (ret) {
                sprintf(rs->logs, "jump folder:[%s] failed, ret:%d - \n", folder, ret);
                print_f(rs->plogs, "P6", rs->logs);
            } else {
                fscur = nxtf;
            }
            
            n = 0;

            memset(strPath, 0, 32*128);

            pa = fscur;
            i = 0;
            while(pa) {
                strcpy(strPath[i],((pa->dflen > 0) && (pa->dflen < 128))? pa->dfLFN:pa->dfSFN);
                pa = pa->pa;
                i++;
                if (i >= 32) break;
            }

            memset(strFullPath, 0, 544);
        
            pr = strFullPath;
            while (i) {
                i --;
                *pr = '/';
                pr += 1;
            
                ret = strlen(strPath[i]);
                strncpy(pr, strPath[i], ret);
                pr += ret;        
            }

            *pr = '\0';
            
            sendbuf[3] = 'R';
            sprintf(rs->logs, "%s,%s,%c", strFullPath, ((fscur->dflen > 0) && (fscur->dflen < 128))? fscur->dfLFN:fscur->dfSFN, (fscur->dftype==ASPFS_TYPE_ROOT)?'R':((fscur->dftype==ASPFS_TYPE_DIR)?'D':'F'));
            n = strlen(rs->logs);
            if (n > 256) n = 256;
            memcpy(&sendbuf[5], rs->logs, n);
            sendbuf[5+n] = 0xfb;
            sendbuf[5+n+1] = '\n';
            sendbuf[5+n+2] = '\0';
            ret = write(rs->psocket_at->connfd, sendbuf, 5+n+3);
            sprintf(rs->logs, "socket send, len:%d content[%s] from %d, ret:%d\n", 5+n+3, sendbuf, rs->psocket_at->connfd, ret);
            print_f(rs->plogs, "P6", rs->logs);
        }
        else if (opcode == 0x12) { /* download file */
            n = 0;
            //ret = mspFS_FileSearch(&dnld, rs->psFat->fatRootdir, folder);
            ret = mspFS_Search(&dnld, rs->psFat->fatRootdir, folder, ASPFS_TYPE_FILE);
            if (ret) {
                sprintf(rs->logs, "search file failed ret=%d\n", ret);
                print_f(rs->plogs, "P6", rs->logs);

                sendbuf[3] = 'F';
            } else {
                sprintf(rs->logs, "search file OK ret=%d\n", ret);
                print_f(rs->plogs, "P6", rs->logs);

                sprintf(rs->logs, "show folder: \n");
                print_f(rs->plogs, "P6", rs->logs);
                mspFS_showFolder(dnld->pa);
                secStr = (dnld->dfclstnum - 2)*rs->psFat->fatBootsec->secPrClst + rs->psFat->fatBootsec->secWhroot;
                secLen = dnld->dflength / 512 + ((dnld->dflength%512)==0?0:1);
                sprintf(rs->logs, "start sector:%d sector len:%d, clstStr:%d, size:%d\n", secStr, secLen, dnld->dfclstnum, dnld->dflength);
                print_f(rs->plogs, "P6", rs->logs);

                if (pfat->fatFileDnld) {
                    sprintf(rs->logs, "SD read file to APP (pendding) status:0x%.8x\n", pfat->fatStatus);
                    print_f(rs->plogs, "P6", rs->logs);
                    sendbuf[3] = 'P'; /* pendding */
                } else {
                    if (dnld->dflength) {
                        pfat->fatFileDnld = dnld;
                        pftb->h = 0;
                        sendbuf[3] = 'D';
                    } else {
                        sprintf(rs->logs, "SD read file to APP (empty) status:0x%.8x\n", pfat->fatStatus);
                        print_f(rs->plogs, "P6", rs->logs);
                        sendbuf[3] = 'Z'; /* zero size */
                    }
                }
                memset(strFullPath, 0, 1024);
                sprintf(strFullPath, "%s,%d,0x%.8x", (dnld->dflen == 0)?dnld->dfSFN:dnld->dfLFN, dnld->dflength, dnld->dftype);
                n = strlen(strFullPath);
                memcpy(&sendbuf[5], strFullPath, n);
            }

            sendbuf[5+n] = 0xfb;
            sendbuf[5+n+1] = '\n';
            sendbuf[5+n+2] = '\0';
            ret = write(rs->psocket_at->connfd, sendbuf, 5+n+3);
            sprintf(rs->logs, "socket send, len:%d content[%s] from %d, ret:%d\n", 5+n+3, sendbuf, rs->psocket_at->connfd, ret);
            print_f(rs->plogs, "P6", rs->logs);
            
        } 
        else if (opcode == 0x11) { /* folder list */
            nxtf = 0;
            //ret = mspFS_folderJump(&nxtf, fscur, folder);
            //ret = mspFS_FolderSearch(&nxtf, rs->psFat->fatRootdir, folder);
            ret = mspFS_Search(&nxtf, rs->psFat->fatRootdir, folder, ASPFS_TYPE_DIR);
            if (ret) {
                sprintf(rs->logs, "jump folder:[%s] failed, ret:%d - 2\n", folder, ret);
                print_f(rs->plogs, "P6", rs->logs);
                nxtf = fscur;
            }

            if (nxtf) {
                sprintf(rs->logs, "jump folder:[%s] done, ret:%d, get next folder: 0x%x\n", folder, ret, nxtf);
                print_f(rs->plogs, "P6", rs->logs);
                fscur = nxtf;
            } else {
                sprintf(rs->logs, "jump folder:[%s] failed, ret:%d, get next folder == 0\n", folder, ret);
                print_f(rs->plogs, "P6", rs->logs);
                
                sendbuf[1] = 0x10;
                sendbuf[3] = 'E';
                sendbuf[5+1] = '\n';
                sendbuf[5+2] = '\0';
                ret = write(rs->psocket_at->connfd, sendbuf, 5+3);
                sprintf(rs->logs, "socket send, len:%d content[%s] from %d, ret:%d\n", 5+3, sendbuf, rs->psocket_at->connfd, ret);
                print_f(rs->plogs, "P6", rs->logs);

                goto socketEnd;
            }

            brt = fscur->ch;

            while (brt) {
                while ((brt->dfstats != ASPFS_STATUS_EN) || (strcmp(brt->dfSFN, ".") == 0) 
                       || (brt->dfattrib & ASPFS_ATTR_HIDDEN) || (brt->dfattrib & ASPFS_ATTR_SYSTEM)) {
                    sprintf(rs->logs, "file status[0x%.8x] name[%s] type[0x%.8x] \n", brt->dfstats, brt->dfSFN, brt->dftype);
                    print_f(rs->plogs, "P6", rs->logs);
                    brt = brt->br;           
                }

                if (brt->dflen) {
                    n = strlen(brt->dfLFN);
                    memcpy(&sendbuf[5], brt->dfLFN, n);
                } else {
                    n = strlen(brt->dfSFN);
                    memcpy(&sendbuf[5], brt->dfSFN, n);
                }

                if (brt->dftype == ASPFS_TYPE_FILE) {
                    sendbuf[3] = 'F';
                } else {
                    sendbuf[3] = 'D';
                }

                sendbuf[5+n] = 0xfb;
                sendbuf[5+n+1] = '\n';
                sendbuf[5+n+2] = '\0';
                ret = write(rs->psocket_at->connfd, sendbuf, 5+n+3);
                sprintf(rs->logs, "socket send, len:%d content[%s] from %d, ret:%d\n", 5+n+3, sendbuf, rs->psocket_at->connfd, ret);
                print_f(rs->plogs, "P6", rs->logs);

                brt = brt->br;
                cnt++;
            }

            if (cnt == 0) {
                fscur = rs->psFat->fatRootdir;
            }

            pfat->fatCurDir = fscur;

        }
        else if (opcode == 0x13) { /* upload file */
            sprintf(rs->logs, "handle opcode: 0x%x\n", opcode);
            print_f(rs->plogs, "P6", rs->logs);

            ret = asp_strsplit(&strinfo, folder, n);
            n = 0;
            if (ret > 0) {
                sprintf(rs->logs, "split str found, ret:%d\n", ret);
                print_f(rs->plogs, "P6", rs->logs);
                /*
                nexinfo = strinfo;
                i = 0;
                while (nexinfo) {
                    sprintf(rs->logs, "%d.[%s],len:%d \n", i, nexinfo->infoStr, nexinfo->infoLen);
                    print_f(rs->plogs, "P6", rs->logs);
                    nexinfo = nexinfo->n;
                    i++;
                }
                */
                for (i = 0; i < ret; i++) {
                    nexinfo = asp_getInfo(strinfo, i);
                    if (nexinfo) {
                        sprintf(rs->logs, "%d.[%s]\n", i, nexinfo->infoStr);
                        print_f(rs->plogs, "P6", rs->logs);
                    }
                }                

                nexinfo = asp_getInfo(strinfo, 0);
                if (nexinfo) {
                    sprintf(rs->logs, "%d.%s\n", 0, nexinfo->infoStr);
                    print_f(rs->plogs, "P6", rs->logs);
                }
            
                //ret = mspFS_FileSearch(&dnld, rs->psFat->fatRootdir, nexinfo->infoStr);
                ret = mspFS_Search(&upld, rs->psFat->fatRootdir, nexinfo->infoStr, ASPFS_TYPE_FILE);
                if (ret) {
                    sprintf(rs->logs, "search upload file[%s], not found ret=%d\n", nexinfo->infoStr, ret);
                    print_f(rs->plogs, "P6", rs->logs);

                    sendbuf[3] = 'O';
                } else {
                    sprintf(rs->logs, "search upload file[%s], found ret=%d\n", nexinfo->infoStr, ret);
                    print_f(rs->plogs, "P6", rs->logs);

                    if (recvbuf[be-1] == 'O')  {
                        sendbuf[3] = 'O';
                    } else {
                        sendbuf[3] = 'W';            
                    }
                }

                nexinfo = asp_getInfo(strinfo, 9);
                if (nexinfo) {
                    sprintf(rs->logs, "%d.%s\n", 9, nexinfo->infoStr);
                    print_f(rs->plogs, "P6", rs->logs);

                    if (strcmp("ASP", nexinfo->infoStr) != 0) {
                        sendbuf[3] = 'F';
                    }
                } else {
                    sendbuf[3] = 'F';
                }

                nexinfo = asp_getInfo(strinfo, 2);
                if (nexinfo) {
                    sprintf(rs->logs, "%d.%s\n", 2, nexinfo->infoStr);
                    print_f(rs->plogs, "P6", rs->logs);
                } else {
                    sendbuf[3] = 'F';
                }

                num = atoi(nexinfo->infoStr);
                sprintf(rs->logs, "file length: %d\n", num);
                print_f(rs->plogs, "P6", rs->logs);

                clstSize = pfat->fatBootsec->secSize * pfat->fatBootsec->secPrClst;
                if (num == 0) {
                    cnt = 0;
                } else if (num % clstSize) {
                    cnt = num / clstSize + 1;
                } else {
                    cnt = num / clstSize;
                }

                if (sendbuf[3] == 'W') {
                    nexinfo = asp_getInfo(strinfo, 0);
                    if (nexinfo) {
                        sprintf(rs->logs, "%d.%s\n", 0, nexinfo->infoStr);
                        print_f(rs->plogs, "P6", rs->logs);
                    }
                    sprintf(rs->logs, "WARNING!!! file [%s] existed !!\n", nexinfo->infoStr);
                    print_f(rs->plogs, "P6", rs->logs);
                } else if (cnt > pftb->ftbMng.ftfreeClst) {
                    sendbuf[3] = 'F';
                } else if (pfat->fatFileUpld) {
                    sendbuf[3] = 'P';
                } else {
                    mspFS_allocDir(pfat, &upld);
                    if (!upld) {
                        sendbuf[3] = 'F';
                    } else {
                        memset(upld, 0, sizeof(struct directnFile_s));
                        upld->dftype = ASPFS_TYPE_FILE;
                        upld->dfstats = ASPFS_STATUS_DIS;
                        upld->dfattrib = ASPFS_ATTR_ARCHIVE;
                        upld->dfclstnum = 0; /* start cluster */

                        for (i = 0 ; i < 3; i++) {
                            nexinfo = asp_getInfo(strinfo, 3+i);
                            if (nexinfo) {
                                adata[i] = atoi(nexinfo->infoStr);
                                sprintf(rs->logs, "%d.%s = %d\n", 2+i, nexinfo->infoStr, adata[i]);
                                print_f(rs->plogs, "P6", rs->logs);
                            } else {
                                break;
                            } 
                        }

                        for (i = 0 ; i < 3; i++) {
                            nexinfo = asp_getInfo(strinfo, 6+i);
                            if (nexinfo) {
                                atime[i] = atoi(nexinfo->infoStr);
                                sprintf(rs->logs, "%d.%s = %d\n", 5+i, nexinfo->infoStr, atime[i]);
                                print_f(rs->plogs, "P6", rs->logs);
                            } else {
                                break;
                            } 
                        }
                        
                        upld->dfcredate = ((((adata[0] - 1980) & 0xff) << 16) | ((adata[1] & 0xff) << 8) | (adata[2] & 0xff));
                        upld->dfcretime = (((atime[0]&0xff) << 16) | ((atime[1]&0xff) << 8) | (atime[2]&0xff));
                        upld->dflstacdate = ((((adata[0] - 1980)&0xff) << 16) | ((adata[1]&0xff) << 8) | (adata[2]&0xff));
                        upld->dfrecodate = ((((adata[0] - 1980)&0xff) << 16) | ((adata[1]&0xff) << 8) | (adata[2]&0xff));
                        upld->dfrecotime = (((atime[0]&0xff) << 16) | ((atime[1]&0xff) << 8) | (atime[2]&0xff));

                        upld->dflength = num; /* file length */
                        
                        nexinfo = asp_getInfo(strinfo, 1);
                        if (nexinfo) {
                            sprintf(rs->logs, "%d.%s\n", 1, nexinfo->infoStr);
                            print_f(rs->plogs, "P6", rs->logs);
                        } else {
                            sendbuf[3] = 'F';
                        } 
                        
                        len = strlen(nexinfo->infoStr);
                        if (len > 255) {
                            len = 255;
                        }

                        ret = asp_idxofch(nexinfo->infoStr, '.', 0, len);
                        if (ret == -1) {
                            if (len > 11) {
                                strncpy(upld->dfSFN, nexinfo->infoStr, 10);
                                upld->dfSFN[10] = '~';
                                upld->dfSFN[11] = '\0';
                                strncpy(upld->dfLFN,  nexinfo->infoStr, len);
                                upld->dflen = len;
                                upld->dfLFN[len] = '\0';
                            } else {
                                strncpy(upld->dfSFN, nexinfo->infoStr, len);
                                upld->dfSFN[len] = '\0';
                            }
                        } else {
                            if (ret == (len - 1 - 3)) {
                                if (len > 12) {
                                    strncpy(upld->dfSFN, nexinfo->infoStr, 7);
                                    upld->dfSFN[7] = '~';
                                    upld->dfSFN[8] = '.';
                                    strncpy(&upld->dfSFN[9], &nexinfo->infoStr[len -3], 3);
                                    upld->dfSFN[12] = '\0';
                                    strncpy(upld->dfLFN,  nexinfo->infoStr, len);
                                    upld->dflen = len;
                                    upld->dfLFN[len] = '\0';
                                } else {
                                    strncpy(upld->dfSFN, nexinfo->infoStr, len);
                                    upld->dfSFN[len] = '\0';                        
                                }
                            } else {
                                if (len > 11) {
                                    strncpy(upld->dfSFN, nexinfo->infoStr, 10);
                                    upld->dfSFN[10] = '~';
                                    upld->dfSFN[11] = '\0';
                                    strncpy(upld->dfLFN,  nexinfo->infoStr, len);
                                    upld->dflen = len;
                                    upld->dfLFN[len] = '\0';
                                } else {
                                    strncpy(upld->dfSFN, nexinfo->infoStr, len);
                                    upld->dfSFN[len] = '\0';
                                }
                            }
                        }

                        sprintf(rs->logs, "SFN[%s] LFS[%s] len:%d\n", upld->dfSFN, upld->dfLFN, upld->dflen);
                        print_f(rs->plogs, "P6", rs->logs);
/*
                        nexinfo = asp_getInfo(strinfo, 0);
                        if (nexinfo) {
                            sprintf(rs->logs, "%d.%s\n", 0, nexinfo->infoStr);
                            print_f(rs->plogs, "P6", rs->logs);
                        } else {
                            break;
                        } 

                        ret = mspFS_FileSearch(&dnld, rs->psFat->fatRootdir, nexinfo->infoStr);
                        if (ret) {
                            sprintf(rs->logs, "search upload file[%s], not found ret=%d\n", nexinfo->infoStr, ret);
                            print_f(rs->plogs, "P6", rs->logs);
                            sendbuf[3] = 'O';
                        } else {
                            sendbuf[3] = 'W';            
                        }
*/                        
                        aspFS_insertFATChild(fscur, upld);

                        pfat->fatFileUpld = upld;
                        
                        debugPrintDir(upld);
                    }
                }

                memset(strFullPath, 0, 1024);
                sprintf(strFullPath, "%s,%d,0x%.8x", (upld->dflen == 0)?upld->dfSFN:upld->dfLFN, upld->dflength, upld->dftype);
                n = strlen(strFullPath);
                memcpy(&sendbuf[5], strFullPath, n);
                
                sprintf(rs->logs, "new file need %d clst, available %d clst\n", cnt, pftb->ftbMng.ftfreeClst);
                print_f(rs->plogs, "P6", rs->logs);
                
            }
            else {
                sendbuf[3] = 'F';
                sprintf(rs->logs, "split str not found, ret:%d\n", ret);
                print_f(rs->plogs, "P6", rs->logs);
            }

            sendbuf[5+n] = 0xfb;
            sendbuf[5+n+1] = '\n';
            sendbuf[5+n+2] = '\0';
            ret = write(rs->psocket_at->connfd, sendbuf, 5+n+3);
            sprintf(rs->logs, "socket send, len:%d content[%s] from %d, ret:%d\n", 5+n+3, sendbuf, rs->psocket_at->connfd, ret);
            print_f(rs->plogs, "P6", rs->logs);

            /* free memory */
            nexinfo = strinfo;
            while(nexinfo) {
                nexinfo = asp_freeInfo(nexinfo);
            }
        } 
        else {
            sendbuf[3] = 'E';
            sendbuf[5+1] = '\n';
            sendbuf[5+2] = '\0';
            ret = write(rs->psocket_at->connfd, sendbuf, 5+3);
            sprintf(rs->logs, "socket send, len:%d content[%s] from %d, ret:%d\n", 5+3, sendbuf, rs->psocket_at->connfd, ret);
            print_f(rs->plogs, "P6", rs->logs);    

            sprintf(rs->logs, "error!! get opcode = 0x%x\n", opcode);
            print_f(rs->plogs, "P6", rs->logs);
        }

        socketEnd:
        
        close(rs->psocket_at->connfd);
        rs->psocket_at->connfd = 0;
    }

    p6_end(rs);
    return 0;
}

#define P7_TX_LOG (0)
static int p7(struct procRes_s *rs)
{
    char chbuf[32];
    char ch=0, *addr=0, *cltaddr;
    int ret=0, len=0, num=0, tx=0, cltport=0;
    int cmode;
    struct sdFAT_s *pfat=0;
    struct sdFATable_s   *pftb=0;
    uint32_t secStr=0, secLen=0, datLen=0, minLen=0;
    struct info16Bit_s *p=0, *c=0;
    
    pfat = rs->psFat;
    pftb = pfat->fatTable;

    sprintf(rs->logs, "p7\n");
    print_f(rs->plogs, "P7", rs->logs);

    c = &rs->pmch->cur;
    p = &rs->pmch->tmp;

    p7_init(rs);

    rs->psocket_n->listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (rs->psocket_n->listenfd < 0) { 
        sprintf(rs->logs, "p7 get socket ret: %d", rs->psocket_n->listenfd);
        error_handle(rs->logs, 4017);
    }

    memset(&rs->psocket_n->serv_addr, '0', sizeof(struct sockaddr_in));

    rs->psocket_n->serv_addr.sin_family = AF_INET;
    rs->psocket_n->serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    rs->psocket_n->serv_addr.sin_port = htons(7000); 

    ret = bind(rs->psocket_n->listenfd, (struct sockaddr*)&rs->psocket_n->serv_addr, sizeof(struct sockaddr_in));
    if (ret < 0) {
        //perror("bind:");
        ret = -1;
        if (setsockopt(rs->psocket_n->listenfd, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof(int)) == -1) {
            perror("setsockopt");    
            
            sprintf(rs->logs, "p7 get bind ret: %d", ret);
            error_handle(rs->logs, 24127);
        }
    }

    ret = listen(rs->psocket_n->listenfd, 10); 
    if (ret < 0) {
        sprintf(rs->logs, "p7 get listen ret: %d", ret);
        error_handle(rs->logs, 4035);
    }

    while (1) {
        sprintf(rs->logs, ")\n");
        print_f(rs->plogs, "P7", rs->logs);
#if 1 /* disable for testing */
        len = sizeof(struct sockaddr_in);
        memset(&rs->psocket_n->clint_addr, 0, len);
        rs->psocket_n->connfd = accept(rs->psocket_n->listenfd, (struct sockaddr*)&rs->psocket_n->clint_addr, &len); 
        if (rs->psocket_n->connfd < 0) {
            sprintf(rs->logs, "P7 get connect failed ret:%d", rs->psocket_n->connfd);
            error_handle(rs->logs, 4045);
            goto socketEnd;
        } else {
            //cltaddr = rs->psocket_n->clint_addr.sa_data;
            cltaddr = inet_ntoa(rs->psocket_n->clint_addr.sin_addr);
            cltport = ntohs(rs->psocket_n->clint_addr.sin_port);
            sprintf(rs->logs, "get connection id: %d [%s:%d]\n", rs->psocket_n->connfd, cltaddr, cltport);
            print_f(rs->plogs, "P7", rs->logs);
        }
#else
        rs->psocket_n->connfd = 7;
#endif
        ret = 1; ch = 0;
        while (ret > 0) {

            ret = rs_ipc_get(rs, &ch, 1);
            switch (ch) {
                case 'E':
                    goto socketEnd;
                    break;
                case 'n':
                    cmode = 1;
                    break;
                case 'r':
                    cmode = 2;
                    break;
                case 'd':
                    cmode = 3;
                    break;
                case 'u':
                    cmode = 4;
                    break;
                default:
                    break;
            }

            if (cmode == 1) {
                tx = 0;num = 0;
                while (1) {
                    len = ring_buf_cons(rs->pcmdTx, &addr);
                    if (len >= 0) {
                        tx++;
                    
                        msync(addr, len, MS_SYNC);
                        /* send data to wifi socket */
                        //sprintf(rs->logs, " %d -%d \n", len, tx);
                        //print_f(rs->plogs, "P7", rs->logs);         
                        if (len != 0) {
                            #if 1 /* debug */
                            num = write(rs->psocket_n->connfd, addr, len);
                            #else
                            num = len;
                            #endif
#if P7_TX_LOG
                            sprintf(rs->logs, "tx %d - %d \n", num, tx);
                            print_f(rs->plogs, "P7", rs->logs);         
#endif
                        }
                        rs_ipc_put(rs, "n", 1);
                    } else {
                        sprintf(rs->logs, "%c socket tx %d %d %d- end\n", ch, rs->psocket_n->connfd, num, tx);
                        print_f(rs->plogs, "P7", rs->logs);         
                        break;
                    }

                    if (ch != 'N') {
                        ch = 0;
                        rs_ipc_get(rs, &ch, 1);
                    }
                }
                while (ch != 'N') {
                    sprintf(rs->logs, "%c clr\n", ch);
                    print_f(rs->plogs, "P7", rs->logs);         
                    ch = 0;
                    rs_ipc_get(rs, &ch, 1);
                }

                rs_ipc_put(rs, "N", 1);
                sprintf(rs->logs, "%c socket tx %d - end\n", ch, tx);
                print_f(rs->plogs, "P7", rs->logs);       
                break;
            }
            else if (cmode == 2) {
                if (rs->psocket_n->connfd > 0) {
                    rs_ipc_put(rs, "R", 1);
                    continue;
                } else {
                    rs_ipc_put(rs, "r", 1);
                    break;
                }
            }
            else if (cmode == 3) {
#if MSP_P4_SAVE_DAT
                ret = file_save_get(&rs->fdat_s[0], "/mnt/mmc2/tx/p4%d.dat");
                if (ret) {
                    sprintf(rs->logs, "get tx log data file error - %d, hold here\n", ret);
                    print_f(rs->plogs, "P7", rs->logs);         
                    while(1);
                } else {
                    sprintf(rs->logs, "get tx log data file ok - %d, f: %d\n", ret, rs->fdat_s[0]);
                    print_f(rs->plogs, "P7", rs->logs);         
                }
#endif
                tx = 0;
                while (1) {
                    len = ring_buf_cons_dual(rs->pdataRx, &addr, tx);
                    if (len >= 0) {
                        //printf("cons 0x%x %d %d \n", addr, len, tx);
                        tx++;
                    
                        msync(addr, len, MS_SYNC);
                        /* send data to wifi socket */
#if P7_TX_LOG
                        sprintf(rs->logs, " %d -%d \n", len, tx);
                        print_f(rs->plogs, "P7", rs->logs);         
#endif
                        if (len != 0) {
#if 1 /* debug */
                            num = write(rs->psocket_n->connfd, addr, len);
#else
                            num = len;
#endif
                            //printf("socket tx %d %d\n", rs->psocket_r->connfd, num);
                            //sprintf(rs->logs, "%c socket tx %d %d %d \n", ch, rs->psocket_n->connfd, num, tx);
                            //print_f(rs->plogs, "P7", rs->logs);         
#if MSP_P4_SAVE_DAT
                            fwrite(addr, 1, len, rs->fdat_s[0]);
                            fflush(rs->fdat_s[0]);
#endif
                        }
                    } else {
                        sprintf(rs->logs, "%c socket tx %d %d %d- end\n", ch, rs->psocket_n->connfd, num, tx);
                        print_f(rs->plogs, "P7", rs->logs);         
                        break;
                    }

                    if (ch != 'D') {
                        ch = 0;
                        rs_ipc_get(rs, &ch, 1);
                    }
                }
                while (ch != 'D') {
                    sprintf(rs->logs, "%c clr\n", ch);
                    print_f(rs->plogs, "P7", rs->logs);         
                    ch = 0;
                    rs_ipc_get(rs, &ch, 1);
                }

                rs_ipc_put(rs, "D", 1);
                sprintf(rs->logs, "%c socket tx %d - end\n", ch, tx);
                print_f(rs->plogs, "P7", rs->logs);         
#if MSP_P4_SAVE_DAT
                fclose(rs->fdat_s[0]);
#endif
                break;
            }
            else if (cmode == 4) {                
                secStr = c->opinfo;
                secLen = p->opinfo;
                datLen = secLen * 512;
                minLen = 16 * 512;

                sprintf(rs->logs, "ready for tx %d/%d\n", datLen, minLen);
                print_f(rs->plogs, "P7", rs->logs);

                if (datLen < minLen) {
                    datLen = minLen;
                }

                rs_ipc_get(rs, &ch, 1);

                memset(chbuf, 0, 32);
                sprintf(chbuf, "%d\0", datLen);
                ret = write(rs->psocket_n->connfd, chbuf, strlen(chbuf));
                sprintf(rs->logs, "get %c socket tx [%s], ret:%d\n", ch, chbuf, ret);
                print_f(rs->plogs, "P7", rs->logs);       
                
                while (1) {
                    rs_ipc_get(rs, &ch, 1);
                    if (ch == 'U') break;
                }
                
                rs_ipc_put(rs, "U", 1);
                sprintf(rs->logs, "%c socket tx %d - end\n", ch, tx);
                print_f(rs->plogs, "P7", rs->logs);       
                
                if (!pftb->c) {
                    memset(chbuf, 0, 32);
                    sprintf(chbuf, "%c\0", ch);
                    ret = write(rs->psocket_n->connfd, chbuf, strlen(chbuf));
                    sprintf(rs->logs, "send %c to APP to notice the end, ret:%d\n", ch, ret);
                    print_f(rs->plogs, "P7", rs->logs);       
                
                    sprintf(rs->logs, "connect break \n");
                    print_f(rs->plogs, "P7", rs->logs);       

                    break;
                }
            }
            else {
                sprintf(rs->logs, "cmode: %d - 7\n", cmode);
                print_f(rs->plogs, "P7", rs->logs);
                error_handle(rs->logs, 5141);
            }
            
        }

        socketEnd:
        close(rs->psocket_n->connfd);
        rs->psocket_n->connfd = 0;
    }

    p7_end(rs);
    return 0;
}

void *get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }	
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int get_in_port(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return ntohs(((struct sockaddr_in*)sa)->sin_port);
    }	
    return ntohs(((struct sockaddr_in6*)sa)->sin6_port);
}

static int p8(struct procRes_s *rs)
{
#define RECVLEN 1024
#define MYPORT "8000"
    int ret=0, n=0, tot=0, len=0, cltport=0;
    char *recvbuf=0, *sendbuf=0;
    char ack[8] = "ack\n\0";
    char *cltaddr=0;

    struct addrinfo hints, *servinfo, *p;
    struct addrinfo clients, *clintinfo, *c;
    struct sockaddr_in *saddr, *haddr;
    const struct sockaddr_storage their_addr;
    char s[INET6_ADDRSTRLEN];
    char d[INET6_ADDRSTRLEN];
    char port[8];
    int rv, sockfd, sockback;

    struct ifaddrs *ifaddr, *ifa;

    memset(&hints, 0, sizeof(hints));

    sprintf(rs->logs, "p8\n");
    print_f(rs->plogs, "P8", rs->logs);

    p8_init(rs);

    recvbuf = aspMalloc(RECVLEN);
    if (!recvbuf) {
        sprintf(rs->logs, "p8 get memory alloc falied");
        error_handle(rs->logs, 24043);
    }

    sendbuf = aspMalloc(RECVLEN);
    if (!sendbuf) {
        sprintf(rs->logs, "p8 get memory alloc falied");
        error_handle(rs->logs, 24179);
    }

#if 1
    hints.ai_family = AF_INET; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    while (1) {

        if (getifaddrs(&ifaddr) == -1) {
            perror("getifaddrs");        
            //exit(EXIT_FAILURE);    
            continue;
        }

        memset(sendbuf, 0, RECVLEN);
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == NULL)
                continue;

            ret=getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in),s, INET6_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);

            if((strcmp(ifa->ifa_name, rs->pnetIntfs)==0)&&(ifa->ifa_addr->sa_family==AF_INET)) {
                if (ret != 0) {
                    printf("getnameinfo() failed: %s\n", gai_strerror(ret));
                    //exit(EXIT_FAILURE);
                    continue;
                }
                printf("\tInterface : <%s>\n",ifa->ifa_name );
                printf("\t  Address : <%s>\n", s);

                sprintf(sendbuf, "iface: %s addr: %s port: %d", rs->pnetIntfs, s, 8000);
            }
        }
        freeifaddrs(ifaddr);

        if (strlen(sendbuf) == 0) {
            continue;
        }
        
        if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));	
            return 1;	
        }
        for(p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                perror("listener: socket");
                continue;
            }
            if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {

                //perror("listener: bind");
                ret = -1;
                if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof(int)) == -1) {
                    perror("setsockopt");    
                    close(sockfd);
                    continue;
                }
            }
            
            break;
        }

        if (p == NULL) {
            fprintf(stderr, "listener: failed to bind socket\n");
            freeaddrinfo(servinfo);
            continue;
        }

        printf("listener: waiting to recvfrom...\n");
        memset(recvbuf, 0, RECVLEN);
        len = sizeof(their_addr);
        if ((n = recvfrom(sockfd, recvbuf, RECVLEN-1 , 0, (struct sockaddr *)&their_addr, &len)) == -1) {
            perror("recvfrom");
            freeaddrinfo(servinfo);
            close(sockfd);
            continue;
        }

        //haddr = (struct sockaddr_in *)p->ai_addr;
        //sprintf(sendbuf, "addr: %s port: %d", inet_ntop(AF_INET, &(haddr->sin_addr), s, INET_ADDRSTRLEN), get_in_port((struct sockaddr *)&(haddr->sin_addr)));

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), d, sizeof(d));
        memset(port, 0, 8);
        sprintf(port, "%d", get_in_port((struct sockaddr *)&their_addr));
        printf("listener: got packet from %s : %s\n", d, port);
        printf("listener: packet is %d bytes long\n", n);	
        recvbuf[n] = '\0';	
        printf("listener: packet contains \"%s\"\n", recvbuf);	

        freeaddrinfo(servinfo);
        close(sockfd);
        
        memset(&clients, 0, sizeof(clients));
        clients.ai_family = AF_INET; // set to AF_INET to force IPv4
        clients.ai_socktype = SOCK_DGRAM;

        if ((rv = getaddrinfo(d, port, &clients, &clintinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));	
            close(sockfd);
            continue;
        }

        for(c = clintinfo; c != NULL; c = c->ai_next) {
            if ((sockback = socket(c->ai_family, c->ai_socktype, c->ai_protocol)) == -1) {
                perror("listener: socket");
                continue;
            }
            break;
        }

        if (c == NULL) {
            fprintf(stderr, "listener: failed to bind socket\n");
            freeaddrinfo(clintinfo);
            continue;
        }

        n = strlen(sendbuf);
        sendbuf[n] = '\0';
        saddr = (struct sockaddr_in *)c->ai_addr;
        printf("listener: sendto packet contains \"%s\", size: %d, addr: %s \n", sendbuf, n, inet_ntop(AF_INET, &(saddr->sin_addr), d, INET_ADDRSTRLEN));	
        
        if (n = sendto(sockback, sendbuf, n+1 , 0, c->ai_addr, c->ai_addrlen) == -1) {
            perror("sendto");
            close(sockback);
            continue;
        }

        freeaddrinfo(clintinfo);
        close(sockback);
    }

#else
    rs->psocket_v->listenfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (rs->psocket_v->listenfd < 0) { 
        sprintf(rs->logs, "p8 get socket ret: %d", rs->psocket_v->listenfd);
        error_handle(rs->logs, 24025);
    }

    memset(&rs->psocket_v->serv_addr, '0', sizeof(struct sockaddr_in));

    rs->psocket_v->serv_addr.sin_family = AF_INET;
    rs->psocket_v->serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    rs->psocket_v->serv_addr.sin_port = htons(8000); 

    ret = bind(rs->psocket_v->listenfd, (struct sockaddr*)&rs->psocket_v->serv_addr, sizeof(struct sockaddr_in));
    if (ret < 0) {
        perror("bind:");
        sprintf(rs->logs, "p8 get bind ret: %d", ret);
        error_handle(rs->logs, 24037);
    }

    ret = listen(rs->psocket_v->listenfd, 10); 
    if (ret < 0) {
        sprintf(rs->logs, "p8 get listen ret: %d", ret);
        error_handle(rs->logs, 24043);
    }

    while (1) {
        sprintf(rs->logs, "#\n");
        print_f(rs->plogs, "P8", rs->logs);
        
        sprintf(rs->logs, "START \n");
        print_f(rs->plogs, "P8", rs->logs);


        len = sizeof(struct sockaddr_in);
        memset(&rs->psocket_v->clint_addr, 0, len);
        rs->psocket_v->connfd = accept(rs->psocket_v->listenfd, (struct sockaddr*)&rs->psocket_v->clint_addr, &len); 
        if (rs->psocket_v->connfd < 0) {
            sprintf(rs->logs, "P8 get connect failed ret:%d", rs->psocket_v->connfd);
            error_handle(rs->logs, 24053);
            goto socketEnd;
        } else {
            cltaddr = inet_ntoa(rs->psocket_v->clint_addr.sin_addr);
            //cltaddr = rs->psocket_v->clint_addr.sa_data;
            cltport = ntohs(rs->psocket_v->clint_addr.sin_port);
            sprintf(rs->logs, "get connection id: %d [%s:%d]\n", rs->psocket_v->connfd, cltaddr, cltport);
            print_f(rs->plogs, "P8", rs->logs);
        }

        n = 0; tot = 0;
        n = recv(rs->psocket_v->connfd, recvbuf, 1024, 0);
        while (n) {
            tot += n;
            ret = send(rs->psocket_v->connfd, ack, 4, 0);
            
            sprintf(rs->logs, "get and send back %d bytes [%s] \n", n, recvbuf);
            print_f(rs->plogs, "P8", rs->logs);
            
            //shmem_dump(recvbuf, n);
            n = 0;
            n = recv(rs->psocket_v->connfd, recvbuf, 1024, 0);

        }

        sprintf(rs->logs, "END total: %d\n", tot);
        print_f(rs->plogs, "P8", rs->logs);

        socketEnd:
        close(rs->psocket_v->connfd);
        rs->psocket_v->connfd = 0;
    }
#endif
    p8_end(rs);
    return 0;
}

#define DATA_RX_SIZE 64
#define DATA_TX_SIZE 64
#define CMD_RX_SIZE 64
#define CMD_TX_SIZE 64
int main(int argc, char *argv[])
{
//static char spi1[] = "/dev/spidev32766.0"; 
//static char spi0[] = "/dev/spidev32765.0"; 
    char dir[256] = "/mnt/mmc2";
    struct mainRes_s *pmrs;
    struct procRes_s rs[10];
    int ix, ret, len;
    char *log;
    int tdiff;
    int arg[8];
    uint32_t bitset;
    char syscmd[256] = "ls -al";
    
    totMalloc = (int *)mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    memset(totMalloc, 0, sizeof(int));
    totSalloc = (int *)mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    memset(totSalloc, 0, sizeof(int));

    pmrs = (struct mainRes_s *)aspSalloc(sizeof(struct mainRes_s));
    memset(pmrs, 0, sizeof(struct mainRes_s));

    pmrs->plog.max = OUT_BUFF_LEN - 4096;
    pmrs->plog.pool = aspSalloc(pmrs->plog.max);
    if (!pmrs->plog.pool) {printf("get log pool share memory failed\n"); return 0;}
    mlogPool = &pmrs->plog;
    pmrs->plog.len = 0;
    pmrs->plog.cur = pmrs->plog.pool;

    ret = file_save_get(&pmrs->flog, "/mnt/mmc2/rx/%d.log");
    if (ret) {printf("get log file failed\n"); return 0;}
    mlog = pmrs->flog;
    ret = fwrite("test file write \n", 1, 16, pmrs->flog);
    sprintf(pmrs->log, "write file size: %d/%d\n", ret, 16);
    print_f(&pmrs->plog, "fwrite", pmrs->log);
    fflush(pmrs->flog);

    sprintf(pmrs->log, "argc:%d\n", argc);
    print_f(&pmrs->plog, "main", pmrs->log);
// show arguments
    memset(arg, 0, 8);
    ix = 0;
    while(argc) {
        arg[ix] = atoi(argv[ix]);
        sprintf(pmrs->log, "%d %d %s\n", ix, arg[ix], argv[ix]);
        print_f(&pmrs->plog, "arg", pmrs->log);
        ix++;
        argc--;
        if (ix > 7) break;
    }

    pmrs->spioc1 = aspSalloc(sizeof(struct spi_ioc_transfer));
    pmrs->spioc2 = aspSalloc(sizeof(struct spi_ioc_transfer));

    /* create folder */
    sprintf(syscmd, "mkdir -p /root/scaner");
    ret = doSystemCmd(syscmd);

    sprintf(syscmd, "mkdir -p /mnt/mmc2/rx");
    ret = doSystemCmd(syscmd);

    sprintf(syscmd, "mkdir -p /mnt/mmc2/tx");
    ret = doSystemCmd(syscmd);

// launchAP or directAccess
    /* clear status */
    sprintf(syscmd, "kill -9 $(ps aux | grep 'uap0' | awk '{print $1}')");
    ret = doSystemCmd(syscmd);

    //sprintf(syscmd, "kill -9 $(ps aux | grep 'mothership' | awk '{print $1}')");
    //ret = doSystemCmd(syscmd);

    sprintf(syscmd, "kill -9 $(ps aux | grep 'hostapd' | awk '{print $1}')");
    ret = doSystemCmd(syscmd);

    sprintf(syscmd, "ifconfig uap0 down");
    ret = doSystemCmd(syscmd);

    sprintf(syscmd, "kill -9 $(ps aux | grep 'mlan0' | awk '{print $1}')");
    ret = doSystemCmd(syscmd);
        
    sprintf(syscmd, "kill -9 $(ps aux | grep 'wpa_supplicant' | awk '{print $1}')");
    ret = doSystemCmd(syscmd);

    sprintf(syscmd, "ifconfig mlan0 down");
    ret = doSystemCmd(syscmd);
    
    sprintf(pmrs->log, "network interface: %s \n", pmrs->netIntfs);
    print_f(&pmrs->plog, "inet", pmrs->log);

    sleep(1);
    
// initial share parameter
    len = sizeof(struct aspMetaData);
    pmrs->metaout = aspSalloc(len);
    pmrs->metain = aspSalloc(len);

    len = SPI_TRUNK_SZ;
    pmrs->metaMass.masspt = aspSalloc(len);    
    pmrs->metaMass.massMax = len;
    pmrs->metaMass.massUsed = 0;

    if ((pmrs->metaout) && (pmrs->metain) && (pmrs->metaMass.masspt)) {
        sprintf(pmrs->log, "inbuff addr(0x%.8x), outbuff addr(0x%.8x), massbuff addr(0x%.8x) \n", pmrs->metain, pmrs->metaout, pmrs->metaMass.masspt);
        print_f(&pmrs->plog, "meta", pmrs->log);
    } else {
        sprintf(pmrs->log, "Error!! allocate meta memory failed!!!! \n");
        print_f(&pmrs->plog, "meta", pmrs->log);
    }

    sprintf(pmrs->log, "allocate %d byte share memory for meta data, addr: 0x%.8x\n", len, pmrs->metaout);
    print_f(&pmrs->plog, "metaout", pmrs->log);
    
    /* data mode rx from spi */
    clock_gettime(CLOCK_REALTIME, &pmrs->time[0]);
    pmrs->dataRx.pp = memory_init(&pmrs->dataRx.slotn, DATA_RX_SIZE*SPI_TRUNK_SZ, SPI_TRUNK_SZ); // 4MB
    if (!pmrs->dataRx.pp) goto end;
    pmrs->dataRx.r = (struct ring_p *)aspSalloc(sizeof(struct ring_p));
    pmrs->dataRx.totsz = DATA_RX_SIZE*SPI_TRUNK_SZ;
    pmrs->dataRx.chksz = SPI_TRUNK_SZ;
    pmrs->dataRx.svdist = 8;
    //sprintf(pmrs->log, "totsz:%d pp:0x%.8x\n", pmrs->dataRx.totsz, pmrs->dataRx.pp);
    //print_f(&pmrs->plog, "minit_result", pmrs->log);
    //for (ix = 0; ix < pmrs->dataRx.slotn; ix++) {
    //    sprintf(pmrs->log, "[%d] 0x%.8x\n", ix, pmrs->dataRx.pp[ix]);
    //    print_f(&pmrs->plog, "shminit_result", pmrs->log);
    //}
    clock_gettime(CLOCK_REALTIME, &pmrs->time[1]);
    tdiff = time_diff(&pmrs->time[0], &pmrs->time[1], 1000);
    sprintf(pmrs->log, "tdiff:%d \n", tdiff);
    print_f(&pmrs->plog, "time_diff", pmrs->log);

    /* data mode tx to spi */
    clock_gettime(CLOCK_REALTIME, &pmrs->time[0]);
    pmrs->dataTx.pp = memory_init(&pmrs->dataTx.slotn, DATA_TX_SIZE*SPI_TRUNK_SZ, SPI_TRUNK_SZ); // 4MB
    if (!pmrs->dataTx.pp) goto end;
    pmrs->dataTx.r = (struct ring_p *)aspSalloc(sizeof(struct ring_p));
    pmrs->dataTx.totsz = DATA_TX_SIZE*SPI_TRUNK_SZ;
    pmrs->dataTx.chksz = SPI_TRUNK_SZ;
    pmrs->dataTx.svdist = 12;
    //sprintf(pmrs->log, "totsz:%d pp:0x%.8x\n", pmrs->dataTx.totsz, pmrs->dataTx.pp);
    //print_f(&pmrs->plog, "minit_result", pmrs->log);
    //for (ix = 0; ix < pmrs->dataTx.slotn; ix++) {
    //    sprintf(pmrs->log, "[%d] 0x%.8x\n", ix, pmrs->dataTx.pp[ix]);
    //    print_f(&pmrs->plog, "shminit_result", pmrs->log);
    //}
    clock_gettime(CLOCK_REALTIME, &pmrs->time[1]);
    tdiff = time_diff(&pmrs->time[0], &pmrs->time[1], 1000);
    sprintf(pmrs->log, "tdiff:%d \n", tdiff);
    print_f(&pmrs->plog, "time_diff", pmrs->log);

    /* cmd mode rx from spi */
    clock_gettime(CLOCK_REALTIME, &pmrs->time[0]);
    pmrs->cmdRx.pp = memory_init(&pmrs->cmdRx.slotn, CMD_RX_SIZE*SPI_TRUNK_SZ, SPI_TRUNK_SZ); // 4MB
    if (!pmrs->cmdRx.pp) goto end;
    pmrs->cmdRx.r = (struct ring_p *)aspSalloc(sizeof(struct ring_p));
    pmrs->cmdRx.totsz = CMD_RX_SIZE*SPI_TRUNK_SZ;;
    pmrs->cmdRx.chksz = SPI_TRUNK_SZ;
    pmrs->cmdRx.svdist = 16;
    //sprintf(pmrs->log, "totsz:%d pp:0x%.8x\n", pmrs->cmdRx.totsz, pmrs->cmdRx.pp);
    //print_f(&pmrs->plog, "minit_result", pmrs->log);
    //for (ix = 0; ix < pmrs->cmdRx.slotn; ix++) {
    //    sprintf(pmrs->log, "[%d] 0x%.8x\n", ix, pmrs->cmdRx.pp[ix]);
    //    print_f(&pmrs->plog, "shminit_result", pmrs->log);
    //}
    clock_gettime(CLOCK_REALTIME, &pmrs->time[1]);
    tdiff = time_diff(&pmrs->time[0], &pmrs->time[1], 1000);
    sprintf(pmrs->log, "tdiff:%d \n", tdiff);
    print_f(&pmrs->plog, "time_diff", pmrs->log);
    
    /* cmd mode tx to spi */
    clock_gettime(CLOCK_REALTIME, &pmrs->time[0]);
    pmrs->cmdTx.pp = memory_init(&pmrs->cmdTx.slotn, CMD_TX_SIZE*SPI_TRUNK_SZ, SPI_TRUNK_SZ); // 4MB
    if (!pmrs->cmdTx.pp) goto end;
    pmrs->cmdTx.r = (struct ring_p *)aspSalloc(sizeof(struct ring_p));
    pmrs->cmdTx.totsz = CMD_TX_SIZE*SPI_TRUNK_SZ;
    pmrs->cmdTx.chksz = SPI_TRUNK_SZ;
    pmrs->cmdTx.svdist = 16;
    //sprintf(pmrs->log, "totsz:%d pp:0x%.8x\n", pmrs->cmdTx.totsz, pmrs->cmdTx.pp);
    //print_f(&pmrs->plog, "minit_result", pmrs->log);
    //for (ix = 0; ix < pmrs->cmdTx.slotn; ix++) {
    //    sprintf(pmrs->log, "[%d] 0x%.8x\n", ix, pmrs->cmdTx.pp[ix]);
    //    print_f(&pmrs->plog, "shminit_result", pmrs->log);
    //}
    clock_gettime(CLOCK_REALTIME, &pmrs->time[1]);
    tdiff = time_diff(&pmrs->time[0], &pmrs->time[1], 1000);
    sprintf(pmrs->log, "tdiff:%d \n", tdiff);
    print_f(&pmrs->plog, "time_diff", pmrs->log);

    pmrs->wtg.wtRlt =  aspSalloc(16);
    if (pmrs->wtg.wtRlt) {
        sprintf(pmrs->log, "wtg result buff:0x%.8x - DONE\n", pmrs->wtg.wtRlt);
        print_f(&pmrs->plog, "WTG", pmrs->log);
    } else {
        sprintf(pmrs->log, "wtg result buff alloc failed!!- ERROR\n", pmrs->wtg.wtRlt);
        print_f(&pmrs->plog, "WTG", pmrs->log);
    }
    pmrs->wtg.wtMrs = pmrs;

    ret = file_save_get(&pmrs->fs, "/mnt/mmc2/rx/%d.bin");
    if (ret) {printf("get save file failed\n"); return 0;}
    //ret = fwrite("test file write \n", 1, 16, pmrs->fs);
    sprintf(pmrs->log, "write file size: %d/%d\n", ret, 16);
    print_f(&pmrs->plog, "fwrite", pmrs->log);

    char paramFilePath[128] = "/root/scaner/scannerParam.bin";
    FILE *fprm=0;
    struct aspConfig_s* ctb = 0;
    int parmLen=0, parmTotz=0, readLen=0;

    parmTotz = ASPOP_CODE_MAX*sizeof(struct aspConfig_s);
    
    fprm = fopen(paramFilePath, "r");
    if (fprm) {
        ctb = pmrs->configTable;

        ret = fseek(fprm, 0, SEEK_END);
        if (ret) {
            sprintf(pmrs->log, " file seek failed!! ret:%d \n", ret);
            print_f(&pmrs->plog, "PRAM", pmrs->log);
        } 

        parmLen = ftell(fprm);
        sprintf(pmrs->log, " file [%s] size: %d \n", paramFilePath, parmLen);
        print_f(&pmrs->plog, "PRAM", pmrs->log);

        ret = fseek(fprm, 0, SEEK_SET);
        if (ret) {
            sprintf(pmrs->log, " file seek failed!! ret:%d \n", ret);
            print_f(&pmrs->plog, "PRAM", pmrs->log);
        }
        if (parmLen == parmTotz) {
            readLen = fread(ctb, 1, parmLen, fprm);
        } else {
            sprintf(pmrs->log, " file size error !!! filesize:%d, memsize: %d\n", parmLen, parmTotz);
            print_f(&pmrs->plog, "PRAM", pmrs->log);
        }
        fclose(fprm);
        /* reset the run time parameters */
        #if 0
        ctb = &pmrs->configTable[ASPOP_SCAN_SINGLE];
        ctb->opStatus = ASPOP_STA_NONE;
        ctb->opCode = OP_SINGLE;
        ctb->opType = ASPOP_TYPE_VALUE;
        ctb->opValue = 0xff;
        ctb->opMask = ASPOP_MASK_8;
        ctb->opBitlen = 8;
        
        ctb = &pmrs->configTable[ASPOP_SCAN_DOUBLE];
        ctb->opStatus = ASPOP_STA_NONE;
        ctb->opCode = OP_DOUBLE;
        ctb->opType = ASPOP_TYPE_VALUE;
        ctb->opValue = 0xff;
        ctb->opMask = ASPOP_MASK_8;
        ctb->opBitlen = 8;
        
        ctb = &pmrs->configTable[ASPOP_ACTION]; 
        ctb->opStatus = ASPOP_STA_NONE;
        ctb->opCode = OP_ACTION;
        ctb->opType = ASPOP_TYPE_VALUE;
        ctb->opValue = 0xff;
        ctb->opMask = ASPOP_MASK_3;
        ctb->opBitlen = 8;
        #else
        for (ix = 0; ix < ASPOP_CODE_MAX; ix++) {
            ctb = &pmrs->configTable[ix];
            switch(ix) {
            case ASPOP_SCAN_SINGLE: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_SINGLE;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SCAN_DOUBLE: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_DOUBLE;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_ACTION: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_ACTION;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_3;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFAT_RD: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_SDRD;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFAT_WT: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_SDWT;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFAT_STR01: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STSEC_00;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFAT_STR02: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STSEC_01;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFAT_STR03: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STSEC_02;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFAT_STR04: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STSEC_03;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFAT_LEN01: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_00;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFAT_LEN02: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_01;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFAT_LEN03: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_02;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFAT_LEN04: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_03;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFAT_SDAT: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_SDAT;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_REG_RD: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_RGRD;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_REG_WT: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_RGWT;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_REG_ADDRH: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_RGADD_H;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_REG_ADDRL: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_RGADD_L;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_REG_DAT: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_RGDAT;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SUP_SAVE: 
                ctb->opStatus = ASPOP_STA_NONE; // for debug, should be ASPOP_STA_NONE
                ctb->opCode = OP_SUPBACK;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff; // for debug, should be 0xff
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFREE_FREESEC: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_FREESEC;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFREE_STR01: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STSEC_00;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFREE_STR02: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STSEC_01;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFREE_STR03: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STSEC_02;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFREE_STR04: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STSEC_03;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFREE_LEN01: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_00;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFREE_LEN02: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_01;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFREE_LEN03: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_02;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFREE_LEN04: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_03;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDUSED_USEDSEC: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_USEDSEC;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDUSED_STR01: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STSEC_00;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDUSED_STR02: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STSEC_01;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDUSED_STR03: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STSEC_02;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDUSED_STR04: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STSEC_03;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDUSED_LEN01: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_00;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDUSED_LEN02: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_01;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDUSED_LEN03: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_02;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDUSED_LEN04: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_03;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_CROP_01: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_CROP_01;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xffffffff;
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_CROP_02: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_CROP_02;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xffffffff;
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_CROP_03: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_CROP_03;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xffffffff;
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_CROP_04: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_CROP_04;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xffffffff;
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_CROP_05: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_CROP_05;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xffffffff;
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_CROP_06: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_CROP_06;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xffffffff;
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_IMG_LEN: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_IMG_LEN;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xffffffff;
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_CROP_COOR_XH: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_00;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_CROP_COOR_XL: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_01;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_CROP_COOR_YH: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_02;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_CROP_COOR_YL: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_03;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_EG_DECT: 
                ctb->opStatus = ASPOP_STA_UPD; /* default enable to test CROP */
                ctb->opCode = OP_EG_DECT;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0x1;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            #if 0 /* test AP mode */
            case ASPOP_AP_MODE: 
                ctb->opStatus = ASPOP_STA_APP; //default for debug ASPOP_STA_NONE;
                ctb->opCode = OP_AP_MODEN;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = APM_AP;  /* default ap mode */
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            #endif
            case ASPOP_XCROP_GAT: 
                ctb->opStatus = ASPOP_STA_NONE; /* set to ASPOP_STA_SCAN from scanner*/
                ctb->opCode = OP_META_DAT;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0x0;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_XCROP_LINSTR: 
                ctb->opStatus = ASPOP_STA_NONE; /* set to ASPOP_STA_SCAN from scanner*/
                ctb->opCode = OP_META_DAT;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0x0;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_XCROP_LINREC: 
                ctb->opStatus = ASPOP_STA_NONE; /* set to ASPOP_STA_SCAN from scanner*/
                ctb->opCode = OP_META_DAT;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0x0;
                ctb->opMask = ASPOP_MASK_16;
                ctb->opBitlen = 16;
                break;
            }
        }
        #endif
    }
    
    if (readLen == 0) { 
        sprintf(pmrs->log, " load scaner parameter at [%s] failed !!! Reset configuration!!!", paramFilePath);
        print_f(&pmrs->plog, "PRAM", pmrs->log);
    
        for (ix = 0; ix < ASPOP_CODE_MAX; ix++) {
            ctb = &pmrs->configTable[ix];
            switch(ix) {
            case ASPOP_CODE_NONE:   
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = 0;
                ctb->opType = ASPOP_TYPE_NONE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_0;
                ctb->opBitlen = 0;
                break;
            case ASPOP_FILE_FORMAT: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_FFORMAT;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_COLOR_MODE:  
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_COLRMOD;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_3;
                ctb->opBitlen = 8;
                break;
            case ASPOP_COMPRES_RATE:
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_COMPRAT;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_3;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SCAN_SINGLE: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_SINGLE;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SCAN_DOUBLE: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_DOUBLE;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_ACTION: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_ACTION;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_3;
                ctb->opBitlen = 8;
                break;
            case ASPOP_RESOLUTION:
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_RESOLTN;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_3;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SCAN_GRAVITY:
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_SCANGAV;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_2;
                ctb->opBitlen = 8;
                break;
            case ASPOP_MAX_WIDTH:   
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_MAXWIDH;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_2;
                ctb->opBitlen = 8;
                break;
            case ASPOP_WIDTH_ADJ_H: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_WIDTHAD_H;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_WIDTH_ADJ_L: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_WIDTHAD_L;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SCAN_LENS_H: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_SCANLEN_H;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SCAN_LENS_L: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_SCANLEN_L;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_INTER_IMG: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_INTERIMG;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_3;
                ctb->opBitlen = 8;
                break;
            case ASPOP_AFEIC_SEL: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_AFEIC;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_EXT_PULSE: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_EXTPULSE;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_3;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFAT_RD: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_SDRD;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFAT_WT: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_SDWT;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFAT_STR01: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STSEC_00;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFAT_STR02: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STSEC_01;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFAT_STR03: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STSEC_02;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFAT_STR04: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STSEC_03;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFAT_LEN01: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_00;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFAT_LEN02: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_01;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFAT_LEN03: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_02;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFAT_LEN04: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_03;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFAT_SDAT: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_SDAT;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_REG_RD: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_RGRD;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_REG_WT: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_RGWT;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_REG_ADDRH: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_RGADD_H;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_REG_ADDRL: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_RGADD_L;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_REG_DAT: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_RGDAT;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SUP_SAVE: 
                ctb->opStatus = ASPOP_STA_NONE; // for debug, should be ASPOP_STA_NONE
                ctb->opCode = OP_SUPBACK;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff; // for debug, should be 0xff
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFREE_FREESEC: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_FREESEC;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFREE_STR01: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STSEC_00;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFREE_STR02: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STSEC_01;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFREE_STR03: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STSEC_02;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFREE_STR04: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STSEC_03;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFREE_LEN01: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_00;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFREE_LEN02: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_01;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFREE_LEN03: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_02;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDFREE_LEN04: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_03;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDUSED_USEDSEC: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_USEDSEC;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDUSED_STR01: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STSEC_00;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDUSED_STR02: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STSEC_01;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDUSED_STR03: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STSEC_02;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDUSED_STR04: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STSEC_03;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDUSED_LEN01: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_00;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDUSED_LEN02: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_01;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDUSED_LEN03: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_02;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_SDUSED_LEN04: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_03;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_FUNTEST_00: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_FUNCTEST_00;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_FUNTEST_01: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_FUNCTEST_01;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_FUNTEST_02: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_FUNCTEST_02;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_FUNTEST_03: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_FUNCTEST_03;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_FUNTEST_04: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_FUNCTEST_04;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_FUNTEST_05: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_FUNCTEST_05;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_FUNTEST_06: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_FUNCTEST_06;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_FUNTEST_07: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_FUNCTEST_07;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_FUNTEST_08: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_FUNCTEST_08;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_FUNTEST_09: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_FUNCTEST_09;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_FUNTEST_10: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_FUNCTEST_10;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_FUNTEST_11: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_FUNCTEST_11;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_FUNTEST_12: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_FUNCTEST_12;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_FUNTEST_13: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_FUNCTEST_13;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_FUNTEST_14: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_FUNCTEST_14;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_FUNTEST_15: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_FUNCTEST_15;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_CROP_01: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_CROP_01;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xffffffff;
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_CROP_02: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_CROP_02;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xffffffff;
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_CROP_03: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_CROP_03;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xffffffff;
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_CROP_04: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_CROP_04;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xffffffff;
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_CROP_05: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_CROP_05;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xffffffff;
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_CROP_06: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_CROP_06;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xffffffff;
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_CROP_07: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_CROP_01;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xffffffff;
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_CROP_08: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_CROP_02;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xffffffff;
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_CROP_09: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_CROP_03;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xffffffff;
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_CROP_10: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_CROP_04;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xffffffff;
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_CROP_11: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_CROP_05;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xffffffff;
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_CROP_12: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_CROP_06;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xffffffff;
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_CROP_13: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_CROP_01;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xffffffff;
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_CROP_14: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_CROP_02;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xffffffff;
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_CROP_15: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_CROP_03;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xffffffff;
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_CROP_16: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_CROP_04;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xffffffff;
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_CROP_17: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_CROP_05;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xffffffff;
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_CROP_18: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_CROP_06;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xffffffff;
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_IMG_LEN: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_IMG_LEN;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xffffffff;
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_CROP_COOR_XH: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_00;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_CROP_COOR_XL: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_01;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_CROP_COOR_YH: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_02;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_CROP_COOR_YL: 
                ctb->opStatus = ASPOP_STA_NONE;
                ctb->opCode = OP_STLEN_03;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0xff;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_EG_DECT: 
                ctb->opStatus = ASPOP_STA_UPD; /* default enable to test CROP */
                ctb->opCode = OP_EG_DECT;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0x1;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_AP_MODE: 
                ctb->opStatus = ASPOP_STA_APP; //default for debug ASPOP_STA_NONE;
                ctb->opCode = OP_AP_MODEN;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = APM_AP;  /* default ap mode */
                ctb->opMask = ASPOP_MASK_32;
                ctb->opBitlen = 32;
                break;
            case ASPOP_XCROP_GAT: 
                ctb->opStatus = ASPOP_STA_NONE; /* set to ASPOP_STA_SCAN from scanner*/
                ctb->opCode = OP_META_DAT;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0x0;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_XCROP_LINSTR: 
                ctb->opStatus = ASPOP_STA_NONE; /* set to ASPOP_STA_SCAN from scanner*/
                ctb->opCode = OP_META_DAT;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0x0;
                ctb->opMask = ASPOP_MASK_8;
                ctb->opBitlen = 8;
                break;
            case ASPOP_XCROP_LINREC: 
                ctb->opStatus = ASPOP_STA_NONE; /* set to ASPOP_STA_SCAN from scanner*/
                ctb->opCode = OP_META_DAT;
                ctb->opType = ASPOP_TYPE_VALUE;
                ctb->opValue = 0x0;
                ctb->opMask = ASPOP_MASK_16;
                ctb->opBitlen = 16;
                break;
            default: break;
            }
        }
    }
    
    #if 0 /* debug print */
    for (ix = 0; ix < ASPOP_CODE_MAX; ix++) {
        ctb = &pmrs->configTable[ix];
            printf("ctb[%d] 0x%.2x opcode:0x%.2x 0x%.2x val:0x%.2x mask:0x%.2x len:%d \n", ix,
            ctb->opStatus,
            ctb->opCode,
            ctb->opType,
            ctb->opValue,
            ctb->opMask,
            ctb->opBitlen);
    }
    #endif
    
    #if 0 /* manual launch AP mode or Direct mode, will disable if AP mode complete */
    if (arg[1] == 0) {
        /* launch AP  */
        sprintf(syscmd, "/root/script/launchAP_88w8787.sh");
        ret = doSystemCmd(syscmd);
        memset(pmrs->netIntfs, 0, 16);
        sprintf(pmrs->netIntfs, "%s", "uap0");
    } else {
        /* launch wpa connect */
        sprintf(syscmd, "/root/script/iw_con.sh");
        ret = doSystemCmd(syscmd);
        memset(pmrs->netIntfs, 0, 16);
        sprintf(pmrs->netIntfs, "%s", "mlan0");
    }
    #else
    /* read AP config */
    char ssidPath[128] = "/root/scaner/ssid.bin";
    char pskPath[128] = "/root/scaner/psk.bin";
    FILE *fssid=0, *fpsk=0;
    int wfcLen;
    struct apWifiConfig_s *pwfc=0;

    pwfc = &pmrs->wifconf;
    
    fssid = fopen(ssidPath, "r");
    if (fssid) {
        fpsk = fopen(pskPath, "r");
        readLen = 0;
        if (fpsk) {
            ret = fseek(fpsk, 0, SEEK_END);
            if (ret) {
                sprintf(pmrs->log, " file seek failed!! ret:%d \n", ret);
                print_f(&pmrs->plog, "WIFC", pmrs->log);
            } 

            wfcLen = ftell(fpsk);
            sprintf(pmrs->log, " file [%s] size: %d \n", pskPath, wfcLen);
            print_f(&pmrs->plog, "WIFC", pmrs->log);

            ret = fseek(fpsk, 0, SEEK_SET);
            if (ret) {
                sprintf(pmrs->log, " file seek failed!! ret:%d \n", ret);
                print_f(&pmrs->plog, "WIFC", pmrs->log);
            }

            if ((wfcLen >= 8) && (wfcLen <=63)) {
                readLen = fread(pwfc->wfpsk, 1, wfcLen, fpsk);
                pwfc->wfpsk[readLen] = '\0';
                pwfc->wfpskLen = readLen;
            } else {
                sprintf(pmrs->log, " file size error !!! filesize:%d, readLen: %d\n", wfcLen, readLen);
                print_f(&pmrs->plog, "WIFC", pmrs->log);
            }
            
            fclose(fpsk);
        }

        if (readLen > 0) {
            readLen = 0;
            ret = fseek(fssid, 0, SEEK_END);
            if (ret) {
                sprintf(pmrs->log, " file seek failed!! ret:%d \n", ret);
                print_f(&pmrs->plog, "WIFC", pmrs->log);
            } 

            wfcLen = ftell(fssid);
            sprintf(pmrs->log, " file [%s] size: %d \n", ssidPath, wfcLen);
            print_f(&pmrs->plog, "WIFC", pmrs->log);

            ret = fseek(fssid, 0, SEEK_SET);
            if (ret) {
                sprintf(pmrs->log, " file seek failed!! ret:%d \n", ret);
                print_f(&pmrs->plog, "WIFC", pmrs->log);
            }

            if ((wfcLen > 0) && (wfcLen <= 32)) {
                readLen = fread(pwfc->wfssid, 1, wfcLen, fssid);
                pwfc->wfssid[readLen] = '\0';
                pwfc->wfsidLen = readLen;
            } else {
                sprintf(pmrs->log, " file size error !!! filesize:%d, readLen: %d\n", wfcLen, readLen);
                print_f(&pmrs->plog, "WIFC", pmrs->log);
            }            
        }
        fclose(fssid);
    }

    /* AP mode launch or not */
    int isLaunch = 0;
    char s[INET6_ADDRSTRLEN];
    struct ifaddrs *ifaddr, *ifa;
    struct modersp_s tmpModersp;

    ctb = &pmrs->configTable[ASPOP_AP_MODE];
    if (ctb->opCode != OP_AP_MODEN) {        
        sprintf(pmrs->log, " WARNING!!! get wrong opcode value 0x%x", ctb->opCode);
        print_f(&pmrs->plog, "APM", pmrs->log);
    }

    if (ctb->opValue == APM_AP) {
        /* launch wpa connect */
        pwfc = &pmrs->wifconf;
        if ((pwfc->wfpskLen > 0) && (pwfc->wfsidLen > 0)) {
            sprintf(pmrs->log, "launch AP mode ... ssid: \"%s\", psk: \"%s\"\n", pwfc ->wfssid, pwfc->wfpsk);
            print_f(&pmrs->plog, "APM", pmrs->log);

            /* launch wpa connect */
            sprintf(syscmd, "/root/script/wpa_conf.sh \\\"%s\\\" \\\"%s\\\" /etc/wpa_supplicant.conf ", pwfc ->wfssid, pwfc->wfpsk);
            ret = doSystemCmd(syscmd);

            sprintf(syscmd, "wpa_supplicant -B -c /etc/wpa_supplicant.conf -imlan0 -Dnl80211 -dd");
            ret = doSystemCmd(syscmd);

            sleep(3);

            sprintf(syscmd, "udhcpc -i mlan0 -t 3 -n");
            ret = doSystemCmd(syscmd);

            sprintf(pmrs->log, "exec [%s]...\n", syscmd);
            print_f(&pmrs->plog, "APM", pmrs->log);

            memset(pmrs->netIntfs, 0, 16);
            sprintf(pmrs->netIntfs, "%s", "mlan0");

            ret = getifaddrs(&ifaddr);
            if (ret == -1) {
                perror("getifaddrs");        
                //exit(EXIT_FAILURE);    
            } 
            else {
                for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
                    if (ifa->ifa_addr == NULL)
                        continue;

                    ret=getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),s, INET6_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);
                    if (ret != 0) {
                        printf("getnameinfo() failed: %s\n", gai_strerror(ret));
                        //exit(EXIT_FAILURE);
                        continue;
                    }
                    
                    if((strcmp(ifa->ifa_name, pmrs->netIntfs)==0) && (ifa->ifa_addr->sa_family == AF_INET)) {

                        printf("\tInterface : <%s>\n",ifa->ifa_name );
                        printf("\t  Address : <%s>\n", s);

                        sprintf(pmrs->log, "iface: %s addr: %s", pmrs->netIntfs, s);
                        print_f(&pmrs->plog, "APM", pmrs->log);

                        isLaunch = 1;
                    }
                }

                freeifaddrs(ifaddr);
            }
            
            if (!isLaunch) {
                ctb->opValue = APM_DIRECT;
                fs109(pmrs, &tmpModersp);
            }
        } else {
            sprintf(pmrs->log, "failed to launch AP mode, no ssid and psk ...\n");
            print_f(&pmrs->plog, "APM", pmrs->log);
        }
    }
    
    if (!isLaunch) {
        /* launch AP  */
        sprintf(syscmd, "/root/script/clr_all.sh");
        ret = doSystemCmd(syscmd);

        sleep(1);

        sprintf(syscmd, "/root/script/launchAP_88w8787.sh");
        ret = doSystemCmd(syscmd);
        memset(pmrs->netIntfs, 0, 16);
        sprintf(pmrs->netIntfs, "%s", "uap0");
    }
  
    if ((pwfc->wfsidLen > 0) && (pwfc->wfpskLen > 0)) {
        sprintf(pmrs->log, " get ssid: [%s] size: %d, psk: [%s] size: %d\n", pwfc->wfssid, pwfc->wfsidLen, pwfc->wfpsk, pwfc->wfpskLen);
        print_f(&pmrs->plog, "WIFC", pmrs->log);
    } else {
        sprintf(pmrs->log, " ssid and psk are unavilable!!");
        print_f(&pmrs->plog, "WIFC", pmrs->log);
    }    
    #endif

    /* FAT */
    pmrs->aspFat.fatBootsec = (struct sdbootsec_s *)aspSalloc(sizeof(struct sdbootsec_s));
    if (!pmrs->aspFat.fatBootsec) {
        sprintf(pmrs->log, "alloc share memory for FAT boot sector FAIL!!!\n", pmrs->aspFat.fatBootsec); 
        print_f(&pmrs->plog, "FAT", pmrs->log);
        goto end;
    } else {
        sprintf(pmrs->log, "alloc share memory for FAT boot sector DONE [0x%x]!!!\n", pmrs->aspFat.fatBootsec); 
        print_f(&pmrs->plog, "FAT", pmrs->log);
    }
    
    pmrs->aspFat.fatFSinfo = (struct sdFSinfo_s *)aspSalloc(sizeof(struct sdFSinfo_s));
    if (!pmrs->aspFat.fatFSinfo) {
        sprintf(pmrs->log, "alloc share memory for FAT file system info FAIL!!!\n", pmrs->aspFat.fatFSinfo); 
        print_f(&pmrs->plog, "FAT", pmrs->log);
        goto end;
    } else {
        sprintf(pmrs->log, "alloc share memory for FAT file system info DONE [0x%x]!!!\n", pmrs->aspFat.fatFSinfo); 
        print_f(&pmrs->plog, "FAT", pmrs->log);
    }

    pmrs->aspFat.fatTable = (struct sdFATable_s *)aspSalloc(sizeof(struct sdFATable_s));
    if (!pmrs->aspFat.fatTable) {
        sprintf(pmrs->log, "alloc share memory for FAT table FAIL!!!\n", pmrs->aspFat.fatTable); 
        print_f(&pmrs->plog, "FAT", pmrs->log);
        goto end;
    } else {
        sprintf(pmrs->log, "alloc share memory for FAT table DONE [0x%x]!!!\n", pmrs->aspFat.fatTable); 
        print_f(&pmrs->plog, "FAT", pmrs->log);
    }

    pmrs->aspFat.fatDirPool = (struct sdDirPool_s *)aspSalloc(sizeof(struct sdDirPool_s));
    if (!pmrs->aspFat.fatDirPool) {
        sprintf(pmrs->log, "alloc share memory for FAT dir pool FAIL!!!\n", pmrs->aspFat.fatDirPool); 
        print_f(&pmrs->plog, "FAT", pmrs->log);
        goto end;
    } else {
        sprintf(pmrs->log, "alloc share memory for FAT dir pool DONE [0x%x]!!!\n", pmrs->aspFat.fatDirPool); 
        print_f(&pmrs->plog, "FAT", pmrs->log);
    }

    struct sdDirPool_s    *pool;
    pool = pmrs->aspFat.fatDirPool;
    pool->dirPool = (struct directnFile_s *)aspSalloc(sizeof(struct directnFile_s) * DIR_POOL_SIZE);
    if (!pool->dirPool) {
        sprintf(pmrs->log, "alloc share memory for FAT dir pool content FAIL!!!\n", pool->dirPool); 
        print_f(&pmrs->plog, "FAT", pmrs->log);
        goto end;
    } else {
        sprintf(pmrs->log, "alloc share memory for FAT dir pool content DONE [0x%x] , size is %d x %d = %d bytes!!!\n", pool->dirPool, DIR_POOL_SIZE, sizeof(struct directnFile_s),sizeof(struct directnFile_s)*DIR_POOL_SIZE); 
        print_f(&pmrs->plog, "FAT", pmrs->log);
        memset(pool->dirPool, 0, sizeof(struct directnFile_s)*DIR_POOL_SIZE);
    }
    pool->dirMax = DIR_POOL_SIZE;
    pool->dirUsed = 0;
    
    pool->parBuf.dirParseBuff = aspSalloc(8*1024*1024); // 8MB
    if (!pool->parBuf.dirParseBuff) {
        sprintf(pmrs->log, "alloc share memory for FAT dir parsing buffer FAIL!!!\n"); 
        print_f(&pmrs->plog, "FAT", pmrs->log);
        goto end;
    } else {
        sprintf(pmrs->log, "alloc share memory for FAT dir parsing buffer DONE [0x%x] , size is %d!!!\n", pool->parBuf.dirParseBuff, 8*1024*1024); 
        print_f(&pmrs->plog, "FAT", pmrs->log);
    }
    pool->parBuf.dirBuffMax = 8*1024*1024;
    pool->parBuf.dirBuffUsed = 0;

    pmrs->aspFat.fatStatus = ASPFAT_STATUS_INIT;

/*
    ret = mspFS_createRoot(&pmrs->aspFat.fatRootdir, &pmrs->aspFat, dir);
    if (!ret) {
        sprintf(pmrs->log, "FS root [%s] create done, root:0x%x\n", dir, pmrs->aspFat.fatRootdir);
        print_f(&pmrs->plog, "FAT", pmrs->log);
        ret = mspFS_insertChilds(&pmrs->aspFat, pmrs->aspFat.fatRootdir);
        if (!ret) {
            sprintf(pmrs->log, "FS insert ch done\n");
            print_f(&pmrs->plog, "FAT", pmrs->log);
            mspFS_showFolder( pmrs->aspFat.fatRootdir);
        } else {
            sprintf(pmrs->log, "FS insert ch failed\n");
            print_f(&pmrs->plog, "FAT", pmrs->log);
        }
    } else {
        sprintf(pmrs->log, "FS root [%s] create failed ret:%d\n", ret);
        print_f(&pmrs->plog, "FAT", pmrs->log);
    }
*/

// spidev id
    int fd0=0, fd1=0;
#if SPIDEV_SWITCH
    fd0 = open(spidev_1, O_RDWR);
#else
    fd0 = open(spidev_0, O_RDWR);
#endif
    if (fd0 <= 0) {
        sprintf(pmrs->log, "can't open device[%s]\n", spidev_0); 
        print_f(&pmrs->plog, "SPI", pmrs->log);
        goto end;
    } else {
        sprintf(pmrs->log, "open device[%s] id: %d \n", spidev_0, fd0); 
        print_f(&pmrs->plog, "SPI", pmrs->log);
    }
    if (spidev_1) {
#if SPIDEV_SWITCH
        fd1 = open(spidev_0, O_RDWR);
#else
        fd1 = open(spidev_1, O_RDWR);
#endif
        if (fd1 <= 0) {
            sprintf(pmrs->log, "can't open device[%s]\n", spidev_1); 
            print_f(&pmrs->plog, "SPI", pmrs->log);
            fd1 = 0;
        } else {
            sprintf(pmrs->log, "open device[%s] id: %d\n", spidev_1, fd1); 
            print_f(&pmrs->plog, "SPI", pmrs->log);
        }
    } else {
        fd1 = fd0;
    }

    pmrs->sfm[0] = fd0;
    pmrs->sfm[1] = fd1;
    pmrs->smode = 0;
    pmrs->smode |= SPI_MODE_1;

    bitset = 1;
    msp_spi_conf(pmrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(pmrs->log, "Set spi 0 slave ready: %d\n", bitset);
    print_f(&pmrs->plog, "SPI", pmrs->log);
    bitset = 1;
    msp_spi_conf(pmrs->sfm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(pmrs->log, "Set spi 1 slave ready: %d\n", bitset);
    print_f(&pmrs->plog, "SPI", pmrs->log);

    bitset = 8;
    msp_spi_conf(pmrs->sfm[0], _IOW(SPI_IOC_MAGIC, 3, __u8), &bitset);   //SPI_IOC_WR_BITS_PER_WORD    
    msp_spi_conf(pmrs->sfm[1], _IOW(SPI_IOC_MAGIC, 3, __u8), &bitset);   //SPI_IOC_WR_BITS_PER_WORD    

    bitset = 0;     
    msp_spi_conf(pmrs->sfm[0], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL    
    bitset = 1;    
    msp_spi_conf(pmrs->sfm[1], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL

#if 0    
    while (1) {
        bitset = 0;
        msp_spi_conf(pmrs->sfm[0], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
        sprintf(pmrs->log, "wait for RDY become 1, get RDY pin: %d\n", bitset);
        print_f(&pmrs->plog, "SPI", pmrs->log);
        
        //sleep(1);
        
        if (bitset == 1) break;
    }
#endif

    /* set RDY pin to low before spi setup ready */
    bitset = 0;
    ret = msp_spi_conf(pmrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(pmrs->log, "Set RDY low at beginning\n");
    print_f(&pmrs->plog, "SPI", pmrs->log);
    
    /*
     * spi mode 
     */ 
    ret = msp_spi_conf(pmrs->sfm[0], SPI_IOC_WR_MODE, &pmrs->smode);
    if (ret == -1) 
        printf("can't set spi mode\n"); 
    
    ret = msp_spi_conf(pmrs->sfm[0], SPI_IOC_RD_MODE, &pmrs->smode);
    if (ret == -1) 
        printf("can't get spi mode\n"); 
    
    /*
     * spi mode 
     */ 
    ret = msp_spi_conf(pmrs->sfm[1], SPI_IOC_WR_MODE, &pmrs->smode); 
    if (ret == -1) 
        printf("can't set spi mode\n"); 
    
    ret = msp_spi_conf(pmrs->sfm[1], SPI_IOC_RD_MODE, &pmrs->smode);
    if (ret == -1) 
        printf("can't get spi mode\n"); 

// IPC
    pipe(pmrs->pipedn[0].rt);
    //pipe2(pmrs->pipedn[0].rt, O_NONBLOCK);
    pipe(pmrs->pipedn[1].rt);
    pipe(pmrs->pipedn[2].rt);
    pipe(pmrs->pipedn[3].rt);
    pipe(pmrs->pipedn[4].rt);
    pipe(pmrs->pipedn[5].rt);
    //pipe(pmrs->pipedn[6].rt);
    pipe2(pmrs->pipedn[6].rt, O_NONBLOCK);
    pipe(pmrs->pipedn[7].rt);
    pipe(pmrs->pipedn[8].rt);
    pipe(pmrs->pipedn[9].rt);
    
    pipe2(pmrs->pipeup[0].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[1].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[2].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[3].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[4].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[5].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[6].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[7].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[8].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[9].rt, O_NONBLOCK);
    
    res_put_in(&rs[0], pmrs, 0);
    res_put_in(&rs[1], pmrs, 1);
    res_put_in(&rs[2], pmrs, 2);
    res_put_in(&rs[3], pmrs, 3);
    res_put_in(&rs[4], pmrs, 4);
    res_put_in(&rs[5], pmrs, 5);
    res_put_in(&rs[6], pmrs, 6);
    res_put_in(&rs[7], pmrs, 7);
    res_put_in(&rs[8], pmrs, 8);
    res_put_in(&rs[9], pmrs, 9);
    
//  Share memory init
    ring_buf_init(&pmrs->dataRx);
    pmrs->dataRx.r->folw.seq = 1;
    pmrs->dataRx.r->psudo.seq = 1;
    ring_buf_init(&pmrs->dataTx);
    ring_buf_init(&pmrs->cmdRx);
    ring_buf_init(&pmrs->cmdTx);

// fork process
    pmrs->sid[0] = fork();
    if (!pmrs->sid[0]) {
        p1(&rs[0], &rs[6]);
    } else {
        pmrs->sid[1] = fork();
        if (!pmrs->sid[1]) {
            p2(&rs[1]);
        } else {
            pmrs->sid[2] = fork();
            if (!pmrs->sid[2]) {
                p3(&rs[2]);
            } else {
                pmrs->sid[3] = fork();
                if (!pmrs->sid[3]) {
                    p4(&rs[3]);
                } else {                
                    pmrs->sid[4] = fork();
                    if (!pmrs->sid[4]) {
                        p5(&rs[4], &rs[5]);
                    } else { 
                        pmrs->sid[5] = fork();
                        if (!pmrs->sid[5]) {
                            dbg(pmrs);
                        } else {
                            pmrs->sid[6] = fork();
                            if (!pmrs->sid[6]) {
                                p6(&rs[7]);
                            } else {
                                pmrs->sid[7] = fork();
                                if (!pmrs->sid[7]) {
                                    p7(&rs[8]);
                                } else {                
                                    pmrs->sid[8] = fork();
                                    if (!pmrs->sid[8]) {
                                        p8(&rs[9]);
                                    } else {                
                                        p0(pmrs);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    end:

    sprintf(pmrs->log, "something wrong in mothership, break!\n");
    print_f(&pmrs->plog, "main", pmrs->log);
    printf_flush(&pmrs->plog, pmrs->flog);

    return 0;
}

#define MSP_SAVE_LOG (1)

static int print_dbg(struct logPool_s *plog, char *str, int size)
{
    int len, n;
    if(!str) return (-1);

    if (!plog) return (-2);
    
    msync(plog, sizeof(struct logPool_s), MS_SYNC);
    len = strlen(str);
    if (!size) { 
        n = len;
    } else if (len > size) {
        n = size;
    } else {
        n = len;
    }

    if ((len + plog->len) > plog->max) return (-3);
    memcpy(plog->cur, str, n);
    plog->cur += n;
    plog->len += n;
 
    return 0;
}

static int printf_dbgflush(struct logPool_s *plog, struct mainRes_s *mrs)
{
    msync(plog, sizeof(struct logPool_s), MS_SYNC);
    if (plog->cur == plog->pool) return (-1);
    if (plog->len > plog->max) return (-2);

    msync(plog->pool, plog->len, MS_SYNC);
    mrs_ipc_put(mrs, plog->pool, plog->len, 5);
    
#if !MSP_SAVE_LOG
    plog->cur = plog->pool;
    plog->len = 0;
#endif

    return 0;
}

static int print_f(struct logPool_s *plog, char *head, char *str)
{
    int len;
    char ch[1152];
    
#if 0 /* remove log */
    return 0;
#endif

    if (!str) return (-1);

    if (head) {
        //if (!strcmp(head, "P2")) return 0;
        sprintf(ch, "[%s] %s", head, str);
    } else {
        sprintf(ch, "%s", str);
    }

    printf("%s",ch);

#if MSP_SAVE_LOG
    if (!plog) return (-2);
    
    msync(plog, sizeof(struct logPool_s), MS_SYNC);
    len = strlen(ch);
    if ((len + plog->len) > plog->max) return (-3);
    memcpy(plog->cur, ch, strlen(ch));
    plog->cur += len;
    plog->len += len;
#endif
    //if (!mlog) return (-4);
    //fwrite(ch, 1, strlen(ch), mlog);
    //fflush(mlog);
    //fprintf(mlog, "%s", ch);
    
    return 0;
}

static int printf_flush(struct logPool_s *plog, FILE *f) 
{
#if MSP_SAVE_LOG
    msync(plog, sizeof(struct logPool_s), MS_SYNC);
    if (plog->cur == plog->pool) return (-1);
    if (plog->len > plog->max) return (-2);

    msync(plog->pool, plog->len, MS_SYNC);
    fwrite(plog->pool, 1, plog->len, f);
    fflush(f);
    sync();

    plog->cur = plog->pool;
    plog->len = 0;
#endif
    return 0;
}

static char **memory_init(int *sz, int tsize, int csize)
{
    char *mbuf, *tmpB;
    char **pma;
    int asz, idx;
    char mlog[256];
    
    if ((!tsize) || (!csize)) return (0);
    if (tsize % csize) return (0);
    if (!(tsize / csize)) return (0);
        
    asz = tsize / csize;
    //pma = (char **) aspMalloc(sizeof(char *) * asz);
    pma = (char **) aspSalloc(sizeof(char *) * asz);
    
    //sprintf(mlog, "asz:%d pma:0x%.8x\n", asz, pma);
    //print_f(mlogPool, "memory_init", mlog);
    
    mbuf = aspSalloc(tsize);
    
    //sprintf(mlog, "aspSalloc get 0x%.8x\n", mbuf);
    //print_f(mlogPool, "memory_init", mlog);
        
    tmpB = mbuf;
    for (idx = 0; idx < asz; idx++) {
        pma[idx] = mbuf;
        
        //sprintf(mlog, "%d 0x%.8x\n", idx, pma[idx]);
        //print_f(mlogPool, "memory_init", mlog);
        
        mbuf += csize;
    }
    *sz = asz;
    return pma;
}

static int time_diff(struct timespec *s, struct timespec *e, int unit)
{
    unsigned long long cur, tnow, lnow, past, tbef, lpast, gunit;
    int diff;

    gunit = unit;
    //clock_gettime(CLOCK_REALTIME, &curtime);
    cur = s->tv_sec;
    tnow = s->tv_nsec;
    lnow = cur * 1000000000+tnow;
    
    //clock_gettime(CLOCK_REALTIME, &curtime);
    past = e->tv_sec;
    tbef = e->tv_nsec;      
    lpast = past * 1000000000+tbef; 

    if (lpast < lnow) {
        diff = -1;
    } else {
        diff = (lpast - lnow)/gunit;
    }

    return diff;
}

static FILE *find_save(char *dst, char *tmple)
{
    int i;
    FILE *f;
    for (i =0; i < 1000; i++) {
        sprintf(dst, tmple, i);
        f = fopen(dst, "r");
        if (!f) {
            printf("open file [%s]\n", dst);
            break;
        } else {
            //printf("open file [%s] succeed \n", dst);
        }
    }
    f = fopen(dst, "w");
    return f;
}

static int file_save_get(FILE **fp, char *path1)
{
//static char path1[] = "/mnt/mmc2/rx/%d.bin"; 

    char dst[128], temp[128], flog[256];
    FILE *f = NULL;
    int i;

    if (!path1) return (-1);

    sprintf(temp, "%s", path1);

    for (i =0; i < 1000; i++) {
        sprintf(dst, temp, i);
        f = fopen(dst, "r");
        if (!f) {
            sprintf(flog, "open file [%s]", dst);
            print_f(mlogPool, "save", flog);
            break;
        } else
            fclose(f);
    }
    f = fopen(dst, "w");

    *fp = f;
    return 0;
}

static int res_put_in(struct procRes_s *rs, struct mainRes_s *mrs, int idx)
{
    rs->pcmdRx = &mrs->cmdRx;
    rs->pcmdTx = &mrs->cmdTx;
    rs->pdataRx = &mrs->dataRx;
    rs->pdataTx = &mrs->dataTx;
    rs->fs_s = mrs->fs;
    rs->flog_s = mrs->flog;
    rs->tm[0] = &mrs->time[0];
    rs->tm[1] = &mrs->time[1];

    rs->ppipedn = &mrs->pipedn[idx];
    rs->ppipeup = &mrs->pipeup[idx];
    rs->plogs = &mrs->plog;

    if((idx == 0) || (idx == 1)) {
        rs->spifd = mrs->sfm[0];
    } else if (idx == 2) {
        rs->spifd = mrs->sfm[1];
    }

    rs->psocket_r = &mrs->socket_r;
    rs->psocket_t = &mrs->socket_t; 
    rs->psocket_at = &mrs->socket_at;
    rs->psocket_n = &mrs->socket_n;
    rs->psocket_v = &mrs->socket_v;

    rs->pmch = &mrs->mchine;
    rs->pstdata = &mrs->stdata;
    rs->pcfgTable = mrs->configTable;
    rs->psFat = &mrs->aspFat;
    rs->pnetIntfs = mrs->netIntfs;
    rs->pwifconf = &mrs->wifconf;
    rs->pmetaout = mrs->metaout;
    rs->pmetain = mrs->metain;
    rs->pmetaMass = &mrs->metaMass;

    rs->rspioc1 = mrs->spioc1;
    rs->rspioc2 = mrs->spioc2;

    return 0;
}


