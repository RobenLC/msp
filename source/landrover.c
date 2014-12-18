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
#define SPI_CPHA  0x01          /* clock phase */
#define SPI_CPOL  0x02          /* clock polarity */
#define SPI_MODE_0		(0|0)
#define SPI_MODE_1		(0|SPI_CPHA)
#define SPI_MODE_2		(SPI_CPOL|0)
#define SPI_MODE_3		(SPI_CPOL|SPI_CPHA)
#define SPI_SPEED    60000000

#define OP_PON 0x1
#define OP_QRY 0x2
#define OP_RDY 0x3
#define OP_DAT 0x4
#define OP_SCM 0x5
#define OP_DCM 0x6
#define OP_FIH  0x7

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
    uint32_t   infocnt;
};

struct machineCtrl_s{
    uint32_t seqcnt;
    struct info16Bit_s cur;
    struct info16Bit_s get;
};

struct mainRes_s{
    int sid[6];
    int sfm[2];
    int smode;
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

inline uint16_t abs_info(struct info16Bit_s *p, uint16_t info)
{
    p->data = info & 0xff;
    p->opcode = (info >> 8) & 0xf;
    p->seqnum = (info >> 12) & 0x7;
    p->inout = (info >> 15) & 0x1;

    return info;
}

inline uint16_t pkg_info(struct info16Bit_s *p)
{
    uint16_t info = 0;
    info |= p->data & 0xff;
    info |= (p->opcode & 0xf) << 8;
    info |= (p->seqnum & 0x7) << 12;
    info |= (p->inout & 0x1) << 15;

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
                next = PSWT;
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
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "spy", str); 
                //next = PSMAX;
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
                evt = 0x1; /* jump to next stage */
                break;
            default:
                sprintf(str, "default\n"); 
                print_f(mlogPool, "bullet", str); 
                next = PSSET;
                break;
        }
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
                evt = 0x1; /* jump to next stage */
                break;
            default:
                sprintf(str, "default\n"); 
                print_f(mlogPool, "laser", str); 
                next = PSSET;
                break;
        }
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

    sprintf(str, "op_02 - rlt:0x%.8x \n", rlt); 
    print_f(mlogPool, "spy", str); 

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

    sprintf(str, "op_04 - rlt:0x%.8x \n", rlt); 
    print_f(mlogPool, "spy", str); 

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
    rlt = abs_result(data->result);	

    sprintf(str, "op_05 - rlt:0x%.8x \n", rlt); 
    print_f(mlogPool, "spy", str); 

    switch (rlt) {
        case STINIT:
            ch = 9; 
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
    sprintf(str, "op_01: rlt: %.8x result: %.8x ans:%d\n", rlt, data->result, data->ansp0); 
    print_f(mlogPool, "bullet", str);  

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
    sprintf(str, "op_02: rlt: %.8x result: %.8x ans:%d\n", rlt, data->result, data->ansp0); 
    print_f(mlogPool, "bullet", str);  

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
    sprintf(str, "op_02: rlt: %.8x result: %.8x ans:%d\n", rlt, data->result, data->ansp0);  
    print_f(mlogPool, "laser", str);  

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
    sprintf(str, "op_05: rlt: %.8x result: %.8x ans:%d\n", rlt, data->result, data->ansp0);  
    print_f(mlogPool, "laser", str);  

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
    sprintf(str, "get d:%d, %d /%d \n", dist, dualn, folwn);
    print_f(mlogPool, "ring", str);

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
    sprintf(str, "get d:%d, %d \n", dist, leadn, folwn);
    print_f(mlogPool, "ring", str);

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

    sprintf(str, "cons, d: %d %d/%d \n", dist, leadn, folwn);
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
        if ((pp->r->folw.run == pp->r->lead.run) &&
            (pp->r->folw.seq == pp->r->lead.seq)) {
            return pp->lastsz;
        }
    }

    msync(pp, sizeof(struct shmem_s), MS_SYNC);

    return pp->chksz;
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
    if (ret < 0)
        printf("can't send spi message\n");
    
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
    munmap(mrs->dataRx.pp[0], 1024*61440);
    munmap(mrs->dataTx.pp[0], 256*61440);
    munmap(mrs->cmdRx.pp[0], 256*61440);
    munmap(mrs->cmdTx.pp[0], 512*61440);
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
        return 1;
    }
    return 0; 
}
static int fs03(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p;

    //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    //print_f(&mrs->plog, "fs03", mrs->log);

    p = &mrs->mchine.cur;

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = OP_PON;
    p->inout = 0;
    p->seqnum = mrs->mchine.seqcnt;
	
    mrs_ipc_put(mrs, "c", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
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
        return 1;
    }
    return 0; 
}

