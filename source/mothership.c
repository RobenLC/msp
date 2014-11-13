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

static FILE *mlog = 0;
static struct logPool_s *mlogPool;

typedef int (*func)(int argc, char *argv[]);

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
    int result;
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

struct mainRes_s{
    int sid[6];
    int sfm[2];
    int smode;
    // 3 pipe
    struct pipe_s pipedn[6];
    struct pipe_s pipeup[6];
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

struct procRes_s{
    // pipe
    int spifd;
    struct pipe_s *ppipedn;
    struct pipe_s *ppipeup;
    struct shmem_s *pdataRx;
    struct shmem_s *pdataTx;
    struct shmem_s *pcmdRx;
    struct shmem_s *pcmdTx;

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
static int p1(struct procRes_s *rs);
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

static int ps_test(struct psdata_s *data)
{
    data->result += 1;
    if ((data->result & 0xf) == PSMAX) {
        data->result = (data->result & 0xf0) + 0x10;
    }

    if (((data->result & 0xf0) >> 4)== SMAX) {
        data->result = -1;
    }

    return data->result;

}
static int stspy_01(struct psdata_s *data)
{ 
    // keep polling, kind of idle mode
    // jump to next status if receive any op code
    char str[128]; 
    sprintf(str, "spy 01\n"); 
    print_f(mlogPool, "st", str); 

    return ps_test(data);
}
static int stspy_02(struct psdata_s *data) 
{ 
    char str[128]; 
    sprintf(str, "spy 02\n"); 
    print_f(mlogPool, "st", str); 
    return ps_test(data);
}
static int stspy_03(struct psdata_s *data) 
{ 
    char str[128]; 
    sprintf(str, "spy 03\n"); 
    print_f(mlogPool, "st", str); 
    return ps_test(data);
}
static int stspy_04(struct psdata_s *data) 
{ 
    char str[128]; 
    sprintf(str, "spy 04\n"); 
    print_f(mlogPool, "st", str); 
    return ps_test(data);
}
static int stspy_05(struct psdata_s *data) 
{ 
    char str[128]; 
    sprintf(str, "spy 05\n"); 
    print_f(mlogPool, "st", str); 
    return ps_test(data);
}
static int stbullet_01(struct psdata_s *data) 
{ 
    char str[128]; 
    sprintf(str, "bullet 01\n"); 
    print_f(mlogPool, "st", str);  
    return ps_test(data);
}
static int stbullet_02(struct psdata_s *data) 
{ 
    char str[128]; 
    sprintf(str, "bullet 02\n"); 
    print_f(mlogPool, "st", str);  
    return ps_test(data);
}
static int stbullet_03(struct psdata_s *data) 
{ 
    char str[128]; 
    sprintf(str, "bullet 03\n"); 
    print_f(mlogPool, "st", str);  
    return ps_test(data);
}
static int stbullet_04(struct psdata_s *data) 
{ 
    char str[128]; 
    sprintf(str, "bullet 04\n"); 
    print_f(mlogPool, "st", str);  
    return ps_test(data);
}
static int stbullet_05(struct psdata_s *data) 
{ 
    char str[128]; 
    sprintf(str, "bullet 05\n"); 
    print_f(mlogPool, "st", str);  
    return ps_test(data);
}
static int stlaser_01(struct psdata_s *data) 
{ 
    char str[128]; 
    sprintf(str, "laser 01\n"); 
    print_f(mlogPool, "st", str);  
    return ps_test(data);
}
static int stlaser_02(struct psdata_s *data) 
{ 
    char str[128]; 
    sprintf(str, "laser 02\n"); 
    print_f(mlogPool, "st", str);  
    return ps_test(data);
}
static int stlaser_03(struct psdata_s *data) 
{ 
    char str[128]; 
    sprintf(str, "laser 03\n"); 
    print_f(mlogPool, "st", str);  
    return ps_test(data);
}
static int stlaser_04(struct psdata_s *data) 
{ 
    char str[128]; 
    sprintf(str, "laser 04\n"); 
    print_f(mlogPool, "st", str);  
    return ps_test(data);
}
static int stlaser_05(struct psdata_s *data) 
{ 
    char str[128]; 
    sprintf(str, "laser 05\n"); 
    print_f(mlogPool, "st", str);  
    return ps_test(data);
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
    sprintf(str, "d:%d, %d /%d \n", dist, dualn, folwn);
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

    sprintf(str, "[cons], d: %d %d/%d/%d \n", dist, leadn, dualn, folwn);
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
        tr[i].speed_hz = 1000000;
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

    ch = '0';

    if (argc == 2) {
        ch = '3';
    }

    if (argc == 1) {
        ch = '4';
    }

    sprintf(str, "cmdfunc_01 argc:%d [%s] ch:%c\n", argc, argv[0], ch); 
    print_f(mlogPool, "cmdfunc", str);

    mrs_ipc_put(mrs, &ch, 1, 0);
    return 1;
}

static int dbg(struct mainRes_s *mrs)
{
    int ci, pi, ret;
    char cmd[256], *addr[3];

    struct cmd_s cmdtab[8] = {{0, "poll", cmdfunc_01}, {1, "command", cmdfunc_01}, {2, "data", cmdfunc_01}, {3, "run", cmdfunc_01}, 
                                {4, "aspect", cmdfunc_01}, {5, "leo", cmdfunc_01}, {6, "mothership", cmdfunc_01}, {7, "lanch", cmdfunc_01}};

    p0_init(mrs);
	
    while (1) {
        /* command parsing */
        ci = 0;    

        /*
        while (1) {
            cmd[ci] = fgetc(stdin);
            if (cmd[ci] == '\n') {
                cmd[ci] = '\0';
                break;
            }
            ci++;
        }
        */

        ci = mrs_ipc_get(mrs, cmd, 256, 5);

        if (ci > 0) {
            cmd[ci] = '\0';
            sprintf(mrs->log, "get [%s] size:%d \n", cmd, ci);
            print_f(&mrs->plog, "DBG", mrs->log);
        } else {
            printf_flush(&mrs->plog, mrs->flog);
            usleep(500);
            continue;
        }

        pi = 0;
        while (pi < 8) {
            if ((strlen(cmd) != strlen(cmdtab[pi].str))) {
            } else if (!strncmp(cmd, cmdtab[pi].str, strlen(cmdtab[pi].str))) {
                 break;
            }
            pi++;
        }

        /* command execution */
        if (pi == 8) {
            sprintf(mrs->log, "cmd not found!\n");
            print_f(&mrs->plog, "DBG", mrs->log);
        } 
        else if (pi < 8) {
            addr[0] = (char *)mrs;
            sprintf(mrs->log, "input [%d]%s:%d[%s]\n", pi, cmdtab[pi].str, cmdtab[pi].id, cmd);
            print_f(&mrs->plog, "DBG", mrs->log);
            ret = cmdtab[pi].pfunc(cmdtab[pi].id, addr);

            mrs_ipc_put(mrs, "t", 1, 0);
        } else {
            sprintf(mrs->log, "error input cmd: [%d]\n", pi);
            print_f(&mrs->plog, "DBG", mrs->log);
        }

        printf_flush(&mrs->plog, mrs->flog);
    }

    p0_end(mrs);
}

static int p0(struct mainRes_s *mrs)
{
    int pi, pt, pc, tp, ret, sz[3], chk[2], bksz[2], seq[2], bitset, wc;
    char ch, c1, str[128], dst[17];
    char *addr[3], *stop_at;
    int pmode = 0, pstatus[2], cstatus = 0;

    int px, ci;

    p0_init(mrs);
    /* the initial mode is command mode, the rdy pin is pull low at begin */
    // in charge of share memory and processed control
    // put the initial status in shared memory which is the default tx data for command mode
    // send 'c' to p1 to start the command mode
    // send 'c' to p5 to start the socket recv
    // parsing command in shared memory which get from socket 
    // parsing command in shared memory whcih get form spi
    // 
    
    while (!pmode) {
        ret = mrs_ipc_get(mrs, &ch, 1, 0);
        if (ret > 0) {
            sprintf(mrs->log, "ret:%d ch:%c\n", ret, ch);
            print_f(&mrs->plog, "P0", mrs->log);
        }
		
        if (ret <= 0) {
            //sprintf(mrs->log, "ret:%d\n", ret);
            //print_f(&mrs->plog, "P0", mrs->log);
        }
        else if (ch == '2') {
            // set mode to data mode
            pmode = 1;
            bksz[0] = 0;
            bksz[1] = 0;
            mrs->dataRx.lastflg = 0;
            mrs->dataRx.lastsz = 0;
            mrs->dataRx.dualsz = 0;
            pstatus[0] = 0;
            pstatus[1] = 0;
            cstatus = 0;
            bitset = 0;
            ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL

            bitset = 1;
            ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL
        
            bitset = 1;
            ret = ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
            sprintf(mrs->log, "Set spi0 data mode: %d\n", bitset);
            print_f(&mrs->plog, "P0", mrs->log);
 
            bitset = 1;
            ret = ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
            sprintf(mrs->log, "Set spi1 data mode: %d\n", bitset);
            print_f(&mrs->plog, "P0", mrs->log);

            bitset = 0;
            ret = ioctl(mrs->sfm[0], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_RD_CTL_PIN
            sprintf(mrs->log, "Get RDY: %d\n", bitset);
            print_f(&mrs->plog, "P0", mrs->log);

            mrs_ipc_put(mrs, "d", 1, 1);
            mrs_ipc_put(mrs, "d", 1, 2);
        }
        else if (ch == '3') {
            bitset = 0;
            ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL

            bitset = 1;
            ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL
        
            bitset = 1;
            ret = ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
            sprintf(mrs->log, "Set spi0 data mode: %d\n", bitset);
            print_f(&mrs->plog, "P0", mrs->log);
 
            bitset = 1;
            ret = ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
            sprintf(mrs->log, "Set spi1 data mode: %d\n", bitset);
            print_f(&mrs->plog, "P0", mrs->log);

            bitset = 0;
            ret = ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_WT_CTL_PIN
            sprintf(mrs->log, "Set RDY: %d\n", bitset);
            print_f(&mrs->plog, "P0", mrs->log);

            bitset = 0;
            ret = ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_WT_CTL_PIN
            sprintf(mrs->log, "Set RDY: %d\n", bitset);
            print_f(&mrs->plog, "P0", mrs->log);

            pmode = 2;
        }
        else if (ch == '4') { /* app -> wifi -> spi */
            bitset = 0;
            ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL

            bitset = 1;
            ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL
        
            bitset = 0;
            ret = ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
            sprintf(mrs->log, "Set spi0 data mode: %d\n", bitset);
            print_f(&mrs->plog, "P0", mrs->log);
 
            bitset = 0;
            ret = ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
            sprintf(mrs->log, "Set spi1 data mode: %d\n", bitset);
            print_f(&mrs->plog, "P0", mrs->log);

            bitset = 0;
            ret = ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_WT_CTL_PIN
            sprintf(mrs->log, "Set RDY: %d\n", bitset);
            print_f(&mrs->plog, "P0", mrs->log);

            pmode = 3;
        }
        else {
            sprintf(mrs->log, "%c\n", ch);
            print_f(&mrs->plog, "P0", mrs->log);
        }
        usleep(500);
    }
    sprintf(mrs->log, "pmode select: 0x%x \n", pmode);
    print_f(&mrs->plog, "P0", mrs->log);

    pi = 0; pt = 0; pc = 0, seq[0] = 0, seq[1] = 1; wc = 0; 
    chk[0] = 0; chk[1] = 0;
    while (1) {
        //sprintf(mrs->log, ".");
        //print_f(&mrs->plog, "P0", mrs->log);

        // p2 data mode spi0 rx
        
        if (pmode == 1) {
            ret = mrs_ipc_get(mrs, &ch, 1, 1);
            while (ret > 0) {
                if (ch == 'p') {
                    seq[0] += 2;
                    mrs_ipc_put(mrs, "d", 1, 3);
                }
                if (ch == 'd') {
                    sprintf(mrs->log, "0 %d end\n", seq[0]);
                    print_f(&mrs->plog, "P0", mrs->log);
                    mrs_ipc_put(mrs, "d", 1, 3);
                    chk[0] = 1;
                    mrs_ipc_put(mrs, "D", 1, 3);
                }
                ret = mrs_ipc_get(mrs, &ch, 1, 1);
            }
            ret = mrs_ipc_get(mrs, &ch, 1, 2);
            while (ret > 0) {
                if (ch == 'p') {
                    seq[1] += 2;
                    mrs_ipc_put(mrs, "d", 1, 3);
                }
                if (ch == 'd') {
                    sprintf(mrs->log, "1 %d end\n", seq[1]);
                    print_f(&mrs->plog, "P0", mrs->log);

                    mrs_ipc_put(mrs, "d", 1, 3);
                    chk[1] = 1;
                    mrs_ipc_put(mrs, "D", 1, 3);
                }
                ret = mrs_ipc_get(mrs, &ch, 1, 2);
            }
            if (chk[0] && chk[1]) pmode = 0;
        }
        if (pmode == 2) {
            ret = mrs_ipc_get(mrs, &ch, 1, 4);
            if (ret > 0) {
                sprintf(mrs->log, "0 [%c]\n", ch);
                print_f(&mrs->plog, "P0", mrs->log);				

                ret = mrs_ipc_get(mrs, str, 128, 4);
                if (ret > 0) {
                    str[ret] = '\0';
                    sprintf(mrs->log, "[%s] sz:%d\n", str, ret);
                    print_f(&mrs->plog, "P0", mrs->log);

                    // send command to trigger dual spi mode
                    pmode = 1;
                    mrs_ipc_put(mrs, "d", 1, 1);
                    mrs_ipc_put(mrs, "d", 1, 2);
                }
            }
        }
        if (pmode == 3) {
            ret = mrs_ipc_get(mrs, &ch, 1, 4);
            if (ret > 0) {
                sprintf(mrs->log, "0 [%c]\n", ch);
                print_f(&mrs->plog, "P0", mrs->log);				

                ret = mrs_ipc_get(mrs, str, 128, 4);
                if (ret > 0) {
                    str[ret] = '\0';
                    sprintf(mrs->log, "[%s] sz:%d\n", str, ret);
                    print_f(&mrs->plog, "P0", mrs->log);

                    // send command to trigger spi command mode
                    pmode = 4;
                    seq[0] = 0;
                    mrs_ipc_put(mrs, "c", 1, 3);
                }
            }
        }
        if (pmode == 4) {
            ret = mrs_ipc_get(mrs, &ch, 1, 3);
            while (ret > 0) {
                if (ch == 'c') {
                    seq[0] += 1;
                    mrs_ipc_put(mrs, "c", 1, 1);
                }
                if (ch == 'C') {
                    sprintf(mrs->log, "tcp rx %d end\n", seq[0]);
                    print_f(&mrs->plog, "P0", mrs->log);
                    mrs_ipc_put(mrs, "c", 1, 1);
                    chk[0] = 1;
                    mrs_ipc_put(mrs, "C", 1, 1);
                }
                ret = mrs_ipc_get(mrs, &ch, 1, 3);
            }
            if (chk[0]) pmode = 0;
        }
        usleep(10);
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

static int p1(struct procRes_s *rs)
{
    int px, pi, ret, ci;
    char ch, cmd[32];
    char *addr;

    sprintf(rs->logs, "p1\n");
    print_f(rs->plogs, "P1", rs->logs);
    struct psdata_s stdata;
    stfunc pf[SMAX][PSMAX] = {{stspy_01, stspy_02, stspy_03, stspy_04, stspy_05},
                            {stbullet_01, stbullet_02, stbullet_03, stbullet_04, stbullet_05},
                            {stlaser_01, stlaser_02, stlaser_03, stlaser_04, stlaser_05}};

    p1_init(rs);
    // wait for ch from p0
    // state machine control

    pi = 0;
    ch = '1';
    while (1) {
        sprintf(rs->logs, "+\n");
        print_f(rs->plogs, "P1", rs->logs);

        ret = rs_ipc_get(rs, &ch, 1);
        if (ret > 0) {
            sprintf(rs->logs, "%c\n", ch);
            print_f(rs->plogs, "P1", rs->logs);

            stdata.result = 0;
            ret = 0;
            while (1) {
                sprintf(rs->logs, "%x\n", ret);
                print_f(rs->plogs, "P1", rs->logs);
                pi = (ret & 0xf0) >> 4;
                px = (ret & 0xf);
                ret = (*pf[pi][px])(&stdata);
                if (ret == (-1)) break;
            }

        }
        rs_ipc_put(rs, &ch, 1);
        usleep(10);
    }

    p1_end(rs);
    return 0;
}

static int p2(struct procRes_s *rs)
{
    int px, pi, ret[5], size, opsz, tEnd, acusz;
    char ch, dst[17], sz[17], str[128];
    char *addr, *stop_at, *psudorx;
    sprintf(rs->logs, "p2\n");
    print_f(rs->plogs, "P2", rs->logs);
    p2_init(rs);
    // wait for ch from p0
    // in charge of spi0 data mode
    // 'd': data mode, store the incomming infom into share memory
    // send 'd' to notice the p0 that we have incomming data chunk

    psudorx = malloc(61440);

    ch = '2';
    pi = 0; px = 0; tEnd = 0; acusz = 0;
    while (1) {
        //sprintf(rs->logs, "!\n");
        //print_f(rs->plogs, "P2", rs->logs);

        ret[0] = rs_ipc_get(rs, &ch, 1);
        if (ret[0] > 0) {
            //sprintf(rs->logs, "recv ch: %c\n", ch);
            //print_f(rs->plogs, "P2", rs->logs);

            if (ch == 'd') {
                pi = 0;  
                while (1) {
                    size = ring_buf_get_dual(rs->pdataRx, &addr, pi);
                    opsz = mtx_data(rs->spifd, addr, NULL, 1, size, 1024*1024);
                    //printf("0 spi %d\n", opsz);

                    ring_buf_prod_dual(rs->pdataRx, pi);
                    //shmem_dump(addr, 32);

                    if (opsz != size) break;
                    rs_ipc_put(rs, "p", 1);
                    pi += 2;
                }
                ring_buf_set_last_dual(rs->pdataRx, opsz, pi);
                ret[1] = rs_ipc_put(rs, "d", 1);
            }
            if ((ch == 'c') || (ch == 'C')) {
                size = ring_buf_cons(rs->pcmdTx, &addr);
                if (size >= 0) {
                    
                    msync(addr, size, MS_SYNC);
                    /* send data to spi command mode */
                    opsz = mtx_data(rs->spifd, psudorx, addr, 1, size, 1024*1024);
                    //sprintf(rs->logs, "[%d] spi tx %d\n", px, opsz);
                    //print_f(rs->plogs, "P2", rs->logs);
                    px++;

                } else {
                    sprintf(rs->logs, "[%d] spi tx cons ret:%d\n", px, size);
                    print_f(rs->plogs, "P2", rs->logs);
                }
            }
        }
    }

    p2_end(rs);
    return 0;
}

static int p3(struct procRes_s *rs)
{
    int px, pi, ret[5], size, opsz, tEnd, acusz;
    char ch, dst[17], sz[17], str[128];
    char *addr, *stop_at;
    sprintf(rs->logs, "p3\n");
    print_f(rs->plogs, "P3", rs->logs);

    p3_init(rs);
    // wait for ch from p0
    // in charge of spi1 data mode
    // 'd': data mode, forward the socket incoming inform into share memory

    pi = 0; px = 0; tEnd = 0;
    while (1) {
        sprintf(rs->logs, "/\n");
        print_f(rs->plogs, "P3", rs->logs);

        ret[0] = rs_ipc_get(rs, &ch, 1);
        if (ret[0] > 0) {
            sprintf(rs->logs, "recv ch: %c\n", ch);
            print_f(rs->plogs, "P3", rs->logs);

            if (ch == 'd') {

                pi = 1;  
                while (1) {
                    size = ring_buf_get_dual(rs->pdataRx, &addr, pi);

                    opsz = mtx_data(rs->spifd, addr, NULL, 1, size, 1024*1024);
                    //sprintf(rs->logs, "1 spi %d\n", opsz);
                    //print_f(rs->plogs, "P5", rs->logs);

                    //shmem_dump(addr, 32);
                    ring_buf_prod_dual(rs->pdataRx, pi);

                    if (opsz != size) break;
                    rs_ipc_put(rs, "p", 1);
                    pi += 2;
                }
                ring_buf_set_last_dual(rs->pdataRx, opsz, pi);
                ret[1] = rs_ipc_put(rs, "d", 1);

            }
        }
    }

    p3_end(rs);
    return 0;
}

static int p4(struct procRes_s *rs)
{
    int px, pi, ret, size, opsz, cstatus, cmode, n, acuhk, errtor=0;
    char ch, dst[17], sz[17], str[128];
    char *addr, *stop_at;
    sprintf(rs->logs, "p4\n");
    print_f(rs->plogs, "P4", rs->logs);

    p4_init(rs);
    // wait for ch from p0
    // in charge of socket send

    int acusz;
    char *recvbuf, *tmp;
    acusz = 0;

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
    memset(&rs->psocket_t->serv_addr, '0', sizeof(struct sockaddr_in));

    rs->psocket_t->serv_addr.sin_family = AF_INET;
    rs->psocket_t->serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    rs->psocket_t->serv_addr.sin_port = htons(6000); 

    bind(rs->psocket_t->listenfd, (struct sockaddr*)&rs->psocket_t->serv_addr, sizeof(struct sockaddr_in)); 
    listen(rs->psocket_t->listenfd, 10); 

    while (1) {
        //printf("^");
        sprintf(rs->logs, "^\n");
        print_f(rs->plogs, "P4", rs->logs);

        rs->psocket_t->connfd = accept(rs->psocket_t->listenfd, (struct sockaddr*)NULL, NULL); 

        if (rs->psocket_t->connfd < 0) {
            sprintf(rs->logs, "connect failed [%d]\n", rs->psocket_t->connfd);
            print_f(rs->plogs, "P4", rs->logs);
			
            continue;
        }
//        n = read(rs->psocket_t->connfd, recvbuf, 1024);
        n = 0;
        recvbuf[0] = '\0';
        sprintf(rs->logs, "socket connected %d\n", rs->psocket_t->connfd);
        print_f(rs->plogs, "P4", rs->logs);

        pi = 0;
        ret = rs_ipc_get(rs, &ch, 1);
        while (ret > 0) {
            //printf("%c ", ch);
            if ((ch == 'd') || (ch == 'D')) {
                size = ring_buf_cons_dual(rs->pdataRx, &addr, pi);
                if (size >= 0) {
                    //printf("cons 0x%x %d %d \n", addr, size, pi);
                    pi++;
                    
                    msync(addr, size, MS_SYNC);
                    /* send data to wifi socket */
                    opsz = write(rs->psocket_t->connfd, addr, size);
                    //printf("socket tx %d %d\n", rs->psocket_r->connfd, opsz);
                }
            }
            else if (ch == 'c') {
                px = 0;

                size = ring_buf_get(rs->pcmdTx, &addr);
                acuhk = 0;
                n = read(rs->psocket_t->connfd, addr, size);
                while (n <= 0) {
                    //sprintf(rs->logs, "[wait] socket receive %d/%d bytes from %d\n", px, n, size, rs->psocket_t->connfd);
                    //print_f(rs->plogs, "P4", rs->logs);
                    n = read(rs->psocket_t->connfd, addr, size);
                }

                addr += n;
                acuhk += n;
                while ((n < size) && (n > 0)) {
                    size = size - n;

                    errtor = 0;
                    n = read(rs->psocket_t->connfd, addr, size);
                    while (n <= 0) {
                        if (errtor > 3) break;
                        n = read(rs->psocket_t->connfd, addr, size);
                        errtor ++;
                    }

                    addr += n;
                    acuhk += n;
                }
				
                //sprintf(rs->logs, "[%d] socket receive %d/%d bytes from %d, n:%d\n", px, acuhk, size, rs->psocket_t->connfd, n);
                //print_f(rs->plogs, "P4", rs->logs);
                px++;
                ring_buf_prod(rs->pcmdTx);
                rs_ipc_put(rs, "c", 1);

                if (n >= 0) {
                while (1) {
                    size = ring_buf_get(rs->pcmdTx, &addr);

                    acuhk = 0;

                    errtor = 0;
                    n = read(rs->psocket_t->connfd, addr, size);
                    while (n <= 0) {
                        if (errtor > 3) break;
                        n = read(rs->psocket_t->connfd, addr, size);
                        errtor ++;
                    }

                    addr += n;
                    acuhk += n;
                    while ((n < size) && (n > 0)) {
                        size = size - n;

                        errtor = 0;
                        n = read(rs->psocket_t->connfd, addr, size);
                        while (n <= 0) {
                            if (errtor > 3) break;
                            n = read(rs->psocket_t->connfd, addr, size);
                            errtor ++;
                        }

                        addr += n;
                        acuhk += n;
                    }
                    
                    //sprintf(rs->logs, "[%d] socket receive %d/%d bytes from %d n:%d\n", px, acuhk, size, rs->psocket_t->connfd, n);
                    //print_f(rs->plogs, "P4", rs->logs);
                    px++;

                    ring_buf_prod(rs->pcmdTx);
                    if (n <= 0) break;
                    //shmem_dump(addr, 32);

                    rs_ipc_put(rs, "c", 1);
                }
                }
                ring_buf_set_last(rs->pcmdTx, acuhk);

                sprintf(rs->logs, "[%d] socket receive %d/%d bytes end\n", px, n, size);
                print_f(rs->plogs, "P4", rs->logs);

                rs_ipc_put(rs, "C", 1);
                break;
            }
            ret = rs_ipc_get(rs, &ch, 1);
        }

        close(rs->psocket_t->connfd);
        //rs->psocket_r->connfd = 0;
    }

    p4_end(rs);
    return 0;
}

static int p5(struct procRes_s *rs, struct procRes_s *rcmd)
{
    int px, pi, ret, n, size, opsz, acusz;
    char ch, *recvbuf, *addr;
    sprintf(rs->logs, "p5\n");
    print_f(rs->plogs, "P5", rs->logs);

    p5_init(rs);
    // wait for ch from p0
    // in charge of socket recv

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
    memset(&rs->psocket_r->serv_addr, '0', sizeof(struct sockaddr_in));

    rs->psocket_r->serv_addr.sin_family = AF_INET;
    rs->psocket_r->serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    rs->psocket_r->serv_addr.sin_port = htons(5000); 

    bind(rs->psocket_r->listenfd, (struct sockaddr*)&rs->psocket_r->serv_addr, sizeof(struct sockaddr_in)); 
    listen(rs->psocket_r->listenfd, 10); 

    while (1) {
        //printf("#");
        sprintf(rs->logs, "#\n");
        print_f(rs->plogs, "P5", rs->logs);

        rs->psocket_r->connfd = accept(rs->psocket_r->listenfd, (struct sockaddr*)NULL, NULL); 

        memset(recvbuf, 0x0, 1024);

        n = read(rs->psocket_r->connfd, recvbuf, 1024);
        if (recvbuf[n-1] == '\n')         recvbuf[n-1] = '\0';
        sprintf(rs->logs, "socket receive %d char [%s] from %d\n", n, recvbuf, rs->psocket_r->connfd);
        print_f(rs->plogs, "P5", rs->logs);

        rs_ipc_put(rs, "s", 1);
        if (n > 0) {
            rs_ipc_put(rs, recvbuf, n);
            sprintf(rs->logs, "send to p0 [%s]\n", recvbuf);
            print_f(rs->plogs, "P5", rs->logs);
            
            ret = write(rs->psocket_r->connfd, recvbuf, n);
            sprintf(rs->logs, "send back app [%s] size:%d/%d\n", recvbuf, ret, n);
            print_f(rs->plogs, "P5", rs->logs);

            rs_ipc_put(rcmd, recvbuf, n);
        }

        close(rs->psocket_r->connfd);
        //rs->psocket_r->connfd = 0;
    }

    p5_end(rs);
    return 0;
}

int main(int argc, char *argv[])
{
static char spi1[] = "/dev/spidev32766.0"; 
static char spi0[] = "/dev/spidev32765.0"; 

    struct mainRes_s *pmrs;
    struct procRes_s rs[6];
    int ix, ret;
    char *log;
    int tdiff;
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
    pmrs->dataTx.pp = memory_init(&pmrs->dataTx.slotn, 256*61440, 61440);
    if (!pmrs->dataTx.pp) goto end;
    pmrs->dataTx.r = (struct ring_p *)mmap(NULL, sizeof(struct ring_p), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    pmrs->dataTx.totsz = 256*61440;
    pmrs->dataTx.chksz = 61440;
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
    pmrs->cmdRx.pp = memory_init(&pmrs->cmdRx.slotn, 256*61440, 61440);
    if (!pmrs->cmdRx.pp) goto end;
    pmrs->cmdRx.r = (struct ring_p *)mmap(NULL, sizeof(struct ring_p), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    pmrs->cmdRx.totsz = 256*61440;;
    pmrs->cmdRx.chksz = 61440;
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
    pmrs->cmdTx.pp = memory_init(&pmrs->cmdTx.slotn, 512*61440, 61440);
    if (!pmrs->cmdTx.pp) goto end;
    pmrs->cmdTx.r = (struct ring_p *)mmap(NULL, sizeof(struct ring_p), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    pmrs->cmdTx.totsz = 512*61440;
    pmrs->cmdTx.chksz = 61440;
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

   /*
     * pull low RDY
     */ 
    bitset = 0;
    ret = ioctl(pmrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_WT_CTL_PIN
    sprintf(pmrs->log, "Set RDY: %d\n", bitset);
    print_f(&pmrs->plog, "Init", pmrs->log);

   /*
     * pull low RDY
     */ 
    bitset = 0;
    ret = ioctl(pmrs->sfm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_WT_CTL_PIN
    sprintf(pmrs->log, "Set RDY: %d\n", bitset);
    print_f(&pmrs->plog, "Init", pmrs->log);

// IPC
    pipe(pmrs->pipedn[0].rt);
    pipe(pmrs->pipedn[1].rt);
    pipe(pmrs->pipedn[2].rt);
    pipe(pmrs->pipedn[3].rt);
    pipe(pmrs->pipedn[4].rt);
    pipe(pmrs->pipedn[5].rt);
	
    pipe2(pmrs->pipeup[0].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[1].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[2].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[3].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[4].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[5].rt, O_NONBLOCK);

    res_put_in(&rs[0], pmrs, 0);
    res_put_in(&rs[1], pmrs, 1);
    res_put_in(&rs[2], pmrs, 2);
    res_put_in(&rs[3], pmrs, 3);
    res_put_in(&rs[4], pmrs, 4);
    res_put_in(&rs[5], pmrs, 5);
	
//  Share memory init
    ring_buf_init(&pmrs->dataRx);
    pmrs->dataRx.r->folw.seq = 1;
    ring_buf_init(&pmrs->dataTx);
    ring_buf_init(&pmrs->cmdRx);
    ring_buf_init(&pmrs->cmdTx);

// fork process
    pmrs->sid[0] = fork();
    if (!pmrs->sid[0]) {
        p1(&rs[0]);
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

    if((idx == 0) || (idx == 1)) {
        rs->spifd = mrs->sfm[0];
    } else if (idx == 2) {
        rs->spifd = mrs->sfm[1];
    }

    rs->psocket_r = &mrs->socket_r;
    rs->psocket_t = &mrs->socket_t;	
    return 0;
}


