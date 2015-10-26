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
//main()

#define ENABLE_SPI1        (1)
#define SPI_CPHA  0x01          /* clock phase */
#define SPI_CPOL  0x02          /* clock polarity */
#define SPI_MODE_0		(0|0)
#define SPI_MODE_1		(0|SPI_CPHA)
#define SPI_MODE_2		(SPI_CPOL|0)
#define SPI_MODE_3		(SPI_CPOL|SPI_CPHA)
#define SPI_SPEED    10000000

#define OP_MAX 0xff
#define OP_NONE 0x00

/* flow operation */
#define OP_PON          0x1                
#define OP_QRY          0x2
#define OP_RDY          0x3                
#define OP_SINGLE       0x4
#define OP_DOUBLE       0x5
#define OP_ACTION       0x6
#define OP_FIH          0x7
#define OP_BACK          0x8
/* SD read write operation */               
#define OP_SDRD          0x20
#define OP_SDWT          0x21
#define OP_STSEC_00     0x22
#define OP_STSEC_01     0x23
#define OP_STSEC_02     0x24
#define OP_STSEC_03     0x25
#define OP_STLEN_00     0x26
#define OP_STLEN_01     0x27
#define OP_STLEN_02     0x28
#define OP_STLEN_03     0x29
#define OP_SDAT         0x2a
#define OP_FREESEC      0x2b
#define OP_USEDSEC      0x2c
#define OP_SDINIT          0x2d
#define OP_SDSTATS       0x2e
#define OP_EG_DECT       0x2f
/* scanner parameters */
#define OP_MSG          0x30       
#define OP_FFORMAT      0x31
#define OP_COLRMOD      0x32
#define OP_COMPRAT      0x33
#define OP_RESOLTN      0x34
#define OP_SCANGAV      0x35
#define OP_MAXWIDH      0x36
#define OP_WIDTHAD_H    0x37
#define OP_WIDTHAD_L    0x38
#define OP_SCANLEN_H    0x39
#define OP_SCANLEN_L    0x3a
#define OP_INTERIMG     0x3b
#define OP_AFEIC        0x3c
#define OP_EXTPULSE     0x3d
#define OP_SUPBACK      0x3e
#define OP_LOG          0x3f
#define OP_RGRD          0x40
#define OP_RGWT          0x41
#define OP_RGDAT        0x42
#define OP_RGADD_H      0x43
#define OP_RGADD_L      0x44

#define OP_CROP_01        0x45
#define OP_CROP_02        0x46
#define OP_CROP_03        0x47
#define OP_CROP_04        0x48
#define OP_CROP_05        0x49
#define OP_CROP_06        0x4a

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
#define OP_SAVE         0x80
#define OP_ERROR        0xe0

#define SEC_LEN 512
#define SPI_TRUNK_SZ   (32768)
#define DIRECT_WT_DISK    (0)

static FILE *mlog = 0;
static struct logPool_s *mlogPool;
static char *infpath;

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
    BULLET, // 1
    LASER,  // 2
    AUTO_A,  // 3
    AUTO_B,  // 4
    AUTO_C,  // 5
    AUTO_D,  // 6
    AUTO_E,  // 7
    AUTO_F,  // 8
    AUTO_G,  // 9
    SMAX,   // 9
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
    DOUSCAN_WHITE_BAL,
} doubleScan_e;

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
};

struct info16Bit_s{
    uint8_t     inout;
    uint8_t     seqnum;
    uint8_t     opcode;
    uint8_t     data;
    uint32_t   info;
};

struct SdAddrs_s{
    uint32_t f;
    uint32_t sdc;
    union {
        uint32_t n;
        uint8_t d[4];
    };
    uint32_t sda;
};

struct DiskFile_s{
    int  rtops;
    int  rtlen;
    FILE *vsd;
    char *sdt;
    int rtMax;
};

struct RegisterRW_s{
    uint32_t rga16;
    int rgidx;
    int rgact;
};

struct machineCtrl_s{
    uint32_t seqcnt;
    struct info16Bit_s tmp;
    struct info16Bit_s cur;
    struct info16Bit_s get;
    struct SdAddrs_s sdst;
    struct SdAddrs_s sdln;
    struct DiskFile_s fdsk;
    struct RegisterRW_s regp;
};

struct virtualReg_s {
    uint32_t vrAddr;
    uint32_t vrValue;
};

struct mainRes_s{
    int sid[6];
    int sfm[2];
    int smode;
    struct virtualReg_s regTable[128];
    struct machineCtrl_s mchine;
    // 3 pipe
    struct pipe_s pipedn[7];
    struct pipe_s pipeup[7];
    // data mode share memory
    struct shmem_s dataRx;
    struct shmem_s dataTx; /* we don't have data mode Tx, so use it as cmdRx for spi1 */
    // command mode share memory
    struct shmem_s cmdRx; /* cmdRx for spi0 */
    struct shmem_s cmdTx;
    int sd_init;
    // file save
    FILE *fptn;
    FILE *fs;
    // file log
    FILE *flog;
    FILE *fdat;
    // time measurement
    struct timespec time[2];
    // log buffer
    char log[256];
    struct socket_s socket_r;
    struct socket_s socket_t;
    struct logPool_s plog;
    char filein[128];
};

typedef int (*fselec)(struct mainRes_s *mrs, struct modersp_s *modersp);

struct fselec_s{
    int  id;
    fselec pfunc;
};

struct procRes_s{
    // pipe
    int spifd;
    struct virtualReg_s *pregtb;
    struct pipe_s *ppipedn;
    struct pipe_s *ppipeup;
    struct shmem_s *pdataRx;
    struct shmem_s *pdataTx;
    struct shmem_s *pcmdRx;
    struct shmem_s *pcmdTx;
    struct machineCtrl_s *pmch;

    int *psd_init;
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
    FILE *fdat_s;
    // time measurement
    struct timespec *tm[2];
    char logs[256];
    struct socket_s *psocket_r;
    struct socket_s *psocket_t;
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

static int pn_init(struct procRes_s *rs);
static int pn_end(struct procRes_s *rs);
//IPC wrap
static int rs_ipc_put(struct procRes_s *rs, char *str, int size);
static int rs_ipc_get(struct procRes_s *rs, char *str, int size);
static int mrs_ipc_put(struct mainRes_s *mrs, char *str, int size, int idx);
static int mrs_ipc_get(struct mainRes_s *mrs, char *str, int size, int idx);
static int mtx_data(int fd, uint8_t *rx_buff, uint8_t *tx_buff, int num, int pksz, int maxsz);

static int ring_buf_init(struct shmem_s *pp);
static int ring_buf_get_dual(struct shmem_s *pp, char **addr, int sel);
static int ring_buf_set_last_dual(struct shmem_s *pp, int size, int sel);
static int ring_buf_prod_dual(struct shmem_s *pp, int sel);
static int ring_buf_cons_dual(struct shmem_s *pp, char **addr, int sel);
static int ring_buf_get(struct shmem_s *pp, char **addr);
static int ring_buf_set_last(struct shmem_s *pp, int size);
static int ring_buf_prod(struct shmem_s *pp);
static int ring_buf_cons(struct shmem_s *pp, char **addr, int *len);
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
static int stauto_01(struct psdata_s *data);
static int stauto_02(struct psdata_s *data);
static int stauto_03(struct psdata_s *data);
static int stauto_04(struct psdata_s *data);
static int stauto_05(struct psdata_s *data);
static int stauto_06(struct psdata_s *data);
static int stauto_07(struct psdata_s *data);
static int stauto_08(struct psdata_s *data);
static int stauto_09(struct psdata_s *data);
static int stauto_10(struct psdata_s *data);
static int stauto_11(struct psdata_s *data);
static int stauto_12(struct psdata_s *data);
static int stauto_13(struct psdata_s *data);
static int stauto_14(struct psdata_s *data);
static int stauto_15(struct psdata_s *data);
static int stauto_16(struct psdata_s *data);
static int stauto_17(struct psdata_s *data);
static int stauto_18(struct psdata_s *data);
static int stauto_19(struct psdata_s *data);
static int stauto_20(struct psdata_s *data);

FILE *find_save(char *dst, char *tmple)
{
    int i;
    FILE *f=0;
    for (i =0; i < 1000; i++) {
        f = 0;
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

inline uint16_t abs_info(struct info16Bit_s *p, uint16_t info)
{
    p->data = info & 0xff;
    p->opcode = (info >> 8) & 0xff;
    //p->seqnum = (info >> 12) & 0x7;
    //p->inout = (info >> 15) & 0x1;

    return info;
}

inline uint16_t pkg_info(struct info16Bit_s *p)
{
    uint16_t info = 0;
    info |= p->data & 0xff;
    info |= (p->opcode & 0xff) << 8;
    //info |= (p->seqnum & 0x7) << 12;
    //info |= (p->inout & 0x1) << 15;

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

static int next_spy(struct psdata_s *data)
{
    int pro, rlt, next = -1;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    struct info16Bit_s *t;
    t = &data->rs->pmch->tmp;

    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;
    evt = (data->result >> 8) & 0xff;
	
    //sprintf(str, "next %d-%d %d [0x%x]\n", pro, rlt, evt, data->result); 
    //print_f(mlogPool, "spy", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET: /* load file, prepare data and spi */
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "spy", str); 
                next = PSACT;
                break;
            case PSACT: /* check RDY, and start transmitting */
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "spy", str); 
                //next = PSWT;
                next = PSTSM; /* jump to next stage */
                evt = SPY;
                break;
            case PSWT: /* end the transmitting and back to polling OP_QRY */
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "spy", str); 
                next = PSRLT;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "spy", str); 
                next = PSTSM;
                break;
            case PSTSM:
                sprintf(str, "PSTSM - ansp: %d\n", tmpAns); 
                print_f(mlogPool, "spy", str); 
                //next = PSMAX;
                t->opcode = tmpAns & 0xff;
                t->data = (tmpAns >> 8) & 0xff;
                switch (tmpAns & 0xff) {
                case OP_SINGLE: /* currently support */              
                    switch ((tmpAns >> 8) & 0xff) {
                        case SINSCAN_DUAL_STRM:
                        case SINSCAN_DUAL_SD:
                            t->opcode = OP_SINGLE;
                            t->data = (tmpAns >> 8) & 0xff;
                            #if 1
                            next = PSSET; 
                            evt = BULLET;
                            #else
                            //next = PSRLT; /* for OP_SUPBACK */
                            //evt = AUTO_B;
                            #endif
                            break;
                        case SINSCAN_WIFI_ONLY:
                            t->opcode = OP_SINGLE;
                            t->data = SINSCAN_WIFI_ONLY;
                            next = PSTSM;
                            evt = AUTO_C;
                            break;
                        case SINSCAN_SD_ONLY:
                            t->opcode = OP_SINGLE;
                            t->data = SINSCAN_SD_ONLY;
                            next = PSTSM;
                            evt = AUTO_C;
                            break;
                        case SINSCAN_WIFI_SD:
                            t->opcode = OP_SINGLE;
                            t->data = SINSCAN_WIFI_SD;
                            next = PSTSM;
                            evt = AUTO_C;
                            break;
                         default:
                            break;
                    }
                    break;
                case OP_DOUBLE: 
                    switch ((tmpAns >> 8) & 0xff) {
                        case DOUSCAN_WIFI_ONLY:
                            next = PSACT;
                            evt = AUTO_D;
                            break;
                        default:
                            break;
                    }
                    break;
                case OP_SDRD:                                       
                case OP_SDWT:                                       
                    next = PSACT; /* get and repeat value */
                    evt = AUTO_A;
                    break;
                case OP_STSEC_00:                                  
                case OP_STSEC_01:                                  
                case OP_STSEC_02:                                  
                case OP_STSEC_03:                                  
                    if ((tmpAns >> 16) & 0xff) {
                        next = PSACT; 
                        evt = AUTO_E;
                    
                    } else {
                        next = PSWT; 
                        evt = AUTO_A;
                    }
                    break;
                case OP_STLEN_00:                                  
                case OP_STLEN_01:                                  
                case OP_STLEN_02:                                  
                case OP_STLEN_03:                                  
                    if ((tmpAns >> 16) & 0xff) {
                        next = PSWT; 
                        evt = AUTO_E;
                    } else {
                        next = PSRLT; /* get and repeat value */
                        evt = AUTO_A;
                    }
                    break;
                case OP_FFORMAT:                                  
                case OP_COLRMOD:                                  
                case OP_COMPRAT:                                  
                case OP_RESOLTN:                                  
                case OP_SCANGAV:                                  
                case OP_MAXWIDH:                                  
                case OP_WIDTHAD_H:                                
                case OP_WIDTHAD_L:                                
                case OP_SCANLEN_H:                                
                case OP_SCANLEN_L:
                case OP_INTERIMG:
                case OP_AFEIC:
                case OP_EXTPULSE:
                case OP_ACTION:
                case OP_SUPBACK:
                case OP_FUNCTEST_00:
                case OP_FUNCTEST_01:
                case OP_FUNCTEST_02:
                case OP_FUNCTEST_03:
                case OP_FUNCTEST_04:
                case OP_FUNCTEST_05:
                case OP_FUNCTEST_06:
                case OP_FUNCTEST_07:
                case OP_FUNCTEST_08:
                case OP_FUNCTEST_09:
                case OP_FUNCTEST_10:
                case OP_FUNCTEST_11:
                case OP_FUNCTEST_12:
                case OP_FUNCTEST_13:
                case OP_FUNCTEST_14:
                case OP_FUNCTEST_15:
                    next = PSSET; /* get and repeat value */
                    evt = AUTO_A;
                    break;
                case OP_SAVE:
                    next = PSRLT; 
                    evt = AUTO_D;
                    break;
                case OP_SDAT:                                     
                    next = PSSET;
                    evt = AUTO_B;
                    break;
                case OP_FREESEC:
                    next = PSTSM;
                    evt = AUTO_D;
                    break;
                case OP_USEDSEC:
                    next = PSSET;
                    evt = AUTO_E;
                    break;
                case OP_SDINIT:
                    next = PSRLT;
                    evt = AUTO_E;
                    break;
                case OP_SDSTATS:
                    next = PSTSM;
                    evt = AUTO_E;
                    break;
                case OP_RGRD:
                case OP_RGWT:
                case OP_RGADD_H:
                case OP_RGADD_L:
                    next = PSACT;
                    evt = AUTO_B;
                    break;
                case OP_RGDAT:
                    next = PSWT;
                    evt = AUTO_B;
                    break;
                case OP_CROP_01:
                    next = PSSET;
                    evt = AUTO_F;
                    break;
                case OP_CROP_02:
                    next = PSACT;
                    evt = AUTO_F;
                    break;
                case OP_CROP_03:
                    next = PSWT;
                    evt = AUTO_F;
                    break;
                case OP_CROP_04:
                    next = PSRLT;
                    evt = AUTO_F;
                    break;
                case OP_CROP_05:
                    next = PSTSM;
                    evt = AUTO_F;
                    break;
                case OP_CROP_06:
                    next = PSSET;
                    evt = AUTO_G;
                    break;
/*
                case OP_SUPBACK:
                    next = PSTSM;
                    evt = AUTO_B;
*/
                    break;
                default:
                    break;
                }
                break;
            default:
                //sprintf(str, "default\n"); 
                //print_f(mlogPool, "spy", str); 
                next = PSSET;
                break;
        }
    }

    if (next < 0) {
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);

        next = PSTSM; /* break */
        evt = SPY;
    }

    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static uint32_t next_bullet(struct psdata_s *data)
{
    int pro, rlt, next = -1;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;
    evt = (data->result >> 8) & 0xff;

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
            case PSSET: /* load file, prepare data and spi */
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSACT;
                break;
            case PSACT: /* check RDY, and start transmitting */
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSWT;
                break;
            case PSWT: /* end the transmitting and back to polling OP_QRY */
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
                //next = PSMAX;
                next = PSSET;
                /* jump to next stage */
                evt = LASER;
                break;
            default:
                sprintf(str, "default\n"); 
                print_f(mlogPool, "bullet", str); 
                next = PSSET;
                break;
        }
    }

    if (next < 0) {
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);

        next = PSTSM; /* break */
        evt = SPY;
    }

    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static int next_laser(struct psdata_s *data)
{
    int pro, rlt, next = -1;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;
    evt = (data->result >> 8) & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "laser", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET: /* load file, prepare data and spi */
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "laser", str); 
                next = PSACT;
                break;
            case PSACT: /* check RDY, and start transmitting */
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "laser", str); 
                next = PSWT;
                break;
            case PSWT: /* end the transmitting and back to polling OP_QRY */
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
                //next = PSMAX;
                next = PSTSM;
                /* jump to next stage */
                evt = SPY;
                break;
            default:
                sprintf(str, "default\n"); 
                print_f(mlogPool, "laser", str); 
                next = PSSET;
                break;
        }
    }

    if (next < 0) {
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);

        next = PSTSM; /* break */
        evt = SPY;
    }

    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static int next_auto_A(struct psdata_s *data)
{
    int pro, rlt, next = -1;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;
    evt = (data->result >> 8) & 0xff;

    //sprintf(str, "%d-%d, ans:0x%x\n", pro, rlt, data->ansp0); 
    //print_f(mlogPool, "auto_A", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET: /* load file, prepare data and spi */
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "auto_A", str); 
                next = PSTSM; /* jump to next stage */
                evt = SPY;
                break;
            case PSACT: /* check RDY, and start transmitting */
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "auto_A", str); 
                next = PSTSM;
                evt = SPY;
                break;
            case PSWT: /* end the transmitting and back to polling OP_QRY */
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "auto_A", str); 
                next = PSTSM;
                evt = SPY;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT - ans: 0x%x\n", tmpAns); 
                //print_f(mlogPool, "auto_A", str); 
                next = PSTSM;
                evt = SPY;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "auto_A", str); 
                next = PSSET;
                evt = AUTO_B;
                break;
            default:
                sprintf(str, "default\n"); 
                print_f(mlogPool, "laser", str); 
                next = PSSET;
                break;
        }
    }

    if (next < 0) {
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        next = PSTSM; /* break */
        evt = SPY;
    }

    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static int next_auto_B(struct psdata_s *data)
{
    int pro, rlt, next = -1;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;
    evt = (data->result >> 8) & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "auto_A", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET: /* load file, prepare data and spi */
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "auto_B", str); 
                break;
            case PSACT: /* check RDY, and start transmitting */
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "auto_B", str); 
                next = PSTSM;
                evt = SPY;
                break;
            case PSWT: /* end the transmitting and back to polling OP_QRY */
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "auto_B", str); 
                next = PSTSM;
                evt = SPY;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "auto_B", str); 
                next = PSACT;
                evt = BULLET;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "auto_B", str); 
                next = PSSET;
                evt = AUTO_C;
                break;
            default:
                sprintf(str, "default\n"); 
                print_f(mlogPool, "laser", str); 
                next = PSSET;
                break;
        }
    }

    if (next < 0) {
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);

        next = PSTSM; /* break */
        evt = SPY;
    }

    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static int next_auto_C(struct psdata_s *data)
{
    int pro, rlt, next = -1;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;
    evt = (data->result >> 8) & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "auto_A", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET: /* load file, prepare data and spi */
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "auto_C", str); 
                next = PSACT;
                break;
            case PSACT: /* check RDY, and start transmitting */
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "auto_C", str); 
                next = PSWT;
                break;
            case PSWT: /* end the transmitting and back to polling OP_QRY */
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "auto_C", str); 
                next = PSRLT;
                break;
            case PSRLT: /* LAST */
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "auto_C", str); 
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "auto_C", str); 
                next = PSSET; /* break */
                evt = AUTO_D;
                break;
            default:
                sprintf(str, "default\n"); 
                print_f(mlogPool, "laser", str); 
                next = PSSET;
                break;
        }
    }

    if (next < 0) {
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);

        next = PSTSM; /* break */
        evt = SPY;
    }

    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static int next_auto_D(struct psdata_s *data)
{
    int pro, rlt, next = -1;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;
    evt = (data->result >> 8) & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "auto_A", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET: /* load file, prepare data and spi */
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSWT;
                evt = AUTO_C;
                break;
            case PSACT: /* check RDY, and start transmitting */
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSWT;
                break;
            case PSWT: /* end the transmitting and back to polling OP_QRY */
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSWT;
                evt = AUTO_C;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            default:
                sprintf(str, "default\n"); 
                print_f(mlogPool, "laser", str); 
                next = PSSET;
                break;
        }
    }

    if (next < 0) {
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);

        next = PSTSM; /* break */
        evt = SPY;
    }

    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static int next_auto_E(struct psdata_s *data)
{
    int pro, rlt, next = -1;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;
    evt = (data->result >> 8) & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "auto_A", str); 

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
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSACT: 
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSWT: 
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                switch ((tmpAns >> 8) & 0xff) {
                    case 0x01:
                        next = PSTSM; 
                        evt = AUTO_D;
                        break;
                    case 0x02:
                        next = PSSET; 
                        evt = AUTO_E;
                        break;
                    default:
                        next = PSTSM; 
                        evt = SPY;
                        break;
                }
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            default:
                sprintf(str, "default\n"); 
                print_f(mlogPool, "laser", str); 
                next = PSSET;
                break;
        }
    }

    if (next < 0) {
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);

        next = PSTSM; /* break */
        evt = SPY;
    }

    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static int next_auto_F(struct psdata_s *data)
{
    int pro, rlt, next = -1;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;
    evt = (data->result >> 8) & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "auto_A", str); 

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
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSACT: 
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSWT: 
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            default:
                sprintf(str, "default\n"); 
                print_f(mlogPool, "laser", str); 
                next = PSSET;
                break;
        }
    }

    if (next < 0) {
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);

        next = PSTSM; /* break */
        evt = SPY;
    }

    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static int next_auto_G(struct psdata_s *data)
{
    int pro, rlt, next = -1;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;
    evt = (data->result >> 8) & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "auto_A", str); 

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
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSACT: 
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSWT: 
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            default:
                sprintf(str, "default\n"); 
                print_f(mlogPool, "laser", str); 
                next = PSSET;
                break;
        }
    }

    if (next < 0) {
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);

        next = PSTSM; /* break */
        evt = SPY;
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
    //print_f(mlogPool, "psnext", str); 

    switch (sta) {
        case SPY:
            ret = next_spy(data);
            evt = (ret >> 24) & 0xff;
            nxtst = evt; /* state change */

            break;
        case BULLET:
            ret = next_bullet(data);
            evt = (ret >> 24) & 0xff;
            nxtst = evt; /* state change */

            break;
        case LASER:
            ret = next_laser(data);
            evt = (ret >> 24) & 0xff;
            nxtst = evt; /* end the test loop */

            break;
        case AUTO_A:
            ret = next_auto_A(data);
            evt = (ret >> 24) & 0xff;
            nxtst = evt; /* end the test loop */

            break;
        case AUTO_B:
            ret = next_auto_B(data);
            evt = (ret >> 24) & 0xff;
            nxtst = evt; /* end the test loop */

            break;
        case AUTO_C:
            ret = next_auto_C(data);
            evt = (ret >> 24) & 0xff;
            nxtst = evt; /* end the test loop */

            break;
        case AUTO_D:
            ret = next_auto_D(data);
            evt = (ret >> 24) & 0xff;
            nxtst = evt; /* end the test loop */

            break;
        case AUTO_E:
            ret = next_auto_E(data);
            evt = (ret >> 24) & 0xff;
            nxtst = evt; /* end the test loop */

            break;
        case AUTO_F:
            ret = next_auto_F(data);
            evt = (ret >> 24) & 0xff;
            nxtst = evt; /* end the test loop */

            break;
        case AUTO_G:
            ret = next_auto_G(data);
            evt = (ret >> 24) & 0xff;
            nxtst = evt; /* end the test loop */

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
            ch = 7; 
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
    struct info16Bit_s *p;
    struct SdAddrs_s *m, *s;
    
    rlt = abs_result(data->result);	

    s = &data->rs->pmch->sdst;
    m = &data->rs->pmch->sdln;
    
    //sprintf(str, "op_05 - rlt:0x%.8x \n", rlt); 
    //print_f(mlogPool, "spy", str); 

    switch (rlt) {
        case STINIT:
            p = &data->rs->pmch->get;
            memset(p, 0, sizeof(struct info16Bit_s));

            ch = 9; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(str, "op_05: result: %x\n", data->result); 
            //print_f(mlogPool, "spy", str);  
            break;
        case WAIT:
            p = &data->rs->pmch->get;
            sprintf(str, "05: asnp: 0x%x, get pkt: 0x%x 0x%x\n", data->ansp0, p->opcode, p->data); 
            print_f(mlogPool, "spy", str);  

            if ((data->ansp0 & 0xff) == p->opcode) {
                data->ansp0 |= (p->data << 8);
                data->ansp0 |= ((m->f & 0xff)<< 16);
                
                switch (data->ansp0 & 0xff) {
                    case OP_SINGLE: /* currently support */
                    case OP_SDRD:
                    case OP_SDWT:
                    case OP_STSEC_00:
                    case OP_STSEC_01:
                    case OP_STSEC_02:
                    case OP_STSEC_03:
                    case OP_STLEN_00:
                    case OP_STLEN_01:
                    case OP_STLEN_02:
                    case OP_STLEN_03:
                    case OP_FFORMAT:
                    case OP_COLRMOD:
                    case OP_COMPRAT:
                    case OP_RESOLTN:
                    case OP_SCANGAV:
                    case OP_MAXWIDH:
                    case OP_WIDTHAD_H:
                    case OP_WIDTHAD_L:
                    case OP_SCANLEN_H:
                    case OP_SCANLEN_L:
                    case OP_INTERIMG:
                    case OP_AFEIC:
                    case OP_EXTPULSE:
                    case OP_SDAT:
                    case OP_FREESEC:
                    case OP_USEDSEC:
                    case OP_SDINIT:
                    case OP_SDSTATS:
                    case OP_ACTION:
                    case OP_RGRD:
                    case OP_RGWT:
                    case OP_RGDAT:
                    case OP_RGADD_H:
                    case OP_RGADD_L:

                    case OP_CROP_01:
                    case OP_CROP_02:
                    case OP_CROP_03:
                    case OP_CROP_04:
                    case OP_CROP_05:
                    case OP_CROP_06:
                    
                    case OP_SUPBACK:
                    case OP_DOUBLE:
                    case OP_SAVE:
                    case OP_FUNCTEST_00:
                    case OP_FUNCTEST_01:
                    case OP_FUNCTEST_02:
                    case OP_FUNCTEST_03:
                    case OP_FUNCTEST_04:
                    case OP_FUNCTEST_05:
                    case OP_FUNCTEST_06:
                    case OP_FUNCTEST_07:
                    case OP_FUNCTEST_08:
                    case OP_FUNCTEST_09:
                    case OP_FUNCTEST_10:
                    case OP_FUNCTEST_11:
                    case OP_FUNCTEST_12:
                    case OP_FUNCTEST_13:
                    case OP_FUNCTEST_14:
                    case OP_FUNCTEST_15:

                        sprintf(str, "go to next \n"); 
                        print_f(mlogPool, "spy", str);  
                        data->result = emb_result(data->result, NEXT);
                        break;
                    default:
                        break;
                }
            }
            break;
        case NEXT:
            break;
        case BREAK:
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
    rs = data->rs;
    p = &rs->pmch->get;
    rlt = abs_result(data->result);	
    //sprintf(str, "op_01: rlt: %.8x result: %.8x ans:%d\n", rlt, data->result, data->ansp0); 
    //print_f(mlogPool, "bullet", str);  

    switch (rlt) {
        case STINIT:
            ch = 11; 
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
            ch = 13; 
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
            ch = 5; 
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
            }
            break;
        case NEXT:
            break;
        case BREAK:
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
            ch = 19; 
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
            break;
        default:
            break;
    }
    return ps_next(data);

}

