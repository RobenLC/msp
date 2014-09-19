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

//memory alloc. put in/put out
static char **memory_init(int *sz, int tsize, int csize);
//debug printf
static int print_f(char *head, char *str);
//time measurement, start /stop
static int time_diff(struct timespec *s, struct timespec *e, int unit);
//file rw open, save to file for debug
static FILE *file_save_get(void);
//p0: control, monitor, and debug
static int p0(int sid);
//p1: wifi socket connection
static int p1(void);
//p2: spi0 
static int p2(void);
//p3: spi1
static int p3(void);

int main(int argc, char *argv[])
{
    int ix;
    char log[256];
    int arg[8];
    sprintf(log, "argc:%d", argc);
    print_f("main", log);
// show arguments
    ix = 0;
    while(argc) {
        arg[ix] = atoi(argv[ix]);
        sprintf(log, "%d %d %s", ix, arg[ix], argv[ix]);
        print_f("arg", log);
        ix++;
        argc--;
        if (ix > 7) break;
    }
// initial share parameter
    FILE *f;
    struct timespec s, e;
    int tdiff;
    int msz;
    char **mp;

    clock_gettime(CLOCK_REALTIME, &s);
    mp = memory_init(&msz, 1024*61440, 61440);
    //sprintf(log, "msz:%d mp:0x%.8x", msz, mp);
    //print_f("minit_result", log);
    //for (ix = 0; ix < msz; ix++) {
    //    sprintf(log, "[%d] 0x%.8x", ix, mp[ix]);
    //    print_f("minit_result", log);
    //}
    clock_gettime(CLOCK_REALTIME, &e);
    tdiff = time_diff(&s, &e, 1000);

    sprintf(log, "tdiff:%d ", tdiff);
    print_f("time_diff", log);

    f = file_save_get();
    
// fork process
    int sid = 0;
    sid = fork();
    if (!sid) {
        p1();
    } else {
        sid = fork();
        if (!sid) {
            p2();
        } else {
            sid = fork();
            if (!sid) {
                p3();
                
            } else {
                p0(sid);
            }
        }
    }

//release source
    munmap(mp[0], 1024*61440);
    fclose(f);
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

static FILE *file_save_get(void)
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
    
    return f;
}

static int p0(int sid)
{
    char plog[128];
    sprintf(plog, "parent get sid:%d", sid);
    while(1)
    print_f("fork", plog);
}

static int p1(void)
{
    char plog[128];
    sprintf(plog, "child 01");
    while(1)
    print_f("fork", plog);
}

static int p2(void)
{
    char plog[128];
    sprintf(plog, "child 02");
    while(1)
    print_f("fork", plog);
}

static int p3(void)
{
    char plog[128];
    sprintf(plog, "child 03");
    while(1)
    print_f("fork", plog);
}
