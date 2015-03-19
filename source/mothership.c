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
#define SPI_CPHA  0x01          /* clock phase */
#define SPI_CPOL  0x02          /* clock polarity */
#define SPI_MODE_0		(0|0)
#define SPI_MODE_1		(0|SPI_CPHA)
#define SPI_MODE_2		(SPI_CPOL|0)
#define SPI_MODE_3		(SPI_CPOL|SPI_CPHA)

#define OP_PON   0x1
#define OP_QRY   0x2
#define OP_RDY   0x3
#define OP_DAT   0x4
#define OP_SCM   0x5
#define OP_DCM   0x6
#define OP_FIH    0x7
#define OP_DUL    0x8
#define OP_RD      0x9
#define OP_WT     0xa
#define OP_SDAT   0xb

#define OP_STSEC_00  0x10
#define OP_STSEC_01  0x11
#define OP_STSEC_02  0x12
#define OP_STSEC_03  0x13
#define OP_STLEN_00  0x14
#define OP_STLEN_01  0x15
#define OP_STLEN_02  0x16
#define OP_STLEN_03  0x17

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

#define OP_MSG               0x30
#define OP_ERROR           0xe0

#define SPI_MAX_TXSZ  (1024 * 1024)
#define SPI_TRUNK_SZ   (32768)

#define SPI_KTHREAD_USE    (0) /* can't work, has bug */

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
    BULLET,
    LASER,
    DOUBLEC,
    DOUBLED,
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
    ASPOP_STA_WR,
    ASPOP_STA_UPD,
} aspOpSt_e;

typedef enum {
    ASPOP_CODE_NONE = 0,
    ASPOP_FILE_FORMAT,
    ASPOP_COLOR_MODE,
    ASPOP_COMPRES_RATE,
    ASPOP_SCAN_MODE,
    ASPOP_DATA_PATH,
    ASPOP_RESOLUTION,
    ASPOP_SCAN_GRAVITY,
    ASPOP_MAX_WIDTH,
    ASPOP_WIDTH_ADJ_H,
    ASPOP_WIDTH_ADJ_L,
    ASPOP_SCAN_LENS_H,
    ASPOP_SCAN_LENS_L,
    ASPOP_CODE_MAX, /* 13 */
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
} aspOpMask_e;

typedef enum {
    ASPOP_TYPE_NONE = 0,
    ASPOP_TYPE_SINGLE,
    ASPOP_TYPE_MULTI,
    ASPOP_TYPE_VALUE,
} aspOpType_e;

struct aspConfig_s{
    uint32_t opStatus;
    uint32_t opCode;
    uint32_t opValue;
    uint32_t opMask;
    uint32_t opType;
    uint32_t opBitlen;
};

struct aspDirnFile_s{
    uint32_t   dftype;
    uint32_t   dfstats;
    char        dfLFN[256];
    int           dflen;
    uint32_t   dfattrib;
    struct aspDirnFile_s *pa;
    struct aspDirnFile_s *br;
    struct aspDirnFile_s *ch;	
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

struct pipe_s{
    int rt[2];
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
    uint32_t   infocnt;
};

struct machineCtrl_s{
    uint32_t seqcnt;
    struct info16Bit_s cur;
    struct info16Bit_s get;
    struct modersp_s mch;
};

struct mainRes_s{
    int sid[8];
    int sfm[2];
    int smode;
    struct aspConfig_s configTable[ASPOP_CODE_MAX];
    struct aspDirnFile_s root_dirt;
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
};

typedef int (*fselec)(struct mainRes_s *mrs, struct modersp_s *modersp);

struct fselec_s{
    int  id;
    fselec pfunc;
};

struct procRes_s{
    // pipe
    int spifd;
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
    FILE *fdat_s;
    // time measurement
    struct timespec *tm[2];
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
//time measurement, start /stop
static int time_diff(struct timespec *s, struct timespec *e, int unit);
//file rw open, save to file for debug
static int file_save_get(FILE **fp, char *path1);
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

static int mspFS_createRoot(struct aspDirnFile_s **root, char *dir);
static int mspFS_insertChilds(struct aspDirnFile_s *root);
static int mspFS_insertChildDir(struct aspDirnFile_s *parent, char *dir);
static int mspFS_insertChildFile(struct aspDirnFile_s *parent, char *str);
static int mspFS_list(struct aspDirnFile_s *root, int depth);
static int mspFS_search(struct aspDirnFile_s **dir, struct aspDirnFile_s *root, char *path);
static int mspFS_showFolder(struct aspDirnFile_s *root);
static int mspFS_folderJump(struct aspDirnFile_s **dir, struct aspDirnFile_s *root, char *path);

static int atFindIdx(char *str, char ch);

static int mspFS_createRoot(struct aspDirnFile_s **root, char *dir)
{
    char mlog[256];
    DIR *dp;
    struct aspDirnFile_s *r = 0, *c = 0;

    sprintf(mlog, "open directory [%s]\n", dir);
    print_f(mlogPool, "FS", mlog);

    if ((dp = opendir(dir)) == NULL) {
        sprintf(mlog, "Can`t open directory [%s]\n", dir);
        print_f(mlogPool, "FS", mlog);
        return (-1);
    }
    sprintf(mlog, "open directory [%s] done\n", dir);
    print_f(mlogPool, "FS", mlog);

    r = (struct aspDirnFile_s *) malloc(sizeof(struct aspDirnFile_s));
    if (!r) {
        return (-2);
    }else {
            sprintf(mlog, "alloc root fs done [0x%x]\n", r);
            print_f(mlogPool, "FS", mlog);
    }

    c = (struct aspDirnFile_s *) malloc(sizeof(struct aspDirnFile_s));
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
    sprintf(mlog, "[%s] len: %d\n", dir, r->dflen);
    print_f(mlogPool, "FS", mlog);
    if (r->dflen > 255) r->dflen = 255;
    strncpy(r->dfLFN, dir, r->dflen);

    *root = r;

    return 0;
}

static int mspFS_insertChilds(struct aspDirnFile_s *root)
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

    sprintf(mlog, "open directory [%s]\n", root->dfLFN);
    print_f(mlogPool, "FS", mlog);

    if ((dp = opendir(root->dfLFN)) == NULL) {
        printf("Can`t open directory [%s]\n", root->dfLFN);
        ret = -2;
        goto insertEnd;
    }

    sprintf(mlog, "open directory [%s] done\n", root->dfLFN);
    print_f(mlogPool, "FS", mlog);
	
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
            ret = mspFS_insertChildDir(root, entry->d_name);
            if (ret) goto insertEnd;
        } else {
            //sprintf(mlog, "%*s%s\n", TAB_DEPTH, "", entry->d_name, TAB_DEPTH);
            //print_f(mlogPool, "FS", mlog);
            ret = mspFS_insertChildFile(root, entry->d_name);
            if (ret) goto insertEnd;
        }
    }
    chdir("..");	

insertEnd:
    closedir(dp);	

    return ret;
}
static int mspFS_insertChildDir(struct aspDirnFile_s *parent, char *dir)
{
    int ret;
    char mlog[256];
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    struct aspDirnFile_s *r = 0, *c = 0;
    struct aspDirnFile_s *brt = 0;

    r = (struct aspDirnFile_s *) malloc(sizeof(struct aspDirnFile_s));
    if (!r) return (-2);

    c = (struct aspDirnFile_s *) malloc(sizeof(struct aspDirnFile_s));
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

    ret = mspFS_insertChilds(r);
 
    return ret;
}