static int stauto_01(struct psdata_s *data)
{ 
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;

    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_01", str);  

    switch (rlt) {
        case STINIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));
            p->opcode = g->opcode;
            p->data = g->data;

            ch = 24; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if ((data->ansp0 & 0xff) == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_01", str);  
                data->result = emb_result(data->result, NEXT);
            }			
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);

}

static int stauto_02(struct psdata_s *data)
{ 
    char str[128], ch = 0;
    uint32_t rlt;
    uint8_t op;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *s, *m;

    FILE *fp=0;

    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_02", str);  

    switch (rlt) {
        case STINIT:
            s = &data->rs->pmch->sdst;
            m = &data->rs->pmch->sdln;
            //memset(s, 0, sizeof(struct SdAddrs_s));
            //memset(m, 0, sizeof(struct SdAddrs_s));

            ch = 36; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);

            break;
        case WAIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;

            if ((data->ansp0 & 0xff) == g->opcode) {
                if ((data->ansp0 & 0xff) == p->opcode) {
                    sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                    print_f(mlogPool, "auto_02", str);  
                    data->result = emb_result(data->result, NEXT);
                } else {
                    sprintf(str, "ERROR!!! ansp:0x%.2x, g:0x%.2x s:0x%.2x, break!!!\n", data->ansp0, g->opcode, p->opcode);  
                    print_f(mlogPool, "auto_02", str);  
                    data->result = emb_result(data->result, BREAK);
                }		
            } else {
                    sprintf(str, "WAIT!!! ansp:0x%.2x, g:0x%.2x s:0x%.2x!!!\n", data->ansp0, g->opcode, p->opcode);  
                    print_f(mlogPool, "auto_02", str);  
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);

}

