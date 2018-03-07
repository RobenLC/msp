#include <stdint.h> 
#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <getopt.h> 
#include <fcntl.h> 
#include <sys/ioctl.h> 
#include <sys/mman.h> 
#include <linux/types.h> 
#include <linux/spi/spidev.h> 
#include <sys/times.h> 
#include <time.h>
#include <sys/socket.h>

#include <dirent.h>
#include <sys/stat.h>  

#include <linux/poll.h>

#define SPI1_ENABLE (1)

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0])) 
 
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

typedef enum {
    ASPFS_TYPE_ROOT = 0x1,
    ASPFS_TYPE_DIR,
    ASPFS_TYPE_FILE,
} aspFS_Type_e;

struct bitmapHeader_s {
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

static int aspSD_getRoot();
static int aspSD_getDir();

static int mspbitmapHeaderSetup(struct bitmapHeader_s *ph, int clr, int w, int h, int dpi, int flen) 
{
    int rawoffset=0, totsize=0, numclrp=0, calcuraw=0, rawsize=0;
    float resH=0, resV=0, ratio=39.27, fval=0;

    if (!w) return -1;
    if (!h) return -2;
    if (!dpi) return -3;
    if (!flen) return -4;
    memset(ph, 0, sizeof(struct bitmapHeader_s));

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
    printf("memdump[0x%.8x] sz%d: \n", src, size);
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
            if (sum != (fs->dfstats >> 16) & 0xff) {
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
                    printf("%*s\"%s\"\n", depth, "", fs->dfLFN, depth);
                    printf("%*s\"%s\"\n", depth, "", fs->dfSFN, depth);
                } else {
                    printf("%*s\"%s\"\n", depth, "", fs->dfSFN, depth);
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
                    printf("%*s%s\n", depth, "", fs->dfLFN, depth);
                    printf("%*s%s\n", depth, "", fs->dfSFN, depth);
                } else {
                    printf("%*s%s\n", depth, "", fs->dfSFN, depth);
                }
            }

        } else if (fs->dfattrib & ASPFS_ATTR_ARCHIVE) {
            printf("**archive**\n");
            if (fs->dflen) {
                printf("%*s%s\n", depth, "", fs->dfLFN, depth);
                printf("%*s%s\n", depth, "", fs->dfSFN, depth);
            } else {
                printf("%*s%s\n", depth, "", fs->dfSFN, depth);
            }
        } else if (fs->dfattrib & ASPFS_ATTR_HIDDEN) {
            printf("**hidden**\n");
            if (fs->dflen) {
                printf("%*s%s\n", depth, "", fs->dfLFN, depth);
                printf("%*s%s\n", depth, "", fs->dfSFN, depth);
            } else {
                printf("%*s%s\n", depth, "", fs->dfSFN, depth);
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
            printf("%*s%s\n", depth, "", entry->d_name, depth);
            printdir(entry->d_name, depth+4);
        } else
            printf("%*s%s\n", depth, "", entry->d_name, depth);
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
            printf("[R]alloc root fs done [0x%x]\n", r);
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
        printf("[R]root error 0x%x\n", root);
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
            printf("%*s%s\n", TAB_DEPTH, "", entry->d_name, TAB_DEPTH);
            //printdir(entry->d_name, TAB_DEPTH+4);
            ret = aspFS_insertChildDir(root, entry->d_name);
            if (ret) goto insertEnd;
        } else {
            printf("%*s%s\n", TAB_DEPTH, "", entry->d_name, TAB_DEPTH);
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
        printf("[R]root error 0x%x\n", root);
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

    printf("path len: %d, match num: %d, brt:0x%x \n", a, b, brt);

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
        }
    }
    f = fopen(dst, "w");
    return f;
}

FILE *find_open(char *dst, char *tmple)
{
    FILE *f;
    sprintf(dst, tmple);
    f = fopen(dst, "w");
    return f;
}

