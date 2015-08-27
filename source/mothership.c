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

//main()
#define SPI1_ENABLE (1) 
#define SPI_CPHA  0x01          /* clock phase */
#define SPI_CPOL  0x02          /* clock polarity */
#define SPI_MODE_0      (0|0)
#define SPI_MODE_1      (0|SPI_CPHA)
#define SPI_MODE_2      (SPI_CPOL|0)
#define SPI_MODE_3      (SPI_CPOL|SPI_CPHA)
#if SPI1_ENABLE
static char spi1[] = "/dev/spidev32766.0"; 
#else //#if SPI1_ENABLE
static char *spi1 = 0;
#endif //#if SPI1_ENABLE
static char spi0[] = "/dev/spidev32765.0"; 

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

/*
#define OP_DAT 0x08
#define OP_SCM 0x09
#define OP_DUL 0x0a
*/

/* debug */
#define OP_SAVE              0x70
#define OP_ERROR            0xe0

#define SPI_MAX_TXSZ  (1024 * 1024)
#define SPI_TRUNK_SZ   (32768)

#define SPI_KTHREAD_USE    (0) /* can't work, has bug */
#define DIR_POOL_SIZE (20480)
static FILE *mlog = 0;
static struct logPool_s *mlogPool;

typedef int (*func)(int argc, char *argv[]);

typedef enum {
    STINIT = 0,
    WAIT,
    NEXT,
    BREAK,
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
} aspOpSt_e;

typedef enum {
    ASPOP_CODE_NONE = 0,
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
    ASPOP_SDFAT_RD,      /* 16 */
    ASPOP_SDFAT_WT,
    ASPOP_SDFAT_STR01,
    ASPOP_SDFAT_STR02,
    ASPOP_SDFAT_STR03,
    ASPOP_SDFAT_STR04,
    ASPOP_SDFAT_LEN01,
    ASPOP_SDFAT_LEN02,
    ASPOP_SDFAT_LEN03,
    ASPOP_SDFAT_LEN04,
    ASPOP_SDFAT_SDAT,
    ASPOP_REG_RD,
    ASPOP_REG_WT,
    ASPOP_REG_ADDRH,
    ASPOP_REG_ADDRL,
    ASPOP_REG_DAT,
    ASPOP_SUP_SAVE,
    ASPOP_SDFREE_FREESEC,
    ASPOP_SDFREE_STR01,
    ASPOP_SDFREE_STR02,
    ASPOP_SDFREE_STR03,
    ASPOP_SDFREE_STR04,
    ASPOP_SDFREE_LEN01,
    ASPOP_SDFREE_LEN02,
    ASPOP_SDFREE_LEN03,
    ASPOP_SDFREE_LEN04,
    ASPOP_SDUSED_USEDSEC,
    ASPOP_SDUSED_STR01,
    ASPOP_SDUSED_STR02,
    ASPOP_SDUSED_STR03,
    ASPOP_SDUSED_STR04,
    ASPOP_SDUSED_LEN01,
    ASPOP_SDUSED_LEN02,
    ASPOP_SDUSED_LEN03,
    ASPOP_SDUSED_LEN04,
    ASPOP_CODE_MAX, /* 52 */
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
    SUPBACK_RAW,
    SUPBACK_SD,
} supBack_e;

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