static int stauto_03(struct psdata_s *data)
{ 
    char str[128], ch=0, ix=0;
    uint8_t op=0, dt=0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *s, *m;
	
    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_03", str);  

    switch (rlt) {
        case STINIT:
            s = &data->rs->pmch->sdst;
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));

            op = g->opcode;

            switch (op) {
            case OP_STSEC_00:
            case OP_STSEC_01:
            case OP_STSEC_02:
            case OP_STSEC_03:
                ix = (OP_STSEC_03 - op) & 0xf;
                s->d[ix] = g->data;
                s->f &= ~(0x1 << ix);

                sprintf(str, "!! flag: 0x%.2x data: 0x%.2x !!- 1\n", s->f, s->d[ix]);  
                print_f(mlogPool, "auto_03", str);  

                p->opcode = g->opcode;
                p->data = g->data;

                sprintf(str, "!! flag: 0x%.2x data: 0x%.2x !!- 2\n", s->f, s->d[ix]);  
                print_f(mlogPool, "auto_03", str);  

                ch = 24; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);

                memset(g, 0, sizeof(struct info16Bit_s));			
                break;
            default:
                sprintf(str, "ERROR!!! get pkt: 0x%.2x 0x%.2x break!!\n", g->opcode, g->data);  
                print_f(mlogPool, "auto_03", str);  
                data->result = emb_result(data->result, NEXT);
                break;
            }
            break;
        case WAIT:
            g = &data->rs->pmch->get;
            op = g->opcode;
            switch (op) {
            case OP_STSEC_00:
            case OP_STSEC_01:
            case OP_STSEC_02:
            case OP_STSEC_03:
                s = &data->rs->pmch->sdst;

                if ((data->ansp0 & 0xff) == op) {
                    ix = (OP_STSEC_03 - op) & 0xf;

                    if (s->d[ix] == g->data) {
                        s->f |= 0x1 << ix;
                    }

                    sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                    print_f(mlogPool, "auto_03", str);  
                    data->result = emb_result(data->result, NEXT);
                }
                break;
            case 0: /* may add counter to prevent dead lock */
            default:
                break;
            }

            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);

}
static int stauto_04(struct psdata_s *data)
{ 
    char str[128], ch=0, ix=0;
    uint8_t op=0, dt=0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *m;

    rlt = abs_result(data->result);
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_04", str);  

    switch (rlt) {
        case STINIT:
            m = &data->rs->pmch->sdln;
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));

            op = g->opcode;

            switch (op) {
            case OP_STLEN_00:
            case OP_STLEN_01:
            case OP_STLEN_02:
            case OP_STLEN_03:
                ix = (OP_STLEN_03 - op) & 0xf;
                m->d[ix] = g->data;
                m->f &= ~(0x1 << ix);

                sprintf(str, "!! flag: 0x%.2x data: 0x%.2x !! - 1\n", m->f, p->data);  
                print_f(mlogPool, "auto_04", str);  


                p->opcode = g->opcode;
                p->data = g->data;


                sprintf(str, "!! flag: 0x%.2x data: 0x%.2x !! - 2\n", m->f, p->data);  
                print_f(mlogPool, "auto_04", str);  

                ch = 24; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);

                memset(g, 0, sizeof(struct info16Bit_s));			
                break;
            default:
                sprintf(str, "ERROR!!! get pkt: 0x%.2x 0x%.2x break!!\n", g->opcode, g->data);  
                print_f(mlogPool, "auto_04", str);  
                data->result = emb_result(data->result, NEXT);
                break;
            }
            break;
        case WAIT:
            g = &data->rs->pmch->get;
            op = g->opcode;
            switch (op) {
            case OP_STLEN_00:
            case OP_STLEN_01:
            case OP_STLEN_02:
            case OP_STLEN_03:
                m = &data->rs->pmch->sdln;

                if ((data->ansp0 & 0xff) == op) {
                    ix = (OP_STLEN_03 - op) & 0xf;

                    if (m->d[ix] == g->data) {
                        m->f |= 0x1 << ix;
                    }

                    sprintf(str, "m->f: 0x%x, ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", m->f, data->ansp0, g->opcode, g->data);  
                    print_f(mlogPool, "auto_04", str);  
                    data->result = emb_result(data->result, NEXT);

                }
                break;
            case 0: /* may add counter to prevent dead lock */
            default:
                break;
            }

            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);

}
static int stauto_05(struct psdata_s *data)
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *s, *m;
    FILE *fp;

    s = &data->rs->pmch->sdst;
    m = &data->rs->pmch->sdln;
    g = &data->rs->pmch->get;
    p = &data->rs->pmch->cur;

    rlt = abs_result(data->result);
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_05", str);  

    switch (rlt) {
        case STINIT:
            //fp = data->rs->pmch->fdsk.vsd;

            memset(p, 0, sizeof(struct info16Bit_s));

            if ((s->f == 0xf) && (m->f == 0xf)) {
                p->opcode = g->opcode;
                p->data = g->data;

                sprintf(str, "get start sector: %d length: %d , send OP_SDAT confirm !!!\n", s->n, m->n);  
                print_f(mlogPool, "auto_05", str);  

                ch = 24; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);

            } else {
                sprintf(str, "ERROR!!! get pkt: 0x%.2x 0x%.2x, go to next!!\n", g->opcode, g->data);  
                print_f(mlogPool, "auto_05", str);  
                data->result = emb_result(data->result, NEXT);
            }

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if ((g->opcode) && ((data->ansp0 & 0xff) == g->opcode)) {
                s->f = 0;
                m->f = 0;
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_05", str);  
                data->result = emb_result(data->result, NEXT);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_06(struct psdata_s *data)
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct DiskFile_s *pf;
    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_06", str);  

    switch (rlt) {
        case STINIT:
            ch = 26; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
                pf = &data->rs->pmch->fdsk;
                pf->rtlen = 0;
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, BREAK);
                pf = &data->rs->pmch->fdsk;
                pf->rtlen = 0;
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_07(struct psdata_s *data)
{
    char ch = 0;
    uint32_t rlt;
    struct DiskFile_s *pf;
    struct procRes_s *rs;
    struct info16Bit_s *c, *g;
    struct RegisterRW_s *preg;

    rs = data->rs;
    rlt = abs_result(data->result);	

    g = &rs->pmch->get;
    c = &rs->pmch->cur;
    preg = &rs->pmch->regp;

    //sprintf(rs->logs, "op07 result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(rs->plogs, "reg", rs->logs);  

    switch (rlt) {
        case STINIT:
            memset(c, 0, sizeof(struct info16Bit_s));
            c->opcode = g->opcode;
            c->data = g->data;

            ch = 38; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                if (g->opcode == OP_RGRD) {
                    rs->pmch->regp.rgact = g->opcode;
                    sprintf(rs->logs, "OP_RGRD 16bit addr:0x%.8x, go to next!!\n", preg->rga16);  
                    print_f(rs->plogs, "reg", rs->logs);  
                    data->result = emb_result(data->result, NEXT);
                } else if (g->opcode == OP_RGWT) {
                    rs->pmch->regp.rgact = g->opcode;
                    sprintf(rs->logs, "OP_RGWT 16bit addr:0x%.8x, go to next!!\n", preg->rga16);  
                    print_f(rs->plogs, "reg", rs->logs);  
                    data->result = emb_result(data->result, NEXT);
                } else if (g->opcode == OP_RGADD_H) {
                    preg->rga16 &= 0xffff00ff;
                    preg->rga16 |= g->data << 8;
                    sprintf(rs->logs, "OP_RGADD_H 16bit addr:0x%.8x, go to next!!\n", preg->rga16);  
                    print_f(rs->plogs, "reg", rs->logs);  
                    data->result = emb_result(data->result, NEXT);
                } else if (g->opcode == OP_RGADD_L) {
                    preg->rga16 &= 0xffffff00;
                    preg->rga16 |= g->data;
                    sprintf(rs->logs, "OP_RGADD_L 16bit addr:0x%.8x, go to next!!\n", preg->rga16);  
                    print_f(rs->plogs, "reg", rs->logs);  
                    data->result = emb_result(data->result, NEXT);
                } else {
                    sprintf(rs->logs, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                    print_f(rs->plogs, "reg", rs->logs);  
                    data->result = emb_result(data->result, BREAK);
                }
            } else if (data->ansp0 == 2) { 
                sprintf(rs->logs, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, break!!\n", data->ansp0, g->opcode, g->data);  
                print_f(rs->plogs, "reg", rs->logs);  
                data->result = emb_result(data->result, BREAK);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_08(struct psdata_s *data)
{
    int i=0, idx=-1, avl=0;
    char ch = 0;
    uint32_t rlt;
    struct DiskFile_s *pf;
    struct procRes_s *rs;
    struct info16Bit_s *c, *g;
    struct RegisterRW_s *preg;
    struct virtualReg_s *ptb;

    rs = data->rs;
    rlt = abs_result(data->result);	

    g = &rs->pmch->get;
    c = &rs->pmch->cur;
    preg = &rs->pmch->regp;
    ptb = rs->pregtb;

    //sprintf(rs->logs, "op08 result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(rs->plogs, "reg", rs->logs);  

    switch (rlt) {
        case STINIT:
           memset(c, 0, sizeof(struct info16Bit_s));
           c->opcode = g->opcode;

           idx = -1; avl = -1;
           if (preg->rgact == OP_RGRD) {
                for (i = 0; i < 128; i++) {
                    if (avl < 0) {
                        if (ptb[i].vrAddr == 0) {
                            avl = i;
                        }
                    }
                    if (ptb[i].vrAddr == preg->rga16) {
                        idx = i; 
                        break;
                    }
                }

                if (idx < 0) {
                    preg->rgidx = avl;
                    c->data = 0;
                } else {
                    preg->rgidx = idx;
                    c->data = ptb[idx].vrValue;
                }
                sprintf(rs->logs, "OP_RGRD rgact 0x%x, get reg data: 0x%x, idx:%d, avl:%d !!\n", preg->rgact, c->data, idx, avl);  
                print_f(rs->plogs, "reg", rs->logs);  

                ch = 38; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);

                memset(g, 0, sizeof(struct info16Bit_s));
            } else if (preg->rgact == OP_RGWT) {
                c->data = g->data;
                ch = 38; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);

                memset(g, 0, sizeof(struct info16Bit_s));
            } else {
                sprintf(rs->logs, "unknown rgact 0x%x, get pkt: 0x%.2x 0x%.2x, break!!\n", preg->rgact, g->opcode, g->data);  
                print_f(rs->plogs, "reg", rs->logs);  

                data->result = emb_result(data->result, BREAK);
            }
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                if (preg->rgact == OP_RGWT) {
                    idx = -1; avl = -1;
                    for (i = 0; i < 128; i++) {
                        if (avl < 0) {
                            if (ptb[i].vrAddr == 0) {
                                avl = i;
                            }
                        }

                        if (ptb[i].vrAddr == preg->rga16) {
                            idx = i; 
                            break;
                        }
                    }

                    if (idx < 0) {
                        preg->rgidx = avl;
                        ptb[avl].vrValue = g->data;
                        ptb[avl].vrAddr = preg->rga16;
                    } else {
                        preg->rgidx = idx;
                        ptb[idx].vrValue = g->data;
                    }
                }

                sprintf(rs->logs, "get op: 0x%.2x/0x%.2x, idx:%d, avl:%d, go to next!!\n", g->opcode, g->data, idx, avl);  
                print_f(rs->plogs, "reg", rs->logs);  
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) { 
                sprintf(rs->logs, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, break!!\n", data->ansp0, g->opcode, g->data);  
                print_f(rs->plogs, "reg", rs->logs);  
                data->result = emb_result(data->result, BREAK);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_09(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct procRes_s *rs;
    struct info16Bit_s *c, *g;    

    rs = data->rs;
    g = &rs->pmch->get;
    c = &rs->pmch->cur;

    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_09", str);  

    switch (rlt) {
        case STINIT:
            ch = 40; 

            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, BREAK);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}
static int stauto_10(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct procRes_s *rs;
    struct info16Bit_s *c, *g;    

    rs = data->rs;
    g = &rs->pmch->get;
    c = &rs->pmch->cur;

    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_10", str);  

    switch (rlt) {
        case STINIT:
            if (c->info) {
                ch = 42; 
                g->info = 0;
            } else {
                ch = 17; 
            }

            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, BREAK);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}
static int stauto_11(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    
    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_11", str);  

    switch (rlt) {
        case STINIT:
            ch = 44; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, BREAK);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_12(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct procRes_s *rs;
    struct info16Bit_s *c, *g;    

    rs = data->rs;
    g = &rs->pmch->get;
    c = &rs->pmch->cur;

    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_12", str);  

    switch (rlt) {
        case STINIT:
            if (g->info) {
                if (c->info == g->info) {
                    c->info = 0;
                    g->info = 0;
                }
            }
            ch = 20; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, BREAK);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_13(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    
    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_13", str);  

    switch (rlt) {
        case STINIT:
            ch = 17; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, BREAK);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_14(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    
    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_14", str);  

    switch (rlt) {
        case STINIT:
            ch = 7; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, BREAK);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_15(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *t;
    t = &data->rs->pmch->tmp;

    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_15", str);  

    switch (rlt) {
        case STINIT:
            ch = 48; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, BREAK);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}
static int stauto_16(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    
    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_16", str);  

    switch (rlt) {
        case STINIT:
            ch = 50; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, BREAK);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_17(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    
    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_17", str);  

    switch (rlt) {
        case STINIT:
            ch = 53; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, BREAK);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_18(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    
    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_18", str);  

    switch (rlt) {
        case STINIT:
            ch = 55; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, BREAK);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_19(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;

    rlt = abs_result(data->result);	
    sprintf(str, "op_19 result: %.8x ansp:%d\n", data->result, data->ansp0);  
    print_f(mlogPool, "save", str);  

    switch (rlt) {
        case STINIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));
            p->opcode = g->opcode;
            p->data = g->data;

            ch = 58; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if (data->ansp0 == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_01", str);  
                data->result = emb_result(data->result, NEXT);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);

}

static int stauto_20(struct psdata_s *data)
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *s, *m;
    FILE *fp;

    s = &data->rs->pmch->sdst;
    m = &data->rs->pmch->sdln;
    g = &data->rs->pmch->get;
    p = &data->rs->pmch->cur;

    rlt = abs_result(data->result);
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_20", str);  

    switch (rlt) {
        case STINIT:
            //fp = data->rs->pmch->fdsk.vsd;
            memset(p, 0, sizeof(struct info16Bit_s));

            if (s->f & 0x100) {
                s->sda = s->n;
                s->f &= ~(0x100);
            } else {
                s->f = 0x100;
            }

            if (m->f & 0x100) {
                m->sda = m->n;
                m->f &= ~(0x100);
            } else {
                m->f = 0x100;
            }

            if ((m->f & 0x100) || (s->f & 0x100)) {
                p->opcode = g->opcode;
                p->data = g->data;           

                ch = 24; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            } else {
                data->result = emb_result(data->result, BREAK);

                sprintf(str, "get start sector: %d length: %d DONE !!!\n", s->sda, m->sda);  
                print_f(mlogPool, "auto_20", str);  
            }
                
            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:

            if ((data->ansp0 & 0xff) == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_20", str);  
                data->result = emb_result(data->result, NEXT);
            }
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_21(struct psdata_s *data)
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *s, *m;
    FILE *fp;

    rlt = abs_result(data->result);
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_05", str);  

    switch (rlt) {
        case STINIT:
            //fp = data->rs->pmch->fdsk.vsd;
            s = &data->rs->pmch->sdst;
            m = &data->rs->pmch->sdln;
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            //memset(p, 0, sizeof(struct info16Bit_s));

            if (m->f & 0x200) {
                s->f = 0;
                m->f = 0;
                
                sprintf(str, "send start sector: %d/%d, data length: %d/%d confirm !!!\n", s->n, s->sda, m->n, m->sda);  
                print_f(mlogPool, "auto_21", str);  
                data->result = emb_result(data->result, BREAK);
            
            } else {
                if ((s->sda) && (m->sda)) {
                    p->opcode = g->opcode;
                    p->data = g->data;

                    sprintf(str, "get start sector: %d, total length: %d , secPrClst: %d, data length: %d confirm !!!\n", s->n, m->sda, g->data, m->n);  
                    print_f(mlogPool, "auto_21", str);  

                    ch = 59; 
                    rs_ipc_put(data->rs, &ch, 1);
                    data->result = emb_result(data->result, WAIT);

                } else {
                    s->f = 0;
                    m->f = 0;

                    sprintf(str, "ERROR!!! get start sector: %d, total length: %d , flag: 0x%x, 0x%x !!!\n", s->sda, m->sda, s->f, m->f);  
                    print_f(mlogPool, "auto_21", str);  
                    data->result = emb_result(data->result, BREAK);
                }
            }

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if ((data->ansp0 & 0xff) == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_05", str);  
                data->result = emb_result(data->result, NEXT);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_22(struct psdata_s *data)
{ 
    char str[128], ch=0, ix=0;
    uint8_t op=0, dt=0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *s, *m;
	
    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_22", str);  

    switch (rlt) {
        case STINIT:
            s = &data->rs->pmch->sdst;
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));

            op = g->opcode;

            switch (op) {
            case OP_STSEC_00:
            case OP_STSEC_01:
            case OP_STSEC_02:
            case OP_STSEC_03:
                ix = (OP_STSEC_03 - op) & 0xf;
                //s->d[ix] = g->data;
                //s->f &= ~(0x1 << ix);

                sprintf(str, "!! flag: 0x%.2x data: 0x%.2x !!- 1\n", s->f, s->d[ix]);  
                print_f(mlogPool, "auto_22", str);  

                if (s->f & (0x1 << ix)) {
                    p->opcode = g->opcode;
                    p->data = s->d[ix];
                } else {
                    p->opcode = g->opcode;
                    p->data = g->data;
                    s->d[ix] = g->data;
                }
                
                sprintf(str, "!! flag: 0x%.2x data: 0x%.2x !!- 2\n", s->f, s->d[ix]);  
                print_f(mlogPool, "auto_22", str);  

                ch = 24; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);

                memset(g, 0, sizeof(struct info16Bit_s));			
                break;
            default:
                sprintf(str, "ERROR!!! get pkt: 0x%.2x 0x%.2x break!!\n", g->opcode, g->data);  
                print_f(mlogPool, "auto_22", str);  
                data->result = emb_result(data->result, NEXT);
                break;
            }
            break;
        case WAIT:
            g = &data->rs->pmch->get;
            op = g->opcode;
            switch (op) {
            case OP_STSEC_00:
            case OP_STSEC_01:
            case OP_STSEC_02:
            case OP_STSEC_03:
                s = &data->rs->pmch->sdst;

                if ((data->ansp0 & 0xff) == op) {
                    ix = (OP_STSEC_03 - op) & 0xf;
                    
                    if (s->f & (0x1 << ix)) {
                        s->f &= ~(0x1 << ix);                    
                    } 

                    sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                    print_f(mlogPool, "auto_22", str);  
                    data->result = emb_result(data->result, NEXT);
                }
                break;
            case 0: /* may add counter to prevent dead lock */
            default:
                break;
            }

            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);

}
static int stauto_23(struct psdata_s *data)
{ 
    char str[128], ch=0, ix=0;
    uint8_t op=0, dt=0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *m;

    rlt = abs_result(data->result);
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_23", str);  

    switch (rlt) {
        case STINIT:
            m = &data->rs->pmch->sdln;
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));

            op = g->opcode;

            switch (op) {
            case OP_STLEN_00:
            case OP_STLEN_01:
            case OP_STLEN_02:
            case OP_STLEN_03:
                ix = (OP_STLEN_03 - op) & 0xf;
                //m->d[ix] = g->data;
                //m->f &= ~(0x1 << ix);

                sprintf(str, "!! flag: 0x%.2x data: 0x%.2x !! - 1\n", m->f, p->data);  
                print_f(mlogPool, "auto_23", str);  

                if (m->f & (0x1 << ix)) {
                    p->opcode = g->opcode;
                    p->data = m->d[ix];
                } else {
                    p->opcode = g->opcode;
                    p->data = g->data;
                    m->d[ix] = g->data;
                }

                sprintf(str, "!! flag: 0x%.2x opcode: 0x%.2x, data: 0x%.2x !! - 2\n", m->f, p->opcode, p->data);  
                print_f(mlogPool, "auto_23", str);  

                ch = 24; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);

                memset(g, 0, sizeof(struct info16Bit_s));			
                break;
            default:
                sprintf(str, "ERROR!!! get pkt: 0x%.2x 0x%.2x break!!\n", g->opcode, g->data);  
                print_f(mlogPool, "auto_23", str);  
                data->result = emb_result(data->result, NEXT);
                break;
            }
            break;
        case WAIT:
            g = &data->rs->pmch->get;
            op = g->opcode;
            switch (op) {
            case OP_STLEN_00:
            case OP_STLEN_01:
            case OP_STLEN_02:
            case OP_STLEN_03:
                m = &data->rs->pmch->sdln;

                if ((data->ansp0 & 0xff) == op) {
                    ix = (OP_STLEN_03 - op) & 0xf;

                    if (m->f & (0x1 << ix)) {
                        m->f &= ~(0x1 << ix);
                    }

                    if (op == OP_STLEN_03) {
                        data->ansp0 |= ((m->f >> 8) << 8);
                    }

                    sprintf(str, "m->f: 0x%x, ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", m->f, data->ansp0, g->opcode, g->data);  
                    print_f(mlogPool, "auto_23", str);  
                    data->result = emb_result(data->result, NEXT);

                }
                break;
            case 0: /* may add counter to prevent dead lock */
            default:
                break;
            }

            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);

}

static int stauto_24(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g, *t;
    int *pSdInit = 0;

    rlt = abs_result(data->result);	
    sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    print_f(mlogPool, "auto_24", str);  

    switch (rlt) {
        case STINIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            t = &data->rs->pmch->tmp;
            memset(p, 0, sizeof(struct info16Bit_s));
            p->opcode = g->opcode;
            p->data = g->data;

            ch = 24; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            
            if (g->opcode == p->opcode) {
                sprintf(str, "ansp:0x%.2x, recv: 0x%.2x 0x%.2x, send: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data, p->opcode, p->data);  
                print_f(mlogPool, "auto_24", str);  

                sleep(2);
                pSdInit = data->rs->psd_init;
                *pSdInit = 1;
                sprintf(str, "Set SD status: 0x%.2x \n", *pSdInit);  
                print_f(mlogPool, "auto_24", str);  

                data->result = emb_result(data->result, NEXT);
            }	
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);


}

static int stauto_25(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    int *pSdInit = 0, SDinit = 0;
    
    rlt = abs_result(data->result);	
    sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    print_f(mlogPool, "auto_25", str);  

    switch (rlt) {
        case STINIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));

            pSdInit = data->rs->psd_init;
            SDinit = *pSdInit;
            msync(pSdInit, sizeof(int), MS_SYNC);
            
            p->opcode = OP_SDSTATS;
            p->data = SDinit & 0xff;

            sprintf(str, "SD status: 0x%.2x \n", p->data);  
            print_f(mlogPool, "auto_25", str);  

            ch = 24; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            
            if (g->opcode == p->opcode) {
                sprintf(str, "ansp:0x%.2x, recv: 0x%.2x 0x%.2x, send: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data, p->opcode, p->data);  
                print_f(mlogPool, "auto_25", str);  
                data->result = emb_result(data->result, NEXT);
            }	
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_26(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *s, *m;

    rlt = abs_result(data->result);
    
    switch (rlt) {
        case STINIT:
            s = &data->rs->pmch->sdst;
            m = &data->rs->pmch->sdln;
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            //memset(p, 0, sizeof(struct info16Bit_s));

            p->opcode = OP_CROP_01;
            p->data = 0xff;
            
            ch = 60; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(str, "go %d\n", ch);  
            print_f(mlogPool, "auto_26", str);  

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if ((data->ansp0 & 0xff) == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_26", str);  
                data->result = emb_result(data->result, NEXT);
            }
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_27(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *s, *m;

    rlt = abs_result(data->result);
    
    switch (rlt) {
        case STINIT:
            s = &data->rs->pmch->sdst;
            m = &data->rs->pmch->sdln;
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            //memset(p, 0, sizeof(struct info16Bit_s));

            p->opcode = OP_CROP_02;
            p->data = 0xff;

            ch = 61; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(str, "go %d\n", ch);  
            print_f(mlogPool, "auto_27", str);  

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if ((data->ansp0 & 0xff) == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_27", str);  
                data->result = emb_result(data->result, NEXT);
            }
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_28(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *s, *m;

    rlt = abs_result(data->result);
    
    switch (rlt) {
        case STINIT:
            s = &data->rs->pmch->sdst;
            m = &data->rs->pmch->sdln;
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            //memset(p, 0, sizeof(struct info16Bit_s));

            p->opcode = OP_CROP_03;
            p->data = 0xff;

            ch = 62; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(str, "go %d\n", ch);  
            print_f(mlogPool, "auto_28", str);  

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if ((data->ansp0 & 0xff) == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_28", str);  
                data->result = emb_result(data->result, NEXT);
            }
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_29(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *s, *m;

    rlt = abs_result(data->result);
    
    switch (rlt) {
        case STINIT:
            s = &data->rs->pmch->sdst;
            m = &data->rs->pmch->sdln;
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            //memset(p, 0, sizeof(struct info16Bit_s));

            p->opcode = OP_CROP_04;
            p->data = 0xff;

            ch = 63; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(str, "go %d\n", ch);  
            print_f(mlogPool, "auto_29", str);  

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if ((data->ansp0 & 0xff) == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_29", str);  
                data->result = emb_result(data->result, NEXT);
            }
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_30(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *s, *m;

    rlt = abs_result(data->result);
    
    switch (rlt) {
        case STINIT:
            s = &data->rs->pmch->sdst;
            m = &data->rs->pmch->sdln;
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            //memset(p, 0, sizeof(struct info16Bit_s));

            p->opcode = OP_CROP_05;
            p->data = 0xff;

            ch = 64; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(str, "go %d\n", ch);  
            print_f(mlogPool, "auto_30", str);  

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if ((data->ansp0 & 0xff) == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_30", str);  
                data->result = emb_result(data->result, NEXT);
            }
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_31(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *s, *m;

    rlt = abs_result(data->result);
    
    switch (rlt) {
        case STINIT:
            s = &data->rs->pmch->sdst;
            m = &data->rs->pmch->sdln;
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            //memset(p, 0, sizeof(struct info16Bit_s));

            p->opcode = OP_CROP_06;
            p->data = 0xff;

            ch = 65; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(str, "go %d\n", ch);  
            print_f(mlogPool, "auto_31", str);  

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if ((data->ansp0 & 0xff) == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_31", str);  
                data->result = emb_result(data->result, NEXT);
            }
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_32(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    int *pSdInit = 0, SDinit = 0;
    
    rlt = abs_result(data->result);	
    sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    print_f(mlogPool, "auto_32", str);  

    switch (rlt) {
        case STINIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));

            pSdInit = data->rs->psd_init;
            SDinit = *pSdInit;
            msync(pSdInit, sizeof(int), MS_SYNC);
            
            p->opcode = 0;
            p->data = 0;

            ch = 24; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            
            if (g->opcode == p->opcode) {
                sprintf(str, "ansp:0x%.2x, recv: 0x%.2x 0x%.2x, send: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data, p->opcode, p->data);  
                print_f(mlogPool, "auto_32", str);  
                data->result = emb_result(data->result, NEXT);
            }	
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_33(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    int *pSdInit = 0, SDinit = 0;
    
    rlt = abs_result(data->result);	
    sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    print_f(mlogPool, "auto_33", str);  

    switch (rlt) {
        case STINIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));

            pSdInit = data->rs->psd_init;
            SDinit = *pSdInit;
            msync(pSdInit, sizeof(int), MS_SYNC);
            
            p->opcode = 0;
            p->data = 0;

            ch = 24; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            
            if (g->opcode == p->opcode) {
                sprintf(str, "ansp:0x%.2x, recv: 0x%.2x 0x%.2x, send: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data, p->opcode, p->data);  
                print_f(mlogPool, "auto_33", str);  
                data->result = emb_result(data->result, NEXT);
            }	
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_34(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    int *pSdInit = 0, SDinit = 0;
    
    rlt = abs_result(data->result);	
    sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    print_f(mlogPool, "auto_34", str);  

    switch (rlt) {
        case STINIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));

            pSdInit = data->rs->psd_init;
            SDinit = *pSdInit;
            msync(pSdInit, sizeof(int), MS_SYNC);
            
            p->opcode = 0;
            p->data = 0;

            ch = 24; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            
            if (g->opcode == p->opcode) {
                sprintf(str, "ansp:0x%.2x, recv: 0x%.2x 0x%.2x, send: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data, p->opcode, p->data);  
                print_f(mlogPool, "auto_34", str);  
                data->result = emb_result(data->result, NEXT);
            }	
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_35(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    int *pSdInit = 0, SDinit = 0;
    
    rlt = abs_result(data->result);	
    sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    print_f(mlogPool, "auto_35", str);  

    switch (rlt) {
        case STINIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));

            pSdInit = data->rs->psd_init;
            SDinit = *pSdInit;
            msync(pSdInit, sizeof(int), MS_SYNC);
            
            p->opcode = 0;
            p->data = 0;

            ch = 24; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            
            if (g->opcode == p->opcode) {
                sprintf(str, "ansp:0x%.2x, recv: 0x%.2x 0x%.2x, send: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data, p->opcode, p->data);  
                print_f(mlogPool, "auto_35", str);  
                data->result = emb_result(data->result, NEXT);
            }	
            
            break;
        case NEXT:
            break;
        case BREAK:
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
        //printf("[dback] ch:%c rt:%d idx:%d \n", ch,  len, seq);
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
    //printf("shmem pop:0x%.8x, seq:%d sz:%d\n", *addr, seq, sz);
    if (sz < 0) return (-1);
    sprintf(str, "d%.8xl%.8d\n", *addr, sz);
    print_f(&mrs->plog, "pop", str);
    //printf("[%s]\n", str);
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
        print_f(NULL, NULL, str);

        if (!((inc+1) % 16)) {
            sprintf(str, "\n");
            print_f(NULL, NULL, str);
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
    char str[128];
    sel = sel % 2;

    if (sel) {
        pp->dualsz = size;
    } else {
        pp->lastsz = size;
    }
    pp->lastflg += 1;

    //sprintf(str, "[last] d:%d l:%d flg:%d \n", pp->dualsz, pp->lastsz, pp->lastflg);
    //print_f(mlogPool, "ring", str);

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

    //sprintf(str, "cons d: %d %d/%d/%d \n", dist, leadn, dualn, folwn);
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

    //sprintf(str, "cons dual len:%d \n", ret);
    //print_f(mlogPool, "ring", str);

    msync(pp, sizeof(struct shmem_s), MS_SYNC);
    return ret;
}

static int ring_buf_cons(struct shmem_s *pp, char **addr, int *len)
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
    *len = 0;

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

            *len = pp->lastsz;
            return -2;
        }
    }

    msync(pp, sizeof(struct shmem_s), MS_SYNC);

    *len = pp->chksz;
    return 0;
}

static int mtx_data(int fd, uint8_t *rx_buff, uint8_t *tx_buff, int num, int pksz, int maxsz)
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
        tr[i].delay_usecs = 0;
        tr[i].speed_hz = SPI_SPEED;
        tr[i].bits_per_word = 8;
        
        tx += pkt_size;
        rx += pkt_size;
    }
    
    ret = ioctl(fd, SPI_IOC_MESSAGE(i), tr);
    if (ret < 0) {
        printf("can't send spi message, ret: %d\n", ret);
    }
    //printf("tx/rx len: %d\n", ret);
    
    free(tr);
    return ret;
}

static int mtx_data_16(int fd, uint16_t *rx_buff, uint16_t *tx_buff, int num, int pksz, int maxsz)
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
        tr[i].delay_usecs = 0;
        tr[i].speed_hz = SPI_SPEED;
        tr[i].bits_per_word = 16;
        
        tx += pkt_size;
        rx += pkt_size;
    }
    
    ret = ioctl(fd, SPI_IOC_MESSAGE(i), tr);
    if (ret < 0)
        printf("can't send spi message\n");
    
    //printf("tx/rx len: %d\n", ret);
    
    free(tr);
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

static int cmdfunc_01(int argc, char *argv[])
{
    struct mainRes_s *mrs;
    char str[256], ch;
    if (!argv) return -1;

    mrs = (struct mainRes_s *)argv[0];

    ch = '\0';

    if (argc == 0) {
        ch = 'p';
    }

    if (argc == 2) {
        ch = '3';
    }

    if (argc == 1) {
        ch = '4';
    }

    if (argc == 5) {
        ch = 's';
    }

    if (argc == 7) {
        ch = 'b';
    }

    sprintf(str, "cmdfunc_01 argc:%d ch:%c\n", argc, ch); 
    print_f(mlogPool, "cmdfunc", str);

    mrs_ipc_put(mrs, &ch, 1, 6);
    return 1;
}

static int dbg(struct mainRes_s *mrs)
{
    int ci, pi, ret, idle = 0;
    char cmd[256], *addr[3];
    char poll[32] = "poll";

    struct cmd_s cmdtab[8] = {{0, "poll", cmdfunc_01}, {1, "command", cmdfunc_01}, {2, "data", cmdfunc_01}, {3, "run", cmdfunc_01}, 
                                {4, "aspect", cmdfunc_01}, {5, "leo", cmdfunc_01}, {6, "mothership", cmdfunc_01}, {7, "lanch", cmdfunc_01}};

    p0_init(mrs);
	
    while (1) {
        /* command parsing */
        ci = 0;    
        ci = mrs_ipc_get(mrs, cmd, 256, 5);

        if (ci > 0) {
            cmd[ci] = '\0';
            //sprintf(mrs->log, "get [%s] size:%d \n", cmd, ci);
            //print_f(&mrs->plog, "DBG", mrs->log);
        } else {
            if (idle > 5) {
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

        /* command execution */
        if (pi < 8) {
            addr[0] = (char *)mrs;
            sprintf(mrs->log, "input [%d]%s\n", pi, cmdtab[pi].str, cmdtab[pi].id, cmd);
            print_f(&mrs->plog, "DBG", mrs->log);
            ret = cmdtab[pi].pfunc(cmdtab[pi].id, addr);

            //mrs_ipc_put(mrs, "t", 1, 6);
        }

        cmd[0] = '\0';

        printf_flush(&mrs->plog, mrs->flog);
    }

    p0_end(mrs);
}

static int fs00(struct mainRes_s *mrs, struct modersp_s *modersp){ return 0; }
static int fs01(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    //print_f(&mrs->plog, "fs01", mrs->log);
    /* wait until control pin become 0 */

    mrs_ipc_put(mrs, "g", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}
static int fs02(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int len=0;
    char ch=0;
    //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    //print_f(&mrs->plog, "fs02", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'G')){
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
    int bitset = 0;
    struct info16Bit_s *p;

    //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    //print_f(&mrs->plog, "fs03", mrs->log);

    bitset = 1;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 0 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs03", mrs->log);
    bitset = 1;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 1 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs03", mrs->log);

    modersp->r = 1;
    return 1; 
}

static int fs04(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0, bitset=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        //print_f(&mrs->plog, "fs04", mrs->log);

        if (p->opcode == OP_PON) {
            bitset = 1;
            ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
            sprintf(mrs->log, "Set spi 0 slave ready: %d\n", bitset);
            print_f(&mrs->plog, "fs04", mrs->log);
            bitset = 1;
            ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
            sprintf(mrs->log, "Set spi 1 slave ready: %d\n", bitset);
            print_f(&mrs->plog, "fs04", mrs->log);

            modersp->r = 1;
            return 1;
        } else {
            modersp->m = modersp->m - 1;        
            return 2;
        }
    }
    return 0; 
}

static int fs05(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    //print_f(&mrs->plog, "fs05", mrs->log);
    /* wait until control pin become 1 */
	
    mrs_ipc_put(mrs, "b", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}
static int fs06(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int len=0;
    char ch=0;
    //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    //print_f(&mrs->plog, "fs06", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'B')){
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

static int fs07(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p;

    p = &mrs->mchine.cur;
    sprintf(mrs->log, "set 0x%.2x 0x%.2x \n", p->opcode, p->data);
    print_f(&mrs->plog, "fs07", mrs->log);

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = OP_RDY;
    p->inout = 0;
    p->seqnum = mrs->mchine.seqcnt;
	
    mrs_ipc_put(mrs, "c", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs08(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        print_f(&mrs->plog, "fs08", mrs->log);

        if (p->opcode == OP_RDY) {
            modersp->r = 1;
            return 1;
        } else {
            modersp->m = modersp->m - 1;        
            modersp->r = 2;
            return 2;
        }
    }
    return 0; 
}

static int fs09(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p;


    p = &mrs->mchine.cur;


    p->opcode = OP_QRY;
    p->data = 0;
	
    sprintf(mrs->log, "put op:0x%.2x data:0x%.2x \n", p->opcode, p->data);
    print_f(&mrs->plog, "fs09", mrs->log);

    mrs_ipc_put(mrs, "c", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs10(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);
        p = &mrs->mchine.get;
        sprintf(mrs->log, "get op:0x%.2x data:0x%.2x \n", p->opcode, p->data);
        print_f(&mrs->plog, "fs10", mrs->log);

        switch (p->opcode) {
        case OP_SINGLE: /* currently support */              
        case OP_DOUBLE: 
        case OP_SDRD:                                       
        case OP_SDWT:                                       
        case OP_SDAT:                                     
        case OP_FREESEC:
        case OP_USEDSEC:
        case OP_SDINIT:
        case OP_SDSTATS:
        case OP_STSEC_00:                                  
        case OP_STSEC_01:                                  
        case OP_STSEC_02:                                  
        case OP_STSEC_03:                                  
        case OP_STLEN_00:                                  
        case OP_STLEN_01:                                  
        case OP_STLEN_02:                                  
        case OP_STLEN_03:                                  
        case OP_FFORMAT:                                  
        case OP_COLRMOD:                                  
        case OP_COMPRAT:                                  
        case OP_RESOLTN:                                  
        case OP_SCANGAV:                                  
        case OP_MAXWIDH:                                  
        case OP_WIDTHAD_H:                                
        case OP_WIDTHAD_L:                                
        case OP_SCANLEN_H:                                
        case OP_SCANLEN_L:                              
        case OP_INTERIMG:
        case OP_AFEIC:
        case OP_EXTPULSE:
        case OP_ACTION:
        case OP_RGRD:
        case OP_RGWT:
        case OP_RGDAT:
        case OP_RGADD_H:
        case OP_RGADD_L:

        case OP_CROP_01:
        case OP_CROP_02:
        case OP_CROP_03:
        case OP_CROP_04:
        case OP_CROP_05:
        case OP_CROP_06:

        case OP_SUPBACK:
        case OP_SAVE:
        case OP_FUNCTEST_00:
        case OP_FUNCTEST_01:
        case OP_FUNCTEST_02:
        case OP_FUNCTEST_03:
        case OP_FUNCTEST_04:
        case OP_FUNCTEST_05:
        case OP_FUNCTEST_06:
        case OP_FUNCTEST_07:
        case OP_FUNCTEST_08:
        case OP_FUNCTEST_09:
        case OP_FUNCTEST_10:
        case OP_FUNCTEST_11:
        case OP_FUNCTEST_12:
        case OP_FUNCTEST_13:
        case OP_FUNCTEST_14:
        case OP_FUNCTEST_15:

            modersp->r = p->opcode | (p->data << 8);
            return 1;
            break;                                       
        default:
            modersp->m = modersp->m - 1;        
            return 2;
            break;
        }
    }

    return 0; 
}

static int fs11(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p, *t;
    //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    //print_f(&mrs->plog, "fs11", mrs->log);

    p = &mrs->mchine.cur;
    t = &mrs->mchine.tmp;
    
    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = t->opcode;
    p->data = t->data;

    mrs_ipc_put(mrs, "c", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs12(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p, *c;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        c = &mrs->mchine.cur;
        //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        //print_f(&mrs->plog, "fs12", mrs->log);

        if ((p->opcode == c->opcode) && (p->data == c->data)) {
            modersp->r = 1;
            return 1;
        } else {
            modersp->m = modersp->m - 1;        
            modersp->r = 2;
            return 2;
        }
    }
    return 0; 
}

static int fs13(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset=0, ret;
    bitset = 0;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi0 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs13", mrs->log);
    bitset = 0;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi1 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs13", mrs->log);

    int bits = 8;
    ret = ioctl(mrs->sfm[0], SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) {
        sprintf(mrs->log, "can't set bits per word"); 
        print_f(&mrs->plog, "fs13", mrs->log);
    }
    ret = ioctl(mrs->sfm[0], SPI_IOC_RD_BITS_PER_WORD, &bits); 
    if (ret == -1) {
        sprintf(mrs->log, "can't get bits per word"); 
        print_f(&mrs->plog, "fs13", mrs->log);
    }

    ret = ioctl(mrs->sfm[1], SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) {
        sprintf(mrs->log, "can't set bits per word"); 
        print_f(&mrs->plog, "fs13", mrs->log);
    }
    ret = ioctl(mrs->sfm[1], SPI_IOC_RD_BITS_PER_WORD, &bits); 
    if (ret == -1) {
        sprintf(mrs->log, "can't get bits per word"); 
        print_f(&mrs->plog, "fs13", mrs->log);
    }

    ring_buf_init(&mrs->dataRx);
    mrs->dataRx.r->folw.seq = 1;

    modersp->r = 1;
    return 1;
}
static int fs14(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    sprintf(mrs->log, "read patern file\n");
    print_f(&mrs->plog, "fs14", mrs->log);

    mrs_ipc_put(mrs, "r", 1, 1);
    modersp->m = modersp->m + 1;
    modersp->d = 0;

    mrs_ipc_put(mrs, "b", 1, 3);
    mrs_ipc_put(mrs, "b", 1, 4);
    modersp->m = 21;
    modersp->d = 15;

    return 0;
}

static int fs15(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;

    //sprintf(mrs->log, "fs15 \n");
    //print_f(&mrs->plog, "fs15", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if (len > 0) {
        if ((ch == 'r') || (ch == 'e')) {
            if (modersp->d % 2) {
                mrs_ipc_put(mrs, &ch, 1, 4);
            } else {
                mrs_ipc_put(mrs, &ch, 1, 3);
            }
            modersp->m = modersp->m + 1;
        }
    }

    return 0; 
}

static int fs16(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0, bitset;
    char ch=0;

    //sprintf(mrs->log, "fs16 \n");
    //print_f(&mrs->plog, "fs16", mrs->log);

    if (modersp->d % 2) {
        len = mrs_ipc_get(mrs, &ch, 1, 4);
    } else {
        len = mrs_ipc_get(mrs, &ch, 1, 3);
    }

    if (len > 0) {    
        //sprintf(mrs->log, "get ch:%c \n", ch);
        //print_f(&mrs->plog, "fs16", mrs->log);

        if (ch == 'r') {
            modersp->m = modersp->m - 1;
            modersp->d += 1;
            modersp->r = 2;
            return 2;
        }
        if (ch == 'e') {
            sprintf(mrs->log, "get ch:%c \n", ch);
            print_f(&mrs->plog, "fs16", mrs->log);
            modersp->r = 1;
            return 1;
        }
    }

    return 0; 
}

static int fs17(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    struct info16Bit_s *p;

    sprintf(mrs->log, "get OP_FIH \n");
    print_f(&mrs->plog, "fs17", mrs->log);

    p = &mrs->mchine.cur;

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = OP_FIH;
    p->inout = 0;
    p->seqnum = mrs->mchine.seqcnt;
	
    mrs_ipc_put(mrs, "c", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs18(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        //print_f(&mrs->plog, "fs18", mrs->log);

        if (p->opcode == OP_FIH) {
            modersp->m = 23;
        } else {
            modersp->m = modersp->m - 1; 
            modersp->r = 2;
            return 2;
        }
    }
    return 0; 
}

static int fs19(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset;
    bitset = 1;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    //printf("Set spi 0 slave ready: %d\n", bitset);
    sprintf(mrs->log, "Set spi 0 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs19", mrs->log);

    bitset = 1;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    //printf("Set spi 1 slave ready: %d\n", bitset);
    sprintf(mrs->log, "Set spi 1 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs19", mrs->log);

    modersp->r = 1;
    return 1;
}

static int fs20(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset;
    usleep(1);

    bitset = 1;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",0, bitset, modersp->d);
    print_f(&mrs->plog, "fs20", mrs->log);

    bitset = 1;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",1, bitset, modersp->d);
    print_f(&mrs->plog, "fs20", mrs->log);

    usleep(1000);

    bitset = 0;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",0, bitset, modersp->d);
    print_f(&mrs->plog, "fs20", mrs->log);

    bitset = 0;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",1, bitset, modersp->d);
    print_f(&mrs->plog, "fs20", mrs->log);

    usleep(120000);

    bitset = 1;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",0, bitset, modersp->d);
    print_f(&mrs->plog, "fs20", mrs->log);

    bitset = 1;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",1, bitset, modersp->d);
    print_f(&mrs->plog, "fs20", mrs->log);

    usleep(1000);

    bitset = 0;
    ioctl(mrs->sfm[0], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
    sprintf(mrs->log, "[%d]Get RDY pin %d, cnt:%d\n",0, bitset, modersp->d);
    print_f(&mrs->plog, "fs20", mrs->log);

    bitset = 0;
    ioctl(mrs->sfm[1], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
    sprintf(mrs->log, "[%d]Get RDY pin %d, cnt:%d\n",1, bitset, modersp->d);
    print_f(&mrs->plog, "fs20", mrs->log);

    //usleep(60000);

    bitset = 0;
    ioctl(mrs->sfm[0], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
    sprintf(mrs->log, "[%d]Get RDY pin %d, cnt:%d\n",0, bitset, modersp->d);
    print_f(&mrs->plog, "fs20", mrs->log);

    bitset = 0;
    ioctl(mrs->sfm[1], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
    sprintf(mrs->log, "[%d]Get RDY pin %d, cnt:%d\n",1, bitset, modersp->d);
    print_f(&mrs->plog, "fs20", mrs->log);

/*    
    bitset = 1;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 0 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs20", mrs->log);

    bitset = 1;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 1 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs20", mrs->log);
*/
    modersp->r = 1;
    return 1;
}

static int fs21(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int len=0;
    char ch=0;

    //sprintf(mrs->log, "fs21 \n");
    //print_f(&mrs->plog, "fs21", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'B')){
        modersp->m = modersp->m + 1;
    }
    return 0; 

}

static int fs22(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int len=0;
    char ch=0;

    //sprintf(mrs->log, "fs21 \n");
    //print_f(&mrs->plog, "fs22", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 4);
    if ((len > 0) && (ch == 'B')){
        modersp->m = modersp->d;
        modersp->d = 0;
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
    struct info16Bit_s *p;

    p = &mrs->mchine.cur;
    sprintf(mrs->log, "put  0x%.2x 0x%.2x \n",p->opcode, p->data);
    print_f(&mrs->plog, "fs24", mrs->log);

    msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);
	
    mrs_ipc_put(mrs, "c", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs25(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int ret = 0;
    int len=0;
    char ch=0;
    struct info16Bit_s *g;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        g = &mrs->mchine.get;
        //sprintf(mrs->log, "pull 0x%.2x 0x%.2x \n", g->opcode, g->data);
        //print_f(&mrs->plog, "fs25", mrs->log);

        if (g->opcode) {
            if (!modersp->r) {
                modersp->r = g->opcode;
            }
            if (modersp->d) {
                modersp->m = modersp->d;
                modersp->d = 0;
                ret = 2;
            } else {
                ret = 1;
            }
        } else {
            modersp->m = modersp->m - 1;        
            ret = 2;
        }
        sprintf(mrs->log, "r:%d, d:%d m:%d, op:0x%x,0x%x\n",modersp->r, modersp->d, modersp->m, g->opcode, g->data);
        print_f(&mrs->plog, "fs25", mrs->log);

        return ret;
    }
    return 0; 
}

static int fs26(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    char ch=0;
    uint8_t *uch;
    uint32_t startSec = 0, secNum = 0;
    uint32_t startAddr = 0, bLength = 0, maxLen=0;
    int ret = 0, len=0;
    struct info16Bit_s *c, *p;
    struct DiskFile_s *pf;

    sprintf(mrs->log, "read disk file\n");
    print_f(&mrs->plog, "fs26", mrs->log);

    msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);
    pf = &mrs->mchine.fdsk;

    uch = mrs->mchine.sdst.d;
    startSec = mrs->mchine.sdst.n;
    secNum = mrs->mchine.sdln.n;
    mrs->mchine.sdst.f = 0;
    mrs->mchine.sdln.f = 0;

    maxLen = pf->rtMax;

    sprintf(mrs->log, "secStr: %d (%x,%x,%x,%x), secLen: %d\n", startSec, uch[0], uch[1], uch[2], uch[3], secNum);
    print_f(&mrs->plog, "fs26", mrs->log);


    startAddr = startSec * SEC_LEN;
    bLength = secNum * SEC_LEN;

    if (bLength > maxLen) {
        ret = -1;
        goto end;
    }
#if 0 /* pre load in main, so don't have to do it again */
    if (!pf->vsd) {
        ret = -2;
        goto end;
    }


    ret = fseek(pf->vsd, startAddr, SEEK_SET);
    if (ret) {
        sprintf(mrs->log, "seek to %d failed!!! \n", startAddr);
        print_f(&mrs->plog, "fs26", mrs->log);
        goto end;
    }

    char *buff=pf->sdt; 
    int totsz = bLength;

    while (totsz) {
        if (totsz < 32768) {
            len = totsz;
            totsz = 0;
        } else {
            len = 32768;
            totsz -= 32768;
        }
    
        len = fread(buff, 1, len, pf->vsd);
        sprintf(mrs->log, "read file size: %d/%d \n", len, totsz);
        print_f(&mrs->plog, "fs26", mrs->log);
        buff += len;
    }
//#else
    len = fread(pf->sdt, 1, bLength, pf->vsd);
    sprintf(mrs->log, "read file size: %d/%d \n", len, bLength);
    print_f(&mrs->plog, "fs26", mrs->log);
    msync(pf->sdt, bLength, MS_SYNC);
#endif

    if (!ret) {
        pf->rtlen = bLength;

        c = &mrs->mchine.cur;
        p = &mrs->mchine.get;
        c->opcode = p->opcode;
        c->data = p->data;
        modersp->m = 24;
        modersp->d = 27;
        return 2;
    } else {
        pf->rtlen = 0;
    }

end:
/*
    fclose(pf->vsd);
    pf->vsd = 0;
*/
    c = &mrs->mchine.cur;

    c->opcode = 0xe0;
    c->data = 0x5f;
    modersp->m = 24;
    modersp->d = 0;
    modersp->r = 2;
    return 2;
}

static int fs27(struct mainRes_s *mrs, struct modersp_s *modersp) 
{
    int bitset, ret;
    bitset = 0;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi0 set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs27", mrs->log);

    bitset = 1;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "spi0 set ready: %d\n", bitset);
    print_f(&mrs->plog, "fs27", mrs->log);


    int bits = 8;
    ret = ioctl(mrs->sfm[0], SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) {
        sprintf(mrs->log, "can't set bits per word"); 
        print_f(&mrs->plog, "fs27", mrs->log);
    }
    ret = ioctl(mrs->sfm[0], SPI_IOC_RD_BITS_PER_WORD, &bits); 
    if (ret == -1) {
        sprintf(mrs->log, "can't get bits per word"); 
        print_f(&mrs->plog, "fs27", mrs->log);
    }

    ret = ioctl(mrs->sfm[1], SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) {
        sprintf(mrs->log, "can't set bits per word"); 
        print_f(&mrs->plog, "fs27", mrs->log);
    }
    ret = ioctl(mrs->sfm[1], SPI_IOC_RD_BITS_PER_WORD, &bits); 
    if (ret == -1) {
        sprintf(mrs->log, "can't get bits per word"); 
        print_f(&mrs->plog, "fs27", mrs->log);
    }

    modersp->m = modersp->m + 1;

    return 0;
}

static int fs28(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    char ch;
	
    sprintf(mrs->log, "start to do  OP_SDAT !!!\n");
    print_f(&mrs->plog, "fs28", mrs->log);

    ch = 's';
    mrs_ipc_put(mrs, &ch, 1, 3);

    modersp->m = modersp->m+1;
    return 0; 
}

static int fs29(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;

    sprintf(mrs->log, "wait!!!\n");
    print_f(&mrs->plog, "fs29", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'S')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        sprintf(mrs->log, "get S OP_SDAT DONE!!!\n");
        print_f(&mrs->plog, "fs29", mrs->log);

        modersp->m = modersp->m + 1;
        return 0;
    } else if ((len > 0) && (ch == 's')) {
        sprintf(mrs->log, "get S OP_SDAT FAIL!!!\n");
        print_f(&mrs->plog, "fs29", mrs->log);

        modersp->r = 2;
        return 1;
    }

    return 0; 
}

static int fs30(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset;
    //usleep(60000);

    bitset = 0;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "Set spi%d RDY pin: %d, finished!! \n", 0, bitset);
    print_f(&mrs->plog, "fs30", mrs->log);

    usleep(60000);

    bitset = 0;
    ioctl(mrs->sfm[0], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
    sprintf(mrs->log, "Get spi%d RDY pin: %d \n", 0, bitset);
    print_f(&mrs->plog, "fs30", mrs->log);

    //usleep(60000);

    bitset = 0;
    ioctl(mrs->sfm[0], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
    sprintf(mrs->log, "Get spi%d RDY pin: %d \n", 0, bitset);
    print_f(&mrs->plog, "fs30", mrs->log);

    bitset = 1;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "spi0 set ready: %d\n", bitset);
    print_f(&mrs->plog, "fs30", mrs->log);

    modersp->d = modersp->m + 1;
    modersp->m = 1;
    return 2;

}

static int fs31(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    struct info16Bit_s *p;

    sprintf(mrs->log, "send OP_FIH \n");
    print_f(&mrs->plog, "fs31", mrs->log);

    p = &mrs->mchine.cur;

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = OP_FIH;
    p->inout = 0;
    p->seqnum = mrs->mchine.seqcnt;
	
    mrs_ipc_put(mrs, "c", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs32(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        print_f(&mrs->plog, "fs32", mrs->log);

        if (p->opcode == OP_FIH) {
            if (modersp->d) {
                modersp->m = modersp->d;
                modersp->d = 0;
            } else {
                modersp->m = modersp->m + 1;
            }
            modersp->v = 1;
        } else {
            modersp->m = modersp->m - 1; 
            modersp->r = 2;
            return 2;
        }
    }
    return 0; 
}

static int fs33(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset;

    bitset = modersp->v;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "spi0 set ready: %d\n", bitset);
    print_f(&mrs->plog, "fs33", mrs->log);

    if (modersp->d) {
        modersp->m = modersp->d;
        modersp->d = 0;
    } else {
        modersp->m = modersp->m + 1;
    }

    return 0;
}

static int fs34(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p;

    p = &mrs->mchine.cur;
    sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    print_f(&mrs->plog, "fs34", mrs->log);

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = OP_RDY;
    p->inout = 0;
    p->seqnum = mrs->mchine.seqcnt;
	
    mrs_ipc_put(mrs, "c", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs35(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int ret=0;
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        print_f(&mrs->plog, "fs35", mrs->log);

        if (p->opcode == OP_RDY) {

            if (mrs->mchine.fdsk.rtops == OP_SDWT) {
                modersp->m = 37;
                return 2;
            } else {
                modersp->r = 1;
                return 1;
            }
        } else {
            modersp->r = 2;
            return 1;
        }
    }
    return 0; 
}

static int fs36(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int ret = -1;
    struct info16Bit_s *p, *c;
    //char diskname[128] = "/mnt/mmc2/disk_rmImg.bin";
    //char diskname[128] = "/mnt/mmc2/disk_rx127_255log.bin";
    //char diskname[128] = "/mnt/mmc2/disk_golden.bin";
    //char diskname[128] = "/mnt/mmc2/debug_fat.bin";
    //char diskname[128] = "/mnt/mmc2/disk_05.bin";
    //char diskname[128] = "/dev/mmcblk0p1";
    //char diskname[128] = "/dev/mmcblk0";
    //char diskname[128] = "/mnt/mmc2/empty_256.dsk";
    //char diskname[128] = "/mnt/mmc2/folder_256.dsk";
    //char diskname[128] = "/mnt/mmc2/disk_LFN_64.bin";
    char diskname[128] = "/mnt/mmc2/disk_onefolder.bin";
    struct DiskFile_s *fd;
    FILE *fp=0;


    fd = &mrs->mchine.fdsk;
    p = &mrs->mchine.get;
    c = &mrs->mchine.cur;
/*
    fd->vsd = fopen(diskname, "r");
    if (fd->vsd == NULL) {
        sprintf(mrs->log, "disk file [%s] open failed!!! \n", diskname);
        print_f(&mrs->plog, "fs36", mrs->log);
        goto err;
    }

    ret = fseek(fd->vsd, 0, SEEK_END);
    if (ret) {
        sprintf(mrs->log, "seek file [%s] failed!!! \n", diskname);
        print_f(&mrs->plog, "fs36", mrs->log);
        goto err;
    }
*/
    sprintf(mrs->log, "open disk file [%s], op:0x%x\n", diskname, p->opcode);
    print_f(&mrs->plog, "fs36", mrs->log);

    if (p->opcode == OP_SDRD) {
        fd->rtops =  OP_SDRD;
    } else if (p->opcode == OP_SDWT) {
        fd->rtops =  OP_SDWT;
#if DIRECT_WT_DISK 
        fp = fopen(diskname, "w+");
#endif
    } else {
        goto err;
    }

    if (!fd->sdt) {
        sprintf(mrs->log, "ERROR!!! open file [%s] failed, opcodet: 0x%.2x/0x%.2x !!!\n", diskname, p->opcode, p->data);  
        print_f(&mrs->plog, "fs36", mrs->log);
        goto err;
    } else {
        if (fp) {
            fd->vsd = fp;
        }
        c->opcode = p->opcode;
        c->data = p->data;
        ret = 0;
    }

err:
    if (ret) {
        c->opcode = 0xE0;
        c->data = 0x5f;
    }

    p->opcode = 0;
    p->data = 0;

    modersp->m = 24;
    modersp->d = 0;
    modersp->r = 0;
    return 2;
}

static int fs37(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int acusz = 0;
    int ret=0;
    int len=0;
    char ch=0;
    struct DiskFile_s *pf;
    //acusz = rs->pmch->sdln.n * SEC_LEN;
    acusz = mrs->mchine.sdln.n * SEC_LEN;
            
    pf = &mrs->mchine.fdsk;
    sprintf(mrs->log, "save to file size: %d\n", pf->rtMax);
    print_f(&mrs->plog, "fs37", mrs->log);

    msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);
    msync(pf->sdt, pf->rtMax, MS_SYNC);
    
/*
    ret = fseek(pf->vsd, 0, SEEK_SET);
    if (ret) {
        sprintf(mrs->log, "seek file to zero failed!!! \n");
        print_f(&mrs->plog, "fs37", mrs->log);
        modersp->r = 2;
        return 1;
    }

    if (pf->vsd) {
#if 0
        msync(pf->sdt, acusz, MS_SYNC);            
        ret = fwrite(pf->sdt, 1, acusz, mrs->fs);
        fflush(mrs->fs);
        sync();
//#else 
        ret = fwrite(pf->sdt, 1, pf->rtMax, pf->vsd);
#endif
        sprintf(mrs->log, "write file size: %d/%d \n", ret, pf->rtMax);
        print_f(&mrs->plog, "fs37", mrs->log);
        fflush(pf->vsd);
        fsync((int)pf->vsd);
        
    } else {
        ret = -1;
        sprintf(mrs->log, "write file size: %d/%d failed!!!, fp == 0\n", ret, pf->rtMax);
        print_f(&mrs->plog, "fs37", mrs->log);
    }


    //sync();

    fclose(pf->vsd);
    pf->vsd = 0;
*/
    pf->rtops = 0 ;

    if (ret > 0) {
        modersp->r = 1;
    } else {
        modersp->r = 2;
    }

    return 1; 
}

static int fs38(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p;

    p = &mrs->mchine.cur;
    sprintf(mrs->log, "put  0x%.2x 0x%.2x \n",p->opcode, p->data);
    print_f(&mrs->plog, "fs38", mrs->log);

    //msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);
	
    mrs_ipc_put(mrs, "c", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs39(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int ret = 0;
    int len=0;
    char ch=0;
    struct info16Bit_s *g, *c;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        c = &mrs->mchine.cur;
        g = &mrs->mchine.get;
        sprintf(mrs->log, "pull 0x%.2x 0x%.2x \n", g->opcode, g->data);
        print_f(&mrs->plog, "fs39", mrs->log);

        if (g->opcode) {
            if (g->opcode == c->opcode) {
                modersp->r = 1;
            } else {
                modersp->r = 2;
            }

            if (modersp->d) {
                modersp->m = modersp->d;
                modersp->d = 0;
                ret = 2;
            } else {
                ret = 1;
            }
        } else {
            modersp->r = 2;
            ret = 1;
        }
        sprintf(mrs->log, "r:%d, d:%d m:%d, op:0x%x,0x%x\n",modersp->r, modersp->d, modersp->m, g->opcode, g->data);
        print_f(&mrs->plog, "fs39", mrs->log);

        return ret;
    }
    return 0; 
}

static int fs40(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p;

    p = &mrs->mchine.cur;

    sprintf(mrs->log, "set 0x%.2x 0x%.2x \n", p->opcode, p->data);
    print_f(&mrs->plog, "fs40", mrs->log);

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = OP_SUPBACK;
    p->data = 0;

    mrs_ipc_put(mrs, "c", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs41(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;

        sprintf(mrs->log, "get 0x%.2x 0x%.2x \n", p->opcode, p->data);
        print_f(&mrs->plog, "fs41", mrs->log);

        if ((p->opcode == OP_SINGLE) && (p->data == SINSCAN_DUAL_STRM)) {
            modersp->m = modersp->m + 1;
            return 2;
        } else {
            modersp->m = modersp->m - 1;        
            return 2;
        }
    }
    return 0; 
}

static int fs42(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p;
    p = &mrs->mchine.cur;

    sprintf(mrs->log, "set 0x%.2x 0x%.2x \n", p->opcode, p->data);
    print_f(&mrs->plog, "fs2", mrs->log);

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = OP_SUPBACK;
    p->data = 0;
    
    mrs_ipc_put(mrs, "c", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs43(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;

        sprintf(mrs->log, "get 0x%.2x 0x%.2x \n", p->opcode, p->data);
        print_f(&mrs->plog, "fs43", mrs->log);

        if (p->opcode == OP_SUPBACK) {
            if (modersp->d) {
                modersp->m = modersp->d;
                modersp->d = 0;
                return 2;
            } else {
                modersp->r = 1;
                return 1;
            }
        } else {
            modersp->m = modersp->m - 1;        
            modersp->r = 2;
            return 2;
        }
    }
    return 0; 
}

static int fs44(struct mainRes_s *mrs, struct modersp_s *modersp) 
{
    int bitset=0, ret;

    sprintf(mrs->log, "set up\n");
    print_f(&mrs->plog, "fs44", mrs->log);
    

    bitset = 0;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi0 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs13", mrs->log);
    bitset = 0;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi1 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs13", mrs->log);

    int bits = 8;
    ret = ioctl(mrs->sfm[0], SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) {
        sprintf(mrs->log, "can't set bits per word"); 
        print_f(&mrs->plog, "fs13", mrs->log);
    }
    ret = ioctl(mrs->sfm[0], SPI_IOC_RD_BITS_PER_WORD, &bits); 
    if (ret == -1) {
        sprintf(mrs->log, "can't get bits per word"); 
        print_f(&mrs->plog, "fs13", mrs->log);
    }

    ret = ioctl(mrs->sfm[1], SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) {
        sprintf(mrs->log, "can't set bits per word"); 
        print_f(&mrs->plog, "fs13", mrs->log);
    }
    ret = ioctl(mrs->sfm[1], SPI_IOC_RD_BITS_PER_WORD, &bits); 
    if (ret == -1) {
        sprintf(mrs->log, "can't get bits per word"); 
        print_f(&mrs->plog, "fs13", mrs->log);
    }

    ring_buf_init(&mrs->dataTx);
    
    //mrs_ipc_put(mrs, "k", 1, 1);
    mrs_ipc_put(mrs, "b", 1, 3);
    
    modersp->m = modersp->m + 1;
    return 2;
}

static int fs45(struct mainRes_s *mrs, struct modersp_s *modersp) 
{
    int len=0;
    char ch=0;

    //sprintf(mrs->log, "wait for CTL pin to be high \n");
    //print_f(&mrs->plog, "fs45", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'B')){
        modersp->v = 0;
        mrs_ipc_put(mrs, "k", 1, 3);
        modersp->m = modersp->m + 1;
        return 2;
    }

    return 0; 
}

static int fs46(struct mainRes_s *mrs, struct modersp_s *modersp) 
{
    int len=0;
    char ch=0;

    //sprintf(mrs->log, "cnt: %d\n", modersp->v);
    //print_f(&mrs->plog, "fs46", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    while (len) {
        if (len > 0) {
            modersp->v += 1;

            sprintf(mrs->log, "cnt: %d ch:%c\n", modersp->v, ch);
            print_f(&mrs->plog, "fs46", mrs->log);

            if (ch == 'k') {
                mrs_ipc_put(mrs, "k", 1, 1);
            } else if (ch == 'K') {
                mrs_ipc_put(mrs, "K", 1, 1);
                modersp->m = modersp->m + 1;
                return 2;
            }
        }
        len = mrs_ipc_get(mrs, &ch, 1, 3);
    }
    return 0; 

}
static int fs47(struct mainRes_s *mrs, struct modersp_s *modersp) 
{
    int len=0;
    char ch=0;

    //sprintf(mrs->log, "save file done \n");
    //print_f(&mrs->plog, "fs47", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if ((len > 0) && (ch == 'K')){
        modersp->r = 1;
        return 1;
    }

    return 0; 
}

static int fs48(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p, *t;
    p = &mrs->mchine.cur;
    t = &mrs->mchine.tmp;
    
    sprintf(mrs->log, "set 0x%.2x 0x%.2x \n", p->opcode, p->data);
    print_f(&mrs->plog, "fs48", mrs->log);

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = t->opcode;
    p->data = t->data;
    
    mrs_ipc_put(mrs, "c", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs49(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p, *c;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        c = &mrs->mchine.cur;

        sprintf(mrs->log, "get 0x%.2x 0x%.2x \n", p->opcode, p->data);
        print_f(&mrs->plog, "fs49", mrs->log);

        if ((p->opcode == c->opcode) && (p->data == c->data)) {
            if (modersp->d) {
                modersp->m = modersp->d;
                modersp->d = 0;
                return 2;
            } else {
                modersp->r = 1;
                return 1;
            }
        } else {
            modersp->m = modersp->m - 1;        
            modersp->r = 2;
            return 2;
        }
    }
    return 0; 
}

static int fs50(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset, ret;
    sprintf(mrs->log, "read patern file\n");
    print_f(&mrs->plog, "fs50", mrs->log);
    
    bitset = 0;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi0 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs50", mrs->log);

    int bits = 8;
    ret = ioctl(mrs->sfm[0], SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) {
        sprintf(mrs->log, "spi0 can't set bits per word"); 
        print_f(&mrs->plog, "fs50", mrs->log);
    }
    ret = ioctl(mrs->sfm[0], SPI_IOC_RD_BITS_PER_WORD, &bits); 
    if (ret == -1) {
        sprintf(mrs->log, "spi0 can't get bits per word"); 
        print_f(&mrs->plog, "fs50", mrs->log);
    }

    ring_buf_init(&mrs->dataTx);
    mrs_ipc_put(mrs, "s", 1, 1);

    modersp->m = modersp->m + 1;
    modersp->v = 0;

    return 0;
}

static int fs51(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;

    //sprintf(mrs->log, "cnt: %d\n", modersp->v);
    //print_f(&mrs->plog, "fs52", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    while (len) {
        if (len > 0) {
            modersp->v += 1;

            //sprintf(mrs->log, "cnt: %d ch:%c\n", modersp->v, ch);
            //print_f(&mrs->plog, "fs51", mrs->log);

            if (ch == 's') {
                mrs_ipc_put(mrs, "i", 1, 3);
            } else if (ch == 'S') {
                mrs_ipc_put(mrs, "I", 1, 3);
                modersp->m = modersp->m + 1;
                return 2;
            }
        }
        len = mrs_ipc_get(mrs, &ch, 1, 1);
    }
    return 0; 
}
static int fs52(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;

    //sprintf(mrs->log, "spi send done \n");
    //print_f(&mrs->plog, "fs52", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'I')){
        modersp->m = 20;
        modersp->d = 0;
        return 2;
    }

    return 0; 
}

static int fs53(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p;
    p = &mrs->mchine.cur;

    sprintf(mrs->log, "set 0x%.2x 0x%.2x \n", p->opcode, p->data);
    print_f(&mrs->plog, "fs48", mrs->log);

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = OP_DOUBLE;
    p->data = DOUSCAN_WIFI_ONLY;
    
    mrs_ipc_put(mrs, "c", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs54(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;

        sprintf(mrs->log, "get 0x%.2x 0x%.2x \n", p->opcode, p->data);
        print_f(&mrs->plog, "fs54", mrs->log);

        if ((p->opcode == OP_DOUBLE) && (p->data == DOUSCAN_WIFI_ONLY)) {
            modersp->r = 1;
            return 1;
        } else {
            modersp->m = modersp->m - 1;        
            modersp->r = 2;
            return 2;
        }
    }
    return 0; 
}

static int fs55(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset, ret;
    sprintf(mrs->log, "read patern file\n");
    print_f(&mrs->plog, "fs55", mrs->log);
    
    bitset = 0;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi0 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs55", mrs->log);

    bitset = 1;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 0 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs55", mrs->log);

    int bits = 8;
    ret = ioctl(mrs->sfm[0], SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) {
        sprintf(mrs->log, "spi0 can't set bits per word"); 
        print_f(&mrs->plog, "fs55", mrs->log);
    }
    ret = ioctl(mrs->sfm[0], SPI_IOC_RD_BITS_PER_WORD, &bits); 
    if (ret == -1) {
        sprintf(mrs->log, "spi0 can't get bits per word"); 
        print_f(&mrs->plog, "fs55", mrs->log);
    }

    bitset = 0;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi1 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs55", mrs->log);
    
    bitset = 1;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 1 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs55", mrs->log);

    bits = 8;
    ret = ioctl(mrs->sfm[1], SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) {
        sprintf(mrs->log, "spi1 can't set bits per word"); 
        print_f(&mrs->plog, "fs55", mrs->log);
    }
    ret = ioctl(mrs->sfm[1], SPI_IOC_RD_BITS_PER_WORD, &bits); 
    if (ret == -1) {
        sprintf(mrs->log, "spi1 can't get bits per word"); 
        print_f(&mrs->plog, "fs55", mrs->log);
    }


    ring_buf_init(&mrs->dataTx);
    ring_buf_init(&mrs->cmdTx);
    mrs_ipc_put(mrs, "d", 1, 1);

    modersp->m = modersp->m + 1;
    modersp->v = 0;

    return 0;
}

static int fs56(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;

    sprintf(mrs->log, "v: 0x%.2x\n", modersp->v);
    print_f(&mrs->plog, "fs56", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    while (len > 0) {
        //if (len > 0) {
            //sprintf(mrs->log, "cnt: %d ch:%c\n", modersp->v, ch);
            //print_f(&mrs->plog, "fs56", mrs->log);

            if (ch == 's') {
                mrs_ipc_put(mrs, "i", 1, 3);
            } else if (ch == 'd') {
                mrs_ipc_put(mrs, "i", 1, 4);
            } else if (ch == 'S') {
                modersp->v |= 0x01;
                mrs_ipc_put(mrs, "I", 1, 3);
            } else if (ch == 'D') {
                modersp->v |= 0x10;
                mrs_ipc_put(mrs, "I", 1, 4);
            } else {
                sprintf(mrs->log, "WARNING!! unknown ch:%c\n", ch);
                print_f(&mrs->plog, "fs56", mrs->log);
            }            
        //}
        len = mrs_ipc_get(mrs, &ch, 1, 1);
    }

    if (modersp->v & 0x11) {
        modersp->m = modersp->m + 1;
        modersp->v = 0;
        sprintf(mrs->log, "go to next fs, v: 0x%.2x\n", modersp->v);
        print_f(&mrs->plog, "fs56", mrs->log);

        return 2;
    }

    return 0; 
    
}
static int fs57(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch0=0, ch1=0;

    //sprintf(mrs->log, "spi send done \n");
    //print_f(&mrs->plog, "fs57", mrs->log);

    len=0;
    len = mrs_ipc_get(mrs, &ch0, 1, 3);
    if ((len > 0) && (ch0 == 'I')){
        modersp->v |= 0x01;
    }

    len=0;
    len = mrs_ipc_get(mrs, &ch1, 1, 4);
    if ((len > 0) && (ch1 == 'I')){
        modersp->v |= 0x10;
    }

    if (modersp->v == 0x11) {
        modersp->m = 20;
        modersp->d = 0;

        sprintf(mrs->log, "go to next fs, v: 0x%.2x\n", modersp->v);
        print_f(&mrs->plog, "fs57", mrs->log);
        
        modersp->v = 0;
        return 2;
    }
    
    return 0; 
}

static int fs58(struct mainRes_s *mrs, struct modersp_s *modersp) 
{
    int ret = -1;
    struct info16Bit_s *p, *c;
    //char diskname[128] = "/mnt/mmc2/disk_rmImg.bin";
    //char diskname[128] = "/mnt/mmc2/disk_rx127_255log.bin";
    //char diskname[128] = "/mnt/mmc2/disk_golden.bin";
    //char diskname[128] = "/mnt/mmc2/debug_fat.bin";
    //char diskname[128] = "/mnt/mmc2/disk_05.bin";
    //char diskname[128] = "/dev/mmcblk0p1";
    //char diskname[128] = "/dev/mmcblk0";
    //char diskname[128] = "/mnt/mmc2/empty_256.dsk";
    //char diskname[128] = "/mnt/mmc2/folder_256.dsk";
    //char diskname[128] = "/mnt/mmc2/disk_LFN_64.bin";
    char diskname[128] = "/mnt/mmc2/disk_onefolder.bin";
    struct DiskFile_s *fd;
    FILE *fp=0;

    fd = &mrs->mchine.fdsk;
    p = &mrs->mchine.get;
    c = &mrs->mchine.cur;

    fp = fopen(diskname, "w+");
    if (fp == NULL) {
        sprintf(mrs->log, "save disk file [%s] open failed!!! \n", diskname);
        print_f(&mrs->plog, "fs58", mrs->log);
        modersp->r = 2;
        return 1;
    }

    ret = fseek(fp, 0, SEEK_SET);
    if (ret) {
        sprintf(mrs->log, "seek file [%s] failed!!! \n", diskname);
        print_f(&mrs->plog, "fs58", mrs->log);
        modersp->r = 2;
        return 1;
    }

    sprintf(mrs->log, "start to write disk file [%s], op:0x%x\n", diskname, p->opcode);
    print_f(&mrs->plog, "fs58", mrs->log);

    if (!fd->sdt) {
        sprintf(mrs->log, "ERROR!!! memory is not existed, opcodet: 0x%.2x/0x%.2x !!!\n", p->opcode, p->data);  
        print_f(&mrs->plog, "fs58", mrs->log);
        modersp->r = 2;
        return 1;
    } else {
        msync(fd->sdt, fd->rtMax, MS_SYNC);

        ret = fwrite(fd->sdt, 1, fd->rtMax, fp);

        sprintf(mrs->log, "write file size: %d/%d \n", ret, fd->rtMax);
        print_f(&mrs->plog, "fs58", mrs->log);
        fflush(fd->vsd);
        fsync((int)fd->vsd);        
    }

    fclose(fp);
    
    sync();
    
    modersp->d = 0;    
    modersp->m = 24;
    return 2;
}
static int fs59(struct mainRes_s *mrs, struct modersp_s *modersp) 
{
#define SAMPLE_MAX 16
    int ret = -1, idx = 0, fileLen = 0;
    struct info16Bit_s *p, *c;
    char samplefile[128] = "/mnt/mmc2/sample/greenhill_%.2d.jpg";
    char sampledst[128];
    struct DiskFile_s *fd;
    uint32_t strSec, secLen, strflag, lenflag;
    struct SdAddrs_s *psstr, *pslen;
    FILE *fp = 0;
    char *saveDst=0;

    
    psstr = &mrs->mchine.sdst;
    pslen = &mrs->mchine.sdln;

    fd = &mrs->mchine.fdsk;
    p = &mrs->mchine.get;
    c = &mrs->mchine.cur;

    idx = psstr->sdc % SAMPLE_MAX;
    psstr->sdc++;

    sprintf(sampledst, samplefile, idx);
    fp = fopen(sampledst, "r");
    if (!fp) {
        sprintf(mrs->log, "ERROR!!!file read [%s] failed \n", sampledst);
        print_f(&mrs->plog, "fs59", mrs->log);
        modersp->r = 2;
        return 1;        
    } else {
        sprintf(mrs->log, "file read [%s] ok \n", sampledst);
        print_f(&mrs->plog, "fs59", mrs->log);
    }
    
    ret = fseek(fp, 0, SEEK_END);
    if (ret) {
        sprintf(mrs->log, " file seek failed!! ret:%d \n", ret);
        print_f(&mrs->plog, "fs59", mrs->log);
        modersp->r = 2;
        return 1;        
    } 

    fileLen = ftell(fp);
    sprintf(mrs->log, " file [%s] size: %d \n", sampledst, fileLen);
    print_f(&mrs->plog, "fs59", mrs->log);

    ret = fseek(fp, 0, SEEK_SET);
    if (ret) {
        sprintf(mrs->log, " file seek failed!! ret:%d \n", ret);
        print_f(&mrs->plog, "fs59", mrs->log);
        modersp->r = 2;
        return 1;        
    }

    strSec = psstr->sda;
    secLen = fileLen/SEC_LEN  + (((fileLen%SEC_LEN) == 0) ?0:1); // should be length of file

    sprintf(mrs->log, "start to write [%s] to SD strSec: %d, secLen: %d, size: %d  \n", sampledst, strSec, secLen, fileLen);
    print_f(&mrs->plog, "fs59", mrs->log);

    if (!fd->sdt) {
        sprintf(mrs->log, "ERROR!!! memory is not existed");  
        print_f(&mrs->plog, "fs59", mrs->log);
        modersp->r = 2;
        return 1;
    } 
    
    msync(fd->sdt, fd->rtMax, MS_SYNC);

    saveDst = fd->sdt + (strSec * SEC_LEN);
    ret = fread(saveDst, 1, fileLen, fp);
    if (ret != fileLen) {
        sprintf(mrs->log, "WARNING!!! read file size:%d/%d", ret, fileLen);  
        print_f(&mrs->plog, "fs59", mrs->log);
    }

    psstr->n = strSec;
    pslen->n = secLen;

    psstr->f = 0xf;
    pslen->f = 0xf;
    pslen->f |= 0x200;

    modersp->m = 24;
    return 0;
}

#define CROP_COOD_01 {20, 80}
#define CROP_COOD_02 {75, 135}
#define CROP_COOD_03 {85, 135}
#define CROP_COOD_04 {140, 80}
#define CROP_COOD_05 {85, 25}
#define CROP_COOD_06 {75, 25}

#define CROP_SCALE 10
static int fs60(struct mainRes_s *mrs, struct modersp_s *modersp)  
{
    int axy[2] = CROP_COOD_01;
    int id=0;
    uint32_t tmp32=0;
    uint16_t  x=0, y=0;
    struct SdAddrs_s *psstr, *pslen;
    
    pslen = &mrs->mchine.sdln;

    x = axy[0] * CROP_SCALE;
    y = axy[1] * CROP_SCALE;;

    tmp32 = (x << 16) | y;

    pslen->n = 0;
    for (id = 0; id < 4; id++) {
        pslen->d[id] = (tmp32 >> (8 * (3 - id))) & 0xff;
    }

    sprintf(mrs->log, "x, y = (%d, %d)[0x%.8x]\n", x, y, pslen->n);
    print_f(&mrs->plog, "fs60", mrs->log);

    pslen->f = 0xf;
    pslen->f |= 0x200;
    
    modersp->m = 24;
    //modersp->r = 1;
    return 0;
}

static int fs61(struct mainRes_s *mrs, struct modersp_s *modersp)  
{
    int axy[2] = CROP_COOD_02;
    int id=0;
    uint32_t tmp32=0;

    uint16_t  x=0, y=0;
    struct SdAddrs_s *psstr, *pslen;
    
    pslen = &mrs->mchine.sdln;

    x = axy[0] * CROP_SCALE;
    y = axy[1] * CROP_SCALE;;
    
    tmp32 = (x << 16) | y;

    pslen->n = 0;
    for (id = 0; id < 4; id++) {
        pslen->d[id] = (tmp32 >> (8 * (3 - id))) & 0xff;
    }

    sprintf(mrs->log, "x, y = (%d, %d)[0x%.8x]\n", x, y, pslen->n);
    print_f(&mrs->plog, "fs61", mrs->log);

    pslen->f = 0xf;
    pslen->f |= 0x200;
    
   modersp->m = 24;
   // modersp->r = 1;
    return 0;
}

static int fs62(struct mainRes_s *mrs, struct modersp_s *modersp)  
{
    int axy[2] = CROP_COOD_03;
    int id=0;
    uint32_t tmp32=0;

    uint16_t  x=0, y=0;
    struct SdAddrs_s *psstr, *pslen;
    
    pslen = &mrs->mchine.sdln;

    x = axy[0] * CROP_SCALE;
    y = axy[1] * CROP_SCALE;;

    tmp32 = (x << 16) | y;

    pslen->n = 0;
    for (id = 0; id < 4; id++) {
        pslen->d[id] = (tmp32 >> (8 * (3 - id))) & 0xff;
    }

    sprintf(mrs->log, "x, y = (%d, %d)[0x%.8x]\n", x, y, pslen->n);
    print_f(&mrs->plog, "fs62", mrs->log);

    pslen->f = 0xf;
    pslen->f |= 0x200;
    
    modersp->m = 24;
    //modersp->r = 1;
    return 0;
}

static int fs63(struct mainRes_s *mrs, struct modersp_s *modersp)  
{
    int axy[2] = CROP_COOD_04;
    int id=0;
    uint32_t tmp32=0;

    uint16_t  x=0, y=0;
    struct SdAddrs_s *psstr, *pslen;
    
    pslen = &mrs->mchine.sdln;

    x = axy[0] * CROP_SCALE;
    y = axy[1] * CROP_SCALE;;

    tmp32 = (x << 16) | y;

    pslen->n = 0;
    for (id = 0; id < 4; id++) {
        pslen->d[id] = (tmp32 >> (8 * (3 - id))) & 0xff;
    }

    sprintf(mrs->log, "x, y = (%d, %d)[0x%.8x]\n", x, y, pslen->n);
    print_f(&mrs->plog, "fs63", mrs->log);

    pslen->f = 0xf;
    pslen->f |= 0x200;
    
    modersp->m = 24;
    //modersp->r = 1;
    return 0;
}

static int fs64(struct mainRes_s *mrs, struct modersp_s *modersp)  
{
    int axy[2] = CROP_COOD_05;
    int id=0;
    uint32_t tmp32=0;

    uint16_t  x=0, y=0;
    struct SdAddrs_s *psstr, *pslen;
    
    pslen = &mrs->mchine.sdln;

    x = axy[0] * CROP_SCALE;
    y = axy[1] * CROP_SCALE;;

    tmp32 = (x << 16) | y;

    pslen->n = 0;
    for (id = 0; id < 4; id++) {
        pslen->d[id] = (tmp32 >> (8 * (3 - id))) & 0xff;
    }

    sprintf(mrs->log, "x, y = (%d, %d)[0x%.8x]\n", x, y, pslen->n);
    print_f(&mrs->plog, "fs64", mrs->log);

    pslen->f = 0xf;
    pslen->f |= 0x200;
    
    modersp->m = 24;
    //modersp->r = 1;
    return 0;
}

static int fs65(struct mainRes_s *mrs, struct modersp_s *modersp)  
{
    int axy[2] = CROP_COOD_06;
    int id=0;
    uint32_t tmp32=0;

    uint16_t  x=0, y=0;
    struct SdAddrs_s *psstr, *pslen;
    
    pslen = &mrs->mchine.sdln;

    x = axy[0] * CROP_SCALE;
    y = axy[1] * CROP_SCALE;;

    tmp32 = (x << 16) | y;

    pslen->n = 0;
    for (id = 0; id < 4; id++) {
        pslen->d[id] = (tmp32 >> (8 * (3 - id))) & 0xff;
    }

    sprintf(mrs->log, "x, y = (%d, %d)[0x%.8x]\n", x, y, pslen->n);
    print_f(&mrs->plog, "fs65", mrs->log);

    pslen->f = 0xf;
    pslen->f |= 0x200;
    
    modersp->m = 24;
    //modersp->r = 1;
    return 0;
}

static int fs66(struct mainRes_s *mrs, struct modersp_s *modersp)  
{
    modersp->r = 1;
    return 1;
}

static int fs67(struct mainRes_s *mrs, struct modersp_s *modersp)  
{
    modersp->r = 1;
    return 1;
}

static int fs68(struct mainRes_s *mrs, struct modersp_s *modersp)  
{
    modersp->r = 1;
    return 1;
}

static int fs69(struct mainRes_s *mrs, struct modersp_s *modersp)  
{
    modersp->r = 1;
    return 1;
}

static int p0(struct mainRes_s *mrs)
{
#define PS_NUM 70
    int len, tmp, ret;
    char str[128], ch;

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
                                 {65, fs65},{66, fs66},{67, fs67},{68, fs68},{69, fs69}};

    p0_init(mrs);

    // [todo]initial value
    modesw.m = -1;
    modesw.r = 0;
    modesw.d = 0;
    while (1) {
        //sprintf(mrs->log, ".");
        //print_f(&mrs->plog, "P0", mrs->log);

        // p2 data mode spi0 rx
        len = mrs_ipc_get(mrs, &ch, 1, 0);
        if (len > 0) {
            //sprintf(mrs->log, "ret:%d ch:%c\n", ret, ch);
            //print_f(&mrs->plog, "P0", mrs->log);

            if (modesw.m == -1) {
                if ((ch > 0) && (ch < PS_NUM)) {
                    modesw.m = ch;
                }
            } else {
                /* todo: interrupt state machine here */
            }
        }

        if ((modesw.m > 0) && (modesw.m < PS_NUM)) {
            ret = (*afselec[modesw.m].pfunc)(mrs, &modesw);
            //sprintf(mrs->log, "pmode:%d rsp:%d\n", modesw.m, modesw.r);
            //print_f(&mrs->plog, "P0", mrs->log);
            if (ret == 1) {
                tmp = modesw.m;
                modesw.m = 0;
            }
        }

        if (modesw.m == 0) {
            sprintf(mrs->log, "pmode:%d rsp:%d - end\n", tmp, modesw.r);
            print_f(&mrs->plog, "P0", mrs->log);

            ch = modesw.r; /* response */
            modesw.m = -1;
            modesw.r = 0;
            modesw.d = 0;

            mrs_ipc_put(mrs, &ch, 1, 0);
        } else {
            mrs_ipc_put(mrs, "$", 1, 0);
        }

        usleep(100);
        //sleep(2);
    }

    // save to file for debug
    //if (pmode == 1) {
    //   msync(mrs->dataRx.pp[0], 1024*SPI_TRUNK_SZ, MS_SYNC);
    //    ret = fwrite(mrs->dataRx.pp[0], 1, 1024*SPI_TRUNK_SZ, mrs->fs);
    //    printf("\np0 write file %d size %d/%d \n", mrs->fs, 1024*SPI_TRUNK_SZ, ret);
    //}

    p0_end(mrs);
    return 0;
}

static int p1(struct procRes_s *rs, struct procRes_s *rcmd)
{
    int px, pi, ret = 0, ci;
    char ch, cmd, cmdt;
    char *addr;
    uint32_t evt;

    sprintf(rs->logs, "p1\n");
    print_f(rs->plogs, "P1", rs->logs);
    struct psdata_s stdata;
    stfunc pf[SMAX][PSMAX] = {{stspy_01, stspy_02, stspy_03, stspy_04, stspy_05}, // SPY
                            {stbullet_01, stbullet_02, stbullet_03, stbullet_04, stbullet_05}, // BULLET
                            {stlaser_01, stlaser_02, stlaser_03, stlaser_04, stlaser_05},  // LASER
                            {stauto_01, stauto_02, stauto_03, stauto_04, stauto_05}, // AUTO_A
                            {stauto_06, stauto_07, stauto_08, stauto_09, stauto_10}, // AUTO_B
                            {stauto_11, stauto_12, stauto_13, stauto_14, stauto_15}, // AUTO_C
                            {stauto_16, stauto_17, stauto_18, stauto_19, stauto_20}, // AUTO_D
                            {stauto_21, stauto_22, stauto_23, stauto_24, stauto_25}, // AUTO_E
                            {stauto_26, stauto_27, stauto_28, stauto_29, stauto_30}, // AUTO_F
                            {stauto_31, stauto_32, stauto_33, stauto_34, stauto_35}}; // AUTO_G
    /* A.1 ~ A.4 for start sector 01 - 04*/
    /* A.5 = A.8 for sector len   01 - 04*/
    /* A.9 for sd sector transmitting using command mode */
    p1_init(rs);
    // wait for ch from p0
    // state machine control
    stdata.rs = rs;
    pi = 0;    stdata.result = 0;    cmd = '\0';   cmdt = '\0';
    while (1) {
        //sprintf(rs->logs, "+\n");
        //print_f(rs->plogs, "P1", rs->logs);

        ci = 0; 
        ci = rs_ipc_get(rcmd, &cmd, 1);
        while (ci > 0) {
            sprintf(rs->logs, "%c\n", cmd);
            print_f(rs->plogs, "P1CMD", rs->logs);

            if (cmdt == '\0') {
                if (cmd == 's') {
                    stdata.result = 0x0;
                    cmdt = cmd;
                } else if (cmd == 'b') {
                    stdata.result = emb_stanPro(0, STINIT, BULLET, PSSET);
                    cmdt = cmd;
                } else if (cmd == 'p') {
                    stdata.result = emb_stanPro(0, STINIT, SPY, PSSET);
                    cmdt = cmd;
                }
            } else { /* command to interrupt state machine here */

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

        if (((ret > 0) && (ch != '$')) ||
            (cmdt != '\0')) {
            stdata.ansp0 = ch;
            evt = stdata.result;
            pi = (evt >> 8) & 0xff;
            px = (evt & 0xff);
            if ((pi >= SMAX) || (px >= PSMAX)) {
                cmdt = '\0';
                continue;
            }

            stdata.result = (*pf[pi][px])(&stdata);

            //sprintf(rs->logs, "ret:%d ch:%d evt:0x%.4x\n", ret, ch, evt);
            //print_f(rs->plogs, "P1", rs->logs);
        }

        //rs_ipc_put(rs, &ch, 1);
        //usleep(60000);
    }

    p1_end(rs);
    return 0;
}

static int p2(struct procRes_s *rs)
{
#define SAVE_OUT (0)

    /* spi0 */
    char ch;
    int totsz=0, fsize=0, pi=0, len, opsz=0, ret=0, max=0, tlen=0;
    char *addr;
    char filedst[128];
    //char filename[128] = "/mnt/mmc2/sample1.mp4";
    //char filename[128] = "/mnt/mmc2/handmade.jpg";
    char filename[128] = "/mnt/mmc2/scan_pro.jpg";
    //char filename[128] = "/mnt/mmc2/textfile_02.bin";
    char fileback[128] = "/mnt/mmc2/tx/recv_%d.bin";
#if SAVE_OUT
    char fileout[128] = "/mnt/mmc2/tx/sample_%d.bin";
#endif
    //char filename[128] = "/mnt/mmc2/hilldesert.jpg";
    //char filename[128] = "/mnt/mmc2/sample1.mp4";
    //char filename[128] = "/mnt/mmc2/pattern2.txt";
    FILE *fp = NULL, *fout=NULL;
	
    if (infpath[0] != '\0') {
        strcpy(filename, infpath);
    }

    fp = fopen(filename, "r");
    if (!fp) {
        sprintf(rs->logs, "file read [%s] failed \n", filename);
        print_f(rs->plogs, "P2", rs->logs);
        while(1);
    } else {
        sprintf(rs->logs, "file read [%s] ok \n", filename);
        print_f(rs->plogs, "P2", rs->logs);
    }
    fclose(fp);

    sprintf(rs->logs, "p2\n");
    print_f(rs->plogs, "P2", rs->logs);

    p2_init(rs);
    while (1) {
        //sprintf(rs->logs, "!\n");
        //print_f(rs->plogs, "P2", rs->logs);

        len = rs_ipc_get(rs, &ch, 1);
        if (len > 0) {
            //sprintf(rs->logs, "%c \n", ch);
            //print_f(rs->plogs, "P2", rs->logs);

            if (ch == 'r') {
#if SAVE_OUT
                fout = find_save(filedst, fileout);
                if (fout) {
                    sprintf(rs->logs, "file save back to [%s]\n",filedst);
                    print_f(rs->plogs, "P2", rs->logs); 
                } else {
                    sprintf(rs->logs, "FAIL to find file [%s]\n",fileback);
                    print_f(rs->plogs, "P2", rs->logs); 
                }
#endif
                fp = fopen(filename, "r");
                if (!fp) {
                    sprintf(rs->logs, "file read [%s] failed \n", filename);
                    print_f(rs->plogs, "P2", rs->logs);
                    continue;
                } else {
                    sprintf(rs->logs, "file read [%s] ok \n", filename);
                    print_f(rs->plogs, "P2", rs->logs);
                }

                totsz = 0;
                pi = 0;
                
                ret = fseek(fp, 0, SEEK_END);
                if (ret) {
                    sprintf(rs->logs, " file seek failed!! ret:%d \n", ret);
                    print_f(rs->plogs, "P2", rs->logs);
                } 
                
                max = ftell(fp);
                sprintf(rs->logs, " file [%s] size: %d \n", filename, max);
                print_f(rs->plogs, "P2", rs->logs);

                ret = fseek(fp, 0, SEEK_SET);
                if (ret) {
                    sprintf(rs->logs, " file seek failed!! ret:%d \n", ret);
                    print_f(rs->plogs, "P2", rs->logs);
                }

                while (1) {
                    len = ring_buf_get_dual(rs->pdataRx, &addr, pi);      
                    memset(addr, 0xff, len);
                    if (max < len) {
                        len = max;
                    }

                    ret = fseek(fp, totsz, SEEK_SET);
                    if (ret) {
                        sprintf(rs->logs, " file seek failed!! ret:%d \n", ret);
                        print_f(rs->plogs, "P2", rs->logs);
                    }

                    msync(addr, len, MS_SYNC);
                    
                    fsize = fread(addr, 1, len, fp);

                    
                    totsz += fsize;
                    max -= len;

                    ring_buf_prod_dual(rs->pdataRx, pi);
#if SAVE_OUT
                    tlen = fwrite(addr, 1, len, fout);
#else
                    tlen = len;
#endif
                    sprintf(rs->logs, " %d %d/%d/%d - %d/%d\n", pi, fsize, tlen, len, totsz, max);
                    print_f(rs->plogs, "P2", rs->logs);
                    
                    if (!max) break;
                    pi++;
                    rs_ipc_put(rs, "r", 1);
                    
                }
#if SAVE_OUT
                /* align to SPI_TRUNK_SZ */
                tlen = fsize % 1024;
                sprintf(rs->logs, "1.r %d sz %d \n", tlen, fsize);
                print_f(rs->plogs, "P2", rs->logs);

                if (tlen) {
                    fsize = fsize + 1024 - tlen;
                }

                tlen = totsz % 1024;
                sprintf(rs->logs, "2.r %d sz %d \n", tlen, totsz);
                print_f(rs->plogs, "P2", rs->logs);

                if (tlen) {
                    totsz = totsz + 1024 - tlen;
                }
#endif
                ring_buf_set_last_dual(rs->pdataRx, fsize, pi);
                rs_ipc_put(rs, "r", 1);
                rs_ipc_put(rs, "e", 1);

                rs->pmch->cur.info = totsz;

                sprintf(rs->logs, "file [%s] read size: %d \n",filename, totsz);
                print_f(rs->plogs, "P2", rs->logs);
#if SAVE_OUT
                fflush(fout);
                fclose(fout);
#endif
                fclose(fp);
                fp = NULL;
            }

            if (ch == 'g') {
                int totsz=0, fsize=0, pi=0, len, ret;
                char *addr, ch, *laddr, *saddr, *pool;
                char filename[128] = "/mnt/mmc2/pattern2.txt";
                FILE *fp;

                laddr = malloc(64);
                saddr = malloc(64);
                pool = malloc(4096);

                ch = 0x30;
                addr = laddr;
                for (pi = 0; pi < 31; pi++) {
                    addr[pi] = ch;
                    ch++;
                }

                ch = 0x30;
                addr = laddr+31;
                for (pi = 0; pi < 31; pi++) {
                    addr[pi] = ch;
                    ch++;
                }

                fp = fopen(filename, "wr");
                if (!fp) {
                    sprintf(rs->logs, "file read [%s] failed \n", filename);
                    print_f(rs->plogs, "P2", rs->logs);
                }
/*
                addr = pool;
                for (pi = 0; pi < 128; pi++) {
                    memcpy(addr, laddr + (pi % 32), 31);
                    addr+=31;
                    *addr = '\n';
                    addr+=1;
                }
*/
                for (pi = 0; pi < 2097151; pi++) {
                    memcpy(saddr, laddr + (pi % 31), 31);
                    saddr[31] = '\n';
                    ret = fwrite(saddr, 1, 32, fp);
                    //sprintf(rs->logs, "g ret:%d - %d\n", ret, pi);
                    //print_f(rs->plogs, "P2", rs->logs);
                }
                sprintf(rs->logs, "g ret:%d - %d\n", ret, pi);
                print_f(rs->plogs, "P2", rs->logs);

                fclose(fp);

                fp = fopen(filename, "r");
                if (!fp) {
                    sprintf(rs->logs, "file read [%s] failed \n", filename);
                    print_f(rs->plogs, "P2", rs->logs);
                }

                len = ring_buf_get_dual(rs->pdataRx, &addr, pi);
                fsize = fread(addr, 1, len, fp);
                totsz += fsize;
                while (fsize == len) {
                    ring_buf_prod_dual(rs->pdataRx, pi);
                    pi++;
                    rs_ipc_put(rs, "r", 1);
                    len = ring_buf_get_dual(rs->pdataRx, &addr, pi);
                    fsize = fread(addr, 1, len, fp);
                    totsz += fsize;
                }

                ring_buf_prod_dual(rs->pdataRx, pi);
                ring_buf_set_last_dual(rs->pdataRx, fsize, pi);
                rs_ipc_put(rs, "r", 1);
                rs_ipc_put(rs, "e", 1);

                sprintf(rs->logs, "file [%s] read size: %d \n",filename, totsz);
                print_f(rs->plogs, "P2", rs->logs);

                fclose(fp);
            }

            if (ch == 'k') { 
                fp = find_save(filedst, fileback);
                if (fp) {
                    sprintf(rs->logs, "file save back to [%s]\n",filedst);
                    print_f(rs->plogs, "P2", rs->logs); 
                } else {
                    sprintf(rs->logs, "FAIL to find file [%s]\n",fileback);
                    print_f(rs->plogs, "P2", rs->logs); 
                }
                
                totsz = rs->pmch->cur.info;
                sprintf(rs->logs, "total size: %d\n", totsz);
                print_f(rs->plogs, "P2", rs->logs); 

                totsz = 0;
                pi = 0;
                while (1) {
                    ret = ring_buf_cons(rs->pdataTx, &addr, &len);
                    if ((ret == 0) ||(ret == -2)) {

                        if (len > 0) {
                            pi++;                    
                            msync(addr, len, MS_SYNC);
                            #if 1 /*debug*/
                            opsz = fwrite(addr, 1, len, fp);
                            totsz += opsz;
                            #else
                            opsz = len;
                            #endif
                            sprintf(rs->logs, "w %d/%d \n", opsz, totsz);
                            print_f(rs->plogs, "P2", rs->logs);        

                            memset(addr, 0x95, len);
                        }

                        if (ret == -2) {
                            sprintf(rs->logs, "save len:%d cnt:%d total:%d -loop end\n", opsz, pi, totsz);
                            print_f(rs->plogs, "P2", rs->logs);         
                            break;
                        }
                    } else {
                        sprintf(rs->logs, "wait for buffer, ret:%d\n", ret);
                        print_f(rs->plogs, "P2", rs->logs);         
                    }

                    if (ch != 'K') {
                        ch = 0;
                        rs_ipc_get(rs, &ch, 1);
                    }
                }

                while (ch != 'K') {
                    sprintf(rs->logs, "%c clr\n", ch);
                    print_f(rs->plogs, "P2", rs->logs);         
                    ch = 0;
                    rs_ipc_get(rs, &ch, 1);
                }

                sync();
                fflush(fp);
                fclose(fp);

                rs_ipc_put(rs, "K", 1);
                sprintf(rs->logs, "file save cnt:%d total:%d- end\n", pi, totsz);
                print_f(rs->plogs, "P2", rs->logs);      
                
            }

            if (ch == 's') { /*single*/
                fp = fopen(filename, "r");
                if (!fp) {
                    sprintf(rs->logs, "file read [%s] failed \n", filename);
                    print_f(rs->plogs, "P2", rs->logs);
                    continue;
                } else {
                    sprintf(rs->logs, "file read [%s] ok \n", filename);
                    print_f(rs->plogs, "P2", rs->logs);
                }

                totsz = 0;
                pi = 0;
                
                ret = fseek(fp, 0, SEEK_END);
                if (ret) {
                    sprintf(rs->logs, " file seek failed!! ret:%d \n", ret);
                    print_f(rs->plogs, "P2", rs->logs);
                } 
                
                max = ftell(fp);
                sprintf(rs->logs, " file [%s] size: %d \n", filename, max);
                print_f(rs->plogs, "P2", rs->logs);

                ret = fseek(fp, 0, SEEK_SET);
                if (ret) {
                    sprintf(rs->logs, " file seek failed!! ret:%d \n", ret);
                    print_f(rs->plogs, "P2", rs->logs);
                }

                while (1) {
                    len = 0;
                    len = ring_buf_get(rs->pdataTx, &addr);
                    while (len <= 0) {
                        usleep(150000);
                        len = ring_buf_get(rs->pdataTx, &addr);
                    }

                    memset(addr, 0xff, len);
                    if (max < len) {
                        len = max;
                    }

                    ret = fseek(fp, totsz, SEEK_SET);
                    if (ret) {
                        sprintf(rs->logs, " file seek failed!! ret:%d \n", ret);
                        print_f(rs->plogs, "P2", rs->logs);
                    }

                    msync(addr, len, MS_SYNC);
                    
                    fsize = fread(addr, 1, len, fp);

                    totsz += fsize;
                    max -= len;

                    ring_buf_prod(rs->pdataTx);
                    tlen = len;
                    sprintf(rs->logs, " %d - %d/%d\n", pi, totsz, max);
                    print_f(rs->plogs, "P2s", rs->logs);
                    
                    if (!max) break;
                    pi++;
                    rs_ipc_put(rs, "s", 1);
                    
                }
                

                /* align to SPI_TRUNK_SZ */
                tlen = fsize % 1024;
                sprintf(rs->logs, "1.r %d sz %d \n", tlen, fsize);
                print_f(rs->plogs, "P2", rs->logs);

                if (tlen) {
                    fsize = fsize + 1024 - tlen;
                }

                tlen = totsz % 1024;
                sprintf(rs->logs, "2.r %d sz %d \n", tlen, totsz);
                print_f(rs->plogs, "P2", rs->logs);

                if (tlen) {
                    totsz = totsz + 1024 - tlen;
                }

                ring_buf_set_last(rs->pdataTx, fsize);
                rs_ipc_put(rs, "s", 1);
                rs_ipc_put(rs, "S", 1);

                rs->pmch->cur.info = totsz;

                sprintf(rs->logs, "file [%s] read size: %d \n",filename, totsz);
                print_f(rs->plogs, "P2", rs->logs);

                fclose(fp);
                fp = NULL;
            }

            if (ch == 'd') { /*double*/
                fp = fopen(filename, "r");
                if (!fp) {
                    sprintf(rs->logs, "file read [%s] failed \n", filename);
                    print_f(rs->plogs, "P2d", rs->logs);
                    continue;
                } else {
                    sprintf(rs->logs, "file read [%s] ok \n", filename);
                    print_f(rs->plogs, "P2d", rs->logs);
                }

                totsz = 0;
                pi = 0;
                
                ret = fseek(fp, 0, SEEK_END);
                if (ret) {
                    sprintf(rs->logs, " file seek failed!! ret:%d \n", ret);
                    print_f(rs->plogs, "P2", rs->logs);
                } 
                
                max = ftell(fp);
                sprintf(rs->logs, " file [%s] size: %d \n", filename, max);
                print_f(rs->plogs, "P2", rs->logs);

                ret = fseek(fp, 0, SEEK_SET);
                if (ret) {
                    sprintf(rs->logs, " file seek failed!! ret:%d \n", ret);
                    print_f(rs->plogs, "P2", rs->logs);
                }

                while (1) {
#if 1
                    ret = fseek(fp, totsz, SEEK_SET);
                    if (ret) {
                        sprintf(rs->logs, " file seek failed!! ret:%d - 1\n", ret);
                        print_f(rs->plogs, "P2", rs->logs);
                        break;
                    }
                    len = 0;
                    len = ring_buf_get(rs->pdataTx, &addr);
                    while (len <= 0) {
                        usleep(10000);
                        len = ring_buf_get(rs->pdataTx, &addr);
                    }
                    memset(addr, 0xff, len);
                    if (max < len) {
                        len = max;
                    }
                    msync(addr, len, MS_SYNC);
                    fsize = fread(addr, 1, len, fp);
                    ring_buf_prod(rs->pdataTx);
                    tlen = len;
                    sprintf(rs->logs, " %d - %d,%d - 1\n", pi, totsz, fsize);
                    print_f(rs->plogs, "P2d", rs->logs);
                    rs_ipc_put(rs, "s", 1);
                    msync(addr, len, MS_SYNC);
#endif
#if 1
                    ret = fseek(fp, totsz, SEEK_SET);
                    if (ret) {
                        sprintf(rs->logs, " file seek failed!! ret:%d - 2\n", ret);
                        print_f(rs->plogs, "P2", rs->logs);
                        break;
                    }
                    len = 0;
                    len = ring_buf_get(rs->pcmdTx, &addr);
                    while (len <= 0) {
                        usleep(10000);
                        len = ring_buf_get(rs->pcmdTx, &addr);
                    }
                    memset(addr, 0xff, len);
                    if (max < len) {
                        len = max;
                    }
                    msync(addr, len, MS_SYNC);
                    fsize = fread(addr, 1, len, fp);
                    ring_buf_prod(rs->pcmdTx);
                    tlen = len;
                    sprintf(rs->logs, " %d - %d,%d - 2\n", pi, totsz, fsize);
                    print_f(rs->plogs, "P2d", rs->logs);
                    rs_ipc_put(rs, "d", 1);
                    msync(addr, len, MS_SYNC);
#endif
                    totsz += fsize;
                    max -= len;
                    
                    if (!max) break;
                    pi++;
                }
                

                /* align to SPI_TRUNK_SZ */
                tlen = fsize % 1024;
                sprintf(rs->logs, "1.r %d sz %d \n", tlen, fsize);
                print_f(rs->plogs, "P2", rs->logs);

                if (tlen) {
                    fsize = fsize + 1024 - tlen;
                }

                tlen = totsz % 1024;
                sprintf(rs->logs, "2.r %d sz %d \n", tlen, totsz);
                print_f(rs->plogs, "P2", rs->logs);

                if (tlen) {
                    totsz = totsz + 1024 - tlen;
                }

                ring_buf_set_last(rs->pdataTx, fsize);
                ring_buf_set_last(rs->pcmdTx, fsize);
                rs_ipc_put(rs, "S", 1);
                rs_ipc_put(rs, "D", 1);

                rs->pmch->cur.info = totsz;

                sprintf(rs->logs, "file [%s] read size: %d \n",filename, totsz);
                print_f(rs->plogs, "P2d", rs->logs);

                fclose(fp);
                fp = NULL;
            }
        }
    }

    p2_end(rs);
    return 0;
}

static int p3(struct procRes_s *rs)
{
    /* spi1 */
    char ch;
    int len;

    sprintf(rs->logs, "p3\n");
    print_f(rs->plogs, "P3", rs->logs);

    p3_init(rs);
    while (1) {
        //sprintf(rs->logs, "/\n");
        //print_f(rs->plogs, "P3", rs->logs);

        len = rs_ipc_get(rs, &ch, 1);
        if (len > 0) {
            sprintf(rs->logs, "%c \n", ch);
            print_f(rs->plogs, "P3", rs->logs);
        }
    }

    p3_end(rs);
    return 0;
}

#define MSP_P4_SAVE_DAT (0)
static int p4(struct procRes_s *rs)
{
#define OUT_SAVE (0)

#if OUT_SAVE
    char fileout[128] = "/mnt/mmc2/tx/fat_lov.bin";
    FILE *fout = NULL;
#endif
    float flsize, fltime;
    char ch, *tx, *rx, *tx_buff, *rx_buff, *addr=0;
    uint16_t *tx16, *rx16, in16;
    int len = 0, cmode = 0, ret, pi=0, acusz=0, starts=0, opsz=0, totsz, tdiff;
    int slen=0;
    uint32_t bitset;
    struct DiskFile_s *pf;
    
    sprintf(rs->logs, "p4\n");
    print_f(rs->plogs, "P4", rs->logs);

    p4_init(rs);

    pf = &rs->pmch->fdsk;
    tx = malloc(64);
    rx = malloc(64);
    rx_buff = malloc(SPI_TRUNK_SZ);

    int i;
    for (i = 0; i < 64; i+=4) {
        tx[i] = 0xaa;
        tx[i+1] = 0x55; 
        tx[i+2] = 0xff; 
        tx[i+3] = 0xee; 
    }

    tx16 = (uint16_t *)tx;
    rx16 = (uint16_t *)rx;	

    while (1) {
        //printf("^");
        //sprintf(rs->logs, "^\n");
        //print_f(rs->plogs, "P4", rs->logs);

        len = 0;
        len = rs_ipc_get(rs, &ch, 1);
        if (len > 0) {
            //sprintf(rs->logs, "%c \n", ch);
            //print_f(rs->plogs, "P4", rs->logs);

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
                case 's':
                    cmode = 5;
                    break;
                case 'k':
                    cmode = 6;
                    break;
                case 'i':
                    cmode = 7;
                    break;
                default:
                    break;
            }
        }

        if (cmode == 1) {
            bitset = 1;
            ioctl(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
            sprintf(rs->logs, "Check if RDY pin == 0 (pin:%d)\n", bitset);
            print_f(rs->plogs, "P4", rs->logs);

            while (1) {
                bitset = 1;
                ioctl(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
                //sprintf(rs->logs, "Get RDY pin: %d\n", bitset);
                //print_f(rs->plogs, "P4", rs->logs);

                if (bitset == 0) break;
            }
            if (!bitset) rs_ipc_put(rs, "G", 1);
        } else if (cmode == 2) {
            int bits = 16;
            ret = ioctl(rs->spifd, SPI_IOC_WR_BITS_PER_WORD, &bits);
            if (ret == -1) {
                sprintf(rs->logs, "can't set bits per word"); 
                print_f(rs->plogs, "P4", rs->logs);
            }
            ret = ioctl(rs->spifd, SPI_IOC_RD_BITS_PER_WORD, &bits); 
            if (ret == -1) {
                sprintf(rs->logs, "can't get bits per word"); 
                print_f(rs->plogs, "P4", rs->logs);
            }

            len = 0;

            msync(rs->pmch, sizeof(struct machineCtrl_s), MS_SYNC);            
            *tx16 = pkg_info(&rs->pmch->cur);
            len = mtx_data_16(rs->spifd, rx16, tx16, 1, 2, 1024);
            if (len > 0) {
                in16 = rx16[0];

                //sprintf(rs->logs, "16Bits send: 0x%.4x, get: 0x%.4x\n", tx16[0], in16);
                //print_f(rs->plogs, "P4", rs->logs);

                abs_info(&rs->pmch->get, in16);
                rs_ipc_put(rs, "C", 1);
            }
            sprintf(rs->logs, "16Bits send: 0x%.4x, get: 0x%.4x, len: %d\n", *tx16, in16, len);
            print_f(rs->plogs, "P4", rs->logs);
        } else if (cmode == 3) {
            int size=0, opsz;
            char *addr;
            pi = 0;
            size = ring_buf_cons_dual(rs->pdataRx, &addr, pi);
            if (size >= 0) {
                //sprintf(rs->logs, "cons 0x%x %d %d \n", addr, size, pi);
                //print_f(rs->plogs, "P4", rs->logs);
                 
                msync(addr, size, MS_SYNC);

#if OUT_SAVE
                fout = fopen(fileout, "a+");
                if (!fout) {
                    sprintf(rs->logs, "file read [%s] failed \n", fileout);
                    print_f(rs->plogs, "P4", rs->logs);
                } else {
                    sprintf(rs->logs, "file read [%s] ok \n", fileout);
                    print_f(rs->plogs, "P4", rs->logs);

                    slen = fwrite(addr, 1, size, fout);

                    fflush(fout);
                    fclose(fout);
                    fout = NULL;

                }
#endif

                /* send data to wifi socket */
                //opsz = write(rs->psocket_t->connfd, addr, size);
                opsz = mtx_data(rs->spifd, NULL, addr, 1, size, 1024*1024);
                //printf("socket tx %d %d\n", rs->psocket_r->connfd, opsz);

                sprintf(rs->logs, "%d/%d, %d\n", opsz, size, slen);
                print_f(rs->plogs, "P4", rs->logs);

                pi+=2;

                rs_ipc_put(rs, "r", 1);
            } else {
                sprintf(rs->logs, "socket tx %d %d - end\n", opsz, size);
                print_f(rs->plogs, "P4", rs->logs);
                rs_ipc_put(rs, "e", 1);
            }
        } else if (cmode == 4) {
            bitset = 0;
            ioctl(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
            sprintf(rs->logs, "Check if RDY pin == 1 (pin:%d)\n", bitset);
            print_f(rs->plogs, "P4", rs->logs);

            while (1) {
                bitset = 0;
                ioctl(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
                //sprintf(rs->logs, "Get RDY pin: %d\n", bitset);
                //print_f(rs->plogs, "P4", rs->logs);

                if (bitset == 1) break;
            }
            if (bitset) rs_ipc_put(rs, "B", 1);
        } else if (cmode == 5) {
#if MSP_P4_SAVE_DAT
                rs->fdat_s = 0;
                if (pf->rtops ==  OP_SDWT) {
                    ret = file_save_get(&rs->fdat_s, "/mnt/mmc2/tx/%d.dat");
                    /*
                    if (ret) {
                        sprintf(rs->logs, "get tx log data file error - %d, hold here\n", ret);
                        print_f(rs->plogs, "P4", rs->logs);         
                        while(1);
                    } else {
                        sprintf(rs->logs, "get tx log data file ok - %d, f: %x\n", ret, rs->fdat_s);
                        print_f(rs->plogs, "P4", rs->logs);         
                    }
                    */
                }
#endif
            starts = rs->pmch->sdst.n * SEC_LEN;
            acusz = rs->pmch->sdln.n * SEC_LEN;

            rs->pmch->sdln.f = 0;
            rs->pmch->sdst.f = 0;

            if (acusz > rs->pmch->fdsk.rtMax) {
                sprintf(rs->logs, "OP_SDAT failed due to size > max, len:%d,mx:%d\n", acusz, rs->pmch->fdsk.rtMax);
                print_f(rs->plogs, "P4", rs->logs);

                rs_ipc_put(rs, "s", 1);
                continue;
            }


            if (!rs->pmch->fdsk.sdt) {
                sprintf(rs->logs, "OP_SDAT failed due to input buff == null\n");
                print_f(rs->plogs, "P4", rs->logs);

                rs_ipc_put(rs, "s", 1);
                continue;
            }

            tx_buff = rs->pmch->fdsk.sdt + starts;
            msync(tx_buff, acusz, MS_SYNC);
            //shmem_dump(tx_buff, acusz);

            pi = 0;
            while (acusz>0) {
                if (acusz > SPI_TRUNK_SZ) {
                    slen = SPI_TRUNK_SZ;
                    acusz -= slen;
                } else {
                    slen = acusz;
                    acusz = 0;
                }

                if (pf->rtops ==  OP_SDWT) {
                    len = mtx_data(rs->spifd, tx_buff, rx_buff, 1, slen, 1024*1024);
                    //shmem_dump(tx_buff, len);
#if OUT_SAVE
                fout = fopen(fileout, "a+");
                if (!fout) {
                    sprintf(rs->logs, "file read [%s] failed \n", fileout);
                    print_f(rs->plogs, "P4", rs->logs);
                } else {
                    sprintf(rs->logs, "file read [%s] ok \n", fileout);
                    print_f(rs->plogs, "P4", rs->logs);

                    slen = fwrite(tx_buff, 1, slen, fout);

                    fflush(fout);
                    fclose(fout);
                    fout = NULL;

                }
#endif
                } else if (pf->rtops ==  OP_SDRD) {
                    len = mtx_data(rs->spifd, rx_buff, tx_buff, 1, slen, 1024*1024);                
                } else {
                    len = 0;
                }

                sprintf(rs->logs, "%d.Send %d/%d bytes!!\n", pi, len, slen);
                print_f(rs->plogs, "P4", rs->logs);

#if MSP_P4_SAVE_DAT
                if (rs->fdat_s) {
                    msync(tx_buff, slen, MS_SYNC);
                    fwrite(tx_buff, 1, slen, rs->fdat_s);
                }
#endif
                pi++;
                tx_buff += len;
            }

            rs_ipc_put(rs, "S", 1);

#if MSP_P4_SAVE_DAT
            if (rs->fdat_s) {
                fflush(rs->fdat_s);
                fclose(rs->fdat_s);
            }
#endif
            rs->fdat_s = 0;
        }
        else if (cmode == 6) {
        
            totsz = rs->pmch->cur.info;
            sprintf(rs->logs, "get total size:%d\n", totsz);
            print_f(rs->plogs, "P4", rs->logs);

            acusz = 0;

            while (1) {
                len = ring_buf_get(rs->pdataTx, &addr);

                if (len > 0) {
                    memset(addr, 0xaa, len);
                
                    if (totsz < len) {
                        len = totsz;
                    }

                    msync(addr, len, MS_SYNC);
                    opsz = mtx_data(rs->spifd, addr, addr, 1, len, 1024*1024);  


                    if (opsz <= 0) {
                        sprintf(rs->logs, "ERROR!!! tx %d/%d acusz:%d\n", opsz, len, acusz);
                        print_f(rs->plogs, "P4", rs->logs);
                        break;
                    }
                    
                    acusz += opsz;
                    totsz -= opsz;
                    
                    sprintf(rs->logs, "t %d / %d\n", opsz, totsz);
                    print_f(rs->plogs, "P4", rs->logs);

                    if (!totsz) break;

                    ring_buf_prod(rs->pdataTx);
                
                    rs_ipc_put(rs, "k", 1);          
                }
            }

            totsz = rs->pmch->cur.info;
            rs->pmch->get.info = acusz;

            ring_buf_prod(rs->pdataTx);
            ring_buf_set_last(rs->pdataTx, opsz);
            rs_ipc_put(rs, "k", 1);          
            rs_ipc_put(rs, "K", 1);                      
            sprintf(rs->logs, "tx acusz:%d totsz:%d - end\n", acusz, totsz);
            print_f(rs->plogs, "P4", rs->logs);

        }
        else if (cmode == 7) {
            clock_gettime(CLOCK_REALTIME, rs->tm[0]);
            totsz = 0;
            pi = 0;
            while (1) {
                ret = ring_buf_cons(rs->pdataTx, &addr, &len);
                if ((ret == 0) ||(ret == -2)) {
                    if (len > 0) {
                        pi++;                    
                        msync(addr, len, MS_SYNC);
                        #if 1 /*debug*/
                        opsz = mtx_data(rs->spifd, addr, addr, 1, len, 1024*1024);  
                        totsz += opsz;
                        #else
                        opsz = len;
                        #endif
                        sprintf(rs->logs, "t %d/%d \n", opsz, totsz);
                        print_f(rs->plogs, "P4", rs->logs);        
                        memset(addr, 0x95, len);
                    }

                    if (ret == -2) {
                        sprintf(rs->logs, "send len:%d cnt:%d total:%d -loop end\n", opsz, pi, totsz);
                        print_f(rs->plogs, "P4", rs->logs);         
                        break;
                    }
                } else {
                    sprintf(rs->logs, "wait for buffer, ret:%d\n", ret);
                    print_f(rs->plogs, "P4", rs->logs);         
                }

                if (ch != 'I') {
                    ch = 0;
                    rs_ipc_get(rs, &ch, 1);
                }
            }

            clock_gettime(CLOCK_REALTIME, rs->tm[1]);

            tdiff = time_diff(rs->tm[0], rs->tm[1], 1000);

            rs->pmch->get.info = totsz;
            
            flsize = totsz;
            fltime = tdiff;
            sprintf(rs->logs, "time:%d us, totsz:%d bytes, thoutghput: %f MBits\n", tdiff, totsz, (flsize*8)/fltime);
            print_f(rs->plogs, "P4", rs->logs);

            while (ch != 'I') {
                sprintf(rs->logs, "%c clr\n", ch);
                print_f(rs->plogs, "P4", rs->logs);         
                ch = 0;
                rs_ipc_get(rs, &ch, 1);
            }

            rs_ipc_put(rs, "I", 1);
            sprintf(rs->logs, "spi send cnt:%d total:%d- end\n", pi, totsz);
            print_f(rs->plogs, "P4", rs->logs);      
        }

    }

    p4_end(rs);
    return 0;
}

static int p5(struct procRes_s *rs, struct procRes_s *rcmd)
{
    char ch, *tx, *rx, *addr=0;
    uint16_t *tx16, *rx16;
    int len, cmode, ret, pi=1;
    uint32_t bitset;
    int totsz=0, opsz=0;

    sprintf(rs->logs, "p5\n");
    print_f(rs->plogs, "P5", rs->logs);

    p5_init(rs);

    rs_ipc_put(rcmd, "poll", 4);

    tx = malloc(64);
    rx = malloc(64);

    int i;
    for (i = 0; i < 64; i+=4) {
        tx[i] = 0xaa;
        tx[i+1] = 0x55; 
        tx[i+2] = 0xff; 
        tx[i+3] = 0xee; 
    }

    tx16 = (uint16_t *)tx;
    rx16 = (uint16_t *)rx;

    while (1) {
        //sprintf(rs->logs, "#\n");
       //print_f(rs->plogs, "P5", rs->logs);

        len = rs_ipc_get(rs, &ch, 1);
        if (len > 0) {
            //sprintf(rs->logs, "%c \n", ch);
            //print_f(rs->plogs, "P5", rs->logs);

            switch (ch) {
                case 'g':
                    cmode = 1;
                    break;
                case 'c':
                    cmode = 2;
                case 'e':
                    pi = 1;
                case 'r':
                    cmode = 3;
                    break;
                case 'b':
                    cmode = 4;
                    break;
                case 'i':
                    cmode = 5;
                    break;
                default:
                    break;
            }
        }

        if (cmode == 1) {
            while (1) {
                bitset = 1;
                ioctl(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
                sprintf(rs->logs, "Get RDY pin: %d\n", bitset);
                print_f(rs->plogs, "P5", rs->logs);
                if (bitset == 0) break;
            }
            if (!bitset) rs_ipc_put(rs, "G", 1);
        } else if (cmode == 2) {
            int bits = 16;
            ret = ioctl(rs->spifd, SPI_IOC_WR_BITS_PER_WORD, &bits);
            if (ret == -1) {
                sprintf(rs->logs, "can't set bits per word"); 
                print_f(rs->plogs, "P5", rs->logs);
            }
            ret = ioctl(rs->spifd, SPI_IOC_RD_BITS_PER_WORD, &bits); 
            if (ret == -1) {
                sprintf(rs->logs, "can't get bits per word"); 
                print_f(rs->plogs, "P5", rs->logs);
            }

            len = 0;
            len = mtx_data_16(rs->spifd, rx16, tx16, 1, 1, 64);
            if (len > 0) rs_ipc_put(rs, "C", 1);
            sprintf(rs->logs, "16Bits get: %.8x\n", *rx16);
            print_f(rs->plogs, "P5", rs->logs);
        } else if (cmode == 3) {
            int size=0, opsz;
            char *addr;
            size = ring_buf_cons_dual(rs->pdataRx, &addr, pi);
            if (size >= 0) {
                //sprintf(rs->logs, "cons 0x%x %d %d \n", addr, size, pi);
                //print_f(rs->plogs, "P5", rs->logs);
                 
                msync(addr, size, MS_SYNC);
                /* send data to wifi socket */
                //opsz = write(rs->psocket_t->connfd, addr, size);
                opsz = mtx_data(rs->spifd, NULL, addr, 1, size, 1024*1024);
                //printf("socket tx %d %d\n", rs->psocket_r->connfd, opsz);
                sprintf(rs->logs, "%d/%d\n", opsz, size);
                print_f(rs->plogs, "P5", rs->logs);

                pi+=2;
                rs_ipc_put(rs, "r", 1);
            } else {
                sprintf(rs->logs, "socket tx %d %d - end\n", opsz, size);
                print_f(rs->plogs, "P5", rs->logs);
                rs_ipc_put(rs, "e", 1);
            }
        } else if (cmode == 4) {
            bitset = 0;
            ioctl(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
            sprintf(rs->logs, "Check if RDY pin == 1 (pin:%d)\n", bitset);
            print_f(rs->plogs, "P5", rs->logs);

            while (1) {
                bitset = 0;
                ioctl(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
                //sprintf(rs->logs, "Get RDY pin: %d\n", bitset);
                //print_f(rs->plogs, "P4", rs->logs);

                if (bitset == 1) break;
            }
            if (bitset) rs_ipc_put(rs, "B", 1);
        }
        else if (cmode == 5) {
            totsz = 0;
            pi = 0;
            while (1) {
                ret = ring_buf_cons(rs->pcmdTx, &addr, &len);
                if ((ret == 0) ||(ret == -2)) {
                    if (len > 0) {
                        pi++;                    
                        msync(addr, len, MS_SYNC);
                        #if ENABLE_SPI1 /*debug*/
                        opsz = mtx_data(rs->spifd, addr, addr, 1, len, 1024*1024);  
                        totsz += opsz;
                        #else
                        opsz = len;
                        #endif
                        sprintf(rs->logs, "t %d/%d \n", opsz, totsz);
                        print_f(rs->plogs, "P5", rs->logs);        
                        memset(addr, 0x95, len);
                    }

                    if (ret == -2) {
                        sprintf(rs->logs, "send len:%d cnt:%d total:%d -loop end\n", opsz, pi, totsz);
                        print_f(rs->plogs, "P5", rs->logs);         
                        break;
                    }
                } else {
                    sprintf(rs->logs, "wait for buffer, ret:%d\n", ret);
                    print_f(rs->plogs, "P5", rs->logs);         
                }

                if (ch != 'I') {
                    ch = 0;
                    rs_ipc_get(rs, &ch, 1);
                }
            }

            while (ch != 'I') {
                sprintf(rs->logs, "%c clr\n", ch);
                print_f(rs->plogs, "P5", rs->logs);         
                ch = 0;
                rs_ipc_get(rs, &ch, 1);
            }

            rs_ipc_put(rs, "I", 1);
            sprintf(rs->logs, "spi send cnt:%d total:%d- end\n", pi, totsz);
            print_f(rs->plogs, "P5", rs->logs);      
        }
    }

    p5_end(rs);
    return 0;
}

int main(int argc, char *argv[])
{
//char diskname[128] = "/mnt/mmc2/disk_rmImg.bin";
//char diskname[128] = "/mnt/mmc2/disk_rx127_255log.bin";
//char diskname[128] = "/mnt/mmc2/disk_golden.bin";
//char diskname[128] = "/mnt/mmc2/debug_fat.bin";
//char diskname[128] = "/mnt/mmc2/disk_05.bin";
//char diskname[128] = "/dev/mmcblk0p1";
//char diskname[128] = "/dev/mmcblk0";
//char diskname[128] = "/mnt/mmc2/empty_256.dsk";
//char diskname[128] = "/mnt/mmc2/folder_256.dsk";
//char diskname[128] = "/mnt/mmc2/disk_LFN_64.bin";
char diskname[128] = "/mnt/mmc2/disk_onefolder.bin";
static char spi1[] = "/dev/spidev32766.0"; 
static char spi0[] = "/dev/spidev32765.0"; 

    struct mainRes_s *pmrs;
    struct procRes_s rs[7];
    int ix, ret;
    char *log;
    int tdiff, speed;
    int arg[8];
    uint32_t bitset;

    pmrs = (struct mainRes_s *)mmap(NULL, sizeof(struct mainRes_s), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);;

    if (argc > 1) {
        sprintf(pmrs->filein, "%s", argv[1]);
    } else {
        pmrs->filein[0] = '\0';
    }
    printf("Get file input: [%s] \n", pmrs->filein);

    infpath = pmrs->filein;

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
    pmrs->dataRx.pp = memory_init(&pmrs->dataRx.slotn, 4096*SPI_TRUNK_SZ, SPI_TRUNK_SZ);
    if (!pmrs->dataRx.pp) goto end;
    pmrs->dataRx.r = (struct ring_p *)mmap(NULL, sizeof(struct ring_p), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    pmrs->dataRx.totsz = 4096*SPI_TRUNK_SZ;
    pmrs->dataRx.chksz = SPI_TRUNK_SZ;
    pmrs->dataRx.svdist = 16;
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
    pmrs->dataTx.svdist = 8;
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
    pmrs->cmdRx.svdist = 8;
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
    pmrs->cmdTx.pp = memory_init(&pmrs->cmdTx.slotn, 256*SPI_TRUNK_SZ, SPI_TRUNK_SZ);
    if (!pmrs->cmdTx.pp) goto end;
    pmrs->cmdTx.r = (struct ring_p *)mmap(NULL, sizeof(struct ring_p), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    pmrs->cmdTx.totsz = 256*SPI_TRUNK_SZ;
    pmrs->cmdTx.chksz = SPI_TRUNK_SZ;
    pmrs->cmdTx.svdist = 8;
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

    pmrs->mchine.fdsk.vsd = fopen(diskname, "r");
    if (pmrs->mchine.fdsk.vsd == NULL) {
        sprintf(pmrs->log, "disk file [%s] open failed!!! \n", diskname);
        print_f(&pmrs->plog, "FDISK", pmrs->log);
        goto end;
    }

    ret = fseek(pmrs->mchine.fdsk.vsd, 0, SEEK_END);
    if (ret) {
        sprintf(pmrs->log, "seek file [%s] failed!!! \n", diskname);
        print_f(&pmrs->plog, "FDISK", pmrs->log);
        goto end;
    }

    pmrs->mchine.fdsk.rtMax = ftell(pmrs->mchine.fdsk.vsd);
    //pmrs->mchine.fdsk.rtMax = -1;
    if (!pmrs->mchine.fdsk.rtMax) {
        sprintf(pmrs->log, "get file [%s] size failed!!! \n", diskname);
        print_f(&pmrs->plog, "FDISK", pmrs->log);
        goto end;
    } else if (pmrs->mchine.fdsk.rtMax < 0) {
        pmrs->mchine.fdsk.rtMax = 16 * 1024 * 1024;
        sprintf(pmrs->log, "size is unknown, set file [%s] size : %d!!! \n", diskname, pmrs->mchine.fdsk.rtMax);
        print_f(&pmrs->plog, "FDISK", pmrs->log);
        goto end;
    }else {
        sprintf(pmrs->log, "get file [%s] size : %d!!! \n", diskname, pmrs->mchine.fdsk.rtMax);
        print_f(&pmrs->plog, "FDISK", pmrs->log);
    }

    pmrs->mchine.fdsk.sdt = mmap(NULL, pmrs->mchine.fdsk.rtMax, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);;
    if (!pmrs->mchine.fdsk.sdt) {
        sprintf(pmrs->log, "memory alloc for [%s] size : %d, failed!!! \n", diskname, pmrs->mchine.fdsk.rtMax);
        print_f(&pmrs->plog, "FDISK", pmrs->log);
        goto end;
    }

    ret = fseek(pmrs->mchine.fdsk.vsd, 0, SEEK_SET);
    if (ret) {
        sprintf(pmrs->log, "seek for [%s] to 0 failed!!! \n", diskname);
        print_f(&pmrs->plog, "FDISK", pmrs->log);
        goto end;
    }

    ret = fread(pmrs->mchine.fdsk.sdt, 1, pmrs->mchine.fdsk.rtMax, pmrs->mchine.fdsk.vsd);
    sprintf(pmrs->log, "read file size: %d/%d \n", ret, pmrs->mchine.fdsk.rtMax);
    print_f(&pmrs->plog, "FDISK", pmrs->log);

    fclose(pmrs->mchine.fdsk.vsd);
    pmrs->mchine.fdsk.vsd = 0;

    ret = file_save_get(&pmrs->fs, "/mnt/mmc2/rx/%d.bin");
    if (ret) {printf("get save file failed\n"); return 0;}
    //ret = fwrite("test file write \n", 1, 16, pmrs->fs);
    sprintf(pmrs->log, "write file size: %d/%d\n", ret, 16);
    print_f(&pmrs->plog, "fwrite", pmrs->log);

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
    /*
     * spi speed 
     */ 

    bitset = 0;
    ioctl(pmrs->sfm[0], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL

    bitset = 1;
    ioctl(pmrs->sfm[1], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL

    speed = SPI_SPEED;
    ret = ioctl(pmrs->sfm[0], SPI_IOC_WR_MAX_SPEED_HZ, &speed);   //?�t�v 
    if (ret == -1) 
        printf("can't set max speed hz\n"); 
    else 
        printf("set spi0 max speed: %d hz\n", speed); 
        
    ret = ioctl(pmrs->sfm[1], SPI_IOC_WR_MAX_SPEED_HZ, &speed);   //?�t�v 
    if (ret == -1) 
        printf("can't set max speed hz\n"); 
    else 
        printf("set spi1 max speed: %d hz\n", speed); 

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
	
    pipe2(pmrs->pipeup[0].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[1].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[2].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[3].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[4].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[5].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[6].rt, O_NONBLOCK);

    res_put_in(&rs[0], pmrs, 0);
    res_put_in(&rs[1], pmrs, 1);
    res_put_in(&rs[2], pmrs, 2);
    res_put_in(&rs[3], pmrs, 3);
    res_put_in(&rs[4], pmrs, 4);
    res_put_in(&rs[5], pmrs, 5);
    res_put_in(&rs[6], pmrs, 6);
	
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
                            p0(pmrs);
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

static int print_f(struct logPool_s *plog, char *head, char *str)
{
    int len;
    char ch[256];
    if(!str) return (-1);

    if (head)
        sprintf(ch, "[%s] %s", head, str);
    else 
        sprintf(ch, "%s", str);

    printf("%s",ch);

    if (!plog) return (-2);
	
    msync(plog, sizeof(struct logPool_s), MS_SYNC);
    len = strlen(ch);
    if ((len + plog->len) > plog->max) return (-3);
    memcpy(plog->cur, ch, strlen(ch));
    plog->cur += len;
    plog->len += len;

    //if (!mlog) return (-4);
    //fwrite(ch, 1, strlen(ch), mlog);
    //fflush(mlog);
    //fprintf(mlog, "%s", ch);
	
    return 0;
}

static int printf_flush(struct logPool_s *plog, FILE *f) 
{
    msync(plog, sizeof(struct logPool_s), MS_SYNC);
    if (plog->cur == plog->pool) return (-1);
    if (plog->len > plog->max) return (-2);

    msync(plog->pool, plog->len, MS_SYNC);
    fwrite(plog->pool, 1, plog->len, f);
    fflush(f);

    plog->cur = plog->pool;
    plog->len = 0;
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

    diff = (lpast - lnow)/gunit;

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
            sprintf(flog, "open file [%s]\n", dst);
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
    rs->fdat_s = mrs->fdat;
    rs->tm[0] = &mrs->time[0];
    rs->tm[1] = &mrs->time[1];

    rs->ppipedn = &mrs->pipedn[idx];
    rs->ppipeup = &mrs->pipeup[idx];
    rs->plogs = &mrs->plog;

    if((idx == 0) || (idx == 1) || (idx == 3)) {
        rs->spifd = mrs->sfm[0];
    } else if ((idx == 2) || (idx == 4)) {
        rs->spifd = mrs->sfm[1];
    }

    rs->psocket_r = &mrs->socket_r;
    rs->psocket_t = &mrs->socket_t;	

    rs->pmch = &mrs->mchine;
    rs->pregtb = mrs->regTable;
    rs->psd_init = &mrs->sd_init;
    return 0;
}


