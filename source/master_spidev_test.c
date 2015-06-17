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

#define SPI_TRUNK_SZ 32768
#define TRUNK_SIZE SPI_TRUNK_SZ

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
#define OP_SDAT 0xb
#define OP_RGRD 0xc
#define OP_RGWT 0xd
#define OP_RGDAT 0xe
#define OP_ACTION 0xf

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
#define OP_INTERIMG      0x2c

#define OP_SUP               0x31

#define SEC_LEN 512
static void pabort(const char *s) 
{ 
    perror(s); 
    abort(); 
} 
 
static const char *device = "/dev/spidev32765.0"; 
char data_path[256] = "/root/tx/1.jpg"; 
static uint8_t mode; 
static uint8_t bits = 8; 
static uint32_t speed = 30000000; 
static uint16_t delay; 
static uint16_t command = 0; 
static uint8_t loop = 0; 

static int mem_dump(char *src, int size)
{
    int inc;
    if (!src) return -1;

    inc = 0;
    printf("memdump[0x%.8x] sz%d: \n", src, size);
    while (inc < size) {
        printf("%.2x ", *src);

        inc++;
        src++;
        if (!((inc+1) % 16)) {
            printf("\n");
        }
    }

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

    struct spi_ioc_transfer tr = {  //?﹍てspi_ioc_transfer?疼蔨 
        .tx_buf = (unsigned long)tx, 
        .rx_buf = (unsigned long)rx, 
        .len = BUFF_SIZE,
        .delay_usecs = delay, 
        .speed_hz = speed, 
        .bits_per_word = bits, 
    }; 
    
    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);   //ioctl纐?巨,???誹 
    if (ret < 1) 
        pabort("can't send spi message"); 
 
    errcnt = 0; i = 0;
    for (ret = 0; ret < BUFF_SIZE; ret++) { //ゴ钡Μ??? 
        if (!(ret % 6))     //6??誹?罫ゴ 
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
  if (ret < 1)
      pabort("can't send spi message");

  //printf("tx/rx len: %d\n", ret);

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
  if (ret < 1)
      pabort("can't send spi message");

  //printf("tx/rx len: %d\n", ret);

  free(tr);
  return ret;
}

