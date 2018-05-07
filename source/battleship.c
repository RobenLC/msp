/* by AspectMicrosystems crop. */
/*
 * battleship.c
 *
 * Copyright (C) 2018 Leo Chen <leoc@aspectmicrosystems.com.tw>
 *
 */
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
#include <math.h>
#include "dmpKey.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
#include "dmpmap.h"

#include <linux/poll.h>
#include <sys/epoll.h>
#include <errno.h> 
#include <sys/signal.h>

#define DBG_27_DV     0
#define DBG_27_EPOL  0

#define USB_METADATA_SIZE 512
#define USB_BUFF_SIZE (32768)
#define USB_RING_BUFF_NUM   (5000) //(1024)

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

#define MAX_EVENTS (32)

static int *batTotSalloc=0;

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
    struct ring_s psudo;
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

struct usbhost_s{
    struct shmem_s *pushring;
    char *puhsmeta;
    int *pushrx;
    int *pushtx;
    int pushcnt;
};

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

static unsigned long long int time_get_us(struct timespec *s)
{
    unsigned long long cur, tnow, lnow, gunit;
    unsigned long long us, deg;

    gunit = 1000;
    deg = 1000000000;
    
    cur = s->tv_sec;
    tnow = s->tv_nsec;
    lnow = cur * deg + tnow;

    us = lnow / gunit;

    return us;
}

static unsigned long long int time_get_ms(struct timespec *s)
{
    unsigned long long int cur, tnow, lnow, gunit;
    unsigned long long int ms, deg;

    gunit = 1000000;
    deg = 1000000000;

    cur = s->tv_sec;
    tnow = s->tv_nsec;
    lnow = cur * deg + tnow;

    ms = lnow / gunit;

    return ms;
}

