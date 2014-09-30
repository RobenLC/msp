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
//main()
#define SPI_CPHA  0x01          /* clock phase */
#define SPI_CPOL  0x02          /* clock polarity */
#define SPI_MODE_0		(0|0)
#define SPI_MODE_1		(0|SPI_CPHA)
#define SPI_MODE_2		(SPI_CPOL|0)
#define SPI_MODE_3		(SPI_CPOL|SPI_CPHA)

struct pipe_s{
    int rt[2];
};

struct ring_s{
    int run;
    int seq;
};

struct shmem_s{
    int totsz;
    int chksz;
    int slotn;
    char **pp;
    int svdist;
    struct ring_s lead;
    struct ring_s dual;
    struct ring_s folw;
    int lastflg;
    int lastsz;
    int dualsz;	
};

struct mainRes_s{
    int sid[5];
    int sfm[2];
    int smode;
    // 3 pipe
    struct pipe_s pipedn[5];
    struct pipe_s pipeup[5];
    // data mode share memory
    struct shmem_s dataRx;
    struct shmem_s dataTx;
    // command mode share memory
    struct shmem_s cmdRx;
    struct shmem_s cmdTx;
    // file save
    FILE *fs;
    // time measurement
    struct timespec time[2];
    // log buffer
    char log[256];

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
    // time measurement
    struct timespec *tm[2];
    char *logs;

};

//memory alloc. put in/put out
static char **memory_init(int *sz, int tsize, int csize);
//debug printf
static int print_f(char *head, char *str);
//time measurement, start /stop
static int time_diff(struct timespec *s, struct timespec *e, int unit);
//file rw open, save to file for debug
static int file_save_get(FILE **fp);
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
static int p5(struct procRes_s *rs);
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
static int shmem_from_str(char **addr, char *dst, char *sz);
static int shmem_dump(char *src, int size);
static int shmem_pop_send(struct mainRes_s *mrs, char **addr, int seq, int p);
static int shmem_rlt_get(struct mainRes_s *mrs, int seq, int p);

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
        printf("[dback] ch:%c rt:%d idx:%d \n", ch,  len, seq);
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
    printf("shmem pop:0x%.8x, seq:%d sz:%d\n", *addr, seq, sz);
    if (sz < 0) return (-1);
    sprintf(str, "d%.8xl%.8d", *addr, sz);
    printf("[%s]\n", str);
    mrs_ipc_put(mrs, str, 18, p);

    return sz;
}