static int data_process(char *rx, char *tx, FILE *fp, int fd, int pktsz, int num)
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
        printf("find save dst failed ret:%d\n", fp);
        goto end;
    } else
        printf("find save dst succeed[%s] ret:%d\n", path, fp);

    /* scanner default setting */
    mode &= ~SPI_MODE_3;
    printf("mode initial: 0x%x\n", mode & SPI_MODE_3);
    mode |= SPI_MODE_1;

    fd = open(device, O_RDWR);  //ゴ???ゅン 
    if (fd < 0) 
        pabort("can't open device"); 

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
        
    if (spi1) {
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
    if (ret == -1) 
        pabort("can't set spi mode"); 

    ret = spi_config(fm[0], SPI_IOC_RD_MODE, &mode);
    if (ret == -1) 
        pabort("can't get spi mode"); 
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

#define PT_BUF_SIZE (32768)
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
        
        ptfd[0].fd = open(ptdevpath, O_WRONLY);
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
#define USB_SAVE_RESULT 1
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
        struct bitmapHeader_s bmpheader, *pbh;

        pbh = &bmpheader;
        
        memset(pbh, 0, sizeof(struct bitmapHeader_s));
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
                                    
                                    memcpy(&pbh->aspbmpMagic[2], ptbuf, sizeof(struct bitmapHeader_s) - 2);

                                    mspbitmapHeaderSetup(pbh, 24, pbh->aspbiWidth, tlen, -1, acusz+recvsz);

                                    memcpy(ptbuf, &pbh->aspbmpMagic[2], sizeof(struct bitmapHeader_s) - 2);
                                    
                                    printf("%s scan length: %d filelen\n", fileStr, tlen, filelen);
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
        static char ptfileSend[] = "/mnt/mmc2/usb/send001.jpg";
        char ptfilepath[128];
        char *ptrecv, *ptsend, *palloc=0;
        int ptret=0, recvsz=0, acusz=0, wrtsz=0, maxsz=0, sendsz=0, lastsz=0;
        int cntTx=0, usCost=0, bufsize=0;
        double throughput=0.0;
        FILE *fsave=0, *fsend=0;
        struct timespec tstart, tend;
        
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

redo: 

    sz = 0;
    wtsz = 0;
    lsz = 0;
    pid = 0;
    txhldsz = 0;
    *tmp, buf='c';
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
            printf("get share memory for timespec failed - %d\n", tspi);
            goto end;
        }
        clock_gettime(CLOCK_REALTIME, &tspi[0]);  
        clock_gettime(CLOCK_REALTIME, &tspi[1]);  
        int tcost, tlast;
        struct tms time;
        struct timespec curtime, tdiff[2];
        
        if (tbuff)
            printf("%d bytes memory alloc succeed! [0x%.8x]\n", TSIZE, tbuff);
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
                        sprintf(lastaddr, "%d", dstBuff);
                        printf("p0 write e  addr:%x str:%s ret:%d\n", dstBuff, lastaddr, ret);
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
                        printf("p0 process wait wtsz:%d, lastaddr:%x/%x\n", wtsz, addr, dstBuff);
                        break;
                    }
                }
                
                if (dstBuff > addr) {
                    printf("p0 memcpy from  %x to %x sz:%d\n", dstBuff-lsz, addr, lsz);
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
                    sprintf(lastaddr, "%d", dstBuff);
                    printf("p1 write e addr:%x str:%s ret:%d\n", dstBuff, lastaddr, ret);
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
                    printf("p1 process wtsz:%d, lastaddr:%x/%x\n",  wtsz, addr, dstBuff);
                    break;
                }
            }
        
            if (dstBuff > addr) {
                printf("p1 memcpy from  %x to %x sz:%d\n", dstBuff-lsz, addr, lsz);
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
            printf("[0x%x]root dir offset: %d per:%d\n", dkbuf, arg2, arg3);
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
        printf("find save dst failed ret:%d\n", fp_02);
        goto end;
    } else {
        printf("find save dst succeed ret:%d\n", fp_02);
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
        printf("allcate buff [0x%x] size: %d \n", dstBuff, TSIZE);
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
                   printf("[%d]file %d write size: %d\n", gid, fp, ret);
                               ret = fwrite("================", 1, opsz, fp);
                   printf("[%d]file %d write size: %d\n", gid, fp, ret);
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
                kill(sid[0]);
                kill(sid[1]);
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
        printf("write file %d size %d/%d \n", fp, wtsz, ret);
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
            printf("get share memory for timespec failed - %d\n", tspi);
            goto end;
        }
        clock_gettime(CLOCK_REALTIME, &tspi[0]);  
        clock_gettime(CLOCK_REALTIME, &tspi[1]);  
        int tcost, tlast;
        struct tms time;
        struct timespec curtime, tdiff[2];

        if (tbuff)
            printf("%d bytes memory alloc succeed! [0x%.8x]\n", TSIZE, tbuff);
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
                        sprintf(lastaddr, "%d", dstBuff);
                        printf("p0 write e  addr:%x str:%s ret:%d\n", dstBuff, lastaddr, ret);
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
                        printf("p0 process wait wtsz:%d, lastaddr:%x/%x\n", wtsz, addr, dstBuff);
                        break;
                    }
                }
                
                if (dstBuff > addr) {
                    printf("p0 memcpy from  %x to %x sz:%d\n", dstBuff-lsz, addr, lsz);
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
                    sprintf(lastaddr, "%d", dstBuff);
                    printf("p1 write e addr:%x str:%s ret:%d\n", dstBuff, lastaddr, ret);
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
                    printf("p1 process wtsz:%d, lastaddr:%x/%x\n",  wtsz, addr, dstBuff);
                    break;
                }
            }

            if (dstBuff > addr) {
                printf("p1 memcpy from  %x to %x sz:%d\n", dstBuff-lsz, addr, lsz);
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
            printf("find save dst failed ret:%d\n", fp);
        else
            printf("find save dst succeed ret:%d\n", fp);
        goto end;
    }
    if (sel == 12){
        return;
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