static void* aspSalloc(int slen)
{
    int tot=0;
    char *p=0;
    
    tot = *batTotSalloc;
    tot += slen;
    printf("*******************  salloc size: %d / %d\n", slen, tot);
    *batTotSalloc = tot;
    
    p = mmap(NULL, slen, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    return p;
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
    //pma = (char **) aspMemalloc(sizeof(char *) * asz);
    pma = (char **) aspSalloc(sizeof(char *) * asz);
    
    //sprintf(mlog, "asz:%d pma:0x%.8x\n", asz, pma);
    //print_f(mlogPool, "memory_init", mlog);
    
    mbuf = aspSalloc(tsize);
    
    //sprintf(mlog, "aspSalloc get 0x%.8x\n", mbuf);
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

#define LOG_DUAL_STREAM_RING (0)
static int ring_buf_init(struct shmem_s *pp)
{
    int idx=0;
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
    pp->r->psudo.run = 0;
    pp->r->psudo.seq = 0;
    pp->lastflg = 0;
    pp->lastsz = 0;
    pp->dualsz = 0;

    for (idx=0; idx <USB_RING_BUFF_NUM; idx++) {
        memset(pp->pp[idx], 0x00, USB_BUFF_SIZE);
        msync(pp->pp[idx], USB_BUFF_SIZE, MS_SYNC);
    }

    msync(pp, sizeof(struct shmem_s), MS_SYNC);
    return 0;
}

static int ring_buf_get(struct shmem_s *pp, char **addr)
{
    char str[128];
    int leadn = 0;
    int folwn = 0;
    int maxn = 0;
    int dist;

    folwn = pp->r->folw.run * pp->slotn + pp->r->folw.seq;
    leadn = pp->r->lead.run * pp->slotn + pp->r->lead.seq;
    maxn = pp->slotn; 

    dist = leadn - folwn;
    //sprintf_f(str, "get d:%d, L:%d, f:%d, tot: %d\n", dist, leadn, folwn, maxn);
    //print_f(mlogPool, "ring", str);

    if (dist > (pp->slotn - 2))  return -1;

    if ((pp->r->lead.seq + 1) < pp->slotn) {
        *addr = pp->pp[pp->r->lead.seq+1];
    } else {
        *addr = pp->pp[0];
    }

    return pp->chksz;   
}

static int ring_buf_set_last(struct shmem_s *pp, int size)
{
    char str[128];
    int tlen=0;

    if (size > pp->chksz) {
        printf( "ERROR!!! set last %d > max %d \n", size, pp->chksz);
        size = pp->chksz;
    }

    #if 0
    tlen = size % MIN_SECTOR_SIZE;
    if (tlen) {
        size = size + MIN_SECTOR_SIZE - tlen;
    }
    #endif

    pp->lastsz = size;
    pp->lastflg = 1;

    printf( "set last l:%d f:%d \n", pp->lastsz, pp->lastflg);

    msync(pp, sizeof(struct shmem_s), MS_SYNC);
    return pp->lastflg;
}

static int ring_buf_prod(struct shmem_s *pp)
{
    char str[128];
    if ((pp->r->lead.seq + 1) < pp->slotn) {
        pp->r->lead.seq += 1;
    } else {
        pp->r->lead.seq = 0;
        pp->r->lead.run += 1;
    }

    msync(pp, sizeof(struct shmem_s), MS_SYNC);
    
    //sprintf_f(str, "prod %d %d/\n", pp->r->lead.run, pp->r->lead.seq);
    //print_f(mlogPool, "ring", str);

    return 0;
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

    //sprintf_f(str, "cons, d: %d %d/%d - %d\n", dist, leadn, folwn,pp->lastflg);
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
        printf("last, f: %d %d/l: %d %d sz: %d\n", pp->r->folw.run, pp->r->folw.seq, pp->r->lead.run, pp->r->lead.seq, pp->lastsz);

        if ((pp->r->folw.run == pp->r->lead.run) &&
            (pp->r->folw.seq == pp->r->lead.seq)) {
            return pp->lastsz;
        }
    }

    msync(pp, sizeof(struct shmem_s), MS_SYNC);

    return pp->chksz;
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

#define USB_TX_LOG 0
static int usb_send(char *pts, int usbfd, int len)
{
    int ret=0, send=0;
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
    printf("[UW] usb write %d bytes, ret: %d (2)\n", len, send);
#endif
    return send;    
}

static int usb_read(char *ptr, int usbfd, int len)
{
    int ret=0, recv=0;
    
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
    //printf("[UR] usb read %d bytes, ret: %d (2)\n", len, recv);
#endif
    
    return recv;    
}

static int batusb_device(struct usbdev_s *pudv, char *strpath)
{
    int ret=0, cntTx=0, ifx=0, uret=0;
    uint8_t cmd=0, opc=0, dat=0;
    uint32_t usbentsRx=0, usbentsTx=0, getents=0;
    int usbfd=0, epollfd=0, rxfd=0;
    int ptret=0, recvsz=0, acusz=0, wrtsz=0, maxsz=0, sendsz=0, lastsz=0;
    int cntLp0=0, cntLp1=0, cntLpx=0;
    char chq=0, chd=0;
    char *metaPt=0, *addrd=0;
    int *piptx=0, *piprx=0;
    int cntTx=0, usCost=0, bufsize=0, seqtx=0, retry=0, msCost=0;
    char *ptrecv, *ptsend, *palloc=0;
    int pipRet=0, lens=0;
    
    struct timespec tstart, tend;
    struct epoll_event eventRx, getevents[MAX_EVENTS];
    struct usbhost_s *phost1=0, *phost2=0;
    struct usbhost_s *puscur=0;
    struct shmem_s *usbTx=0, *usbTxd=0, *usbCur=0;
    struct aspMetaData_s *metaRx = 0;
    
    char csw[13] = {0x55, 0x53, 0x42, 0x43, 0x11, 0x22, 0x33, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00};

    bufsize = USB_BUFF_SIZE;
    
    ptrecv = malloc(USB_BUFF_SIZE);
    if (ptrecv) {
        printf(" recv buff alloc succeed! size: %d \n", bufsize);
    } else {
        printf(" recv buff alloc failed! size: %d \n", bufsize);
        goto end;
    }        

    phost1 = pudv->pushost1;
    phost2 = pudv->pushost2;
    metaRx = phost1->puhsmeta;
    
    usbfd = open(strpath, O_RDWR);
    if (usbfd <= 0) {
        printf("can't open device[%s]\n", strpath); 
        close(usbfd);
        goto end;
    }
    else {
        printf("open device[%s]\n", strpath); 
    }
        
    usb_nonblock_set(usbfd);
        
    epollfd = epoll_create1(O_CLOEXEC);
    if (epollfd < 0) {
        perror("epoll_create1");
        //exit(EXIT_FAILURE);
        printf("epoll create failed, errno: %d\n", errno);
    } else {
        printf("epoll create succeed, epollfd: %d, errno: %d\n", epollfd, errno);
    } 

    usb_nonblock_set(usbfd);

    eventRx.data.fd = usbfd;
    eventRx.events = EPOLLIN | EPOLLOUT | EPOLLET;
    ret = epoll_ctl (epollfd, EPOLL_CTL_ADD, usbfd, &eventRx);
    if (ret == -1) {
        perror ("epoll_ctl");
        printf("spoll set ctl failed errno: %ds\n", errno);
    }

    cntTx = 0;
    ifx = 0;
    while (1) {
        #if DBG_27_EPOL
        printf("[ePol] cmd: 0x%.2x, opc: 0x%.2x, dat: 0x%.2x, lastsz: %d, cnt: %d \n", cmd, opc, dat, lastsz, cntTx);
        #endif

        uret = epoll_wait(epollfd, getevents, MAX_EVENTS, 100);
        if (uret < 0) {
            perror("epoll_wait");
            printf("[ePol] failed errno: %d ret: %d\n", errno, uret);
        } else if (uret == 0) {
            #if DBG_27_EPOL
            printf("[ePol] timeout errno: %d ret: %d\n", errno, uret);
            #endif
        } else {
            rxfd = getevents[0].data.fd;
            getents = getevents[0].events;

            #if DBG_27_EPOL
            printf("[ePol] usbfd: %d, evt: 0x%x ret: %d - %d\n", usbfd, getevents[0].events, uret, ifx);
            #endif
            if (getents & EPOLLIN) {
                if (rxfd == usbfd) {               
                    usbentsRx = 1;
                }
            }
                
            if (getents & EPOLLOUT) {
                if (rxfd == usbfd) {               
                    usbentsTx = 1;
                }
            }
        }
        #if DBG_27_EPOL
        printf("[ePol] epoll rx: %d, tx: %d ret: %d \n", usbentsRx, usbentsTx, uret);
        #endif

        if (usbentsTx == 1) {
            #if DBG_27_EPOL
            printf("[ePol] Tx cmd: 0x%.2x, opc: 0x%.2x, dat: 0x%.2x, lastsz: %d, cnt: %d \n", cmd, opc, dat, lastsz, cntTx);
            #endif
            if ((cmd == 0x12) && ((opc == 0x04) || (opc == 0x05))) {

#if 0
                    while (1) {
                        chq = 0;
                        printf("[DV] get meta !!!\n");
                        pipRet = read(pipeRx[0], &chq, 1);
#if DBG_27_DV
                        if (pipRet < 0) {
                            printf("[DV] get meta resp ret: %d !!!\n", pipRet);
                            usleep(100000);
                        } else {
                            printf("[DV] get meta resp chq:%c ,ret: %d\n", chq, pipRet);
                            if (chq == 'D') {
                                break;
                            }
                        }
#endif
                    }
#endif

#if 0
                    if (puscur) {
                        usbCur = puscur->pushring;
                        piptx = puscur->pushtx;
                        piprx = puscur->pushrx; 
                    } else {
                        /* error */
                    }
#endif
                if ((opc == 0x05) && (!phost2->pushcnt)) {

                        cntLp0 = 0;
                        while (1) {
                            chq = 0;
                            pipRet = 0;
                            pipRet = read(pipeRx[0], &chq, 1);
                            if (pipRet < 0) {
                                //printf("[DV] get pipe send data ret: %d, error!!\n", pipRet);
                                usleep(10000);
                                continue;
                            }
                            else {
                                //printf("[LP0] chq: %c - %d \n", chq, cntLp0);
                                cntLp0++;
                                if (chq == 'E') {
                                    printf("[LP0] chq: %c - %d \n", chq, cntLp0);
                                    break;
                                }
                                else {
                                    continue;
                                }
                            }
                        }

                        phost1->pushcnt = cntLp0;

                        cntLp1 = 0;
                        while (1) {
                            chq = 0;
                            pipRet = 0;
                            pipRet = read(pipeRxd[0], &chq, 1);
                            if (pipRet < 0) {
                                //printf("[DV] get pipe send data ret: %d, error!!\n", pipRet);
                                usleep(10000);
                                continue;
                            }
                            else {
                                //printf("[LP1] chq: %c - %d \n", chq, cntLp1);
                                cntLp1++;
                                if (chq == 'E') {
                                    printf("[LP1] chq: %c - %d \n", chq, cntLp1);
                                    break;
                                }
                                else {
                                    continue;
                                }
                            }
                        }

                        phost2->pushcnt = cntLp1;
                        
                    }

                    if ((opc == 0x04) && (!phost1->pushcnt)) {
                        cntLp0 = 0;
                        while (1) {
                            chq = 0;
                            pipRet = 0;
                            pipRet = read(pipeRx[0], &chq, 1);
                            if (pipRet < 0) {
                                //printf("[DV] get pipe send data ret: %d, error!!\n", pipRet);
                                usleep(10000);
                                continue;
                            }
                            else {
                                //printf("[LP0] chq: %c - %d \n", chq, cntLp0);
                                cntLp0++;
                                if (chq == 'E') {
                                    break;
                                }
                                else {
                                    continue;
                                }
                            }
                        }

                        phost1->pushcnt = cntLp0;
                    }
                    
                    while (1) {       
#if DBG_27_DV
                        printf("[DV] addrd: 0x%.8x \n", addrd);
#endif
                        

                        while ((!addrd) && (puscur->pushcnt > 0)) {
#if 0
                            chq = 0;
                            pipRet = read(piprx[0], &chq, 1);
                            if (pipRet < 0) {
                                //printf("[DV] get pipe send data ret: %d, error!!\n", pipRet);
                                usleep(1000);
                                continue;
                            }
                            else {
                                if (chq == 'E') {
                                    printf("[DV] get pipe chq: %c ,ret: %d, get last trunk \n", chq, pipRet);
                                }
#if DBG_27_DV
                                else {
                                    printf("[DV] get pipe chq: %c ,ret: %d\n", chq, pipRet);
                                }
#endif
                            }
#endif

                            lens = ring_buf_cons(usbCur, &addrd);                
                            while (lens <= 0) {
                                //printf("[DV] cons ring buff ret: %d \n", lens);
                                usleep(1000);
                                lens = ring_buf_cons(usbCur, &addrd);                
                            }
                            puscur->pushcnt --;

                            if (puscur->pushcnt == 0) {
                                printf("[DV] the last trunk size is %d \n", lens);
                            }
#if 0
                            if (chq == 'E') {
                                printf("[DV] the last trunk size is %d \n", lens);
                            }
#endif
                        }

                        if (cntTx == 0) {
                            clock_gettime(CLOCK_REALTIME, &tstart);
                            printf("[DV] start time %llu ms \n", time_get_ms(&tstart));
                        }    

                        sendsz = write(usbfd, addrd, lens);
                        if (sendsz < 0) {
#if DBG_27_DV
                            printf("[DV] usb send ret: %d [addr: 0x%.8x]!!!\n", sendsz, addrd);
#endif
#if 0
                            usbentsTx = 0;
                            break;
#else
                            //usleep(5000);
                            continue;
#endif
                        }
                        else {
#if DBG_27_DV
                            printf("[DV] usb TX size: %d, ret: %d \n", lens, sendsz);
#endif

                            acusz += sendsz;
                            
                            if (lens == sendsz) {
                                addrd = 0;
                                //if (chq == 'E') break;
                                if (puscur->pushcnt == 0) break;
                            } else {
                                lens -= sendsz;
                                addrd += sendsz;
                                continue;
                            }
                        }
                                
                        cntTx ++;

                    }

                    if ((puscur->pushcnt == 0) && (addrd == 0)) {

                        clock_gettime(CLOCK_REALTIME, &tend);
                        printf("[DV] end time %llu ms \n", time_get_ms(&tend));
                        usCost = test_time_diff(&tstart, &tend, 1000);
                        //msCost = test_time_diff(&tstart, &tend, 1000000);
                        throughput = acusz*8.0 / usCost*1.0;
                        printf("[DV] usb throughput: %d bytes / %d ms = %lf MBits\n", acusz, usCost / 1000, throughput);

                        cntTx = 0;
                        cmd = 0;
                        ring_buf_init(usbCur);
                    } else {
                        continue;
                    }

                }
                else if (cmd == 0x11) {
                    seqtx++;
                    csw[12] = seqtx;

                    wrtsz = 0;
                    retry = 0;
                    while (1) {
                        wrtsz = write(usbfd, csw, 13);
#if DBG_27_DV
                        printf("[DV] usb TX size: %d \n====================\n", wrtsz); 
#endif
                        if (wrtsz > 0) {
                            break;
                        }
                        retry++;
                        if (retry > 32768) {
                            break;
                        }
                    }

                    if (wrtsz < 0) {
                        usbentsTx = 0;
                        continue;
                    }
                    
                    shmem_dump(csw, wrtsz);
                    
                    cmd = 0;
                }
                else {
                    /* do nothing */
                }      
            }
            

            if (usbentsRx == 1) {
#if DBG_27_EPOL
                printf("[ePol] Rx cmd: 0x%.2x, opc: 0x%.2x, dat: 0x%.2x, lastsz: %d, cnt: %d \n", cmd, opc, dat, lastsz, cntTx);
#endif
                while (1) {
                if ((opc == 0x4c) && (dat == 0x01)) {
                    recvsz = read(usbfd, ptrecv, USB_METADATA_SIZE);
#if DBG_27_DV
                    printf("[DV] usb RX size: %d / %d \n====================\n", recvsz, USB_METADATA_SIZE); 
#endif              
                    if (recvsz < 0) {
                        //usbentsRx = 0;
                        break;
                    }

                    dat = 0;
                    
                    if (recvsz != USB_METADATA_SIZE) {
                        printf("[DV] read meta failed, receive size: %d \n====================\n", recvsz); 
                        shmem_dump(ptrecv, recvsz);                        

                        break;
                    }
                    
#if 1//DBG_27_DV
                    shmem_dump(ptrecv, recvsz);
#endif
                    memcpy(metaPt, ptrecv, 512);
                    msync(metaPt, 512, MS_SYNC);

                    if ((metaRx->ASP_MAGIC[0] != 0x20) || (metaRx->ASP_MAGIC[1] != 0x14)) {
                        break;
                    }
                    
                    chq = 'm';
                    pipRet = write(pipeTx[1], &chq, 1);
                    if (pipRet < 0) {
                        printf("[DV]  pipe send meta ret: %d \n", pipRet);
                        goto end;
                    }

                    chd = 'm';
                    pipRet = write(pipeTxd[1], &chd, 1);
                    if (pipRet < 0) {
                        printf("[DV]  pipe send meta ret: %d \n", pipRet);
                        goto end;
                    }
                    
                    break;
                }
                else {
                    recvsz = read(usbfd, ptrecv, 31);
#if DBG_27_DV
                    printf("[DV] usb RX size: %d / %d \n====================\n", recvsz, 31); 
#endif
                    if (recvsz < 0) {
                        usbentsRx = 0;
                        break;
                    }
                    shmem_dump(ptrecv, recvsz);
                    
                    if ((ptrecv[0] == 0x55) && (ptrecv[1] == 0x53) && (ptrecv[2] == 0x42)) {
                        cmd = ptrecv[15];
                        opc = ptrecv[16];
                        dat = ptrecv[17];
                        printf("[DVF] usb get cmd: 0x%.2x opc: 0x%.2x, dat: 0x%.2x \n",cmd, opc, dat);       
            
                        if (cmd == 0x11) {
                            if ((opc == 0x4c) && (dat == 0x01)) {                     
                                continue;
                            }
                            else if ((opc == 0x04) && (dat == 0x85)) {

                                puscur = 0;
                                ring_buf_init(usbTx);
                                while (1) {
                                    chq = 0;
                                    pipRet = read(pipeRx[0], &chq, 1);
                                    if (pipRet < 0) {
                                        break;
                                    }
                                    else {
#if 1 //DBG_27_DV
                                        printf("[DV] clean pipe get chq: %c \n", chq);
#endif
                                    }
                                }
                        
                                break;
                            }
                            else if ((opc == 0x05) && (dat == 0x85)) {

                                puscur = 0;
                                ring_buf_init(usbTx);
                                while (1) {
                                    chq = 0;
                                    pipRet = read(pipeRx[0], &chq, 1);
                                    if (pipRet < 0) {
                                        break;
                                    }
                                    else {
#if 1 //DBG_27_DV
                                        printf("[DV] clean pipe get chq: %c \n", chq);
#endif
                                    }
                                }

                                ring_buf_init(usbTxd);
                                while (1) {
                                    chd = 0;
                                    pipRet = read(pipeRxd[0], &chd, 1);
                                    if (pipRet < 0) {
                                        break;
                                    }
                                    else {
#if 1 //DBG_27_DV
                                        printf("[DV] clean pipe get chd: %c \n", chd);
#endif
                                    }
                                }
                        
                                break;
                            }
                            else {
                                continue;
                            }
                        }
                        else if (cmd == 0x12) {
                            if (opc == 0x04) {
                                if (!puscur) {
                                    puscur = phost1;                    

                                    chq = 'd';
                                    pipRet = write(pipeTx[1], &chq, 1);
                                    if (pipRet < 0) {
                                        printf("[DV]  pipe send meta ret: %d \n", pipRet);
                                        goto end;
                                    }
                                    
                                    usbCur = puscur->pushring;
                                    piptx = puscur->pushtx;
                                    piprx = puscur->pushrx; 
                                }
                                else {
                                    /* should't be here */
                                }
                                
                                acusz = 0;
                                break;
                            }
                            if (opc == 0x05) {
                                if (!puscur) {
                                    puscur = phost1;                    

                                    chq = 'a';
                                    pipRet = write(pipeTx[1], &chq, 1);
                                    if (pipRet < 0) {
                                        printf("[DV]  pipe send meta ret: %d \n", pipRet);
                                        goto end;
                                    }

                                    chd = 's';
                                    pipRet = write(pipeTxd[1], &chd, 1);
                                    if (pipRet < 0) {
                                        printf("[DV]  pipe send meta ret: %d \n", pipRet);
                                        goto end;
                                    }

                                    usbCur = puscur->pushring;
                                    piptx = puscur->pushtx;
                                    piprx = puscur->pushrx; 
                                }
                                else if (puscur == phost1) {
                                    puscur = phost2;
                                    
                                    usbCur = puscur->pushring;
                                    piptx = puscur->pushtx;
                                    piprx = puscur->pushrx; 
                                }
                                else {
                                    /* should't be here */
                                }
                                
                                acusz = 0;
                                break;
                            }
                            else {
                                continue;
                            }
                        }
                        else {
                            continue;
                        }
                    }            
                }
                }
            }

        }

    end:
    
    return 0;
}

#define DBG_USB_HS 0
#define USB_HS_SAVE_RESULT 0
static int batusb_host(struct usbhost_s *puhs, char *strpath)
{
    struct pollfd ptfd[1];
    char ptrecv[32];
    int ptret=0, recvsz=0, acusz=0, wrtsz=0;
    int cntRecv=0, usCost=0, bufsize=0;
    int bufmax=0, idx=0, printlog=0;
    double throughput=0.0;
    struct timespec tstart, tend;
    struct aspMetaData_s meta, *pmeta=0;
    int len=0, pieRet=0;
    char *ptm=0, *pcur=0, *addr=0;
    char chr=0, opc=0, dat=0;
    char CBW[32] = {0x55, 0x53, 0x42, 0x43, 0x11, 0x22, 0x33, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    int usbid=0;
#if USB_HS_SAVE_RESULT
    FILE *fsave=0;
    char *pImage=0;
    static char ptfileSave[] = "/mnt/mmc2/usb/image_RX_%.3d.jpg";
    char ptfilepath[128];
#endif
    char cmdMtx[5][2] = {{'m', 0x01},{'d', 0x02},{'a', 0x03},{'s', 0x04},{'p', 0x05}};
    uint8_t cmdchr=0;
    struct shmem_s *pTx=0;
    char *pMta=0;
    int *pPtx=0, *pPrx=0;
    struct timespec utstart, utend;
    int tcnt=0;

    pTx = puhs->pushring;
    pMta = puhs->puhsmeta;
    pPtx = puhs->pushtx;
    pPrx = puhs->pushrx;
    
    usbid = open(strpath, O_RDWR);
    if (usbid < 0) {
        printf("can't open device[%s]\n", strpath); 
        close(usbid);
        goto end;
    }
    else {
        printf("open device[%s]\n", strpath); 
    }

    //usb_nonblock_set(usbid);

    while(1) {
        tcnt = 0;
#if 1
        ptfd[0].fd = pPtx[0];
        ptfd[0].events = POLLIN;

        while(1) {
            tcnt++;
            ptret = poll(ptfd, 1, 2000);
            printf("[%s] poll return %d evt: 0x%.2x - %d\n", strpath, ptret, ptfd[0].revents, tcnt);
            if (ptret > 0) {
                //sleep(2);
                read(pPtx[0], &chr, 1);
                printf("[%s] get chr: %c \n", strpath, chr);
                break;
            }
        }
#else
        while (1) {
            chr = 0;
            pieRet = read(pPtx[0], &chr, 1);
            if (pieRet > 0) {
                printf("[%s] get chr: %c \n", strpath, chr);
                break;
            } else {
                //printf("[HS] get chr ret: %d \n", pieRet);
                usleep(1000);
            }
        }
#endif
        memset(ptrecv, 0, 32);

#if DBG_USB_HS
        for (idx=0; idx < 5; idx++) {
            printf("%d. %c - %d \n", idx, cmdMtx[idx][0], cmdMtx[idx][1]);
        }
#endif

        switch (chr) {
        case 'm':
            cmdchr = cmdMtx[0][1];
            break;
        case 'd':
            opc = OP_SINGLE;
            dat = OPSUB_USB_Scan;
            cmdchr = cmdMtx[1][1];
            break;
        case 'a':
            opc = OP_DUPLEX;
            dat = OPSUB_USB_Scan;
            usleep(500000);
            //sleep(5);
            cmdchr = cmdMtx[1][1];
            break;
        case 's':
            opc = OP_DUPLEX;
            dat = OPSUB_USB_Scan;
            cmdchr = cmdMtx[1][1];
            break;
        case 'p':
            cmdchr = cmdMtx[4][1];
            break;
        default:
            break;
        }

        if (cmdchr == 0x01) { 
            pmeta = &meta;
            ptm = (char *)pmeta;
            
            msync(pMta, USB_METADATA_SIZE, MS_SYNC);
            memcpy(ptm, pMta, sizeof(struct aspMetaData_s));

            printf("[HS] get meta magic number: 0x%.2x, 0x%.2x !!!\n", meta.ASP_MAGIC[0], meta.ASP_MAGIC[1]);

#if 0 /* remove ready */    
            insert_cbw(CBW, CBW_CMD_READY, OP_Hand_Scan, OPSUB_USB_Scan);
            usb_send(CBW, usbid, 31);
#endif

            insert_cbw(CBW, CBW_CMD_SEND_OPCODE, OP_META, OP_META_Sub1);
            usb_send(CBW, usbid, 31);
    
            usb_send(ptm, usbid, USB_METADATA_SIZE);
            shmem_dump(ptm, USB_METADATA_SIZE);
                
            usb_read(ptrecv, usbid, 13);
#if DBG_USB_HS
            printf("[HS] dump 13 bytes");
            shmem_dump(ptrecv, 13);
#endif

            //chr = 'M';
            //pieRet = write(pPrx[1], &chr, 1);
        }
        else if (cmdchr == 0x02) {
        
#if 0 /* remove ready */
            insert_cbw(CBW, CBW_CMD_READY, 0, 0);
            usb_send(CBW, usbid, 31);
#endif

            insert_cbw(CBW, CBW_CMD_SEND_OPCODE, opc, dat);
            usb_send(CBW, usbid, 31);

            usb_read(ptrecv, usbid, 13);
#if DBG_USB_HS
            printf("[HS] dump 13 bytes");
            shmem_dump(ptrecv, 13);
#endif

            insert_cbw(CBW, CBW_CMD_START_SCAN, opc, dat);
            usb_send(CBW, usbid, 31);     
        
#if USB_HS_SAVE_RESULT
            fsave = find_save(ptfilepath, ptfileSave);
            if (!fsave) {
                goto end;    
            }

            bufmax = 8*1024*1024;
            pImage = malloc(bufmax);
            pcur = pImage;
#endif

            recvsz = 0;
            acusz = 0;
            tcnt = 0;

            len = ring_buf_get(pTx, &addr);    
            while (len <= 0) {
                sleep(1);
                printf("[%s]buffer full!!! ret:%d !!", strpath, len);
                len = ring_buf_get(pTx, &addr);            
            }
            
            while(1) {
            
#if 0 /* test drop line */
                usleep(10000);
#endif
                recvsz = usb_read(addr, usbid, len);
#if 0                
                if (tcnt) {
                    clock_gettime(CLOCK_REALTIME, &utend);
                    //usCost = test_time_diff(&utstart, &utend, 1000);
                    //printf("[%s] read %d (%d ms)\n", strpath, recvsz, usCost/1000);
                }
#endif 
                if (recvsz > 0) {
#if USB_HS_SAVE_RESULT
                    memcpy(pcur, addr, recvsz);
#endif
                    ring_buf_prod(pTx);        
                }
                
                if (recvsz < 0) {
                    printf("[HS] usb read ret: %d !!!\n", recvsz);
                    //continue;
                    break;
                }
                else if (recvsz == 0) {
                    continue;
                }
                else {
                    /*do nothing*/
                }

                tcnt ++;

                if (tcnt == 1) {
                    clock_gettime(CLOCK_REALTIME, &utstart);
                    printf("[%s] start ... \n", strpath);
                }
#if USB_HS_SAVE_RESULT        
                pcur += recvsz;
#endif
                acusz += recvsz;

                if (recvsz < len) {
                    clock_gettime(CLOCK_REALTIME, &utend);
                    ring_buf_set_last(pTx, recvsz);
                    break;
                } else {
                    chr = 'D';
                    pieRet = write(pPrx[1], &chr, 1);
                }
        
#if USB_HS_SAVE_RESULT               
                if (acusz > bufmax) {
                    printf("[HS] save image error due to buffer size not enough!!!");
                    break;
                }
#endif

                len = ring_buf_get(pTx, &addr);
                while (len <= 0) {
                    sleep(1);
                    printf("[HS](%s) buffer full!!! ret:%d !!", strpath, len);
                    len = ring_buf_get(pTx, &addr);            
                }
            }

            chr = 'E';
            pieRet = write(pPrx[1], &chr, 1);
    
#if USB_HS_SAVE_RESULT
            wrtsz = fwrite(pImage, 1, acusz, fsave);
#endif
            usCost = test_time_diff(&utstart, &utend, 1000);
            throughput = acusz*8.0 / usCost*1.0;

            printf("[HS] total read size: %d, write file size: %d throughput: %lf Mbits \n", acusz, wrtsz, throughput);
            
#if USB_HS_SAVE_RESULT
            sync();
            fclose(fsave);
            free(pImage);
#endif

        }
    }
end:

    if (usbid) close(usbid);
    
    while (1) {
        chr = 0;
        pieRet = read(pPtx[0], &chr, 1);
        if (pieRet > 0) {
            printf("[%s] get chr: %c \n", strpath, chr);
        } else {
            //printf("[HS] get chr ret: %d \n", pieRet);
            usleep(1000);
        }
    }
    
    return 0;
}

int main(int argc, char *argv[]) 
{ 
    uint32_t bitset;
    int sel, arg0 = 0, arg1 = 0, arg2 = 0, arg3 = 0, arg4 = 0;
    int ret; 

    struct pollfd ptfd[1];
    static char strMetaPath[] = "/mnt/mmc2/usb/meta.bin";
    static char ptdevpath[] = "/dev/g_printer";
    static char pthostpath1[] = "/dev/usb/lp0";
    static char pthostpath2[] = "/dev/usb/lp1";
    char ptfilepath[128];

    double throughput=0.0;
    FILE *fsave=0;
    int pipeTx[2], pipeRx[2], pipRet=0, lens=0, pipeTxd[2], pipeRxd[2];

    struct aspMetaData_s *metaRx = 0;
    struct usbhost_s *pushost=0, *pushostd=0, *puscur=0;
    struct usbdev_s *pusdev=0;
    struct shmem_s *usbTx=0, *usbTxd=0;
    
    char endTran[64] = {};
    uint8_t cmd=0, opc=0, dat=0;
    uint32_t usbentsRx=0, usbentsTx=0, getents=0;
    char *metaPt=0, *addrd=0;
    int thrid[5];

    struct epoll_event eventRx, eventTx, getevents[MAX_EVENTS];
    int usbfd=0, epollfd=0, uret=0, ifx=0, rxfd=0, txfd=0;

    arg0 = 0;
    arg1 = 0;
    arg2 = 0;
         
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
        
    batTotSalloc = (int *)mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    if (!batTotSalloc) goto end;

    pusdev = (struct usbdev_s *)malloc(sizeof(struct usbdev_s));
    
    pushost = (struct usbhost_s *)malloc(sizeof(struct usbhost_s));
    usbTx = (struct shmem_s *)aspSalloc(sizeof(struct shmem_s));

    usbTx->pp = memory_init(&usbTx->slotn, USB_RING_BUFF_NUM*bufsize, bufsize); // 150MB 
    if (!usbTx->pp) goto end;
    usbTx->r = (struct ring_p *)aspSalloc(sizeof(struct ring_p));
    usbTx->totsz = USB_RING_BUFF_NUM*bufsize;
    usbTx->chksz = bufsize;
    usbTx->svdist = 8;

    pushostd = (struct usbhost_s *)malloc(sizeof(struct usbhost_s));
    usbTxd = (struct shmem_s *)aspSalloc(sizeof(struct shmem_s));

    usbTxd->pp = memory_init(&usbTxd->slotn, USB_RING_BUFF_NUM*bufsize, bufsize); // 150MB
    if (!usbTxd->pp) goto end;
    usbTxd->r = (struct ring_p *)aspSalloc(sizeof(struct ring_p));
    usbTxd->totsz = USB_RING_BUFF_NUM*bufsize;
    usbTxd->chksz = bufsize;
    usbTxd->svdist = 8;

    metaRx = (struct aspMetaData_s *)aspSalloc(sizeof(struct aspMetaData_s));
    metaPt = (char *)metaRx;

#if 0
    pipe(pipeTx);
    pipe(pipeRx);

    pipe(pipeTxd);
    pipe(pipeRxd);
#else
    pipe2(pipeTx, O_NONBLOCK);
    pipe2(pipeRx, O_NONBLOCK);

    pipe2(pipeTxd, O_NONBLOCK);
    pipe2(pipeRxd, O_NONBLOCK);
#endif

    pushost->pushring = usbTx;
    pushost->puhsmeta = metaPt;
    pushost->pushrx = pipeRx;
    pushost->pushtx = pipeTx;

    pushostd->pushring = usbTxd;
    pushostd->puhsmeta = metaPt;
    pushostd->pushrx = pipeRxd;
    pushostd->pushtx = pipeTxd;

    pusdev->pushost1 = pushost;
    pusdev->pushost2 = pushostd;
    
    if (usbfd){ /* usb printer test usb scam */
                
        
        
        palloc = ptsend;
        
        thrid[0] = fork();
        if (!thrid[0]) {
            printf("fork pid: %d\n", thrid[0]);
        }
        else {
            printf("[PID] create thread 0 id: %d \n", thrid[0]);

            thrid[1] = fork();

            if (thrid[1] < 0) {
            }
            else if (thrid[1] > 0) {
                printf("[PID] create thread 1 id: %d \n", thrid[1]);
            } else {

                thrid[2] = fork();
                if (!thrid[2]) {
                
                    printf("[DVF] meta usb host _0_ start, PID[%d] \n", thrid[2]);
                    ret = batusb_device(pusdev, pthostpath1);
                    printf("[DVF] meta usb host _0_ end, PID[%d] ret: %d \n", thrid[2], ret);
                    exit(0);
                    goto  end;
                } else {
                    printf("[PID] create thread 2 id: %d exit\n", thrid[2]);
                    printf("[DVF] meta usb host _0_ PID[%d] \n", thrid[2]);
                }
                
                thrid[3] = fork();
                if (!thrid[3]) {
                
                    printf("[DVF] meta usb host _1_ start, PID[%d] \n", thrid[3]);
                    ret = batusb_host(pushost, pthostpath1);
                    printf("[DVF] meta usb host _1_ end, PID[%d] ret: %d \n", thrid[3], ret);
                    exit(0);
                    goto  end;
                } else {
                    printf("[PID] create thread 3 id: %d exit\n", thrid[3]);
                    printf("[DVF] meta usb host _1_ PID[%d] \n", thrid[3]);
                }
            
                thrid[4] = fork();
                if (!thrid[4]) {
                
                    printf("[DVF] meta usb host _2_ start, PID[%d] \n", thrid[4]);
                    ret = batusb_host(pushostd, pthostpath2);
                    printf("[DVF] meta usb host _2_ end, PID[%d] ret: %d \n", thrid[4], ret);
                    exit(0);
                    goto  end;
                } else {
                    printf("[PID] create thread 4 id: %d exit\n", thrid[4]);
                    printf("[DVF] meta usb host _2_ PID[%d] \n", thrid[4]);
                }

                exit(0);
            }
#if 0
            if (thrid[2]) {
                kill(thrid[2], SIGKILL);
            }

            if (thrid[4]) {
                kill(thrid[4], SIGKILL);
            }
#endif

            for(cntTx=0; cntTx < 5; cntTx++) {
                printf("[PID] thread id: %d - %d\n", thrid[cntTx], cntTx);
            }

            cntTx = 0;
            ptfd[0].fd = usbfd;
            ptfd[0].events = POLLOUT | POLLIN;
            while(1) {
                cntTx++;
                ptret = poll(ptfd, 1, 5000);
                printf("[TH0] poll return %d evt: 0x%.2x - %d\n", ptret, ptfd[0].revents, cntTx);
                if (ptret > 0) {
                    sleep(5);
                }
            }
            
        }

        goto end;
    }
 end:
        if (usbfd) close(usbfd);
        if (palloc) free(palloc);
 
}