static int mspFS_insertChildFile(struct aspDirnFile_s *parent, char *str)
{
    char mlog[256];
    struct aspDirnFile_s *r = 0;
    struct aspDirnFile_s *brt = 0;

    r = (struct aspDirnFile_s *) malloc(sizeof(struct aspDirnFile_s));
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

static int mspFS_list(struct aspDirnFile_s *root, int depth)
{
    char mlog[256];
    struct aspDirnFile_s *fs = 0;
    if (!root) return (-1);

    fs = root->ch;
    while (fs) {
        sprintf(mlog, "%*s%s[%d]\n", depth, "", fs->dfLFN, fs->dftype);
        print_f(mlogPool, "FS", mlog);
        if (fs->dftype == ASPFS_TYPE_DIR) {
            mspFS_list(fs, depth + 4);
        }
        fs = fs->br;
    }

    return 0;
}

static int mspFS_search(struct aspDirnFile_s **dir, struct aspDirnFile_s *root, char *path)
{
    char mlog[256];
    int ret = 0;
    char split = '/';
    char *ch;
    char rmp[16][256];
    int a = 0, b = 0;
    struct aspDirnFile_s *brt;

    ret = strlen(path);
    sprintf(mlog, "path[%s] root[%s] len:%d\n", path, root->dfLFN, ret);
    print_f(mlogPool, "FS", mlog);

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
            sprintf(mlog, "%x ", *ch);
            print_f(mlogPool, "FS", mlog);
        }
        ch++;
        ret --;
    }

    sprintf(mlog, "\n a:%d, b:%d \n", a, b);
    print_f(mlogPool, "FS", mlog);

    for (b = 0; b <= a; b++) {
        sprintf(mlog, "[%d.%d]%s \n", a, b, rmp[b]);
        print_f(mlogPool, "FS", mlog);
    }

    ret = -1;
    b = 0;
    brt = root->ch;
    while (brt) {
        sprintf(mlog, "comp[%s] [%s] \n", brt->dfLFN, &rmp[b][0]);
        print_f(mlogPool, "FS", mlog);
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

    sprintf(mlog, "path len: %d, match num: %d, brt:0x%x \n", a, b, brt);
    print_f(mlogPool, "FS", mlog);

    while((brt) && (b>=0)) {
        sprintf(mlog, "[%d][%s][%s] \n", b, &rmp[b][0], brt->dfLFN);
        print_f(mlogPool, "FS", mlog);
        b--;
        brt = brt->pa;
    }

    return ret;
}

static int mspFS_showFolder(struct aspDirnFile_s *root)
{
    char mlog[256];
    struct aspDirnFile_s *brt = 0;
    if (!root) return (-1);
    if (root->dftype == ASPFS_TYPE_FILE) return (-2);
	
    sprintf(mlog, "%s \n", root->dfLFN);
    print_f(mlogPool, "FS", mlog);

    brt = root->ch;
    while (brt) {
        sprintf(mlog, "|-[%c] %s\n", brt->dftype == ASPFS_TYPE_DIR?'D':'F', brt->dfLFN);
        print_f(mlogPool, "FS", mlog);
        brt = brt->br;
    }
    return 0;
}

static int mspFS_folderJump(struct aspDirnFile_s **dir, struct aspDirnFile_s *root, char *path)
{
    char mlog[256];
    int retval = 0;
    struct aspDirnFile_s *brt;

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
    p->opcode = (info >> 8) & 0xf;
    p->seqnum = (info >> 12) & 0x7;
    p->inout = (info >> 15) & 0x1;

    //sprintf(str, "info: 0x%.4x \n", info); 
    //print_f(mlogPool, "abs_info", str); 

    return info;
}

inline uint16_t pkg_info(struct info16Bit_s *p)
{
    char str[128];
    uint16_t info = 0;
    info |= p->data & 0xff;
    info |= (p->opcode & 0xf) << 8;
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
                next = PSWT;
                break;
            case PSWT:
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "spy", str); 
                next = PSRLT;
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
                //next = PSWT;
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
                evt = 0x1; /* jump to next stage */
                break;
            default:
                //sprintf(str, "default\n"); 
                //print_f(mlogPool, "laser", str); 
                next = PSSET;
                break;
        }
    }
    else if (rlt == BREAK) {
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
            if (evt == 0x1) nxtst = SPY; /* end the test loop */
            break;
        case DOUBLEC:
            ret = next_doubleC(data);
            evt = (ret >> 24) & 0xff;
            if (evt == 0x1) nxtst = DOUBLED; /* end the test loop */
            break;
        case DOUBLED:
            ret = next_doubleD(data);
            evt = (ret >> 24) & 0xff;
            if (evt == 0x1) nxtst = SMAX; /* end the test loop */
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
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0; 
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
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0; 
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
            if (data->ansp0 == 1)
                data->result = emb_result(data->result, NEXT);
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0; 
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
            if (data->ansp0 == 1)
                data->result = emb_result(data->result, NEXT);
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0; 
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
            if (data->ansp0 == 1)
                data->result = emb_result(data->result, NEXT);
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0; 
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
            if (data->ansp0 == 1)
                data->result = emb_result(data->result, NEXT);
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0; 
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
            if (data->ansp0 == 1)
                data->result = emb_result(data->result, NEXT);
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0; 
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
            if (data->ansp0 == 1)
                data->result = emb_result(data->result, NEXT);
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0; 
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
            if (data->ansp0 == 1)
                data->result = emb_result(data->result, NEXT);
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0; 
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }

    return ps_next(data);
}

static int stdob_10(struct psdata_s *data)
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
            if (data->ansp0 == 1)
                data->result = emb_result(data->result, NEXT);
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0; 
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
            if (data->ansp0 == 1)
                data->result = emb_result(data->result, NEXT);
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0; 
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
            ch = 3; 
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
            }            
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0; 
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
            if (data->ansp0 == 1)
                data->result = emb_result(data->result, NEXT);
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0; 
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
            if (data->ansp0 == 1)
                data->result = emb_result(data->result, NEXT);
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0; 
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
            if (data->ansp0 == 1)
                data->result = emb_result(data->result, NEXT);
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0; 
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
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0; 
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
            if (data->ansp0 == 1)
                data->result = emb_result(data->result, NEXT);
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0; 
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
            if (data->ansp0 == 1)
                data->result = emb_result(data->result, NEXT);
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0; 
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
            if (data->ansp0 == 1)
                data->result = emb_result(data->result, NEXT);
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0; 
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
            ch = 1; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0; 
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
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0; 
            rs_ipc_put(data->rs, &ch, 1);
            break;
        default:
            break;
    }
    return ps_next(data);
}
static int stlaser_02(struct psdata_s *data) 
{ 
    char str[128], ch = 0;
    uint32_t rlt;
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
                data->result = emb_result(data->result, NEXT);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0; 
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
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0; 
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
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0; 
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
            }
            break;
        case NEXT:
            break;
        case BREAK:
            ch = 0; 
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

        inc++;
        src++;
        if (!((inc+1) % 16)) {
            sprintf(str, "\n");
            print_f(mlogPool, NULL, str);
        }
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
    //sprintf(str, "d:%d, %d /%d \n", dist, dualn, folwn);
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
    char str[128];
    sel = sel % 2;

    if (sel) {
        pp->dualsz = size;
    } else {
        pp->lastsz = size;
    }
    pp->lastflg += 1;

    sprintf(str, "[last] d:%d l:%d flg:%d \n", pp->dualsz, pp->lastsz, pp->lastflg);
    print_f(mlogPool, "ring", str);

    msync(pp, sizeof(struct shmem_s), MS_SYNC);
    return pp->lastflg;
}

