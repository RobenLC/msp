//#include <stdint.h> 
//#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 
//#include <string.h>
//#include <getopt.h> 
//#include <fcntl.h> 
//#include <sys/ioctl.h> 
#include <sys/mman.h> 
//#include <linux/types.h> 
//#include <linux/spi/spidev.h> 
//#include <sys/times.h> 
#include <time.h>
//main()

struct pipe_s{
    int r;
    int t;
};

struct mainRes_s{
    int sid[3];
    // 3 pipe
    struct pipe_s pipedn[3];
    struct pipe_s pipeup[3];
    // data mode share memory
    int cdsz;
    int mdsz;
    char **dmp;
    // command mode share memory
    int ccsz;
    int mcsz;
    char **cmp;
    // file save
    FILE *fs;
    // time measurement
    struct timespec time[2];
    // log buffer
    char log[256];
};

struct procRes_s{
    // pipe
    struct pipe_s pipedn_m;
    struct pipe_s pipeup_m;
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
//p1: wifi socket connection
static int p1(struct procRes_s *rs);
static int p1_init(struct procRes_s *rs);
static int p1_end(struct procRes_s *rs);
//p2: spi0 
static int p2(struct procRes_s *rs);
static int p2_init(struct procRes_s *rs);
static int p2_end(struct procRes_s *rs);
//p3: spi1
static int p3(struct procRes_s *rs);
static int p3_init(struct procRes_s *rs);
static int p3_end(struct procRes_s *rs);
static int pn_init(struct procRes_s *rs);
static int pn_end(struct procRes_s *rs);
//IPC wrap
static int rs_ipc_put(struct procRes_s *rs, char *str, int size);
static int rs_ipc_get(struct procRes_s *rs, char *str, int size);
static int mrs_ipc_put(struct mainRes_s *mrs, char *str, int size, int idx);
static int mrs_ipc_get(struct mainRes_s *mrs, char *str, int size, int idx);

static int mrs_ipc_get(struct mainRes_s *mrs, char *str, int size, int idx)
{
    int ret;
    ret = read(mrs->pipeup[idx].r, str, size);
    return ret;
}

static int mrs_ipc_put(struct mainRes_s *mrs, char *str, int size, int idx)
{
    int ret;
    ret = write(mrs->pipedn[idx].t, str, size);
    return ret;
}

static int rs_ipc_put(struct procRes_s *rs, char *str, int size)
{
    int ret;
    ret = write(rs->pipeup_m.t, str, size);
    return ret;
}

static int rs_ipc_get(struct procRes_s *rs, char *str, int size)
{
    int ret;
    ret = read(rs->pipedn_m.r, str, size);
    return ret;
}

static int pn_init(struct procRes_s *rs)
{
    close(rs->pipedn_m.t);
    close(rs->pipeup_m.r);
    return 0;
}

static int pn_end(struct procRes_s *rs)
{
    close(rs->pipedn_m.r); 
    close(rs->pipeup_m.t);
    return 0;
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
    close(mrs->pipedn[0].r);
    close(mrs->pipedn[1].r);
    close(mrs->pipedn[2].r);

    close(mrs->pipeup[0].t);
    close(mrs->pipeup[1].t);
    close(mrs->pipeup[2].t);

    return 0;
}

static int p0_end(struct mainRes_s *mrs)
{
    close(mrs->pipeup[0].r);
    close(mrs->pipeup[1].r);
    close(mrs->pipeup[2].r);
    
    close(mrs->pipedn[0].t);
    close(mrs->pipedn[1].t);
    close(mrs->pipedn[2].t);

    kill(mrs->sid[0]);
    kill(mrs->sid[1]);
    kill(mrs->sid[2]);

    fclose(mrs->fs);
    munmap(mrs->dmp[0], 1024*61440);
    munmap(mrs->cmp[0], 16*512);
    free(mrs->dmp);
    free(mrs->cmp);
    return 0;
}

static int p0(struct mainRes_s *mrs)
{
    int pi, pt, tp;
    char ch, c1;

    p0_init(mrs);
    /* the initial mode is command mode, the rdy pin is pull low at begin */
    // put the initial status in shared memory which is the default tx data for command mode
    // send 'c' to p1 to start the command mode
    // send 'c' to p3 to start the socket recv
    // parsing command in shared memory which get from socket 
    // parsing command in shared memory whcih get form spi
    // 

    p0_end(mrs);
    return 0;
}

static int p1(struct procRes_s *rs)
{
    int px, pi, ret;

    p1_init(rs);
    // wait for ch from p0
    // 'c': command mode, store the incoming infom into share memory
    // send 'c' to notice the p0 that we have incoming command
    // 'd': data mode, store the incomming infom into share memory 
    // send 'd' to notice the p0 that we have incomming data chunk

    p1_end(rs);
    return 0;
}

static int p2(struct procRes_s *rs)
{
    int px, pi, ret;
    
    p2_init(rs);
    // wait for ch from p0
    // 'd': data mode, store the incomming infom into share memory
    // send 'd' to notice the p0 that we have incomming data chunk

    p2_end(rs);
    return 0;
}

static int p3(struct procRes_s *rs)
{
    int px, pi, ret;
    
    p3_init(rs);
    // wait for ch from p0
    // 'c': command mode, store the socket incoming inform into share memory
    // 'd': data mode, forward the socket incoming inform into share memory
    p3_end(rs);
    return 0;
}

int main(int argc, char *argv[])
{
    struct mainRes_s mrs;
    struct procRes_s rs[3];
    int ix, ret;
    char *log;
    int tdiff;
    int arg[8];
	;
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
    clock_gettime(CLOCK_REALTIME, &mrs.time[0]);
    mrs.dmp = memory_init(&mrs.mdsz, 1024*61440, 61440);
    //sprintf(mrs.log, "msz:%d dmp:0x%.8x", mrs.mdsz, mrs.dmp);
    //print_f("minit_result", mrs.log);
    //for (ix = 0; ix < mrs.mdsz; ix++) {
    //    sprintf(mrs.log, "[%d] 0x%.8x", ix, mrs.dmp[ix]);
    //    print_f("minit_result", mrs.log);
    //}
    clock_gettime(CLOCK_REALTIME, &mrs.time[1]);
    tdiff = time_diff(&mrs.time[0], &mrs.time[1], 1000);
    sprintf(mrs.log, "tdiff:%d ", tdiff);
    print_f("time_diff", mrs.log);

    clock_gettime(CLOCK_REALTIME, &mrs.time[0]);
    mrs.cmp= memory_init(&mrs.mcsz, 16*512, 512);
    //sprintf(mrs.log, "msz:%d dmp:0x%.8x", mrs.mcsz, mrs.cmp);
    //print_f("minit_result", mrs.log);
    //for (ix = 0; ix < mrs.mcsz; ix++) {
    //    sprintf(mrs.log, "[%d] 0x%.8x", ix, mrs.cmp[ix]);
    //    print_f("minit_result", mrs.log);
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
    
// IPC
    pipe(&mrs.pipedn[0]);
    pipe(&mrs.pipedn[1]);
    pipe(&mrs.pipedn[2]);

    pipe(&mrs.pipeup[0]);
    pipe(&mrs.pipeup[1]);
    pipe(&mrs.pipeup[2]);

    res_put_in(&rs[0], &mrs, 0);
    res_put_in(&rs[1], &mrs, 1);
    res_put_in(&rs[2], &mrs, 2);

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
                p0(&mrs);
            }
        }
    }

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
    //char mlog[256];
    
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
    rs->ccsz_s = mrs->ccsz;
    rs->mcsz_s = mrs->mcsz;
    rs->cmp_s = mrs->cmp;
    rs->mdsz_s = mrs->mdsz;
    rs->mdsz_s = mrs->mdsz;
    rs->dmp_s = mrs->dmp;
    rs->fs_s = mrs->fs;
    rs->logs = mrs->log;
    rs->tm[0] = &mrs->time[0];
    rs->tm[1] = &mrs->time[1];

    rs->pipedn_m.t = mrs->pipedn[idx].t;
    rs->pipedn_m.r = mrs->pipedn[idx].r;
    rs->pipeup_m.t = mrs->pipeup[idx].t;
    rs->pipeup_m.r = mrs->pipeup[idx].r;
    
    return 0;
}