static void _tx_data(int fd, uint8_t *rx_buff, uint8_t *tx_buff, int ex_size)
{
  int ret, i, errcnt; 

  uint8_t tg;
  uint8_t *tx = tx_buff;
  uint8_t *rx = rx_buff;
  struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = ex_size,
        .delay_usecs = delay,
        .speed_hz = speed,
        .bits_per_word = bits,
    };
    
  ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
  if (ret < 1)
      pabort("can't send spi message");
        
  //  errcnt = 0; i = 0;
  //  for (ret = 0; ret < ex_size; ret++) { //ゴ钡Μ??? 
  //      if (!(ret % 6))     //6??誹?罫ゴ 
  //          puts(""); 
    //          tg = (ret - 0) & 0xff;
    //          if (rx[ret] != tg) {
    //          errcnt++;
    //          i = 1;
    //            }
  //      printf("%.2X:%.2X/%d ", rx[ret], tg, i); 
    //          i  = 0;
  //  } 
  //  puts(""); 
  //  printf(" error count: %d\n", errcnt);
  puts("");
}
#endif
static void print_usage(const char *prog)   //?????ゴ?獺 
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
        static const struct option lopts[] = {  //??㏑ 
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
        case '3':   //3???家Α 
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
		strcpy(data_path, optarg);
            //data_path = optarg;
            break;
        default:    //???? 
            print_usage(argv[0]); 
            break; 
        } 
    } 
} 
#if 1
int main(int argc, char *argv[]) 
{ 
static char spi0[] = "/dev/spidev32765.0"; 
static char spi1[] = "/dev/spidev32766.0"; 
    uint32_t bitset;
    int sel, arg0 = 0, arg1 = 0, arg2 = 0, arg3 = 0, arg4 = 0;
    int fd, ret; 
    arg0 = 0;
	arg1 = 0;
	arg2 = 0;
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

        uint8_t *tx_buff, *rx_buff;
        FILE *fpd;
        int fsize, buffsize;
        buffsize = 128*1024*1024;
        tx_buff = malloc(buffsize);
        if (tx_buff) {
            printf(" tx buff alloc success!!\n");
        }
        rx_buff = malloc(buffsize);
        if (rx_buff) {
            printf(" rx buff alloc success!!\n");
        }

	if (((sel == 11) || (sel == 13)) && (argc > 3))
		strcpy(data_path, argv[3]);
        /* open target file which will be transmitted */
        printf(" open file %s \n", data_path);
        fpd = fopen(data_path, "r");
       
        if (!fpd) {
            printf(" %s file open failed \n", data_path);
            goto end;
        }
	
        printf(" %s file open succeed \n", data_path);
        /* read the file into Tx buffer */
        //fsize = fread(tx_buff, 1, buffsize, fpd);
        //printf(" [%s] size: %d \n", data_path, fsize);

        FILE *fp;
        static char data_save[] = "/mnt/mmc2/rx/%d.bin"; 
        static char path[256];

        fp = find_save(path, data_save);
        if (!fp) {
            printf("find save dst failed ret:%d\n", fp);
            goto end;
        } else
            printf("find save dst succeed ret:%d\n", fp);

        int fd0, fd1;
        fd0 = open(spi0, O_RDWR);
        if (fd0 < 0) {
            printf("can't open device[%s]\n", spi0); 
		goto end;
        }
        else 
            printf("open device[%s]\n", spi0); 
        fd1 = open(spi1, O_RDWR);
        if (fd1 < 0) {
                printf("can't open device[%s]\n", spi1); 
		goto end;
        }
        else 
            printf("open device[%s]\n", spi1); 


    /*
     * spi0 mode 
     */ 
    ret = ioctl(fd0, SPI_IOC_WR_MODE, &mode);    //?家Α 
    if (ret == -1) 
        pabort("can't set spi mode"); 
 
    ret = ioctl(fd0, SPI_IOC_RD_MODE, &mode);    //?家Α 
    if (ret == -1) 
        pabort("can't get spi mode"); 

   /*
     * spi1 mode
     */ 
    ret = ioctl(fd1, SPI_IOC_WR_MODE, &mode);    //?家Α 
    if (ret == -1) 
        pabort("can't set spi mode"); 
 
    ret = ioctl(fd1, SPI_IOC_RD_MODE, &mode);    //?家Α 
    if (ret == -1) 
        pabort("can't get spi mode"); 
	

       int fm[2] = {fd0, fd1};
	char rxans[512];
	char tx[512];
	char rx[512];
	int i;
	for (i = 0; i < 512; i++) {
		rxans[i] = i & 0x95;
		tx[i] = i & 0x95;
	}

    if (sel == 19){ /* command mode test ex[19 startsec secNum filename.bin]*/ /* OP_SDWT */

        int startSec = 0, secNum = 0;
        int startAddr = 0, bLength = 0;
        char diskpath[128];
        FILE *dkf = 0;
        char *outbuf, *total=0;
        int ret = 0, len = 0, cnt = 0, acusz = 0, max = 0;
        int sLen = 0;

        startSec = arg0;
        secNum = arg1;

        /* open target file which will be transmitted */
        strcpy(diskpath, argv[4]);
        printf(" open file [%s] \n", diskpath);
        dkf = fopen(diskpath, "r");
       
        if (!dkf) {
            printf(" [%s] file open failed \n", diskpath);
            goto end;
        }	
        printf(" [%s] file open succeed \n", diskpath);

        bitset = 0;
        ret = ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        printf("Set spi%d data mode: %d\n", 0, bitset);

        bitset = 1;
        ret = ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
        printf("Set spi%d slve ready: %d\n", 0, bitset);

        startAddr = startSec * SEC_LEN;
        bLength = secNum * SEC_LEN;

        ret = fseek(dkf, 0, SEEK_END);
        if (ret) {
            printf("seek file failed: ret:%d \n", ret);
            goto end;
        }

        max = ftell(dkf);
        if ((startAddr + bLength) > max) {
            printf("dump size overflow start:%d len:%d max:%d\n", startAddr, bLength, max);
            //goto end;
        } else {
            printf("disk dump, start:%d len:%d max:%d\n", startAddr, bLength, max);
        }

        max = startAddr + bLength;
        total = mmap(NULL, max, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);;
        if (!total) {
            goto end;
        }

        ret = fseek(dkf, 0, SEEK_SET);
        if (ret) {
            printf("seek file failed: ret:%d \n", ret);
            goto end;
        }
        
        ret = fread(total, 1, max, dkf);
        printf("read file size: %d/%d \n", ret, max);

        outbuf = total + startAddr;
        //mem_dump(outbuf, bLength);
        acusz = bLength;

        cnt = 0;
        while (acusz>0) {
            if (acusz > TRUNK_SIZE) {
                sLen = TRUNK_SIZE;
                acusz -= sLen;
            } else {
                sLen = acusz;
                acusz = 0;
            }

            len = tx_data(fm[0], outbuf, outbuf, 1, sLen, 1024*1024);
            printf("[%d]Send %d/%d bytes!!\n", cnt, len, sLen);

            cnt++;
            outbuf += len;
    	 }

        usleep(100000);

        bitset = 0;
        ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        printf("[R]Set spi%d RDY pin: %d, finished!! \n", 0, bitset);

        usleep(100000);

        bitset = 0;
        ioctl(fm[0], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
        printf("[R]Get spi%d RDY pin: %d \n", 0, bitset);

        usleep(100000);

        bitset = 0;
        ioctl(fm[0], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
        printf("[R]Get spi%d RDY pin: %d \n", 0, bitset);

        ret = fseek(dkf, startAddr, SEEK_SET);
        if (ret) {
            printf("seek file failed: ret:%d \n", ret);
            goto end;
        }

        fclose(dkf);

        printf(" reopen file [%s] \n", diskpath);
        dkf = fopen(diskpath, "rw");
       
        if (!dkf) {
            printf(" [%s] file reopen failed \n", diskpath);
            goto end;
        }
        printf(" [%s] file reopen succeed \n", diskpath);

        ret = fseek(dkf, 0, SEEK_SET);
        if (ret) {
            printf("seek file failed: ret:%d \n", ret);
            goto end;
        }
        
        outbuf = total + startAddr;
        //mem_dump(outbuf, bLength);
        ret = fwrite(total, 1, max, dkf);
        printf("write file size: %d/%d \n", ret, max);
        fflush(dkf);
        fclose(dkf);

        munmap(total, max);
        goto end;
    }

    if (sel == 18){ /* command mode test ex[18 startsec secNum filename.bin]*/

        int startSec = 0, secNum = 0;
        int startAddr = 0, bLength = 0;
        char diskpath[128];
        FILE *dkf = 0;
        char *outbuf;
        int ret = 0, len = 0, cnt = 0, acusz = 0, max = 0;
        int sLen = 0;

        startSec = arg0;
        secNum = arg1;

        /* open target file which will be transmitted */
        strcpy(diskpath, argv[4]);
        printf(" open file [%s] \n", diskpath);
        dkf = fopen(diskpath, "r");
       
        if (!dkf) {
            printf(" [%s] file open failed \n", diskpath);
            goto end;
        }	
        printf(" [%s] file open succeed \n", diskpath);

        bitset = 0;
        ret = ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        printf("Set spi%d data mode: %d\n", 0, bitset);

        bitset = 1;
        ret = ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
        printf("Set spi%d slve ready: %d\n", 0, bitset);

        startAddr = startSec * SEC_LEN;
        bLength = secNum * SEC_LEN;

        ret = fseek(dkf, 0, SEEK_END);
        if (ret) {
            printf("seek file failed: ret:%d \n", ret);
            goto end;
        }

        max = ftell(dkf);
        if ((startAddr + bLength) > max) {
            printf("dump size overflow start:%d len:%d max:%d\n", startAddr, bLength, max);
            //goto end;
        } else {
            printf("disk dump, start:%d len:%d max:%d\n", startAddr, bLength, max);
        }

        ret = fseek(dkf, startAddr, SEEK_SET);
        if (ret) {
            printf("seek file failed: ret:%d \n", ret);
            goto end;
        }

        ret = fread(tx_buff, 1, bLength, dkf);
        printf("read file size: %d/%d \n", ret, bLength);

        acusz = bLength;
        outbuf = tx_buff;
        cnt = 0;
        while (acusz>0) {
            if (acusz > TRUNK_SIZE) {
                sLen = TRUNK_SIZE;
                acusz -= sLen;
            } else {
                sLen = acusz;
                acusz = 0;
            }

            len = tx_data(fm[0], rx_buff, outbuf, 1, sLen, 1024*1024);
            printf("[%d]Send %d/%d bytes!!\n", cnt, len, sLen);

            cnt++;
            outbuf += len;
    	 }

        usleep(100000);

        bitset = 0;
        ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        printf("[R]Set spi%d RDY pin: %d, finished!! \n", 0, bitset);

        usleep(100000);

        bitset = 0;
        ioctl(fm[0], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
        printf("[R]Get spi%d RDY pin: %d \n", 0, bitset);

        usleep(100000);

        bitset = 0;
        ioctl(fm[0], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
        printf("[R]Get spi%d RDY pin: %d \n", 0, bitset);

        goto end;
    }

    if (sel == 17){ /* command mode test ex[17 1/0]*/

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

        memset(tx_buff, TRUNK_SIZE, 0x55);

            memset(rx_buff, TRUNK_SIZE, 0xff);

            len = tx_data(fm[spis], rx_buff, tx_buff, 1, TRUNK_SIZE, 1024*1024);

            printf("receive %d bytes, write to file %d bytes \n", cnt, len, ret);

        acusz += len;
        printf("file save [%s], receive total size: %d \n", path, acusz);

        goto end;
    }
    if (sel == 16){ /* command mode test ex[16 num]*/
#define ARRY_MAX  61
        int ret=0;
        uint8_t tx8[4], rx8[4];
        uint8_t op[ARRY_MAX] = {	0xaa
							, 	OP_PON,	 		OP_QRY,	 		OP_RDY,	 		OP_DAT,	 		OP_SCM			/* 0x01 -0x05 */
							, 	OP_DCM,		 	OP_FIH,	 		OP_DUL,	 		OP_SDRD,	 	OP_SDWT          	/* 0x06 -0x0a */
							, 	OP_SDAT,	 	OP_RGRD,	 	OP_RGWT,	 	OP_RGDAT,	 	OP_ACTION		/* 0x0b -0x0f  */
							, 	OP_NONE,	 	OP_NONE,	 	OP_NONE,		OP_NONE,		OP_STSEC_0 		/* 0x10 -0x14  */
							,	OP_STLEN_0,		OP_NONE,		OP_NONE,		OP_RGADD_H,	OP_RGADD_L 		/* 0x15 -0x19  */
							,	OP_NONE,		OP_NONE,		OP_NONE,		OP_NONE,		OP_NONE     		/* 0x1A -0x1E  */
							,	OP_NONE,		OP_FFORMAT,		OP_COLRMOD,	OP_COMPRAT,	OP_SCANMOD     	/* 0x1F -0x23  */
							,	OP_DATPATH,		OP_RESOLTN,		OP_SCANGAV,	OP_MAXWIDH,	OP_WIDTHAD_H	/* 0x24 -0x28  */
							,	OP_WIDTHAD_L,	OP_SCANLEN_H,	OP_SCANLEN_L,	OP_NONE,		OP_NONE		/* 0x29 -0x2D  */
							,	OP_NONE,		OP_NONE,		OP_NONE,		OP_SUP,		OP_NONE		/* 0x2E -0x32  */
							,	OP_NONE,		OP_NONE,		OP_NONE,		OP_NONE,		OP_NONE		/* 0x33 -0x37  */
							,	OP_NONE,		OP_NONE,		OP_NONE,		OP_NONE,		OP_MAX};		/* 0x38 -0x3C  */
        uint8_t st[4] = {OP_STSEC_0, OP_STSEC_1, OP_STSEC_2, OP_STSEC_3};
        uint8_t ln[4] = {OP_STLEN_0, OP_STLEN_1, OP_STLEN_2, OP_STLEN_3};
        uint8_t staddr = 0, stlen = 0, btidx = 0;

        tx8[0] = op[arg0];
        tx8[1] = arg1;

        if (arg0 > ARRY_MAX) {
            printf("Error!! Index overflow!![%d]\n", arg0);
            goto end;
        }

        btidx = arg2 % 4;
        if (op[arg0] == OP_STSEC_0) {
            staddr = arg1 & (0xff << (8 * btidx));
            
            tx8[0] = st[btidx];
            tx8[1] = staddr;
            
            printf("start secter: %.8x, send:%.2x \n", arg1, staddr);
        }

        if (op[arg0] == OP_STLEN_0) {
            stlen = arg1 & (0xff << (8 * btidx));
            tx8[0] = ln[btidx];
            tx8[1] = stlen;
            
            printf("secter length: %.8x, send:%.2x\n", arg1, stlen);
        }
/*
        bitset = 1;
        ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
        printf("Set spi 0 slave ready: %d\n", bitset);
*/
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
    if (sel == 15) { /* 16Bits inform mode [15 spi size bits]*/
        int ret=0;
        uint16_t *tx16, *rx16, *tmp16;
        uint8_t *tx8, *rx8, *tmp8;
        tx16 = malloc(SPI_TRUNK_SZ);
        rx16 = malloc(SPI_TRUNK_SZ);
        tx8 = malloc(SPI_TRUNK_SZ);
        rx8 = malloc(SPI_TRUNK_SZ);

        int i;
        tmp8 = (uint8_t *)tx16;
        for(i = 0; i < SPI_TRUNK_SZ; i++) {
            *tmp8 = i & 0xff;
            tmp8++;
        }

        tmp8 = (uint8_t *)tx8;
        for(i = 0; i < SPI_TRUNK_SZ; i++) {
            *tmp8 = i & 0xff;
            tmp8++;
        }

        arg0 = arg0 % 2;
        bits = arg2;
        ret = ioctl(fm[arg0], SPI_IOC_WR_BITS_PER_WORD, &bits);
        if (ret == -1) 
            pabort("can't set bits per word"); 
 
        ret = ioctl(fm[arg0], SPI_IOC_RD_BITS_PER_WORD, &bits); 
        if (ret == -1) 
            pabort("can't get bits per word"); 

        if (bits == 16) {
            ret = tx_data_16(fm[arg0], rx16, tx16, 1, arg1, SPI_TRUNK_SZ);
            int i;
            tmp16 = rx16;
            for (i = 0; i < ret; i+=2) {
                if (((i % 16) == 0) && (i != 0)) printf("\n");
                printf("0x%.4x ", *tmp16);
                tmp16++;
            }
            printf("\n");
        }
        if (bits == 8) {
            ret = tx_data(fm[arg0], rx8, tx8, 1, arg1, SPI_TRUNK_SZ);
            int i;
            tmp8 = rx8;
            for (i = 0; i < ret; i+=1) {
                if (((i % 16) == 0) && (i != 0)) printf("\n");
                printf("0x%.2x ", *tmp8);
                tmp8++;
            }
            printf("\n");

        }

        printf("spi%d bits: %d txsize: %d/%d\n", arg0, bits, ret, arg1);
        goto end;
    }
    if (sel == 14) { /* dual continuous command mode [14 20 path1 path2 pktsize] ex: 14 20 ./01.mp4 ./02.mp4 512 */
#define DCTSIZE (128*1024*1024)
#define PKTSZ  SPI_TRUNK_SZ
        int chunksize, acusz;
        chunksize = PKTSZ;
        FILE *fp2, *fpd1, *fpd2;
        char svpath2[128], srcpath1[128], srcpath2[128];

        strcpy(svpath2, data_save);
        fp2 = find_save(svpath2, data_save);
        if (!fp2) {
            printf("find save dst failed ret:%d\n", fp2);
            goto end;
        } else
            printf("find save dst [%s] succeed ret:%d\n", svpath2, fp2);

        if (argv[3]) {
     		strcpy(srcpath1, argv[3]);
              fpd1 = fopen(srcpath1, "r");
     		if (!fpd1) {
            	    printf(" %s file open failed \n", srcpath1);
	           goto end;
        	} else
        	        printf(" %s file open succeed \n", srcpath1);
        }

        if (argv[4]) {
     		strcpy(srcpath2, argv[4]);
              fpd2 = fopen(srcpath2, "r");
     		if (!fpd2) {
            	    printf(" %s file open failed \n", srcpath2);
	           goto end;
        	} else
        	        printf(" %s file open succeed \n", srcpath2);
        }

        mode &= ~SPI_MODE_3;
        mode |= SPI_MODE_1;

        ret = ioctl(fm[0], SPI_IOC_WR_MODE, &mode);    //?家Α 
        if (ret == -1) 
            pabort("can't set spi mode"); 
 
        ret = ioctl(fm[0], SPI_IOC_RD_MODE, &mode);    //?家Α 
        if (ret == -1) 
            pabort("can't get spi mode"); 
	
        printf("spi%d mode:0x%x \n", 0, mode);

        ret = ioctl(fm[1], SPI_IOC_WR_MODE, &mode);    //?家Α 
        if (ret == -1) 
            pabort("can't set spi mode"); 
 
        ret = ioctl(fm[1], SPI_IOC_RD_MODE, &mode);    //?家Α 
        if (ret == -1) 
            pabort("can't get spi mode"); 
	
        printf("spi%d mode:0x%x \n", 1, mode);

        if (arg0)
            speed = arg0 * 1000000;

        /*
         * max speed hz     //?竚硉瞯
         */ 
        ret = ioctl(fm[0], SPI_IOC_WR_MAX_SPEED_HZ, &speed);   //?硉瞯 
        if (ret == -1) 
            pabort("can't set max speed hz"); 
        
        ret = ioctl(fm[0], SPI_IOC_RD_MAX_SPEED_HZ, &speed);   //?硉瞯 
        if (ret == -1) 
            pabort("can't get max speed hz"); 

        printf("spi%d max speed: %d Hz (%d KHz)\n", 0, speed, speed/1000); 

        ret = ioctl(fm[1], SPI_IOC_WR_MAX_SPEED_HZ, &speed);   //?硉瞯 
        if (ret == -1) 
            pabort("can't set max speed hz"); 
        
        ret = ioctl(fm[1], SPI_IOC_RD_MAX_SPEED_HZ, &speed);   //?硉瞯 
        if (ret == -1) 
            pabort("can't get max speed hz"); 

        printf("spi%d max speed: %d Hz (%d KHz)\n", 1, speed, speed/1000); 
		
    /*
     * bits per word    
     */ 
    ret = ioctl(fm[0], SPI_IOC_WR_BITS_PER_WORD, &bits);   
    if (ret == -1) 
        pabort("can't set bits per word"); 
 
    ret = ioctl(fm[0], SPI_IOC_RD_BITS_PER_WORD, &bits);   
    if (ret == -1) 
        pabort("can't get bits per word"); 

    printf("spi%d bits per word: %d\n", 0, bits);  

    ret = ioctl(fm[1], SPI_IOC_WR_BITS_PER_WORD, &bits);   
    if (ret == -1) 
        pabort("can't set bits per word"); 
 
    ret = ioctl(fm[1], SPI_IOC_RD_BITS_PER_WORD, &bits);   
    if (ret == -1) 
        pabort("can't get bits per word"); 

    printf("spi%d bits per word: %d\n", 1, bits); 

        
        bitset = 0;
        ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL
        
        bitset = 1;
        ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL
        
        int pksize = 512;
        int pknum = 120;
        int trunksz, remainsz, pkcnt;
        char *srcBuff1, *srctmp1, *srcBuff2, *srctmp2;
        int fsz1, fsz2;
        char * tbuff;
        int pid;
		
        if ((arg3) && (arg3 <= chunksize) && !(arg3%pksize) && !(chunksize%arg3)) {
        	pksize = arg3;
        }
        pknum = chunksize / pksize;
        printf("pksize:%d pknum:%d chunksize:%d\n", pksize, pknum, chunksize);
        struct timespec *tspi = (struct timespec *)mmap(NULL, sizeof(struct timespec) * 2, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);;
        if (!tspi) {
            printf("get share memory for timespec failed - %d\n", tspi);
            goto end;
        }
        int tcost, tlast;
        struct tms time;
        struct timespec curtime, tdiff[2];
        unsigned long long cur, tnow, lnow, past, tbef, lpast, tmp;
        
        trunksz = pknum * pksize;
        
        pkcnt = 0;
        tbuff = tx_buff;
        
        clock_gettime(CLOCK_REALTIME, &curtime);
        printf("%llu, %llu \n", curtime.tv_sec, curtime.tv_nsec);
        times(&time);
        printf("%llu %llu %llu %llu \n", time.tms_utime, time.tms_stime, time.tms_cutime, time.tms_cstime);
        
        
        srcBuff1 = mmap(NULL, DCTSIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
        srctmp1 = srcBuff1;

        srcBuff2 = mmap(NULL, DCTSIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
        srctmp2 = srcBuff2;
        
        fsz1 = fread(srcBuff1, 1, DCTSIZE, fpd1);
        printf(" [%s] size: %d, read to share memory\n", srcpath1, fsz1);

        fsz2 = fread(srcBuff2, 1, DCTSIZE, fpd2);
        printf(" [%s] size: %d, read to share memory\n", srcpath2, fsz2);

/*
        memset(srcBuff1, 0xf0, fsz1);
        memset(srcBuff2, 0xf0, fsz2);
*/
        memset(tx_buff, 0xf0, trunksz);
        
/*
	memcpy(srcBuff, tx_buff, fsize);
	printf(" [%s] size: %d, copy to share memory\n", data_path, fsize);
*/

        bitset = 1;
        ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
        printf("Set spi 0 slave ready: %d\n", bitset);

        bitset = 1;
        ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
        printf("Set spi 0 slave ready: %d\n", bitset);

        bitset = 0;
        ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        printf("Set data mode: %d\n", bitset);
        
        bitset = 1;
        ioctl(fm[0], _IOR(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_RD_DATA_MODE
        printf("Get data mode: %d\n", bitset);
        
        bitset = 0;
        ioctl(fm[0], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
        printf("Get RDY pin: %d\n", bitset);

        bitset = 0;
        ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        printf("Set data mode: %d\n", bitset);
        
        bitset = 1;
        ioctl(fm[1], _IOR(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_RD_DATA_MODE
        printf("Get data mode: %d\n", bitset);
        
        bitset = 0;
        ioctl(fm[1], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
        printf("Get RDY pin: %d\n", bitset);
                
        acusz = 0;
 
        int sid;
        sid = fork();
        if (sid) {

        remainsz = fsz1;

        while (1) {
            if (remainsz < trunksz) {
                if (remainsz < pksize)
                    pknum = 1;
                else {
                     pknum = remainsz / pksize;
                     if (remainsz % pksize) pknum += 1;
                }
                remainsz = 0;
            } else {
                remainsz -= trunksz;
            }

            clock_gettime(CLOCK_REALTIME, &tdiff[0]);            
#if 1 /* send real data */
            ret = tx_data(fm[0], srcBuff1, srcBuff1, pknum, pksize, 1024*1024);
#else
            ret = tx_data(fm[0], srcBuff1, tx_buff, pknum, pksize, 1024*1024);
#endif
            clock_gettime(CLOCK_REALTIME, &tspi[0]);   

            msync(&tspi[1], sizeof(struct timespec), MS_SYNC);
            tlast = test_time_diff(&tspi[1], &tdiff[0], 1000);
            tcost = test_time_diff(&tdiff[0], &tspi[0], 1000);

            acusz += ret;
            printf("[%d] tx %d - %d(%d us/ %d us)\n", pkcnt, ret, acusz, tcost, tlast);
            srcBuff1 += ret;

            if (pkcnt == 0) {
                clock_gettime(CLOCK_REALTIME, &curtime);
                cur = curtime.tv_sec;
                tnow = curtime.tv_nsec;
                lnow = cur * 1000000000+tnow;
                printf("[p0] enter %d t:%llu %llu %llu\n", remainsz,cur,tnow, lnow/1000000);
            }
			
            pkcnt++;
            
            if (remainsz <= 0) break;
            //usleep(1);
        }

        clock_gettime(CLOCK_REALTIME, &curtime);
        past = curtime.tv_sec;
        tbef = curtime.tv_nsec;		
        lpast = past * 1000000000+tbef;	
        
        printf("time cose: %llu s, bandwidth: %llu Mbits/s \n", (lpast - lnow)/1000000000, ((fsz1*8)/((lpast - lnow)/1000000000)) /1000000 );
        
        sleep(3);
        
        bitset = 1;
        ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        printf("[cmd]Set RDY pin: %d cnt:%d\n",  bitset,pkcnt);

        sleep(1);
        
        bitset = 0;
        ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        printf("[cmd]Set RDY pin: %d cnt:%d\n", bitset, pkcnt);

        sleep(2);

        bitset = 0;
        ioctl(fm[0], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
        printf("[%d]Get RDY pin: %d cnt:%d\n", pid, bitset, pkcnt);
/*
        msync(srctmp1, acusz, MS_SYNC);
        ret = fwrite(srctmp1, 1, acusz, fp);
        printf("recv data save to [%s] size: %d/%d \n", path, ret, acusz);
        fflush(fp);
*/
        fclose(fp);

        } else {
        remainsz = fsz2;

        while (1) {
            if (remainsz < trunksz) {
                if (remainsz < pksize)
                    pknum = 1;
                else {
                     pknum = remainsz / pksize;
                     if (remainsz % pksize) pknum += 1;
                }
                remainsz = 0;
            } else {
                remainsz -= trunksz;
            }
            clock_gettime(CLOCK_REALTIME, &tdiff[1]);            
#if 1 /* send real data */
            ret = tx_data(fm[1], srcBuff2, srcBuff2, pknum, pksize, 1024*1024);
#else
            ret = tx_data(fm[1], srcBuff2, tx_buff, pknum, pksize, 1024*1024);
#endif
            clock_gettime(CLOCK_REALTIME, &tspi[1]);   

            msync(&tspi[0], sizeof(struct timespec), MS_SYNC);
            tlast = test_time_diff(&tspi[0], &tdiff[1], 1000);
            tcost = test_time_diff(&tdiff[1], &tspi[1], 1000);

            acusz += ret;
            printf("[%d] tx %d - %d(%d us /%d us)\n", pkcnt, ret, acusz, tcost, tlast);
            srcBuff2 += ret;

            if (pkcnt == 0) {
                clock_gettime(CLOCK_REALTIME, &curtime);
                cur = curtime.tv_sec;
                tnow = curtime.tv_nsec;
                lnow = cur * 1000000000+tnow;
                printf("[p0] enter %d t:%llu %llu %llu\n", remainsz,cur,tnow, lnow/1000000);
            }
			
            pkcnt++;
            
            if (remainsz <= 0) break;
            //usleep(1);
        }

        clock_gettime(CLOCK_REALTIME, &curtime);
        past = curtime.tv_sec;
        tbef = curtime.tv_nsec;		
        lpast = past * 1000000000+tbef;	
        
        tmp = (lpast - lnow);
        if (tmp < 1000000000) {
            printf("time cose: %llu us, bandwidth: %llu Bits/s \n",  tmp/1000, (fsize*8)/(tmp/1000));            
        } else {
            printf("time cose: %llu s, bandwidth: %llu MBits/s \n",  tmp/1000000000, ((fsize*8)/((lpast - lnow)/1000000)) /1000 );            
        }

        
        sleep(3);
        
        bitset = 1;
        ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        printf("[cmd]Set RDY pin: %d cnt:%d\n",  bitset,pkcnt);

        sleep(1);
        
        bitset = 0;
        ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        printf("[cmd]Set RDY pin: %d cnt:%d\n", bitset, pkcnt);

        sleep(2);

        bitset = 0;
        ioctl(fm[1], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
        printf("[%d]Get RDY pin: %d cnt:%d\n", pid, bitset, pkcnt);
/*
        msync(srctmp2, acusz, MS_SYNC);
        ret = fwrite(srctmp2, 1, acusz, fp2);
        printf("recv data save to [%s] size: %d/%d \n", svpath2, ret, acusz);
        fflush(fp2);
*/
        fclose(fp2);
        }

        munmap(srctmp1, DCTSIZE);
        munmap(srctmp2, DCTSIZE);
        goto end;
    }
    if (sel == 13) { /* continuous command mode [13 20 path pktsize spi] ex: 13 20 ./01.mp3 512 1*/
#define TSIZE (128*1024*1024)
#define PKTSZ  SPI_TRUNK_SZ
#define SAVE_FILE 0
#define USE_SHARE_MEM 1
#define MEASURE_TIME_DIFF 0

        int chunksize, acusz;
        chunksize = PKTSZ;

        mode &= ~SPI_MODE_3;
        mode |= SPI_MODE_1;

        arg3 = arg3 % 2;

        printf("spi select: [%d] \n", arg3);

        ret = ioctl(fm[arg3], SPI_IOC_WR_MODE, &mode);    //?家Α 
        if (ret == -1) 
            pabort("can't set spi mode"); 
 
        ret = ioctl(fm[arg3], SPI_IOC_RD_MODE, &mode);    //?家Α 
        if (ret == -1) 
            pabort("can't get spi mode"); 
	
        printf("spi%d mode:0x%x \n", arg3, mode);

        if (arg0)
            speed = arg0 * 1000000;
        /*
         * max speed hz     //?竚硉瞯
         */ 
        ret = ioctl(fm[arg3], SPI_IOC_WR_MAX_SPEED_HZ, &speed);   //?硉瞯 
        if (ret == -1) 
            pabort("can't set max speed hz"); 
        
        ret = ioctl(fm[arg3], SPI_IOC_RD_MAX_SPEED_HZ, &speed);   //?硉瞯 
        if (ret == -1) 
            pabort("can't get max speed hz"); 
        //ゴ家Α,–ぶ㎝硉瞯獺 
        printf("spi mode: %d\n", mode); 
        printf("bits per word: %d\n", bits); 
        printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000); 
        
        bitset = 0;
        ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL
        
        bitset = 1;
        ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL
        
        int pksize = 512;
        int pknum = 120;
        int trunksz, remainsz, pkcnt;
        char *srcBuff, *srctmp;
        char * tbuff;
        int pid;
		
        if ((arg2) && (arg2 <= chunksize) && !(arg2%pksize) && !(chunksize%arg2)) {
        	pksize = arg2;
        }
        pknum = chunksize / pksize;
        printf("pksize:%d pknum:%d chunksize:%d\n", pksize, pknum, chunksize);
        struct tms time;
        struct timespec curtime;
        unsigned long long cur, tnow, lnow, past, tbef, lpast, tmp, tdiff, torg;
        
        trunksz = pknum * pksize;
        
        pkcnt = 0;
        tbuff = tx_buff;
        
        clock_gettime(CLOCK_REALTIME, &curtime);
        printf("%llu, %llu \n", curtime.tv_sec, curtime.tv_nsec);
        times(&time);
        printf("%llu %llu %llu %llu \n", time.tms_utime, time.tms_stime, time.tms_cutime, time.tms_cstime);
        
#if USE_SHARE_MEM
        srcBuff = mmap(NULL, TSIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
        srctmp = srcBuff;
        memset(srcBuff, 0xf0, TSIZE);
        msync(srcBuff, TSIZE, MS_SYNC);
#else
        srcBuff = rx_buff;
        srctmp = srcBuff;
        memset(srcBuff, 0xf0, buffsize);
#endif

#if USE_SHARE_MEM
        fsize = fread(srcBuff, 1, TSIZE, fpd);
        printf(" [%s] size: %d, read to share memory\n", data_path, fsize);
#else
        printf(" [%s] NOT read to share memory\n", data_path);
#endif  
        //memset(srcBuff, 0xf0, fsize);
        memset(tx_buff, 0xf0, trunksz);
        
        remainsz = fsize;
/*
	memcpy(srcBuff, tx_buff, fsize);
	printf(" [%s] size: %d, copy to share memory\n", data_path, fsize);
*/

        bitset = 1;
        ioctl(fm[arg3], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
        printf("Set spi %d slave ready: %d\n", arg3, bitset);

        bitset = 0;
        ioctl(fm[arg3], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        printf("Set spi%d data mode: %d\n", arg3, bitset);
        
        bitset = 1;
        ioctl(fm[arg3], _IOR(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_RD_DATA_MODE
        printf("Get spi%d data mode: %d\n", arg3, bitset);
        
        bitset = -1;
        ioctl(fm[arg3], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
        printf("Get spi%d RDY pin: %d\n", arg3, bitset);

        clock_gettime(CLOCK_REALTIME, &curtime);
        past = curtime.tv_sec;
        tbef = curtime.tv_nsec;		
        torg = past * 1000000000+tbef;	

        acusz = 0;
        while (1) {
#if USE_SHARE_MEM
            if (remainsz < trunksz) {
                if (remainsz < pksize)
                    pknum = 1;
                else {
                     pknum = remainsz / pksize;
                     if (remainsz % pksize) pknum += 1;
                }
                remainsz = 0;

            } else {
                remainsz -= trunksz;
            }
#else
            fsize = fread(srcBuff, 1, trunksz, fpd);
            if (fsize < trunksz) {
                if (fsize < pksize)
                    pknum = 1;
                else {
                     pknum = fsize / pksize;
                     if (fsize % pksize) pknum += 1;
                }
                fsize = 0;
            }
            remainsz = fsize;
#endif
            ret = tx_data(fm[arg3], srcBuff, srcBuff, pknum, pksize, 1024*1024);
            acusz += ret;
            printf("[%d] tx %d - %d\n", pkcnt, ret, acusz);
#if USE_SHARE_MEM
            srcBuff += ret;
#endif

#if MEASURE_TIME_DIFF
        clock_gettime(CLOCK_REALTIME, &curtime);
        past = curtime.tv_sec;
        tbef = curtime.tv_nsec;		
        lpast = past * 1000000000+tbef;	

        tdiff = (lpast - torg);
        if (tdiff < 1000000000) {
            printf("time diff: %llu us \n",  tdiff/1000);
        } else {
            printf("time diff: %llu s\n",  tdiff/1000000000);            
        }
#endif
        torg = lpast;

#if 0
            if (remainsz == 0) {
                /* pull low RDY right away at the end of tx */
                bitset = 0;
                ioctl(fm[arg3], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
                printf("[R]Set spi%d RDY pin: %d\n",  arg3, bitset);
            }
#endif
            if (pkcnt == 0) {
                clock_gettime(CLOCK_REALTIME, &curtime);
                cur = curtime.tv_sec;
                tnow = curtime.tv_nsec;
                lnow = cur * 1000000000+tnow;
                printf("[p%d] enter %d t:%llu %llu %llu\n", arg3, remainsz,cur,tnow, lnow/1000000);
            }
			
            pkcnt++;
            
            if (remainsz <= 0) break;
            //usleep(1);
        }

        clock_gettime(CLOCK_REALTIME, &curtime);
        past = curtime.tv_sec;
        tbef = curtime.tv_nsec;		
        lpast = past * 1000000000+tbef;	

        tmp = (lpast - lnow);
        if (tmp < 1000000000) {
            printf("time cose: %llu us, bandwidth: %llu Bits/s \n",  tmp/1000, (fsize*8)/(tmp/1000));            
        } else {
            printf("time cose: %llu s, bandwidth: %llu MBits/s \n",  tmp/1000000000, ((fsize*8)/((lpast - lnow)/1000000)) /1000 );            
        }

        usleep(50000);

        bitset = 0;
        ioctl(fm[arg3], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        printf("[R]Set spi%d RDY pin: %d, finished!! \n", arg3, bitset);

        usleep(300000);
		
        ioctl(fm[arg3], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
        printf("[R]Get spi%d RDY pin: %d, finished!! \n", arg3, bitset);

#if SAVE_FILE
        msync(srctmp, acusz, MS_SYNC);
        ret = fwrite(srctmp, 1, acusz, fp);
        printf("recv data save to [%s] size: %d/%d \n", path, ret, acusz);
#else
        printf("recv data NOT save to [%s] size: %d/%d \n", path, ret, acusz);
#endif
        fflush(fp);
        fclose(fp);

        munmap(srctmp, TSIZE);
        goto end;
    }
    if (sel == 12) { /* command mode [12 1 1]*/
	int pid = 0, i, ci, hi, hex;
	char str[128], *stop_at, ch, hx[2];
	ci = 0; hex = 0; hi = 0;
	arg1 = arg1 % 2;
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

    ret = ioctl(fm[arg1], SPI_IOC_WR_MODE, &mode);    //?家Α 
    if (ret == -1) 
        pabort("can't set spi mode"); 
 
    ret = ioctl(fm[arg1], SPI_IOC_RD_MODE, &mode);    //?家Α 
    if (ret == -1) 
        pabort("can't get spi mode"); 
	
	printf("spi%d mode:0x%x \n", arg1, mode);
	
	while (1) {
		ch = fgetc(stdin);

		if (ch != '\n') {
			hx[ci%2] = ch;
			if (!((ci + 1) % 2)) {
				hex = strtoul(hx, &stop_at, 16);
				str[hi] = hex;
				printf("get hex: %x: acu: ", hex);
				i = 0;
				hi++;
	                     while (i < hi) {
					printf("%x ", str[i]);
					i++;
				}
				printf("\n");
			}
			ci ++;
		} else {
			str[hi] = '\0';
			ret = tx_data(fm[arg1], rx_buff, str, 1, hi, 128);
			printf("spi send size: %d get: \n", ret);
			i = 0;
			while (i < ret) {
				printf("%.2x ", rx_buff[i]);
				i++;
			}
			printf("\n");
			ci = 0;
			hi = 0;
		}	
	}
	
	goto end;

	}
    if (sel == 11) { /* dual channel data mode ex:[./master_spi.bin 11 50 file_path 30720 30720 sleepus]*/

#define TIME_DIFF_MEAS  (1)

	#define TSIZE (128*1024*1024)
	#define PKTSZ  SPI_TRUNK_SZ
	int chunksize;
	chunksize = arg3;

	if (arg0)
		speed = arg0 * 1000000;

    /*
     * max speed hz     //?竚硉瞯
     */ 
    ret = ioctl(fm[0], SPI_IOC_WR_MAX_SPEED_HZ, &speed);   //?硉瞯 
    if (ret == -1) 
        pabort("can't set max speed hz"); 
 
    ret = ioctl(fm[0], SPI_IOC_RD_MAX_SPEED_HZ, &speed);   //?硉瞯 
    if (ret == -1) 
        pabort("can't get max speed hz"); 

    ret = ioctl(fm[1], SPI_IOC_WR_MAX_SPEED_HZ, &speed);   //?硉瞯 
    if (ret == -1) 
        pabort("can't set max speed hz"); 
 
    ret = ioctl(fm[1], SPI_IOC_RD_MAX_SPEED_HZ, &speed);   //?硉瞯 
    if (ret == -1) 
        pabort("can't get max speed hz"); 

    ret = ioctl(fm[0], SPI_IOC_RD_MODE, &mode);    //?家Α 
    if (ret == -1) 
        pabort("can't get spi mode"); 
	
	printf("spi%d mode:0x%x \n", 0, mode);

    ret = ioctl(fm[1], SPI_IOC_RD_MODE, &mode);    //?家Α 
    if (ret == -1) 
        pabort("can't get spi mode"); 
	
	printf("spi%d mode:0x%x \n", 1, mode);

 
    /*
     * bits per word    
     */ 
    ret = ioctl(fm[0], SPI_IOC_WR_BITS_PER_WORD, &bits);   
    if (ret == -1) 
        pabort("can't set bits per word"); 
 
    ret = ioctl(fm[0], SPI_IOC_RD_BITS_PER_WORD, &bits);   
    if (ret == -1) 
        pabort("can't get bits per word"); 
 
    ret = ioctl(fm[1], SPI_IOC_WR_BITS_PER_WORD, &bits);   
    if (ret == -1) 
        pabort("can't set bits per word"); 
 
    ret = ioctl(fm[1], SPI_IOC_RD_BITS_PER_WORD, &bits);   
    if (ret == -1) 
        pabort("can't get bits per word"); 

    //ゴ家Α,–ぶ㎝硉瞯獺 
    printf("spi mode: %d\n", mode); 
    printf("bits per word: %d\n", bits); 
    printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000); 
	
        bitset = 0;
        ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL

        bitset = 1;
        ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL

        bitset = 1;
        ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
        printf("Set spi 0 slave ready: %d\n", bitset);

        bitset = 1;
        ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
        printf("Set spi 0 slave ready: %d\n", bitset);

		
        int pksize = 1024;
        int pknum = 60;
        int trunksz, remainsz, pkcnt;
	char *srcBuff, *srctmp;
	char * tbuff, buf;
	int pid;
        int pipefd[2];
        int pipefc[2];

	if ((arg2) && (arg2 <= chunksize) && !(arg2%pksize) && !(chunksize%arg2)) {
		pksize = arg2;
	}
	pknum = chunksize / pksize;
	printf("pksize:%d pknum:%d chunksize:%d\n", pksize, pknum, chunksize);
	
        struct timespec *tspi = (struct timespec *)mmap(NULL, sizeof(struct timespec) * 2, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);;
        if (!tspi) {
            printf("get share memory for timespec failed - %d\n", tspi);
            goto end;
        }

        int tcost, tlast;
        struct tms time;
        struct timespec curtime, tdiff[2];

	unsigned long long cur, tnow, lnow, past, tbef, lpast, tmp;
	
        trunksz = pknum * pksize;

        pkcnt = 0;
	tbuff = tx_buff;
	
	clock_gettime(CLOCK_REALTIME, &curtime);
	printf("%llu, %llu \n", curtime.tv_sec, curtime.tv_nsec);
	times(&time);
	printf("%llu %llu %llu %llu \n", time.tms_utime, time.tms_stime, time.tms_cutime, time.tms_cstime);


	srcBuff = mmap(NULL, TSIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	srctmp = srcBuff;

        fsize = fread(srcBuff, 1, TSIZE, fpd);
        printf(" [%s] size: %d, read to share memory\n", data_path, fsize);

        remainsz = fsize;
/*
	memcpy(srcBuff, tx_buff, fsize);
	printf(" [%s] size: %d, copy to share memory\n", data_path, fsize);
*/
        bitset = 1;
        ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        printf("spi0 Set RDY pin: %d\n", bitset);

        bitset = 1;
        ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        printf("spi1 Set RDY pin: %d\n", bitset);

        bitset = 0;
        ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        printf("spi0 Set data mode: %d\n", bitset);
		
        bitset = 0;
        ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        printf("spi1 Set data mode: %d\n", bitset);
        
        pipe(pipefd);
        pipe(pipefc);

	pid = fork();
	printf("pid: %d \n", pid);

	if (pid) {
              close(pipefd[0]); // close the read-end of the pipe, I'm not going to use it
              close(pipefc[1]);

            	if (remainsz < trunksz) {
			if (remainsz < pksize)
				pknum = 1;
			else {
				pknum = remainsz / pksize;
				 if (remainsz % pksize)
					pknum += 1;
			}
			remainsz = 0;
            	} else {
			remainsz -= trunksz * 2;
		}
					
		ret = tx_data(fm[0], NULL, srcBuff, pknum, pksize, 1024*1024);
              write(pipefd[1], "d", 1); // send the content of argv[1] to the reader
		srcBuff += ret + chunksize;
		pkcnt++;

		clock_gettime(CLOCK_REALTIME, &curtime);
		cur = curtime.tv_sec;
		tnow = curtime.tv_nsec;
		lnow = cur * 1000000000+tnow;
		printf("[p0] enter %d t:%llu %llu %llu\n", remainsz,cur,tnow, lnow/1000000);

		while (remainsz > 0) {
	
	            	if (remainsz < trunksz) {
				if (remainsz < pksize)
					pknum = 1;
				else {
					pknum = remainsz / pksize;
					 if (remainsz % pksize)
						pknum += 1;
				}
				remainsz = 0;
	            	} else {
				remainsz -= trunksz * 2;
			}

                     ret = read(pipefc[0], &buf, 1); 
#if TIME_DIFF_MEAS
                     clock_gettime(CLOCK_REALTIME, &tdiff[0]);            
#endif
			ret = tx_data(fm[0], NULL, srcBuff, pknum, pksize, 1024*1024);
#if TIME_DIFF_MEAS
                     clock_gettime(CLOCK_REALTIME, &tspi[0]);   
                     tlast = test_time_diff(&tspi[1], &tdiff[0], 1000);
                     tcost = test_time_diff(&tdiff[0], &tspi[0], 1000);
			printf("[p0] tx %d (%d us /%d us)\n", ret, tcost, tlast);

                     if (arg4) {
                         usleep(arg4);
                     }
#endif
                     msync(tspi, sizeof(struct timespec)*2, MS_SYNC);
                     write(pipefd[1], "d", 1); // send the content of argv[1] to the reader
/*
if (((srcBuff - srctmp) < 0x28B9005) && ((srcBuff - srctmp) > 0x28B8005)) {
	char *ch;
	ch = srcBuff;;
	printf("0x%.8x: \n", (uint32_t)(ch - srctmp));
	for (sel = 0; sel < 1024; sel++) {
		printf("%.2x ", *ch);
		if (!((sel + 1) % 16)) printf("\n");
		ch++;
	}
}
*/
			//printf("[p0] tx %d fd%d\n", ret, 0);
			srcBuff += ret + chunksize;

			pkcnt++;

		}
              close(pipefd[1]); // close the write-end of the pipe, thus sending EOF to the reader
              close(pipefc[0]);
	}else {
              close(pipefd[1]); // close the write-end of the pipe, I'm not going to use it
              close(pipefc[0]);
		remainsz -= trunksz;
		
		srcBuff += trunksz;
		while (1) {
			if (remainsz <= 0) break;
			
	            	if (remainsz < trunksz) {
				if (remainsz < pksize)
					pknum = 1;
				else {
					pknum = remainsz / pksize;
					 if (remainsz % pksize)
						pknum += 1;
				}
				remainsz = 0;
	            	} else {
				remainsz -= trunksz * 2;
			}

                     read(pipefd[0], &buf, 1); 
                     //clock_gettime(CLOCK_REALTIME, &curtime);
                     //cur = curtime.tv_sec;
                     //tnow = curtime.tv_nsec;
                     //now = cur * 1000000000+tnow;
                     //printf("[p1] enter %d t:%llu %llu %llu\n", remainsz,cur,tnow, lnow/1000000);
#if TIME_DIFF_MEAS                     
                     clock_gettime(CLOCK_REALTIME, &tdiff[1]); 
#endif          
			ret = tx_data(fm[1], NULL, srcBuff, pknum, pksize, 1024*1024);		 
#if TIME_DIFF_MEAS                     
                     clock_gettime(CLOCK_REALTIME, &tspi[1]);   
                     tlast = test_time_diff(&tspi[0], &tdiff[1], 1000);
                     tcost = test_time_diff(&tdiff[1], &tspi[1], 1000);

			printf("[p1] tx %d (%d us /%d us)\n", ret, tcost, tlast);
                     if (arg4) {
                         usleep(arg4);
                     }
#endif
                     msync(tspi, sizeof(struct timespec) * 2, MS_SYNC);
                     write(pipefc[1], "d", 1); // send the content of argv[1] to the reader
/*
if (((srcBuff - srctmp) < 0x28B9005) && ((srcBuff - srctmp) > 0x28B8005)) {
	char *ch;
	ch = srcBuff;;
	printf("0x%.8x: \n", (uint32_t)(ch - srctmp));
	for (sel = 0; sel < 1024; sel++) {
		printf("%.2x ", *ch);
		if (!((sel + 1) % 16)) printf("\n");
		ch++;
	}
}
*/
			//printf("[p1] tx %d fd%d\n", ret, 1);
			srcBuff += ret + chunksize;
		}
             close(pipefd[0]); // close the read-end of the pipe
             close(pipefc[1]);
	}


	clock_gettime(CLOCK_REALTIME, &curtime);
	past = curtime.tv_sec;
	tbef = curtime.tv_nsec;		
	lpast = past * 1000000000+tbef;	

        tmp = (lpast - lnow);
        if (tmp < 1000000000) {
            printf("time cose: %llu us, bandwidth: %llu Bits/s \n",  tmp/1000, (fsize*8)/(tmp/1000));            
        } else {
            printf("time cose: %llu s, bandwidth: %llu MBits/s \n",  tmp/1000000000, ((fsize*8)/((lpast - lnow)/1000000)) /1000 );            
        }

	
	usleep(3000);

        bitset = 1;
        ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        printf("[%d]Set RDY pin: %d cnt:%d\n", pid, bitset,pkcnt);


        bitset = 1;
        ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        printf("[%d]Set RDY pin: %d cnt:%d\n", pid, bitset,pkcnt);

        usleep(300000);
	
        bitset = 0;
        ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        printf("[%d]Set RDY pin: %d cnt:%d\n", pid, bitset, pkcnt);

        bitset = 0;
        ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        printf("[%d]Set RDY pin: %d cnt:%d\n", pid, bitset, pkcnt);

        usleep(300000);
	
        bitset = 1;
        ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        printf("[%d]Set RDY pin: %d cnt:%d\n", pid, bitset, pkcnt);

        bitset = 1;
        ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        printf("[%d]Set RDY pin: %d cnt:%d\n", pid, bitset, pkcnt);
	
        bitset = 0;
        ioctl(fm[0], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
        printf("[%d]Get RDY pin: %d cnt:%d\n", pid, bitset, pkcnt);

        bitset = 0;
        ioctl(fm[1], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
        printf("[%d]Get RDY pin: %d cnt:%d\n", pid, bitset, pkcnt);


	munmap(srctmp, TSIZE);
	goto end;
    }

    if (sel == 10) { /* command mode */
	while (1) {
	        bitset = 0;
       	 ioctl(fd0, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
        	 printf("Get RDY pin: %d\n", bitset);
		if (bitset == 1)
			break;
		sleep(3);
	}

	tx[0] = 0x53;
	tx[1] = 0x80;
	ret = tx_command(fd0, rx, tx, 512);
	printf("Send cmd 0x%x 0x%x ret:%d\n", tx[0], tx[1], ret);
	ret = chk_reply(rx, rxans, 512);
	printf("check reply ret: %d \n", ret);

	if (ret == 0) {
		bitset = 0;
        	ioctl(fd1, _IOW(SPI_IOC_MAGIC, 7, __u32), &bitset);   //SPI_IOC_WR_CS_PIN
        	printf("Set CS pin: %d\n", bitset);
	}
        goto end;
    }
    if (sel == 3) { /* set cs pin */
        bitset = arg0;
        ioctl(fm[arg1], _IOW(SPI_IOC_MAGIC, 7, __u32), &bitset);   //SPI_IOC_WR_CS_PIN
        printf("Set CS pin: %d\n", arg0);
        goto end;
    }
    if (sel == 4) { /* set RDY pin */
        bitset = arg0;
        ioctl(fm[arg1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        printf("Set RDY pin: %d\n", arg0);
        goto end;
    }
    if (sel == 5) { /* get RDY pin */
        bitset = 0;
        ioctl(fm[arg1], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
        printf("Get RDY pin: %d\n", bitset);
        goto end;
    }
    if (sel == 6) { /* read File */
    /* prepare transmitting */
    /* Tx/Rx buffer alloc */       
        uint8_t *tx_buff, *rx_buff;
        FILE *fpd;
        int fsize, buffsize;
        buffsize = 1*1024*1024;
        tx_buff = malloc(buffsize);
        if (tx_buff) {
            printf(" tx buff alloc success!!\n");
        }
        rx_buff = malloc(buffsize);
        if (rx_buff) {
            printf(" rx buff alloc success!!\n");
        }
       
        /* open target file which will be transmitted */
        printf(" open file %s \n", data_path);
        fpd = fopen(data_path, "r");
       
        if (!fpd) {
            printf(" %s file open failed \n", data_path);
            goto end;
        }
	
        printf(" %s file open succeed \n", data_path);
        /* read the file into Tx buffer */
        fsize = fread(tx_buff, 1, buffsize, fpd);
        printf(" [%s] size: %d \n", data_path, fsize);

        free(tx_buff);
        free(rx_buff);	
        goto end;
    }
    
    if (sel == 7) { /* get slve ready */
        bitset = 0;
        ioctl(fm[arg1], _IOR(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_RD_SLVE_READY
        printf("Get slave ready: %d\n", bitset);
        goto end;
    }

    if (sel == 2) { /* set slve ready */
        bitset = arg0;
        ioctl(fm[arg1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
        printf("Set slave ready: %d\n", arg0);
        goto end;
    }

    if (sel == 8) { /* get data mode */
        bitset = 0;
        ioctl(fm[arg1], _IOR(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_RD_DATA_MODE
        printf("Get spi%d data mode: %d\n", arg1, bitset);
        goto end;
    }

    if (sel == 9) { /* set data mode */
        bitset = arg0;
        ioctl(fm[arg1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        printf("Set spi%d data mode: %d\n", arg1, arg0);
        goto end;
    }

    ret = 0;

    
    while (1) {
        if (!(ret % 100000)) {
            
            if (sel == 1) {
                printf(" Tx bitset %d \n", bitset);
                ioctl(fd, _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WD_CTL_PIN
                if (bitset == 1)
                    bitset = 0;
                else
                    bitset = 1;            
            }
            else {
                printf(" Rx bitset %d \n", bitset);
                ret = ioctl(fd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN            
            }
        }
          
        ret++;
    }
end:
    free(rx_buff);
    free(tx_buff);
    close(fd0);
    close(fd1);
    fclose(fpd);
}
#else
int main(int argc, char *argv[]) 
{ 
    int ret = 0; 
    int fd; 
      uint8_t cmd_tx[16] = {0x53, 0x80,};
      uint8_t cmd_rx[16] = {0,};

      
    parse_opts(argc, argv); //秆猂?????? 
 
    fd = open(device, O_RDWR);  //ゴ???ゅン 
    if (fd < 0) 
        pabort("can't open device"); 
 
    /*
     * spi mode //?竚spi??家Α
     */ 
    ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);    //?家Α 
    if (ret == -1) 
        pabort("can't set spi mode"); 
 
    ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);    //?家Α 
    if (ret == -1) 
        pabort("can't get spi mode"); 
 
    /*
     * bits per word    //?竚–?ぶ
     */ 
    ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);   //? –?ぶ 
    if (ret == -1) 
        pabort("can't set bits per word"); 
 
    ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);   //? –?ぶ 
    if (ret == -1) 
        pabort("can't get bits per word"); 
 
    /*
     * max speed hz     //?竚硉瞯
     */ 
    ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);   //?硉瞯 
    if (ret == -1) 
        pabort("can't set max speed hz"); 
 
    ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);   //?硉瞯 
    if (ret == -1) 
        pabort("can't get max speed hz"); 
    //ゴ家Α,–ぶ㎝硉瞯獺 
    printf("spi mode: %d\n", mode); 
    printf("bits per word: %d\n", bits); 
    printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000); 

    /* spi work sequence start here */
    
    ioctl(fd, _IOW(SPI_IOC_MAGIC, 7, __u32), &bits);   //SPI_IOC_REL_CTL_PIN
    int cmd_size = 2;

    uint32_t bitset;    
    uint32_t temp32;
    uint32_t stage;
    uint32_t getbit;
    stage = 1;
    printf(" \n*****[%d]*****\n", stage++);
    /* 1. check control_pin ready or not */

    getbit = 0;
    temp32 = 2;
    while (getbit == 0) {
        ret = ioctl(fd, _IOR(SPI_IOC_MAGIC, 6, __u32), &getbit);   //SPI_IOC_RD_CTL_PIN
        if (ret == -1) 
            pabort("can't SPI_IOC_RD_CTL_PIN"); 
        if (temp32 != getbit) {
            printf("wait for slave power up, ctl_pin = %d \n", getbit);
            temp32 = getbit;
        }
    }
    

    getbit = 1;
    temp32 = 2;
    uint32_t getcmd;
    getcmd = 0;

    command = 0x5380;
    cmd_tx[0] = command & 0xff;
    cmd_tx[1] = (command >> 8) & 0xff;
    printf(" \n*****[%d]*****\n", stage++);
    /* 2. check slave ready or not */
    while (getcmd != 0x8053) {

        while (getbit == 1) {
            ret = ioctl(fd, _IOR(SPI_IOC_MAGIC, 6, __u32), &getbit);   //SPI_IOC_RD_CTL_PIN
            if (ret == -1) 
                pabort("can't SPI_IOC_RD_CTL_PIN"); 
            if (temp32 != getbit) {
                printf("wait for slave power up, ctl_pin = %d \n", getbit);
                temp32 = getbit;
            }
        }
        
        tx_command(fd, cmd_rx, cmd_tx, 2);
        getcmd = cmd_rx[0] | (cmd_rx[1] << 8);
        if (temp32 != getcmd) {
            printf(" get status:0x%x\n", getcmd);
            temp32 = getcmd;
        }
        if (getcmd == 0x0538) {
            command = 0x0538;
            cmd_tx[0] = command & 0xff;
            cmd_tx[1] = (command >> 8) & 0xff;
            printf(" tx:%x %x \n", cmd_tx[1], cmd_tx[0]);
        }
    }


	
    printf(" \n*****[%d]*****\n", stage++); 
    /* 3. send request transmitting command and check */
    /* to be done */
    command = 0x7290;
    cmd_tx[0] = command & 0xff;
    cmd_tx[1] = (command >> 8) & 0xff;
    printf(" tx:%x %x \n", cmd_tx[1], cmd_tx[0]);


    getcmd = 0;
    temp32 = 1;
    while (1) {
        while (getbit == 1) {
            ret = ioctl(fd, _IOR(SPI_IOC_MAGIC, 6, __u32), &getbit);   //SPI_IOC_RD_CTL_PIN
            if (ret == -1) 
                pabort("can't SPI_IOC_RD_CTL_PIN"); 
            if (temp32 != getbit) {
                printf("wait for slave power up, ctl_pin = %d \n", getbit);
                temp32 = getbit;
            }
        }
        
        printf(" tx:%x %x \n", cmd_tx[1], cmd_tx[0]);
        tx_command(fd, cmd_rx, cmd_tx, 2);
        
        if (getcmd == 0x9072) {
            bitset = 0;
            ioctl(fd, _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WD_CTL_PIN
            break;
        }        
        
        getcmd = cmd_rx[0] | (cmd_rx[1] << 8);
        printf(" get status:0x%x\n", getcmd);
        if (temp32 != getcmd) {
            printf(" get status:0x%x\n", getcmd);
            temp32 = getcmd;
        }
        if (getcmd == 0x0729) {
            command = 0x0729;
            cmd_tx[0] = command & 0xff;
            cmd_tx[1] = (command >> 8) & 0xff;
            printf(" tx:%x %x \n", cmd_tx[1], cmd_tx[0]);
        }
    }

    usleep(10);
    bitset = 1;
    ioctl(fd, _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WD_CTL_PIN
    usleep(10);    
    ioctl(fd, _IOW(SPI_IOC_MAGIC, 7, __u32), &bits);   //SPI_IOC_REL_CTL_PIN
    
    printf(" \n*****[%d]*****\n", stage++);
    /* 4. check control_pin ready or not */
    getbit = 1;
    temp32 = 2;
    while (getbit == 1) {
        ret = ioctl(fd, _IOR(SPI_IOC_MAGIC, 6, __u32), &getbit);   //SPI_IOC_RD_CTL_PIN
        if (ret == -1) 
            pabort("can't SPI_IOC_RD_CTL_PIN"); 
        if (temp32 != getbit) {
            printf("wait for slave ready for tx, ctl_pin = %d \n", getbit);
            temp32 = getbit;
        }
    }

    printf(" \n*****[%d]*****\n", stage++);    
    /* 5. trasmitting */                
    /* transmit the file piece by piece */
    #define PKT_SIZE 1024
    int usize;
    int count;
    
    count = 0;
    uint8_t *ptr;
    usize = 0;
    ptr = tx_buff;
    while (usize < fsize) {
        count++;

        getbit = 0;
        while (getbit == 0) {
            ioctl(fd, _IOR(SPI_IOC_MAGIC, 6, __u32), &getbit);   //SPI_IOC_RD_CTL_PIN
        }
        
        usize += PKT_SIZE;
        printf(" %d r:%d ",count, usize);
        tx_data(fd, rx_buff, ptr, PKT_SIZE);
        
        ptr += PKT_SIZE;
    }
    
    printf(" \n*****[%d]*****\n", stage++);
    /* 6. pull down the control_pin to notice the end of transmitting */
    bitset = 0;
    ioctl(fd, _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WD_CTL_PIN
    usleep(100);
    bitset = 1;
    ioctl(fd, _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WD_CTL_PIN
    
    ioctl(fd, _IOW(SPI_IOC_MAGIC, 7, __u32), &bits);   //SPI_IOC_REL_CTL_PIN
    usleep(100);
    
    printf(" \n*****[%d]*****\n", stage++);
    /* 7. request status to back to command mode */
    command = 0x5380;
    cmd_tx[0] = command & 0xff;
    cmd_tx[1] = (command >> 8) & 0xff;
    printf(" tx:%x %x \n", cmd_tx[1], cmd_tx[0]);
    
    getcmd = 0;
    temp32 = 1;
    while (getcmd != 0x8053) {
        getbit = 1;
        while (getbit == 1) {
            ret = ioctl(fd, _IOR(SPI_IOC_MAGIC, 6, __u32), &getbit);   //SPI_IOC_RD_CTL_PIN
            if (ret == -1) 
                pabort("can't SPI_IOC_RD_CTL_PIN"); 
            if (temp32 != getbit) {
                printf("wait for slave power up, ctl_pin = %d \n", getbit);
                temp32 = getbit;
            }
        }
        tx_command(fd, cmd_rx, cmd_tx, 2);
        getcmd = cmd_rx[0] | (cmd_rx[1] << 8);
        if (temp32 != getcmd) {
            printf(" get status:0x%x\n", getcmd);
            temp32 = getcmd;
        }
        if (getcmd == 0x0538) {
            command = 0x0538;
            cmd_tx[0] = command & 0xff;
            cmd_tx[1] = (command >> 8) & 0xff;
            printf(" tx:%x %x \n", cmd_tx[1], cmd_tx[0]);
        }
    }
    
    bitset = 0;
    ioctl(fd, _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WD_CTL_PIN
    usleep(100);
    bitset = 1;
    ioctl(fd, _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WD_CTL_PIN
    usleep(10);    
    ioctl(fd, _IOW(SPI_IOC_MAGIC, 7, __u32), &bits);   //SPI_IOC_REL_CTL_PIN
    
    printf(" \n*****[%d]*****\n", stage++);
    
    fclose(fpd);
    free(tx_buff);
    free(rx_buff);
         
    close(fd);  //???? 
 
    return ret; 
} 
#endif