static int ring_buf_set_last(struct shmem_s *pp, int size)
{
    pp->lastsz = size;
    pp->lastflg = 1;

    msync(pp, sizeof(struct shmem_s), MS_SYNC);
    return pp->lastflg;
}
static int ring_buf_prod_dual(struct shmem_s *pp, int sel)
{
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
    return 0;
}

static int ring_buf_prod(struct shmem_s *pp)
{
    if ((pp->r->lead.seq + 1) < pp->slotn) {
        pp->r->lead.seq += 1;
    } else {
        pp->r->lead.seq = 0;
        pp->r->lead.run += 1;
    }
    msync(pp, sizeof(struct shmem_s), MS_SYNC);
    return 0;
}

static int ring_buf_cons_dual(struct shmem_s *pp, char **addr, int sel)
{
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

    //sprintf(str, "[cons], d: %d %d/%d/%d \n", dist, leadn, dualn, folwn);
    //print_f(mlogPool, "ring", str);

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
                return pp->dualsz;
            }
        } else {
            if ((pp->r->folw.run == pp->r->lead.run) &&
             (pp->r->folw.seq == pp->r->lead.seq)) {
                return pp->lastsz;
            }
        }
    }
    msync(pp, sizeof(struct shmem_s), MS_SYNC);
    return pp->chksz;
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

    //sprintf(str, "cons, d: %d %d/%d \n", dist, leadn, folwn);
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
        if ((pp->r->folw.run == pp->r->lead.run) &&
            (pp->r->folw.seq == pp->r->lead.seq)) {
            return pp->lastsz;
        }
    }

    msync(pp, sizeof(struct shmem_s), MS_SYNC);

    return pp->chksz;
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

    ret = ioctl(fd, SPI_IOC_MESSAGE(1), tr);
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
    munmap(mrs->cmdTx.pp[0], 512*SPI_TRUNK_SZ);
    free(mrs->dataRx.pp);
    free(mrs->cmdRx.pp);
    free(mrs->dataTx.pp);
    free(mrs->cmdTx.pp);
    return 0;
}

