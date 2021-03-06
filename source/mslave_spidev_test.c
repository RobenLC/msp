#include <stdint.h> 
#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <getopt.h> 
#include <fcntl.h> 
#include <sys/ioctl.h> 
#include <sys/mman.h> 

#include <sys/types.h>  
#include <signal.h>

#include <linux/types.h> 
#include <linux/spi/spidev.h> 
#include <sys/times.h> 
#include <time.h>
#include <sys/socket.h>

#include <dirent.h>
#include <sys/stat.h>  
//#include <linux/poll.h>
#include <poll.h>
#include <sys/epoll.h>
#include <errno.h> 

#define SPI1_ENABLE (1)

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0])) 

/* USB ioctl  */
#define IOCNR_GET_DEVICE_ID		1
#define IOCNR_GET_VID_PID		6
#define IOCNR_CONTI_READ_START  8
#define IOCNR_CONTI_READ_STOP    9
#define IOCNR_CONTI_READ_PROBE    10
#define IOCNR_CONTI_READ_ONCE    11
#define IOCNR_CONTI_READ_RESET   12
#define IOCNR_CONTI_BUFF_CREATE    13
#define IOCNR_CONTI_BUFF_PROBE    14
#define IOCNR_CONTI_BUFF_RELEASE    15

#define USB_IOC_CONTI_READ_START  _IOC(_IOC_NONE, 'P', IOCNR_CONTI_READ_START, 0)
#define USB_IOC_CONTI_READ_STOP    _IOC(_IOC_NONE, 'P', IOCNR_CONTI_READ_STOP, 0)
#define USB_IOC_CONTI_READ_PROBE  _IOC(_IOC_NONE, 'P', IOCNR_CONTI_READ_PROBE, 0)
#define USB_IOC_CONTI_READ_ONCE   _IOC(_IOC_NONE, 'P', IOCNR_CONTI_READ_ONCE, 0)
#define USB_IOC_CONTI_READ_RESET   _IOC(_IOC_NONE, 'P', IOCNR_CONTI_READ_RESET, 0)
#define USB_IOC_CONTI_BUFF_CREATE  _IOC(_IOC_NONE, 'P', IOCNR_CONTI_BUFF_CREATE, 0)
#define USB_IOC_CONTI_BUFF_PROBE  _IOC(_IOC_NONE, 'P', IOCNR_CONTI_BUFF_PROBE, 0)
#define USB_IOC_CONTI_BUFF_RELEASE  _IOC(_IOC_NONE, 'P', IOCNR_CONTI_BUFF_RELEASE, 0)
#define LPIOC_GET_DEVICE_ID(len)     _IOC(_IOC_READ, 'P', IOCNR_GET_DEVICE_ID, len) 
#define LPIOC_GET_VID_PID(len) _IOC(_IOC_READ, 'P', IOCNR_GET_VID_PID, len)

#define USB_IOCT_LOOP_START(a, b)               ioctl(a, USB_IOC_CONTI_READ_START, b)
#define USB_IOCT_LOOP_CONTI_READ(a, b)     ioctl(a, USB_IOC_CONTI_READ_PROBE, b)
#define USB_IOCT_LOOP_STOP(a, b)               ioctl(a, USB_IOC_CONTI_READ_STOP, b)
#define USB_IOCT_LOOP_ONCE(a, b)               ioctl(a, USB_IOC_CONTI_READ_ONCE, b)
#define USB_IOCT_LOOP_RESET(a, b)               ioctl(a, USB_IOC_CONTI_READ_RESET, b)
#define USB_IOCT_LOOP_BUFF_CREATE(a, b)     ioctl(a, USB_IOC_CONTI_BUFF_CREATE, b)
#define USB_IOCT_LOOP_BUFF_PROBE(a, b)     ioctl(a, USB_IOC_CONTI_BUFF_PROBE, b)
#define USB_IOCT_LOOP_BUFF_RELEASE(a, b)     ioctl(a, USB_IOC_CONTI_BUFF_RELEASE, b)

#define USB_IOCT_GET_DEVICE_ID(a, b)          ioctl(a, LPIOC_GET_DEVICE_ID(4), b)
#define USB_IOCT_GET_VID_PID(a, b)          ioctl(a, LPIOC_GET_VID_PID(8), b)

#define USB_CALLBACK_SUBMIT (0)
 
#define SPI_CPHA  0x01          /* clock phase */
#define SPI_CPOL  0x02          /* clock polarity */
#define SPI_MODE_0  (0|0)            /* (original MicroWire) */
#define SPI_MODE_1  (0|SPI_CPHA)
#define SPI_MODE_2  (SPI_CPOL|0)
#define SPI_MODE_3  (SPI_CPOL|SPI_CPHA)
#define SPI_CS_HIGH 0x04          /* chipselect active high? */
#define SPI_LSB_FIRST 0x08          /* per-word bits-on-wire */
#define SPI_3WIRE 0x10          /* SI/SO signals shared */
#define SPI_LOOP  0x20          /* loopback mode */
#define SPI_NO_CS 0x40          /* 1 dev/bus, no chipselect */
#define SPI_READY 0x80          /* slave pulls low to pause */
#define SPI_TX_DUAL 0x100         /* transmit with 2 wires */
#define SPI_TX_QUAD 0x200         /* transmit with 4 wires */
#define SPI_RX_DUAL 0x400         /* receive with 2 wires */
#define SPI_RX_QUAD 0x800         /* receive with 4 wires */
#define BUFF_SIZE  2048

#define TSIZE (32*1024*1024)
#define SPI_TRUNK_SZ             32768

#define OP_PON 0x1
#define OP_QRY 0x2
#define OP_RDY 0x3
#define OP_DAT 0x4
#define OP_SCM 0x5
#define OP_DCM 0x6
#define OP_FIH  0x7
#define OP_DUL 0x8
#define OP_SDRD 0x9
#define OP_SDWT 0xa
#define OP_SDAT 0xb /* single data mode */
#define OP_RGRD 0xc
#define OP_RGWT 0xd
#define OP_RGDAT 0xe

#define OP_MAX 0xff
#define OP_NONE 0x00

#define OP_STSEC_0  0x10
#define OP_STSEC_1  0x11
#define OP_STSEC_2  0x12
#define OP_STSEC_3  0x13
#define OP_STLEN_0  0x14
#define OP_STLEN_1  0x15
#define OP_STLEN_2  0x16
#define OP_STLEN_3  0x17
#define OP_RGADD_H  0x18
#define OP_RGADD_L  0x19

#define OP_FFORMAT      0x20
#define OP_COLRMOD      0x21
#define OP_COMPRAT      0x22
#define OP_SCANMOD      0x23
#define OP_DATPATH      0x24
#define OP_RESOLTN       0x25
#define OP_SCANGAV       0x26
#define OP_MAXWIDH      0x27
#define OP_WIDTHAD_H   0x28
#define OP_WIDTHAD_L   0x29
#define OP_SCANLEN_H    0x2a
#define OP_SCANLEN_L    0x2b

#define OP_SUP               0x31

#define sprintf_f  sprintf

struct intMbs_s{
    union {
        uint32_t n;
        uint8_t d[4];
    };
};

struct aspMetaData_s{
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
  unsigned char  INTERNAL_IMG;             //0x3b
  unsigned char  AFE_IC_SELEC;             //0x3c
  unsigned char  EXTNAL_PULSE;            //0x3d
  unsigned char  SUP_WRITEBK;             //0x3e
  unsigned char  OP_FUNC_00;               //0x70
  unsigned char  OP_FUNC_01;               //0x71
  unsigned char  OP_FUNC_02;               //0x72
  unsigned char  OP_FUNC_03;               //0x73
  unsigned char  OP_FUNC_04;               //0x74
  unsigned char  OP_FUNC_05;               //0x75
  unsigned char  OP_FUNC_06;               //0x76
  unsigned char  OP_FUNC_07;               //0x77
  unsigned char  OP_FUNC_08;               //0x78
  unsigned char  OP_FUNC_09;               //0x79
  unsigned char  OP_FUNC_10;               //0x7A
  unsigned char  OP_FUNC_11;               //0x7B
  unsigned char  OP_FUNC_12;               //0x7C
  unsigned char  OP_FUNC_13;               //0x7D
  unsigned char  OP_FUNC_14;               //0x7E
  unsigned char  OP_FUNC_15;               //0x7F  
  unsigned char  BLEEDTHROU_ADJUST; //0x81
  unsigned char  BLACKWHITE_THSHLD; //0x82  
  unsigned char  SD_CLK_RATE_16;        //0x83    
  unsigned char  PAPER_SIZE;                //0x84  
  unsigned char  JPGRATE_ENG_17;       //0x85  
  unsigned char  OP_FUNC_18;               //0x86  
  unsigned char  OP_FUNC_19;               //0x87  
  unsigned char  OP_FUNC_20;               //0x88  
  unsigned char  OP_RESERVE[20];          // byte[64]
  
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

struct directnFile_s{
    uint32_t   dftype;
    uint32_t   dfstats;
    char        dfLFN[256];
    char        dfSFN[16];
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

struct pipe_s{
    int rt[2];
};

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

struct sdFATitem_s{
    char fitBP[4];
};
struct sdFATable_s{
    struct sdFATitem_s *ftbFat;
    int ftbLen;
};

struct sdFAT_s{
    struct sdbootsec_s   *fatBootsec;
    struct sdFSinfo_s     *fatFSinfo;
    struct sdFATable_s   *fatTable;
    struct directnFile_s   *fatRootdir;
};

static void pabort(const char *s) 
{ 
    perror(s); 
    abort(); 
} 
/*
 struct dirent {
    ino_t d_ino;
    off_t d_off;
    unsigned short int d_reclen;
    unsigned char d_type;
    char d_name[256];
};
*/
static const char *device = "/dev/spidev32765.0"; 
static const char *data_path = "/mnt/mmc2/tmp/1.jpg"; 
static uint32_t mode; 
static uint8_t bits = 8; 
static uint32_t speed = 1000000; 
static uint16_t delay; 
static uint16_t command = 0; 
static uint8_t loop = 0; 

static struct logPool_s *mlogPool=0;

typedef enum {
    ASPFS_TYPE_ROOT = 0x1,
    ASPFS_TYPE_DIR,
    ASPFS_TYPE_FILE,
} aspFS_Type_e;

struct slvbitMapHeader_s {
    char aspbmpMagic[4];
    int    aspbhSize;
    char aspbhReserve[4];
    int    aspbhRawoffset;
    int    aspbiSize;
    int    aspbiWidth;
    int    aspbiHeight;
    int    aspbiCPP;
    int    aspbiCompMethd;
    int    aspbiRawSize;
    int    aspbiResoluH;
    int    aspbiResoluV;
    int    aspbiNumCinCP;
    int    aspbiNumImpColor;
    char *bitbuf;
};

struct logPool_s{
    char *pool;
    char *cur;
    int max;
    int len;
    uint32_t dislog;
};

/* construct the file system */
static int aspFS_createRoot(struct directnFile_s **root, char *dir);
static int aspFS_insertChilds(struct directnFile_s *root);
static int aspFS_insertChildDir(struct directnFile_s *parent, char *dir);
static int aspFS_insertChildFile(struct directnFile_s *parent, char *str);
static int aspFS_list(struct directnFile_s *root, int depth);
static int aspFS_search(struct directnFile_s **dir, struct directnFile_s *root, char *path);
static int aspFS_showFolder(struct directnFile_s *root);
static int aspFS_folderJump(struct directnFile_s **dir, struct directnFile_s *root, char *path);
static int aspFS_createFATRoot(struct sdFAT_s *pfat);
static int aspFS_insertFATChilds(struct directnFile_s *root, char *dir, int max);
static int aspFS_insertFATChild(struct directnFile_s *parent, struct directnFile_s *r);


static int aspFS_deleteNote(struct directnFile_s *root, char *path);
static int aspFS_save(struct directnFile_s *root, FILE fp);
static int aspFS_extract(struct directnFile_s *root, FILE fp);
static int aspFS_getNote(struct directnFile_s *note, struct directnFile_s *root, char *path);
static int aspFS_getFilelist(char *flst, struct directnFile_s *note);
static uint32_t msb2lsb(struct intMbs_s *msb);
static int aspSD_getRoot();
static int aspSD_getDir();

static int mspbitmapHeaderSetup(struct slvbitMapHeader_s *ph, int clr, int w, int h, int dpi, int flen) 
{
    int rawoffset=0, totsize=0, numclrp=0, calcuraw=0, rawsize=0;
    float resH=0, resV=0, ratio=39.27, fval=0;

    if (!w) return -1;
    if (!h) return -2;
    if (!dpi) return -3;
    if (!flen) return -4;
    memset(ph, 0, sizeof(struct slvbitMapHeader_s));

    if (clr == 8) {
        numclrp = 256;
        rawoffset = 1078;
        calcuraw = w * h;
    }
    else if (clr == 24) {
        numclrp = 0;
        rawoffset = 54;
        calcuraw = w * h * 3;
    } else {
        printf("[BMP] reset header ERROR!!! color bits is %d \n", clr);
        return -5;
    }

    if (calcuraw != flen) {
        printf("[BMP] WARNNING!!! raw size %d is wrong, should be %d x %d x %d= %d \n", flen, w, h, clr / 8, calcuraw);
        if (flen > calcuraw) {
            rawsize = calcuraw;
        } else {
            rawsize = flen;
        }
    } else {
        rawsize = calcuraw;
    }

    totsize = rawsize + rawoffset;
    
    fval = dpi;
    resH = fval * ratio;
    fval = dpi;
    resV = fval * ratio;
    
    ph->aspbmpMagic[2] = 'B';
    ph->aspbmpMagic[3] = 'M';       
    ph->aspbhSize = totsize; // file total size
    ph->aspbhRawoffset = rawoffset; // header size include color table 54 + 1024 = 1078
    ph->aspbiSize = 40;
    ph->aspbiWidth = w; // W
    ph->aspbiHeight = h; // H
    ph->aspbiCPP = 1;
    ph->aspbiCPP |= clr << 16;  // 8 or 24
    ph->aspbiCompMethd = 0;
    ph->aspbiRawSize = rawsize; // size of raw
    //ph->aspbiResoluH = (int)resH; // dpi x 39.27
    //ph->aspbiResoluV = (int)resV; // dpi x 39.27
    ph->aspbiNumCinCP = numclrp;  // 24bit is 0, 8bit is 256
    ph->aspbiNumImpColor = 0;

    return 0;
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
    if (!tlen) return -4;
    
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

                //printf("[changeImgLen] Length = %d -> %d\n", len, tlen);
                
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

static int shmem_dump(char *src, int size)
{
    int inc;
    if (!src) return -1;

    inc = 0;
    printf("memdump[0x%.8x] sz%d: \n", (uint32_t)src, size);
    while (inc < size) {
        printf("%.2x ", *src);

        if (!((inc+1) % 16)) {
            printf("\n");
        }
        inc++;
        src++;
    }

    printf("\n");
    
    return inc;
}

static int spi_config(int dev, int flag, uint32_t *bitset) {
    int ret;
    
    if (!dev) {
        printf("spi device id error, dev:%d\n", dev);
        return -1;
    }
    ret = ioctl(dev, flag, bitset);

    return ret;
}

static void aspFSrmspace(char *str, int len)
{
    int sc=0;
    char *end=0;
    if (!str) return;
    if (!len) return;
    end = str + len;
    while (end > str) {
        if (*end == 0x20) *end = 0;
        if (*end != 0) break;
        end --;
    }
}

static uint8_t aspFSchecksum(uint8_t *pFcbName)
{
    int len=0;
    uint8_t sum=0;

    for (len=11; len != 0; len--) {
        sum = ((sum & 0x1) ? 0x80 : 0) + (sum >> 1) + *pFcbName;
        pFcbName++;
    }

    return sum;
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

static uint32_t aspFStimeAsb(uint32_t fst)
{
    uint32_t val=0, s=0, m=0, h=0;
    s = fst & 0x1f; // 0 -4, 5bits
    m = (fst >> 5) & 0x3f; // 5 - 10, 6bits
    h = (fst >> 11) & 0x1f; // 11 - 15, 5bits
    val |= (h << 16) | (m << 8) | s;
    return val;
}

static char aspLnameFilter(char ch)
{
    char def = '_', *p=0;
    char notAllow[16] = {0x22, 0x2a, 0x2b, 0x2c, 0x2c, 0x2f, 0x3a, 0x3b, 
                              0x3c, 0x3d, 0x3e, 0x3f, 0x5b, 0x5c, 0x5d, 0x7c};

    if (ch == 0x0)  return ch;
    if (ch < 0x20)  return def;

    p = notAllow + 15;
    while (p >= notAllow) {
        if (*p == ch) return def;
        p --;
    }

    return ch;
}

static int aspNameCpy(char *raw, char *dst, int offset, int len, int jump)
{
    char ch=0;
    int i=0, cnt=0, idx=0;

    cnt = 0;
    for (i = 0; i < len; i++) {
        idx = offset+i*jump;
        if (idx > 32) return (-1);
        ch = aspLnameFilter(raw[idx]);
        if (ch == 0xff) return cnt;
        *dst = ch;
        dst ++;
        cnt++;
        if (ch == 0) return cnt;
    }

    //printf("cpy cnt:%d \n", cnt);
    return cnt;
}

static int aspLnameAbs(char *raw, char *dst) 
{
    int cnt=0, ret=0;
    char ch=0;
    if (!raw) return (-1);
    if (!dst) return (-2);

    ret = aspNameCpy(raw, dst, 1, 5, 2);
    cnt += ret;
    if (ret != 5) return cnt;

    dst += ret;
    ret = aspNameCpy(raw, dst, 14, 6, 2);
    cnt += ret;
    if (ret != 6) return cnt;
    
    dst += ret;
    ret = aspNameCpy(raw, dst, 28, 2, 2);
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
        return 0;
    } else if (fs->dfstats == ASPFS_STATUS_DIS) {
        if (fs->dflen) {
            //printf("LONG file name parsing... last parsing [len:%d]\n", fs->dflen);
            sum = aspFSchecksum(raw);
            if (sum != ((fs->dfstats >> 16) & 0xff)) {
                ret = -11;
                //memset(fs, 0x00, sizeof(struct directnFile_s));
                //printf("checksum error: %x / %x\n", sum, (fs->dfstats >> 16) & 0xff);
                //goto fsparseEnd;
            }
        }
        pstN = fs->dfSFN;
        ret = aspNameCpy(raw, pstN, 0, 11, 1);
        if (ret != 11) {
            memset(fs, 0x00, sizeof(struct directnFile_s));
            //printf("short name copy error ret:%d \n", ret);
            goto fsparseEnd;
        }

        aspFSrmspace(pstN, 11);

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
        } else {
            fs->dfstats = ASPFS_STATUS_ING;
            fs->dfstats |= (ld & 0xf) << 8;
            fs->dfstats |= (raw[13] & 0xff) << 16;
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
            //printf("LONG file name parsing... broken here ret:%d\n", ret);
            //memset(fs, 0x00, sizeof(struct directnFile_s));
            goto fsparseEnd;
        }
        
        if ((ld & 0xf) == 0x01) {
            fs->dfstats = ASPFS_STATUS_DIS;
        } else {
            fs->dfstats = ASPFS_STATUS_ING;
            fs->dfstats |= (ld & 0xf) << 8;
            fs->dfstats |= (raw[13] & 0xff) << 16;
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

void prinfatdir(char *df, int rsz, int shift, int depth, int root, int per)
{
    char *raw=0;
    int ret=0, len=0;
    int offset=0, sec=0;
    char *nxraw=0;
    struct directnFile_s *fs;
    if (!df) return;
    if (!rsz) return;
    if ((shift * 512) > rsz) return;

    raw = df + (shift * 512);
    
    //printf("raw[0x%.8x] df[0x%.8x] rsz[%d]\n", raw, df, rsz);

    fs = malloc(sizeof(struct directnFile_s));
    memset(fs, 0, sizeof(struct directnFile_s));

    len = rsz - shift;
    ret = aspRawParseDir(raw, fs, len);
    while (1) {

        if (fs->dfstats != ASPFS_STATUS_EN) {
        } else if (fs->dfattrib & ASPFS_ATTR_DIRECTORY) {
            printf("**dir**\n");
            if (strcmp(fs->dfSFN, ".") != 0 && 
                 strcmp(fs->dfSFN, "..") != 0 )  {
                if (fs->dflen) {
                    printf("%*s\"%s\"\n", depth, "", fs->dfLFN);
                    
                    printf("%*s\"%s\"\n", depth, "", fs->dfSFN);
                } else {
                    printf("%*s\"%s\"\n", depth, "", fs->dfSFN);
                }
                sec = (fs->dfclstnum - 2) * per;
                offset = root + sec;
                nxraw = df + (offset * 512);
            
                //printf("[0x%.8x]next clst:%d sec:%d offset:0x%.6x \n", nxraw, fs->dfclstnum, sec, offset);
                if ((nxraw - df) > rsz) {
                    printf("overflow %d/%d \n", nxraw - df, rsz);
                } else {
                    prinfatdir(df, rsz, offset, depth+4, root, per);
                }            
            } else {
                if (fs->dflen) {
                    printf("%*s%s\n", depth, "", fs->dfLFN);
                    printf("%*s%s\n", depth, "", fs->dfSFN);
                } else {
                    printf("%*s%s\n", depth, "", fs->dfSFN);
                }
            }

        } else if (fs->dfattrib & ASPFS_ATTR_ARCHIVE) {
            printf("**archive**\n");
            if (fs->dflen) {
                printf("%*s%s\n", depth, "", fs->dfLFN);
                printf("%*s%s\n", depth, "", fs->dfSFN);
            } else {
                printf("%*s%s\n", depth, "", fs->dfSFN);
            }
        } else if (fs->dfattrib & ASPFS_ATTR_HIDDEN) {
            printf("**hidden**\n");
            if (fs->dflen) {
                printf("%*s%s\n", depth, "", fs->dfLFN);
                printf("%*s%s\n", depth, "", fs->dfSFN);
            } else {
                printf("%*s%s\n", depth, "", fs->dfSFN);
            }
        } else {
            printf("unknown! [0x%.4x]\n", fs->dfattrib);     
        }

        raw += ret;
        len -= ret;
        if (len <= 0) break;
        if (ret == 0) break;

        memset(fs, 0, sizeof(struct directnFile_s));
        ret = aspRawParseDir(raw, fs, len);

    }

}

void printdir(char *dir, int depth)
{
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;

    if ((dp = opendir(dir)) == NULL) {
        fprintf(stderr, "Can`t open directory %s\n", dir);
        return ;
    }
    
    chdir(dir);
    while ((entry = readdir(dp)) != NULL) {
        lstat(entry->d_name, &statbuf);
        if (S_ISDIR(statbuf.st_mode)) {
            if (strcmp(entry->d_name, ".") == 0 || 
                strcmp(entry->d_name, "..") == 0 )  
                continue;   
            printf("%*s%s\n", depth, "", entry->d_name);
            printdir(entry->d_name, depth+4);
        } else
            printf("%*s%s\n", depth, "", entry->d_name);
    }
    chdir("..");
    closedir(dp);   
}

static int aspFS_createRoot(struct directnFile_s **root, char *dir)
{
    DIR *dp;
    struct directnFile_s *r = 0;

    printf("[R]open directory [%s]\n", dir);
    if ((dp = opendir(dir)) == NULL) {
        printf("[R]Can`t open directory [%s]\n", dir);
        return (-1);
    }
    printf("[R]open directory [%s] done\n", dir);

    r = (struct directnFile_s *) malloc(sizeof(struct directnFile_s));
    if (!r) {
        return (-2);
    }else {
            printf("[R]alloc root fs done [0x%x]\n", (uint32_t)r);
    }

    r->pa = 0;
    r->br = 0;
    r->ch = 0;
    r->dftype = ASPFS_TYPE_ROOT;
    r->dfattrib = 0;
    r->dfstats = 0;

    r->dflen = strlen(dir);
    printf("[%s] len: %d\n", dir, r->dflen);
    if (r->dflen > 255) r->dflen = 255;
    strncpy(r->dfLFN, dir, r->dflen);

    *root = r;

    return 0;
}

static int aspFS_insertChilds(struct directnFile_s *root)
{
#define TAB_DEPTH   4
    int ret = 0;
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;

    if (!root) {
        printf("[R]root error 0x%x\n", (uint32_t)root);
        ret = -1;
        goto insertEnd;
    }

    printf("[R]open directory [%s]\n", root->dfLFN);

    if ((dp = opendir(root->dfLFN)) == NULL) {
        printf("Can`t open directory [%s]\n", root->dfLFN);
        ret = -2;
        goto insertEnd;
    }

    printf("[R]open directory [%s] done\n", root->dfLFN);
    
    chdir(root->dfLFN);
    while ((entry = readdir(dp)) != NULL) {
        lstat(entry->d_name, &statbuf);
        if (S_ISDIR(statbuf.st_mode)) {
            if (strcmp(entry->d_name, ".") == 0 || 
                strcmp(entry->d_name, "..") == 0 ) {
                continue;   
            }
            printf("%*s%s\n", TAB_DEPTH, "", entry->d_name);
            //printdir(entry->d_name, TAB_DEPTH+4);
            ret = aspFS_insertChildDir(root, entry->d_name);
            if (ret) goto insertEnd;
        } else {
            printf("%*s%s\n", TAB_DEPTH, "", entry->d_name);
            ret = aspFS_insertChildFile(root, entry->d_name);
            if (ret) goto insertEnd;
        }
    }
    chdir("..");    

insertEnd:
    closedir(dp);   

    return ret;
}
static int aspFS_insertChildDir(struct directnFile_s *parent, char *dir)
{
    int ret;
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    struct directnFile_s *r = 0;
    struct directnFile_s *brt = 0;

    r = (struct directnFile_s *) malloc(sizeof(struct directnFile_s));
    if (!r) return (-2);

    r->pa = parent;
    r->br = 0;
    r->ch = 0;
    r->dfattrib = 0;
    r->dfstats = 0;
    r->dftype = ASPFS_TYPE_DIR;

    r->dflen = strlen(dir);
    printf("[%d][%s] len: %d \n", r->dftype, dir, r->dflen);
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

    ret = aspFS_insertChilds(r);
 
    return ret;
}

static int aspFS_insertChildFile(struct directnFile_s *parent, char *str)
{
    struct directnFile_s *r = 0;
    struct directnFile_s *brt = 0;

    r = (struct directnFile_s *) malloc(sizeof(struct directnFile_s));
    if (!r) return (-2);

    r->pa = parent;
    r->br = 0;
    r->ch = 0;
    r->dfattrib = 0;
    r->dfstats = 0;
    r->dftype = ASPFS_TYPE_FILE;

    r->dflen = strlen(str);
    printf("[%d][%s] len: %d \n", r->dftype, str, r->dflen);
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

static int aspFS_createFATRoot(struct sdFAT_s *pfat)
{
    char dir[32] = "/";
    struct directnFile_s *r = 0, *c = 0;

    r = (struct directnFile_s *) malloc(sizeof(struct directnFile_s));
    if (!r) {
        return (-1);
    }
    memset(r, 0, sizeof(struct directnFile_s));

    c = (struct directnFile_s *) malloc(sizeof(struct directnFile_s));
    if (!c) {
        return (-2);
    }
    memset(c, 0, sizeof(struct directnFile_s));

    c->pa = r;
    c->br = 0;
    c->ch = 0;
    c->dftype = ASPFS_TYPE_DIR;
    c->dfattrib = 0;
    c->dfstats = ASPFS_STATUS_EN;
    c->dflen = 2;
    strcpy(c->dfLFN, "..");

    r->pa = 0;
    r->br = 0;
    r->ch = c;
    r->dftype = ASPFS_TYPE_ROOT;
    r->dfattrib = 0;
    r->dfstats = 0;

    r->dflen = strlen(dir);
    if (r->dflen > 255) r->dflen = 255;
    strncpy(r->dfLFN, dir, r->dflen);

    pfat->fatRootdir = r;

    return 0;
}

static int aspFS_insertFATChilds(struct directnFile_s *root, char *dir, int max)
{
#define TAB_DEPTH   4
    int ret = 0, cnt = 0;
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    struct directnFile_s *dfs = 0;

    char *dkbuf=0;

    if (!root) {
        printf("[R]root error 0x%x\n", (uint32_t)root);
        ret = -1;
        goto insertEnd;
    }

    if (root->dflen) {
        printf("[R]open directory [%s]\n", root->dfLFN);
    } else {
        printf("[R]open directory [%s]\n", root->dfSFN);
    }

    if ((!dir) || (max <=0)) {
        printf("[R]Can`t open directory \n");
        ret = -2;
        goto insertEnd;
    }

    dkbuf = dir;
    dfs = malloc(sizeof(struct directnFile_s));
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
            printf("[R]short name: %s \n", dfs->dfSFN);
            if (dfs->dflen > 0) {
                printf("[R]long name: %s, len:%d \n", dfs->dfLFN, dfs->dflen);
            }
            debugPrintDir(dfs);
            aspFS_insertFATChild(root, dfs);
        }

        dkbuf += ret;
        max -= ret;
        cnt++;

        dfs = malloc(sizeof(struct directnFile_s));
        if (!dfs) {
            ret = -3;
            goto insertEnd;
        }
        memset(dfs, 0, sizeof(struct directnFile_s));

        ret = aspRawParseDir(dkbuf, dfs, max);
        if (!ret) break;
        printf("[%d] ret: %d, last:%d \n", cnt, ret, max);
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

static int aspFS_search(struct directnFile_s **dir, struct directnFile_s *root, char *path)
{
    int ret = 0;
    char split = '/';
    char *ch;
    char rmp[16][256];
    int a = 0, b = 0;
    struct directnFile_s *brt;

    ret = strlen(path);
    printf("path[%s] root[%s] len:%d\n", path, root->dfLFN, ret);

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
            printf("%x ", *ch);
        }
        ch++;
        ret --;
    }

    printf("\n a:%d, b:%d \n", a, b);

    for (b = 0; b <= a; b++) {
        printf("[%d.%d]%s \n", a, b, rmp[b]);
    }

    ret = -1;
    b = 0;
    brt = root->ch;
    while (brt) {
        printf("comp[%s] [%s] \n", brt->dfLFN, &rmp[b][0]);
     if (strcmp("..", &rmp[b][0]) == 0) {
            b++;
            if (brt->dftype != ASPFS_TYPE_ROOT) {
                brt = brt->pa;
            }
     } else if (strcmp(".", &rmp[b][0]) == 0) {
            b++;
        } else if (strcmp(brt->dfLFN, &rmp[b][0]) == 0) {
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

    printf("path len: %d, match num: %d, brt:0x%x \n", a, b, (uint32_t)brt);

    while((brt) && (b>=0)) {
        printf("[%d][%s][%s] \n", b, &rmp[b][0], brt->dfLFN);
        b--;
        brt = brt->pa;
    }

    return ret;
}

static int aspFS_showFolder(struct directnFile_s *root)
{
    struct directnFile_s *brt = 0;
    if (!root) return (-1);
    if (root->dftype == ASPFS_TYPE_FILE) return (-2);
    
    printf("%s \n", root->dfLFN);

    brt = root->ch;
    while (brt) {
        printf("|-[%c] %s\n", brt->dftype == ASPFS_TYPE_DIR?'D':'F', brt->dfLFN);
        brt = brt->br;
    }
    return 0;
}

static int aspFS_folderJump(struct directnFile_s **dir, struct directnFile_s *root, char *path)
{
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
            if ((brt->dftype == ASPFS_TYPE_DIR) && 
                 (strcmp(brt->dfLFN, path) == 0)) {
                *dir = brt;
                retval = 4;
                break;
            }
            brt = brt->br;
        }

        if (!brt) retval = (-3);
    }

    return retval;
}

static int test_time_diff(struct timespec *s, struct timespec *e, int unit)
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
 
static int test_gen(char *tx0, char *tx1, int len) {
    int i;
    char ch0[64], az;
    char ch1[64];
    az = 48;
    for (i = 0; i < 63; i++) {
        ch0[i] = az++;
        ch1[i] = az;
    }
    ch0[i] = '\n';
    ch1[i] = '\n';
    
    for (i = 0; i < len; i++) {
        tx0[i] = ch0[i%64];
        tx1[i] = ch1[i%64];
    }

    return i;
}

static void transfer(int fd) 
{ 
    int ret, i, errcnt; 
    uint8_t *tx, tg; 

    tx = malloc(BUFF_SIZE);
    for (i = 0; i < BUFF_SIZE; i++) {
        tx[i] = i & 0xff;
    }

    uint8_t *rx;
    rx = malloc(BUFF_SIZE);

    struct spi_ioc_transfer tr = {  
        .tx_buf = (unsigned long)tx, 
        .rx_buf = (unsigned long)rx, 
        .len = BUFF_SIZE,
        .delay_usecs = delay, 
        .speed_hz = speed, 
        .bits_per_word = bits, 
    }; 
    
    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 1) 
        pabort("can't send spi message"); 
 
    errcnt = 0; i = 0;
    for (ret = 0; ret < BUFF_SIZE; ret++) {
        if (!(ret % 6))
            puts(""); 
        tg = (ret - 0) & 0xff;
        if (rx[ret] != tg) {
              errcnt++;
              i = 1;
        }

        printf("%.2X:%.2X/%d ", rx[ret], tg, i); 
        i  = 0;
    } 
    puts(""); 
    printf(" error count: %d\n", errcnt);
} 

#if 1
static int tx_command(
  int fd, 
  uint8_t *ex_rx, 
  uint8_t *ex_tx, 
  int ex_size)
{
  int ret;
  uint8_t *tx;
  uint8_t *rx;
  int size;

  tx = ex_tx;
  rx = ex_rx;
  size = ex_size;
    
  struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = size,
        .delay_usecs = delay,
        .speed_hz = speed,
        .bits_per_word = bits,
    };
    
  ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
  if (ret < 1)
      pabort("can't send spi message");
  return ret;
}

static int tx_data(int fd, uint8_t *rx_buff, uint8_t *tx_buff, int num, int pksz, int maxsz)
{
    int pkt_size;
    int ret, i, errcnt; 
    int remain;

    struct spi_ioc_transfer *tr = malloc(sizeof(struct spi_ioc_transfer) * num);
    
    uint8_t tg;
    uint8_t *tx = tx_buff;
    uint8_t *rx = rx_buff;  
    pkt_size = pksz;
    remain = maxsz;

    for (i = 0; i < num; i++) {
        remain -= pkt_size;
        if (remain < 0) break;

        tr[i].tx_buf = (unsigned long)tx;
        tr[i].rx_buf = (unsigned long)rx;
        tr[i].len = pkt_size;
        tr[i].delay_usecs = delay;
        tr[i].speed_hz = speed;
        tr[i].bits_per_word = bits;
        
        tx += pkt_size;
        rx += pkt_size;
    }
    
    ret = ioctl(fd, SPI_IOC_MESSAGE(i), tr);
    //printf("tx/rx len: %d\n", ret);

/*
    if (ret < 0) {
        //pabort("can't send spi message");
        ret = 0 - ret;
    }
*/
    
    free(tr);
    return ret;
}

static int tx_data_16(int fd, uint16_t *rx_buff, uint16_t *tx_buff, int num, int pksz, int maxsz)
{
    int pkt_size;
    int ret, i, errcnt; 
    int remain;

    struct spi_ioc_transfer *tr = malloc(sizeof(struct spi_ioc_transfer) * num);
    
    uint8_t tg;
    uint16_t *tx = tx_buff;
    uint16_t *rx = rx_buff;  
    pkt_size = pksz;
    remain = maxsz;

    for (i = 0; i < num; i++) {
        remain -= pkt_size;
        if (remain < 0) break;

        tr[i].tx_buf = (unsigned long)tx;
        tr[i].rx_buf = (unsigned long)rx;
        tr[i].len = pkt_size;
        tr[i].delay_usecs = delay;
        tr[i].speed_hz = speed;
        tr[i].bits_per_word = 16;
        
        tx += pkt_size;
        rx += pkt_size;
    }
    
    ret = ioctl(fd, SPI_IOC_MESSAGE(i), tr);
    if (ret < 0)
        pabort("can't send spi message");
    
    printf("tx/rx len: %d\n", ret);
    
    free(tr);
    return ret;
}

static void _tx_data(int fd, uint8_t *rx_buff, uint8_t *tx_buff, int ex_size, int num)
{
    #define PKT_SIZE 1024
    int ret, i, errcnt; 
    int remain;

    struct spi_ioc_transfer *tr = malloc(sizeof(struct spi_ioc_transfer) * num);
    
    uint8_t tg;
    uint8_t *tx = tx_buff;
    uint8_t *rx = rx_buff;  

    for (i = 0; i < num; i++) {
        tr[i].tx_buf = (unsigned long)tx;
        tr[i].rx_buf = (unsigned long)rx;
        tr[i].len = PKT_SIZE;
        tr[i].delay_usecs = delay;
        tr[i].speed_hz = speed;
        tr[i].bits_per_word = bits;
        
        tx += PKT_SIZE;
        rx += PKT_SIZE;
    }
    
    ret = ioctl(fd, SPI_IOC_MESSAGE(num), tr);
    if (ret < 1)
        pabort("can't send spi message");
    
    printf("tx/rx len: %d\n", ret);
    
    free(tr);
}
#endif
static void print_usage(const char *prog) 
{ 
    printf("Usage: %s [-DsbdlHOLC3]\n", prog); 
    puts("  -D --device   device to use (default /dev/spidev1.1)\n" 
         "  -s --speed    max speed (Hz)\n" 
         "  -d --delay    delay (usec)\n" 
         "  -b --bpw      bits per word \n" 
         "  -l --loop     loopback\n" 
         "  -H --cpha     clock phase\n" 
         "  -O --cpol     clock polarity\n" 
         "  -L --lsb      least significant bit first\n" 
         "  -C --cs-high  chip select active high\n" 
         "  -3 --3wire    SI/SO signals shared\n"
         "  -m --command  command mode\n"
         "  -w --while(1) infinite loop\n"
         "  -p --path     data path\n"); 
    exit(1); 
} 
 
static void parse_opts(int argc, char *argv[]) 
{ 
    while (1) { 
        static const struct option lopts[] = {
            { "device",  1, 0, 'D' }, 
            { "speed",   1, 0, 's' }, 
            { "delay",   1, 0, 'd' }, 
            { "bpw",     1, 0, 'b' }, 
            { "command", 1, 0, 'm' }, 
            { "path   ", 1, 0, 'p' },
            { "whloop",  1, 0, 'w' }, 
            { "loop",    0, 0, 'l' }, 
            { "cpha",    0, 0, 'H' }, 
            { "cpol",    0, 0, 'O' }, 
            { "lsb",     0, 0, 'L' }, 
            { "cs-high", 0, 0, 'C' }, 
            { "3wire",   0, 0, '3' }, 
            { "no-cs",   0, 0, 'N' }, 
            { "ready",   0, 0, 'R' }, 
            { NULL, 0, 0, 0 }, 
        }; 
        int c; 
 
        c = getopt_long(argc, argv, "D:s:d:b:m:w:p:lHOLC3NR", lopts, NULL); 
 
        if (c == -1) 
            break; 
 
        switch (c) { 
        case 'D':   //?? 
                printf(" -D %s \n", optarg);
            device = optarg; 
            break; 
        case 's':   //硉瞯 
              printf(" -s %s \n", optarg);
            speed = atoi(optarg); 
            break; 
        case 'd':   //┑??? 
            delay = atoi(optarg); 
            break; 
        case 'b':   //–ぶ 
            bits = atoi(optarg); 
            break; 
        case 'l':   //癳家Α 
            mode |= SPI_LOOP; 
            break; 
        case 'H':   //?? 
            mode |= SPI_CPHA; 
            break; 
        case 'O':   //??体┦ 
            mode |= SPI_CPOL; 
            break; 
        case 'L':   //lsb 程Τ 
            mode |= SPI_LSB_FIRST; 
            break; 
        case 'C':   //?蔼?キ 
            mode |= SPI_CS_HIGH; 
            break; 
        case '3':
            mode |= SPI_3WIRE; 
            break; 
        case 'N':   //?? 
            mode |= SPI_NO_CS; 
            break; 
        case 'R':   //?审┰?キ氨ゎ?誹?? 
            mode |= SPI_READY; 
            break; 
        case 'm':   //command input
              printf(" -m %s \n", optarg);
            command = strtoul(optarg, NULL, 16);
            break;
        case 'w':   //command input
              printf(" -w %s \n", optarg);
            loop = atoi(optarg);
            break;
        case 'p':   //command input
              printf(" -p %s \n", optarg);
            data_path = optarg;
            break;
        default:    //???? 
            print_usage(argv[0]); 
            break; 
        } 
    } 
} 
static int chk_reply(char * rx, char *ans, int sz)
{
    int i;
    for (i=2; i < sz; i++)
        if (rx[i] != ans[i]) 
            break;

    if (i < sz)
        return i;
    else
        return 0;
}

static int print_f(struct logPool_s *plog, char *head, char *str)
{
    uint32_t logdisplayflag=0;
    int len;
    char ch[1152];
    
    if (!str) return (-1);

    if (head) {
        //if (!strcmp(head, "P2")) return 0;
        sprintf(ch, "[%s] %s", head, str);
    } else {
        sprintf(ch, "%s", str);
    }

    printf("%s",ch);

#if 0
    if (!plog) return (-2);

    logdisplayflag = plog->dislog;
    
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

FILE *find_save(char *dst, char *tmple)
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
            fclose(f);
        }
    }
    f = fopen(dst, "w");
    return f;
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
static int dbgMeta(unsigned int funcbits, struct aspMetaData_s *pmeta) 
{
    char mlog[256];
    char *pch=0;
    
    msync(pmeta, sizeof(struct aspMetaData_s), MS_SYNC);
    sprintf_f(mlog, "********************************************\n");
    print_f(mlogPool, "META", mlog);
    sprintf_f(mlog, "_ debug print , funcBits: 0x%.8x, magic[0]: 0x%.2x magic[1]: 0x%.2x \n", funcbits, pmeta->ASP_MAGIC[0], pmeta->ASP_MAGIC[1]);
    print_f(mlogPool, "META", mlog);
    
    if ((pmeta->ASP_MAGIC[0] != 0x20) || (pmeta->ASP_MAGIC[1] != 0x14)) {
        sprintf_f(mlog, " Warning!!! magic[0]: 0x%.2x magic[1]: 0x%.2x \n", pmeta->ASP_MAGIC[0], pmeta->ASP_MAGIC[1]);
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "********************************************\n");
        print_f(mlogPool, "META", mlog);
        return -2;
    }
    