static int fs07(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p;

    p = &mrs->mchine.cur;
    //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    //print_f(&mrs->plog, "fs07", mrs->log);

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
        //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        //print_f(&mrs->plog, "fs08", mrs->log);

        if (p->opcode == OP_RDY) {
            return 1;
        } else {
            modersp->m = modersp->m - 1;        
            return 2;
        }
    }
    return 0; 
}

static int fs09(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p;

    //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    //print_f(&mrs->plog, "fs09", mrs->log);

    p = &mrs->mchine.cur;

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = OP_QRY;
    p->inout = 0;
    p->seqnum = mrs->mchine.seqcnt;
	
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
        //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        //print_f(&mrs->plog, "fs10", mrs->log);

        if (p->opcode == OP_DAT) {
            return 1;
        } else if (p->opcode == OP_DCM) {
            return 1;
        } else {
            modersp->m = modersp->m - 1;        
            return 2;
        }
    }
    return 0; 
}

static int fs11(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p;
    //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    //print_f(&mrs->plog, "fs11", mrs->log);

    p = &mrs->mchine.cur;

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = OP_DAT;
    p->inout = 0;
    p->seqnum = mrs->mchine.seqcnt;
	
    mrs_ipc_put(mrs, "c", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs12(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        //print_f(&mrs->plog, "fs12", mrs->log);

        if (p->opcode == OP_DAT) {
            return 1;
        } else {
            modersp->m = modersp->m - 1;        
            return 2;
        }
    }
    return 0; 
}

static int fs13(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset=0, ret;
    bitset = 1;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi0 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs13", mrs->log);
    bitset = 1;
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
            return 2;
        }
        if (ch == 'e') {
            sprintf(mrs->log, "get ch:%c \n", ch);
            print_f(&mrs->plog, "fs16", mrs->log);

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
            return 2;
        }
    }
    return 0; 
}

static int fs19(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset;
    bitset = 0;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    //printf("Set spi 0 slave ready: %d\n", bitset);
    sprintf(mrs->log, "Set spi 0 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs19", mrs->log);

    bitset = 0;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    //printf("Set spi 1 slave ready: %d\n", bitset);
    sprintf(mrs->log, "Set spi 1 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs19", mrs->log);

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

    sleep(1);

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

    bitset = 0;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 0 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs20", mrs->log);

    bitset = 0;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 1 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs20", mrs->log);

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

    return 1;
}

static int p0(struct mainRes_s *mrs)
{
#define PS_NUM 24
    int len, tmp;
    char str[128], ch;

    struct modersp_s modesw;
    struct fselec_s afselec[PS_NUM] = {{ 0, fs00},{ 1, fs01},{ 2, fs02},{ 3, fs03},{ 4, fs04},
                                 { 5, fs05},{ 6, fs06},{ 7, fs07},{ 8, fs08},{ 9, fs09},
                                 {10, fs10},{11, fs11},{12, fs12},{13, fs13},{14, fs14},
                                 {15, fs15},{16, fs16},{17, fs17},{18, fs18},{19, fs19},
                                 {20, fs20},{21, fs21},{22, fs22},{23, fs23},
                                };

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
            modesw.r = (*afselec[modesw.m].pfunc)(mrs, &modesw);
            //sprintf(mrs->log, "pmode:%d rsp:%d\n", modesw.m, modesw.r);
            //print_f(&mrs->plog, "P0", mrs->log);
            if (modesw.r == 1) {
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

            mrs_ipc_put(mrs, &ch, 1, 0);
        } else {
            mrs_ipc_put(mrs, "$", 1, 0);
        }

        usleep(100);
        //sleep(2);
    }

    // save to file for debug
    //if (pmode == 1) {
    //   msync(mrs->dataRx.pp[0], 1024*61440, MS_SYNC);
    //    ret = fwrite(mrs->dataRx.pp[0], 1, 1024*61440, mrs->fs);
    //    printf("\np0 write file %d size %d/%d \n", mrs->fs, 1024*61440, ret);
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
    stfunc pf[SMAX][PSMAX] = {{stspy_01, stspy_02, stspy_03, stspy_04, stspy_05},
                            {stbullet_01, stbullet_02, stbullet_03, stbullet_04, stbullet_05},
                            {stlaser_01, stlaser_02, stlaser_03, stlaser_04, stlaser_05}};

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
        //usleep(100000);
    }

    p1_end(rs);
    return 0;
}