struct supdataBack_s{
    struct supdataBack_s   *n;
    int supdataUsed;
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

struct mainRes_s{
    int sid[8];
    int sfm[2];
    int smode;
    struct psdata_s stdata;
    struct sdFAT_s aspFat;
    struct aspConfig_s configTable[ASPOP_CODE_MAX];
    struct folderQueue_s *folder_dirt;
    struct machineCtrl_s mchine;
    // 3 pipe
    struct pipe_s pipedn[9];
    struct pipe_s pipeup[9];
    // data mode share memory
    struct shmem_s dataRx;
    struct shmem_s dataTx; /* we don't have data mode Tx, so use it as cmdRx for spi1 */
    // command mode share memory
    struct shmem_s cmdRx; /* cmdRx for spi0 */
    struct shmem_s cmdTx;
    // file save
    FILE *fs;
    // file log
    FILE *flog;
    // time measurement
    struct timespec time[2];
    // log buffer
    char log[256];
    struct socket_s socket_r;
    struct socket_s socket_t;
    struct socket_s socket_at;
    struct socket_s socket_n;
    struct logPool_s plog;
    struct aspWaitRlt_s wtg;
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
    char logs[256];
    struct socket_s *psocket_r;
    struct socket_s *psocket_t;
    struct socket_s *psocket_at;
    struct socket_s *psocket_n;
    struct logPool_s *plogs;
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

void aspFree(void *p)
{
    printf("  free 0x%.8x \n", p);
    //free(p);
}
void debugPrintBootSec(struct sdbootsec_s *psec)
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

void debugPrintDir(struct directnFile_s *pf)
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


static int cfgValueOffset(int val, int offset)
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
            p = malloc(sizeof(struct aspInfoSplit_s));
            memset(p, 0, sizeof(struct aspInfoSplit_s));            
            cnt++;
        } else {
            p = malloc(sizeof(struct aspInfoSplit_s));
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
        
        p->infoStr = malloc(len+1);
        p->infoLen = len;
        memcpy(p->infoStr, str+cur,  len);

        p->infoStr[len] = '\0';

        sprintf(log, "%d, %s\n", p->infoLen, p->infoStr);
        print_f(mlogPool, "SPLT", log);

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
    raw[14] = tmp32 & 0xff;
    raw[15] = (tmp32 >> 8) & 0xff;

    tmp32 = aspFSdateCps(fs->dfcredate);
    raw[16] = tmp32 & 0xff;
    raw[17] = (tmp32 >> 8) & 0xff;

    tmp32 = aspFSdateCps(fs->dflstacdate);
    raw[18] = tmp32 & 0xff;
    raw[19] = (tmp32 >> 8) & 0xff;

    tmp32 = aspFStimeCps(fs->dfrecotime);
    raw[22] = tmp32 & 0xff;
    raw[23] = (tmp32 >> 8) & 0xff;

    tmp32 = aspFSdateCps(fs->dfrecodate);
    raw[24] = tmp32 & 0xff;
    raw[25] = (tmp32 >> 8) & 0xff;

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

    if (ch == 0x0)  return ch;
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
    end = str + len;
    while (end > str) {
        if (*end == 0x20) {
            space++;
            *end = 0;
        }
        if (*end != 0) break;
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
    d = fst & 0xf; // 0 -4, 5bits
    m = (fst >> 5) & 0xf; // 5 - 8, 4bits
    y = (fst >> 9) & 0x7f; // 9 - 15, 7bits
    val |= (y << 16) | (m << 8) | d;
    return val;
}

static uint32_t aspFSdateCps(uint32_t val)
{
    uint32_t fst=0, y=0, m=0, d=0;

    d = val & 0xf; // 0 -4, 5bits
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
    int leN=0, cnt=0, idx=0, ret = 0;
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
            //printf("\nSHORT file name parsing... [len:%d]\n", fs->dflen);
        }

        cnt = aspFSrmspace(pstN, 11);
        if (cnt == 0) {
            memset(pstN, 0, 16);
            ret = aspNameCpyfromRaw(raw, pstN, 0, 8, 1);
            if (ret != 8) {
                memset(fs, 0x00, sizeof(struct directnFile_s));
                //printf("short name copy error ret:%d \n", ret);
                goto fsparseEnd;
            }
            aspFSrmspace(pstN, 8);
            pstN += strlen(pstN);
            *pstN = '.';
            pstN += 1;
            ret = aspNameCpyfromRaw(raw, pstN, 8, 3, 1);
            if (ret != 3) {
                memset(fs, 0x00, sizeof(struct directnFile_s));
                //printf("short name copy error ret:%d \n", ret);
                goto fsparseEnd;
            }
            aspFSrmspace(pstN, 3);
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
        if (nd != ((ld & 0xf) - 1)) {
            //memset(fs, 0x00, sizeof(struct directnFile_s));
            fs->dfstats = ASPFS_STATUS_DIS;
            return aspRawParseDir(raw, fs, last);
        }
        //printf("LONG file name parsing...\n");
    
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

    newList = (struct adFATLinkList_s *)malloc(sizeof(struct adFATLinkList_s));
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

    printf("   Get next FAT val: %d\n", val);

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

        printf("  compare nxt:%d and cur:%d\n", nxt, cur);    
        
        if (nxt == (cur+1)) {
            cur = nxt;
            nxt = 0;
            llen += 1;
        } else {
            ls->ftStart = lstr;
            ls->ftLen = llen;

            ret = mspSD_createFATLinkList(&ls->n);
            if (ret) return ret;

            printf("  diff nxt:%d, cur:%d str:%d, len:%d\n", nxt, cur, lstr, llen);

            cur = nxt;
            lstr = cur;
            llen = 1; 
            ls = ls->n;
        }
        nxt = mspSD_getNextFAT(cur, fat, max);
        printf("  return nxt:%d\n", nxt);
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
    //r = (struct directnFile_s *) malloc(sizeof(struct directnFile_s));
    if (!r) {
        return (-2);
    }else {
            sprintf(mlog, "alloc root fs done [0x%x]\n", r);
            print_f(mlogPool, "FS", mlog);
    }

    mspFS_allocDir(psFat, &c);
    //c = (struct directnFile_s *) malloc(sizeof(struct directnFile_s));
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
    //r = (struct directnFile_s *) malloc(sizeof(struct directnFile_s));
    if (!r) return (-2);

    mspFS_allocDir(psFat, &c);
    //c = (struct directnFile_s *) malloc(sizeof(struct directnFile_s));
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
    //r = (struct directnFile_s *) malloc(sizeof(struct directnFile_s));
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

    //rmp = malloc(256*256);
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

static uint32_t next_SDAO(struct psdata_s *data)
{
    int pro, rlt, next = 0;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
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
                evt = 0x1; /* jump to next stage */
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
                evt = 0x1; /* jump to next stage */
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
                evt = 0x1; /* jump to next stage */
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
                evt = 0x1; /* jump to next stage */
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
                evt = 0x1; /* jump to next stage */
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
                next = PSACT;
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
                evt = 0x1;
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
                evt = 0x1; /* jump to next stage */
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
                if (tmpAns == 1) {
                    next = PSMAX;
                } else if (tmpAns == 2) {
                    next = PSSET;
                    evt = SUPI; 
                } else if (tmpAns == 3) {
                    next = PSWT;
                    evt = SDAO; 
                }

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
            if (evt == 0x1) nxtst = BULLET; /* state change */
            break;
        case BULLET:
            ret = next_bullet(data);
            evt = (ret >> 24) & 0xff;
            if (evt == 0x1) nxtst = LASER; /* state change */
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
            if (evt == 0x1) nxtst = DOUBLED; /* end the test loop */
            break;
        case DOUBLED:
            ret = next_doubleD(data);
            evt = (ret >> 24) & 0xff;
            if (evt == 0x1) nxtst = REGE; /* end the test loop */
            break;
        case REGE:
            ret = next_registerE(data);
            evt = (ret >> 24) & 0xff;
            if (evt == 0x1) nxtst = REGF; /* end the test loop */
            break;
        case REGF:
            ret = next_registerF(data);
            evt = (ret >> 24) & 0xff;
            if (evt == 0x1) nxtst = FATG; /* end the test loop */
            break;
        case FATG:
            ret = next_FAT32G(data);
            evt = (ret >> 24) & 0xff;
            if (evt == 0x1) nxtst = FATH; /* end the test loop */
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

    sprintf(str, "01 - rlt:0x%x \n", rlt); 
    print_f(mlogPool, "dob", str); 

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

static int stdob_09(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    rlt = abs_result(data->result); 

    //sprintf(str, "wt_01 - rlt:0x%.8x \n", data->result); 
    //print_f(mlogPool, "wt", str); 

    switch (rlt) {
        case STINIT:
            ch = 41; 
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
            } else if (!(pdt->opStatus & ASPOP_STA_APP)) {
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
            } else if (!(pdt->opStatus & ASPOP_STA_APP)) {
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
            } else if (!(pdt->opStatus & ASPOP_STA_APP)) {
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
            } else if (!(pdt->opStatus & ASPOP_STA_APP)) {
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
            } else if (!(pdt->opStatus & ASPOP_STA_APP)) {
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
            } else if (!(pdt->opStatus & ASPOP_STA_APP)) {
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
            } else if (!(pdt->opStatus & ASPOP_STA_APP)) {
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
            } else if (!(pdt->opStatus & ASPOP_STA_APP)) {
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

    sprintf(rs->logs, "op_28 rlt:0x%x \n", rlt); 
    print_f(rs->plogs, "FAT", rs->logs);  

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
                    ch = 89; /*TODO*/
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
                secStr = 0;
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

    sprintf(rs->logs, "op_33 rlt:0x%x \n", rlt); 
    print_f(rs->plogs, "SIG", rs->logs);  

    switch (rlt) {
        case STINIT:
            ch = 25; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(rs->logs, "op_33: result: %x, goto %d\n", data->result, ch); 
            print_f(rs->plogs, "SIG", rs->logs);  
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
    
    rs = data->rs;
    rlt = abs_result(data->result); 
    
    p = &rs->pmch->get;
    c = &rs->pmch->cur;

    //sprintf(rs->logs, "op_34 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SIG", rs->logs);  

    switch (rlt) {
        case STINIT:
            ch = 41; 
            c->opcode =  OP_SINGLE;
            c->data = SINSCAN_WIFI_ONLY;
            memset(p, 0, sizeof(struct info16Bit_s));

            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(rs->logs, "op_34: result: %x, goto %d\n", data->result, ch); 
            print_f(rs->plogs, "SIG", rs->logs);  
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
            sprintf(rs->logs, "op_35: result: %x, goto %d\n", data->result, ch); 
            print_f(rs->plogs, "SIG", rs->logs);  
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
    
    rs = data->rs;
    rlt = abs_result(data->result); 

    //sprintf(rs->logs, "op_36 rlt:0x%x \n", rlt); 
    //print_f(rs->plogs, "SIN", rs->logs);  

    switch (rlt) {
        case STINIT:
            ch = 48; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(rs->logs, "op_36: result: %x, goto %d\n", data->result, ch); 
            print_f(rs->plogs, "SIN", rs->logs);  
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

static int stdow_37(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    
    rs = data->rs;
    rlt = abs_result(data->result); 

    sprintf(rs->logs, "op_37 rlt:0x%x \n", rlt); 
    print_f(rs->plogs, "DOW", rs->logs);  

    switch (rlt) {
        case STINIT:
            ch = 70; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(rs->logs, "op_37: result: 0x%x, goto %d\n", data->result, ch); 
            print_f(rs->plogs, "DOW", rs->logs);  
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

    sprintf(rs->logs, "op_38 rlt:0x%x \n", rlt); 
    print_f(rs->plogs, "UPD", rs->logs);  

    switch (rlt) {
        case STINIT:
            ch = 75; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(rs->logs, "op_38: result: %x, goto %d\n", data->result, ch); 
            print_f(rs->plogs, "UPD", rs->logs);  
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

    sprintf(rs->logs, "op_39 rlt:0x%x \n", rlt); 
    print_f(rs->plogs, "UPD", rs->logs);  

    switch (rlt) {
        case STINIT:
            ch = 0; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(rs->logs, "op_39: result: %x, goto %d\n", data->result, ch); 
            print_f(rs->plogs, "UPD", rs->logs);  
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

    sprintf(rs->logs, "op_40 rlt:0x%x \n", rlt); 
    print_f(rs->plogs, "UPD", rs->logs);  

    switch (rlt) {
        case STINIT:
            ch = 0; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(rs->logs, "op_40: result: %x, goto %d\n", data->result, ch); 
            print_f(rs->plogs, "UPD", rs->logs);  
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
            sprintf(rs->logs, "op_41: result: %x, goto %d\n", data->result, ch); 
            print_f(rs->plogs, "SAV", rs->logs);  
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

    sprintf(rs->logs, "op_52 rlt:0x%x \n", rlt); 
    print_f(rs->plogs, "SDA", rs->logs);  

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
            
            sprintf(rs->logs, "op_61: result: %x, goto %d, fatStatus: %x\n", data->result, ch, pFat->fatStatus); 
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

static int stsda_62(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    
    rs = data->rs;
    rlt = abs_result(data->result); 

    sprintf(rs->logs, "op_62 rlt:0x%x \n", rlt); 
    print_f(rs->plogs, "SDA", rs->logs);  

    switch (rlt) {
        case STINIT:
            ch = 93; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(rs->logs, "op_62: result: %x, goto %d\n", data->result, ch); 
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

static int stwbk_63(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    
    rs = data->rs;
    rlt = abs_result(data->result); 

    sprintf(rs->logs, "op_63 rlt:0x%x \n", rlt); 
    print_f(rs->plogs, "SDWBK", rs->logs);  

    switch (rlt) {
        case STINIT:
            ch = 98; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(rs->logs, "op_63: result: %x, goto %d\n", data->result, ch); 
            print_f(rs->plogs, "SDWBK", rs->logs);  
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

static int stsda_64(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    
    rs = data->rs;
    rlt = abs_result(data->result); 

    sprintf(rs->logs, "op_64 rlt:0x%x \n", rlt); 
    print_f(rs->plogs, "SDA", rs->logs);  

    switch (rlt) {
        case STINIT:
            ch = 0; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(rs->logs, "op_64: result: %x, goto %d\n", data->result, ch); 
            print_f(rs->plogs, "SDA", rs->logs);  
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

static int stsda_65(struct psdata_s *data)
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct procRes_s *rs;
    
    rs = data->rs;
    rlt = abs_result(data->result); 

    sprintf(rs->logs, "op_65 rlt:0x%x \n", rlt); 
    print_f(rs->plogs, "SDA", rs->logs);  

    switch (rlt) {
        case STINIT:
            ch = 0; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(rs->logs, "op_65: result: %x, goto %d\n", data->result, ch); 
            print_f(rs->plogs, "SDA", rs->logs);  
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
    rlt = abs_result(data->result); 
    //sprintf(str, "op_02: rlt: %.8x result: %.8x ans:%d\n", rlt, data->result, data->ansp0); 
    //print_f(mlogPool, "bullet", str);  

    switch (rlt) {
        case STINIT:
            ch = 12; 
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
                pct = data->rs->pcfgTable;
                ret = cfgTableGet(pct, ASPOP_SUP_SAVE, &val);
                if (ret) {
                    sprintf(str, "ASPOP_SUP_SAVE not available, ret:%d\n", ret);  
                    print_f(mlogPool, "laser02", str);  
                } else {
                    sprintf(str, "ASPOP_SUP_SAVE : 0x%.8x \n", val);  
                    print_f(mlogPool, "laser02", str);  
                    if (!data->rs->psFat->fatSupcur) {
                        sprintf(str, "WARNING!!! buffered link list is not allocated!!\n");  
                        print_f(mlogPool, "laser02", str);  
                    } else if (val == SUPBACK_RAW) {
                        data->ansp0 = 2;
                    } else if (val ==SUPBACK_SD) {
                        data->ansp0 = 3;
                    } else {
                        sprintf(str, "WARNING!!! unexpected OP_SUPBACK value: 0x%x \n", val);  
                        print_f(mlogPool, "laser02", str);  
                    }
                }

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

    tlen = size % 1024;
    if (tlen) {
        size = size + 1024 - tlen;
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
    tlen = size % 1024;
    if (tlen) {
        size = size + 1024 - tlen;
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

    sprintf(str, "prod %d %d, %d %d\n", pp->r->lead.run, pp->r->lead.seq, pp->r->dual.run, pp->r->dual.seq);
    print_f(mlogPool, "ring", str);

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

    sprintf(str, "cons d: %d %d/%d/%d \n", dist, leadn, dualn, folwn);
    print_f(mlogPool, "ring", str);

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

    sprintf(str, "cons, d: %d %d/%d - %d\n", dist, leadn, folwn,pp->lastflg);
    print_f(mlogPool, "ring", str);

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

static int msp_spi_conf(int dev, int flag, void *bitset)
{
    char str[128];
    int ret;
    
    if (!dev) {
        sprintf(str, "spi device id == 0\n");
        print_f(mlogPool, "spi-config", str);
        return -1;
    }
    ret = ioctl(dev, flag, bitset);

    return ret;
}

static int mtx_data(int fd, uint8_t *rx_buff, uint8_t *tx_buff, int pksz, struct spi_ioc_transfer *tr)
{
    char mlog[256];
    int ret;

    if (pksz > SPI_MAX_TXSZ) return (-3);

    tr->tx_buf = (unsigned long)tx_buff;
    tr->rx_buf = (unsigned long)rx_buff;
    tr->len = pksz;
    tr->delay_usecs = 0;
    tr->speed_hz = 1000000;
    tr->bits_per_word = 8;

    ret = msp_spi_conf(fd, SPI_IOC_MESSAGE(1), tr);
    if (ret < 0) {
        //sprintf(mlog, "can't send spi message\n");
        //print_f(mlogPool, "SPI", mlog);
    }
    //sprintf(mlog, "tx/rx len: %d\n", ret);
    
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

    sprintf(mrs->log, "wait rlt: %c 0x%.2x\n", ch, ch); 
    print_f(&mrs->plog, "DBG", mrs->log);

    return ret;
}

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

    sprintf(mrs->log, "cmdfunc_upd2host opc:0x%x, dat:0x%x\n", pkt->opcode, pkt->data); 
    print_f(&mrs->plog, "DBG", mrs->log);

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
    sprintf(mrs->log, "1.wt get %c\n", *rlt); 
    print_f(&mrs->plog, "DBG", mrs->log);

    pwt->wtComp = 0;
    n = aspWaitResult(pwt);
    if (n) {
        ret = (n * 10) -3;
        goto end; /* -33 */
    }    
    *rsp = *rlt;

    if (*rlt == 0x1) {
        sprintf(mrs->log, "succeed:"); 
        print_dbg(&mrs->plog, mrs->log, n);
    } else {
        sprintf(mrs->log, "failed:"); 
        print_dbg(&mrs->plog, mrs->log, n);
    }
    sprintf(mrs->log, "2.wt get 0x%x\n", *rlt); 
    print_f(&mrs->plog, "DBG", mrs->log);

    n = mrs_ipc_get(mrs, mrs->log, 256, pwt->wtChan);
    while (n > 0) {
        print_dbg(&mrs->plog, mrs->log, n);
        n = mrs_ipc_get(mrs, mrs->log, 256, pwt->wtChan);
    }

    dt16 = pkg_info(&mrs->mchine.get);
    abs_info(pkt, dt16);
    sprintf(mrs->log, "3.wt get pkt op:0x%x, data:0x%x\n", pkt->opcode, pkt->data); 
    print_f(&mrs->plog, "DBG", mrs->log);
    
end:

    return ret;
}

static int cmdfunc_wt_opcode(int argc, char *argv[])
{
    char *rlt=0, rsp=0;
    int ret=0, ix=0, n=0, err=0, upd=0, brk=0;
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

    struct aspConfig_s* ctb = 0;
    for (ix = 0; ix < ASPOP_CODE_MAX; ix++) {
        ctb = &mrs->configTable[ix];
        if (!ctb) {ret = -5; goto end;}
        if (ctb->opStatus & ASPOP_STA_APP) {
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
        sprintf(mrs->log, "E,%d,%d,%d", err, ret, brk);
    } else {
        sprintf(mrs->log, "D,%d,%d,%d", err, ret, brk);
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
    pkt->data = 0;
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
        
    if ((n) && (rsp != 0x1)) {
         sprintf(mrs->log, "ERROR!!, n=%d rsp=%d opc:0x%x dat:0x%x\n", n, rsp, pkt->opcode, pkt->data); 
         print_f(&mrs->plog, "DBG", mrs->log);
    }

    sprintf(mrs->log, "cmdfunc_boot_opcode n = %d, rsp = %d\n", n, rsp); 
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
            break;
        }
        ctb = 0;
    }

    if (!ctb) {
        sprintf(mrs->log, "cmdfunc_opcode - 3\n"); 
        print_f(&mrs->plog, "DBG", mrs->log);
        ret = -7;
        goto end;
    }

    if (opcode[0] == 0xaa) {
        cd = opcode[3];

        if (cd == ctb->opValue) {
            ctb->opStatus |= ASPOP_STA_APP;
            param = ctb->opValue;
            goto end;
        }

        ret = cmdfunc_opchk_single(cd, ctb->opMask, ctb->opBitlen, ctb->opType);
        if (ret < 0) {
            ret = (ret * 10) -6;
        } else {            
            ctb->opValue = cd;
            ctb->opStatus |= ASPOP_STA_APP;
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
#define CMD_SIZE 15

    int ci, pi, ret, idle=0, wait=-1, loglen=0;
    char cmd[256], *addr[3], rsp[256], ch, *plog;
    char poll[32] = "poll";

    struct cmd_s cmdtab[CMD_SIZE] = {{0, "poll", cmdfunc_01}, {1, "action", cmdfunc_act_opcode}, {2, "rgw", cmdfunc_regw_opcode}, {3, "op", cmdfunc_opcode}, 
                                {4, "wt", cmdfunc_wt_opcode}, {5, "go", cmdfunc_go_opcode}, {6, "rgr", cmdfunc_regr_opcode}, {7, "launch", cmdfunc_lh_opcode},
                                {8, "boot", cmdfunc_boot_opcode}, {9, "single", cmdfunc_single_opcode}, {10, "dnld", cmdfunc_dnld_opcode}, {11, "upld", cmdfunc_upld_opcode},
                                {12, "save", cmdfunc_save_opcode}, {13, "free", cmdfunc_free_opcode}, {14, "used", cmdfunc_used_opcode}};

    p0_init(mrs);

    plog = malloc(2048);
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
    modersp->m = modersp->m + 1;
    return 0; 
}
static int fs02(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    char ch;
    int len;
    sprintf(mrs->log, "check RDY high d:%d\n", modersp->d);
    print_f(&mrs->plog, "fs02", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if ((len > 0) && (ch == 'B')){
        //modersp->m = modersp->m + 1;
        if (modersp->d) {
            modersp->m = modersp->d;
            modersp->d = 0;
            return 2;
        } else {
            modersp->r = 1;
            return 1;
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
        sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        print_f(&mrs->plog, "fs04", mrs->log);

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
        sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        print_f(&mrs->plog, "fs07", mrs->log);

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
        sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        print_f(&mrs->plog, "fs09", mrs->log);

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
    
    sprintf(mrs->log, "set %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    print_f(&mrs->plog, "fs12", mrs->log);
    
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
        sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        print_f(&mrs->plog, "fs13", mrs->log);

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

    sprintf(mrs->log, "set %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    print_f(&mrs->plog, "fs14", mrs->log);
    
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
        sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        print_f(&mrs->plog, "fs15", mrs->log);

        if ((p->opcode == OP_SINGLE) && (p->data == SINSCAN_DUAL_STRM)) {
            modersp->m = modersp->m + 1;
            return 2;
        } else if ((p->opcode == OP_SINGLE) && (p->data == SINSCAN_DUAL_SD)) {
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
    
    ret = mrs_ipc_get(mrs, &ch, 1, 1);
    while (ret > 0) {
        if (ch == 'p') {
            if (sc) {
                len = ring_buf_cons_dual_psudo(&mrs->dataRx, &addr, modersp->v);
                //sprintf(mrs->log, "1. get psudo len:%d, cnt:%d\n", len, modersp->v);
                //print_f(&mrs->plog, "fs18", mrs->log);

                if (len >= 0) {
                    dst = sc->supdataBuff;
                    memcpy(dst, addr, len);
                    sc->supdataUsed = len;

                    s = malloc(sizeof(struct supdataBack_s));
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
        ret = mrs_ipc_get(mrs, &ch, 1, 1);
    }



    ret = mrs_ipc_get(mrs, &ch, 1, 2);
    while (ret > 0) {
        if (ch == 'p') {
            if (sc) {
                len = ring_buf_cons_dual_psudo(&mrs->dataRx, &addr, modersp->v);
                //sprintf(mrs->log, "2. get psudo len:%d, cnt:%d\n", len, modersp->v);
                //print_f(&mrs->plog, "fs18", mrs->log);

                if (len >= 0) {
                    dst = sc->supdataBuff;
                    memcpy(dst, addr, len);
                    sc->supdataUsed = len;

                    s = malloc(sizeof(struct supdataBack_s));
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
        ret = mrs_ipc_get(mrs, &ch, 1, 2);
    }

    if (modersp->r == 0x3) {
        if (sc) {
            len = ring_buf_cons_dual_psudo(&mrs->dataRx, &addr, modersp->v);
            while (len >= 0) {
                sprintf(mrs->log, "3. get psudo len:%d, cnt:%d\n", len, modersp->v);
                print_f(&mrs->plog, "fs18", mrs->log);

                dst = sc->supdataBuff;
                memcpy(dst, addr, len);
                sc->supdataUsed = len;
                
                s = malloc(sizeof(struct supdataBack_s));
                memset(s, 0, sizeof(struct supdataBack_s));
                sc->n = s;
                sc = sc->n;

                pfat->fatSupcur = sc;
                modersp->v += 1;  
                len = ring_buf_cons_dual_psudo(&mrs->dataRx, &addr, modersp->v);
            }

            s = pfat->fatSupdata;
            while (s) {
                if (s->supdataUsed == 0) {
                    break;
                }
                sc = s;
                s = s->n;
            }

            if (s) {
                sc->n = 0;
            }

            while (s) {
                sc = s;
                s = s->n;
                aspFree(sc);
            }
            pfat->fatSupcur = 0;
        }
        mrs_ipc_put(mrs, "D", 1, 3);
        sprintf(mrs->log, "%d end\n", modersp->v);
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

        modersp->d = 0;
        modersp->m = 1;
        return 2;
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
        sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        print_f(&mrs->plog, "fs22", mrs->log);

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

    sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    print_f(&mrs->plog, "fs29", mrs->log);
    
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
        sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        print_f(&mrs->plog, "fs30", mrs->log);

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

    sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    print_f(&mrs->plog, "fs31", mrs->log);
    
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
        sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        print_f(&mrs->plog, "fs32", mrs->log);

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
    return 2;
}

static int fs35(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int ret, bitset;
    char ch;

    //sprintf(mrs->log, "%d\n", modersp->v);
    //print_f(&mrs->plog, "fs35", mrs->log);

    ret = mrs_ipc_get(mrs, &ch, 1, 1);
    while (ret > 0) {
        if (ch == 'p') {
            modersp->v += 1;
            mrs_ipc_put(mrs, "n", 1, 3);
        }

        if (ch == 'd') {
            sprintf(mrs->log, "0 %d end\n", modersp->v);
            print_f(&mrs->plog, "fs35", mrs->log);

            mrs_ipc_put(mrs, "n", 1, 3);
            modersp->r |= 0x1;
            //mrs_ipc_put(mrs, "e", 1, 3);
        }
        ret = mrs_ipc_get(mrs, &ch, 1, 1);
    }

    ret = mrs_ipc_get(mrs, &ch, 1, 2);
    while (ret > 0) {
        if (ch == 'p') {
            modersp->c += 1;
            mrs_ipc_put(mrs, "n", 1, 8);
        }

        if (ch == 'd') {
            sprintf(mrs->log, "1 %d end\n", modersp->v);
            print_f(&mrs->plog, "fs35", mrs->log);
 
            mrs_ipc_put(mrs, "n", 1, 8);
            modersp->r |= 0x2;
            //mrs_ipc_put(mrs, "e", 1, 3);
        }
        ret = mrs_ipc_get(mrs, &ch, 1, 2);
    }

    if (modersp->r == 0x3) {
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
        ring_buf_init(&mrs->cmdRx);
        ring_buf_init(&mrs->cmdTx);

        bitset = 0;
        msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        sprintf(mrs->log, "set RDY pin %d\n",bitset);
        print_f(&mrs->plog, "fs36", mrs->log);
        usleep(60000);
            
        modersp->d = modersp->m + 1;
        modersp->m = 1;
        return 2;
    }

    return 0;
}

static int fs37(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int bitset, ret;

    bitset = 0;
    msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",0, bitset, modersp->d);
    print_f(&mrs->plog, "fs37", mrs->log);

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
        sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        print_f(&mrs->plog, "fs39", mrs->log);

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
        sprintf(mrs->log, "get 0x%.2x/0x%.2x 0x%.2x/0x%.2x\n", p->opcode, c->opcode, p->data, c->data);
        print_f(&mrs->plog, "fs44", mrs->log);

        if (p->opcode == c->opcode) {
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

        bitset = 0;
        msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        sprintf(mrs->log, "set RDY pin %d\n",bitset);
        print_f(&mrs->plog, "fs47", mrs->log);

        usleep(60000);

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
        sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        print_f(&mrs->plog, "fs49", mrs->log);

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

    int val=0, i=0;
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

        debugPrintBootSec(psec);
        pParBuf->dirBuffUsed = 0;

        if (psec->secSize == 512) {
            pfat->fatStatus |= ASPFAT_STATUS_BOOT_SEC;
        }

        modersp->r = 1;
    }else {
        secStr = c->opinfo;
        secLen = p->opinfo;

        sprintf(mrs->log, "buff empty, set str:%d, len:%d \n", secStr, secLen);
        print_f(&mrs->plog, "fs50", mrs->log);

        cfgTableSet(pct, ASPOP_SDFAT_RD, 1);

        val = cfgValueOffset(secStr, 0);
        cfgTableSet(pct, ASPOP_SDFAT_STR01, val);
        val = cfgValueOffset(secStr, 8);
        cfgTableSet(pct, ASPOP_SDFAT_STR02, val);
        val = cfgValueOffset(secStr, 16);
        cfgTableSet(pct, ASPOP_SDFAT_STR03, val);
        val = cfgValueOffset(secStr, 24);
        cfgTableSet(pct, ASPOP_SDFAT_STR04, val);
        val = cfgValueOffset(secLen, 0);
        cfgTableSet(pct, ASPOP_SDFAT_LEN01, val);
        val = cfgValueOffset(secLen, 8);
        cfgTableSet(pct, ASPOP_SDFAT_LEN02, val);
        val = cfgValueOffset(secLen, 16);
        cfgTableSet(pct, ASPOP_SDFAT_LEN03, val);
        val = cfgValueOffset(secLen, 24);
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
                    pfdirt = malloc(sizeof(struct folderQueue_s));
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

        val = cfgValueOffset(secStr, 0);
        cfgTableSet(pct, ASPOP_SDFAT_STR01, val);
        val = cfgValueOffset(secStr, 8);
        cfgTableSet(pct, ASPOP_SDFAT_STR02, val);
        val = cfgValueOffset(secStr, 16);
        cfgTableSet(pct, ASPOP_SDFAT_STR03, val);
        val = cfgValueOffset(secStr, 24);
        cfgTableSet(pct, ASPOP_SDFAT_STR04, val);
        val = cfgValueOffset(secLen, 0);
        cfgTableSet(pct, ASPOP_SDFAT_LEN01, val);
        val = cfgValueOffset(secLen, 8);
        cfgTableSet(pct, ASPOP_SDFAT_LEN02, val);
        val = cfgValueOffset(secLen, 16);
        cfgTableSet(pct, ASPOP_SDFAT_LEN03, val);
        val = cfgValueOffset(secLen, 24);
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

                        pfdirt = malloc(sizeof(struct folderQueue_s));
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

        val = cfgValueOffset(secStr, 0);
        cfgTableSet(pct, ASPOP_SDFAT_STR01, val);
        val = cfgValueOffset(secStr, 8);
        cfgTableSet(pct, ASPOP_SDFAT_STR02, val);
        val = cfgValueOffset(secStr, 16);
        cfgTableSet(pct, ASPOP_SDFAT_STR03, val);
        val = cfgValueOffset(secStr, 24);
        cfgTableSet(pct, ASPOP_SDFAT_STR04, val);
        val = cfgValueOffset(secLen, 0);
        cfgTableSet(pct, ASPOP_SDFAT_LEN01, val);
        val = cfgValueOffset(secLen, 8);
        cfgTableSet(pct, ASPOP_SDFAT_LEN02, val);
        val = cfgValueOffset(secLen, 16);
        cfgTableSet(pct, ASPOP_SDFAT_LEN03, val);
        val = cfgValueOffset(secLen, 24);
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

        val = cfgValueOffset(secStr, 0);
        cfgTableSet(pct, ASPOP_SDFAT_STR01, val);
        val = cfgValueOffset(secStr, 8);
        cfgTableSet(pct, ASPOP_SDFAT_STR02, val);
        val = cfgValueOffset(secStr, 16);
        cfgTableSet(pct, ASPOP_SDFAT_STR03, val);
        val = cfgValueOffset(secStr, 24);
        cfgTableSet(pct, ASPOP_SDFAT_STR04, val);
        val = cfgValueOffset(secLen, 0);
        cfgTableSet(pct, ASPOP_SDFAT_LEN01, val);
        val = cfgValueOffset(secLen, 8);
        cfgTableSet(pct, ASPOP_SDFAT_LEN02, val);
        val = cfgValueOffset(secLen, 16);
        cfgTableSet(pct, ASPOP_SDFAT_LEN03, val);
        val = cfgValueOffset(secLen, 24);
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

    sprintf(mrs->log, "read FAT  \n");
    print_f(&mrs->plog, "fs55", mrs->log);

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
                //pftb->ftbFat1 = malloc(val);
                pftb->ftbFat1 = mmap(NULL, val, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);;
                if (!pftb->ftbFat1) {
                    sprintf(mrs->log, "malloc for FAT table FAIL!! \n");
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
                    sprintf(mrs->log, "%d get fat len:%d, total:%d\n", pi, len, pftb->ftbLen);
                    print_f(&mrs->plog, "fs55", mrs->log);
                }
            } else {
                sprintf(mrs->log, "end, len:%d\n", len);
                print_f(&mrs->plog, "fs55", mrs->log);
            }

            if (ch == 'd') {
                sprintf(mrs->log, "spi0 %d end\n", modersp->v);
                print_f(&mrs->plog, "fs55", mrs->log);
                
                bitset = 0;
                msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
                sprintf(mrs->log, "set RDY pin %d\n",bitset);
                print_f(&mrs->plog, "fs55", mrs->log);

#if SPI_KTHREAD_USE
                bitset = 0;
                ret = msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
                sprintf(mrs->log, "Stop spi0 spidev thread, ret: 0x%x\n", ret);
                print_f(&mrs->plog, "fs55", mrs->log);
#endif
                usleep(60000);

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

    modersp->r = 0xed;    
    return 1;
}

static int fs57(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    struct info16Bit_s *p;
    p = &mrs->mchine.cur;

    p->opcode = OP_SUPBACK;
    p->data = 0;

    sprintf(mrs->log, "set opcode OP_SUPBACK: 0x%.2x 0x%.2x \n", p->opcode, p->data);
    print_f(&mrs->plog, "fs57", mrs->log);
    
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
        sprintf(mrs->log, "get opcode 0x%.2x 0x%.2x \n", p->opcode, p->data);
        print_f(&mrs->plog, "fs58", mrs->log);

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
    uint32_t val=0;
    struct supdataBack_s *s=0;
    struct aspConfig_s *pct=0;
    struct sdFAT_s *pfat=0;

    pct = mrs->configTable;
    pfat = &mrs->aspFat;

    sprintf(mrs->log, "initial the fatSupdata !!!  \n");
    print_f(&mrs->plog, "fs59", mrs->log);

    s = malloc(sizeof(struct supdataBack_s));
    if (!s) {
        sprintf(mrs->log, "FAIL to initial the fatSupdata !!! \n");
        print_f(&mrs->plog, "fs59", mrs->log);

        modersp->r = 2;
        return 1;
    }

    //cfgTableSet(pct, ASPOP_SUP_SAVE, (uint32_t)s);
    pfat->fatSupdata = s;
    pfat->fatSupcur = pfat->fatSupdata;

end:

    if (modersp->d) {
        modersp->m = modersp->d;
        modersp->d = 0;
        return 2;
    } else {
        modersp->r = 2;
        return 1;
    }
}

static int fs60(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    struct info16Bit_s *p;
    p = &mrs->mchine.cur;

    p->opcode = OP_SUPBACK;
    p->data = 0;

    sprintf(mrs->log, "set opcode OP_SUPBACK: 0x%.2x 0x%.2x \n", p->opcode, p->data);
    print_f(&mrs->plog, "fs60", mrs->log);
    
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
        sprintf(mrs->log, "get opcode 0x%.2x 0x%.2x \n", p->opcode, p->data);
        print_f(&mrs->plog, "fs61", mrs->log);

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

    sprintf(mrs->log, "set opcode OP_SUPBACK: 0x%.2x 0x%.2x \n", p->opcode, p->data);
    print_f(&mrs->plog, "fs62", mrs->log);
    
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
        sprintf(mrs->log, "get opcode 0x%.2x 0x%.2x \n", p->opcode, p->data);
        print_f(&mrs->plog, "fs63", mrs->log);

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
        
        if (sh->supdataUsed < len) {
            len = sh->supdataUsed;
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
        
            bitset = 0;
            msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
            sprintf(mrs->log, "set RDY pin %d\n",bitset);
            print_f(&mrs->plog, "fs66", mrs->log);

#if SPI_KTHREAD_USE
            bitset = 0;
            ret = msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
            sprintf(mrs->log, "Stop spi0 spidev thread, ret: 0x%x\n", ret);
            print_f(&mrs->plog, "fs66", mrs->log);
#endif
            usleep(60000);

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

    modersp->m = modersp->m + 1;
    return 2;
}
static int fs68(struct mainRes_s *mrs, struct modersp_s *modersp) 
{ 
    int ret, bitset;
    char ch;

    //sprintf(mrs->log, "%d\n", modersp->v);
    //print_f(&mrs->plog, "fs68", mrs->log);

    ret = mrs_ipc_get(mrs, &ch, 1, 1);
    while (ret > 0) {
        if (ch == 'p') {
            modersp->v += 1;
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
        mrs_ipc_put(mrs, "N", 1, 3);
        sprintf(mrs->log, "%d end\n", modersp->v);
        print_f(&mrs->plog, "fs68", mrs->log);
        modersp->m = modersp->m + 1;
        return 2;
    }

    return 0; 
}
static int fs69(struct mainRes_s *mrs, struct modersp_s *modersp) 
{ 
    int len=0, bitset=0, ret=0;
    char ch=0;
    struct info16Bit_s *p;

    //sprintf(mrs->log, "wait spi0 tx end\n");
    //print_f(&mrs->plog, "fs66", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if (len > 0) {

        sprintf(mrs->log, "ch: %c - end\n", ch);
        print_f(&mrs->plog, "fs69", mrs->log);

        if (ch == 'N') {

            ring_buf_init(&mrs->cmdRx);

#if SPI_KTHREAD_USE
            bitset = 0;
            ret = msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
            sprintf(mrs->log, "Stop spi0 spidev thread, ret: 0x%x\n", ret);
            print_f(&mrs->plog, "fs69", mrs->log);
#endif

            bitset = 0;
            msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
            sprintf(mrs->log, "set RDY pin %d\n",bitset);
            print_f(&mrs->plog, "fs69", mrs->log);
            usleep(60000);

            modersp->r = 1;            
            return 1;
        } else  {
            modersp->r = 2;
            return 1;
        }
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

        val = cfgValueOffset(secStr, 0);
        cfgTableSet(pct, ASPOP_SDFAT_STR01, val);
        val = cfgValueOffset(secStr, 8);
        cfgTableSet(pct, ASPOP_SDFAT_STR02, val);
        val = cfgValueOffset(secStr, 16);
        cfgTableSet(pct, ASPOP_SDFAT_STR03, val);
        val = cfgValueOffset(secStr, 24);
        cfgTableSet(pct, ASPOP_SDFAT_STR04, val);
        val = cfgValueOffset(secLen, 0);
        cfgTableSet(pct, ASPOP_SDFAT_LEN01, val);
        val = cfgValueOffset(secLen, 8);
        cfgTableSet(pct, ASPOP_SDFAT_LEN02, val);
        val = cfgValueOffset(secLen, 16);
        cfgTableSet(pct, ASPOP_SDFAT_LEN03, val);
        val = cfgValueOffset(secLen, 24);
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
    int len=0, bitset=0, ret=0;
    char ch=0;
    struct info16Bit_s *p;

    sprintf(mrs->log, "wait spi0 tx end\n");
    print_f(&mrs->plog, "fs74", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if (len > 0) {

        sprintf(mrs->log, "ch: %c - end\n", ch);
        print_f(&mrs->plog, "fs74", mrs->log);

        if (ch == 'N') {

            ring_buf_init(&mrs->cmdRx);

#if SPI_KTHREAD_USE
            bitset = 0;
            ret = msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
            sprintf(mrs->log, "Stop spi0 spidev thread, ret: 0x%x\n", ret);
            print_f(&mrs->plog, "fs74", mrs->log);
#endif

            bitset = 0;
            msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
            sprintf(mrs->log, "set RDY pin %d\n",bitset);
            print_f(&mrs->plog, "fs74", mrs->log);
            usleep(60000);

            modersp->m = 48;            
            return 2;
        } else  {
            modersp->r = 2;
            return 1;
        }
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
        sprintf(mrs->log, "FAT table parsing for root dictionary FAIL!! pending!! \n", ret);
        print_f(&mrs->plog, "fs75", mrs->log);
        modersp->r = 2;
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

        val = cfgValueOffset(secStr, 0);
        cfgTableSet(pct, ASPOP_SDFAT_STR01, val);
        val = cfgValueOffset(secStr, 8);
        cfgTableSet(pct, ASPOP_SDFAT_STR02, val);
        val = cfgValueOffset(secStr, 16);
        cfgTableSet(pct, ASPOP_SDFAT_STR03, val);
        val = cfgValueOffset(secStr, 24);
        cfgTableSet(pct, ASPOP_SDFAT_STR04, val);
        val = cfgValueOffset(secLen, 0);
        cfgTableSet(pct, ASPOP_SDFAT_LEN01, val);
        val = cfgValueOffset(secLen, 8);
        cfgTableSet(pct, ASPOP_SDFAT_LEN02, val);
        val = cfgValueOffset(secLen, 16);
        cfgTableSet(pct, ASPOP_SDFAT_LEN03, val);
        val = cfgValueOffset(secLen, 24);
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

#if SPI_KTHREAD_USE
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

#if SPI_KTHREAD_USE
            bitset = 0;
            ret = msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
            sprintf(mrs->log, "Stop spi0 spidev thread, ret: 0x%x\n", ret);
            print_f(&mrs->plog, "fs79", mrs->log);
#endif

            bitset = 0;
            msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
            sprintf(mrs->log, "set RDY pin %d\n",bitset);
            print_f(&mrs->plog, "fs79", mrs->log);
            usleep(60000);

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

        val = cfgValueOffset(secStr, 0);
        cfgTableSet(pct, ASPOP_SDFAT_STR01, val);
        val = cfgValueOffset(secStr, 8);
        cfgTableSet(pct, ASPOP_SDFAT_STR02, val);
        val = cfgValueOffset(secStr, 16);
        cfgTableSet(pct, ASPOP_SDFAT_STR03, val);
        val = cfgValueOffset(secStr, 24);
        cfgTableSet(pct, ASPOP_SDFAT_STR04, val);
        val = cfgValueOffset(secLen, 0);
        cfgTableSet(pct, ASPOP_SDFAT_LEN01, val);
        val = cfgValueOffset(secLen, 8);
        cfgTableSet(pct, ASPOP_SDFAT_LEN02, val);
        val = cfgValueOffset(secLen, 16);
        cfgTableSet(pct, ASPOP_SDFAT_LEN03, val);
        val = cfgValueOffset(secLen, 24);
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
    char clstPath[128] = "/mnt/mmc2/clstnew.bin";
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
    
    sprintf(mrs->log, "DFE read from SD (0x%x)\n", pfat->fatFileUpld);
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
                sprintf(mrs->log, "FAT table save to [%s] size:%d\n", clstPath, len);
                print_f(&mrs->plog, "fs81", mrs->log);
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

                sprintf(mrs->log, "folder allocate new cluster for file: %s \n", curDir->dfSFN);
                print_f(&mrs->plog, "fs81", mrs->log);

                if (!pftb->h) {
                    padd = 0;

                    pnxf = 0;
                    ret = mspSD_allocFreeFATList(&padd, 1, pfre, &pnxf);
                    if (ret) {
                        sprintf(mrs->log, "free FAT table parsing for file upload FAIL!!ret:%d (%s)\n", ret, curDir->dfSFN);
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

                                sprintf(mrs->log, "free used FREE FAT linklist, 0x%.8x start: %d, length: %d \n", pclst, pclst->ftStart, pclst->ftLen);
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
        pfdirt = malloc(sizeof(struct folderQueue_s));
        pfdirt->fdObj = pa;
        pfdirt->fdnxt = 0;

        if (!pftb->h) {
            pflsh = 0;

            ret = mspSD_parseFAT2LinkList(&pflsh, pa->dfclstnum, pftb->ftbFat1, (psec->secTotal - psec->secWhroot) / psec->secPrClst);
            if (ret) {
                sprintf(mrs->log, "FAT table parsing for root dictionary FAIL!!ret:%d (%s)\n", ret, pa->dfSFN);
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
            sprintf(mrs->log, "FAT link list head should be zero, str:%d len:%d\n", pflnt->ftStart, pflnt->ftLen);
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

        val = cfgValueOffset(secStr, 0);
        cfgTableSet(pct, ASPOP_SDFAT_STR01, val);
        val = cfgValueOffset(secStr, 8);
        cfgTableSet(pct, ASPOP_SDFAT_STR02, val);
        val = cfgValueOffset(secStr, 16);
        cfgTableSet(pct, ASPOP_SDFAT_STR03, val);
        val = cfgValueOffset(secStr, 24);
        cfgTableSet(pct, ASPOP_SDFAT_STR04, val);
        val = cfgValueOffset(secLen, 0);
        cfgTableSet(pct, ASPOP_SDFAT_LEN01, val);
        val = cfgValueOffset(secLen, 8);
        cfgTableSet(pct, ASPOP_SDFAT_LEN02, val);
        val = cfgValueOffset(secLen, 16);
        cfgTableSet(pct, ASPOP_SDFAT_LEN03, val);
        val = cfgValueOffset(secLen, 24);
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

#if SPI_KTHREAD_USE
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

#if SPI_KTHREAD_USE
            bitset = 0;
            ret = msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
            sprintf(mrs->log, "Stop spi0 spidev thread, ret: 0x%x\n", ret);
            print_f(&mrs->plog, "fs83", mrs->log);
#endif

            bitset = 0;
            msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
            sprintf(mrs->log, "set RDY pin %d\n",bitset);
            print_f(&mrs->plog, "fs83", mrs->log);
            usleep(60000);

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

    sprintf(mrs->log, "set opcode OP_SAVE: 0x%.2x 0x%.2x \n", p->opcode, p->data);
    print_f(&mrs->plog, "fs84", mrs->log);
    
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
        sprintf(mrs->log, "get opcode 0x%.2x 0x%.2x \n", p->opcode, p->data);
        print_f(&mrs->plog, "fs85", mrs->log);

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

    sprintf(mrs->log, "set opcode OP_SAVE: 0x%.2x 0x%.2x \n", p->opcode, p->data);
    print_f(&mrs->plog, "fs86", mrs->log);
    
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
        sprintf(mrs->log, "get opcode 0x%.2x 0x%.2x \n", p->opcode, p->data);
        print_f(&mrs->plog, "fs87", mrs->log);

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
    char clstPath[128] = "/mnt/mmc2/clstnew.bin";
    
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

        pr = malloc(len);
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

        val = cfgValueOffset(secStr, 0);
        cfgTableSet(pct, ASPOP_SDFAT_STR01, val);
        val = cfgValueOffset(secStr, 8);
        cfgTableSet(pct, ASPOP_SDFAT_STR02, val);
        val = cfgValueOffset(secStr, 16);
        cfgTableSet(pct, ASPOP_SDFAT_STR03, val);
        val = cfgValueOffset(secStr, 24);
        cfgTableSet(pct, ASPOP_SDFAT_STR04, val);
        val = cfgValueOffset(secLen, 0);
        cfgTableSet(pct, ASPOP_SDFAT_LEN01, val);
        val = cfgValueOffset(secLen, 8);
        cfgTableSet(pct, ASPOP_SDFAT_LEN02, val);
        val = cfgValueOffset(secLen, 16);
        cfgTableSet(pct, ASPOP_SDFAT_LEN03, val);
        val = cfgValueOffset(secLen, 24);
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

            val = cfgValueOffset(secStr, 0);
            cfgTableSet(pct, ASPOP_SDFAT_STR01, val);
            val = cfgValueOffset(secStr, 8);
            cfgTableSet(pct, ASPOP_SDFAT_STR02, val);
            val = cfgValueOffset(secStr, 16);
            cfgTableSet(pct, ASPOP_SDFAT_STR03, val);
            val = cfgValueOffset(secStr, 24);
            cfgTableSet(pct, ASPOP_SDFAT_STR04, val);
            val = cfgValueOffset(secLen, 0);
            cfgTableSet(pct, ASPOP_SDFAT_LEN01, val);
            val = cfgValueOffset(secLen, 8);
            cfgTableSet(pct, ASPOP_SDFAT_LEN02, val);
            val = cfgValueOffset(secLen, 16);
            cfgTableSet(pct, ASPOP_SDFAT_LEN03, val);
            val = cfgValueOffset(secLen, 24);
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

#if SPI_KTHREAD_USE
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

#if SPI_KTHREAD_USE
            bitset = 0;
            ret = msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
            sprintf(mrs->log, "Stop spi0 spidev thread, ret: 0x%x\n", ret);
            print_f(&mrs->plog, "fs90", mrs->log);
#endif

            bitset = 0;
            msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
            sprintf(mrs->log, "set RDY pin %d\n",bitset);
            print_f(&mrs->plog, "fs90", mrs->log);
            usleep(60000);

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

    val = cfgValueOffset(secStr, 0);
    cfgTableSet(pct, ASPOP_SDFREE_STR01, val);
    val = cfgValueOffset(secStr, 8);
    cfgTableSet(pct, ASPOP_SDFREE_STR02, val);
    val = cfgValueOffset(secStr, 16);
    cfgTableSet(pct, ASPOP_SDFREE_STR03, val);
    val = cfgValueOffset(secStr, 24);
    cfgTableSet(pct, ASPOP_SDFREE_STR04, val);
    val = cfgValueOffset(secLen, 0);
    cfgTableSet(pct, ASPOP_SDFREE_LEN01, val);
    val = cfgValueOffset(secLen, 8);
    cfgTableSet(pct, ASPOP_SDFREE_LEN02, val);
    val = cfgValueOffset(secLen, 16);
    cfgTableSet(pct, ASPOP_SDFREE_LEN03, val);
    val = cfgValueOffset(secLen, 24);
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

    val = cfgValueOffset(secStr, 0);
    cfgTableSet(pct, ASPOP_SDUSED_STR01, val);
    val = cfgValueOffset(secStr, 8);
    cfgTableSet(pct, ASPOP_SDUSED_STR02, val);
    val = cfgValueOffset(secStr, 16);
    cfgTableSet(pct, ASPOP_SDUSED_STR03, val);
    val = cfgValueOffset(secStr, 24);
    cfgTableSet(pct, ASPOP_SDUSED_STR04, val);
    val = cfgValueOffset(secLen, 0);
    cfgTableSet(pct, ASPOP_SDUSED_LEN01, val);
    val = cfgValueOffset(secLen, 8);
    cfgTableSet(pct, ASPOP_SDUSED_LEN02, val);
    val = cfgValueOffset(secLen, 16);
    cfgTableSet(pct, ASPOP_SDUSED_LEN03, val);
    val = cfgValueOffset(secLen, 24);
    cfgTableSet(pct, ASPOP_SDUSED_LEN04, val);

    modersp->r = 1;

    return 1;
}

static int fs93(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    char fnameSave[16] = "ASPA%.4d.jpg";
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
    b32 |= val ;

    ret += cfgTableGet(pct, ASPOP_SDUSED_STR02, &val);
    b32 |= val << 8;
    
    ret += cfgTableGet(pct, ASPOP_SDUSED_STR03, &val);
    b32 |= val << 16;
    
    ret += cfgTableGet(pct, ASPOP_SDUSED_STR04, &val);
    b32 |= val << 24;
    secStr = b32;

    b32 = 0;
    ret += cfgTableGet(pct, ASPOP_SDUSED_LEN01, &val);
    b32 |= val;
    
    ret += cfgTableGet(pct, ASPOP_SDUSED_LEN02, &val);
    b32 |= val << 8;
    
    ret += cfgTableGet(pct, ASPOP_SDUSED_LEN03, &val);
    b32 |= val << 16;
    
    ret += cfgTableGet(pct, ASPOP_SDUSED_LEN04, &val);
    b32 |= val << 24;
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
    upld->dfrecotime = (((atime[0]  << 16)&0xff) | ((atime[1]&0xff) << 8) | (atime[2]&0xff));

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

        val = cfgValueOffset(secStr, 0);
        cfgTableSet(pct, ASPOP_SDFAT_STR01, val);
        val = cfgValueOffset(secStr, 8);
        cfgTableSet(pct, ASPOP_SDFAT_STR02, val);
        val = cfgValueOffset(secStr, 16);
        cfgTableSet(pct, ASPOP_SDFAT_STR03, val);
        val = cfgValueOffset(secStr, 24);
        cfgTableSet(pct, ASPOP_SDFAT_STR04, val);
        val = cfgValueOffset(secLen, 0);
        cfgTableSet(pct, ASPOP_SDFAT_LEN01, val);
        val = cfgValueOffset(secLen, 8);
        cfgTableSet(pct, ASPOP_SDFAT_LEN02, val);
        val = cfgValueOffset(secLen, 16);
        cfgTableSet(pct, ASPOP_SDFAT_LEN03, val);
        val = cfgValueOffset(secLen, 24);
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
    sprintf(mrs->log, "data flow upload to SD\n");
    print_f(&mrs->plog, "fs77", mrs->log);

    sprintf(mrs->log, "trigger spi0\n");
    print_f(&mrs->plog, "fs95", mrs->log);

#if SPI_KTHREAD_USE
    bitset = 0;
    ret = msp_spi_conf(mrs->sfm[0], _IOR(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_START_THREAD
    sprintf(mrs->log, "Start spi0 spidev thread, ret: 0x%x\n", ret);
    print_f(&mrs->plog, "fs95", mrs->log);
#endif

    ring_buf_init(&mrs->cmdTx);

    mrs_ipc_put(mrs, "u", 1, 3);
    clock_gettime(CLOCK_REALTIME, &mrs->time[0]);
    mrs_ipc_put(mrs, "u", 1, 8);
            
    modersp->m = modersp->m + 1;
    return 2;
}

static int fs96(struct mainRes_s *mrs, struct modersp_s *modersp) 
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
            print_f(&mrs->plog, "fs96", mrs->log);

            mrs_ipc_put(mrs, "U", 1, 1);

            mrs_ipc_put(mrs, "U", 1, 8);
            
            modersp->r |= 0x1;
        }
        ret = mrs_ipc_get(mrs, &ch, 1, 3);
    }

    if (modersp->r & 0x1) {
        sprintf(mrs->log, "%d end\n", modersp->v);
        print_f(&mrs->plog, "fs96", mrs->log);
        modersp->m = modersp->m + 1;
        return 2;
    }

    return 0; 
}

static int fs97(struct mainRes_s *mrs, struct modersp_s *modersp) 
{ 
    int len=0, bitset=0, ret=0;
    char ch=0;
    struct info16Bit_s *p;

    //sprintf(mrs->log, "wait spi0 tx end\n");
    //print_f(&mrs->plog, "fs79", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if (len > 0) {

        sprintf(mrs->log, "ch: %c - end\n", ch);
        print_f(&mrs->plog, "fs97", mrs->log);

        if (ch == 'U') {

            ring_buf_init(&mrs->cmdTx);

#if SPI_KTHREAD_USE
            bitset = 0;
            ret = msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
            sprintf(mrs->log, "Stop spi0 spidev thread, ret: 0x%x\n", ret);
            print_f(&mrs->plog, "fs97", mrs->log);
#endif

            bitset = 0;
            msp_spi_conf(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
            sprintf(mrs->log, "set RDY pin %d\n",bitset);
            print_f(&mrs->plog, "fs97", mrs->log);
            usleep(60000);

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
        print_f(&mrs->plog, "fs98", mrs->log);

        modersp->r = 3;
        return 1;
    }

    pfre = pftb->ftbMng.f;
    if (!pfre) {
        sprintf(mrs->log, "Error!! free space link list is empty \n");
        print_f(&mrs->plog, "fs98", mrs->log);
        modersp->r = 0xed;
        return 1;
    }
    
    if (!pfat->fatFileUpld) {
        modersp->r = 0xed;
        return 1;
    }    

    sprintf(mrs->log, "upload file: %s \n", pfat->fatFileUpld->dfSFN);
    print_f(&mrs->plog, "fs98", mrs->log);
   
    curDir = pfat->fatFileUpld;
    if (!pftb->h) {
        pflsh = 0;

        if (curDir->dflength % clstByte) {
            clstLen = (curDir->dflength / clstByte) + 1;
        } else {
            clstLen = (curDir->dflength / clstByte);        
        }

        sprintf(mrs->log, "needed cluster length: %d \n", clstLen);
        print_f(&mrs->plog, "fs98", mrs->log);
        
        if (clstLen) {
            ret = mspSD_allocFreeFATList(&pflsh, clstLen, pfre, &pnxf);
            if (ret) {
                sprintf(mrs->log, "free FAT table parsing for file upload FAIL!!ret:%d (%s)\n", ret, curDir->dfSFN);
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

                sprintf(mrs->log, " re-calculate total free cluster: %d \n free sector: %d (size: %d) \n", freeClst, freeClst * psec->secPrClst, freeClst * psec->secPrClst * psec->secSize);
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

        pfat->fatStatus |= ASPFAT_STATUS_DFECHK;
        pfat->fatStatus |= ASPFAT_STATUS_DFEWT;
        modersp->r = 1;
    } else {
        sprintf(mrs->log, "FAT table parsing for root dictionary FAIL!! pending!! \n", ret);
        print_f(&mrs->plog, "fs98", mrs->log);
        modersp->r = 2;
    }

    return 1;
}

static int fs99(struct mainRes_s *mrs, struct modersp_s *modersp){return 1;}
static int fs100(struct mainRes_s *mrs, struct modersp_s *modersp){return 1;}
static int fs101(struct mainRes_s *mrs, struct modersp_s *modersp){return 1;}
static int fs102(struct mainRes_s *mrs, struct modersp_s *modersp){return 1;}
static int fs103(struct mainRes_s *mrs, struct modersp_s *modersp){return 1;}
static int fs104(struct mainRes_s *mrs, struct modersp_s *modersp){return 1;}

static int p0(struct mainRes_s *mrs)
{
#define PS_NUM 105

    int ret=0, len=0, tmp=0;
    char ch=0;

    struct modersp_s modesw;
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
                                 {100, fs100},{101, fs101},{102, fs103},{103, fs103},{104, fs104}};                                 
                                 
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
                                 {100, hd100},{101, hd101},{102, hd103},{103, hd103},{104, hd104}};                                 
    p0_init(mrs);

    modesw.m = -2;
    modesw.r = 0;
    modesw.d = 0;

    while (1) {
        //sprintf(mrs->log, ".\n");
        //print_f(&mrs->plog, "P0", mrs->log);
        len = mrs_ipc_get(mrs, &ch, 1, 0);
        if (len > 0) {
            sprintf(mrs->log, "modesw.m:%d ch:0x%x\n", modesw.m, ch);
            print_f(&mrs->plog, "P0", mrs->log);

            if (modesw.m == -2) {
                if ((ch >=0) && (ch < PS_NUM)) {
                    modesw.m = ch;
                }
            } else {
                /* todo: interrupt state machine here */
                if (ch == 0x7f) {
                    ret = 0;
                    ret = (*errHdle[modesw.m].pfunc)(mrs, &modesw);
                    if (!ret) {
                        modesw.m = -1;
                        modesw.d = -1;
                        modesw.r = 0xed;
                    }
                    sprintf(mrs->log, "!! Error handle !! m:%d d:%d ret:%d\n", modesw.m, modesw.d, ret);
                    print_f(&mrs->plog, "P0", mrs->log);
                }
            }
        }

        if ((modesw.m >= 0) && (modesw.m < PS_NUM)) {
            ret = (*afselec[modesw.m].pfunc)(mrs, &modesw);
            //sprintf(mrs->log, "pmode:%d rsp:%d\n", modesw.m, modesw.r);
            //print_f(&mrs->plog, "P0", mrs->log);
            if (ret == 1) {
                tmp = modesw.m;
                modesw.m = -1;
            }
        }

        if (modesw.m == -1) {
            sprintf(mrs->log, "pmode:%d rsp:%d - end\n", tmp, modesw.r);
            print_f(&mrs->plog, "P0", mrs->log);

            ch = modesw.r; /* response */
            modesw.r = 0;
            modesw.v = 0;
            modesw.d = 0;
            modesw.m = -2;
            tmp = -1;

            mrs_ipc_put(mrs, &ch, 1, 0);
        } else {
            mrs_ipc_put(mrs, "$", 1, 0);
        }

        usleep(10000);
    }

    p0_end(mrs);
    return 0;
}

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
                            {stsda_61, stsda_62, stwbk_63, stsda_64, stsda_65}}; // SDAO
                            

    p1_init(rs);
    stdata = rs->pstdata;
    // wait for ch from p0
    // state machine control
    stdata->rs = rs;
    pi = 0;    stdata->result = 0;    cmd = '\0';   cmdt = 'w';
    while (1) {
        //sprintf(rs->logs, "+\n");
        //print_f(rs->plogs, "P1", rs->logs);

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
                    stdata->result = emb_stanPro(0, STINIT, FATH, PSTSM);
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
            sprintf(rs->logs, "rsp, ret:%d ch:0x%.2x\n", ret, ch);
            print_f(rs->plogs, "P1", rs->logs);
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

                sprintf(rs->logs, "<%c,0x%x,done>\n", cmdt, ch);
                print_f(rs->plogs, "P1", rs->logs);

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
#define MSP_P2_SAVE_DAT (0)
#define IN_SAVE (0)
#define TIME_MEASURE (0)
static int p2(struct procRes_s *rs)
{
    FILE *fp=0;
    char fatPath[128] = "/mnt/mmc2/fatTab.bin";
#if IN_SAVE
    char filename[128] = "/mnt/mmc2/tx/input_x3.bin";
    FILE *fin = NULL;
#endif
    struct spi_ioc_transfer *tr = malloc(sizeof(struct spi_ioc_transfer));
    struct timespec tnow;
    struct sdParseBuff_s *pabuf=0;
    int px, pi=0, ret, len=0, opsz, cmode=0, tdiff, tlast, twait, tlen=0, maxsz=0;
    int bitset, totsz=0;
    uint16_t send16, recv16;
    uint32_t secStr=0, secLen=0, datLen=0, minLen=0;
    struct info16Bit_s *p=0, *c=0;
    struct sdFAT_s *pfat=0;
    struct sdFATable_s   *pftb=0;
    struct sdbootsec_s   *psec=0;
    
    char ch, str[128], rx8[4], tx8[4];
    char *addr, *laddr, *rx_buff;
    sprintf(rs->logs, "p2\n");
    print_f(rs->plogs, "P2", rs->logs);

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

    rx_buff = malloc(SPI_TRUNK_SZ);
    ch = '2';

    while (1) {
        //sprintf(rs->logs, "!\n");
        //print_f(rs->plogs, "P2", rs->logs);

        ret = rs_ipc_get(rs, &ch, 1);
        if (ret > 0) {
            sprintf(rs->logs, "recv ch: %c\n", ch);
            print_f(rs->plogs, "P2", rs->logs);
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

                sprintf(rs->logs, "send 0x%.2x 0x%.2x \n", tx8[0], tx8[1]);
                print_f(rs->plogs, "P2", rs->logs);

                len = mtx_data(rs->spifd, rx8, tx8, 2, tr);
                if (len > 0) {
                    recv16 = rx8[1] | (rx8[0] << 8);
                    abs_info(&rs->pmch->get, recv16);
                    rs_ipc_put(rs, "C", 1);
                } else {
                    sprintf(rs->logs, "ch = X \n");
                    print_f(rs->plogs, "P2", rs->logs);

                    bitset = 0;
                    msp_spi_conf(rs->spifd, _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN

                    rs_ipc_put(rs, "X", 1);                
                }
                sprintf(rs->logs, "recv 0x%.2x 0x%.2x len=%d\n", rx8[0], rx8[1], len);
                print_f(rs->plogs, "P2", rs->logs);

            } else if (cmode == 4) {
                //sprintf(rs->logs, "cmode: %d - 5\n", cmode);
                //print_f(rs->plogs, "P2", rs->logs);

                bitset = 0;
                msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
                //sprintf(rs->logs, "Check if RDY pin == 1 (pin:%d)\n", bitset);
                //print_f(rs->plogs, "P2", rs->logs);
            
                while (1) {
                    bitset = 0;
                    msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
                    //sprintf(rs->logs, "Get RDY pin: %d\n", bitset);
                    //print_f(rs->plogs, "P4", rs->logs);
            
                    if (bitset == 1) break;
                }
                if (bitset) rs_ipc_put(rs, "B", 1);
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
                    memset(addr, 0xff, len);
                    msync(addr, len, MS_SYNC);                    
#if  TIME_MEASURE
                    clock_gettime(CLOCK_REALTIME, rs->tm[0]);
#endif
#if SPI_KTHREAD_USE
                    opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr);  //SPI_IOC_PROBE_THREAD
                    while (opsz == 0) {
                        usleep(1000);
                        opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr); //kthread 
                        sprintf(rs->logs, "kth opsz:%d\n", opsz);
                        print_f(rs->plogs, "P2", rs->logs);  
                    }
#else
                    opsz = mtx_data(rs->spifd, addr, NULL, len, tr);

#endif        

#if MSP_P2_SAVE_DAT
                    msync(addr, len, MS_SYNC);                    
                    fwrite(addr, 1, len, rs->fdat_s[1]);
                    fflush(rs->fdat_s[1]);
#endif
                    //printf("0 spi %d\n", opsz);
                    sprintf(rs->logs, "r %d / %d\n", opsz, len);
                    print_f(rs->plogs, "P2", rs->logs);
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
                                usleep(1000);
                                sprintf(rs->logs, "kth opsz:%d\n", opsz);
                                print_f(rs->plogs, "P2", rs->logs);  
                                continue;
                            }
#else
                            opsz = mtx_data(rs->spifd, addr, NULL, len, tr);
#endif
                            //usleep(10000);
                            sprintf(rs->logs, "spi0 recv %d\n", opsz);
                            print_f(rs->plogs, "P2", rs->logs);
                        }

                        //msync(addr, len, MS_SYNC);
                        ring_buf_prod(rs->pcmdRx);    
                        if (opsz < 0) {
                            sprintf(rs->logs, "opsz:%d break!\n", opsz);
                            print_f(rs->plogs, "P2", rs->logs);    
                            break;
                        }
                        rs_ipc_put(rs, "p", 1);
                        pi += 1;
                    }
                }

                sprintf(rs->logs, "len:%d opsz:%d ret:%d, break!\n", len, opsz, ret);
                print_f(rs->plogs, "P2", rs->logs);    

                opsz = 0 - opsz;
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
                        usleep(1000);
                        opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr);  //SPI_IOC_PROBE_THREAD
                        sprintf(rs->logs, "kth opsz:%d\n", opsz);
                        print_f(rs->plogs, "P2", rs->logs);  
                    }
#else
                    opsz = mtx_data(rs->spifd, addr, NULL, SPI_TRUNK_SZ, tr);
#endif
                    sprintf(rs->logs, "r %d\n", opsz);
                    print_f(rs->plogs, "P2", rs->logs);

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
                        usleep(1000);
                        opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr); //kthread 
                        sprintf(rs->logs, "kth opsz:%d\n", opsz);
                        print_f(rs->plogs, "P2", rs->logs);  
                    }
#else
                        opsz = mtx_data(rs->spifd, addr, addr, len, tr);
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

                    sprintf(rs->logs, "u t %d\n", len);
                    print_f(rs->plogs, "P2", rs->logs);          

                    if (len > 0) {
#if MSP_P2_SAVE_DAT
                    msync(addr, len, MS_SYNC);                    
                    fwrite(addr, 1, len, rs->fdat_s[1]);
                    fflush(rs->fdat_s[1]);
#endif

                        if (len < SPI_TRUNK_SZ) len = SPI_TRUNK_SZ;

#if SPI_KTHREAD_USE
                    opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr);  //SPI_IOC_PROBE_THREAD
                    while (opsz == 0) {
                        usleep(1000);
                        opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr); //kthread 
                        sprintf(rs->logs, "kth opsz:%d\n", opsz);
                        print_f(rs->plogs, "P2", rs->logs);  
                    }
#else
                        opsz = mtx_data(rs->spifd, addr, addr, len, tr);
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

                addr = malloc(maxsz);

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
                    
#if SPI_KTHREAD_USE
                    opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr);  //SPI_IOC_PROBE_THREAD
                    while (opsz == 0) {
                        usleep(1000);
                        opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr);  //SPI_IOC_PROBE_THREAD
                        sprintf(rs->logs, "kth opsz:%d\n", opsz);
                        print_f(rs->plogs, "P2", rs->logs);  
                    }
#else
                    msync(addr, tlen, MS_SYNC);                    
                    //shmem_dump(addr, datLen);
                    opsz = mtx_data(rs->spifd, addr, addr, tlen, tr);
#endif

                    sprintf(rs->logs, "r (%d)\n", opsz);
                    print_f(rs->plogs, "P2", rs->logs);

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
#if SPI_KTHREAD_USE
                    opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr);  //SPI_IOC_PROBE_THREAD
                    while (opsz == 0) {
                        usleep(1000);
                        opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr);  //SPI_IOC_PROBE_THREAD
                        sprintf(rs->logs, "kth opsz:%d\n", opsz);
                        print_f(rs->plogs, "P2", rs->logs);  
                    }
#else
                    msync(addr, SPI_TRUNK_SZ, MS_SYNC);
                    opsz = mtx_data(rs->spifd, rx_buff, addr, SPI_TRUNK_SZ, tr);
#endif
                    sprintf(rs->logs, "r %d\n", opsz);
                    print_f(rs->plogs, "P2", rs->logs);

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
            else {
                sprintf(rs->logs, "cmode: %d \n", cmode);
                print_f(rs->plogs, "P2", rs->logs);
            }

        }
    }

    p2_end(rs);
    return 0;
}

static int p3(struct procRes_s *rs)
{
#if IN_SAVE
    char filename[128] = "/mnt/mmc2/tx/input_x2.bin";
    FILE *fin = NULL;
#endif

    struct spi_ioc_transfer *tr = malloc(sizeof(struct spi_ioc_transfer));
    struct timespec tnow;
    int pi, ret, len, opsz, cmode, bitset, tdiff, tlast, twait;
    uint16_t send16, recv16;
    char ch, str[128], rx8[4], tx8[4];
    char *addr, *laddr;
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
                        usleep(1000);
                        opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr); //kthread 
                        sprintf(rs->logs, "kth opsz:%d\n", opsz);
                        print_f(rs->plogs, "P2", rs->logs);  
                    }
#else // #if SPI_KTHREAD_USE
                    opsz = mtx_data(rs->spifd, addr, NULL, len, tr);
#endif // #if SPI_KTHREAD_USE
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
                    sprintf(rs->logs, "r %d / %d\n", opsz, len);
                    print_f(rs->plogs, "P3", rs->logs);
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
                        usleep(1000);
                        opsz = msp_spi_conf(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr); //kthread 
                        sprintf(rs->logs, "kth opsz:%d\n", opsz);
                        print_f(rs->plogs, "P2", rs->logs);  
                    }

#else // #if SPI_KTHREAD_USE
                            opsz = mtx_data(rs->spifd, addr, NULL, len, tr);
#endif
                            sprintf(rs->logs, "r %d\n", opsz);
                            print_f(rs->plogs, "P3", rs->logs);
                        }

                        //msync(addr, len, MS_SYNC);
                        ring_buf_prod(rs->pcmdTx);    
                        if (opsz < 0) break;
                        rs_ipc_put(rs, "p", 1);
                        pi += 1;
                    }
                }
                opsz = 0 - opsz;
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

static int p4(struct procRes_s *rs)
{
    float flsize, fltime;
    int px, pi, ret=0, len, opsz, totsz, tdiff;
    int cmode, acuhk, errtor=0;
    char ch, str[128];
    char *addr, *pre;
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

    recvbuf = malloc(1024);
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
        sprintf(rs->logs, "p4 get bind ret: %d", ret);
        error_handle(rs->logs, 3140);
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
        rs->psocket_t->connfd = accept(rs->psocket_t->listenfd, (struct sockaddr*)NULL, NULL); \
        if (rs->psocket_t->connfd < 0) {
            sprintf(rs->logs, "P4 get connect failed ret:%d", rs->psocket_t->connfd);
            error_handle(rs->logs, 3157);
            continue;
        } else {
            sprintf(rs->logs, "get connection id: %d\n", rs->psocket_t->connfd);
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
                            //sprintf(rs->logs, "t %d -%d \n", opsz, pi);
                            //print_f(rs->plogs, "P4", rs->logs);         
#if MSP_P4_SAVE_DAT
                            fwrite(addr, 1, len, rs->fdat_s[0]);
                            fflush(rs->fdat_s[0]);
#endif
                        } else {
                            sprintf(rs->logs, "len:%d \n", len);
                            print_f(rs->plogs, "P4", rs->logs);         
                        }
                    } else {
                        sprintf(rs->logs, "socket tx %d %d- end\n", opsz, pi);
                        print_f(rs->plogs, "P4", rs->logs);         
                        break;
                    }

                    if (ch != 'D') {
                        ch = 0;
                        rs_ipc_get(rs, &ch, 1);
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
                        sprintf(rs->logs, " %d -%d \n", len, pi);
                        print_f(rs->plogs, "P4", rs->logs);         
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
                        usleep(10000000);
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
    int ret, n, num, hd, be, ed, ln, fg;
    char ch, *recvbuf, *addr, *sendbuf;
    char msg[256], opcode=0, param=0, flag = 0;
    sprintf(rs->logs, "p5\n");
    print_f(rs->plogs, "P5", rs->logs);

    p5_init(rs);
    // wait for ch from p0
    // in charge of socket recv

    sendbuf = malloc(2048);
    if (!sendbuf) {
        sprintf(rs->logs, "p5 sendbuf alloc failed! \n");
        print_f(rs->plogs, "P5", rs->logs);
        return (-1);
    }

    recvbuf = malloc(1024);
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
        sprintf(rs->logs, "p5 get bind ret: %d", ret);
        error_handle(rs->logs, 3314);
    }

    ret = listen(rs->psocket_r->listenfd, 10); 
    if (ret < 0) {
        sprintf(rs->logs, "p5 get listen ret: %d", ret);
        error_handle(rs->logs, 3320);
    }
    
    while (1) {
        //printf("#");
        //sprintf(rs->logs, "#\n");
        //print_f(rs->plogs, "P5", rs->logs);
        ret = -1;

        rs->psocket_r->connfd = accept(rs->psocket_r->listenfd, (struct sockaddr*)NULL, NULL); 
        if (rs->psocket_r->connfd < 0) {
            sprintf(rs->logs, "P5 get connect failed ret:%d", rs->psocket_r->connfd);
            error_handle(rs->logs, 3331);
            continue;
        }

        memset(recvbuf, 0x0, 1024);
        memset(sendbuf, 0x0, 1024);
        memset(msg, 0x0, 256);

        n = read(rs->psocket_r->connfd, recvbuf, 1024);
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

        n = strlen(&recvbuf[hd]);
        if (n <= 0) {
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

        n = ed - be - 1;
        if ((n < 255) && (n > 0)) {
            memcpy(msg, &recvbuf[be+1], n);
            msg[n] = '\0';
        } else {
            goto socketEnd;
        }

        if (opcode != OP_MSG) {
            n = 0;
        }

        if (n > 0) {
            //rs_ipc_put(rs, "s", 1);
            //rs_ipc_put(rs, msg, n);
            //sprintf(rs->logs, "send to p0 [%s]\n", recvbuf);
            //print_f(rs->plogs, "P5", rs->logs);
            
            //ret = write(rs->psocket_r->connfd, msg, n);
            sprintf(rs->logs, "send back app [%s] size:%d/%d\n", msg, ret, n);
            print_f(rs->plogs, "P5", rs->logs);
            rs_ipc_put(rcmd, msg, n);
        }else {
            msg[0] = 'o';
            msg[1] = 'p';
            rs_ipc_put(rcmd, msg, 2);


            ch = 0; n = 0;
            n = rs_ipc_get(rcmd, &ch, 1);

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

            ch = 0; n = 0;
            n = rs_ipc_get(rcmd, &ch, 1);

            if (ch != 'p') {
                opcode = OP_ERROR; 
                n = rs_ipc_get(rcmd, &ch, 1);
                param = ch;
            } else {
                n = rs_ipc_get(rcmd, &ch, 1);
                param = ch;
            }
        }

        //usleep(100000);
        memset(sendbuf, 0, 2048);

        sendbuf[0] = 0xfe;
        sendbuf[1] = ((opcode & 0x80) ? 1:0) + 1;
        sendbuf[2] = opcode & 0x7f;
        sendbuf[3] = 0xfd;
        //sendbuf[3] = 'P';//0x0;
        sendbuf[6] = 0xfc;

        n = rs_ipc_get(rcmd, &sendbuf[7], 2048 - 7);
        sendbuf[4] = ((param & 0x80) ? 1:0) + 1;
        sendbuf[5] = param & 0x7f;
        
        sendbuf[7+n] = 0xfb;
        sendbuf[7+n+1] = '\0';
        sendbuf[7+n+2] = '\0';
        ret = write(rs->psocket_r->connfd, sendbuf, 7+n+3);
        //sprintf(rs->logs, "socket send, len:%d content[%s] from %d, ret:%d, opcode:%d, [%x][%x][%x][%x]\n", 7+n+3, sendbuf, rs->psocket_r->connfd, ret, opcode, sendbuf[1], sendbuf[2], sendbuf[4], sendbuf[5]);
        //print_f(rs->plogs, "P5", sendbuf);
        //printf("[p5]:%s\n", sendbuf);

        socketEnd:
        //sprintf(rs->logs, "END receive len[%d]content[%s]hd[%d]be[%d]ed[%d]ln[%d]fg[%d]\n", n, recvbuf, hd, be, ed, ln, fg);
        sprintf(rs->logs, "END receive len[%d]hd[%d]be[%d]ed[%d]ln[%d]fg[%d]\n", n, hd, be, ed, ln, fg);
        print_f(rs->plogs, "P5", rs->logs);

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
static int p6(struct procRes_s *rs)
{
    char strFullPath[1024];
    char strPath[32][128];
    //char **strPath; 
    char *recvbuf, *sendbuf, *pr;
    int ret, n, num, hd, be, ed, ln, cnt=0, i, len=0;
    char opc=0;
    char opcode=0, param=0, flag = 0;
    uint32_t clstSize=0;
    uint32_t adata[3], atime[3];
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

    struct aspInfoSplit_s *strinfo=0, *nexinfo=0;
    
    pct = rs->pcfgTable;
    pfat = rs->psFat;
    pftb = pfat->fatTable;

    char dir[256] = "/mnt/mmc2";
    //char dir[256] = "/root";
    char folder[256];

    sprintf(rs->logs, "p6\n");
    print_f(rs->plogs, "P6", rs->logs);

    p6_init(rs);
/*
    while (!rs->psFat->fatRootdir);    
    fscur = rs->psFat->fatRootdir;
*/

    recvbuf = malloc(1024);
    if (!recvbuf) {
        sprintf(rs->logs, "recvbuf alloc failed! \n");
        print_f(rs->plogs, "P6", rs->logs);
        return (-1);
    } else {
        sprintf(rs->logs, "recvbuf alloc success! 0x%x\n", recvbuf);
        print_f(rs->plogs, "P6", rs->logs);
    }

    sendbuf = malloc(1024);
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
        sprintf(rs->logs, "p6 get bind ret: %d", ret);
        error_handle(rs->logs, 3795);
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
        
        rs->psocket_at->connfd = accept(rs->psocket_at->listenfd, (struct sockaddr*)NULL, NULL); 
        if (rs->psocket_at->connfd < 0) {
            sprintf(rs->logs, "P6 get connect failed ret:%d", rs->psocket_at->connfd);
            error_handle(rs->logs, 3812);
            goto socketEnd;
        }

        memset(recvbuf, 0x0, 1024);
        memset(sendbuf, 0x0, 1024);
        
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
        sprintf(rs->logs, "opcode:[0x%x]arg[0x%x]\n", opcode, param);
        print_f(rs->plogs, "P6", rs->logs);

        n = strlen(&recvbuf[hd]);
        if (n <= 0) {
            goto socketEnd;
        }

        sprintf(rs->logs, "receive len[%d]content[%s]hd[%d]be[%d]ed[%d]ln[%d]\n", n, &recvbuf[hd], hd, be, ed, ln);
        print_f(rs->plogs, "P6", rs->logs);

        sprintf(rs->logs, "opcode:[0x%x]arg[0x%x]\n", recvbuf[hd+1], recvbuf[be-1]);
        print_f(rs->plogs, "P6", rs->logs);

        n = ed - be - 1;
        if ((n < 255) && (n > 0)) {
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


        if (opcode == 0x14) { /* update UTC time */
            sprintf(rs->logs, "handle opcode: 0x%x\n", opcode);
            print_f(rs->plogs, "P6", rs->logs);

            ret = asp_strsplit(&strinfo, folder, n);
            n = 0;
            if (ret > 0) {
                sprintf(rs->logs, "split str found, ret:%d\n", ret);
                print_f(rs->plogs, "P6", rs->logs);

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
                        sprintf(rs->logs, "%d.%s = %d\n", 2+i, nexinfo->infoStr, adata[i]);
                        print_f(rs->plogs, "P6", rs->logs);
                    } else {
                        break;
                    } 
                }

                for (i = 0 ; i < 3; i++) {
                    nexinfo = asp_getInfo(strinfo, i+3);
                    if (nexinfo) {
                        atime[i] = atoi(nexinfo->infoStr);
                        sprintf(rs->logs, "%d.%s = %d\n", 5+i, nexinfo->infoStr, atime[i]);
                        print_f(rs->plogs, "P6", rs->logs);
                    } else {
                        break;
                    } 
                }

                memset(curTime, 0, 16);
                sprintf(curTime, "%.4d%.2d%.2d%.2d%.2d", adata[0], adata[1], adata[2], atime[0], atime[1]);

                sprintf(syscmd, "date -s %s", curTime);
                ret = system(syscmd);
                sprintf(rs->logs, "system command:[%s] ret:%d \n", syscmd, ret);
                print_f(rs->plogs, "P6", rs->logs);

                sprintf(syscmd, "hwclock -w");
                ret = system(syscmd);
                sprintf(rs->logs, "system command:[%s] ret:%d \n", syscmd, ret);
                print_f(rs->plogs, "P6", rs->logs);

                sprintf(syscmd, "date");
                ret = system(syscmd);
                sprintf(rs->logs, "system command:[%s] ret:%d \n", syscmd, ret);
                print_f(rs->plogs, "P6", rs->logs);

                sprintf(rs->logs, "system time update: %s \n", curTime);
                print_f(rs->plogs, "P6", rs->logs);
                
                char *wday[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"}; 
                struct tm *p; 
                time_t timep;

                time(&timep);
                sprintf(rs->logs, "get current %s in C \n", ctime(&timep));
                print_f(rs->plogs, "P6", rs->logs);
 
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
                        upld->dfrecotime = (((atime[0]  << 16)&0xff) | ((atime[1]&0xff) << 8) | (atime[2]&0xff));

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

static int p7(struct procRes_s *rs)
{
    char chbuf[32];
    char ch=0, *addr=0;
    int ret=0, len=0, num=0, tx=0;
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
        sprintf(rs->logs, "p7 get bind ret: %d", ret);
        error_handle(rs->logs, 4029);
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
        rs->psocket_n->connfd = accept(rs->psocket_n->listenfd, (struct sockaddr*)NULL, NULL); 
        if (rs->psocket_n->connfd < 0) {
            sprintf(rs->logs, "P7 get connect failed ret:%d", rs->psocket_n->connfd);
            error_handle(rs->logs, 4045);
            goto socketEnd;
        } else {
            sprintf(rs->logs, "get connection id: %d\n", rs->psocket_n->connfd);
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
                            sprintf(rs->logs, "%c socket tx %d %d %d \n", ch, rs->psocket_n->connfd, num, tx);
                            print_f(rs->plogs, "P7", rs->logs);         
                        }
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
                        sprintf(rs->logs, " %d -%d \n", len, tx);
                        print_f(rs->plogs, "P7", rs->logs);         
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

int main(int argc, char *argv[])
{
//static char spi1[] = "/dev/spidev32766.0"; 
//static char spi0[] = "/dev/spidev32765.0"; 
    char dir[256] = "/mnt/mmc2";
    struct mainRes_s *pmrs;
    struct procRes_s rs[9];
    int ix, ret;
    char *log;
    int tdiff;
    int arg[8];
    uint32_t bitset;

    pmrs = (struct mainRes_s *)mmap(NULL, sizeof(struct mainRes_s), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);;
    memset(pmrs, 0, sizeof(struct mainRes_s));

    pmrs->plog.max = 6*1024*1024;
    pmrs->plog.pool = mmap(NULL, pmrs->plog.max, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
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
    ix = 0;
    while(argc) {
        arg[ix] = atoi(argv[ix]);
        sprintf(pmrs->log, "%d %d %s\n", ix, arg[ix], argv[ix]);
        print_f(&pmrs->plog, "arg", pmrs->log);
        ix++;
        argc--;
        if (ix > 7) break;
    }
// initial share parameter
    /* data mode rx from spi */
    clock_gettime(CLOCK_REALTIME, &pmrs->time[0]);
    pmrs->dataRx.pp = memory_init(&pmrs->dataRx.slotn, 128*SPI_TRUNK_SZ, SPI_TRUNK_SZ);
    if (!pmrs->dataRx.pp) goto end;
    pmrs->dataRx.r = (struct ring_p *)mmap(NULL, sizeof(struct ring_p), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    pmrs->dataRx.totsz = 128*SPI_TRUNK_SZ;
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
    pmrs->dataTx.pp = memory_init(&pmrs->dataTx.slotn, 128*SPI_TRUNK_SZ, SPI_TRUNK_SZ);
    if (!pmrs->dataTx.pp) goto end;
    pmrs->dataTx.r = (struct ring_p *)mmap(NULL, sizeof(struct ring_p), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    pmrs->dataTx.totsz = 128*SPI_TRUNK_SZ;
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
    pmrs->cmdRx.pp = memory_init(&pmrs->cmdRx.slotn, 128*SPI_TRUNK_SZ, SPI_TRUNK_SZ);
    if (!pmrs->cmdRx.pp) goto end;
    pmrs->cmdRx.r = (struct ring_p *)mmap(NULL, sizeof(struct ring_p), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    pmrs->cmdRx.totsz = 128*SPI_TRUNK_SZ;;
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
    pmrs->cmdTx.pp = memory_init(&pmrs->cmdTx.slotn, 512*SPI_TRUNK_SZ, SPI_TRUNK_SZ);
    if (!pmrs->cmdTx.pp) goto end;
    pmrs->cmdTx.r = (struct ring_p *)mmap(NULL, sizeof(struct ring_p), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    pmrs->cmdTx.totsz = 512*SPI_TRUNK_SZ;
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

    pmrs->wtg.wtRlt =  mmap(NULL, 16, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
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

    struct aspConfig_s* ctb = 0;
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
            ctb->opMask = ASPOP_MASK_3;
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
            ctb->opMask = ASPOP_MASK_3;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SCAN_DOUBLE: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_DOUBLE;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_3;
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
            ctb->opStatus = ASPOP_STA_UPD; // for debug, should be ASPOP_STA_NONE
            ctb->opCode = OP_SUPBACK;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = SUPBACK_SD; // for debug, should be 0xff
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

        default: break;
        }
    }

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

    /* FAT */
    pmrs->aspFat.fatBootsec = (struct sdbootsec_s *)mmap(NULL, sizeof(struct sdbootsec_s), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    if (!pmrs->aspFat.fatBootsec) {
        sprintf(pmrs->log, "alloc share memory for FAT boot sector FAIL!!!\n", pmrs->aspFat.fatBootsec); 
        print_f(&pmrs->plog, "FAT", pmrs->log);
        goto end;
    } else {
        sprintf(pmrs->log, "alloc share memory for FAT boot sector DONE [0x%x]!!!\n", pmrs->aspFat.fatBootsec); 
        print_f(&pmrs->plog, "FAT", pmrs->log);
    }
    
    pmrs->aspFat.fatFSinfo = (struct sdFSinfo_s *)mmap(NULL, sizeof(struct sdFSinfo_s), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    if (!pmrs->aspFat.fatFSinfo) {
        sprintf(pmrs->log, "alloc share memory for FAT file system info FAIL!!!\n", pmrs->aspFat.fatFSinfo); 
        print_f(&pmrs->plog, "FAT", pmrs->log);
        goto end;
    } else {
        sprintf(pmrs->log, "alloc share memory for FAT file system info DONE [0x%x]!!!\n", pmrs->aspFat.fatFSinfo); 
        print_f(&pmrs->plog, "FAT", pmrs->log);
    }

    pmrs->aspFat.fatTable = (struct sdFATable_s *)mmap(NULL, sizeof(struct sdFATable_s), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    if (!pmrs->aspFat.fatTable) {
        sprintf(pmrs->log, "alloc share memory for FAT table FAIL!!!\n", pmrs->aspFat.fatTable); 
        print_f(&pmrs->plog, "FAT", pmrs->log);
        goto end;
    } else {
        sprintf(pmrs->log, "alloc share memory for FAT table DONE [0x%x]!!!\n", pmrs->aspFat.fatTable); 
        print_f(&pmrs->plog, "FAT", pmrs->log);
    }

    pmrs->aspFat.fatDirPool = (struct sdDirPool_s *)mmap(NULL, sizeof(struct sdDirPool_s), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
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
    pool->dirPool = (struct directnFile_s *)mmap(NULL, sizeof(struct directnFile_s) * DIR_POOL_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
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
    
    pool->parBuf.dirParseBuff = mmap(NULL, 8*1024*1024, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
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
    int fd0, fd1;
    fd0 = open(spi0, O_RDWR);
    if (fd0 <= 0) {
        sprintf(pmrs->log, "can't open device[%s]\n", spi0); 
        print_f(&pmrs->plog, "SPI", pmrs->log);
        goto end;
    } else {
        sprintf(pmrs->log, "open device[%s]\n", spi0); 
        print_f(&pmrs->plog, "SPI", pmrs->log);
    }
    if (spi1) {
        fd1 = open(spi1, O_RDWR);
        if (fd1 <= 0) {
            sprintf(pmrs->log, "can't open device[%s]\n", spi1); 
            print_f(&pmrs->plog, "SPI", pmrs->log);
            fd1 = 0;
        } else {
            sprintf(pmrs->log, "open device[%s]\n", spi1); 
            print_f(&pmrs->plog, "SPI", pmrs->log);
        }
    } else {
        fd1 = fd0;
    }

    pmrs->sfm[0] = fd0;
    pmrs->sfm[1] = fd1;
    pmrs->smode = 0;
    pmrs->smode |= SPI_MODE_1;

    /* set RDY pin to low before spi setup ready */
    bitset = 0;
    ret = msp_spi_conf(pmrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(pmrs->log, "Set RDY low at beginning\n");
    print_f(&pmrs->plog, "SPI", pmrs->log);

    bitset = 0;     
    msp_spi_conf(pmrs->sfm[0], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL    
    bitset = 1;    
    msp_spi_conf(pmrs->sfm[1], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL

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

    pipe2(pmrs->pipeup[0].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[1].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[2].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[3].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[4].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[5].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[6].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[7].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[8].rt, O_NONBLOCK);

    res_put_in(&rs[0], pmrs, 0);
    res_put_in(&rs[1], pmrs, 1);
    res_put_in(&rs[2], pmrs, 2);
    res_put_in(&rs[3], pmrs, 3);
    res_put_in(&rs[4], pmrs, 4);
    res_put_in(&rs[5], pmrs, 5);
    res_put_in(&rs[6], pmrs, 6);
    res_put_in(&rs[7], pmrs, 7);
    res_put_in(&rs[8], pmrs, 8);

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
                                    p0(pmrs);
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

#define MSP_SAVE_LOG (0)

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

    plog->cur = plog->pool;
    plog->len = 0;

    return 0;
}

static int print_f(struct logPool_s *plog, char *head, char *str)
{
    int len;
    char ch[256];
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
    pma = (char **) malloc(sizeof(char *) * asz);
    
    //sprintf(mlog, "asz:%d pma:0x%.8x\n", asz, pma);
    //print_f(mlogPool, "memory_init", mlog);
    
    mbuf = mmap(NULL, tsize, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    
    //sprintf(mlog, "mmap get 0x%.8x\n", mbuf);
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

    rs->pmch = &mrs->mchine;
    rs->pstdata = &mrs->stdata;
    rs->pcfgTable = mrs->configTable;
    rs->psFat = &mrs->aspFat;

    return 0;
}