    if (funcbits == ASPMETA_FUNC_NONE) {
        sprintf_f(mlog, "********************************************\n");
        print_f(mlogPool, "META", mlog);
        return -3;
    }

    if (funcbits & ASPMETA_FUNC_CONF) {
        sprintf_f(mlog, "__ASPMETA_FUNC_CONF__(0x%x & 0x%x = 0x%x)\n", funcbits, ASPMETA_FUNC_CONF, (funcbits & ASPMETA_FUNC_CONF));
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "FILE_FORMAT: 0x%.2x    \n",pmeta->FILE_FORMAT     );          //0x31
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "COLOR_MODE: 0x%.2x      \n",pmeta->COLOR_MODE      );        //0x32
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "COMPRESSION_RATE: 0x%.2x\n",pmeta->COMPRESSION_RATE);   //0x33
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "RESOLUTION: 0x%.2x      \n",pmeta->RESOLUTION      );         //0x34
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "SCAN_GRAVITY: 0x%.2x    \n",pmeta->SCAN_GRAVITY    );       //0x35
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "CIS_MAX_Width: 0x%.2x   \n",pmeta->CIS_MAX_WIDTH   );        //0x36
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "WIDTH_ADJUST_H: 0x%.2x  \n",pmeta->WIDTH_ADJUST_H  );     //0x37
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "WIDTH_ADJUST_L: 0x%.2x  \n",pmeta->WIDTH_ADJUST_L  );      //0x38
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "SCAN_LENGTH_H: 0x%.2x   \n",pmeta->SCAN_LENGTH_H   );      //0x39
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "SCAN_LENGTH_L: 0x%.2x   \n",pmeta->SCAN_LENGTH_L   );       //0x3a
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "INTERNAL_IMG: 0x%.2x    \n",pmeta->INTERNAL_IMG    );         //0x3b
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "AFE_IC_SELEC: 0x%.2x    \n",pmeta->AFE_IC_SELEC    );         //0x3c
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "EXTNAL_PULSE: 0x%.2x    \n",pmeta->EXTNAL_PULSE    );         //0x3d
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "SUP_WRITEBK: 0x%.2x     \n",pmeta->SUP_WRITEBK     );       //0x3e
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "OP_FUNC_00: 0x%.2x      \n",pmeta->OP_FUNC_00      );     //0x70
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "OP_FUNC_01: 0x%.2x      \n",pmeta->OP_FUNC_01      );     //0x71
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "OP_FUNC_02: 0x%.2x      \n",pmeta->OP_FUNC_02      );     //0x72
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "OP_FUNC_03: 0x%.2x      \n",pmeta->OP_FUNC_03      );     //0x73
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "OP_FUNC_04: 0x%.2x      \n",pmeta->OP_FUNC_04      );     //0x74
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "OP_FUNC_05: 0x%.2x      \n",pmeta->OP_FUNC_05      );     //0x75
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "OP_FUNC_06: 0x%.2x      \n",pmeta->OP_FUNC_06      );     //0x76
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "OP_FUNC_07: 0x%.2x      \n",pmeta->OP_FUNC_07      );     //0x77
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "OP_FUNC_08: 0x%.2x      \n",pmeta->OP_FUNC_08      );     //0x78
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "OP_FUNC_09: 0x%.2x      \n",pmeta->OP_FUNC_09      );     //0x79
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "OP_FUNC_10: 0x%.2x      \n",pmeta->OP_FUNC_10      );     //0x7A
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "OP_FUNC_11: 0x%.2x      \n",pmeta->OP_FUNC_11      );     //0x7B
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "OP_FUNC_12: 0x%.2x      \n",pmeta->OP_FUNC_12      );     //0x7C
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "OP_FUNC_13: 0x%.2x      \n",pmeta->OP_FUNC_13      );     //0x7D
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "OP_FUNC_14: 0x%.2x      \n",pmeta->OP_FUNC_14      );     //0x7E
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "OP_FUNC_15: 0x%.2x      \n",pmeta->OP_FUNC_15      );     //0x7F  
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "BLEEDTHROU_ADJUST: 0x%.2x      \n",pmeta->BLEEDTHROU_ADJUST);
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "BLACKWHITE_THSHLD: 0x%.2x      \n",pmeta->BLACKWHITE_THSHLD);
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "SD_CLK_RATE_16: 0x%.2x      \n",pmeta->SD_CLK_RATE_16);
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "PAPER_SIZE: 0x%.2x      \n",pmeta->PAPER_SIZE);
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "JPGRATE_ENG_17: 0x%.2x      \n",pmeta->JPGRATE_ENG_17);
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "OP_FUNC_18: 0x%.2x      \n",pmeta->OP_FUNC_18);
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "OP_FUNC_19: 0x%.2x      \n",pmeta->OP_FUNC_19);
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "OP_FUNC_20: 0x%.2x      \n",pmeta->OP_FUNC_20);
        print_f(mlogPool, "META", mlog);
    }
    
    if (funcbits & ASPMETA_FUNC_CROP) {
        sprintf_f(mlog, "__ASPMETA_FUNC_CROP__(0x%x & 0x%x = 0x%x)\n", funcbits, ASPMETA_FUNC_CROP, (funcbits & ASPMETA_FUNC_CROP));
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "CROP_POSX_01: %d, %d\n", msb2lsb(&pmeta->CROP_POS_1) >> 16, msb2lsb(&pmeta->CROP_POS_1) & 0xffff);                      //byte[68]
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "CROP_POSX_02: %d, %d\n", msb2lsb(&pmeta->CROP_POS_2) >> 16, msb2lsb(&pmeta->CROP_POS_2) & 0xffff);                      //byte[72]
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "CROP_POSX_03: %d, %d\n", msb2lsb(&pmeta->CROP_POS_3) >> 16, msb2lsb(&pmeta->CROP_POS_3) & 0xffff);                      //byte[76]
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "CROP_POSX_04: %d, %d\n", msb2lsb(&pmeta->CROP_POS_4) >> 16, msb2lsb(&pmeta->CROP_POS_4) & 0xffff);                      //byte[80]
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "CROP_POSX_05: %d, %d\n", msb2lsb(&pmeta->CROP_POS_5) >> 16, msb2lsb(&pmeta->CROP_POS_5) & 0xffff);                      //byte[84]
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "CROP_POSX_06: %d, %d\n", msb2lsb(&pmeta->CROP_POS_6) >> 16, msb2lsb(&pmeta->CROP_POS_6) & 0xffff);                      //byte[88]
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "CROP_POSX_07: %d, %d\n", msb2lsb(&pmeta->CROP_POS_7) >> 16, msb2lsb(&pmeta->CROP_POS_7) & 0xffff);                      //byte[92]
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "CROP_POSX_08: %d, %d\n", msb2lsb(&pmeta->CROP_POS_8) >> 16, msb2lsb(&pmeta->CROP_POS_8) & 0xffff);                      //byte[96]
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "CROP_POSX_09: %d, %d\n", msb2lsb(&pmeta->CROP_POS_9) >> 16, msb2lsb(&pmeta->CROP_POS_9) & 0xffff);                      //byte[100]
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "CROP_POSX_10: %d, %d\n", msb2lsb(&pmeta->CROP_POS_10) >> 16, msb2lsb(&pmeta->CROP_POS_10) & 0xffff);                      //byte[104]
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "CROP_POSX_11: %d, %d\n", msb2lsb(&pmeta->CROP_POS_11) >> 16, msb2lsb(&pmeta->CROP_POS_11) & 0xffff);                      //byte[108]
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "CROP_POSX_12: %d, %d\n", msb2lsb(&pmeta->CROP_POS_12) >> 16, msb2lsb(&pmeta->CROP_POS_12) & 0xffff);                      //byte[112]
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "CROP_POSX_13: %d, %d\n", msb2lsb(&pmeta->CROP_POS_13) >> 16, msb2lsb(&pmeta->CROP_POS_13) & 0xffff);                      //byte[116]
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "CROP_POSX_14: %d, %d\n", msb2lsb(&pmeta->CROP_POS_14) >> 16, msb2lsb(&pmeta->CROP_POS_14) & 0xffff);                      //byte[120]
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "CROP_POSX_15: %d, %d\n", msb2lsb(&pmeta->CROP_POS_15) >> 16, msb2lsb(&pmeta->CROP_POS_15) & 0xffff);                      //byte[124]
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "CROP_POSX_16: %d, %d\n", msb2lsb(&pmeta->CROP_POS_16) >> 16, msb2lsb(&pmeta->CROP_POS_16) & 0xffff);                      //byte[128]
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "CROP_POSX_17: %d, %d\n", msb2lsb(&pmeta->CROP_POS_17) >> 16, msb2lsb(&pmeta->CROP_POS_17) & 0xffff);                      //byte[132]
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "CROP_POSX_18: %d, %d\n", msb2lsb(&pmeta->CROP_POS_18) >> 16, msb2lsb(&pmeta->CROP_POS_18) & 0xffff);                      //byte[136]
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "YLine_Gap: %.d      \n",pmeta->YLine_Gap); 
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "Start_YLine_No: %d      \n",pmeta->Start_YLine_No); 
        print_f(mlogPool, "META", mlog);
        pch = (char *)&pmeta->YLines_Recorded;
        sprintf_f(mlog, "YLines_Recorded: %d      \n",(pch[0] << 8) | pch[1]); 
        print_f(mlogPool, "META", mlog);
    }

    if (funcbits & ASPMETA_FUNC_IMGLEN) {
        sprintf_f(mlog, "__ASPMETA_FUNC_IMGLEN__(0x%x & 0x%x = 0x%x)\n", funcbits, ASPMETA_FUNC_IMGLEN, (funcbits & ASPMETA_FUNC_IMGLEN));
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "SCAN_IMAGE_LEN: %d\n", msb2lsb(&pmeta->SCAN_IMAGE_LEN));                      //byte[124]        
        print_f(mlogPool, "META", mlog);
    }

    if (funcbits & ASPMETA_FUNC_SDFREE) {      
        sprintf_f(mlog, "__ASPMETA_FUNC_SDFREE__(0x%x & 0x%x = 0x%x)\n", funcbits, ASPMETA_FUNC_SDFREE, (funcbits & ASPMETA_FUNC_SDFREE));
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "FREE_SECTOR_ADD: %d\n", msb2lsb(&pmeta->FREE_SECTOR_ADD));                      //byte[128]            
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "FREE_SECTOR_LEN: %d\n", msb2lsb(&pmeta->FREE_SECTOR_LEN));                      //byte[132]        
        print_f(mlogPool, "META", mlog);
    }

    if (funcbits & ASPMETA_FUNC_SDUSED) {
        sprintf_f(mlog, "__ASPMETA_FUNC_SDUSED__(0x%x & 0x%x = 0x%x)\n", funcbits, ASPMETA_FUNC_SDUSED, (funcbits & ASPMETA_FUNC_SDUSED));
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "USED_SECTOR_ADD: %d\n", msb2lsb(&pmeta->USED_SECTOR_ADD));                      //byte[136]            
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "USED_SECTOR_LEN: %d\n", msb2lsb(&pmeta->USED_SECTOR_LEN));                      //byte[140]        
        print_f(mlogPool, "META", mlog);
    }

    if (funcbits & ASPMETA_FUNC_SDRD) {
        sprintf_f(mlog, "__ASPMETA_FUNC_SDRD__(0x%x & 0x%x = 0x%x)\n", funcbits, ASPMETA_FUNC_SDRD, (funcbits & ASPMETA_FUNC_SDRD));
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "SD_RW_SECTOR_ADD: %d\n", msb2lsb(&pmeta->SD_RW_SECTOR_ADD));                      //byte[144]            
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "SD_RW_SECTOR_LEN: %d\n", msb2lsb(&pmeta->SD_RW_SECTOR_LEN));                      //byte[148]        
        print_f(mlogPool, "META", mlog);
    }

    if (funcbits & ASPMETA_FUNC_SDWT) {
        sprintf_f(mlog, "__ASPMETA_FUNC_SDWT__(0x%x & 0x%x = 0x%x)\n", funcbits, ASPMETA_FUNC_SDWT, (funcbits & ASPMETA_FUNC_SDWT));
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "SD_RW_SECTOR_ADD: %d\n", msb2lsb(&pmeta->SD_RW_SECTOR_ADD));                      //byte[136]            
        print_f(mlogPool, "META", mlog);
        sprintf_f(mlog, "SD_RW_SECTOR_LEN: %d\n", msb2lsb(&pmeta->SD_RW_SECTOR_LEN));                      //byte[140]        
        print_f(mlogPool, "META", mlog);
    }

    sprintf_f(mlog, "********************************************\n");
    print_f(mlogPool, "META", mlog);
    return 0;
}