static int p2(struct procRes_s *rs)
{
    /* spi0 */
    char ch;
    int len;
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
                int totsz=0, fsize=0, pi=0, len;
                char *addr;
                //char filename[128] = "/mnt/mmc2/sample1.mp4";
                char filename[128] = "/mnt/mmc2/pattern2.txt";
                FILE *fp;

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

static int p4(struct procRes_s *rs)
{
    char ch, *tx, *rx;
    uint16_t *tx16, *rx16, in16;
    int len = 0, cmode = 0, ret, pi=0;
    uint32_t bitset;

    sprintf(rs->logs, "p4\n");
    print_f(rs->plogs, "P4", rs->logs);

    p4_init(rs);

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
            tx16[0] = pkg_info(&rs->pmch->cur);
            len = mtx_data_16(rs->spifd, rx16, tx16, 1, 2, 1024);
            if (len > 0) {
                in16 = rx16[0];

                sprintf(rs->logs, "16Bits send: 0x%.4x, get: 0x%.4x\n", tx16[0], in16);
                print_f(rs->plogs, "P4", rs->logs);

                abs_info(&rs->pmch->get, in16);
                rs_ipc_put(rs, "C", 1);
            }
            //sprintf(rs->logs, "16Bits send: 0x%.4x, get: 0x%.4x\n", tx16[0], in16);
            //print_f(rs->plogs, "P4", rs->logs);
        } else if (cmode == 3) {
            int size=0, opsz;
            char *addr;
            size = ring_buf_cons_dual(rs->pdataRx, &addr, pi);
            if (size >= 0) {
                //sprintf(rs->logs, "cons 0x%x %d %d \n", addr, size, pi);
                //print_f(rs->plogs, "P4", rs->logs);
                 
                msync(addr, size, MS_SYNC);
                /* send data to wifi socket */
                //opsz = write(rs->psocket_t->connfd, addr, size);
                opsz = mtx_data(rs->spifd, NULL, addr, 1, size, 1024*1024);
                //printf("socket tx %d %d\n", rs->psocket_r->connfd, opsz);
                //sprintf(rs->logs, "[%d] socket tx %d %d\n", rs->spifd, opsz, size);
                //print_f(rs->plogs, "P4", rs->logs);

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
        }

    }

    p4_end(rs);
    return 0;
}

static int p5(struct procRes_s *rs, struct procRes_s *rcmd)
{
    char ch, *tx, *rx;
    uint16_t *tx16, *rx16;
    int len, cmode, ret, pi=1;
    uint32_t bitset;

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
            len = mtx_data_16(rs->spifd, rx16, tx16, 1, 2, 64);
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
                //sprintf(rs->logs, "[%d] socket tx %d %d\n", rs->spifd, opsz, size);
                //print_f(rs->plogs, "P5", rs->logs);

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

    }

    p5_end(rs);
    return 0;
}

int main(int argc, char *argv[])
{
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
    pmrs->dataRx.pp = memory_init(&pmrs->dataRx.slotn, 1024*61440, 61440);
    if (!pmrs->dataRx.pp) goto end;
    pmrs->dataRx.r = (struct ring_p *)mmap(NULL, sizeof(struct ring_p), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    pmrs->dataRx.totsz = 1024*61440;
    pmrs->dataRx.chksz = 61440;
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
    pmrs->dataTx.pp = memory_init(&pmrs->dataTx.slotn, 1*61440, 61440);
    if (!pmrs->dataTx.pp) goto end;
    pmrs->dataTx.r = (struct ring_p *)mmap(NULL, sizeof(struct ring_p), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    pmrs->dataTx.totsz = 1*61440;
    pmrs->dataTx.chksz = 61440;
    pmrs->dataTx.svdist = 1;
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
    pmrs->cmdRx.pp = memory_init(&pmrs->cmdRx.slotn, 1*61440, 61440);
    if (!pmrs->cmdRx.pp) goto end;
    pmrs->cmdRx.r = (struct ring_p *)mmap(NULL, sizeof(struct ring_p), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    pmrs->cmdRx.totsz = 1*61440;;
    pmrs->cmdRx.chksz = 61440;
    pmrs->cmdRx.svdist = 1;
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
    pmrs->cmdTx.pp = memory_init(&pmrs->cmdTx.slotn, 1*61440, 61440);
    if (!pmrs->cmdTx.pp) goto end;
    pmrs->cmdTx.r = (struct ring_p *)mmap(NULL, sizeof(struct ring_p), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    pmrs->cmdTx.totsz = 1*61440;
    pmrs->cmdTx.chksz = 61440;
    pmrs->cmdTx.svdist = 1;
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
    ret = ioctl(pmrs->sfm[0], SPI_IOC_WR_MAX_SPEED_HZ, &speed);   //?速率 
    if (ret == -1) 
        printf("can't set max speed hz"); 
        
    ret = ioctl(pmrs->sfm[1], SPI_IOC_WR_MAX_SPEED_HZ, &speed);   //?速率 
    if (ret == -1) 
        printf("can't set max speed hz"); 

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
    if((!head) || (!str)) return (-1);

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

    if((idx == 0) || (idx == 1) || (idx == 3)) {
        rs->spifd = mrs->sfm[0];
    } else if ((idx == 2) || (idx == 4)) {
        rs->spifd = mrs->sfm[1];
    }

    rs->psocket_r = &mrs->socket_r;
    rs->psocket_t = &mrs->socket_t;	

    rs->pmch = &mrs->mchine;
    return 0;
}


