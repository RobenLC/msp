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
    int pipedn1[2];
    int pipedn2[2];
    int pipedn3[2];
    int pipeup1[2];
    int pipeup2[2];
    int pipeup3[2];
    
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
    int pipedn_s[2];
    int pipeup_s[2];
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
static int p0(struct mainRes_s *rs);
//p1: wifi socket connection
static int p1(struct procRes_s *rs);
//p2: spi0 
static int p2(struct procRes_s *rs);
//p3: spi1
static int p3(struct procRes_s *rs);

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
    pipe(mrs.pipedn1);
    pipe(mrs.pipedn2);
    pipe(mrs.pipedn3);

    pipe(mrs.pipeup1);
    pipe(mrs.pipeup2);
    pipe(mrs.pipeup3);
    
    res_put_in(&rs[0], &mrs, 1);
    res_put_in(&rs[1], &mrs, 2);
    res_put_in(&rs[2], &mrs, 3);

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

static int p0(struct mainRes_s *mrs)
{
    int pi, pt;
    char ch, c1;
    //while(1){}
    close(mrs->pipedn1[0]);
    close(mrs->pipedn2[0]);
    close(mrs->pipedn3[0]);

    close(mrs->pipeup1[1]);
    close(mrs->pipeup2[1]);
    close(mrs->pipeup3[1]);
    
    c1 = 0x30;
    for (pt = 0; pt < 10; pt++) {
        c1 += 1;
        for (pi = 0; pi < 3; pi++) {
            ch = c1 + pi;
            sprintf(mrs->log, "sid[%d] send %c ", pi, ch);
            print_f("p0", mrs->log);
            if (pi == 0) { write(mrs->pipedn1[1], &ch, 1); }
            if (pi == 1) { write(mrs->pipedn2[1], &ch, 1); }
            if (pi == 2) { write(mrs->pipedn3[1], &ch, 1); }
        }
    }

    ch = 0; pi = 0;
    while (1) {
        pi = read(mrs->pipeup1[0], &ch, 1);
        if (pi) { 
            printf("po get %c, ret:%d \n", ch, pi);
            
            if (ch == '1') {break;}
        }
        //print_f("p0", "get 1");
        //if (ch == '1') {
        //    sprintf(mrs->log, "sid[%d] send %c and ready to be kill", mrs->sid[0], ch);
        //    print_f("p0", mrs->log);
        //    //break;
        //}
    }
    ch = 0;
    while (1) {
        pi = read(mrs->pipeup2[0], &ch, 1);
        if (pi) { 
            printf("po get %c, ret:%d \n", ch, pi);

            if (ch == '2') {break;}
        }
        //printf("po get %c \n", ch);
        //if (ch == '2') {
        //    sprintf(mrs->log, "sid[%d] send %c and ready to be kill", mrs->sid[1], ch);
        //    print_f("p0", mrs->log);
        //    //break;
        //}
    }
    ch = 0;
    while (1) {
        pi = read(mrs->pipeup3[0], &ch, 1);
        if (pi) { 
            printf("po get %c, ret:%d \n", ch, pi);
            
            if (ch == '3') {break;}
        }
        //print_f("p0", "get 3");
        //if (ch == '3') {
        //    sprintf(mrs->log, "sid[%d] send %c and ready to be kill", mrs->sid[2], ch);
        //    print_f("p0", mrs->log);
        //    //break;
        //}
    }

//    while (1) {
        //if (!(pt & 0x1))
        //    read(mrs->pipeup[0].r, &ch, 1);
        //if (!(pt & 0x2))
        //    read(mrs->pipeup[1].r, &ch, 1);
        //if (!(pt & 0x4))
        //    read(mrs->pipeup[2].r, &ch, 1);
            
  //      sprintf(mrs->log, "get ch:%c", ch);
  //      print_f("p0", mrs->log);
        
        //switch (ch) {
        //    case '1':
        //        pt |= 0x1;
        //        sprintf(mrs->log, "sid[%d] send %c and ready to be kill", mrs->sid[0], ch);
        //        print_f("p0", mrs->log);
        //        break;
        //    case '2':
        //        pt |= 0x2;
        //        sprintf(mrs->log, "sid[%d] send %c and ready to be kill", mrs->sid[1], ch);
        //        print_f("p0", mrs->log);
        //        break;
        //    case '3':
        //        pt |= 0x4;
        //        sprintf(mrs->log, "sid[%d] send %c and ready to be kill", mrs->sid[2], ch);
        //        print_f("p0", mrs->log);
        //        break;
        //    default:
        //        sprintf(mrs->log, "sid[?] send %c and is not expected, warning", ch);
        //        print_f("p0", mrs->log);
        //        break;
        //}
        
//        if (pt & 0x7) break;
//    }

    close(mrs->pipeup1[0]);
    close(mrs->pipeup2[0]);
    close(mrs->pipeup3[0]);
    
    close(mrs->pipedn1[1]);
    close(mrs->pipedn2[1]);
    close(mrs->pipedn3[1]);

    kill(mrs->sid[0]);
    kill(mrs->sid[1]);
    kill(mrs->sid[2]);

    fclose(mrs->fs);
    munmap(mrs->dmp[0], 1024*61440);
    munmap(mrs->cmp[0], 16*512);
    free(mrs->dmp);
    free(mrs->cmp);
}