FILE *find_open(char *dst, char *tmple)
{
    FILE *f;
    sprintf(dst, tmple);
    f = fopen(dst, "w");
    return f;
}

static void data_process(char *rx, char *tx, FILE *fp, int fd, int pktsz, int num)
{
    int ret;
    int wtsz;
    int trunksz;
    trunksz = 1024 * num;

    ret = tx_data(fd, rx, tx, num, pktsz, 1024*1024);
    printf("%d rx %d\n", fd, ret);
    wtsz = fwrite(rx, 1, ret, fp);
    printf("%d wt %d\n", fd, wtsz);


}

static int usb_nonblock_set (int sfd)
{
    int val, ret;
    ret = fcntl (sfd, F_GETFL, 0);
    if (ret == -1)
    {
        perror ("fcntl");
        return -1;
    }

    val = ret;  
    val |= O_NONBLOCK;
    ret = fcntl (sfd, F_SETFL, val);
    if (ret == -1)
    {
        perror ("fcntl");
        return -1;
    }

    return 0;
}

static int insert_cbw(char *cbw, char cmd, char opc, char dat)
{
    if (!cbw) return -1;

    cbw[15] = cmd;
    cbw[16] = opc;
    cbw[17] = dat;

    return 0;
}
#define USB_TX_LOG 1
static int usb_send(char *pts, int usbfd, int len)
{
    int ret=0, send=0, cnt=0;
    struct pollfd pllfd[1];

#if 0
    if (!(len % 512)) {
        len += 1;
    }
#endif

#if 0
    if (!pts) return -1;
    if (!usbfd) return -2;

    pllfd[0].fd = usbfd;
    pllfd[0].events = POLLOUT;
    
    while(1) {
        ret = poll(pllfd, 1, -1);
        //printf("[UW] usb poll ret: %d \n", ret);
        if (ret < 0) {
            printf("[UW] usb poll failed ret: %d\n", ret);
            break;
        }

        if (ret && (pllfd[0].revents & POLLOUT)) {
            
            send = write(pllfd[0].fd, pts, len);
#if USB_TX_LOG
            printf("[UW] usb write %d bytes, ret: %d (1)\n", len, send);
#endif
            break;
        }                
    }
#else
    send = write(usbfd, pts, len);
    while (send < 0) {
        cnt++;
        if (cnt > 3) break;
        perror("[USB] send error !!\n"); 
        printf("epoll create failed, errno: %d ret: %d \n", errno, send);

        usleep(100000);

        send = write(usbfd, pts, len);
    }

    printf("[USB] usb write %d bytes, ret: %d (2)\n", len, send);
#endif
    return send;    
}

static int usb_read(char *ptr, int usbfd, int len)
{
    int ret=0, recv=0, cnt=0;
#if 0
    struct pollfd pllfd[1];
    if (!ptr) return -1;
    if (!usbfd) return -2;

    pllfd[0].fd = usbfd;
    pllfd[0].events = POLLIN;
    
    while(1) {
        ret = poll(pllfd, 1, -1);
        //printf("[UR] usb poll ret: %d \n", ret);
        if (ret < 0) {
            printf("[UR] usb poll failed ret: %d\n", ret);
            break;
        }

        if (ret && (pllfd[0].revents & POLLIN)) {
            
            recv = read(pllfd[0].fd, ptr, len);
#if USB_TX_LOG
            printf("[UR] usb read %d bytes, ret: %d (1)\n", len, recv);
#endif
            break;
        }                
    }
#else
    recv = read(usbfd, ptr, len);
    while (recv < 0) {
        cnt++;
        if (cnt > 2) break;

        perror("[USB] read error !!\n"); 
        printf("read failed, errno: %d ret: %d \n", errno, recv);

        //usleep(100000);
        
        recv = read(usbfd, ptr, len);
    }
    
    //printf("[USB] usb read %d bytes, ret: %d (2)\n", len, recv);
#endif
    
    return recv;    
}

static int mslvBMPClip(struct slvbitMapHeader_s *dst, struct slvbitMapHeader_s *src, int x, int y)
{
    int srcW=0, srcH=0;
    int dstW=0, dstH=0;
    int offset=0, dstImgSize=0, linebytes=0;
    char *srcImg=0, *dstImg=0;
    int bix=0, biy=0;
    int offx=0, offy=0;
    char *ybengin=0, *xbegin=0;
    char *destAddr=0;
    int linebyDst=0;
    char *pdh=0;

    srcW = src->aspbiWidth;
    srcH = src->aspbiHeight;

    dstW= dst->aspbiWidth;
    dstH = dst->aspbiHeight;

    if ((x+dstW) > srcW) return -1;
    if ((y+dstH) > srcH) return -2;

    offset = src->aspbhRawoffset;
    srcImg = src->bitbuf+offset;

    linebytes = !(srcW % 4) ? srcW : srcW - (srcW % 4) + 4;
    linebyDst = !(dstW % 4) ? dstW : dstW - (dstW % 4) + 4;
    printf("allign src: %d / %d, dst: %d / %d\n", linebytes, srcW, linebyDst, dstW);
    dstImgSize = offset + linebyDst * dstH;
    dst->aspbhSize = dstImgSize;
    dst->bitbuf = malloc(dstImgSize);
    
    if (!dst->bitbuf) return -3;
    memset(dst->bitbuf, 0, dstImgSize);

    dstImg = dst->bitbuf + offset;
    memcpy(dst->bitbuf, src->bitbuf, offset);
    
    destAddr = dstImg;
    for (biy=0; biy < dstH; biy++) {
        offy = biy + y;
        ybengin = srcImg + (offy * linebytes);
        xbegin = ybengin + x;
        destAddr = dstImg + (linebyDst * biy);
        for (bix=0; bix < dstW; bix++) {
            *destAddr = *xbegin;
            destAddr ++;
            xbegin ++;
        }
    }
    dst->aspbiRawSize = linebyDst * dstH;
    pdh = &dst->aspbmpMagic[2];
    memcpy(dst->bitbuf, pdh, sizeof(struct slvbitMapHeader_s) - 6);

    return 0;
}


#define DBG_PHY2VIR 0
int phy2vir(uint32_t *pvir, uint32_t phy, int physize, int memfd)
{
    char *ps8_page_start_addr;
    char *curAddr;
    int fd , r, pageSize;
    unsigned int u32_start_addr , u32_len , u32_page_seek_cur , data;

    
    pageSize = getpagesize();
#if 0//DBG_PHY2VIR
    printf("[MEM] get page size: %d \n", pageSize);
#endif
    u32_start_addr = phy;
    u32_len = physize;
    u32_page_seek_cur = u32_start_addr % pageSize;

#if DBG_PHY2VIR
    printf("[MEM] get start addr: 0x%.8x, len: %d, seek: %d\n", u32_start_addr, u32_len, u32_page_seek_cur);
    printf("Start addr : 0x%.8x  , length : %d \n" , u32_start_addr - u32_page_seek_cur , u32_page_seek_cur + u32_len );
#endif

    //sync();
    
    ps8_page_start_addr = mmap( 0 ,                                   // Start addr in file
                           u32_page_seek_cur + u32_len , // len
                           PROT_READ | PROT_WRITE , // mode
                           MAP_SHARED ,                  // flag
                           memfd ,
                           u32_start_addr - u32_page_seek_cur ); // start addr in page system
  
    
    if( MAP_FAILED == ps8_page_start_addr ) {
        printf("mmap() errorn");
        close(memfd);      
        return -1;
    }

    curAddr = ps8_page_start_addr + u32_page_seek_cur;
    *pvir = (uint32_t)curAddr;
#if DBG_PHY2VIR
    printf("[MEM] get addr s:0x%.8x c:0x%.8x\n", ps8_page_start_addr, curAddr);
#endif
    //munmap( ps8_page_start_addr , u32_len );
  
    return 0;
}

static int mslvdbgBitmapHeader(struct slvbitMapHeader_s *ph, int len) 
{
    char mlog[256];

    msync(ph, sizeof(struct slvbitMapHeader_s), MS_SYNC);

    sprintf_f(mlog, "********************************************\n");
    print_f(mlogPool, "BMP", mlog);

    sprintf_f(mlog, "debug print bitmap header length: %d\n", len);
    print_f(mlogPool, "BMP", mlog);

    sprintf_f(mlog, "MAGIC NUMBER: [%c] [%c] \n",ph->aspbmpMagic[2], ph->aspbmpMagic[3]);         
    print_f(mlogPool, "BMP", mlog);

    sprintf_f(mlog, "FILE TOTAL LENGTH: [%d] \n",ph->aspbhSize);                                                 // mod
    print_f(mlogPool, "BMP", mlog);

    sprintf_f(mlog, "HEADER TOTAL LENGTH: [%d] \n",ph->aspbhRawoffset);          
    print_f(mlogPool, "BMP", mlog);

    sprintf_f(mlog, "INFO HEADER LENGTH: [%d] \n",ph->aspbiSize);          
    print_f(mlogPool, "BMP", mlog);

    sprintf_f(mlog, "WIDTH: [%d] \n",ph->aspbiWidth);                                                          // mod
    print_f(mlogPool, "BMP", mlog);

    sprintf_f(mlog, "HEIGHT: [%d] \n",ph->aspbiHeight);                                                        // mod
    print_f(mlogPool, "BMP", mlog);
    
    sprintf_f(mlog, "NUM OF COLOR PLANES: [%d] \n",ph->aspbiCPP & 0xffff);          
    print_f(mlogPool, "BMP", mlog);

    sprintf_f(mlog, "BITS PER PIXEL: [%d] \n",ph->aspbiCPP >> 16);          
    print_f(mlogPool, "BMP", mlog);

    sprintf_f(mlog, "COMPRESSION METHOD: [%d] \n",ph->aspbiCompMethd);          
    print_f(mlogPool, "BMP", mlog);

    sprintf_f(mlog, "SIZE OF RAW: [%d] \n",ph->aspbiRawSize);                                            // mod
    print_f(mlogPool, "BMP", mlog);

    sprintf_f(mlog, "HORIZONTAL RESOLUTION: [%d] \n",ph->aspbiResoluH);          
    print_f(mlogPool, "BMP", mlog);

    sprintf_f(mlog, "VERTICAL RESOLUTION: [%d] \n",ph->aspbiResoluV);          
    print_f(mlogPool, "BMP", mlog);

    sprintf_f(mlog, "NUM OF COLORS IN CP: [%d] \n",ph->aspbiNumCinCP);          
    print_f(mlogPool, "BMP", mlog);

    sprintf_f(mlog, "NUM OF IMPORTANT COLORS: [%d] \n",ph->aspbiNumImpColor);          
    print_f(mlogPool, "BMP", mlog);

    sprintf_f(mlog, "********************************************\n");
    print_f(mlogPool, "BMP", mlog);

    return 0;
}