static int shmem_dump(char *src, int size)
{
    int inc;
    if (!src) return -1;

    inc = 0;
    printf("memdump[0x%.8x] sz%d: \n", src, size);
    while (inc < size) {
        printf("%.2x ", *src);
        inc++;
        src++;
        if (!((inc+1) % 16)) printf("\n");
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
static int ring_buf_init(struct shmem_s *pp)
{
    pp->lead.run = 0;
    pp->lead.seq = 0;
    pp->dual.run = 0;
    pp->dual.seq = 1;
    pp->folw.run = 0;
    pp->folw.seq = 0;
    pp->lastflg = 0;
    pp->lastsz = 0;
    pp->dualsz = 0;

    return 0;
}

static int ring_buf_get_dual(struct shmem_s *pp, char **addr, int sel)
{
    int dualn = 0;
    int folwn = 0;
    int dist;
    int tmps;

    sel = sel % 2;

    folwn = pp->folw.run * pp->slotn + pp->folw.seq;

    if (sel) {
        dualn = pp->dual.run * pp->slotn + pp->dual.seq;
    } else {
        dualn = pp->lead.run * pp->slotn + pp->lead.seq;
    }

    dist = dualn - folwn;
    //printf("d:%d, %d /%d \n", dist, dualn, folwn);
    if (dist > (pp->slotn - 3))  return -1;

    if (sel) {
        if ((pp->dual.seq + 2) < pp->slotn) {
            *addr = pp->pp[pp->dual.seq+2];
        } else {
            tmps = (pp->dual.seq+2) % pp->slotn;
            *addr = pp->pp[tmps];
        }
    } else {
        if ((pp->lead.seq + 2) < pp->slotn) {
            *addr = pp->pp[pp->lead.seq+2];
        } else {
            tmps = (pp->lead.seq+2) % pp->slotn;
            *addr = pp->pp[tmps];
        }
    }

    return pp->chksz;	
}

static int ring_buf_get(struct shmem_s *pp, char **addr)
{
    int leadn = 0;
    int folwn = 0;
    int dist;

    folwn = pp->folw.run * pp->slotn + pp->folw.seq;
    leadn = pp->lead.run * pp->slotn + pp->lead.seq;

    dist = leadn - folwn;
    if (dist > (pp->slotn - 2))  return -1;

    if ((pp->lead.seq + 1) < pp->slotn) {
        *addr = pp->pp[pp->lead.seq+1];
    } else {
        *addr = pp->pp[0];
    }

    return pp->chksz;	
}

static int ring_buf_set_last_dual(struct shmem_s *pp, int size, int sel)
{
    sel = sel % 2;

    if (sel) {
        pp->dualsz = size;
    } else {
        pp->lastsz = size;
    }
    pp->lastflg += 1;

    printf("[last] d:%d l:%d flg:%d \n", pp->dualsz, pp->lastsz, pp->lastflg);
    return pp->lastflg;
}

static int ring_buf_set_last(struct shmem_s *pp, int size)
{
    pp->lastsz = size;
    pp->lastflg = 1;
    return pp->lastflg;
}
static int ring_buf_prod_dual(struct shmem_s *pp, int sel)
{
    sel = sel % 2;
    if (sel) {
        if ((pp->dual.seq + 2) < pp->slotn) {
            pp->dual.seq += 2;
        } else {
            pp->dual.seq = 1;
            pp->dual.run += 1;
        }
    } else {
        if ((pp->lead.seq + 2) < pp->slotn) {
            pp->lead.seq += 2;
        } else {
            pp->lead.seq = 0;
            pp->lead.run += 1;
        }
    }

    return 0;
}

static int ring_buf_prod(struct shmem_s *pp)
{
    if ((pp->lead.seq + 1) < pp->slotn) {
        pp->lead.seq += 1;
    } else {
        pp->lead.seq = 0;
        pp->lead.run += 1;
    }

    return 0;
}

static int ring_buf_cons_dual(struct shmem_s *pp, char **addr, int sel)
{
    int dualn = 0;
    int leadn = 0;
    int folwn = 0;
    int dist;

    folwn = pp->folw.run * pp->slotn + pp->folw.seq;
    dualn = pp->dual.run * pp->slotn + pp->dual.seq;
    leadn = pp->lead.run * pp->slotn + pp->lead.seq;
    if (dualn > leadn) {
        dist = dualn - folwn;
    } else {
        dist = leadn - folwn;
    }

    //printf("[cons], d: %d %d/%d/%d \n", dist, leadn, dualn, folwn);

    if (dist < 1)  return -1;

    if ((pp->folw.seq + 1) < pp->slotn) {
        *addr = pp->pp[pp->folw.seq + 1];
        pp->folw.seq += 1;
    } else {
        *addr = pp->pp[0];
        pp->folw.seq = 0;
        pp->folw.run += 1;
    }

    if (pp->lastflg) {
        printf("[clast] f:%d %d, d:%d %d l: %d %d \n", pp->folw.run, pp->folw.seq, 
			pp->dual.run, pp->dual.seq, pp->lead.run, pp->lead.seq);
        if (dualn > leadn) {
            if ((pp->folw.run == pp->dual.run) &&
             (pp->folw.seq == pp->dual.seq)) {
                return pp->dualsz;
            }
        } else {
            if ((pp->folw.run == pp->lead.run) &&
             (pp->folw.seq == pp->lead.seq)) {
                return pp->lastsz;
            }
        }
    }

    return pp->chksz;
}

static int ring_buf_cons(struct shmem_s *pp, char **addr)
{
    int leadn = 0;
    int folwn = 0;
    int dist;

    folwn = pp->folw.run * pp->slotn + pp->folw.seq;
    leadn = pp->lead.run * pp->slotn + pp->lead.seq;
    dist = leadn - folwn;

    printf("cons, d: %d %d/%d \n", dist, leadn, folwn);

    if (dist < 1)  return -1;

    if ((pp->folw.seq + 1) < pp->slotn) {
        *addr = pp->pp[pp->folw.seq + 1];
        pp->folw.seq += 1;
    } else {
        *addr = pp->pp[0];
        pp->folw.seq = 0;
        pp->folw.run += 1;
    }

    if (pp->lastflg) {
        if ((pp->folw.run == pp->lead.run) &&
            (pp->folw.seq == pp->lead.seq)) {
            return pp->lastsz;
        }
    }

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
    munmap(mrs->dataTx.pp[0], 8*61440);
    munmap(mrs->cmdRx.pp[0], 16*1024);
    munmap(mrs->cmdTx.pp[0], 2048*1024);
    free(mrs->dataRx.pp);
    free(mrs->cmdRx.pp);
    free(mrs->dataTx.pp);
    free(mrs->cmdTx.pp);
    return 0;
}

static int p0(struct mainRes_s *mrs)
{
    int pi, pt, pc, tp, ret, sz[3], chk[2], bksz[2];
    char ch, c1, str[128], dst[17];
    char *addr[3], *stop_at;
    int pmode = 0, pstatus = 0, cstatus = 0;

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
        ch = fgetc(stdin);
        if (ch == '\n') continue;
        if (ch == '2') {
            // set mode to data mode
            pmode = 1;
            bksz[0] = 0;
            bksz[1] = 0;
            mrs->dataRx.lastflg = 0;
            mrs->dataRx.lastsz = 0;
            mrs->dataRx.dualsz = 0;
        }
    }
    pi = 0; pt = 0; pc = 0;
    while (1) {
        //printf(".%d", cstatus);
        // p2 data mode spi0 rx
        if (pmode == 1) {
            if (pstatus == 0) {
                // [TODO] check the remaining buffer, stop spi if we are going to run out of the share memory
                sz[0] = shmem_pop_send(mrs, &addr[0], pi, 1);
                if (sz[0] < 0) continue;
				
                sz[1] = shmem_pop_send(mrs, &addr[1], pi + 1, 2);
                if (sz[1] < 0) continue;

                pstatus = 1;
                chk[0] = 0;
                chk[1] = 0;				
            }
            if (pstatus == 1) {	
                ret = shmem_rlt_get(mrs, pi, 1);
                if (ret > 0)
                    chk[0] = 1;

                ret = shmem_rlt_get(mrs, pi + 1, 2);
                if (ret > 0)
                    chk[1] = 1;

                if ((chk[0]) && (chk[1])) {
                    pstatus = 0;
                    pi += 2;
                    if (mrs->dataRx.lastflg) 
                        pstatus = 2;
                }
            }

            if (cstatus == 0) {
                sz[2] = ring_buf_cons_dual(&mrs->dataRx, &addr[2], pc);
                if (sz[2] >= 0) {
                    printf("[cons]:0x%.8x, idx:%d sz:%d\n", addr[2], pc, sz[2]);
                    sprintf(str, "d%.8xl%.8d", addr[2],sz[2]);
                    //printf("[%s]\n", str);
                    mrs_ipc_put(mrs, str, 18, 3);

                    cstatus = 1;
                }
            } else if (cstatus == 1) {
                ret = mrs_ipc_get(mrs, &ch, 1, 3);
                if ((ret > 0) && (ch == 'd')) {
                    printf("[cons] done idx:%d \n", pc);
                    pc++;
                    cstatus = 0;
                }
            } else {

            }
			
			
        }
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
    int px, pi, ret;
    char ch;
    char *addr;

    printf("p1\n");
    p1_init(rs);
    // wait for ch from p0
    // in charge of spi0 command and command parsing
    // 'c': command mode, store the incoming infom into share memory
    // send 'c' to notice the p0 that we have incoming command

    pi = 0;
    ch = '1';
    while (1) {
        printf("+");
        ret = rs_ipc_get(rs, &ch, 1);
        if (ret > 0) {
            printf("%c", ch);
        }
        ret = rs_ipc_put(rs, &ch, 1);
        if (ret > 0) {
            //printf("[1s:%c]", ch);
        }
        usleep(10);
    }

    p1_end(rs);
    return 0;
}

static int p2(struct procRes_s *rs)
{
    int px, pi, ret[5], size, opsz;
    char ch, dst[17], sz[17], str[128];
    char *addr, *stop_at;
    printf("p2\n");
    p2_init(rs);
    // wait for ch from p0
    // in charge of spi0 data mode
    // 'd': data mode, store the incomming infom into share memory
    // send 'd' to notice the p0 that we have incomming data chunk

    ch = '2';
    pi = 0; px = 0;
    while (1) {
        printf("!");
        ret[0] = rs_ipc_get(rs, &ch, 1);
        if (ret[0] > 0) {
            printf("%c", ch);
            if (ch == 'd') {
                px++;
                ret[2] = rs_ipc_get(rs, dst, 8);
                dst[8] = '\0';
                //printf("[d] %s len:%d \n", dst, ret[2]);
                ret[3] = rs_ipc_get(rs, &ch, 1);
                ret[4] = rs_ipc_get(rs, sz, 8);
                sz[8] = '\0';
                //printf("[%c] %s len:%d,%d \n", ch, sz, ret[3], ret[4]);
                size = shmem_from_str(&addr, dst, sz);
                printf("[2][convert] addr: 0x%.8x, sz:%d idx:%d \n", addr, size, px);

                //if (px > 256) opsz = 20480;
                //else opsz = size;
                opsz = size;
                // data receiving
                memset(addr, 0xf0 | (pi++ & 0xf), opsz);
                //mtx_data(rs->spifd, addr, NULL, 1, opsz, 1024*1024);

                // data finish send back to p0
                if (opsz != size) {
                    sprintf(str, "dl%.8d", opsz);
                    printf("[2][%s]\n", str);
                    ret[1] = rs_ipc_put(rs, str, 10);
                } else {
                    ch = 'd';
                    ret[1] = rs_ipc_put(rs, &ch, 1);
                }
            }
        }
        usleep(100);
    }

    p2_end(rs);
    return 0;
}

static int p3(struct procRes_s *rs)
{
    int px, pi, ret[5], size, opsz;
    char ch, dst[17], sz[17], str[128];
    char *addr, *stop_at;
    printf("p3\n");
    p3_init(rs);
    // wait for ch from p0
    // in charge of spi1 data mode
    // 'd': data mode, forward the socket incoming inform into share memory

    pi = 0; px = 0;
    while (1) {
        printf("/");
        ret[0] = rs_ipc_get(rs, &ch, 1);
        if (ret[0] > 0) {
            printf("%c", ch);
            if (ch == 'd') {
                px++;
    
                ret[2] = rs_ipc_get(rs, dst, 8);
                dst[8] = '\0';
                //printf("[d] %s len:%d \n", dst, ret[2]);
                ret[3] = rs_ipc_get(rs, &ch, 1);
                ret[4] = rs_ipc_get(rs, sz, 8);
                sz[8] = '\0';
                //printf("[%c] %s len:%d,%d \n", ch, sz, ret[3], ret[4]);
                size = shmem_from_str(&addr, dst, sz);
                printf("[3][convert] addr: 0x%.8x, sz:%d idx:%d\n", addr, size, px);

                if (px > 16) opsz = 20480;
                else opsz = size;

                // data receiving, opsz is the actually data size
                memset(addr, 0x0f | ((pi++ & 0xf) << 4), opsz);
                //mtx_data(rs->spifd, addr, NULL, 1, opsz, 1024*1024);

                // data finish send back to p0
                if (opsz != size) {
                    sprintf(str, "dl%.8d", opsz);
                    printf("[3][%s]\n", str);
                    ret[1] = rs_ipc_put(rs, str, 10);
                } else {
                    ch = 'd';
                    ret[1] = rs_ipc_put(rs, &ch, 1);
                }
            }
        }
        usleep(100);
    }

    p3_end(rs);
    return 0;
}

static int p4(struct procRes_s *rs)
{
    int px, pi, ret[5], size, opsz;
    char ch, dst[17], sz[17], str[128];
    char *addr, *stop_at;
    printf("p4\n");
    p4_init(rs);
    // wait for ch from p0
    // in charge of socket send

    while (1) {
        printf("^");
        ret[0] = rs_ipc_get(rs, &ch, 1);
        if (ret[0] > 0) {
            printf("%c", ch);
            if (ch == 'd') {
                ret[2] = rs_ipc_get(rs, dst, 8);
                dst[8] = '\0';
                //printf("[d] %s len:%d \n", dst, ret[2]);
                ret[3] = rs_ipc_get(rs, &ch, 1);
                ret[4] = rs_ipc_get(rs, sz, 8);
                sz[8] = '\0';
                //printf("[%c] %s len:%d,%d \n", ch, sz, ret[3], ret[4]);
                size = shmem_from_str(&addr, dst, sz);
                printf("[4][convert] addr: 0x%.8x, sz:%d \n", addr, size);

                // data sending
                //memset(addr, 0xf0 | (pi++ & 0xf), size);
                msync(addr, size, MS_SYNC);
                ret[0] = fwrite(addr, 1, size, rs->fs_s);
                fflush(rs->fs_s);
                printf("p4 write file %d size %d/%d \n\n", rs->fs_s, size, ret[0]);

                // data finish send back to p0
                ch = 'd';
                ret[1] = rs_ipc_put(rs, &ch, 1);

            }
        }
    }

    p4_end(rs);
    return 0;
}

static int p5(struct procRes_s *rs)
{
    int px, pi, ret;
    char ch;
    printf("p5\n");
    p5_init(rs);
    // wait for ch from p0
    // in charge of socket recv

    while (1) {
        printf("#");
        ret = rs_ipc_get(rs, &ch, 1);
        if (ret > 0) {
            printf("%c", ch);
        }
        ret = rs_ipc_put(rs, &ch, 1);
        if (ret > 0) {
            printf("[5s:%c]", ch);
        }
    }

    p5_end(rs);
    return 0;
}

int main(int argc, char *argv[])
{
static char spi0[] = "/dev/spidev32766.0"; 
static char spi1[] = "/dev/spidev32765.0"; 

    struct mainRes_s mrs;
    struct procRes_s rs[5];
    int ix, ret;
    char *log;
    int tdiff;
    int arg[8];
    uint32_t bitset;

    sprintf(mrs.log, "argc:%d", argc);
    print_f("main", mrs.log);
// show arguments
    ix = 0;
    while(argc) {
        arg[ix] = atoi(argv[ix]);
        sprintf(mrs.log, "%d %d %s", ix, arg[ix], argv[ix]);
        print_f("arg", mrs.log);
        ix++;
        argc--;
        if (ix > 7) break;
    }
// initial share parameter
    /* data mode rx from spi */
    clock_gettime(CLOCK_REALTIME, &mrs.time[0]);
    mrs.dataRx.pp = memory_init(&mrs.dataRx.slotn, 1024*61440, 61440);
    mrs.dataRx.totsz = 1024*61440;
    mrs.dataRx.chksz = 61440;
    mrs.dataRx.svdist = 8;
    //sprintf(mrs.log, "totsz:%d pp:0x%.8x", mrs.dataRx.totsz, mrs.dataRx.pp);
    //print_f("minit_result", mrs.log);
    //for (ix = 0; ix < mrs.dataRx.slotn; ix++) {
    //    sprintf(mrs.log, "[%d] 0x%.8x", ix, mrs.dataRx.pp[ix]);
    //    print_f("shminit_result", mrs.log);
    //}
    clock_gettime(CLOCK_REALTIME, &mrs.time[1]);
    tdiff = time_diff(&mrs.time[0], &mrs.time[1], 1000);
    sprintf(mrs.log, "tdiff:%d ", tdiff);
    print_f("time_diff", mrs.log);

    /* data mode tx to spi */
    clock_gettime(CLOCK_REALTIME, &mrs.time[0]);
    mrs.dataTx.pp = memory_init(&mrs.dataTx.slotn, 8*61440, 61440);
    mrs.dataTx.totsz = 8*61440;
    mrs.dataTx.chksz = 61440;
    mrs.dataTx.svdist = 1;
    //sprintf(mrs.log, "totsz:%d pp:0x%.8x", mrs.dataTx.totsz, mrs.dataTx.pp);
    //print_f("minit_result", mrs.log);
    //for (ix = 0; ix < mrs.dataTx.slotn; ix++) {
    //    sprintf(mrs.log, "[%d] 0x%.8x", ix, mrs.dataTx.pp[ix]);
    //    print_f("shminit_result", mrs.log);
    //}
    clock_gettime(CLOCK_REALTIME, &mrs.time[1]);
    tdiff = time_diff(&mrs.time[0], &mrs.time[1], 1000);
    sprintf(mrs.log, "tdiff:%d ", tdiff);
    print_f("time_diff", mrs.log);

    /* cmd mode rx from spi */
    clock_gettime(CLOCK_REALTIME, &mrs.time[0]);
    mrs.cmdRx.pp = memory_init(&mrs.cmdRx.slotn, 16*1024, 1024);
    mrs.cmdRx.totsz = 16*1024;;
    mrs.cmdRx.chksz = 61440;
    mrs.cmdRx.svdist = 4;
    //sprintf(mrs.log, "totsz:%d pp:0x%.8x", mrs.cmdRx.totsz, mrs.cmdRx.pp);
    //print_f("minit_result", mrs.log);
    //for (ix = 0; ix < mrs.cmdRx.slotn; ix++) {
    //    sprintf(mrs.log, "[%d] 0x%.8x", ix, mrs.cmdRx.pp[ix]);
    //    print_f("shminit_result", mrs.log);
    //}
    clock_gettime(CLOCK_REALTIME, &mrs.time[1]);
    tdiff = time_diff(&mrs.time[0], &mrs.time[1], 1000);
    sprintf(mrs.log, "tdiff:%d ", tdiff);
    print_f("time_diff", mrs.log);
	
    /* cmd mode tx to spi */
    clock_gettime(CLOCK_REALTIME, &mrs.time[0]);
    mrs.cmdTx.pp = memory_init(&mrs.cmdTx.slotn, 2048*1024, 1024);
    mrs.cmdTx.totsz = 2048*1024;;
    mrs.cmdTx.chksz = 1024;
    mrs.cmdTx.svdist = 128;
    //sprintf(mrs.log, "totsz:%d pp:0x%.8x", mrs.cmdTx.totsz, mrs.cmdTx.pp);
    //print_f("minit_result", mrs.log);
    //for (ix = 0; ix < mrs.cmdTx.slotn; ix++) {
    //    sprintf(mrs.log, "[%d] 0x%.8x", ix, mrs.cmdTx.pp[ix]);
    //    print_f("shminit_result", mrs.log);
    //}
    clock_gettime(CLOCK_REALTIME, &mrs.time[1]);
    tdiff = time_diff(&mrs.time[0], &mrs.time[1], 1000);
    sprintf(mrs.log, "tdiff:%d ", tdiff);
    print_f("time_diff", mrs.log);

    ret = file_save_get(&mrs.fs);
    if (ret) {printf("get save file failed\n"); return 0;}
    ret = fwrite("test file write \n", 1, 16, mrs.fs);
    sprintf(mrs.log, "write file size: %d/%d", ret, 16);
    print_f("fwrite", mrs.log);

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

    mrs.sfm[0] = fd0;
    mrs.sfm[1] = fd1;
    mrs.smode = 0;
    mrs.smode |= SPI_MODE_2;

    /* set RDY pin to low before spi setup ready */
    bitset = 0;
    ret = ioctl(mrs.sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    printf("[t]Set RDY low at beginning\n");

    /*
     * spi mode 
     */ 
    ret = ioctl(mrs.sfm[0], SPI_IOC_WR_MODE, &mrs.smode);
    if (ret == -1) 
        printf("can't set spi mode\n"); 
    
    ret = ioctl(mrs.sfm[0], SPI_IOC_RD_MODE, &mrs.smode);
    if (ret == -1) 
        printf("can't get spi mode\n"); 
    
    /*
     * spi mode 
     */ 
    ret = ioctl(mrs.sfm[1], SPI_IOC_WR_MODE, &mrs.smode); 
    if (ret == -1) 
        printf("can't set spi mode\n"); 
    
    ret = ioctl(mrs.sfm[1], SPI_IOC_RD_MODE, &mrs.smode);
    if (ret == -1) 
        printf("can't get spi mode\n"); 

// IPC
    pipe(mrs.pipedn[0].rt);
    pipe(mrs.pipedn[1].rt);
    pipe(mrs.pipedn[2].rt);
    pipe(mrs.pipedn[3].rt);
    pipe(mrs.pipedn[4].rt);
	
    pipe2(mrs.pipeup[0].rt, O_NONBLOCK);
    pipe2(mrs.pipeup[1].rt, O_NONBLOCK);
    pipe2(mrs.pipeup[2].rt, O_NONBLOCK);
    pipe2(mrs.pipeup[3].rt, O_NONBLOCK);
    pipe2(mrs.pipeup[4].rt, O_NONBLOCK);

    res_put_in(&rs[0], &mrs, 0);
    res_put_in(&rs[1], &mrs, 1);
    res_put_in(&rs[2], &mrs, 2);
    res_put_in(&rs[3], &mrs, 3);
    res_put_in(&rs[4], &mrs, 4);

//  Share memory init
    ring_buf_init(&mrs.dataRx);
    mrs.dataRx.folw.seq = 1;
    ring_buf_init(&mrs.dataTx);
    ring_buf_init(&mrs.cmdRx);
    ring_buf_init(&mrs.cmdTx);

// fork process
    mrs.sid[0] = fork();
    if (!mrs.sid[0]) {
        p1(&rs[0]);
    } else {
        mrs.sid[1] = fork();
        if (!mrs.sid[1]) {
            p2(&rs[1]);
        } else {
            mrs.sid[2] = fork();
            if (!mrs.sid[2]) {
                p3(&rs[2]);
            } else {
                mrs.sid[3] = fork();
                if (!mrs.sid[3]) {
                    p4(&rs[3]);
                } else {				
                    mrs.sid[4] = fork();
                    if (!mrs.sid[4]) {
                        p5(&rs[4]);
                    } else { 
                        p0(&mrs);
                    }
                }
            }
        }
    }
    end:

    printf("something wrong in mothership, break! \n");

    return 0;
}

static int print_f(char *head, char *str)
{
    char ch[256];
    if((!head) || (!str)) return (-1);
    sprintf(ch, "[%s] %s\n", head, str);
    printf("%s",ch);
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
    
    //sprintf(mlog, "asz:%d pma:0x%.8x", asz, pma);
    //print_f("memory_init", mlog);
    
    mbuf = mmap(NULL, tsize, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    
    //sprintf(mlog, "mmap get 0x%.8x", mbuf);
    //print_f("memory_init", mlog);
        
    tmpB = mbuf;
    for (idx = 0; idx < asz; idx++) {
        pma[idx] = mbuf;
        
        //sprintf(mlog, "%d 0x%.8x", idx, pma[idx]);
        //print_f("memory_init", mlog);
        
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

static int file_save_get(FILE **fp)
{
static char path1[] = "/mnt/mmc2/rx/%d.bin"; 

    char dst[128], temp[128], flog[256];
    FILE *f = NULL;
    int i;

    sprintf(temp, "%s", path1);

    for (i =0; i < 1000; i++) {
        sprintf(dst, temp, i);
        f = fopen(dst, "r");
        if (!f) {
            sprintf(flog, "open file [%s]", dst);
            print_f("save", flog);
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
    rs->logs = mrs->log;
    rs->tm[0] = &mrs->time[0];
    rs->tm[1] = &mrs->time[1];

    rs->ppipedn = &mrs->pipedn[idx];
    rs->ppipeup = &mrs->pipeup[idx];

    if((idx == 0) || (idx == 1)) {
        rs->spifd = mrs->sfm[0];
    } else if (idx == 2) {
        rs->spifd = mrs->sfm[1];
    }
	
    return 0;
}