static int p1(struct procRes_s *rs)
{
    char buf[128], ch;
    int px, pi;

    close(rs->pipedn_s[1]);
    close(rs->pipeup_s[0]);

    px = 0;
    while(1){
        read(rs->pipedn_s[0], &buf[px], 1); 
        if (buf[px] == '9') {
            ch = '1';
            write(rs->pipeup_s[1], &ch, 1);    
            close(rs->pipedn_s[0]); 
            break;
        }
        px++;
    }
    for (pi = 0; pi <= px; pi++) {
        sprintf(rs->logs, "01 %c", buf[pi]);
        print_f("p1", rs->logs);
    }

}

static int p2(struct procRes_s *rs)
{
    char buf[128], ch;
    int px, pi;
    
    close(rs->pipedn_s[1]);
    close(rs->pipeup_s[0]);
    
    px = 0;
    while(1){
        read(rs->pipedn_s[0], &buf[px], 1); 
        if (buf[px] == '9') {
            ch = '2';
            write(rs->pipeup_s[1], &ch, 1);    
            close(rs->pipedn_s[0]); 
            break;
        }
        px++;
    }
    for (pi = 0; pi <= px; pi++) {
        sprintf(rs->logs, "02 %c", buf[pi]);
        print_f("p2", rs->logs);
    }

}

static int p3(struct procRes_s *rs)
{
    char buf[128], ch;
    int px, pi;
    
    close(rs->pipedn_s[1]);
    close(rs->pipeup_s[0]);
    
    px = 0;
    while(1){
        read(rs->pipedn_s[0], &buf[px], 1); 
        if (buf[px] == '9') {
            ch = '3';
            write(rs->pipeup_s[1], &ch, 1);    
            close(rs->pipedn_s[0]); 
            break;
        }
        px++;
    }
    for (pi = 0; pi <= px; pi++) {
        sprintf(rs->logs, "03 %c", buf[pi]);
        print_f("p3", rs->logs);
    }
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

    if (idx == 1) {
        rs->pipedn_s[0] = mrs->pipedn1[0];
        rs->pipedn_s[1] = mrs->pipedn1[1];
        rs->pipeup_s[0] = mrs->pipeup1[0];
        rs->pipeup_s[1] = mrs->pipeup1[1];
    }
    
    if (idx == 2) {
        rs->pipedn_s[0] = mrs->pipedn2[0];
        rs->pipedn_s[1] = mrs->pipedn2[1];
        rs->pipeup_s[0] = mrs->pipeup2[0];
        rs->pipeup_s[1] = mrs->pipeup2[1];    }
    
    if (idx == 3) {
        rs->pipedn_s[0] = mrs->pipedn3[0];
        rs->pipedn_s[1] = mrs->pipedn3[1];
        rs->pipeup_s[0] = mrs->pipeup3[0];
        rs->pipeup_s[1] = mrs->pipeup3[1];
    }
    
    return 0;
}