int main(int argc, char *argv[]) 
{ 
static char spi0[] = "/dev/spidev32765.0"; 
#if SPI1_ENABLE
static char spi1[] = "/dev/spidev32766.0"; 
#else
static char *spi1 = 0;
#endif
//static char data_wifi[] = "/mnt/mmc2/tmp/1.jpg"; 
static char data_save[] = "/mnt/mmc2/rx/%d.bin"; 
static char path[256];

    uint32_t bitset;
    int sel, arg0 = 0, arg1 = 0, arg2 = 0, arg3 = 0, arg4 = 0;
    int fd, ret; 
    int buffsize;
    uint8_t *tx_buff[2], *rx_buff[2];
    FILE *fp;

    fp = find_save(path, data_save);
    if (!fp) {
        printf("find save dst failed ret:%d\n", (uint32_t)fp);
        goto end;
    } else
        printf("find save dst succeed[%s] ret:%d\n", path, (uint32_t)fp);

    /* scanner default setting */
    mode &= ~SPI_MODE_3;
    printf("mode initial: 0x%x\n", mode & SPI_MODE_3);
    mode |= SPI_MODE_1;

    fd = open(device, O_RDWR);  //ゴ???ゅン 
    if (fd < 0) {
        //pabort("can't open device"); 
        printf("can't open device [%s] \n", device); 
        fd = 0;
    }

    if (argc > 1) {
        printf(" [1]:%s \n", argv[1]);
        sel = atoi(argv[1]);
    }
    if (argc > 2) {
        printf(" [2]:%s \n", argv[2]);
        arg0 = atoi(argv[2]);
    }
    if (argc > 3) {
        printf(" [3]:%s \n", argv[3]);
        arg1 = atoi(argv[3]);
    }
    if (argc > 4) {
        printf(" [4]:%s \n", argv[4]);
        arg2 = atoi(argv[4]);
    }
    if (argc > 5) {
        printf(" [5]:%s \n", argv[5]);
        arg3 = atoi(argv[5]);
    }
    if (argc > 6) {
        printf(" [6]:%s \n", argv[6]);
        arg4 = atoi(argv[6]);
    }   

    buffsize = 1*1024*1024;

    tx_buff[0] = malloc(buffsize);
    if (tx_buff[0]) {
        printf(" tx buff 0 alloc success!!\n");
    } else {
        printf("Error!!buff alloc failed!!");
        goto end;
    }
    memset(tx_buff[0], 0xf0, buffsize);

    rx_buff[0] = malloc(buffsize);
    if (rx_buff[0]) {
        printf(" rx buff 0 alloc success!!\n");
    } else {
        printf("Error!!buff alloc failed!!");
        goto end;
    }
    memset(rx_buff[0], 0xf0, buffsize);

    tx_buff[1] = malloc(buffsize);
    if (tx_buff[1]) {
        printf(" tx buff 1 alloc success!!\n");
    } else {
        printf("Error!!buff alloc failed!!");
        goto end;
    }
    memset(tx_buff[1], 0xf0, buffsize);

    rx_buff[1] = malloc(buffsize);
    if (rx_buff[1]) {
        printf(" rx buff 1 alloc success!!\n");
    } else {
        printf("Error!!buff alloc failed!!");
        goto end;
    }

    memset(rx_buff[1], 0, buffsize);

    ret = test_gen(tx_buff[0], tx_buff[1], buffsize);
    printf("tx pattern gen size: %d/%d \n", ret, buffsize);
    
    int fd0, fd1;
    fd0 = open(spi0, O_RDWR);
    if (fd0 < 0) 
        printf("can't open device[%s]\n", spi0); 
    else 
        printf("open device[%s]\n", spi0); 
        
    if (spi1!=0) {
        fd1 = open(spi1, O_RDWR);
        if (fd1 <= 0) {
                fd1 = 0;
                printf("can't open device[%s]\n", spi1); 
        } else {
            printf("open device[%s]\n", spi1); 
        }
    } else {
        fd1 = 0;
    }

    int fm[2] = {fd0, fd1};
    ret = spi_config(fm[0], SPI_IOC_WR_MODE, &mode);
    if (ret == -1) {
        //pabort("can't set spi mode"); 
        printf("can't set spi mode"); 
    }

    ret = spi_config(fm[0], SPI_IOC_RD_MODE, &mode);
    if (ret == -1) {
        //pabort("can't get spi mode"); 
        printf("can't get spi mode"); 
    }
    
    if (fm[1]) {
        ret = spi_config(fm[1], SPI_IOC_WR_MODE, &mode);
        if (ret == -1) {
            printf("can't set spi mode"); 
        }
        ret = spi_config(fm[1], SPI_IOC_RD_MODE, &mode);
        if (ret == -1) {
            printf("can't get spi mode"); 
        }
    }
    char rxans[512];
    char tx[512];
    char rx[512];
    int i;
    for (i = 0; i < 512; i++) {
        rxans[i] = i & 0x95;
        tx[i] = i & 0x95;
    }

    
    bitset = 0;
    spi_config(fm[0], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL

    bitset = 1;
    spi_config(fm[1], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL

#define RING_BUFF_NUM (500)
#define PT_BUF_SIZE (65536)//32768
#define MAX_EVENTS (2)
#define EPOLLLT (0)
#define CBW_CMD_SEND_OPCODE   0x11
#define CBW_CMD_START_SCAN    0x12
#define CBW_CMD_READY   0x08
#define  OP_POWER_ON         0x01
#define  OP_QUERY            0x02
#define  OP_READY             0x03
#define  OP_SINGLE           0x04
#define  OP_DUPLEX           0x05
#define  OP_ACTION          0x06
//#define  OP_FIH               0x07
#define  OP_SEND_BACK        0x08
#define  OP_Multi_Single     0x09
#define  OP_Multi_DUPLEX     0x0A
#define  OP_Hand_Scan       0x0B
#define  OP_Note_Scan         0x0C
#define  OP_Multi_Hand_Scan  0x0D
#define  OP_META             0x4C
#define OP_META_Sub0      0x0
#define OP_META_Sub1      0x1
#define  OPSUB_WiFi_only      0x01
#define  OPSUB_SD_only       0x02
#define  OPSUB_WiFi_SD       0x03
#define OPSUB_WBC_Proc    0x84
#define  OPSUB_USB_Scan    0x85
#define  OPSUB_DualStream_WiFi_only  0x86
#define  OPSUB_DualStream_SD_only   0x87
#define  OPSUB_Hand_Scan     0x89
#define  OPSUB_Enc_Dec_Test   0x8A
    if (sel == 37){ /* dollar number recognize */
        struct pollfd ptfd[1];
        char ptfilepath[256];
        char selecpath[] = "/mnt/mmc2/dollar/sample/256.bmp";
        char numberimg[] = "/mnt/mmc2/dollar/sample/d100.bmp";
        static char ptfileSave[] = "/mnt/mmc2/dollar/clip%.3d.bmp";
        char *filename=0, *ptfileSend=0;
        int dw=200, dh=100, dx=0, dy=0;
        
        FILE *fdollar=0, *fsave=0;
        int ret=0, len=0;
        char *buf=0, *ph=0, *pd=0;
        struct slvbitMapHeader_s *bhead=0, *bdst=0;
        clock_t ckx[1000];
        int icx=0;

        for (icx=0; icx < 1000; icx++) {
            usleep(3000);
            ckx[icx] = clock();
        }

        for (icx=0; icx < 1000; icx++) {
            printf("%ld ", ckx[icx]);
            if (!(icx+1)%10) printf("\n");
        }

        if (argc > 6) {
            printf(" input file: %s \n", argv[6]);
            filename = argv[6];
        } else {
            filename = selecpath;
        }
        
        if (argc > 2) {
            dw = arg0;
        }

        if (argc > 3) {
            dh = arg1;
        }

        if (argc > 4) {
            dx = arg2;
        }

        if (argc > 5) {
            dy = arg3;
        }
        
        printf(" clip info dw:%d dh:%d dx:%d dy:%d \n", dw, dh, dx, dy);
        
        bhead = malloc(sizeof(struct slvbitMapHeader_s));
        memset(bhead, 0, sizeof(struct slvbitMapHeader_s));
        ph = &bhead->aspbmpMagic[2];

        bdst = malloc(sizeof(struct slvbitMapHeader_s));
        memset(bdst, 0, sizeof(struct slvbitMapHeader_s));
        pd = &bdst->aspbmpMagic[2];
        
        printf(" open file [%s] \n", filename);
        fdollar = fopen(filename, "r");
       
        if (!fdollar) {
            printf(" [%s] file open failed \n", filename);
            goto err;
        }   
        printf(" [%s] file open succeed \n", filename);

        ret = fseek(fdollar, 0, SEEK_END);
        if (ret) {
            printf(" file seek failed!! ret:%d \n", ret);
            goto err;
        }

        len = ftell(fdollar);
        printf(" file [%s] size: %d \n", filename, len);
        
        buf = malloc(len);
        if (buf) {
            printf(" send buff alloc succeed! size: %d \n", len);
        } else {
            printf(" send buff alloc failed! size: %d \n", len);
            goto err;
        }

        ret = fseek(fdollar, 0, SEEK_SET);
        if (ret) {
            printf(" file seek failed!! ret:%d \n", ret);
            goto err;
        }
        
        ret = fread(buf, 1, len, fdollar);
        printf(" file read size: %d/%d \n", ret, len);

        //printf(" dump buffer size: %d \n", 512);
        //shmem_dump(buf, 512);

        len = sizeof(struct slvbitMapHeader_s) - 6;
        memcpy(ph, buf, len);
        bhead->bitbuf = buf;
        
        memcpy(pd, buf, len);
        bdst->aspbiWidth = dw;
        bdst->aspbiHeight = dh;
        bdst->bitbuf = 0;
        
        mslvdbgBitmapHeader(bhead, len);

        dy = bhead->aspbiHeight - dy;
        if (dy < 0) goto err;
        
        ret = mslvBMPClip(bdst, bhead, dx, dy);
        if (ret) {
            printf(" BMP clip ret: %d \n", ret);
            goto err;    
        }

        fsave = find_save(ptfilepath, ptfileSave);
        if (!fsave) {
            goto err;    
        }

        ret = fwrite(bdst->bitbuf, 1, bdst->aspbhSize, fsave);
        printf("total write size: %d / %d \n", ret, bdst->aspbhSize);

        mslvdbgBitmapHeader(bdst, len);

err:
        sync();
        if (fsave) fclose(fsave);
        if (buf) free(buf);
        if (bdst->bitbuf) free(bdst->bitbuf);
        if (bdst) free(bdst);
        if (bhead) free(bhead);
        if (fdollar) fclose(fdollar);
        goto end;
    }
#define MODULE_NAME "/dev/mem"

    if (sel == 36){ /* usb printer duplex scan */
        static char ptdevpath[] = "/dev/usb/lp0";
        static char ptfileMeta[] = "/mnt/mmc2/usb/meta.bin";
        static char ptfileSave[] = "/mnt/mmc2/output/image%.3d.jpg";
        char ptfilepath[128];
        int ptret=0;
        char *ptm=0;
        
        FILE *fsave=0, *fmeta=0;
        struct aspMetaData_s meta, *pmeta=0;
        char CBW[32] = {0x55, 0x53, 0x42, 0x43, 0x11, 0x22, 0x33, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        int mfd;
        int usbids[2];
        int ix=0, acucnt=0;
        int usb0pvid[2];
        uint32_t *phyaddr0=0, ut32=0;
        uint32_t *viraddr0=0, vt32=0;
        uint32_t *tbl0, *tbl1;
        char *chvir=0, *ptrecv=0, *pkcbw=0;
        char opc=0, dat=0, cswst=0, chr=0, csworg=0;
        int recvsz=0, acusz=0, tcnt=0, usbrun=0, usbfolw=0, len=0, wrtsz=0, loopcnt=0;
        int usCost=0;
        double throughput=0.0;
        struct timespec utstart, utend;
        int pipeInfo[2], pipeBack[2];
        int pid=0;
        char ch=0;
        int looptimes=0;

        if (arg0 > 0) {
            looptimes = arg0;
        } else {
            looptimes = 1;
        }

        printf("loop time: [%d] !!! \n", looptimes);
            
        pipe(pipeInfo);
        pipe(pipeBack);
        
        ptrecv = malloc(32);
        memset(ptrecv, 0, 32);

        pkcbw = malloc(96);
        memset(pkcbw, 0, 96);
        
        mfd = open(MODULE_NAME , O_RDWR);
        if(mfd < 0) {
            printf("open() %s errorn" , MODULE_NAME);
        } else {
            printf("open [%s] succeed!!!! \n", MODULE_NAME);
        }
        
        usbids[0] = open(ptdevpath, O_RDWR);
        if (usbids[0] < 0) {
            printf("can't open device[%s]\n", ptdevpath); 
            close(usbids[0]);
            goto end;
        }
        else {
            printf("open device[%s]\n", ptdevpath); 
        }

        pmeta = &meta;
        ptm = (char *)pmeta;

        fmeta = fopen(ptfileMeta, "r");
        if (fmeta) {
            printf("open meta sample file [%s] succeed \n", ptfileMeta);
            ptret = fread(ptm, 1, 512, fmeta);
            printf("read meta size: %d /%d \n", ptret, 512);

            shmem_dump(ptm, 512);
            dbgMeta(msb2lsb(&pmeta->FUNC_BITS), pmeta);
        } else {
            printf("open meta sample file [%s] failed \n", ptfileMeta);
            memset(ptm, 0, sizeof(meta));
            meta.ASP_MAGIC[0] = 0x20;
            meta.ASP_MAGIC[1] = 0x14;
        }

        ret = USB_IOCT_GET_VID_PID(usbids[0], usb0pvid);
        if (ret < 0) {
            printf("[DVF] can't get vid pid for [%s]\n", ptdevpath); 
            close(usbids[0]);
            goto end;
        }
        
        ix = RING_BUFF_NUM;
        ret = USB_IOCT_LOOP_BUFF_CREATE(usbids[0], &ix);
        if (ret < 0) {
            printf("[DVF] can't create buff failed, size: %d [%s]\n", RING_BUFF_NUM, ptdevpath); 
            close(usbids[0]);
            goto end;
        }
        
        phyaddr0 = malloc(RING_BUFF_NUM*4);
        viraddr0 = malloc(RING_BUFF_NUM*4);
        if (!phyaddr0) {
            printf("[DVF] allocate memory failed, size: %d [%s]\n", RING_BUFF_NUM*4, ptdevpath); 
            close(usbids[0]);
            goto end;
        }
        
        ret = USB_IOCT_LOOP_BUFF_PROBE(usbids[0], phyaddr0);
        if (ret < 0) {
            printf("[DVF] can't probe phy addr, size: %d [%s]\n", RING_BUFF_NUM, ptdevpath); 
            close(usbids[0]);
            goto end;
        }
        
        ix = 0;
        printf("[DVF] addr0: \n%d: ", ix);
        for (ix=0; ix < RING_BUFF_NUM; ix++) {
            ut32 = phyaddr0[ix];
            printf("p:0x%.8x ", ut32);
        
            ret = phy2vir(&vt32, ut32, PT_BUF_SIZE, mfd);
            if (ret < 0) {
                printf("[DVF] addr0 phy 2 vir error!!! ret: %d \n", ret);
                break;
            }
            
            viraddr0[ix] = vt32;
            printf("v:0x%.8x ", vt32);
            if ((ix+1) % 4 == 0) {
                printf("\n%d: ", ix);
            }
        }

        printf("[DVF] vaddr0 val check:\n");        
        for (ix=0; ix < RING_BUFF_NUM; ix++) {
            chvir = (char *) viraddr0[ix];
        
            //printf("0x%.2x ", chvir[0]);
            if (chvir[0] != (ix & 0xff)) {
                printf("[DVF] 0e: %d-0x%.2x ", ix, chvir[0]);            
            }
            
            /*
            for (ind=1; ind < PT_BUF_SIZE; ind++) {
                chvir[ind] = (ix + ind) & 0xff;
            }
            
            msync(chvir, PT_BUF_SIZE, MS_SYNC);
            */
            
            //shmem_dump(chvir, 32);

            if ((ix+1) % 16 == 0) {
                //printf("\n");
            }
        }
        
        printf("[DVF] usbid: %d, get pid: 0x%x, vid: 0x%x [%s]\n", usbids[0], usb0pvid[0], usb0pvid[1], ptdevpath);

        pmeta = &meta;
        ptm = (char *)pmeta;
            
        printf("[DVF] get meta magic number: 0x%.2x, 0x%.2x !!!\n", meta.ASP_MAGIC[0], meta.ASP_MAGIC[1]);

        insert_cbw(CBW, CBW_CMD_SEND_OPCODE, OP_META, OP_META_Sub1);
        usb_send(CBW, usbids[0], 31);
    
        usb_send(ptm, usbids[0], 512);
        shmem_dump(ptm, 512);
                
        usb_read(ptrecv, usbids[0], 13);

        printf("[DVF] dump 13 bytes");
        shmem_dump(ptrecv, 13);

        opc = OP_Multi_DUPLEX;
        dat = OPSUB_USB_Scan;
            
        insert_cbw(CBW, CBW_CMD_SEND_OPCODE, opc, dat);
        memcpy(&pkcbw[0], CBW, 32);

        insert_cbw(CBW, CBW_CMD_START_SCAN, opc, dat);
        memcpy(&pkcbw[32], CBW, 32);            

        insert_cbw(CBW, 0x13, opc, dat);
        memcpy(&pkcbw[64], CBW, 32);            

#define USB_HS_SAVE_RESULT 0
#define DBG_USB_HS 1

        /* start loop */
        ptret = USB_IOCT_LOOP_RESET(usbids[0], &bitset);
        printf("[DVF] conti read reset ret: %d \n", ptret);

        ptret = USB_IOCT_LOOP_START(usbids[0], pkcbw);
        printf("[DVF] conti read start ret: %d \n", ptret);

        acucnt = 0;
            
        pid = fork();

        if (!pid) {

            while(1) {
                ret = read(pipeInfo[0], &ch, 1);
                if (ret > 0) {
                    printf("[SAVE] get ch: %c \n", ch);

                    if (ch == 'b') {
                        fsave = find_save(ptfilepath, ptfileSave);
                        if (!fsave) {
                            printf("[SAVE] find save failed!!! \n");
                        } else {
                            printf("[SAVE] find save [%s] !!! \n", ptfilepath);
                        }
                    }
                    if (ch == 'e') {
                        break;
                    }
                } else {
                    printf("[SAVE] get ch failed ret: %d \n", ret);
                }
            
                while (1) {
                    ret = read(pipeInfo[0], &ch, 1);
                    if (ret > 0) {
                        //printf("[SAVE] get ch=%c \n", ch);
                        ix = acucnt % RING_BUFF_NUM;
                        chvir = (char *) viraddr0[ix];

                        //shmem_dump(chvir, 32);

                        acucnt++;

                        fwrite(chvir, 1, PT_BUF_SIZE, fsave);

                        if (ch == 'd') {
                            printf("[SAVE] get ch: %c break\n", ch);
                            break;
                        }
                    } else {
                        printf("[SAVE] get ch failed ret: %d \n", ret);
                    }
                }

                fclose(fsave);
            
            }

            write(pipeBack[1], "E", 1);
            
            exit(0);  
        }

        while(1) {
            usbrun = -1;
            recvsz = 0;
            acusz = 0;
            tcnt = 0;
            usbfolw = 0;
            len = PT_BUF_SIZE;

            write(pipeInfo[1], "b", 1);
                
            while(1) {
                #if 0 /* test drop line */
                usleep(10000);
                #endif

                recvsz = USB_IOCT_LOOP_CONTI_READ(usbids[0], &usbfolw);

                usbfolw += 1;

                #if 0                
                if (tcnt) {
                    clock_gettime(CLOCK_REALTIME, &utend);
                    //usCost = test_time_diff(&utstart, &utend, 1000);
                    //printf("[%s] read %d (%d ms)\n", strpath, recvsz, usCost/1000);
                }
                #endif 
                
                if (recvsz & 0x10000000) {
                    if (recvsz > 0) {
                        usbrun = recvsz  & 0xfffffff;
                        recvsz = len;

                        #if DBG_USB_HS
                        printf("[DVF] recvsz: %d, usbrun: %d - 0\n", recvsz, usbrun);
                        #endif
                    }
                } else if (recvsz > len) {
                    cswst = 0;
                    if (recvsz > 0xfffff) {
                        cswst = (recvsz >> 20) & 0xff;
                        
                        /*should not be here*/
                        printf("[DVF] Error!!! get status: 0x%.2x recv:%d\n", cswst, recvsz);

                        //chr = 'I';
                    }

                    if (recvsz & 0x20000) {
                        recvsz = recvsz  & 0x1ffff;
                        usbrun = -1;
                        //printf("[%s] Error!!! data get the end signal 0x20000 recvsz: %d\n", strpath, recvsz);
                    }
                    else if (recvsz & 0x40000) {
                        recvsz = recvsz  & 0x1ffff;
                        usbrun = 0;
                    }
                    else if (recvsz & 0x80000) {
                        recvsz = recvsz  & 0x1ffff;
                        usbrun = -1;
                    }
                    else {
                        usbrun = recvsz  & 0xfff;
                        recvsz = len;

                        #if DBG_USB_HS
                        printf("[DVF] recvsz: %d, usbrun: %d - 1\n", recvsz, usbrun);
                        #endif
                    }
                    //sprintf_f(rs->logs, "last trunk size: %d \n", recvsz);
                }
                else {
                    if (recvsz > 0) {
                        usbrun = recvsz  & 0xfff;
                        recvsz = len;

                        #if 1//DBG_USB_HS
                        printf("[DVF] recvsz: %d, usbrun: %d - 2\n", recvsz, usbrun);
                        #endif
                    }
                }
                
                if (recvsz < 0) {
                    printf("[DVF] usb read ret: %d\n", recvsz);
                    continue;
                    //break;
                }
                else if (recvsz == 0) {
                    //printf("[%s] usb read ret: %d \n", strpath, recvsz);
                    continue;
                }
                else {
                    /*do nothing*/
                }

                write(pipeInfo[1], "c", 1);

                #if DBG_USB_HS
                printf("[DVF] usb read %d / %d!!\n", recvsz, len);
                //printf(".");
                #endif
                
                //printf("[HS] dump 32 - 0 \n");
                //msync(addr, recvsz, MS_SYNC);
                //shmem_dump(addr, 32);

                tcnt ++;

                if (tcnt == 1) {
                    clock_gettime(CLOCK_REALTIME, &utstart);
                    printf("[DVF] start ... \n");
                }
                
                acusz += recvsz;

                if (((recvsz > 0) && (recvsz < len)) && (usbrun < 0)) {
                    clock_gettime(CLOCK_REALTIME, &utend);
                    //ring_buf_set_last(pTx, recvsz);
                    printf("[DVF] loop last ret: %d, the last size: %d \n", ptret, recvsz);
                    break;
                }

            }

            usCost = test_time_diff(&utstart, &utend, 1000);
            throughput = acusz*8.0 / usCost*1.0;

            printf("[DVF] total read size: %d, write file size: %d throughput: %lf Mbits \n", acusz, wrtsz, throughput);

            usbrun = -1;
            recvsz = 0;
            acusz = 0;
            tcnt = 0;
            cswst = 0;
            
            while(1) {
            
                recvsz = USB_IOCT_LOOP_CONTI_READ(usbids[0], &usbfolw);

                if (recvsz > len) {
                    //sprintf_f(rs->logs, "last trunk size: %d 0x%x\n", recvsz, recvsz);
                    //print_f(rs->plogs, sp, rs->logs);

                    cswst = 0;
                    if (recvsz > 0xfffff) {

                        cswst = (recvsz >> 20) & 0xff;
                        
                        if (cswst == 0x80) {
                            cswst = 0x7f;
                        }
                        
                        csworg = cswst;

                        #if 1 /* pause status */
                        if ((cswst & 0x7f) == 0x22) {
                            cswst = 0x7f;
                            //cswst = 0x21;                        
                            //puhs->pushcswerr = cswst;
                        }
                        else if ((cswst & 0x7f) == 0x23) {
                            cswst = 0x7f;
                            //cswst = 0x21;                        
                            //puhs->pushcswerr = cswst;
                        } else 
                        #endif
                        #if 1 /* stop scan if get error status */
                        if ((cswst & 0x7f) && (cswst != 0x7f)) {

                            chr = 'R';                        
                        }
                        #endif

                        printf("[DVF] get the error status: 0x%.2x (org: 0x%.2x)\n", cswst, csworg & 0x7f);

                        #if 0 /* stop scan if get error status */
                        if ((cswst & 0x7f) && (cswst != 0x7f)) {
                        
                            puhs->pushcswerr = cswst;
                            
                            chr = 'R';                        
                        }
                        #endif
                    } 

                    if (recvsz & 0x20000) {
                        printf("[DVF] get the end signal 0x20000 \n");


                        #if USB_CALLBACK_LOOP 
                        chr = 'R';
                        #else
                        chr = 0;
                        #endif

                        recvsz = recvsz  & 0x1ffff;
                        usbrun = -1;

                        #if 1 /* stop scan by default status */
                        if (cswst == 0x7f) {
                            cswst = 0x21;                        
                        }
                        #endif
                        
                        //sprintf_f(rs->logs, "use the error status: 0x%.2x recv: %d\n", cswst, recvsz);
                        //print_f(rs->plogs, sp, rs->logs);
                    }
                    else if (recvsz & 0x40000) {
                        recvsz = recvsz  & 0x1ffff;
                        usbrun = 0;
                    }
                    else if (recvsz & 0x80000) {
                        recvsz = recvsz  & 0x1ffff;
                        usbrun = -1;
                    }
                    else {
                        usbrun = recvsz  & 0xfff;
                        recvsz = len;

                        printf("[DVF] recvsz: %d, usbrun: %d - m1\n", recvsz, usbrun);
                    }
                }
                else {
                    if (recvsz > 0) {
                        usbrun = recvsz  & 0xffff;
                        recvsz = len;


                        #if DBG_USB_HS
                        printf("[DVF] recvsz: %d, usbrun: %d - m2\n", recvsz, usbrun);
                        #endif
                    }
                }
                                
                if (recvsz < 0) {
                    printf("[DVF] usb read ret: %d \n", recvsz);
                    continue;
                }
                else if (recvsz == 0) {
                    continue;
                }
                else {
                    /*do nothing*/
                }

                write(pipeInfo[1], "d", 1);
                
                printf("[DVF] usb read %d / %d!!\n", recvsz, len);
                //printf(".");
                
                //printf("[HS] dump 32 - 0 \n");
                //msync(addr, recvsz, MS_SYNC);
                //shmem_dump(addr, 32);

                tcnt ++;

                if (tcnt == 1) {
                    //clock_gettime(CLOCK_REALTIME, &utstart);
                    printf("[DVF] start ... \n");
                }
                
                acusz += recvsz;

                if (((recvsz > 0) && (recvsz < len)) && (usbrun < 0)) {
                    //clock_gettime(CLOCK_REALTIME, &utend);
                    //ring_buf_set_last(pTx, recvsz);
                    printf("[DVF] loop last ret: %d, the last size: %d \n", ptret, recvsz);
                    break;
                }
        
            }
            
            //usCost = test_time_diff(&utstart, &utend, 1000);
            //throughput = acusz*8.0 / usCost*1.0;

            printf("[DVF] total read size: %d, write file size: %d throughput: %lf Mbits \n", acusz, wrtsz, throughput);

            if (loopcnt == looptimes) {
                ptret = USB_IOCT_LOOP_STOP(usbids[0], &bitset);
                printf("[DVF] conti read stop ret: %d \n", ptret);
            }
            
            printf("[DVF] loopcnt: %d chr: 0x%.2x\n", loopcnt, chr);

            if (chr == 'R') {
                break;
            }
            
            loopcnt++;
        }

        write(pipeInfo[1], "e", 1);
            
        ret = read(pipeBack[0], &ch, 1);
        if (ret > 0) {
            ret = USB_IOCT_LOOP_BUFF_RELEASE(usbids[0], &ix);
            if (ret < 0) {
                printf("can't release buff failed, size: %d [%s]\n", RING_BUFF_NUM, ptdevpath); 
                close(usbids[0]);
                goto end;
            }

            printf("release buff succeed, size: %d ret: %d \n", RING_BUFF_NUM, ret);

            close(usbids[0]);
        }
        
        goto end;
    }
    
#define USB_SAVE_RESULT (0)
#define USB_RX_LOG 0
    if (sel == 35){ /* usb printer duplex scan */
        struct pollfd ptfd[1];
        static char ptdevpath[] = "/dev/usb/lp0";
        static char ptfileSave[] = "/mnt/mmc2/usb/image%.3d.jpg";
        static char ptfileMeta[] = "/mnt/mmc2/usb/meta.bin";
        char ptfilepath[128];
        char *ptrecv, *ptbuf=0, *pImage=0, *pImage2=0;;
        int ptret=0, recvsz=0, acusz=0, wrtsz=0;
        int cntRecv=0, usCost=0, bufsize=0;
        int bufmax=0, bufidx=0, printlog=0;
        double throughput=0.0;
        struct timespec tstart, tend;
        struct aspMetaData_s meta, *pmeta=0;
        char *ptm=0, *pcur=0;
        char CBW[32] = {0x55, 0x53, 0x42, 0x43, 0x11, 0x22, 0x33, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        int usbid=0, mfd=0;
        FILE *fsave=0, *fmeta=0;
        int looptimes=0;
        int usb0pvid[2];
        int ix=0;
        uint32_t *phyaddr0=0, ut32=0;
        uint32_t *viraddr0=0, vt32=0;
        char *chvir;
        
        mfd = open(MODULE_NAME , O_RDWR);
        if(mfd < 0) {
            printf("open() %s errorn" , MODULE_NAME);
        } else {
            printf("open [%s] succeed!!!! \n", MODULE_NAME);
        }

        //while(1);

        usbid = open(ptdevpath, O_RDWR);
        if (usbid < 0) {
            printf("can't open device[%s] ret: %d \n", ptdevpath, usbid); 
            
            perror("open usb");
            printf("usb create failed, errno: %d\n", errno);
            exit(EXIT_FAILURE);
            close(usbid);
            goto end;
        }
        else {
            printf("open device[%s]\n", ptdevpath); 
        }

#if 1
        
        ret = USB_IOCT_GET_VID_PID(usbid, usb0pvid);
        if (ret < 0) {
            printf("can't get vid pid for [%s]\n", ptdevpath); 
            close(usbid);
            goto end;
        }
        
        ix = RING_BUFF_NUM;
        ret = USB_IOCT_LOOP_BUFF_CREATE(usbid, &ix);
        if (ret < 0) {
            printf("can't create buff failed, size: %d [%s]\n", RING_BUFF_NUM, ptdevpath); 
            close(usbid);
            goto end;
        }
        
        phyaddr0 = malloc(RING_BUFF_NUM*4);
        viraddr0 = malloc(RING_BUFF_NUM*4);
        if ((!phyaddr0) || (!viraddr0)) {
            printf("allocate memory failed, size: %d [%s]\n", RING_BUFF_NUM*4, ptdevpath); 
            close(usbid);
            goto end;
        }
        
        ret = USB_IOCT_LOOP_BUFF_PROBE(usbid, phyaddr0);
        if (ret < 0) {
            printf("can't probe phy addr, size: %d [%s]\n", RING_BUFF_NUM, ptdevpath); 
            close(usbid);
            goto end;
        }
        
        ix = 0;
        printf("addr0: \n%d: ", ix);
        for (ix=0; ix < RING_BUFF_NUM; ix++) {
            ut32 = phyaddr0[ix];
            printf("p:0x%.8x ", ut32);
        
            ret = phy2vir(&vt32, ut32, PT_BUF_SIZE, mfd);
            if (ret < 0) {
                printf("addr0 phy 2 vir error!!! ret: %d \n", ret);
                break;
            }
            
            viraddr0[ix] = vt32;
            printf("v:0x%.8x ", vt32);
            if ((ix+1) % 4 == 0) {
                printf("\n%d: ", ix);
            }
        }

        printf("vaddr0 val check:\n");        
        for (ix=0; ix < RING_BUFF_NUM; ix++) {
            chvir = (char *) viraddr0[ix];
        
            //printf("0x%.2x ", chvir[0]);
            if (chvir[0] != (ix & 0xff)) {
                printf("0e: %d-0x%.2x ", ix, chvir[0]);            
            }
            
            /*
            for (ind=1; ind < PT_BUF_SIZE; ind++) {
                chvir[ind] = (ix + ind) & 0xff;
            }
            
            msync(chvir, PT_BUF_SIZE, MS_SYNC);
            */
            
            //shmem_dump(chvir, 32);

            if ((ix+1) % 16 == 0) {
                //printf("\n");
            }
        }

        printf("usbid: %d, get pid: 0x%x, vid: 0x%x [%s]\n", usbid, usb0pvid[0], usb0pvid[1], ptdevpath);

#endif

        if (arg0 > 0) {
            looptimes = arg0;
        } else {
            looptimes = 1;
        }

        //usb_nonblock_set(usbid);

        pmeta = &meta;
        ptm = (char *)pmeta;

        fmeta = fopen(ptfileMeta, "r");
        if (fmeta) {
            printf("open meta sample file [%s] succeed \n", ptfileMeta);
            ptret = fread(ptm, 1, 512, fmeta);
            printf("read meta size: %d /%d \n", ptret, 512);

            shmem_dump(ptm, 512);
            dbgMeta(msb2lsb(&pmeta->FUNC_BITS), pmeta);
        } else {
            printf("open meta sample file [%s] failed \n", ptfileMeta);
            memset(ptm, 0, sizeof(meta));
            meta.ASP_MAGIC[0] = 0x20;
            meta.ASP_MAGIC[1] = 0x14;
        }

        bufsize = PT_BUF_SIZE;


        printf("usb write size[%d]\n", bufsize); 
        
        ptrecv = malloc(PT_BUF_SIZE);
        memset(ptrecv, 0, PT_BUF_SIZE);
        ptbuf = malloc(PT_BUF_SIZE);
        memset(ptbuf, 0, PT_BUF_SIZE);
        
        //insert_cbw(CBW, CBW_CMD_READY, OP_Hand_Scan, OPSUB_USB_Scan);
        //usb_send(CBW, usbid, 31);
#if 1//USB_SAVE_RESULT
        bufmax = 100*1024*1024;
        pImage = malloc(bufmax);
        pImage2 = malloc(bufmax);
#endif
//while (looptimes) {

        //printf("usb loop times: %d buff size: %d - 1\n", looptimes, PT_BUF_SIZE);

        insert_cbw(CBW, CBW_CMD_SEND_OPCODE, OP_META, OP_META_Sub1);
        usb_send(CBW, usbid, 31);

        usb_send(ptm, usbid, sizeof(meta));

        usb_read(ptrecv, usbid, 13);
        shmem_dump(ptrecv, 13);
        
        insert_cbw(CBW, CBW_CMD_SEND_OPCODE, OP_Multi_DUPLEX, OPSUB_USB_Scan);
        usb_send(CBW, usbid, 31);
        
        usb_read(ptrecv, usbid, 13);
        shmem_dump(ptrecv, 13);
        
while (looptimes) {

        //printf("usb loop times: %d buff size: %d - 2\n", looptimes, PT_BUF_SIZE);
        USB_IOCT_LOOP_RESET(usbid, &ix);
        
        insert_cbw(CBW, CBW_CMD_START_SCAN, OP_Multi_DUPLEX, OPSUB_USB_Scan);
        usb_send(CBW, usbid, 31);     
        
#if USB_SAVE_RESULT
        fsave = find_save(ptfilepath, ptfileSave);
        if (!fsave) {
            goto end;    
        }
#endif

#if 0
        ptret = USB_IOCT_LOOP_RESET(usbid, &bitset);
        printf("\n\n[HS] conti read reset ret: %d \n\n", ptret);
#endif
        pcur = pImage;

        recvsz = 0;
        acusz = 0;
        cntRecv = 0;
        while(1) {
        
            recvsz = usb_read(pcur, usbid, PT_BUF_SIZE);
            
#if USB_RX_LOG
            printf("[HS] read size: %d \n", recvsz);
#endif  

            if (recvsz <= 0) {
                cntRecv++;
                if (cntRecv > 2) {
                    //break;
                }
                continue;
            } else {
                cntRecv = 0;
            }

            //printf("[HS] conti read size: %d \n", recvsz);
            
            pcur += recvsz;
            acusz += recvsz;
            
            if (recvsz < PT_BUF_SIZE) {
                ptret = usb_read(ptrecv, usbid, PT_BUF_SIZE-1);
                printf("reach the last trunk size: %d, redundent read ret: %d, total: %d (%d) - 1\n", recvsz, ptret, acusz, PT_BUF_SIZE-1);        
#if 0//USB_CALLBACK_SUBMIT 
                ptret = USB_IOCT_LOOP_STOP(usbid, &bitset);
                printf("\n\n[HS] conti read stop ret: %d \n\n", ptret);
#endif
                break;
            }
            
            //usleep(5000);

#if USB_SAVE_RESULT
            if (acusz > bufmax) {
                break;
            }
#endif

        }

        usb_read(ptrecv, usbid, 13);
        //shmem_dump(ptrecv, 13);
        
#if USB_SAVE_RESULT
        wrtsz = fwrite(pImage, 1, acusz, fsave);
#endif

        printf("total read size: %d, write file size: %d last szie: %d\n", acusz, wrtsz, recvsz);

#if USB_SAVE_RESULT
        sync();
        fclose(fsave);
#endif

        //while(1);

        fsave = 0;
        
        USB_IOCT_LOOP_RESET(usbid, &ix);
        
        insert_cbw(CBW, CBW_CMD_START_SCAN, OP_Multi_DUPLEX, OPSUB_USB_Scan);
        usb_send(CBW, usbid, 31);     
        
#if USB_SAVE_RESULT
        fsave = find_save(ptfilepath, ptfileSave);
        if (!fsave) {
            goto end;    
        }
#endif

#if 0//USB_CALLBACK_SUBMIT
            /* start loop */
        ptret = USB_IOCT_LOOP_START(usbid, &bitset);
        printf("\n\n[HS] conti read start ret: %d \n\n", ptret);
#endif

        pcur = pImage2;
        recvsz = 0;
        acusz = 0;
        cntRecv = 0;
        while(1) {

            recvsz = usb_read(pcur, usbid, PT_BUF_SIZE);
            
#if USB_RX_LOG
            printf("[HS] read size: %d \n", recvsz);
#endif
            
            if (recvsz <= 0) {
                cntRecv++;
                if (cntRecv > 2) {
                    //break;
                }
                continue;
            } else {
                cntRecv = 0;
            }

            //printf("[HS] conti read size: %d \n", recvsz);
            
            pcur += recvsz;
            acusz += recvsz;

            if (recvsz < PT_BUF_SIZE) {
                ptret = usb_read(ptrecv, usbid, PT_BUF_SIZE-1);
                printf("reach the last trunk size: %d, redundent read ret: %d, total: %d (%d) - 2\n", recvsz, ptret, acusz, PT_BUF_SIZE-1);        
#if 0//USB_CALLBACK_SUBMIT 
                ptret = USB_IOCT_LOOP_STOP(usbid, &bitset);
                printf("\n\n[HS] conti read stop ret: %d \n\n", ptret);
#endif
                break;
            }
            
            //usleep(5000);
#if USB_CALLBACK_SUBMIT 
            if (acusz > bufmax) {
                break;
            }
#endif
        }

        usb_read(ptrecv, usbid, 13);
        //shmem_dump(ptrecv, 13);
        
#if USB_SAVE_RESULT
        wrtsz = fwrite(pImage2, 1, acusz, fsave);
#endif
        printf("total read size: %d, write file size: %d last szie: %d\n", acusz, wrtsz, recvsz);
        
#if USB_SAVE_RESULT 
        sync();
        fclose(fsave);
#endif
        fsave = 0;

        looptimes --;
}


        printf("usb loop times: %d END !!\n", looptimes);

        while (1) {
            insert_cbw(CBW, 0x13, OP_Multi_DUPLEX, OPSUB_USB_Scan);
            usb_send(CBW, usbid, 31);     

            usb_read(ptrecv, usbid, 13);
            //shmem_dump(ptrecv, 13);

            looptimes = ptrecv[11];

            if (!looptimes) {
                break;
            } else {
                while (looptimes) {

                    USB_IOCT_LOOP_RESET(usbid, &ix);
                    
                    insert_cbw(CBW, CBW_CMD_START_SCAN, OP_Multi_DUPLEX, OPSUB_USB_Scan);
                    usb_send(CBW, usbid, 31);     
        
#if USB_SAVE_RESULT
                    fsave = find_save(ptfilepath, ptfileSave);
                    if (!fsave) {
                        goto end;    
                    }
#endif

#if 0//USB_CALLBACK_SUBMIT
                    /* start loop */
                    ptret = USB_IOCT_LOOP_START(usbid, &bitset);
                    printf("\n\n[HS] conti read start ret: %d \n\n", ptret);
#endif

                    pcur = pImage;
                    recvsz = 0;
                    acusz = 0;
                    cntRecv = 0;
                    while(1) {
        
                        recvsz = usb_read(pcur, usbid, PT_BUF_SIZE);
                        
#if USB_RX_LOG
                        printf("[HS] sread size: %d \n", recvsz);
#endif

                        if (recvsz <= 0) {
                            cntRecv++;
                            if (cntRecv > 2) {
                                //break;
                            }
                            continue;
                        } else {
                            cntRecv = 0;
                        }

                        //printf("[HS] conti read size: %d \n", recvsz);

                        pcur += recvsz;
                        acusz += recvsz;

                        if (recvsz < PT_BUF_SIZE) {
                            ptret = usb_read(ptrecv, usbid, PT_BUF_SIZE-1);
                            printf("reach the last trunk size: %d, redundent read ret: %d total: %d (%d) - s\n", recvsz, ptret, acusz, PT_BUF_SIZE-1);        
#if 0//USB_CALLBACK_SUBMIT 
                            ptret = USB_IOCT_LOOP_STOP(usbid, &bitset);
                            printf("\n\n[HS] conti read stop ret: %d \n\n", ptret);
#endif
                            break;
                        }

                        //usleep(5000);

                        if (acusz > bufmax) {
                            break;
                        }
                    }

                    usb_read(ptrecv, usbid, 13);
                    //shmem_dump(ptrecv, 13);
        
#if USB_SAVE_RESULT
                    wrtsz = fwrite(pImage, 1, acusz, fsave);
#endif
                    printf("total read size: %d, write file size: %d last szie: %d\n", acusz, wrtsz, recvsz);

#if USB_SAVE_RESULT
                    sync();
                    fclose(fsave);
#endif            
                    looptimes--;
                }
            }
        }

        ret = USB_IOCT_LOOP_BUFF_RELEASE(usbid, &ix);
        if (ret < 0) {
            printf("can't release buff failed, size: %d [%s]\n", RING_BUFF_NUM, ptdevpath); 
            close(usbid);
            goto end;
        }

        printf("release buff succeed, size: %d ret: %d \n", RING_BUFF_NUM, ret);
        
        free(pImage);
        free(pImage2);

        close(usbid);
        free(ptbuf);
        free(ptrecv);

        sync();

        goto end;
    }
    
    if (sel == 34){ /* usb printer simplex scan */
        struct pollfd ptfd[1];
        static char ptdevpath[] = "/dev/usb/lp0";
        static char ptfileSave[] = "/mnt/mmc2/usb/image%.3d.jpg";
        char ptfilepath[128];
        char *ptrecv, *ptbuf=0, *pImage=0;
        int ptret=0, recvsz=0, acusz=0, wrtsz=0;
        int cntRecv=0, usCost=0, bufsize=0;
        int bufmax=0, bufidx=0, printlog=0;
        double throughput=0.0;
        struct timespec tstart, tend;
        struct aspMetaData_s meta, *pmeta=0;
        char *ptm=0, *pcur=0;
        char CBW[32] = {0x55, 0x53, 0x42, 0x43, 0x11, 0x22, 0x33, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        int usbid=0;
        FILE *fsave=0;
        
        usbid = open(ptdevpath, O_RDWR);
        if (usbid < 0) {
            printf("can't open device[%s]\n", ptdevpath); 
            close(usbid);
            goto end;
        }
        else {
            printf("open device[%s]\n", ptdevpath); 
        }

        //usb_nonblock_set(usbid);

        pmeta = &meta;
        ptm = (char *)pmeta;
        memset(ptm, 0, sizeof(meta));
        meta.ASP_MAGIC[0] = 0x20;
        meta.ASP_MAGIC[1] = 0x14;

        if (arg0 > 0) {
            bufsize = arg0;
        } else {
            bufsize = PT_BUF_SIZE;
        }

        if (bufsize > PT_BUF_SIZE) {
            bufsize = PT_BUF_SIZE;
        }

        printf("usb write size[%d]\n", bufsize); 
        
        ptrecv = malloc(PT_BUF_SIZE);
        memset(ptrecv, 0, PT_BUF_SIZE);
        ptbuf = malloc(PT_BUF_SIZE);
        memset(ptbuf, 0, PT_BUF_SIZE);
        
        //insert_cbw(CBW, CBW_CMD_READY, OP_Hand_Scan, OPSUB_USB_Scan);
        //usb_send(CBW, usbid, 31);

        insert_cbw(CBW, CBW_CMD_SEND_OPCODE, OP_META, OP_META_Sub1);
        usb_send(CBW, usbid, 31);

        usb_send(ptm, usbid, sizeof(meta));

        usb_read(ptrecv, usbid, 13);
        shmem_dump(ptrecv, 13);

        //insert_cbw(CBW, CBW_CMD_READY, 0, 0);
        //usb_send(CBW, usbid, 31);
        
        insert_cbw(CBW, CBW_CMD_SEND_OPCODE, OP_SINGLE, OPSUB_USB_Scan);
        usb_send(CBW, usbid, 31);
        
        usb_read(ptrecv, usbid, 13);
        shmem_dump(ptrecv, 13);
        
        insert_cbw(CBW, CBW_CMD_START_SCAN, OP_SINGLE, OPSUB_USB_Scan);
        usb_send(CBW, usbid, 31);     
        
#if USB_SAVE_RESULT
        fsave = find_save(ptfilepath, ptfileSave);
        if (!fsave) {
            goto end;    
        }

        bufmax = 48*1024*1024;

        pImage = malloc(bufmax);
        pcur = pImage;
        recvsz = 0;
        acusz = 0;

        cntRecv = 0;
        while(1) {
            recvsz = usb_read(pcur, usbid, PT_BUF_SIZE);

            if (recvsz == 0) {
                continue;
            } else if (recvsz < 0) {
                cntRecv++;
                if (cntRecv > 0xffff) {
                    break;
                }
                continue;
            } else {
                cntRecv = 0;
            }
            
            pcur += recvsz;
            acusz += recvsz;

            if (recvsz < PT_BUF_SIZE) {
                ptret = usb_read(ptrecv, usbid, PT_BUF_SIZE+1);
                printf("reach the last trunk size: %d, redundent read ret: %d \n", recvsz, ptret);        
                break;
            }
            
            //usleep(5000);

            if (acusz > bufmax) {
                break;
            }
        }

        wrtsz = fwrite(pImage, 1, acusz, fsave);
        printf("total read size: %d, write file size: %d \n", acusz, wrtsz);
            
        sync();
        fclose(fsave);
        free(pImage);
#endif

        close(usbid);
        free(ptbuf);
        free(ptrecv);

        goto end;
    }
    
    if (sel == 33){ /* usb printer read access (jpg) */
        struct pollfd ptfd[1];
        static char ptdevpath[] = "/dev/usb/lp0";
        static char ptfileSave[] = "/mnt/mmc2/usb/image%.3d.jpg";
        char ptfilepath[128];
        char *ptrecv, *ptbuf=0;
        int ptret=0, recvsz=0, acusz=0, wrtsz=0;
        int cntRecv=0, usCost=0, bufsize=0, cntTx=0;
        int bufmax=0, bufidx=0, printlog=0, fileformat=0;
        double throughput=0.0;
        struct timespec tstart, tend;
        FILE *fsave=0;
        char lastTrunk[64];
        char fileStr[64];
        uint32_t tlen=0, filelen=0;
        struct slvbitMapHeader_s bmpheader, *pbh;

        pbh = &bmpheader;
        
        memset(pbh, 0, sizeof(struct slvbitMapHeader_s));
        memset(lastTrunk, 0, 64);
        
        ptfd[0].fd = open(ptdevpath, O_RDWR);
        if (ptfd[0].fd < 0) {
            printf("can't open device[%s]\n", ptdevpath); 
            close(ptfd[0].fd);
            goto end;
        }
        else {
            printf("open device[%s]\n", ptdevpath); 
        }

        if (arg0 > 0) {
            bufsize = arg0;
        } else {
            bufsize = PT_BUF_SIZE;
        }

        if (arg1 > 0) {
            printlog = arg1;
        }
        
        switch (arg2) {
            case 0:
            sprintf(fileStr, "JPG");
            fileformat = 0;
            break;
            case 1:
            sprintf(fileStr, "BMP");
            fileformat = 1;
            break;
            default:
            sprintf(fileStr, "JPG");
            fileformat = 0;
            break;
        }

        if (arg3 > 0) {
            bufmax = arg3*1024*1024;
        } else {
            bufmax = 256*1024*1024;
        }
        
        printf("usb recv buff size:[%d] format:[%s] \n", bufsize, fileStr); 
        
        ptfd[0].events = POLLIN;
        fsave = find_save(ptfilepath, ptfileSave);
        if (!fsave) {
            goto end;    
        }

        printf("usb find save [%s] \n", ptfilepath);

        //bufmax = 512*1024*1024;
        ptbuf = malloc(bufmax);  //1024*32768
        if (ptbuf) {
            printf("usb allocate memory size:[%d] \n", bufmax);
        } else {
            printf("usb allocate memory size:[%d] failed!!!! \n", bufmax);
        }

        ptrecv = ptbuf;
        while(1) {
            //ptret = read();
            ptret = poll(ptfd, 1, -1);
            //printf("usb poll ret: %d \n", ptret);
            //if (ptret < 0) {
                //printf("usb poll failed ret: %d\n", ptret);
                //break;
            //}

            if (ptret && (ptfd[0].revents & POLLIN)) {
                recvsz = read(ptfd[0].fd, ptrecv, bufsize);

                if (cntTx == 0) {
                    clock_gettime(CLOCK_REALTIME, &tstart);
                    cntTx ++;
                }
                
                //printf("usb recv ret: %d \n", recvsz);
                if (recvsz <= 1) {
                    switch (recvsz) {
                        case 1:
                            printf("usb recv %d byte wait last trunk 64 bytes \n", recvsz);

                            clock_gettime(CLOCK_REALTIME, &tend);
                            
                            bufsize = 64;
                            ptret = read(ptfd[0].fd, lastTrunk, bufsize);
                            printf("usb recv last trunk size: %d, break!!!\n", ptret);        

                            shmem_dump(lastTrunk, 64);

                            switch (fileformat) {
                                case 0:
                                    tlen = lastTrunk[4] + (lastTrunk[5] << 8);
                                    printf("%s scan length: %d \n",fileStr, tlen);
                                    changeJpgLen(ptbuf, tlen, acusz);
                                    break;
                                case 1:
                                    tlen = lastTrunk[4] + (lastTrunk[5] << 8);
                                    filelen = lastTrunk[7] + (lastTrunk[8] << 8) + (lastTrunk[9] << 16) + (lastTrunk[10] << 24);
                                    
                                    printf("%s scan length: %d file length: %d \n",fileStr, tlen, filelen);
                                    
                                    memcpy(&pbh->aspbmpMagic[2], ptbuf, sizeof(struct slvbitMapHeader_s) - 2);

                                    mspbitmapHeaderSetup(pbh, 24, pbh->aspbiWidth, tlen, -1, acusz+recvsz);

                                    memcpy(ptbuf, &pbh->aspbmpMagic[2], sizeof(struct slvbitMapHeader_s) - 2);
                                    
                                    printf("%s scan length: %d filelen: %d\n", fileStr, tlen, filelen);
                                    break;
                                default:
                                    printf("unknown file format: %d \n", fileformat);
                                    tlen = 0;
                                    break;
                            }
                            
                            break;
                        case 0:
                            continue;
                            break;
                        default:
                            printf("usb recv ret: %d, error!!!", recvsz);
                            break;
                    }
                    break;
                }
                
                /*
                if (!recvsz) {
                    continue;
                }
                */
                
                /*
                if (cntRecv == 0) {
                    clock_gettime(CLOCK_REALTIME, &tstart);
                }
                */

                //wrtsz = fwrite(ptrecv, 1, recvsz, fsave);
                wrtsz = recvsz;

                //sync();
                
                acusz += wrtsz;
#if (USB_SAVE_RESULT == 1)
                ptrecv += wrtsz;
#endif
/*
                if (printlog) {
                    //cntRecv ++;
                    printf("usb r %d w %d tot %d \n", recvsz, wrtsz, acusz);
                }
*/
                
/*
                if (recvsz < bufsize) {
                    printf("usb recv end last recv = %d \n", recvsz);
                    clock_gettime(CLOCK_REALTIME, &tend);
                    break;
                }
*/
            }

            
        }

        usCost = test_time_diff(&tstart, &tend, 1000);

        throughput = acusz*8.0 / usCost*1.0;
        
        printf("usb throughput: %d bytes / %d us = %lf MBits\n", acusz, usCost, throughput);

        printf("write file size [%d] \n", acusz);

#if (USB_SAVE_RESULT == 1)
        ptrecv = ptbuf;
        while (acusz) {
            if (acusz > 32768) {
                wrtsz = 32768;
            } else {
                wrtsz = acusz;
            }
            
            recvsz = fwrite(ptrecv, 1, wrtsz, fsave);
            if (recvsz <= 0) {
                break;
            }
            
            acusz -= recvsz;
            ptrecv += recvsz;

            //printf("write file [%d] \n", acusz);
        }
#endif        
        sync();

        close(ptfd[0].fd);
        fclose(fsave);
        free(ptbuf);
        goto end;
    }
    

    if (sel == 32){ /* usb printer test usb scam */
        //#define PT_BUF_SIZE (512)
        struct pollfd ptfd[1];
        static char ptdevpath[] = "/dev/g_printer";
        static char ptfileSend[] = "/mnt/mmc2/usb/greenhill_01.jpg";
        char ptfilepath[128];
        char *ptrecv, *ptsend, *palloc=0;
        int ptret=0, recvsz=0, acusz=0, wrtsz=0, maxsz=0, sendsz=0, lastsz=0;
        int cntTx=0, usCost=0, bufsize=0;
        double throughput=0.0;
        FILE *fsave=0, *fsend=0;
        struct timespec tstart, tend;
        char csw[13] = {0x55, 0x53, 0x42, 0x43, 0x11, 0x22, 0x33, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00};
        uint8_t cmd=0, opc=0, dat=0;
        
        struct epoll_event eventRx, eventTx, getevents[MAX_EVENTS];
        int usbfd=0, epollfd=0, uret=0, ifx=0, rxfd=0, txfd=0;

        usbfd =  open(ptdevpath, O_RDWR);
#if 0
        epollfd = epoll_create1(O_CLOEXEC);
        if (epollfd < 0) {
            perror("epoll_create1");
            //exit(EXIT_FAILURE);
            printf("epoll create failed, errno: %d\n", errno);
        } else {
            printf("epoll create succeed, epollfd: %d, errno: %d\n", epollfd, errno);
        }
        
        //usb_nonblock_set(usbfd);

        eventRx.data.fd = usbfd;
        eventRx.events = EPOLLIN | EPOLLLT;
        ret = epoll_ctl (epollfd, EPOLL_CTL_ADD, usbfd, &eventRx);
        if (ret == -1)
        {
            perror ("epoll_ctl");
            printf("spoll set ctl failed errno: %ds\n", errno);
        }
        

        eventTx.data.fd = usbfd;
        eventTx.events = EPOLLOUT | EPOLLET;
        ret = epoll_ctl (epollfd, EPOLL_CTL_ADD, usbfd, &eventTx);
        if (ret == -1)
        {
            perror ("epoll_ctl");
            printf("spoll set ctl failed errno: %ds\n", errno);
        }
#endif

        ptrecv = malloc(PT_BUF_SIZE);
        if (ptrecv) {
            printf(" recv buff alloc succeed! size: %d \n", maxsz);
        } else {
            printf(" recv buff alloc failed! size: %d \n", maxsz);
            goto end;
        }
        
        printf(" open file [%s] \n", ptfileSend);
        fsend = fopen(ptfileSend, "r");
       
        if (!fsend) {
            printf(" [%s] file open failed \n", ptfileSend);
            goto end;
        }   
        printf(" [%s] file open succeed \n", ptfileSend);

        if (arg0 > 0) {
            bufsize = arg0;        
        } else {
            bufsize = PT_BUF_SIZE;
        }
        printf(" recv buff size:[%d] \n", bufsize);

        ptret = fseek(fsend, 0, SEEK_END);
        if (ptret) {
            printf(" file seek failed!! ret:%d \n", ptret);
            goto end;
        }

        maxsz = ftell(fsend);
        printf(" file [%s] size: %d \n", ptfileSend, maxsz);
        
        ptsend = malloc(maxsz);
        if (ptsend) {
            printf(" send buff alloc succeed! size: %d \n", maxsz);
        } else {
            printf(" send buff alloc failed! size: %d \n", maxsz);
            goto end;
        }

        ptret = fseek(fsend, 0, SEEK_SET);
        if (ptret) {
            printf(" file seek failed!! ret:%d \n", ptret);
            goto end;
        }
        
        ptret = fread(ptsend, 1, maxsz, fsend);
        printf(" output file read size: %d/%d \n", ptret, maxsz);

        fclose(fsend);
        
#if 0
        while (1) {
            printf("epoll %d \n", cntTx);
            cntTx++;

            uret = epoll_wait(epollfd, getevents, MAX_EVENTS, 5000);
            if (uret < 0) {
                perror("epoll_wait");
                printf("nonblock failed errno: %d ret: %d\n", errno, uret);
            } else if (uret == 0) {
                printf("nonblock failed errno: %d ret: %d\n", errno, uret);
            } else {
                for (ifx = 0; ifx < MAX_EVENTS; ifx++) {
            
                if (getevents[ifx].events & EPOLLIN) {
                    rxfd = getevents[ifx].data.fd;
                    recvsz = read(rxfd, ptrecv, PT_BUF_SIZE);
                    printf("epoll nonblock read %d / %d, rxfd: %d, usbfd: %d, ret = %d (%d)\n", recvsz, PT_BUF_SIZE, rxfd, usbfd, uret, ifx);
                    shmem_dump(ptrecv, recvsz);
                    if ((ptrecv[0] == 0x55) && (ptrecv[1] == 0x53) && (ptrecv[2] == 0x42)) {
                        cmd = ptrecv[15];
                        opc = ptrecv[16];
                    }
                }
                
                if (getevents[ifx].events & EPOLLOUT) {
                    printf("cmd: 0x%.2x, opc: 0x%.2x \n", cmd, opc);
                    txfd = getevents[ifx].data.fd;
                    if ((cmd == 0x12) && (opc == 0x04)) {
                        printf("start to send image size: %d \n", maxsz);

                        lastsz = maxsz;        
                        palloc = ptsend;
                        while(1) {
                            if (lastsz > bufsize) {
                                wrtsz = bufsize;
                            } else {
                                wrtsz = lastsz;
                                if (wrtsz == 1) {
                                    wrtsz = 2;
                                }
                            }

                            printf("usb TX size: %d (%d)\n", wrtsz, ifx);
                            sendsz = write(txfd, ptsend, wrtsz);

                            if (sendsz < 0) {
                                printf("usb send ret: %d, error!!!", sendsz);
                                break;
                            }

                            if (cntTx == 0) {
                                clock_gettime(CLOCK_REALTIME, &tstart);
                            }

                            ptsend += sendsz;
                            acusz += sendsz;
                            lastsz = lastsz - sendsz;
                            cntTx ++;

                            //printf("usb send %d/%d to usb total %d last %d\n", sendsz, wrtsz, acusz, lastsz);

                            if (lastsz == 0) {
                                sendsz = write(txfd, ptsend, 1);
                                printf("usb send end last size = %d \n====================\n", sendsz);
                                clock_gettime(CLOCK_REALTIME, &tend);
                                break;
                            }
                        }


                        usCost = test_time_diff(&tstart, &tend, 1000);
                        throughput = maxsz*8.0 / usCost*1.0;
                        printf("usb throughput: %d bytes / %d us = %lf MBits\n", maxsz, usCost, throughput);
                    }
                    else {
                        wrtsz = write(txfd, csw, 13);
                        printf("usb TX size: %d (%d)\n====================\n", wrtsz, ifx); 
                        shmem_dump(csw, wrtsz);
                    }
                }
                }
                
            }
        }
#else   
        ptfd[0].fd = usbfd;
        if (ptfd[0].fd < 0) {
            printf("can't open device[%s]\n", ptdevpath); 
            close(ptfd[0].fd);
            goto end;
        }
        else {
            printf("open device[%s]\n", ptdevpath); 
        }
            
        ptfd[0].events = POLLOUT;
        cntTx = 1;
#define STAGE_DELAY  //printf("\nPass!!!\n"); //sleep(2)

        while (1) {
            /* send ready */
            //memset(ptrecv, 0, PT_BUF_SIZE);
            ptfd[0].events = POLLIN | POLLOUT;

            ptret = poll(ptfd, 1, -1);
            printf("\n====================\npoll return %d evt: 0x%.2x\n", ptret, ptfd[0].revents);
            
            if (ptret < 0) {
                 printf("poll return %d \n", ptret);
                 goto end;
            }

            if (ptfd[0].revents & POLLIN) {
                if ((opc == 0x4c) && (dat == 0x01)) {
                    recvsz = read(ptfd[0].fd, ptrecv, 513);
                    printf("usb RX size: %d \n====================\n", recvsz); 
                    shmem_dump(ptrecv, recvsz);
                    dat = 0;
                } else {
                    recvsz = read(ptfd[0].fd, ptrecv, 31);
                    printf("usb RX size: %d \n====================\n", recvsz); 
                    shmem_dump(ptrecv, recvsz);
/*
                    if ((ptrecv[0] == 0x55) && (ptrecv[1] == 0x53) && (ptrecv[2] == 0x42)) {
                        if ((ptrecv[15] == 0x08) && (ptrecv[16] == 0x0b) && (ptrecv[17] == 0x85)) {
                            //break;
                        }
                    }
*/
                    if ((ptrecv[0] == 0x55) && (ptrecv[1] == 0x53) && (ptrecv[2] == 0x42)) {
                         cmd = ptrecv[15];
                         opc = ptrecv[16];
                         dat = ptrecv[17];
                         printf("usb get opc: 0x%.2x, dat: 0x%.2x \n", opc, dat);                      
                     }            
                }
            }

            if (ptfd[0].revents & POLLOUT) {
                printf("cmd: 0x%.2x, opc: 0x%.2x \n", cmd, opc);
                
                if ((cmd == 0x12) && (opc == 0x04)) {
                    printf("start to send image size: %d \n", maxsz);

                    lastsz = maxsz;        
                    palloc = ptsend;
                    while(1) {
                        if (lastsz > bufsize) {
                            wrtsz = bufsize;
                        } else {
                            wrtsz = lastsz;
                            if (wrtsz == 1) {
                                wrtsz = 2;
                            }
                        }

                        sendsz = write(ptfd[0].fd, ptsend, wrtsz);
                        printf("usb TX size: %d, ret: %d \n", wrtsz, sendsz);
                        
                        if (sendsz < 0) {
                            printf("usb send ret: %d, error!!!", sendsz);
                            break;
                        }

                        if (cntTx == 0) {
                            clock_gettime(CLOCK_REALTIME, &tstart);
                        }

                        ptsend += sendsz;
                        acusz += sendsz;
                        lastsz = lastsz - sendsz;
                        cntTx ++;

                        //printf("usb send %d/%d to usb total %d last %d\n", sendsz, wrtsz, acusz, lastsz);

                        if (lastsz == 0) {
                            sendsz = write(ptfd[0].fd, ptsend, 1);
                            printf("usb send end last size = %d \n====================\n", sendsz);
                            clock_gettime(CLOCK_REALTIME, &tend);
                            break;
                        }
                    }


                    usCost = test_time_diff(&tstart, &tend, 1000);
                    throughput = maxsz*8.0 / usCost*1.0;
                    printf("usb throughput: %d bytes / %d us = %lf MBits\n", maxsz, usCost, throughput);
                
                }
                else {
                    cntTx++;
                    csw[12] = cntTx;
                    wrtsz = write(ptfd[0].fd, csw, 13);
                    printf("usb TX size: %d \n====================\n", wrtsz); 
                    shmem_dump(csw, wrtsz);
                }
            }
            
            //STAGE_DELAY;
        }
#endif
#if 0
        /* reply META CSW */
        //memset(ptrecv, 0, PT_BUF_SIZE);
        ptfd[0].events = POLLOUT;

        ptret = poll(ptfd, 1, -1);
        printf("poll return %d \n", ptret);
            
        if (ptret < 0) {
             printf("poll return %d \n", ptret);
             goto end;
        }

        csw[12] = 1;
        
        if (ptret && (ptfd[0].revents & POLLOUT)) {
            wrtsz = write(ptfd[0].fd, csw, 13);
            printf("usb write size: %d - 3\n", wrtsz); 
            shmem_dump(csw, wrtsz);
        }

        /* reply CSW */
        //memset(ptrecv, 0, PT_BUF_SIZE);
        ptfd[0].events = POLLOUT;

        ptret = poll(ptfd, 1, -1);
        printf("poll return %d \n", ptret);
            
        if (ptret < 0) {
             printf("poll return %d \n", ptret);
             goto end;
        }
        
        csw[12] = 1;

        if (ptret && (ptfd[0].revents & POLLOUT)) {
            wrtsz = write(ptfd[0].fd, csw, 13);
            printf("usb write size: %d - 3\n", wrtsz); 
            shmem_dump(csw, wrtsz);
        }
#endif

        close(ptfd[0].fd);
        free(palloc);
        goto end;
    }

    if (sel == 31){ /* usb printer write access */
        struct pollfd ptfd[1];
        static char ptdevpath[] = "/dev/usb/lp0";
        static char ptfileSave[] = "/mnt/mmc2/usb/image%.3d.jpg";
        char ptfilepath[128];
        char *ptrecv, *ptbuf=0;
        int ptret=0, recvsz=0, acusz=0, wrtsz=0;
        int cntRecv=0, usCost=0, bufsize=0;
        int bufmax=0, bufidx=0, printlog=0;
        double throughput=0.0;
        struct timespec tstart, tend;
        char firstTrunk[32] = {0x55, 0x53, 0x42, 0x43, 0x11, 0x22, 0x33, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        
        ptfd[0].fd = open(ptdevpath, O_RDWR);
        if (ptfd[0].fd < 0) {
            printf("can't open device[%s]\n", ptdevpath); 
            close(ptfd[0].fd);
            goto end;
        }
        else {
            printf("open device[%s]\n", ptdevpath); 
        }

        if (arg0 > 0) {
            bufsize = arg0;
        } else {
            bufsize = PT_BUF_SIZE;
        }

        if (bufsize > PT_BUF_SIZE) {
            bufsize = PT_BUF_SIZE;
        }

        printf("usb write size[%d]\n", bufsize); 
        
        ptbuf = malloc(PT_BUF_SIZE);
        ptrecv = ptbuf;
        
        wrtsz = 32;
        acusz = PT_BUF_SIZE;
        while (acusz > 0) {
            if (acusz > wrtsz) {
                recvsz = wrtsz; 
            } else {
                recvsz = acusz; 
            }
            memcpy(ptrecv, firstTrunk, recvsz);

            acusz -= recvsz;
            ptrecv += recvsz;
        }

        //shmem_dump(ptbuf, PT_BUF_SIZE);

        if (arg1 > 0) {
            printlog = arg1;
        }

        ptfd[0].events = POLLOUT;

        ptrecv = ptbuf;
        while(1) {
            ptret = poll(ptfd, 1, -1);
            printf("usb poll ret: %d \n", ptret);
            if (ptret < 0) {
                printf("usb poll failed ret: %d\n", ptret);
                break;
            }

            if (ptret && (ptfd[0].revents & POLLOUT)) {

                recvsz = write(ptfd[0].fd, ptrecv, bufsize);
                printf("usb write ret: %d \n", recvsz);

                break;
            }                
            
        }

        close(ptfd[0].fd);
        free(ptbuf);

        goto end;
    }
    
    if (sel == 30){ /* usb printer read access (jpg) */
        struct pollfd ptfd[1];
        static char ptdevpath[] = "/dev/usb/lp0";
        static char ptfileSave[] = "/mnt/mmc2/usb/image%.3d.jpg";
        char ptfilepath[128];
        char *ptrecv, *ptbuf=0;
        int ptret=0, recvsz=0, acusz=0, wrtsz=0;
        int cntRecv=0, usCost=0, bufsize=0, cntTx=0;
        int bufmax=0, bufidx=0, printlog=0, fileformat=0;
        double throughput=0.0;
        struct timespec tstart, tend;
        FILE *fsave=0;
        char lastTrunk[64];
        char fileStr[64];
        uint32_t tlen=0, filelen=0;
        struct slvbitMapHeader_s bmpheader, *pbh;

        pbh = &bmpheader;
        
        memset(pbh, 0, sizeof(struct slvbitMapHeader_s));
        memset(lastTrunk, 0, 64);
        
        ptfd[0].fd = open(ptdevpath, O_RDWR);
        if (ptfd[0].fd < 0) {
            printf("can't open device[%s]\n", ptdevpath); 
            close(ptfd[0].fd);
            goto end;
        }
        else {
            printf("open device[%s]\n", ptdevpath); 
        }

        if (arg0 > 0) {
            bufsize = arg0;
        } else {
            bufsize = PT_BUF_SIZE;
        }

        if (arg1 > 0) {
            printlog = arg1;
        }
        
        switch (arg2) {
            case 0:
            sprintf(fileStr, "JPG");
            fileformat = 0;
            break;
            case 1:
            sprintf(fileStr, "BMP");
            fileformat = 1;
            break;
            default:
            sprintf(fileStr, "JPG");
            fileformat = 0;
            break;
        }

        if (arg3 > 0) {
            bufmax = arg3*1024*1024;
        } else {
            bufmax = 256*1024*1024;
        }
        
        printf("usb recv buff size:[%d] format:[%s] \n", bufsize, fileStr); 
        
        ptfd[0].events = POLLIN;
        fsave = find_save(ptfilepath, ptfileSave);
        if (!fsave) {
            goto end;    
        }

        printf("usb find save [%s] \n", ptfilepath);

        //bufmax = 512*1024*1024;
        ptbuf = malloc(bufmax);  //1024*32768
        if (ptbuf) {
            printf("usb allocate memory size:[%d] \n", bufmax);
        } else {
            printf("usb allocate memory size:[%d] failed!!!! \n", bufmax);
        }

        ptrecv = ptbuf;
        while(1) {
            //ptret = read();
            ptret = poll(ptfd, 1, -1);
            //printf("usb poll ret: %d \n", ptret);
            //if (ptret < 0) {
                //printf("usb poll failed ret: %d\n", ptret);
                //break;
            //}

            if (ptret && (ptfd[0].revents & POLLIN)) {
                recvsz = read(ptfd[0].fd, ptrecv, bufsize);

                if (cntTx == 0) {
                    clock_gettime(CLOCK_REALTIME, &tstart);
                    cntTx ++;
                }
                
                //printf("usb recv ret: %d \n", recvsz);
                if (recvsz <= 1) {
                    switch (recvsz) {
                        case 1:
                            printf("usb recv %d byte wait last trunk 64 bytes \n", recvsz);

                            clock_gettime(CLOCK_REALTIME, &tend);
                            
                            bufsize = 64;
                            ptret = read(ptfd[0].fd, lastTrunk, bufsize);
                            printf("usb recv last trunk size: %d, break!!!\n", ptret);        

                            shmem_dump(lastTrunk, 64);

                            switch (fileformat) {
                                case 0:
                                    tlen = lastTrunk[4] + (lastTrunk[5] << 8);
                                    printf("%s scan length: %d \n",fileStr, tlen);
                                    changeJpgLen(ptbuf, tlen, acusz);
                                    break;
                                case 1:
                                    tlen = lastTrunk[4] + (lastTrunk[5] << 8);
                                    filelen = lastTrunk[7] + (lastTrunk[8] << 8) + (lastTrunk[9] << 16) + (lastTrunk[10] << 24);
                                    
                                    printf("%s scan length: %d file length: %d \n",fileStr, tlen, filelen);
                                    
                                    memcpy(&pbh->aspbmpMagic[2], ptbuf, sizeof(struct slvbitMapHeader_s) - 2);

                                    mspbitmapHeaderSetup(pbh, 24, pbh->aspbiWidth, tlen, -1, acusz+recvsz);

                                    memcpy(ptbuf, &pbh->aspbmpMagic[2], sizeof(struct slvbitMapHeader_s) - 2);
                                    
                                    printf("%s scan length: %d filelen: %d\n", fileStr, tlen, filelen);
                                    break;
                                default:
                                    printf("unknown file format: %d \n", fileformat);
                                    tlen = 0;
                                    break;
                            }
                            
                            break;
                        case 0:
                            continue;
                            break;
                        default:
                            printf("usb recv ret: %d, error!!!", recvsz);
                            break;
                    }
                    break;
                }
                
                /*
                if (!recvsz) {
                    continue;
                }
                */
                
                /*
                if (cntRecv == 0) {
                    clock_gettime(CLOCK_REALTIME, &tstart);
                }
                */

                //wrtsz = fwrite(ptrecv, 1, recvsz, fsave);
                wrtsz = recvsz;

                //sync();
                
                acusz += wrtsz;
#if (USB_SAVE_RESULT == 1)
                ptrecv += wrtsz;
#endif
/*
                if (printlog) {
                    //cntRecv ++;
                    printf("usb r %d w %d tot %d \n", recvsz, wrtsz, acusz);
                }
*/
                
/*
                if (recvsz < bufsize) {
                    printf("usb recv end last recv = %d \n", recvsz);
                    clock_gettime(CLOCK_REALTIME, &tend);
                    break;
                }
*/
            }

            
        }

        usCost = test_time_diff(&tstart, &tend, 1000);

        throughput = acusz*8.0 / usCost*1.0;
        
        printf("usb throughput: %d bytes / %d us = %lf MBits\n", acusz, usCost, throughput);

        printf("write file size [%d] \n", acusz);

#if (USB_SAVE_RESULT == 1)
        ptrecv = ptbuf;
        while (acusz) {
            if (acusz > 32768) {
                wrtsz = 32768;
            } else {
                wrtsz = acusz;
            }
            
            recvsz = fwrite(ptrecv, 1, wrtsz, fsave);
            if (recvsz <= 0) {
                break;
            }
            
            acusz -= recvsz;
            ptrecv += recvsz;

            //printf("write file [%d] \n", acusz);
        }
#endif        
        sync();

        close(ptfd[0].fd);
        fclose(fsave);
        free(ptbuf);
        goto end;
    }
    
    if (sel == 29){ /* usb printer write access */
        //#define PT_BUF_SIZE (512)
        struct pollfd ptfd[1];
        static char ptdevpath[] = "/dev/g_printer";
        static char ptfileSend[] = "/mnt/mmc2/usb/greenhill_01.jpg";
        char ptfilepath[128];
        char *ptrecv, *ptsend, *palloc=0;
        int ptret=0, recvsz=0, acusz=0, wrtsz=0, maxsz=0, sendsz=0, lastsz=0;
        int cntTx=0, usCost=0, bufsize=0;
        double throughput=0.0;
        FILE *fsave=0, *fsend=0;
        struct timespec tstart, tend;
        char csw[13] = {0x55, 0x53, 0x42, 0x43, 0x11, 0x22, 0x33, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00};

        ptfd[0].fd = open(ptdevpath, O_RDWR);
        if (ptfd[0].fd < 0) {
            printf("can't open device[%s]\n", ptdevpath); 
            close(ptfd[0].fd);
            goto end;
        }
        else {
            printf("open device[%s]\n", ptdevpath); 
        }

        ptfd[0].events = POLLOUT;

        printf(" open file [%s] \n", ptfileSend);
        fsend = fopen(ptfileSend, "r");
       
        if (!fsend) {
            printf(" [%s] file open failed \n", ptfileSend);
            goto end;
        }   
        printf(" [%s] file open succeed \n", ptfileSend);

        if (arg0 > 0) {
            bufsize = arg0;        
        } else {
            bufsize = PT_BUF_SIZE;
        }
        printf(" recv buff size:[%d] \n", bufsize);

        ptret = fseek(fsend, 0, SEEK_END);
        if (ptret) {
            printf(" file seek failed!! ret:%d \n", ptret);
            goto end;
        }

        maxsz = ftell(fsend);
        printf(" file [%s] size: %d \n", ptfileSend, maxsz);

        ptrecv = malloc(PT_BUF_SIZE);
        if (ptrecv) {
            printf(" recv buff alloc succeed! size: %d \n", maxsz);
        } else {
            printf(" recv buff alloc failed! size: %d \n", maxsz);
            goto end;
        }
        
        ptsend = malloc(maxsz);
        if (ptsend) {
            printf(" send buff alloc succeed! size: %d \n", maxsz);
        } else {
            printf(" send buff alloc failed! size: %d \n", maxsz);
            goto end;
        }

        ptret = fseek(fsend, 0, SEEK_SET);
        if (ptret) {
            printf(" file seek failed!! ret:%d \n", ptret);
            goto end;
        }
        
        ptret = fread(ptsend, 1, maxsz, fsend);
        printf(" output file read size: %d/%d \n", ptret, maxsz);

        fclose(fsend);

#define STAGE_DELAY  //printf("\nPass!!!\n"); //sleep(2)

        while (1) {
            /* send ready */
            //memset(ptrecv, 0, PT_BUF_SIZE);
            ptfd[0].events = POLLIN;

            ptret = poll(ptfd, 1, -1);
            printf("poll return %d \n", ptret);
            
            if (ptret < 0) {
                 printf("poll return %d \n", ptret);
                 goto end;
            }

            if (ptret && (ptfd[0].revents & POLLIN)) {
                recvsz = read(ptfd[0].fd, ptrecv, 32);
                printf("usb recv size: %d - 1\n", recvsz); 
                shmem_dump(ptrecv, recvsz);

                if ((ptrecv[0] == 0x55) && (ptrecv[1] == 0x53) && (ptrecv[2] == 0x42)) {
                    if ((ptrecv[15] == 0x08) && (ptrecv[16] == 0x0b) && (ptrecv[17] == 0x85)) {
                        //break;
                    }
                }
            }
            
            //STAGE_DELAY;
        }

        printf("usb scan start!!! - 1\n");
        
        STAGE_DELAY;

        printf("usb scan start!!! - 2\n");
        
        /* send META CBW */
        //memset(ptrecv, 0, PT_BUF_SIZE);
        ptfd[0].events = POLLIN;

        ptret = poll(ptfd, 1, -1);
        printf("poll return %d \n", ptret);
            
        if (ptret < 0) {
             printf("poll return %d \n", ptret);
             goto end;
        }

        if (ptret && (ptfd[0].revents & POLLIN)) {
            recvsz = read(ptfd[0].fd, ptrecv, 31);
            printf("usb recv size: %d - 2\n", recvsz); 
            shmem_dump(ptrecv, recvsz);
        }

        STAGE_DELAY;
        
        /* send META content */
        //memset(ptrecv, 0, PT_BUF_SIZE);
        ptfd[0].events = POLLIN | POLLOUT;

        ptret = poll(ptfd, 1, -1);
        printf("poll return %d \n", ptret);
            
        if (ptret < 0) {
             printf("poll return %d \n", ptret);
             goto end;
        }

        if (ptret && (ptfd[0].revents & POLLIN)) {
            recvsz = read(ptfd[0].fd, ptrecv, 512);
            printf("usb recv size: %d - 3\n", recvsz); 
            shmem_dump(ptrecv, recvsz);
        }

        STAGE_DELAY;
        
        /* reply META CSW */
        //memset(ptrecv, 0, PT_BUF_SIZE);
        ptfd[0].events = POLLOUT;

        ptret = poll(ptfd, 1, -1);
        printf("poll return %d \n", ptret);
            
        if (ptret < 0) {
             printf("poll return %d \n", ptret);
             goto end;
        }

        csw[12] = 1;
        
        if (ptret && (ptfd[0].revents & POLLOUT)) {
            wrtsz = write(ptfd[0].fd, csw, 13);
            printf("usb write size: %d - 3\n", wrtsz); 
            shmem_dump(csw, wrtsz);
        }
        
        STAGE_DELAY;
        
        /* send ready */
        memset(ptrecv, 0, PT_BUF_SIZE);
        ptfd[0].events = POLLIN;

        ptret = poll(ptfd, 1, -1);
        printf("poll return %d \n", ptret);
            
        if (ptret < 0) {
             printf("poll return %d \n", ptret);
             goto end;
        }

        if (ptret && (ptfd[0].revents & POLLIN)) {
            recvsz = read(ptfd[0].fd, ptrecv, 31);
            printf("usb recv size: %d - 4\n", recvsz); 
            shmem_dump(ptrecv, recvsz);
        }

        STAGE_DELAY;
        
        /* send SCAN opcode CBW */
        memset(ptrecv, 0, PT_BUF_SIZE);
        ptfd[0].events = POLLIN;

        ptret = poll(ptfd, 1, -1);
        printf("poll return %d \n", ptret);
            
        if (ptret < 0) {
             printf("poll return %d \n", ptret);
             goto end;
        }

        if (ptret && (ptfd[0].revents & POLLIN)) {
            recvsz = read(ptfd[0].fd, ptrecv, 31);
            printf("usb recv size: %d - 4\n", recvsz); 
            shmem_dump(ptrecv, recvsz);
        }

        STAGE_DELAY;
        
        /* reply CSW */
        //memset(ptrecv, 0, PT_BUF_SIZE);
        ptfd[0].events = POLLOUT;

        ptret = poll(ptfd, 1, -1);
        printf("poll return %d \n", ptret);
            
        if (ptret < 0) {
             printf("poll return %d \n", ptret);
             goto end;
        }
        
        csw[12] = 1;

        if (ptret && (ptfd[0].revents & POLLOUT)) {
            wrtsz = write(ptfd[0].fd, csw, 13);
            printf("usb write size: %d - 3\n", wrtsz); 
            shmem_dump(csw, wrtsz);
        }

        STAGE_DELAY;
        
        /* send start scan */
        memset(ptrecv, 0, PT_BUF_SIZE);
        ptfd[0].events = POLLIN;

        ptret = poll(ptfd, 1, -1);
        printf("poll return %d \n", ptret);
            
        if (ptret < 0) {
             printf("poll return %d \n", ptret);
             goto end;
        }

        if (ptret && (ptfd[0].revents & POLLIN)) {
            recvsz = read(ptfd[0].fd, ptrecv, 31);
            printf("usb recv size: %d - 4\n", recvsz); 
            shmem_dump(ptrecv, recvsz);
        }

        //STAGE_DELAY;
        
        /* reply image */
        memset(ptrecv, 0, PT_BUF_SIZE);
        ptfd[0].events = POLLOUT;
        
        lastsz = maxsz;        
        palloc = ptsend;
        while(1) {

            ptret = poll(ptfd, 1, -1);

            if (ptret < 0) {
                printf("poll return %d \n", ptret);
                break;
            }

            if (ptret && (ptfd[0].revents & POLLOUT)) {
                if (lastsz > bufsize) {
                    wrtsz = bufsize;
                } else {
                    wrtsz = lastsz;
                    if (wrtsz == 1) {
                        wrtsz = 2;
                    }
                }
                
                sendsz = write(ptfd[0].fd, ptsend, wrtsz);
                
                if (sendsz < 0) {
                    printf("usb send ret: %d, error!!!", sendsz);
                    break;
                }

                if (cntTx == 0) {
                    clock_gettime(CLOCK_REALTIME, &tstart);
                }
                
                ptsend += sendsz;
                acusz += sendsz;
                lastsz = lastsz - sendsz;
                cntTx ++;
                
                //printf("usb send %d/%d to usb total %d last %d\n", sendsz, wrtsz, acusz, lastsz);

                if (lastsz == 0) {
                    sendsz = write(ptfd[0].fd, ptsend, 1);
                    printf("usb send end last size = %d \n", sendsz);
                    clock_gettime(CLOCK_REALTIME, &tend);
                    break;
                }
            }
            
        }

        usCost = test_time_diff(&tstart, &tend, 1000);

        throughput = maxsz*8.0 / usCost*1.0;
        
        printf("usb throughput: %d bytes / %d us = %lf MBits\n", maxsz, usCost, throughput);
        
        close(ptfd[0].fd);
        free(palloc);
        goto end;
    }

    if (sel == 28){ /* list the files in root ex[28]*/
       struct directnFile_s *root = 0;
       char topdir[256] = "/root";
       int ret;
       if (argc > 2) {
           strcpy(topdir, argv[2]);
       }
    
       printf("Directory scan of [%s], %d, %d, %d\n", topdir, ASPFS_TYPE_ROOT, ASPFS_TYPE_DIR, ASPFS_TYPE_FILE);

       ret = aspFS_createRoot(&root, topdir);
       if (ret != 0) printf("[R]aspFS_createRoot failed ret: %d \n", ret);
       else printf("[R]aspFS_createRoot ok ret: %d \n", ret);
       ret = aspFS_insertChilds(root);
       if (ret != 0) printf("[R]aspFS_insertChilds failed ret: %d \n", ret);
       else printf("[R]aspFS_insertChilds ok ret: %d \n", ret);

       printf("[R]aspFS_list...\n");
       aspFS_list(root, 4);

       int i = 0;
       char ch;
       char path[256];
       struct directnFile_s *dfs, *cur;

#if 1
       i = 0;
       while (1) {
           ch = fgetc(stdin);
           if (ch != '\n')
               printf("%c", ch);
           path[i] = ch;
           if (ch == '\n') {
               path[i] = '\0';
            break;
           }
           i++;
       }
       printf("search [%s]\n", path);
       
       ret = aspFS_search(&dfs, root, path);
    if (!ret)   
           printf("ret = %d, search [%s], [%s]\n\n\n",ret ,path, dfs->dfLFN);

       aspFS_showFolder(dfs);
#else
        cur = root;
        while (1) {
            i = 0;
            while (1) {
                ch = fgetc(stdin);
                if (ch != '\n')
                    printf("%c", ch);
                path[i] = ch;
                if (ch == '\n') {
                    path[i] = '\0';
                 break;
                }
                i++;
            }

            ret = aspFS_folderJump(&dfs, cur, path);
            printf("\n folder jump, ret: %d \n", ret);
            if (ret > 0) {
                aspFS_showFolder(dfs);              
                cur = dfs;
            }

        }
#endif

       goto end;
    }

    if (sel == 27){ /* list the files in root ex[27]*/
       char topdir[256] = ".";
       if (argc > 2) {
           strcpy(topdir, argv[2]);
       }
    
    printf("Directory scan of %s\n", topdir);
    printdir(topdir, 0);
    printf("done.\n");

    goto end;
    }

    if (sel == 26){ /* list the files in root ex[26]*/
        
  char name[256][256];
  DIR           *d;
  struct dirent *dir;
  int count = 0;
  int index = 0;
  d = opendir(".");
  if (d)
  {
    while ((dir = readdir(d)) != NULL)
    {
      printf("%s - 0x%x\n", dir->d_name, dir->d_type);
      strcpy(name[count],dir->d_name);
      count++;
    }

    closedir(d);
  }

  while( count > 0 )
  {
      printf("The directory list is %s\r\n",name[index]);
      index++;
      count--;
  }

        goto end;
    }

    if (sel == 25){ /* dual band data mode test with kthread ex[25 0 2 5 1 SPI_TRUNK_SZ]*/
        #define PKTSZ  30720 //SPI_TRUNK_SZ
        int chunksize;
        if (arg4 == PKTSZ) chunksize = PKTSZ;
        else chunksize = SPI_TRUNK_SZ;
        
        printf("***chunk size: %d ***\n", chunksize);
        
        int pipefd[2];
        int pipefs[2];
        int pipefc[2];
        char *tbuff, *tmp, buf='c', *dstBuff, *dstmp;
        char lastaddr[48];
        int sz = 0, wtsz = 0, lsz = 0;
        char *addr;
        int pid = 0;
        int txhldsz = 0;

//redo:  /*not used*/

    sz = 0;
    wtsz = 0;
    lsz = 0;
    pid = 0;
    txhldsz = 0;
    tmp = 0;
    buf='c';
        //tbuff = malloc(TSIZE);
        tbuff = rx_buff[0];
        dstBuff = mmap(NULL, TSIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
        memset(dstBuff, 0x95, TSIZE);
        dstmp = dstBuff;
        
        switch(arg3) {
            case 1:
                    mode |= SPI_MODE_1;
                break;
            case 2:
                    mode |= SPI_MODE_2;
                break;
            case 3:
                    mode |= SPI_MODE_3;
                break;
            default:
                    mode |= SPI_MODE_0;
                break;
        }
        /*
         * spi mode 
         */ 
        ret = ioctl(fm[0], SPI_IOC_WR_MODE, &mode);    //?家Α 
        if (ret == -1) 
            pabort("can't set spi mode"); 
        
        ret = ioctl(fm[0], SPI_IOC_RD_MODE, &mode);    //?家Α 
        if (ret == -1) 
            pabort("can't get spi mode"); 
        
        /*
         * spi mode 
         */ 
        ret = ioctl(fm[1], SPI_IOC_WR_MODE, &mode);    //?家Α 
        if (ret == -1) 
            pabort("can't set spi mode"); 
        
        ret = ioctl(fm[1], SPI_IOC_RD_MODE, &mode);    //?家Α 
        if (ret == -1) 
            pabort("can't get spi mode"); 
        
        bitset = 1;
        ret = ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
        printf("Set slve ready: %d\n", bitset);
        
        bitset = 1;
        ret = ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
        printf("Set slve ready: %d\n", bitset);
        
        bitset = 0;
        ret = ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_WT_CTL_PIN
        printf("Set spi0 RDY: %d\n", bitset);
        
        bitset = 0;
        ret = ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_WT_CTL_PIN
        printf("Set spi1 RDY: %d\n", bitset);
        
        bitset = 0;
        ret = ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        printf("Set spi0 data mode: %d\n", bitset);
        
        bitset = 0;
        ret = ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        printf("Set spi1 data mode: %d\n", bitset);
        
        struct timespec *tspi = (struct timespec *)mmap(NULL, sizeof(struct timespec) * 2, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);;
        if (!tspi) {
            printf("get share memory for timespec failed - %d\n", (uint32_t)tspi);
            goto end;
        }
        clock_gettime(CLOCK_REALTIME, &tspi[0]);  
        clock_gettime(CLOCK_REALTIME, &tspi[1]);  
        int tcost, tlast;
        struct tms time;
        struct timespec curtime, tdiff[2];
        
        if (tbuff!=0)
            printf("%d bytes memory alloc succeed! [0x%.8x]\n", TSIZE, (uint32_t)tbuff);
        else 
            goto end;
        
        tmp = tbuff;
        
        sz = 0;
        pipe(pipefd);
        pipe(pipefs);
        pipe(pipefc);
        
        pid = fork();
    
        if (pid) {
            pid = fork();
            if (pid) {
                uint8_t *cbuff;
                cbuff = tx_buff[1];
                printf("main process to monitor the p0 and p1 \n");
                close(pipefd[0]); // close the read-end of the pipe, I'm not going to use it
                close(pipefs[1]); // close the write-end of the pipe, I'm not going to use it
                close(pipefd[1]); // close the write-end of the pipe, thus sending EOF to the reader
                close(pipefs[0]); // close the read-end of the pipe
                close(pipefc[1]); // close the write-end of the pipe, thus sending EOF to the reader
                
                txhldsz = arg1;
                
                while (1) {
                    //sleep(3);                 
                    ret = read(pipefc[0], &buf, 1); 

                    if (ret <= 0) {
                        //printf("[25] did get pipe, buf:%c\n", buf);
               continue;
                    }
        
                    if ((buf == '0') || (buf == '1')) {
                        cbuff[wtsz] = buf;
                        wtsz++;
                        if (arg0) {
                            if (wtsz == txhldsz) {
        
                                bitset = 1;
                                ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 13, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
                                
                                bitset = 1;
                                ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 13, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
                                
                                if (arg2>0)
                                    lsz = arg2;
                                else
                                    lsz = 10;
                                
                                while(lsz) {
                                    bitset = 0;
                                    ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
                                    //printf("Set RDY: %d, dly:%d \n", bitset, arg2);
                                    usleep(1);
                                
                                    bitset = 1;
                                    ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
                                    //printf("Set RDY: %d\n", bitset);
                                    usleep(1);
                                
                                    lsz --;
                                }
                                
                                bitset = 0;
                                ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 13, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
                                //printf("Set spi0 Tx Hold: %d\n", bitset);
                                
                                bitset = 0;
                                ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 13, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
                                //printf("Set spi1 Tx Hold: %d\n", bitset);
                                
                            }
                        }
                    } else if (buf == 'e') {
                        ret = read(pipefc[0], lastaddr, 32); 
                        sz = atoi(lastaddr);
                        printf("********//main monitor, write count:%d, sz:%d str:%s//********\n", wtsz, sz, lastaddr);
                        for (sz = 0; sz < wtsz; sz++) {
                            printf("%c, ", cbuff[sz]);
                            if (!((sz+1) % 16)) printf("\n");
                        }
                        break;
                    } else {
                        printf("********//main monitor, get buff:%c, ERROR!!!!//********\n", buf);
                    }
                    
                }
                close(pipefc[0]); // close the read-end of the pipe
        
            } else {
                printf("p0 process pid:%d!\n", pid);
                char str[256] = "/root/p0.log";
                
                close(pipefd[0]); // close the read-end of the pipe, I'm not going to use it
                close(pipefs[1]); // close the write-end of the pipe, I'm not going to use it
                close(pipefc[0]); // close the read-end of the pipe
                
                printf("Start spi%d spidev thread, ret: 0x%x\n", 0, ret);
                bitset = 0;
                ret = ioctl(fm[0], _IOR(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_START_THREAD

                while(1) {
                    clock_gettime(CLOCK_REALTIME, &tspi[0]);   
                    
                    ret = ioctl(fm[0], _IOR(SPI_IOC_MAGIC, 15, __u32), dstBuff);  //SPI_IOC_PROBE_THREAD
                    
                    clock_gettime(CLOCK_REALTIME, &tdiff[0]);    
                     msync(tspi, sizeof(struct timespec)*2, MS_SYNC);
                     tlast = test_time_diff(&tspi[1], &tspi[0], 1000);
                     if (tlast == -1) {
                         tlast = 0 - test_time_diff(&tspi[0], &tspi[1], 1000);
                     }
                     tcost = test_time_diff(&tspi[0], &tdiff[0], 1000);
                     
            if (ret == 0) {
                continue;
            }

                    printf("[p0]rx %d - %d (%d us /%d us)\n", ret, wtsz++, tcost, tlast);

                    if (ret > 0) {
                        msync(dstBuff, ret, MS_SYNC);
                        dstBuff += ret + chunksize;
                    }

                    if (ret < 0) {

               ret = 0 - ret;
               if (ret == 1) ret = 0;

                        dstBuff -= chunksize;
                        lsz = ret;
                        write(pipefd[1], "e", 1); // send the content of argv[1] to the reader
                        sprintf(lastaddr, "%d", (uint32_t)dstBuff);
                        printf("p0 write e  addr:%x str:%s ret:%d\n", (uint32_t)dstBuff, lastaddr, ret);
                        write(pipefd[1], lastaddr, 32); 
                        break;
                    }
                    write(pipefd[1], "c", 1);
                    
                    /* send info to control process */
                    write(pipefc[1], "0", 1);
                    //printf("main process write c \n");    
                }


             bitset = 0;
          ret = ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
                 printf("Stop spi%d spidev thread, ret: %d\n", 0, ret);
                
                wtsz = 0;
                while (1) {
                    ret = read(pipefs[0], &buf, 1); 
                    //printf("p0 read %d, buf:%c \n", ret, buf);
                    if (buf == 'c') {
                        wtsz++; 
                    } else if (buf == 'e') {
                        ret = read(pipefs[0], lastaddr, 32); 
                        addr = (char *)atoi(lastaddr);
                        printf("p0 process wait wtsz:%d, lastaddr:%x/%x\n", wtsz, (uint32_t)addr, (uint32_t)dstBuff);
                        break;
                    }
                }
                
                if (dstBuff > addr) {
                    printf("p0 memcpy from  %x to %x sz:%d\n", (uint32_t)dstBuff-lsz, (uint32_t)addr, lsz);
                    msync(dstBuff-lsz, lsz, MS_SYNC);
                    #if 0
                    memcpy(addr, dstBuff-lsz, lsz);
                    #else
                    memcpy(tbuff, dstBuff-lsz, lsz);
                    memcpy(addr, tbuff, lsz);
                    #endif
                    addr += lsz;
                    memset(addr, 0, chunksize);
                    sz = addr - dstmp;
                    
                    /* send info to control process */
                    write(pipefc[1], "e", 1);
                    sprintf(lastaddr, "%d", sz);
                    printf("p0 write e, sz:%x str:%s \n", sz, lastaddr);
                    write(pipefc[1], lastaddr, 32);
        
                    msync(dstmp, sz, MS_SYNC);
                    ret = fwrite(dstmp, 1, sz, fp);
                    printf("\np0 write file [%s] size %d/%d \n", path, sz, ret);
                }
                
                close(pipefd[1]); // close the write-end of the pipe, thus sending EOF to the reader
                close(pipefs[0]); // close the read-end of the pipe
                close(pipefc[0]); // close the read-end of the pipe
            }
        } else {
            char str[256] = "/root/p1.log";
            printf("p1 process pid:%d!\n", pid);
        
            close(pipefd[1]); // close the write-end of the pipe, I'm not going to use it
            close(pipefs[0]); // close the read-end of the pipe, I'm not going to use it
            close(pipefc[0]); // close the read-end of the pipe
        
            printf("Start spi%d spidev thread, ret: 0x%x\n", 1, ret);
            bitset = 0;
            ret = ioctl(fm[1], _IOR(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_START_THREAD

            dstBuff += chunksize;
            
            while(1) {              
                clock_gettime(CLOCK_REALTIME, &tspi[1]);            
                
                ret = ioctl(fm[1], _IOR(SPI_IOC_MAGIC, 15, __u32), dstBuff);  //SPI_IOC_PROBE_THREAD

                clock_gettime(CLOCK_REALTIME, &tdiff[1]);   
                msync(tspi, sizeof(struct timespec) * 2, MS_SYNC);
                tlast = test_time_diff(&tspi[0], &tspi[1], 1000);
                if (tlast == -1) {
                     tlast = 0 - test_time_diff(&tspi[1], &tspi[0], 1000);
                }
                tcost = test_time_diff(&tspi[1], &tdiff[1], 1000);


                if (ret == 0) {
            continue;
          }

                printf("[p1]rx %d - %d (%d us /%d us)\n", ret, wtsz++, tcost, tlast);

                if (ret > 0) {
                    msync(dstBuff, ret, MS_SYNC);
                    dstBuff += ret + chunksize;
                }
                if (ret < 0) {

            ret = 0 - ret;
                    if (ret == 1) ret = 0;
                    
                    dstBuff -= chunksize;
                    lsz = ret;
                    write(pipefs[1], "e", 1); // send the content of argv[1] to the reader
                    sprintf(lastaddr, "%d", (uint32_t)dstBuff);
                    printf("p1 write e addr:%x str:%s ret:%d\n", (uint32_t)dstBuff, lastaddr, ret);
                    write(pipefs[1], lastaddr, 32); // send the content of argv[1] to the reader

                    break;
                }
                write(pipefs[1], "c", 1); // send the content of argv[1] to the reader         
                
                /* send info to control process */
                write(pipefc[1], "1", 1);
                //printf("p1 process write c \n");
            }

            bitset = 0;
            ret = ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
            printf("Stop spi%d spidev thread, ret: %d\n", 1, ret);

            wtsz = 0;
            while (1) {
                ret = read(pipefd[0], &buf, 1); 
                //printf("p1 read %d, buf:%c  \n", ret, buf);
                if (buf == 'c')
                    wtsz++; 
                else if (buf == 'e') {
                    ret = read(pipefd[0], lastaddr, 32); 
                    addr = (char *)atoi(lastaddr);
                    printf("p1 process wtsz:%d, lastaddr:%x/%x\n",  wtsz, (uint32_t)addr, (uint32_t)dstBuff);
                    break;
                }
            }
        
            if (dstBuff > addr) {
                printf("p1 memcpy from  %x to %x sz:%d\n", (uint32_t)dstBuff-lsz, (uint32_t)addr, lsz);
                msync(dstBuff-lsz, lsz, MS_SYNC);
                #if 0
                memcpy(addr, dstBuff-lsz, lsz);
                #else
                memcpy(tbuff, dstBuff-lsz, lsz);
                memcpy(addr, tbuff, lsz);
                #endif
                addr += lsz;
                memset(addr, 0, chunksize);
                sz = addr - dstmp;
                
                /* send info to control process */
                write(pipefc[1], "e", 1);
                sprintf(lastaddr, "%d", sz);
                printf("p1 write e sz:%x str:%s \n", sz, lastaddr);
                write(pipefc[1], lastaddr, 32); 
        
                msync(dstmp, sz, MS_SYNC);
                ret = fwrite(dstmp, 1, sz, fp);
                printf("\np1 write file [%s] size %d/%d \n", path, sz, ret);
            }
        
            close(pipefd[0]); // close the read-end of the pipe
            close(pipefs[1]); // close the write-end    of the pipe, thus sending EOF to the
            close(pipefc[0]); // close the read-end of the pipe
        }
    
        munmap(dstmp, TSIZE);
     //goto redo;
        goto end;
    }
    
    
    if (sel == 24){ /* command mode test ex[24 0/num 1/0]*/
        int ret=0, max=0;
        FILE *dkf;
        char diskpath[128], *dkbuf = 0;
        struct sdRaw_s *raw;

        strcpy(diskpath, argv[3]);
        printf(" open file [%s] \n", diskpath);
        dkf = fopen(diskpath, "r");
       
        if (!dkf) {
            printf(" [%s] file open failed \n", diskpath);
            goto end;
        }   
        printf(" [%s] file open succeed \n", diskpath);

        ret = fseek(dkf, 0, SEEK_END);
        if (ret) {
            printf(" file seek failed!! ret:%d \n", ret);
            goto end;
        }

        max = ftell(dkf);
        printf(" file [%s] size: %d \n", diskpath, max);
        dkbuf = malloc(max);
        if (dkbuf) {
            printf(" dkbuf alloc succeed! size: %d \n", max);
        } else {
            printf(" dkbuf alloc failed! size: %d \n", max);
            goto end;
        }

        ret = fseek(dkf, 0, SEEK_SET);
        if (ret) {
            printf(" file seek failed!! ret:%d \n", ret);
            goto end;
        }
        
        ret = fread(dkbuf, 1, max, dkf);
        printf(" dk file read size: %d/%d \n", ret, max);

        raw = (struct sdRaw_s *) dkbuf;

        int i, j;
        /* dump raw */
        
#if 0
        for (i = 0; i < (max/512); i++) {
            printf("[%d] \n", i);
            for (j = 0; j < 512; j++) {
                if ((j % 16) == 0) printf("%.6x ", i * 512 + j);
                printf("%.2x ", raw[i].rowBP[j]);
                if (((j +1) % 16) == 0) printf("\n");
            }
        }
#endif


#define SF0 (8 * 0)
#define SF1 (8 * 1)
#define SF2 (8 * 2)
#define SF3 (8 * 3)

        struct sdFAT_s *pfat;
        struct sdbootsec_s  *psec;
        char *pr;

        pfat = malloc(sizeof(struct sdFAT_s));
        memset(pfat, 0, sizeof(struct sdFAT_s));
        
        pfat->fatBootsec = malloc(sizeof(struct sdbootsec_s));
        memset(pfat->fatBootsec, 0, sizeof(struct sdbootsec_s));
        
        pfat->fatFSinfo = malloc(sizeof(struct sdFSinfo_s));
        memset(pfat->fatFSinfo, 0, sizeof(struct sdFSinfo_s));
        
        pfat->fatRootdir = 0;
        pfat->fatTable = 0;

        int cnt;
        struct directnFile_s * fsds = 0;

        fsds = (struct directnFile_s *) malloc(sizeof(struct directnFile_s));
        memset(fsds, 0, sizeof(struct directnFile_s));

        char *next, *root;
        int last=0, rootlast;
        int offset=0;
        struct directnFile_s   *ch, *br;

        switch (arg0) {
        case 0: /* read the boot sector */
            psec = pfat->fatBootsec;
            pr = raw[0].rowBP;
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
            break;
        case 1: /* read the fat table */
            cnt = 0;
            ret = aspRawParseDir(dkbuf, fsds, max);
            printf(" raw parsing cnt: %d \n", ret);
            while (max > 0) {
                if (fsds->dfstats) {
                    printf("short name: %s \n", fsds->dfSFN);
                    if (fsds->dflen > 0) {
                        printf("long name: %s, len:%d \n", fsds->dfLFN, fsds->dflen);
                    }
                    debugPrintDir(fsds);
                }

                dkbuf += ret;
                max -= ret;
                cnt++;
                memset(fsds, 0x0, sizeof(struct directnFile_s));
                ret = aspRawParseDir(dkbuf, fsds, max);
                if (!ret) break;
                printf("[%d] ret: %d, last:%d \n", cnt, ret, max);
            }

            printf(" raw parsing end: %d \n", ret);
            break;
        case 2: /* read the dir tree */
            printf("[0x%x]root dir offset: %d per:%d\n", (uint32_t)dkbuf, arg2, arg3);
            prinfatdir(dkbuf, max, arg2, 4, arg2, arg3);
            break;
        case 3:
            ret = aspFS_createFATRoot(pfat);
            if (ret == 0) {
                aspFS_insertFATChilds(pfat->fatRootdir, dkbuf, max);
            }
            break;
        case 4:

            offset = arg2 * 512;
            root = dkbuf + offset;
            rootlast = max - offset;
            
            ret = aspFS_createFATRoot(pfat);
            if (ret == 0) {
                aspFS_insertFATChilds(pfat->fatRootdir, root, rootlast);
            }

            ch = pfat->fatRootdir->ch;

            if (!ch) break;
            
            br = ch->br;            
            while (br) {
                if (br->dfattrib & ASPFS_ATTR_DIRECTORY) {
                    offset = (br->dfclstnum - 2) * arg3;
                    offset = offset * 512;

                    next = root + offset;
                    last = rootlast - offset;
                     aspFS_insertFATChilds(br, next, last);
                }
                br = br->br;            
            }
            
            break;
        default:
            break;
        }   
        goto end;
    }
    if (sel == 23){ /* command mode test ex[23 1/0]*/
#define TRUNK_SIZE 32768

        int ret = 0, len = 0, cnt = 0, acusz = 0;
        int spis = 0;

        spis = arg0 % 2;
        // disable data mode
        bitset = 0;
        ret = ioctl(fm[spis], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        printf("Set spi%d data mode: %d\n", spis, bitset);

        bitset = 1;
        ret = ioctl(fm[spis], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
        printf("Set spi%d slve ready: %d\n", spis, bitset);

        memset(tx_buff[0], 0xaa, TRUNK_SIZE);

        while (1) {
            memset(rx_buff[0], 0x95, TRUNK_SIZE);

            len = tx_data(fm[spis], rx_buff[0], tx_buff[0], 1, TRUNK_SIZE, 1024*1024);

            msync(rx_buff[0], len, MS_SYNC);
            if (len < 0) {
                len = 0 - len;
                ret = fwrite(rx_buff[0], 1, len, fp);
                printf("[%d]receive %d bytes, write to file %d bytes - end\n", cnt, len, ret);
                break;
            }
            ret = fwrite(rx_buff[0], 1, len, fp);
            printf("[%d]receive %d bytes, write to file %d bytes \n", cnt, len, ret);
            acusz += len;
            cnt++;            
            len = 0;
            ret = 0;
        }

        acusz += len;
        printf("file save [%s], receive total size: %d \n", path, acusz);

        goto end;
    }
    if (sel == 22){ /* command mode test ex[22 num]*/

#define ARRY_MAX  61
        int ret=0;
        uint8_t tx8[4], rx8[4];
        uint8_t op[ARRY_MAX] = {    0xaa
                            ,   OP_PON,         OP_QRY,         OP_RDY,         OP_DAT,         OP_SCM          /* 0x01 -0x05 */
                            ,   OP_DCM,         OP_FIH,         OP_DUL,         OP_SDRD,        OP_SDWT             /* 0x06 -0x0a */
                            ,   OP_SDAT,        OP_RGRD,        OP_RGWT,        OP_RGDAT,       OP_NONE             /* 0x0b -0x0f  */
                            ,   OP_NONE,        OP_NONE,        OP_NONE,        OP_NONE,        OP_STSEC_0      /* 0x10 -0x14  */
                            ,   OP_STLEN_0,     OP_NONE,        OP_NONE,        OP_NONE,        OP_NONE             /* 0x15 -0x19  */
                            ,   OP_NONE,        OP_NONE,        OP_NONE,        OP_NONE,        OP_NONE             /* 0x1A -0x1E  */
                            ,   OP_NONE,        OP_FFORMAT,     OP_COLRMOD, OP_COMPRAT, OP_SCANMOD      /* 0x1F -0x23  */
                            ,   OP_DATPATH,     OP_RESOLTN,     OP_SCANGAV, OP_MAXWIDH, OP_WIDTHAD_H    /* 0x24 -0x28  */
                            ,   OP_WIDTHAD_L,   OP_SCANLEN_H,   OP_SCANLEN_L,   OP_NONE,        OP_NONE     /* 0x29 -0x2D  */
                            ,   OP_NONE,        OP_NONE,        OP_NONE,        OP_SUP,        OP_NONE     /* 0x2E -0x32  */
                            ,   OP_NONE,        OP_NONE,        OP_NONE,        OP_NONE,        OP_NONE     /* 0x33 -0x37  */
                            ,   OP_NONE,        OP_NONE,        OP_NONE,        OP_NONE,        OP_MAX};        /* 0x38 -0x3C  */

        uint8_t st[4] = {OP_STSEC_0,    OP_STSEC_1, OP_STSEC_2, OP_STSEC_3};
        uint8_t ln[4] = {OP_STLEN_0,    OP_STLEN_1, OP_STLEN_2, OP_STLEN_3};
        uint8_t staddr = 0, stlen = 0, btidx = 0;

        tx8[0] = op[arg0];
        tx8[1] = arg1;

        if (arg0 > ARRY_MAX) {
            printf("Error!! Index overflow!![%d]\n", arg0);
            goto end;
        }

        btidx = arg2 % 4;
        if (op[arg0] == OP_STSEC_0) {
            staddr = (arg1 >> (8 * btidx)) & 0xff;
            
            tx8[0] = st[btidx];
            tx8[1] = staddr;
            
            printf("start secter: %.8x, send:%.2x \n", arg1, staddr);
        }

        if (op[arg0] == OP_STLEN_0) {
            stlen = (arg1 >> (8 * btidx)) & 0xff;
            tx8[0] = ln[btidx];
            tx8[1] = stlen;
            
            printf("secter length: %.8x, send:%.2x\n", arg1, stlen);
        }
        // disable data mode

        bitset = 0;
        ret = ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        printf("Set spi0 data mode: %d\n", bitset);

        bitset = 0;
        ret = ioctl(fm[0], _IOR(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_RD_DATA_MODE
        printf("Get spi0 data mode: %d\n", bitset);

        bitset = 1;
        ret = ioctl(fm[0], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_RD_CTL_PIN
        printf("Get spi0 RDY: %d\n", bitset);
        bitset = 0;
        ret = ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_WR_CTL_PIN
        printf("Set spi0 RDY: %d\n", bitset);

        bitset = 1;
        ret = ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
        printf("Set spi%d slve ready: %d\n", 0, bitset);

        bits = 8;
        ret = ioctl(fm[0], SPI_IOC_WR_BITS_PER_WORD, &bits);
        if (ret == -1) 
            pabort("can't set bits per word");  
        ret = ioctl(fm[0], SPI_IOC_RD_BITS_PER_WORD, &bits); 
        if (ret == -1) 
            pabort("can't get bits per word"); 

        ret = tx_data(fm[0], rx8, tx8, 1, 2, 1024);
        printf("len:%d, rx: 0x%.2x-0x%.2x, tx: 0x%.2x-0x%.2x \n", ret, rx8[0], rx8[1], tx8[0], tx8[1]);

        goto end;
    }
    if (sel == 21){ /* command mode test ex[21 spi size bits]*/
        int ret=0, i=0, len=0, max=0;
        uint16_t *tx16, *rx16;
        uint8_t *tx8, *rx8;
        uint8_t *tmpTx, *tmpRx;
        tx16 = malloc(SPI_TRUNK_SZ);
        rx16 = malloc(SPI_TRUNK_SZ);
        tx8 = malloc(SPI_TRUNK_SZ);
        rx8 = malloc(SPI_TRUNK_SZ);
        if (bits == 8) {
            max = arg1;
            for (i = 0; i < max; i++) {
                tx8[i] = i;
            }
            tmpTx = (uint8_t *)tx8;

            i = 0;
            printf("\n%d.", i);
            for (i = 0; i < max; i++) {
                if (((i % 16) == 0) && (i != 0)) printf("\n%d.", i);
                printf("0x%.2x ", *tmpTx);
                tmpTx++;
            }
        }

        if (bits == 16) {
            max = arg1 / 2;
            for (i = 0; i < max; i++) {
                tx16[i] = i;
            }
            tmpTx = (uint8_t *)tx16;

            i = 0;
            printf("\n%d.", i);
            for (i = 0; i < max; i++) {
                if (((i % 16) == 0) && (i != 0)) printf("\n%d.", i);
                printf("0x%.2x ", *tmpTx);
                tmpTx++;
            }
        }

        arg0 = arg0 % 2;
        len = arg1;
        bits = arg2;

        printf("\ncommand mode test spi_%d_ transmit %d bytes with bit width %d\n", arg0, len, bits);
        
        ret = ioctl(fm[arg0], SPI_IOC_WR_BITS_PER_WORD, &bits);
        if (ret == -1) 
            pabort("can't set bits per word"); 
 
        ret = ioctl(fm[arg0], SPI_IOC_RD_BITS_PER_WORD, &bits); 
        if (ret == -1) 
            pabort("can't get bits per word"); 

        bitset = 1;
        ret = ioctl(fm[arg0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
        printf("Set slve ready: %d\n", bitset);

        bitset = 0;
        ret = ioctl(fm[arg0], _IOR(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_RD_DATA_MODE
        printf("Get spi0 data mode: %d\n", bitset);

        if (len > SPI_TRUNK_SZ) len = SPI_TRUNK_SZ;

        if (bits == 8) {
            tmpTx = tx8;
            tmpRx = rx8;
        } else if (bits == 16) {
            tmpTx = (uint8_t *)tx16;
            tmpRx = (uint8_t *)rx16;
        } else {
            tmpTx = 0;
            tmpRx = 0;
        }

        if ((!tmpRx) || (!tmpTx)) {
            printf("Error input memory address, break!!\n");
            goto end;
        }

        ret = tx_data(fm[arg0], tmpRx, tmpTx, 1, len, SPI_TRUNK_SZ);

        printf("\n tx end ret: %d / %d\n", ret, len);

        i = 0;
        printf("\n%d.", i);
        for (i = 0; i < len; i+=2) {
            if (((i % 16) == 0) && (i != 0)) printf("\n%d.", i);
            printf("0x%.2x-0x%.2x ", *tmpRx, *(tmpRx+1));
            tmpRx+=2;
        }
        printf("\n");

        printf("spi%d bits: %d rxsize: %d/%d\n", arg0, bits, ret, len);

        goto end;
    }
    if (sel == 20){ /* command mode test ex[20 1 path1 path2]*/
#define TCSIZE 128*1024*1024
        int acusz, send, pktcnt, modeSel;
        char path_02[256];

        FILE *fp_02;

    fp_02 = find_save(path_02, data_save);
    if (!fp_02) {
        printf("find save dst failed ret:%d\n", (uint32_t)fp_02);
        goto end;
    } else {
        printf("find save dst succeed ret:%d\n",(uint32_t)fp_02);
    }

       modeSel = 1;
    mode &= ~SPI_MODE_3;
    switch(modeSel) {
        case 1:
                mode |= SPI_MODE_1;
            break;
        case 2:
                mode |= SPI_MODE_2;
            break;
        case 3:
                mode |= SPI_MODE_3;
            break;
        default:
                mode |= SPI_MODE_0;
            break;
    }

        ret = ioctl(fm[0], SPI_IOC_WR_MODE, &mode);
        if (ret == -1) 
            pabort("can't set spi mode"); 
 
        ret = ioctl(fm[0], SPI_IOC_RD_MODE, &mode);
        if (ret == -1) 
            pabort("can't get spi mode"); 

        ret = ioctl(fm[1], SPI_IOC_WR_MODE, &mode);
        if (ret == -1) 
            pabort("can't set spi mode"); 
 
        ret = ioctl(fm[1], SPI_IOC_RD_MODE, &mode);
        if (ret == -1) 
            pabort("can't get spi mode"); 

        // disable data mode
        bitset = 1;
        ret = ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
        printf("Set spi%d slve ready: %d\n", 0, bitset);

        bitset = 1;
        ret = ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
        printf("Set spi%d slve ready: %d\n", 1, bitset);

        bitset = 0;
        ret = ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        printf("Set spi%d data mode: %d\n", 0, bitset);

        bitset = 0;
        ret = ioctl(fm[0], _IOR(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_RD_DATA_MODE
        printf("Get spi%d data mode: %d\n", 0, bitset);

        bitset = 0;
        ret = ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        printf("Set spi%d data mode: %d\n", 1, bitset);

        bitset = 0;
        ret = ioctl(fm[1], _IOR(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_RD_DATA_MODE
        printf("Get spi%d data mode: %d\n", 1, bitset);

        bitset = 1;
        ret = ioctl(fm[0], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_RD_CTL_PIN
        printf("Get spi%d RDY: %d\n", 0, bitset);
        bitset = 0;
        ret = ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_WR_CTL_PIN
        printf("Set spi%d RDY: %d\n", 0, bitset);

        bitset = 1;
        ret = ioctl(fm[1], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_RD_CTL_PIN
        printf("Get spi%d RDY: %d\n", 1, bitset);
        bitset = 0;
        ret = ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_WR_CTL_PIN
        printf("Set spi%d RDY: %d\n", 1, bitset);

        int sid;
        sid = fork();

        if (sid) {

        acusz = 0; pktcnt = 0; send = 0;
        while (1) {
            
            send = tx_data(fm[0], rx_buff[0], tx_buff[0], 1, SPI_TRUNK_SZ, 1024*1024);
        
            if (send < 0) {
                send = 0 - send;
            }

            if (send == 1) send = 0;

            acusz += send;

            if (send > 0) {
                msync(rx_buff[0], send, MS_SYNC);
                ret = fwrite(rx_buff[0], 1, send, fp);
                if (ret != send) {
                    printf("[%s]write file error, ret:%d len:%d\n", path, ret, send);
                } else {
                    //printf("[%s]write file done, ret:%d len:%d\n", path, ret, len);
                }
            }

            printf("[spi%d][%d] tx %d - %d\n", 0, pktcnt, send, acusz);

            if (send != SPI_TRUNK_SZ) {
                printf("spi%d transmitting complete, save[%s] total size: %d/\n", 1, path, acusz);
                break;
            }

            pktcnt++;
        }

        }else {

        acusz = 0; pktcnt = 0; send = 0;
        while (1) {
            
            send = tx_data(fm[1], rx_buff[1], tx_buff[1], 1, SPI_TRUNK_SZ, 1024*1024);

            if (send < 0) {
                send = 0 - send;
            }

            if (send == 1) send = 0;

            acusz += send;

            if (send > 0) {
                msync(rx_buff[1], send, MS_SYNC);
                ret = fwrite(rx_buff[1], 1, send, fp_02);
                if (ret != send) {
                    printf("[%s]write file error, ret:%d len:%d\n", path_02, ret, send);
                } else {
                    //printf("[%s]write file done, ret:%d len:%d\n", path, ret, len);
                }
            }

            printf("[spi%d][%d] tx %d - %d\n", 1, pktcnt, send, acusz);

            if (send != SPI_TRUNK_SZ) {
                printf("spi%d transmitting complete, save[%s] total size: %d/\n", 1, path_02, acusz);
                break;
            }

            pktcnt++;
        }
        
        }
        
        
        goto end;
    }

    if (sel == 19){ /* command mode test ex[19 1 path spi]*/
#define USE_SHARE_MEM 0
#define SPI_THREAD_EN 1
#define SEND_FILE 0
#define SAVE_FILE 1
        FILE *f;
        int fsize, acusz, send, txsz, pktcnt, len=0;
        char *src, *dstBuff, *dstmp;
        char *save, *svBuff, *svtmp;    
        int modeSel;
#if SEND_FILE
        if (argc > 3) {
            f = fopen(argv[3], "r");
        } else {
            printf("Error!! no input file \n");
        }
        if (!f) printf("Error!! file open failed\n");
#endif
#if USE_SHARE_MEM 
        dstBuff = mmap(NULL, TSIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
     memset(dstBuff, 0x95, TSIZE);
        dstmp = dstBuff;
#else
        dstBuff = rx_buff[0];
     memset(dstBuff, 0x95, buffsize);
        dstmp = dstBuff;
#endif

#if SEND_FILE
        fsize = fread(dstBuff, 1, TSIZE, f);
        printf("open [%s] size: %d \n", argv[3], fsize);
#else
        printf("allcate buff [0x%x] size: %d \n", (uint32_t)dstBuff, TSIZE);
#endif

       modeSel = 1;
    mode &= ~SPI_MODE_3;
    switch(modeSel) {
        case 1:
                mode |= SPI_MODE_1;
            break;
        case 2:
                mode |= SPI_MODE_2;
            break;
        case 3:
                mode |= SPI_MODE_3;
            break;
        default:
                mode |= SPI_MODE_0;
            break;
    }

       arg0 = arg0 % 2;

        ret = ioctl(fm[arg0], SPI_IOC_WR_MODE, &mode);
        if (ret == -1) 
            pabort("can't set spi mode"); 
 
        ret = ioctl(fm[arg0], SPI_IOC_RD_MODE, &mode);
        if (ret == -1) 
            pabort("can't get spi mode"); 


        // disable data mode

        bitset = 1;
        ret = ioctl(fm[arg0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
        printf("Set spi%d slve ready: %d\n", arg0, bitset);

        bitset = 0;
        ret = ioctl(fm[arg0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        printf("Set spi%d data mode: %d\n", arg0, bitset);

        bitset = 0;
        ret = ioctl(fm[arg0], _IOR(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_RD_DATA_MODE
        printf("Get spi%d data mode: %d\n", arg0, bitset);

        bitset = 1;
        ret = ioctl(fm[arg0], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_RD_CTL_PIN
        printf("Get spi%d RDY: %d\n", arg0, bitset);

        bitset = 0;
        ret = ioctl(fm[arg0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_WR_CTL_PIN
        printf("Set spi%d RDY: %d\n", arg0, bitset);

#if SPI_THREAD_EN
        printf("Start spi%d spidev thread, ret: 0x%x\n", 1, ret);
        bitset = 0;
        ret = ioctl(fm[arg0], _IOR(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_START_THREAD
#endif

        src = dstBuff;
        acusz = 0; pktcnt = 0;
//        while (acusz < fsize) {
        while (1) {
/*
            if ((fsize - acusz) > SPI_TRUNK_SZ) {
                txsz = SPI_TRUNK_SZ;
            } else {
                txsz = fsize - acusz;
            }
*/
            txsz = SPI_TRUNK_SZ;
#if SPI_THREAD_EN
            send = ioctl(fm[arg0], _IOR(SPI_IOC_MAGIC, 15, __u32), src);  //SPI_IOC_PROBE_THREAD
#else
            send = tx_data(fm[arg0], src, src, 1, txsz, 1024*1024);
#endif

            if (send == 0) {
                continue;
            } else if (send < 0) {
                if (send == -1) {
                    len = 0;
                } else {
                    len = 0 - send;
                }
            } else {
                len = send;
            }

            acusz += len;

            printf("[%d] tx %d/%d - %d\n", pktcnt, send, len, acusz);
#if USE_SHARE_MEM
#else
#if SAVE_FILE
            if (len > 0) {
                msync(src, len, MS_SYNC);
                ret = fwrite(src, 1, len, fp);
                if (ret != len) {
                    printf("[%s]write file error, ret:%d len:%d\n", path, ret, len);
                } else {
                    //printf("[%s]write file done, ret:%d len:%d\n", path, ret, len);
                }
            }
#endif
#endif
            if (send < 0) {
                printf("spi%d data tx complete %d/%d \n", arg0, send, txsz);
                break;
            }
#if USE_SHARE_MEM
            src += len;
#endif
            pktcnt++;
        }
#if SPI_THREAD_EN
        bitset = 0;
        ret = ioctl(fm[arg0], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
        printf("Stop spi%d spidev thread, ret: %d\n", 0, ret);
#endif
        bitset = 1;
        ret = ioctl(fm[arg0], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_RD_CTL_PIN
        printf("Get spi%d RDY: %d\n", arg0, bitset);

#if USE_SHARE_MEM
        msync(dstBuff, acusz, MS_SYNC);
        ret = fwrite(dstBuff, 1, acusz, fp);
        if (ret != acusz) {
             printf("[%s]write file error, ret:%d len:%d\n", path, ret, acusz);
        } else {
            printf("[%s]file saved, size:%d\n", path, acusz);
        }
#else
        printf("[%s]file saved, size:%d\n", path, acusz);
#endif

        goto end;
    }
    
    if (sel == 18){ /* command mode test ex[18 1 ]*/
        int sid[2], pid, gid, size, pi, opsz;
        struct pipe_s pipe_t;
        char str[128], op, *stop_at;;

        char *shmtx, *dbgbuf[2], *rxbuf;
        char *tmptx, *pdbg ;
        
        shmtx  = mmap(NULL, 2, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
        dbgbuf[0] = mmap(NULL, 4*1024*1024, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
        dbgbuf[1] = mmap(NULL, 4*1024*1024, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
        if (!shmtx) goto end;
        if (!dbgbuf[0]) goto end;
        if (!dbgbuf[1]) goto end;

        //memset(shmtx, 0xaa, 2);
        shmtx[0] = 0xf0;         shmtx[1] = 0xf0;
        memset(dbgbuf[0], 0xf0, 4*1024*1024);
        memset(dbgbuf[1], 0xf0, 4*1024*1024);

        tmptx = shmtx;

        rxbuf = rx_buff[0];
  
        pipe(pipe_t.rt);

        // don't pull low RDY after every transmitting

        bits = 16;
        ret = ioctl(fm[0],  SPI_IOC_WR_BITS_PER_WORD, &bits);
        printf("Set spi0 data wide: %d\n", bits);

        ret = ioctl(fm[1],  SPI_IOC_WR_BITS_PER_WORD, &bits);
        printf("Set spi1 data wide: %d\n", bits);
        
    mode &= ~SPI_MODE_3;
    switch(arg0) {
        case 1:
                mode |= SPI_MODE_1;
            break;
        case 2:
                mode |= SPI_MODE_2;
            break;
        case 3:
                mode |= SPI_MODE_3;
            break;
        default:
                mode |= SPI_MODE_0;
            break;
    }
        ret = ioctl(fm[0], SPI_IOC_WR_MODE, &mode);
        if (ret == -1) 
            pabort("can't set spi mode"); 
 
        ret = ioctl(fm[0], SPI_IOC_RD_MODE, &mode);
        if (ret == -1) 
            pabort("can't get spi mode"); 

/*                        ret = read(pipefs[0], lastaddr, 32); */
        pid = getpid();
        sid[0] = -1; sid[1] = -1;
        sid[0] = fork();
        if (!sid[0]) { // process 1
             gid = pid + sid[0] + sid[1];
             printf("[1]%d\n", gid);
             close(pipe_t.rt[0]);
             rxbuf[0] = 'p';
             pdbg = dbgbuf[0];
             while (1) {
          //printf("!");
                msync(shmtx, 2, MS_SYNC);
          *pdbg = 0;
          pdbg +=1;

                memcpy(pdbg, shmtx, 2);
                pdbg += 2;

          *pdbg = 0;
          pdbg +=1;
                
                ret = tx_data(fm[0], &rxbuf[1], shmtx, 1, 2, 1024);
                //printf("[%d]%x, %x \n", gid, rxbuf[1], rxbuf[2]);
                write(pipe_t.rt[1], rxbuf, 3);
                if (rxbuf[2] == 0xaa) {
                    size = pdbg - dbgbuf[0];
                    sprintf(str, "s0s%.8d", size);
                    write(pipe_t.rt[1], str, 11);
              pdbg = dbgbuf[0];
                }
                if (rxbuf[2] == 0xee) {
                    write(pipe_t.rt[1], "e", 1);
                    break;
                }
                //usleep(1);
             }
             close(pipe_t.rt[1]);
        } else {
            sid[1] = fork();
            if (!sid[1]) { // process 2
                 gid = pid + sid[0] + sid[1];
                 printf("[2]%d\n", gid);
                 close(pipe_t.rt[0]);
                 rxbuf[0] = 'p';
                 pdbg = dbgbuf[1];
                 while (1) {
            //printf("?");
                    msync(shmtx, 2, MS_SYNC);
              *pdbg = 0;
              pdbg +=1;

                    memcpy(pdbg, shmtx, 2);
                 pdbg += 2;

              *pdbg = 0;
              pdbg +=1;
                
                     ret = tx_data(fm[0], &rxbuf[1], shmtx, 1, 2, 1024);
                     //printf("[%d]%x, %x \n", gid, rxbuf[1], rxbuf[2]);
                     write(pipe_t.rt[1], rxbuf, 3);
                     if (rxbuf[2] == 0xaa) {
                         size = pdbg - dbgbuf[1];
                         sprintf(str, "s1s%.8d", size);
                         write(pipe_t.rt[1], str, 11);
                pdbg = dbgbuf[1];
                     }
                     if (rxbuf[2] == 0xee) {
                         write(pipe_t.rt[1], "e", 1);
                         break;
                     }
                  //usleep(1);
                 }
                 close(pipe_t.rt[1]);
            } else { // process 0
                gid = pid + sid[0] + sid[1];
                 printf("[0]%d\n", gid);
                close(pipe_t.rt[1]);
                op = 0;
                while (1) {
#if 0
                    ret = read(pipe_t.rt[0], rxbuf, 3);
                    printf("[%d]size:%d, %x %x %x \n", gid, ret, rxbuf[0], rxbuf[1], rxbuf[2] );
#else
                    ret = read(pipe_t.rt[0], &op, 1);
                    if (ret > 0) {
                        if (op == 'p') {
                               ret = read(pipe_t.rt[0], rxbuf, 2);
                         printf("[%d] 0x%.2x 0x%.2x \n", gid, rxbuf[0], rxbuf[1]);
                            shmtx[0] = rxbuf[0];
                          shmtx[1] = rxbuf[1];
                        }
                        else if (op == 's') {
                               ret = read(pipe_t.rt[0], rxbuf, 10);
                               rxbuf[10] = '\0';
                         printf("[%d] %c \"%s\" \n", gid, op, rxbuf);
                               if (rxbuf[0] == '0') pi = 0;
                               else if (rxbuf[0] == '1') pi = 1;                               
                               else goto end;
                               size = strtoul(&rxbuf[2], &stop_at, 10);
                               opsz = 16 - (size % 16);
                               ret = fwrite(dbgbuf[pi], 1, size, fp);
                   printf("[%d]file %d write size: %d\n", gid, (uint32_t)fp, ret);
                               ret = fwrite("================", 1, opsz, fp);
                   printf("[%d]file %d write size: %d\n", gid, (uint32_t)fp, ret);
                        }
                        else if (op == 'e') {
                             printf("[%d] %c \n", gid, op);
                            break;
                        } else {
                            printf("[?] op:%x \n", op);
                        }
                    }
#endif
                }
                close(pipe_t.rt[0]);
                kill(sid[0], SIGKILL);
                kill(sid[1], SIGKILL);
            }
        }

        goto end;
    }
    if (sel == 17){ /* command mode test ex[17 6]*/
    int lsz=0, cnt=0;
    char *ch;
    
    mode |= SPI_MODE_1;
        ret = ioctl(fd0, SPI_IOC_WR_MODE, &mode);    //?家Α 
        if (ret == -1) 
            pabort("can't set spi mode"); 
 
        ret = ioctl(fd0, SPI_IOC_RD_MODE, &mode);    //?家Α 
        if (ret == -1) 
            pabort("can't get spi mode"); 

    if (arg0 > 0)
        lsz = arg0;

    while (lsz) {
        ch = tx_buff[lsz%2];
        *ch = 0x30 + cnt;
              ret = tx_data(fd0, rx_buff[0], tx_buff[lsz%2], 1, 512, 1024*1024);
        if (ret != 512)
            break;
        lsz --;
        cnt++;
    }
        
        goto end;
    }
        
    if (sel == 16){ /* tx hold test ex[16 0 0]*/
    arg1 = arg1%2;
       bitset = arg0;
    ioctl(fm[arg1], _IOW(SPI_IOC_MAGIC, 13, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    printf("Set spi%d Tx Hold: %d\n", arg1, bitset);
        goto end;
    }

    if (sel == 15){ /* single band data mode test ex[15 0 3]*/
        int wtsz;

        arg0 = arg0 % 2;

        mode &= ~SPI_MODE_3;
     mode |= SPI_MODE_1;

        printf("select spi%d mode: 0x%x tx:0xf0\n", arg0, mode);

        /*
         * spi mode //?竚spi??家Α
         */ 
        ret = ioctl(fm[arg0], SPI_IOC_WR_MODE, &mode);    //?家Α 
        if (ret == -1) 
            pabort("can't set spi mode"); 
 
        ret = ioctl(fm[arg0], SPI_IOC_RD_MODE, &mode);    //?家Α 
        if (ret == -1) 
            pabort("can't get spi mode"); 

        if (!arg1) {
            arg1 = 1;
        }
        
        printf("sel 15 spi[%d] ready to receive data, size: %d num: %d \n", SPI_TRUNK_SZ, arg0, arg1);
        ret = tx_data(fm[arg0], rx_buff[0], tx_buff[0], arg1, SPI_TRUNK_SZ, 1024*1024);
        printf("[%d]rx %d\n", arg0, ret);
        wtsz = fwrite(rx_buff[0], 1, ret, fp);
        printf("write file %d size %d/%d \n", (uint32_t)fp, wtsz, ret);
        goto end;
    }
    if (sel == 14){ /* dual band data mode test ex[14 1 2 5 1 30720]*/
        #define PKTSZ  30720 //SPI_TRUNK_SZ
        int chunksize;
        if (arg4 == PKTSZ) chunksize = PKTSZ;
        else chunksize = SPI_TRUNK_SZ;

        printf("***chunk size: %d ***\n", chunksize);
        
        int pipefd[2];
        int pipefs[2];
        int pipefc[2];
        char *tbuff, *tmp, buf='c', *dstBuff, *dstmp;
        char lastaddr[48];
        int sz = 0, wtsz = 0, lsz = 0;
        char *addr;
        int pid = 0;
        int txhldsz = 0;
        //tbuff = malloc(TSIZE);
        tbuff = rx_buff[0];
        dstBuff = mmap(NULL, TSIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
     memset(dstBuff, 0x95, TSIZE);
        dstmp = dstBuff;

    switch(arg3) {
        case 1:
                mode |= SPI_MODE_1;
            break;
        case 2:
                mode |= SPI_MODE_2;
            break;
        case 3:
                mode |= SPI_MODE_3;
            break;
        default:
                mode |= SPI_MODE_0;
            break;
    }
        /*
         * spi mode 
         */ 
        ret = ioctl(fm[0], SPI_IOC_WR_MODE, &mode);    //?家Α 
        if (ret == -1) 
            pabort("can't set spi mode"); 
        
        ret = ioctl(fm[0], SPI_IOC_RD_MODE, &mode);    //?家Α 
        if (ret == -1) 
            pabort("can't get spi mode"); 

        /*
         * spi mode 
         */ 
        ret = ioctl(fm[1], SPI_IOC_WR_MODE, &mode);    //?家Α 
        if (ret == -1) 
            pabort("can't set spi mode"); 
        
        ret = ioctl(fm[1], SPI_IOC_RD_MODE, &mode);    //?家Α 
        if (ret == -1) 
            pabort("can't get spi mode"); 

        bitset = 1;
        ret = ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
        printf("Set slve ready: %d\n", bitset);

        bitset = 1;
        ret = ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
        printf("Set slve ready: %d\n", bitset);

        bitset = 0;
        ret = ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_WT_CTL_PIN
        printf("Set spi0 RDY: %d\n", bitset);

        bitset = 0;
        ret = ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_WT_CTL_PIN
        printf("Set spi1 RDY: %d\n", bitset);

        bitset = 0;
        ret = ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        printf("Set spi0 data mode: %d\n", bitset);

        bitset = 0;
        ret = ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        printf("Set spi1 data mode: %d\n", bitset);
    
        struct timespec *tspi = (struct timespec *)mmap(NULL, sizeof(struct timespec) * 2, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);;
        if (!tspi) {
            printf("get share memory for timespec failed - %d\n", (uint32_t)tspi);
            goto end;
        }
        clock_gettime(CLOCK_REALTIME, &tspi[0]);  
        clock_gettime(CLOCK_REALTIME, &tspi[1]);  
        int tcost, tlast;
        struct tms time;
        struct timespec curtime, tdiff[2];

        if (tbuff)
            printf("%d bytes memory alloc succeed! [0x%.8x]\n", TSIZE, (uint32_t)tbuff);
        else 
            goto end;

        tmp = tbuff;
        
        sz = 0;
        pipe(pipefd);
        pipe(pipefs);
        pipe(pipefc);
        
        pid = fork();
        
        if (pid) {
            pid = fork();
            if (pid) {
                //usleep(1000000);
        uint8_t *cbuff;
        cbuff = tx_buff[1];
                printf("main process to monitor the p0 and p1 \n");
                close(pipefd[0]); // close the read-end of the pipe, I'm not going to use it
                close(pipefs[1]); // close the write-end of the pipe, I'm not going to use it
                close(pipefd[1]); // close the write-end of the pipe, thus sending EOF to the reader
                close(pipefs[0]); // close the read-end of the pipe
                close(pipefc[1]); // close the write-end of the pipe, thus sending EOF to the reader
                
                txhldsz = arg1;
                
                while (1) {
                    //sleep(3);                 
                    ret = read(pipefc[0], &buf, 1); 
                    //printf("********//read %d, buf:%c//********\n", ret, buf);
                    if ((buf == '0') || (buf == '1')) {
                cbuff[wtsz] = buf;
                        wtsz++;

            if (arg0) {                     
                        if (wtsz == txhldsz) {
                            //printf("********//spi slave send signal to hold the data transmitting(%d/%d)//********\n", wtsz, txhldsz);

                            bitset = 1;
                            ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 13, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
                            //printf("Set spi0 Tx Hold: %d\n", bitset);

                            bitset = 1;
                            ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 13, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
                            //printf("Set spi1 Tx Hold: %d\n", bitset);

                if (arg2>0)
                    lsz = arg2;
                else
                    lsz = 10;

                while(lsz) {
                              bitset = 0;
                             ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
                                //printf("Set RDY: %d, dly:%d \n", bitset, arg2);
                    usleep(1);
                            
                               bitset = 1;
                              ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
                             //printf("Set RDY: %d\n", bitset);
                    usleep(1);
                    
                    lsz --;
                }

                     //         bitset = 0;
                     //        ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
                 //               printf("Set RDY: %d\n", bitset);
                     //         sleep(5);

                            bitset = 0;
                            ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 13, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
                            //printf("Set spi0 Tx Hold: %d\n", bitset);

                            bitset = 0;
                            ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 13, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
                            //printf("Set spi1 Tx Hold: %d\n", bitset);

                        }   
               }
                    } else if (buf == 'e') {
                        ret = read(pipefc[0], lastaddr, 32); 
                        sz = atoi(lastaddr);
                        printf("********//main monitor, write count:%d, sz:%d str:%s//********\n", wtsz, sz, lastaddr);
                for (sz = 0; sz < wtsz; sz++) {
                    printf("%c, ", cbuff[sz]);
                    if (!((sz+1) % 16)) printf("\n");
                }
                        break;
                    } else {
                        printf("********//main monitor, get buff:%c, ERROR!!!!//********\n", buf);
                    }
                    
                }
                close(pipefc[0]); // close the read-end of the pipe

            } else {
                printf("p0 process pid:%d!\n", pid);
                char str[256] = "/root/p0.log";
                
                close(pipefd[0]); // close the read-end of the pipe, I'm not going to use it
                close(pipefs[1]); // close the write-end of the pipe, I'm not going to use it
                close(pipefc[0]); // close the read-end of the pipe
                
                while(1) {
        
                    clock_gettime(CLOCK_REALTIME, &tspi[0]);   
                    ret = tx_data(fm[0], dstBuff, tx_buff[0], 1, chunksize, 1024*1024);
                    if (ret < 0) {
                        ret = 0 - ret;
                    }
                    clock_gettime(CLOCK_REALTIME, &tdiff[0]);    
                     msync(tspi, sizeof(struct timespec)*2, MS_SYNC);
                     tlast = test_time_diff(&tspi[1], &tspi[0], 1000);
                     if (tlast == -1) {
                         tlast = 0 - test_time_diff(&tspi[0], &tspi[1], 1000);
                     }
                     tcost = test_time_diff(&tspi[0], &tdiff[0], 1000);
                    //ret = tx_data(fm[0], dstBuff, NULL, 1, chunksize, 1024*1024);
            //if (arg0) 
                            printf("[p0]rx %d - %d (%d us /%d us)\n", ret, wtsz++, tcost, tlast);
                    msync(dstBuff, ret, MS_SYNC);
/*
if (((dstBuff - dstmp) < 0x28B9005) && ((dstBuff - dstmp) > 0x28B8005)) {
    char *ch;
    ch = dstBuff;;
    printf("0x%.8x: \n",(uint32_t)(ch - dstmp));
    for (sel = 0; sel < 1024; sel++) {
        printf("%.2x ", *ch);
        if (!((sel + 1) % 16)) printf("\n");
        ch++;
    }
}
*/
            dstBuff += ret + chunksize;
                    if (ret != chunksize) {
                        dstBuff -= chunksize;
                        if (ret == 1) ret = 0;
                        lsz = ret;
                        write(pipefd[1], "e", 1); // send the content of argv[1] to the reader
                        sprintf(lastaddr, "%d", (uint32_t)dstBuff);
                        printf("p0 write e  addr:%x str:%s ret:%d\n", (uint32_t)dstBuff, lastaddr, ret);
                        write(pipefd[1], lastaddr, 32); 
                        break;
                    }
                    write(pipefd[1], "c", 1);
                    
                    /* send info to control process */
                    write(pipefc[1], "0", 1);
                    //printf("main process write c \n");    
                }
                
                wtsz = 0;
                while (1) {
                    ret = read(pipefs[0], &buf, 1); 
                    //printf("p0 read %d, buf:%c \n", ret, buf);
                    if (buf == 'c')
                        wtsz++; 
                    else if (buf == 'e') {
                        ret = read(pipefs[0], lastaddr, 32); 
                        addr = (char *)atoi(lastaddr);
                        printf("p0 process wait wtsz:%d, lastaddr:%x/%x\n", wtsz, (uint32_t)addr, (uint32_t)dstBuff);
                        break;
                    }
                }
                
                if (dstBuff > addr) {
                    printf("p0 memcpy from  %x to %x sz:%d\n", (uint32_t)dstBuff-lsz, (uint32_t)addr, lsz);
                    msync(dstBuff-lsz, lsz, MS_SYNC);
                    #if 0
                    memcpy(addr, dstBuff-lsz, lsz);
                    #else
                    memcpy(tbuff, dstBuff-lsz, lsz);
                    memcpy(addr, tbuff, lsz);
                    #endif
                    addr += lsz;
                    memset(addr, 0, chunksize);
                    sz = addr - dstmp;
                    
                    /* send info to control process */
                    write(pipefc[1], "e", 1);
                    sprintf(lastaddr, "%d", sz);
                    printf("p0 write e, sz:%x str:%s \n", sz, lastaddr);
                    write(pipefc[1], lastaddr, 32);

                    msync(dstmp, sz, MS_SYNC);
                    ret = fwrite(dstmp, 1, sz, fp);
                    printf("\np0 write file [%s] size %d/%d \n", path, sz, ret);
                }
                
                close(pipefd[1]); // close the write-end of the pipe, thus sending EOF to the reader
                close(pipefs[0]); // close the read-end of the pipe
                close(pipefc[0]); // close the read-end of the pipe
            }
        } else {
            sleep(2);
            char str[256] = "/root/p1.log";
            printf("p1 process pid:%d!\n", pid);

            close(pipefd[1]); // close the write-end of the pipe, I'm not going to use it
            close(pipefs[0]); // close the read-end of the pipe, I'm not going to use it
            close(pipefc[0]); // close the read-end of the pipe

            dstBuff += chunksize;
            while(1) {
                clock_gettime(CLOCK_REALTIME, &tspi[1]);            
                ret = tx_data(fm[1], dstBuff, tx_buff[0], 1, chunksize, 1024*1024);
                if (ret < 0) {
                    ret = 0 - ret;
                }
                clock_gettime(CLOCK_REALTIME, &tdiff[1]);   
                msync(tspi, sizeof(struct timespec) * 2, MS_SYNC);
                tlast = test_time_diff(&tspi[0], &tspi[1], 1000);
                if (tlast == -1) {
                     tlast = 0 - test_time_diff(&tspi[1], &tspi[0], 1000);
                }
                tcost = test_time_diff(&tspi[1], &tdiff[1], 1000);
                //ret = tx_data(fm[1], dstBuff, NULL, 1, chunksize, 1024*1024);
        //if (arg0)     
                printf("[p1]rx %d - %d (%d us /%d us)\n", ret, wtsz++, tcost, tlast);
                msync(dstBuff, ret, MS_SYNC);
/*
if (((dstBuff - dstmp) < 0x28B9005) && ((dstBuff - dstmp) > 0x28B8005)) {
    char *ch;
    ch = dstBuff;;
    printf("0x%.8x: \n", (uint32_t)(ch - dstmp));
    for (sel = 0; sel < 1024; sel++) {
        printf("%.2x ", *ch);
        if (!((sel + 1) % 16)) printf("\n");
        ch++;
    }
}
*/
                dstBuff += ret + chunksize;
                if (ret != chunksize) {
                    dstBuff -= chunksize;
                    if (ret == 1) ret = 0;
                    lsz = ret;
                    write(pipefs[1], "e", 1); // send the content of argv[1] to the reader
                    sprintf(lastaddr, "%d", (uint32_t)dstBuff);
                    printf("p1 write e addr:%x str:%s ret:%d\n", (uint32_t)dstBuff, lastaddr, ret);
                    write(pipefs[1], lastaddr, 32); // send the content of argv[1] to the reader
                    break;
                }
                write(pipefs[1], "c", 1); // send the content of argv[1] to the reader         
                
                /* send info to control process */
                write(pipefc[1], "1", 1);
                //printf("p1 process write c \n");
            }

            wtsz = 0;
            while (1) {
                ret = read(pipefd[0], &buf, 1); 
                //printf("p1 read %d, buf:%c  \n", ret, buf);
                if (buf == 'c')
                    wtsz++; 
                else if (buf == 'e') {
                    ret = read(pipefd[0], lastaddr, 32); 
                    addr = (char *)atoi(lastaddr);
                    printf("p1 process wtsz:%d, lastaddr:%x/%x\n",  wtsz, (uint32_t)addr, (uint32_t)dstBuff);
                    break;
                }
            }

            if (dstBuff > addr) {
                printf("p1 memcpy from  %x to %x sz:%d\n", (uint32_t)dstBuff-lsz, (uint32_t)addr, lsz);
                msync(dstBuff-lsz, lsz, MS_SYNC);
                #if 0
                memcpy(addr, dstBuff-lsz, lsz);
                #else
                memcpy(tbuff, dstBuff-lsz, lsz);
                memcpy(addr, tbuff, lsz);
                #endif
                addr += lsz;
                memset(addr, 0, chunksize);
                sz = addr - dstmp;
                
                /* send info to control process */
                write(pipefc[1], "e", 1);
                sprintf(lastaddr, "%d", sz);
                printf("p1 write e sz:%x str:%s \n", sz, lastaddr);
                write(pipefc[1], lastaddr, 32); 

                msync(dstmp, sz, MS_SYNC);
                ret = fwrite(dstmp, 1, sz, fp);
                printf("\np1 write file [%s] size %d/%d \n", path, sz, ret);
            }

            close(pipefd[0]); // close the read-end of the pipe
            close(pipefs[1]); // close the write-end    of the pipe, thus sending EOF to the
            close(pipefc[0]); // close the read-end of the pipe
        }

        munmap(dstmp, TSIZE);
        goto end;
    }

    if (sel == 13){ /* data mode test ex[13 1024 100 0]*/
    arg2 = arg2 % 2;
        data_process(rx_buff[0], tx_buff[0], fp, fm[arg2], arg0, arg1);
        goto end;
    }               

    if (sel == 12){ /* data mode test */
        int pid;
        pid = fork();
        if (pid) {
            printf("p1 %d created!\n", pid);
            pid = fork();
            if (pid)
                printf("p2 %d created!\n", pid);
            else
                while(1);
        } else {
            while(1);
        }

        goto end;
    }       
    if (sel == 11){ /* cmd mode test */
        tx[0] = 0xaa;
        tx[1] = 0x55;
        ret = tx_command(fd0, rx, tx, 512);
        printf("reply 0x%x 0x%x \n", tx[0], tx[1]);
        ret = chk_reply(rx, rxans, 512);
        printf("check receive data ret: %d\n", ret);
        while (1) {
                bitset = 1;
                ret = ioctl(fd1, _IOR(SPI_IOC_MAGIC, 7, __u32), &bitset);   //SPI_IOC_RD_CS_PIN
             printf("Get fd1 CS: %d\n", bitset);
             if (bitset == 0) break;
             sleep(3);
        }
        goto end;
    }
    if (sel == 2) { /*set RDY pin*/
        bitset = arg0;
        ret = ioctl(fm[arg1], _IOW(SPI_IOC_MAGIC, 7, __u32), &bitset);  //SPI_IOC_WR_CS_PIN
        printf("Set CS: %d\n", bitset);
        goto end;
    }

    if (sel == 3) { /*set RDY pin*/
        bitset = arg0;
        ret = ioctl(fm[arg1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        printf("Set RDY: %d\n", arg0);
        goto end;
    }

    if (sel == 4) { /* get RDY pin */
        bitset = 0;
        ret = ioctl(fm[arg1], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_RD_CTL_PIN
        printf("Get RDY: %d\n", bitset);
        goto end;
    }

    if (sel == 5) { /* get CS pin */
        bitset = 0;
        ret = ioctl(fm[arg1], _IOR(SPI_IOC_MAGIC, 7, __u32), &bitset);  //SPI_IOC_RD_CS_PIN
        printf("Get CS: %d\n", bitset);
        goto end;
    }

    if (sel == 6){/* set data mode test */
        bitset = arg0;
        ret = ioctl(fm[arg1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        printf("Set data mode: %d\n", arg0);
        goto end;
    }

    if (sel == 7) {/* get data mode test */
        bitset = 0;
        ret = ioctl(fm[arg1], _IOR(SPI_IOC_MAGIC, 8, __u32), &bitset);  //SPI_IOC_RD_DATA_MODE
        printf("Get data mode: %d\n", bitset);
        goto end;
    }
    if (sel == 8){ /* set slve ready */
        bitset = arg0;
        ret = ioctl(fm[arg1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
        printf("Set slve ready: %d\n", arg0);
        goto end;
    }
    if (sel == 9){ /* get slve ready */
        bitset = 0;
        ret = ioctl(fm[arg1], _IOR(SPI_IOC_MAGIC, 11, __u32), &bitset); //SPI_IOC_RD_SLVE_READY
        printf("Get slve ready: %d\n", bitset);
        goto end;
    }
    if (sel == 10){
        fp = find_save(path, data_save);
        if (!fp)
            printf("find save dst failed ret:%d\n", (uint32_t)fp);
        else
            printf("find save dst succeed ret:%d\n", (uint32_t)fp);
        goto end;
    }
    if (sel == 12){
        return -9;
    }
    
    ret = 0;
    
    while (1) {
        if (!(ret % 100000)) {
            
            if (sel == 1) {
                printf(" Tx bitset %d \n", bitset);
                ioctl(fd1, _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WD_CTL_PIN
                if (bitset == 1)
                    bitset = 0;
                else
                    bitset = 1;            
            }
            else {
                printf(" Rx bitset %d \n", bitset);
                ret = ioctl(fd1, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN            
            }
        }
          
        ret++;
    }
end:

    free(tx_buff[0]);
    free(rx_buff[0]);
    free(tx_buff[1]);
    free(rx_buff[1]);

    fclose(fp);
}