static int cmdfunc_opchk_single(uint32_t val, uint32_t mask, int len)
{
    int cnt=0, s=0;
    if (val > mask) return -1;
    if (len > 32) return -2;
    if (!len) return -3;

    s = 0;
    while(s < len) {
        if (val & (0x1 << s)) cnt++;
        s++;
    }

    if (cnt == 0) return -4;
    if (cnt > 1) return -5;

    return val;
}
static int cmdfunc_opcode(int argc, char *argv[])
{
    int val=0;
    int n=0, ix=0, ret=0;
    char ch=0, opcode[5];
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
    for (ix = 0; ix < n; ix++) {
        sprintf(mrs->log, "%d.%.2x\n", ix, opcode[ix]); 
        print_f(&mrs->plog, "DBG", mrs->log);
    }
    /* debug print */

    if (n != 5) {ret = -2; goto end;}
    if (opcode[0] != 0xaa) {ret = -3; goto end;}
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

    if (ctb) {
        cd = opcode[3];
        ret = cmdfunc_opchk_single(cd, ctb->opMask, ctb->opBitlen);
        if (ret > 0) {
            ctb->opValue = ret;
        } else {
            ret = (ret * 10) -6;
        }
        sprintf(mrs->log, "opcode 0x%.2x/0x%.2x, input value: 0x%.2x\n", ctb->opCode, ctb->opValue, val); 
        print_f(&mrs->plog, "DBG", mrs->log);
    } else {
        sprintf(mrs->log, "cmdfunc_opcode - 3\n"); 
        print_f(&mrs->plog, "DBG", mrs->log);
        ret = -7;
    }

end:

    if (ret < 0) {
        ch = 'x';
        mrs_ipc_put(mrs, &ch, 1, 5);
        ch = 0xf0;
        mrs_ipc_put(mrs, &ch, 1, 5);

        if (!ctb) {
            sprintf(mrs->log, "xxfailed: input 0x%x/0x%x, ret:%d", opcode[1], opcode[3], ret); 
        } else {
            sprintf(mrs->log, "xxfailed: input 0x%x/0x%x, mask:0x%x, bitlen:%d, ret:%d", opcode[1], opcode[3], ctb->opMask, ctb->opBitlen, ret); 
        }

    } else {
        ch = 'p';
        mrs_ipc_put(mrs, &ch, 1, 5);

        sprintf(mrs->log, "succeed: result 0x%x/0x%x, ret: %d", ctb->opCode, ctb->opValue, ret);	
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
    int ci, pi, ret, idle=0, wait=-1, loglen=0;
    char cmd[256], *addr[3], rsp[256], ch, *plog;
    char poll[32] = "poll";

    struct cmd_s cmdtab[8] = {{0, "poll", cmdfunc_01}, {1, "command", cmdfunc_01}, {2, "data", cmdfunc_01}, {3, "op", cmdfunc_opcode}, 
                                {4, "aspect", cmdfunc_01}, {5, "go", cmdfunc_01}, {6, "reset", cmdfunc_01}, {7, "launch", cmdfunc_01}};

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
        while (pi < 8) {
            if ((strlen(cmd) == strlen(cmdtab[pi].str))) {
                if (!strncmp(cmd, cmdtab[pi].str, strlen(cmdtab[pi].str))) {
                    break;
                }
            }
            pi++;
        }

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

        /* command execution */
        if (ci > 0) {
            if (pi < 8) {
                addr[0] = (char *)mrs;
                sprintf(mrs->log, "input [%d]%s\n", pi, cmdtab[pi].str, cmdtab[pi].id, cmd);
                print_f(&mrs->plog, "DBG", mrs->log);
                ret = cmdtab[pi].pfunc(cmdtab[pi].id, addr);
                wait = 1;
                memset(plog, 0, 2048);
                loglen = 0;
                memset(cmd, 0, 256);
            } else {
/*
                mrs_ipc_put(mrs, "?", 1, 5); 
*/
                continue;
            }
        } else {
            if (wait < 0) continue;
        }

        ch = 0;
        ret = mrs_ipc_get(mrs, &ch, 1, 6);
        while (ret > 0) {
            sprintf(mrs->log, "%c", ch);
            print_f(&mrs->plog, "DBG", mrs->log);

            if (loglen > 0) {
                plog[loglen] = ch;
                loglen++;
                if ((ch == '>') || (loglen == 2048)) {
/*
                    mrs_ipc_put(mrs, plog, loglen, 5);
*/
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
/*
                mrs_ipc_put(mrs, plog, loglen, 5);
*/
            }
            wait = -1;
        }

        printf_flush(&mrs->plog, mrs->flog);
    }

    p0_end(mrs);
}

static int fs00(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p;
    p = &mrs->mchine.cur;

    sprintf(mrs->log, "usleep(%d) %d -> %d \n", modersp->v, modersp->m, modersp->d);
    print_f(&mrs->plog, "fs00", mrs->log);

    usleep(modersp->v);
    modersp->m = modersp->d;

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
    sprintf(mrs->log, "check RDY high \n");
    print_f(&mrs->plog, "fs02", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if ((len > 0) && (ch == 'B')){
        //modersp->m = modersp->m + 1;
        modersp->r = 1;
        return 1;
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
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 0 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs05", mrs->log);
    bitset = 1;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 1 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs05",mrs->log);

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
    p = &mrs->mchine.cur;

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = OP_DAT;
    p->inout = 0;
    p->seqnum = mrs->mchine.seqcnt;

    sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
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
    p = &mrs->mchine.cur;

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = OP_DAT;
    p->inout = 0;
    p->seqnum = mrs->mchine.seqcnt;

    sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    print_f(&mrs->plog, "fs14", mrs->log);
	
    mrs_ipc_put(mrs, "c", 1, 1);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs15(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        print_f(&mrs->plog, "fs15", mrs->log);

        if (p->opcode == OP_DAT) {
            modersp->m = modersp->m + 1;
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
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi0 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs16", mrs->log);
    bitset = 0;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi1 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs16", mrs->log);

#if SPI_KTHREAD_USE
    bitset = 0;
    ret = ioctl(mrs->sfm[0], _IOR(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_START_THREAD
    sprintf(mrs->log, "Start spi0 spidev thread, ret: 0x%x\n", ret);
    print_f(&mrs->plog, "fs16", mrs->log);

    bitset = 0;
    ret = ioctl(mrs->sfm[1], _IOR(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_START_THREAD
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
    return 2;
}

static int fs18(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int ret, bitset;
    char ch;

    //sprintf(mrs->log, "%d\n", modersp->v);
    //print_f(&mrs->plog, "fs18", mrs->log);

    ret = mrs_ipc_get(mrs, &ch, 1, 1);
    while (ret > 0) {
        if (ch == 'p') {
            modersp->v += 1;
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
            modersp->v += 1;
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
        mrs_ipc_put(mrs, "D", 1, 3);
        sprintf(mrs->log, "%d end\n", modersp->v);
        print_f(&mrs->plog, "fs18", mrs->log);
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

        modersp->m = modersp->m + 1;
        return 2;
    }

    return 0;
}

static int fs20(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int bitset, ret;

    bitset = 0;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi0 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs20", mrs->log);

    bitset = 0;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi1 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs20", mrs->log);

    bitset = 0;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 0 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs20", mrs->log);

    bitset = 0;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 1 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs20", mrs->log);
#if SPI_KTHREAD_USE
    bitset = 0;
    ret = ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
    sprintf(mrs->log, "Stop spi0 spidev thread, ret: 0x%x\n", ret);
    print_f(&mrs->plog, "fs20", mrs->log);

    bitset = 0;
    ret = ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 14, __u32), &bitset);  //SPI_IOC_STOP_THREAD
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
    return 0; 
}

static int fs23(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset;
    bitset = 1;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY

    sprintf(mrs->log, "Set spi 0 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs23", mrs->log);

    bitset = 1;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY

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
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",0, bitset, modersp->d);
    print_f(&mrs->plog, "fs24", mrs->log);

    bitset = 1;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",1, bitset, modersp->d);
    print_f(&mrs->plog, "fs24", mrs->log);

    usleep(1000);

    bitset = 0;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",0, bitset, modersp->d);
    print_f(&mrs->plog, "fs24", mrs->log);

    bitset = 0;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",1, bitset, modersp->d);
    print_f(&mrs->plog, "fs24", mrs->log);

    sleep(1);

    bitset = 1;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",0, bitset, modersp->d);
    print_f(&mrs->plog, "fs24", mrs->log);

    bitset = 1;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",1, bitset, modersp->d);
    print_f(&mrs->plog, "fs24", mrs->log);

    usleep(1000);

    bitset = 0;
    ioctl(mrs->sfm[0], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
    sprintf(mrs->log, "[%d]Get RDY pin %d, cnt:%d\n",0, bitset, modersp->d);
    print_f(&mrs->plog, "fs24", mrs->log);

    bitset = 0;
    ioctl(mrs->sfm[1], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
    sprintf(mrs->log, "[%d]Get RDY pin %d, cnt:%d\n",1, bitset, modersp->d);
    print_f(&mrs->plog, "fs24", mrs->log);

    bitset = 0;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 0 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs24", mrs->log);

    bitset = 0;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
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

    p->opcode = OP_DUL;
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

    p->opcode = OP_DUL;
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

        if (p->opcode == OP_DUL) {
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
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi0 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs33", mrs->log);
    bitset = 0;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi1 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs33", mrs->log);

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
    int len=0;
    char ch=0;
    //sprintf(mrs->log, "wait socket finish \n");
    //print_f(&mrs->plog, "fs19", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'N')) {
        ring_buf_init(&mrs->dataRx);
        mrs->dataRx.r->folw.seq = 1;

        modersp->m = modersp->m + 1;
        return 2;
    }

    return 0;
}

static int fs37(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int bitset, ret;

    bitset = 0;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi0 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs37", mrs->log);

    bitset = 0;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi1 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs37", mrs->log);

    bitset = 0;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 0 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs37", mrs->log);

    bitset = 0;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 1 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs37", mrs->log);

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
    return 0; 
}

static int fs40(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset;
    bitset = 1;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY

    sprintf(mrs->log, "Set spi 0 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs23", mrs->log);

    bitset = 1;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY

    sprintf(mrs->log, "Set spi 1 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs40", mrs->log);

    modersp->r = 1;
    return 1;
}

static int p0(struct mainRes_s *mrs)
{
#define PS_NUM 41

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
                                 {40, fs40}};

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
                if (ch == 0) {
                    modesw.m = ch;
                    modesw.d = -1;
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
    int px, pi, ret = 0, ci, len, logcnt=0;
    char ch, cmd, cmdt, str[128];
    char *addr;
    uint32_t evt;

    sprintf(rs->logs, "p1\n");
    print_f(rs->plogs, "P1", rs->logs);
    struct psdata_s stdata;
    stfunc pf[SMAX][PSMAX] = {{stspy_01, stspy_02, stspy_03, stspy_04, stspy_05},
                            {stbullet_01, stbullet_02, stbullet_03, stbullet_04, stbullet_05},
                            {stlaser_01, stlaser_02, stlaser_03, stlaser_04, stlaser_05},
                            {stdob_01, stdob_02, stdob_03, stdob_04, stdob_05},
                            {stdob_06, stdob_07, stdob_08, stdob_09, stdob_10}};
    p1_init(rs);
    // wait for ch from p0
    // state machine control
    stdata.rs = rs;
    pi = 0;    stdata.result = 0;    cmd = '\0';   cmdt = 'w';
    while (1) {
        //sprintf(rs->logs, "+\n");
        //print_f(rs->plogs, "P1", rs->logs);

        cmd = '\0';
        ci = 0; 
        ci = rs_ipc_get(rcmd, &cmd, 1);
        while (ci > 0) {
            //sprintf(rs->logs, "%c\n", cmd);
            //print_f(rs->plogs, "P1CMD", rs->logs);

            if (cmdt == '\0') {
                if (cmd == 'd') {
                    cmdt = cmd;
                    stdata.result = emb_stanPro(0, STINIT, BULLET, PSSET);
                } else if (cmd == 'p') {
                    cmdt = cmd;
                    stdata.result = emb_stanPro(0, STINIT, SPY, PSTSM);
                } else if (cmd == '=') {
                    cmdt = cmd;
                    stdata.result = emb_stanPro(0, STINIT, SMAX, PSMAX);
                } else if (cmd == 'n') {
                    cmdt = cmd;
                    stdata.result = emb_stanPro(0, STINIT, DOUBLEC, PSSET);
                }

                if (cmdt != '\0') {
                    logcnt = 0;
                    break;
                }
            } else { /* command to interrupt state machine here */
                if (cmd == 'r') {
                    cmdt = cmd;
                    stdata.result = emb_result(stdata.result, BREAK);
                }
            }
            ci = 0;
            ci = rs_ipc_get(rcmd, &cmd, 1);            
        }

        ret = 0; ch = '\0';
        ret = rs_ipc_get(rs, &ch, 1);
        //if (ret > 0) {
        //    sprintf(rs->logs, "ret:%d ch:%c\n", ret, ch);
        //    print_f(rs->plogs, "P1", rs->logs);
        //}

        if (((ret > 0) && (ch != '$')) || (cmdt != '\0')){
            stdata.ansp0 = ch;
            evt = stdata.result;
            pi = (evt >> 8) & 0xff;
            px = (evt & 0xff);
            if ((pi >= SMAX) || (px >= PSMAX)) {
                if (cmdt != '\0') {
                    sprintf(str, "<%c;0x%x;done>", cmdt, ch);
                    print_f(rs->plogs, "P1", str);

                    len = strlen(str);
                    if (len >= 256) len = 255;
                    str[len] = '\0';
                    rs_ipc_put(rcmd, str, len+1);
                    logcnt = 0;

                    cmdt = '\0'; 
                    //sprintf(rs->logs, "comdt:0x%x ch:0x%x evt:0x%.8x - end\n", cmdt, ch, stdata.result);
                    //print_f(rs->plogs, "P1", rs->logs);
                }

                //stdata.result = emb_stanPro(0, STINIT, SPY, PSSET);
                continue;
            }

            stdata.result = (*pf[pi][px])(&stdata);

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

static int p2(struct procRes_s *rs)
{
    struct spi_ioc_transfer *tr = malloc(sizeof(struct spi_ioc_transfer));
    struct timespec tnow;
    int px, pi=0, ret, len=0, opsz, cmode=0, tdiff, tlast, twait;
    int bitset;
    uint16_t send16, recv16;
    char ch, str[128], rx8[4], tx8[4];
    char *addr, *laddr;
    sprintf(rs->logs, "p2\n");
    print_f(rs->plogs, "P2", rs->logs);
    p2_init(rs);
    // wait for ch from p0
    // in charge of spi0 data mode
    // 'd': data mode, store the incomming infom into share memory
    // send 'd' to notice the p0 that we have incomming data chunk

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
                default:
                    break;
            }

            //sprintf(rs->logs, "cmode: %d - 2\n", cmode);
            //print_f(rs->plogs, "P2", rs->logs);
			
            if (cmode == 1) {
                //sprintf(rs->logs, "cmode: %d - 3\n", cmode);
                //print_f(rs->plogs, "P2", rs->logs);

                bitset = 1;
                ioctl(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
                //sprintf(rs->logs, "Check if RDY pin == 0 (pin:%d)\n", bitset);
                //print_f(rs->plogs, "P2", rs->logs);

                while (1) {
                    bitset = 1;
                    ioctl(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
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

                //sprintf(rs->logs, "send %d %d \n", tx8[0], tx8[1]);
                //print_f(rs->plogs, "P2", rs->logs);

                len = mtx_data(rs->spifd, rx8, tx8, 2, tr);
                if (len > 0) {
                    recv16 = rx8[1] | (rx8[0] << 8);
                    abs_info(&rs->pmch->get, recv16);
                    rs_ipc_put(rs, "C", 1);
                }
            } else if (cmode == 4) {
                //sprintf(rs->logs, "cmode: %d - 5\n", cmode);
                //print_f(rs->plogs, "P2", rs->logs);

                bitset = 0;
                ioctl(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
                //sprintf(rs->logs, "Check if RDY pin == 1 (pin:%d)\n", bitset);
                //print_f(rs->plogs, "P2", rs->logs);
            
                while (1) {
                    bitset = 0;
                    ioctl(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
                    //sprintf(rs->logs, "Get RDY pin: %d\n", bitset);
                    //print_f(rs->plogs, "P4", rs->logs);
            
                    if (bitset == 1) break;
                }
                if (bitset) rs_ipc_put(rs, "B", 1);
            }else if (cmode == 5) {
                //sprintf(rs->logs, "cmode: %d\n", cmode);
                //print_f(rs->plogs, "P2", rs->logs);

                pi = 0;  

                while (1) {
                    len = ring_buf_get_dual(rs->pdataRx, &addr, pi);
                    clock_gettime(CLOCK_REALTIME, rs->tm[0]);
#if SPI_KTHREAD_USE
                    opsz = ioctl(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr);  //SPI_IOC_PROBE_THREAD
#else
                    opsz = mtx_data(rs->spifd, addr, NULL, len, tr);

#endif                    
                    //printf("0 spi %d\n", opsz);
                    //sprintf(rs->logs, "spi0 recv %d\n", opsz);
                    //print_f(rs->plogs, "P2", rs->logs);
                    clock_gettime(CLOCK_REALTIME, &tnow);

			if (opsz == 0) {
	                    //sprintf(rs->logs, "opsz:%d\n", opsz);
       	             //print_f(rs->plogs, "P2", rs->logs);	
                           //usleep(1000);
				continue;
			}

                    msync(rs->tm, sizeof(struct timespec) * 2, MS_SYNC);

                    tdiff = time_diff(rs->tm[0], rs->tm[1], 1000);
                    if (tdiff == -1) {
                         tdiff = 0 - time_diff(rs->tm[1], rs->tm[0], 1000);
                    }

                    sprintf(rs->logs, "t %d us\n", tdiff);
                    print_f(rs->plogs, "P2", rs->logs);					

                    if (tdiff < 0) {
                        sprintf(rs->logs, "!!!t %d - %d!!!\n", tdiff, len);
                        print_f(rs->plogs, "P2", rs->logs);
                    }

                    //msync(addr, len, MS_SYNC);
                    ring_buf_prod_dual(rs->pdataRx, pi);
                    //shmem_dump(addr, 32);

                    if (opsz < 0) break;
                    rs_ipc_put(rs, "p", 1);
                    pi += 2;
                }

                opsz = 0 - opsz;
                ring_buf_set_last_dual(rs->pdataRx, opsz, pi);
                rs_ipc_put(rs, "d", 1);
                //sprintf(rs->logs, "spi0 recv end\n");
                //print_f(rs->plogs, "P2", rs->logs);
            }else if (cmode == 6) {
                sprintf(rs->logs, "cmode: %d\n", cmode);
                print_f(rs->plogs, "P2", rs->logs);

                pi = 0;  len = 0;
                len = ring_buf_get(rs->pcmdRx, &addr);
                while (len > 0) {

                    opsz = mtx_data(rs->spifd, addr, NULL, len, tr);
                    sprintf(rs->logs, "spi0 recv %d\n", opsz);
                    print_f(rs->plogs, "P2", rs->logs);

			if (opsz == 0) {
	                    sprintf(rs->logs, "opsz:%d\n", opsz);
       	             print_f(rs->plogs, "P2", rs->logs);	
                           continue;
			}

                    //msync(addr, len, MS_SYNC);
                    ring_buf_prod(rs->pcmdRx);    
                    if (opsz < 0) break;
                    rs_ipc_put(rs, "p", 1);
                    pi += 1;

                    len = ring_buf_get(rs->pcmdRx, &addr);
                }

                opsz = 0 - opsz;
                ring_buf_set_last(rs->pcmdRx, opsz);
                rs_ipc_put(rs, "d", 1);
                sprintf(rs->logs, "spi0 recv end\n");
                print_f(rs->plogs, "P2", rs->logs);

            }else {
                sprintf(rs->logs, "cmode: %d - 7\n", cmode);
                print_f(rs->plogs, "P2", rs->logs);
            }

/*
            else if (cmode == 2) {
                len = ring_buf_cons(rs->pcmdTx, &addr);
                if (len >= 0) {
                    msync(addr, len, MS_SYNC);
                    // send data to spi command mode
                    opsz = mtx_data(rs->spifd, NULL, addr, len, 1024*1024);
                    //sprintf(rs->logs, "[%d] spi tx %d\n", px, opsz);
                    //print_f(rs->plogs, "P2", rs->logs);
                    px++;
                } else {
                    sprintf(rs->logs, "[%d] spi tx cons ret:%d\n", px, len);
                    print_f(rs->plogs, "P2", rs->logs);
                }
            }
*/

        }
    }

    p2_end(rs);
    return 0;
}

static int p3(struct procRes_s *rs)
{
    struct spi_ioc_transfer *tr = malloc(sizeof(struct spi_ioc_transfer));
    struct timespec tnow;
    int pi, ret, len, opsz, cmode, bitset, tdiff, tlast, twait;
    uint16_t send16, recv16;
    char ch, str[128], rx8[4], tx8[4];
    char *addr, *laddr;
    sprintf(rs->logs, "p3\n");
    print_f(rs->plogs, "P3", rs->logs);

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
                ioctl(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
                //sprintf(rs->logs, "Check if RDY pin == 0 (pin:%d)\n", bitset);
                //print_f(rs->plogs, "P3", rs->logs);

                while (1) {
                    bitset = 1;
                    ioctl(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
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
                ioctl(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
                //sprintf(rs->logs, "Check if RDY pin == 1 (pin:%d)\n", bitset);
                //print_f(rs->plogs, "P3", rs->logs);
            
                while (1) {
                    bitset = 0;
                    ioctl(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
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

                    clock_gettime(CLOCK_REALTIME, rs->tm[1]);
#if SPI_KTHREAD_USE
                    opsz = ioctl(rs->spifd, _IOR(SPI_IOC_MAGIC, 15, __u32), addr);  //SPI_IOC_PROBE_THREAD
#else
                    opsz = mtx_data(rs->spifd, addr, NULL, len, tr);
#endif

                    //sprintf(rs->logs, "1 spi %d\n", opsz);
                    //print_f(rs->plogs, "P5", rs->logs);
                    //sprintf(rs->logs, "spi1 recv %d\n", opsz);
                    //print_f(rs->plogs, "P3", rs->logs);

                    clock_gettime(CLOCK_REALTIME, &tnow);

			if (opsz == 0) {
	                    //sprintf(rs->logs, "opsz:%d\n", opsz);
       	             //print_f(rs->plogs, "P3", rs->logs);	
                           //usleep(1000);
				continue;
			}

                    msync(rs->tm, sizeof(struct timespec) * 2, MS_SYNC);

                    tdiff = time_diff(rs->tm[1], rs->tm[0], 1000);
                    if (tdiff == -1) {
                         tdiff = 0 - time_diff(rs->tm[0], rs->tm[1], 1000);
                    }

                    sprintf(rs->logs, "t %d us\n", tdiff);
                    print_f(rs->plogs, "P3", rs->logs);
                    if (tdiff < 0) {                    
                        sprintf(rs->logs, "!!!t %d - %d!!!\n", tdiff, len);
                        print_f(rs->plogs, "P3", rs->logs);
                    }
                    //shmem_dump(addr, 32);
                    //msync(addr, len, MS_SYNC);
                    ring_buf_prod_dual(rs->pdataRx, pi);

                    if (opsz < 0) break;
                    rs_ipc_put(rs, "p", 1);
                    pi += 2;
                }

                opsz = 0 - opsz;
                ring_buf_set_last_dual(rs->pdataRx, opsz, pi);
                rs_ipc_put(rs, "d", 1);
                //sprintf(rs->logs, "spi1 recv end\n");
                //print_f(rs->plogs, "P3", rs->logs);
            }else  if (cmode == 6) {
                sprintf(rs->logs, "cmode: %d\n", cmode);
                print_f(rs->plogs, "P3", rs->logs);

                pi = 0;  len = 0;
                len = ring_buf_get(rs->pcmdTx, &addr);
                while (len > 0) {

                    opsz = mtx_data(rs->spifd, addr, NULL, len, tr);
                    sprintf(rs->logs, "spi0 recv %d\n", opsz);
                    print_f(rs->plogs, "P3", rs->logs);

			if (opsz == 0) {
	                    sprintf(rs->logs, "opsz:%d\n", opsz);
       	             print_f(rs->plogs, "P3", rs->logs);	
                           continue;
			}

                    //msync(addr, len, MS_SYNC);
                    ring_buf_prod(rs->pcmdTx);    
                    if (opsz < 0) break;
                    rs_ipc_put(rs, "p", 1);
                    pi += 1;

                    len = ring_buf_get(rs->pcmdTx, &addr);
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

#define MSP_P4_SAVE_DAT (0)

static int p4(struct procRes_s *rs)
{
    int px, pi, ret=0, len, opsz;
    int cmode, acuhk, errtor=0;
    char ch, str[128];
    char *addr;
    sprintf(rs->logs, "p4\n");
    print_f(rs->plogs, "P4", rs->logs);

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
                ret = file_save_get(&rs->fdat_s, "/mnt/mmc2/tx/%d.dat");
                if (ret) {
                    sprintf(rs->logs, "get tx log data file error - %d, hold here\n", ret);
                    print_f(rs->plogs, "P4", rs->logs);         
                    while(1);
                } else {
                    sprintf(rs->logs, "get tx log data file ok - %d, f: %d\n", ret, rs->fdat_s);
                    print_f(rs->plogs, "P4", rs->logs);         
                }
#endif
                while (1) {
                    len = ring_buf_cons_dual(rs->pdataRx, &addr, pi);
                    if (len >= 0) {
                        //printf("cons 0x%x %d %d \n", addr, len, pi);
                        pi++;
                    
                        msync(addr, len, MS_SYNC);
                        /* send data to wifi socket */
                        sprintf(rs->logs, " %d -%d \n", len, pi);
                        print_f(rs->plogs, "P4", rs->logs);         
                        if (len != 0) {
#if 1 /* debug */
                            opsz = write(rs->psocket_t->connfd, addr, len);
#else
                            opsz = len;
#endif
                            //printf("socket tx %d %d\n", rs->psocket_r->connfd, opsz);
                            //sprintf(rs->logs, "%c socket tx %d %d %d \n", ch, rs->psocket_t->connfd, opsz, pi);
                            //print_f(rs->plogs, "P4", rs->logs);         
#if MSP_P4_SAVE_DAT
                            fwrite(addr, 1, len, rs->fdat_s);
                            fflush(rs->fdat_s);
#endif
                        }
                    } else {
                        sprintf(rs->logs, "%c socket tx %d %d %d- end\n", ch, rs->psocket_t->connfd, opsz, pi);
                        print_f(rs->plogs, "P4", rs->logs);         
                        break;
                    }

                    if (ch != 'D') {
                        ch = 0;
                        rs_ipc_get(rs, &ch, 1);
                    }
                }

		  while (ch != 'D') {
                        sprintf(rs->logs, "%c clr\n", ch);
                        print_f(rs->plogs, "P4", rs->logs);         
                        ch = 0;
                        rs_ipc_get(rs, &ch, 1);
		  }

                rs_ipc_put(rs, "D", 1);
                sprintf(rs->logs, "%c socket tx %d - end\n", ch, pi);
                print_f(rs->plogs, "P4", rs->logs);         
#if MSP_P4_SAVE_DAT
                fclose(rs->fdat_s);
#endif
                break;
            }
            else if (cmode == 2) {
                px = 0;

                len = ring_buf_get(rs->pcmdTx, &addr);
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
                    len = ring_buf_get(rs->pcmdTx, &addr);

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
                break;
            }
            else if (cmode == 4) {
                pi = 0;
                while (1) {
                    len = ring_buf_cons(rs->pcmdRx, &addr);
                    if (len >= 0) {
                        pi++;
                    
                        msync(addr, len, MS_SYNC);
                        /* send data to wifi socket */
                        //sprintf(rs->logs, " %d -%d \n", len, pi);
                        //print_f(rs->plogs, "P4", rs->logs);         
                        if (len != 0) {
                            #if 1 /*debug*/
                            opsz = write(rs->psocket_t->connfd, addr, len);
                            #else
                            opsz = len;
                            #endif
                            sprintf(rs->logs, "%c socket tx %d %d %d \n", ch, rs->psocket_t->connfd, opsz, pi);
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

		  while (ch != 'N') {
                        sprintf(rs->logs, "%c clr\n", ch);
                        print_f(rs->plogs, "P4", rs->logs);         
                        ch = 0;
                        rs_ipc_get(rs, &ch, 1);
		  }

                rs_ipc_put(rs, "N", 1);
                sprintf(rs->logs, "%c socket tx %d - end\n", ch, pi);
                print_f(rs->plogs, "P4", rs->logs);         
                break;
            }else {
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
    int ret, n, num, hd, be, ed, ln;
    char ch, *recvbuf, *addr, *sendbuf;
    char msg[256], opcode=0, param=0;
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
        hd = atFindIdx(recvbuf, '!');
        if (hd < 0) goto socketEnd;
        be = atFindIdx(&recvbuf[hd], '[');
        if (be < 0) goto socketEnd;
        ed = atFindIdx(&recvbuf[hd], ']');
        if (ed < 0) goto socketEnd;
        ln = atFindIdx(&recvbuf[hd], '\0');

        n = strlen(&recvbuf[hd]);
        if (n <= 0) {
            goto socketEnd;
        }

        sprintf(rs->logs, "receive len[%d]content[%s]hd[%d]be[%d]ed[%d]ln[%d]\n", n, &recvbuf[hd], hd, be, ed, ln);
        print_f(rs->plogs, "P5", rs->logs);

        opcode = recvbuf[hd+1]; param = recvbuf[be-1];
        sprintf(rs->logs, "opcode:[0x%x]arg[0x%x]\n", opcode, param);
        print_f(rs->plogs, "P5", rs->logs);

        n = ed - be - 1;
        if ((n < 255) && (n > 0)) {
            memcpy(msg, &recvbuf[be+1], n);
            msg[n] = '\0';
        } else {
            goto socketEnd;
        }

        if (opcode != 0x30) {
            n = 0;
        }

        if (n > 0) {
            //rs_ipc_put(rs, "s", 1);
            //rs_ipc_put(rs, msg, n);
            //sprintf(rs->logs, "send to p0 [%s]\n", recvbuf);
            //print_f(rs->plogs, "P5", rs->logs);
            
            ret = write(rs->psocket_r->connfd, msg, n);
            //sprintf(rs->logs, "send back app [%s] size:%d/%d\n", recvbuf, ret, n);
            //print_f(rs->plogs, "P5", rs->logs);
            rs_ipc_put(rcmd, msg, n);
        }else {
            msg[0] = 'o';
            msg[1] = 'p';
            rs_ipc_put(rcmd, msg, 2);


            ch = 0; n = 0;
            n = rs_ipc_get(rcmd, &ch, 1);

            sprintf(rs->logs, "1.n:%d, ch:%c\n", n, ch);
            print_f(rs->plogs, "P5", rs->logs);

            if (ch == 'o') {			
                msg[0] = 0xaa;			
                msg[1] = opcode;
                msg[2] = '/';
                msg[3] = param;
                msg[4] = 0xa5;
                rs_ipc_put(rcmd, msg, 5);
            }

            ch = 0; n = 0;
            n = rs_ipc_get(rcmd, &ch, 1);
            sprintf(rs->logs, "2.n:%d, ch:%c\n", n, ch);
            print_f(rs->plogs, "P5", rs->logs);

            if (ch != 'p') {			
                opcode = OP_ERROR; 
                n = rs_ipc_get(rcmd, &ch, 1);
                param = ch;
                sprintf(rs->logs, "3.n:%d, ch:0x%.2x\n", n, ch);
                print_f(rs->plogs, "P5", rs->logs);
            }
        }

        usleep(100000);
        memset(sendbuf, 0, 2048);

        sendbuf[0] = '!';
        sendbuf[1] = ((opcode & 0x80) ? 1:0) + 1;
        sendbuf[2] = opcode & 0x7f;
        sendbuf[3] = '+';
        //sendbuf[3] = 'P';//0x0;
        sendbuf[6] = '[';

        n = rs_ipc_get(rcmd, &sendbuf[7], 2048 - 7);
        sendbuf[4] = ((param & 0x80) ? 1:0) + 1;
        sendbuf[5] = param & 0x7f;
		
        sendbuf[7+n] = ']';
        sendbuf[7+n+1] = '\0';
        sendbuf[7+n+2] = '\0';
        ret = write(rs->psocket_r->connfd, sendbuf, 7+n+3);
        sprintf(rs->logs, "socket send, len:%d content[%s] from %d, ret:%d, opcode:%d, [%x][%x][%x][%x]\n", 7+n+3, sendbuf, rs->psocket_r->connfd, ret, opcode, sendbuf[1], sendbuf[2], sendbuf[4], sendbuf[5]);
        print_f(rs->plogs, "P5", rs->logs);

        socketEnd:
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
    char *recvbuf, *sendbuf;
    int ret, n, num, hd, be, ed, ln;
	
    struct aspDirnFile_s *root = 0, *fscur = 0, *nxtf = 0;
    struct aspDirnFile_s *brt;

    //char dir[256] = "/mnt/mmc2";
    char dir[256] = "/root";
    char folder[256];

    sprintf(rs->logs, "p6\n");
    print_f(rs->plogs, "P6", rs->logs);

    p6_init(rs);

    ret = mspFS_createRoot(&root, dir);
    if (!ret) {
        sprintf(rs->logs, "FS root [%s] create done, root:0x%x\n", dir, root);
        print_f(rs->plogs, "P6", rs->logs);
        ret = mspFS_insertChilds(root);
        if (!ret) {
            sprintf(rs->logs, "FS insert ch done\n");
            print_f(rs->plogs, "P6", rs->logs);
            mspFS_showFolder(root);
            fscur = root;
        } else {
            sprintf(rs->logs, "FS insert ch failed\n");
            print_f(rs->plogs, "P6", rs->logs);
        }
    } else {
        sprintf(rs->logs, "FS root [%s] create failed ret:%d\n", ret);
        print_f(rs->plogs, "P6", rs->logs);
    }

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
        hd = atFindIdx(recvbuf, '!');
        if (hd < 0) goto socketEnd;
        be = atFindIdx(&recvbuf[hd], '[');
        if (be < 0) goto socketEnd;
        ed = atFindIdx(&recvbuf[hd], ']');
        if (ed < 0) goto socketEnd;
        ln = atFindIdx(&recvbuf[hd], '\0');

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

        sprintf(rs->logs, "jump folder:[%s]\n", folder);
        print_f(rs->plogs, "P6", rs->logs);

        nxtf = 0;
        ret = mspFS_folderJump(&nxtf, fscur, folder);
        if (ret < 0) {
            sprintf(rs->logs, "jump folder:[%s] failed, ret:%d\n", folder, ret);
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
            goto socketEnd;
        }

        sendbuf[0] = '!';
        sendbuf[1] = 0x11;
        sendbuf[2] = '+';
        sendbuf[3] = 0x01;
        sendbuf[4] = '[';
        brt = fscur->ch;
        while (brt) {
            n = strlen(brt->dfLFN);
            memcpy(&sendbuf[5], brt->dfLFN, n);

            if (brt->dftype == ASPFS_TYPE_FILE) {
                sendbuf[3] = 'F';
            } else {
                sendbuf[3] = 'D';
            }

            sendbuf[5+n] = ']';
            sendbuf[5+n+1] = '\n';
            sendbuf[5+n+2] = '\0';
            ret = write(rs->psocket_at->connfd, sendbuf, 5+n+3);
            sprintf(rs->logs, "socket send, len:%d content[%s] from %d, ret:%d\n", 5+n+3, sendbuf, rs->psocket_at->connfd, ret);
            print_f(rs->plogs, "P6", rs->logs);

            brt = brt->br;
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
    char ch=0, *addr=0;
    int ret=0, len=0, num=0, tx=0;
    int cmode;

    sprintf(rs->logs, "p7\n");
    print_f(rs->plogs, "P7", rs->logs);

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
                ret = file_save_get(&rs->fdat_s, "/mnt/mmc2/tx/%d.dat");
                if (ret) {
                    sprintf(rs->logs, "get tx log data file error - %d, hold here\n", ret);
                    print_f(rs->plogs, "P7", rs->logs);         
                    while(1);
                } else {
                    sprintf(rs->logs, "get tx log data file ok - %d, f: %d\n", ret, rs->fdat_s);
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
                            fwrite(addr, 1, len, rs->fdat_s);
                            fflush(rs->fdat_s);
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
                fclose(rs->fdat_s);
#endif
                break;
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
static char spi1[] = "/dev/spidev32766.0"; 
static char spi0[] = "/dev/spidev32765.0"; 

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
    pmrs->dataRx.pp = memory_init(&pmrs->dataRx.slotn, 1024*SPI_TRUNK_SZ, SPI_TRUNK_SZ);
    if (!pmrs->dataRx.pp) goto end;
    pmrs->dataRx.r = (struct ring_p *)mmap(NULL, sizeof(struct ring_p), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    pmrs->dataRx.totsz = 1024*SPI_TRUNK_SZ;
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
    pmrs->dataTx.pp = memory_init(&pmrs->dataTx.slotn, 256*SPI_TRUNK_SZ, SPI_TRUNK_SZ);
    if (!pmrs->dataTx.pp) goto end;
    pmrs->dataTx.r = (struct ring_p *)mmap(NULL, sizeof(struct ring_p), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    pmrs->dataTx.totsz = 256*SPI_TRUNK_SZ;
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
    pmrs->cmdRx.pp = memory_init(&pmrs->cmdRx.slotn, 256*SPI_TRUNK_SZ, SPI_TRUNK_SZ);
    if (!pmrs->cmdRx.pp) goto end;
    pmrs->cmdRx.r = (struct ring_p *)mmap(NULL, sizeof(struct ring_p), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    pmrs->cmdRx.totsz = 256*SPI_TRUNK_SZ;;
    pmrs->cmdRx.chksz = SPI_TRUNK_SZ;
    pmrs->cmdRx.svdist = 12;
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
            ctb->opValue = 0;
            ctb->opMask = ASPOP_MASK_0;
            ctb->opBitlen = 0;
            break;
        case ASPOP_FILE_FORMAT: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_FFORMAT;
            ctb->opType = ASPOP_TYPE_SINGLE;
            ctb->opValue = 0;
            ctb->opMask = ASPOP_MASK_3;
            ctb->opBitlen = 8;
            break;
        case ASPOP_COLOR_MODE:  
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_COLRMOD;
            ctb->opType = ASPOP_TYPE_SINGLE;
            ctb->opValue = 0;
            ctb->opMask = ASPOP_MASK_4;
            ctb->opBitlen = 8;
            break;
        case ASPOP_COMPRES_RATE:
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_COMPRAT;
            ctb->opType = ASPOP_TYPE_SINGLE;
            ctb->opValue = 0;
            ctb->opMask = ASPOP_MASK_3;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SCAN_MODE:   
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_SCANMOD;
            ctb->opType = ASPOP_TYPE_SINGLE;
            ctb->opValue = 0;
            ctb->opMask = ASPOP_MASK_2;
            ctb->opBitlen = 8;
            break;
        case ASPOP_DATA_PATH:   
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_DATPATH;
            ctb->opType = ASPOP_TYPE_SINGLE;
            ctb->opValue = 0;
            ctb->opMask = ASPOP_MASK_4;
            ctb->opBitlen = 8;
            break;
        case ASPOP_RESOLUTION:
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_RESOLTN;
            ctb->opType = ASPOP_TYPE_SINGLE;
            ctb->opValue = 0;
            ctb->opMask = ASPOP_MASK_4;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SCAN_GRAVITY:
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_SCANGAV;
            ctb->opType = ASPOP_TYPE_SINGLE;
            ctb->opValue = 0;
            ctb->opMask = ASPOP_MASK_2;
            ctb->opBitlen = 8;
            break;
        case ASPOP_MAX_WIDTH:   
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_MAXWIDH;
            ctb->opType = ASPOP_TYPE_SINGLE;
            ctb->opValue = 0;
            ctb->opMask = ASPOP_MASK_3;
            ctb->opBitlen = 8;
            break;
        case ASPOP_WIDTH_ADJ_H: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_WIDTHAD_H;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_WIDTH_ADJ_L: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_WIDTHAD_L;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SCAN_LENS_H: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_SCANLEN_H;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SCAN_LENS_L: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_SCANLEN_L;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0;
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

// spidev id
    int fd0, fd1;
    fd0 = open(spi0, O_RDWR);
    if (fd0 < 0) 
        printf("can't open device[%s]\n", spi0); 
    else 
        printf("open device[%s]\n", spi0); 
    fd1 = open(spi1, O_RDWR);
    if (fd1 < 0) 
            printf("can't open device[%s]\n", spi1); 
    else 
        printf("open device[%s]\n", spi1); 

    pmrs->sfm[0] = fd0;
    pmrs->sfm[1] = fd1;
    pmrs->smode = 0;
    pmrs->smode |= SPI_MODE_1;

    /* set RDY pin to low before spi setup ready */
    //bitset = 0;
    //ret = ioctl(pmrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
   // printf("[t]Set RDY low at beginning\n");


    bitset = 0;     
    ioctl(pmrs->sfm[0], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL    
    bitset = 1;    
    ioctl(pmrs->sfm[1], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL

    /*
     * spi mode 
     */ 
    ret = ioctl(pmrs->sfm[0], SPI_IOC_WR_MODE, &pmrs->smode);
    if (ret == -1) 
        printf("can't set spi mode\n"); 
    
    ret = ioctl(pmrs->sfm[0], SPI_IOC_RD_MODE, &pmrs->smode);
    if (ret == -1) 
        printf("can't get spi mode\n"); 
    
    /*
     * spi mode 
     */ 
    ret = ioctl(pmrs->sfm[1], SPI_IOC_WR_MODE, &pmrs->smode); 
    if (ret == -1) 
        printf("can't set spi mode\n"); 
    
    ret = ioctl(pmrs->sfm[1], SPI_IOC_RD_MODE, &pmrs->smode);
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

static int print_f(struct logPool_s *plog, char *head, char *str)
{
    int len;
    char ch[256];
    if((!head) || (!str)) return (-1);

    if (head)
        sprintf(ch, "[%s] %s", head, str);
    else 
        sprintf(ch, "%s", str);

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
    return 0;
}


