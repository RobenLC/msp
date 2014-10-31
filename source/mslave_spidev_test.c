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

#define TSIZE (128*1024*1024)
		
struct pipe_s{
    int rt[2];
};
    
static void pabort(const char *s) 
{ 
    perror(s); 
    abort(); 
} 
 
static const char *device = "/dev/spidev32765.0"; 
static const char *data_path = "/mnt/mmc2/tmp/1.jpg"; 
static uint8_t mode; 
static uint8_t bits = 8; 
static uint32_t speed = 1000000; 
static uint16_t delay; 
static uint16_t command = 0; 
static uint8_t loop = 0; 

 
static int test_gen(char *tx0, char *tx1, int len) {
    int i;
    char ch0[64], az;
    char ch1[64];
    az = 48;
    for (i = 0; i < 63; i++) {
    	ch0[i] = az++;
    	ch1[i] = az;
    }
    ch0[i] = '\n';
    ch1[i] = '\n';
    
    for (i = 0; i < len; i++) {
        tx0[i] = ch0[i%64];
        tx1[i] = ch1[i%64];
    }

    return i;
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

    struct spi_ioc_transfer tr = {  
        .tx_buf = (unsigned long)tx, 
        .rx_buf = (unsigned long)rx, 
        .len = BUFF_SIZE,
        .delay_usecs = delay, 
        .speed_hz = speed, 
        .bits_per_word = bits, 
    }; 
    
    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 1) 
        pabort("can't send spi message"); 
 
    errcnt = 0; i = 0;
    for (ret = 0; ret < BUFF_SIZE; ret++) {
        if (!(ret % 6))
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
    if (ret < 0)
        pabort("can't send spi message");
    
    //printf("tx/rx len: %d\n", ret);
    
    free(tr);
    return ret;
}

static void _tx_data(int fd, uint8_t *rx_buff, uint8_t *tx_buff, int ex_size, int num)
{
    #define PKT_SIZE 1024
    int ret, i, errcnt; 
    int remain;

    struct spi_ioc_transfer *tr = malloc(sizeof(struct spi_ioc_transfer) * num);
    
    uint8_t tg;
    uint8_t *tx = tx_buff;
    uint8_t *rx = rx_buff;  

    for (i = 0; i < num; i++) {
        tr[i].tx_buf = (unsigned long)tx;
        tr[i].rx_buf = (unsigned long)rx;
        tr[i].len = PKT_SIZE;
        tr[i].delay_usecs = delay;
        tr[i].speed_hz = speed;
        tr[i].bits_per_word = bits;
        
        tx += PKT_SIZE;
        rx += PKT_SIZE;
    }
    
    ret = ioctl(fd, SPI_IOC_MESSAGE(num), tr);
    if (ret < 1)
        pabort("can't send spi message");
    
    printf("tx/rx len: %d\n", ret);
    
    free(tr);
}
#endif
static void print_usage(const char *prog) 
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
        static const struct option lopts[] = {
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
        case '3':
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
            data_path = optarg;
            break;
        default:    //???? 
            print_usage(argv[0]); 
            break; 
        } 
    } 
} 
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

FILE *find_open(char *dst, char *tmple)
{
    FILE *f;
    sprintf(dst, tmple);
    f = fopen(dst, "w");
    return f;
}

static int data_process(char *rx, char *tx, FILE *fp, int fd, int pktsz, int num)
{
    int ret;
    int wtsz;
    int trunksz;
    trunksz = 1024 * num;

    ret = tx_data(fd, rx, tx, num, pktsz, 1024*1024);
    printf("%d rx %d\n", fd, ret);
    wtsz = fwrite(rx, 1, ret, fp);
    printf("%d wt %d\n", fd, wtsz);


}

int main(int argc, char *argv[]) 
{ 
static char spi0[] = "/dev/spidev32765.0"; 
static char spi1[] = "/dev/spidev32766.0"; 
//static char data_wifi[] = "/mnt/mmc2/tmp/1.jpg"; 
static char data_save[] = "/mnt/mmc2/rx/%d.bin"; 
static char path[256];

    uint32_t bitset;
    int sel, arg0 = 0, arg1 = 0, arg2 = 0, arg3 = 0, arg4 = 0;
    int fd, ret; 
    int buffsize;
    uint8_t *tx_buff[2], *rx_buff[2];
    FILE *fp;

    fp = find_save(path, data_save);
    if (!fp) {
        printf("find save dst failed ret:%d\n", fp);
        goto end;
    } else
        printf("find save dst succeed ret:%d\n", fp);

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

    buffsize = 1*1024*1024;

    tx_buff[0] = malloc(buffsize);
    if (tx_buff[0]) {
        printf(" tx buff 0 alloc success!!\n");
    } else {
        printf("Error!!buff alloc failed!!");
        goto end;
    }
    memset(tx_buff[0], 0xf0, buffsize);

    rx_buff[0] = malloc(buffsize);
    if (rx_buff[0]) {
        printf(" rx buff 0 alloc success!!\n");
    } else {
        printf("Error!!buff alloc failed!!");
        goto end;
    }
    memset(rx_buff[0], 0xf0, buffsize);

    tx_buff[1] = malloc(buffsize);
    if (tx_buff[1]) {
        printf(" tx buff 1 alloc success!!\n");
    } else {
        printf("Error!!buff alloc failed!!");
        goto end;
    }
    memset(tx_buff[1], 0, buffsize);

    rx_buff[1] = malloc(buffsize);
    if (rx_buff[1]) {
        printf(" rx buff 1 alloc success!!\n");
    } else {
        printf("Error!!buff alloc failed!!");
        goto end;
    }

    memset(rx_buff[1], 0, buffsize);

    ret = test_gen(tx_buff[0], tx_buff[1], buffsize);
    printf("tx pattern gen size: %d/%d \n", ret, buffsize);
	
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

        int fm[2] = {fd0, fd1};
        
    char rxans[512];
    char tx[512];
    char rx[512];
    int i;
    for (i = 0; i < 512; i++) {
        rxans[i] = i & 0x95;
        tx[i] = i & 0x95;
    }

    
    bitset = 0;
    ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL

    bitset = 1;
    ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL

    if (sel == 20){ /* command mode test ex[20 1 path1 path2]*/
#define TCSIZE 64*1024*1024
        FILE *fp1, *fp2;
        int fsz1, fsz2, acusz, send, txsz, pktcnt;
        char *src, *dstBuff1, *dstmp1, *dstBuff2, *dstmp2;
        char *save, *svBuff, *svtmp;	
        if (argc > 3) {
            fp1 = fopen(argv[3], "r");
        } else {
            printf("Error!! no input file 01\n");
        }
        if (!fp1) printf("Error!! file open [%s] failed\n", argv[3]);
        else printf("File open [%s] succeed\n", argv[3]);

        if (argc > 4) {
            fp2 = fopen(argv[4], "r");
        } else {
            printf("Error!! no input file 02 \n");
        }
        if (!fp2) printf("Error!! file open [%s] failed\n", argv[4]);
        else printf("File open [%s] succeed\n", argv[4]);

        dstBuff1 = mmap(NULL, TCSIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
        if (!dstBuff1) {
            printf("share memory alloc failed \n");
            goto end;
        }
	 memset(dstBuff1, 0x95, TCSIZE);
        dstmp1 = dstBuff1;

        dstBuff2 = mmap(NULL, TCSIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	 memset(dstBuff2, 0x95, TCSIZE);
        dstmp2 = dstBuff2;
        if (!dstBuff2) {
            printf("share memory alloc failed \n");
            goto end;
        }

        fsz1 = fread(dstBuff1, 1, TSIZE, fp1);
        printf("open [%s] size: %d \n", argv[3], fsz1);

        fsz2 = fread(dstBuff2, 1, TSIZE, fp2);
        printf("open [%s] size: %d \n", argv[4], fsz2);

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

        ret = ioctl(fm[0], SPI_IOC_WR_MODE, &mode);
        if (ret == -1) 
            pabort("can't set spi mode"); 
 
        ret = ioctl(fm[0], SPI_IOC_RD_MODE, &mode);
        if (ret == -1) 
            pabort("can't get spi mode"); 

        ret = ioctl(fm[1], SPI_IOC_WR_MODE, &mode);
        if (ret == -1) 
            pabort("can't set spi mode"); 
 
        ret = ioctl(fm[1], SPI_IOC_RD_MODE, &mode);
        if (ret == -1) 
            pabort("can't get spi mode"); 

        // disable data mode
        bitset = 0;
        ret = ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        printf("Set spi0 data mode: %d\n", bitset);

        bitset = 0;
        ret = ioctl(fm[0], _IOR(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_RD_DATA_MODE
        printf("Get spi0 data mode: %d\n", bitset);

        bitset = 0;
        ret = ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        printf("Set spi0 data mode: %d\n", bitset);

        bitset = 0;
        ret = ioctl(fm[1], _IOR(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_RD_DATA_MODE
        printf("Get spi0 data mode: %d\n", bitset);

        bitset = 1;
        ret = ioctl(fm[0], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_RD_CTL_PIN
        printf("Get spi1 RDY: %d\n", bitset);
        bitset = 0;
        ret = ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_WR_CTL_PIN
        printf("Set spi1 RDY: %d\n", bitset);

        bitset = 1;
        ret = ioctl(fm[1], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_RD_CTL_PIN
        printf("Get spi1 RDY: %d\n", bitset);
        bitset = 0;
        ret = ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_WR_CTL_PIN
        printf("Set spi1 RDY: %d\n", bitset);

        int sid;
        sid = fork();

        if (sid) {
			
        src = dstBuff1;

        acusz = 0; pktcnt = 0;
        while (acusz < fsz1) {
            if ((fsz1 - acusz) > 61440) {
                txsz = 61440;
            } else {
                txsz = fsz1 - acusz;
            }
			
            send = tx_data(fm[0], rx_buff[0], src, 1, txsz, 1024*1024);
            acusz += send;
            printf("[%d][%d] tx %d - %d\n", 0, pktcnt, send, acusz);
            if (send != txsz) {
                printf("[%d]Error! spi data tx did not complete %d/%d \n", 0, send, txsz);
                break;
            }
            src += send;

            pktcnt++;
        }

        }else {

        src = dstBuff2;

        acusz = 0; pktcnt = 0;
        while (acusz < fsz2) {
            if ((fsz2 - acusz) > 61440) {
                txsz = 61440;
            } else {
                txsz = fsz2 - acusz;
            }
			
            send = tx_data(fm[1], rx_buff[1], src, 1, txsz, 1024*1024);
            acusz += send;
            printf("[%d][%d] tx %d - %d\n", 1, pktcnt, send, acusz);
            if (send != txsz) {
                printf("[%d]Error! spi data tx did not complete %d/%d \n", 1, send, txsz);
                break;
            }
            src += send;

            pktcnt++;
        }
		
        }
		
		
        goto end;
    }

    if (sel == 19){ /* command mode test ex[19 1 path spi]*/
        FILE *f;
        int fsize, acusz, send, txsz, pktcnt;
        char *src, *dstBuff, *dstmp;
        char *save, *svBuff, *svtmp;	
        if (argc > 3) {
            f = fopen(argv[3], "r");
        } else {
            printf("Error!! no input file \n");
        }
        if (!f) printf("Error!! file open failed\n");

        dstBuff = mmap(NULL, TSIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	 memset(dstBuff, 0x95, TSIZE);
        dstmp = dstBuff;

        fsize = fread(dstBuff, 1, TSIZE, f);
        printf("open [%s] size: %d \n", argv[3], fsize);

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

       arg2 = arg2 % 2;

        ret = ioctl(fm[arg2], SPI_IOC_WR_MODE, &mode);
        if (ret == -1) 
            pabort("can't set spi mode"); 
 
        ret = ioctl(fm[arg2], SPI_IOC_RD_MODE, &mode);
        if (ret == -1) 
            pabort("can't get spi mode"); 


        // disable data mode
        bitset = 0;
        ret = ioctl(fm[arg2], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        printf("Set spi%d data mode: %d\n", arg2, bitset);

        bitset = 0;
        ret = ioctl(fm[arg2], _IOR(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_RD_DATA_MODE
        printf("Get spi%d data mode: %d\n", arg2, bitset);

        bitset = 1;
        ret = ioctl(fm[arg2], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_RD_CTL_PIN
        printf("Get spi%d RDY: %d\n", arg2, bitset);

        bitset = 0;
        ret = ioctl(fm[arg2], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_WR_CTL_PIN
        printf("Set spi%d RDY: %d\n", arg2, bitset);

/*
        bitset = 1;
        ret = ioctl(fm[arg2], _IOW(SPI_IOC_MAGIC, 7, __u32), &bitset);  //SPI_IOC_WR_CS_PIN
        printf("Set CS: %d\n", bitset);

        bitset = 0;
        ret = ioctl(fm[arg2], _IOR(SPI_IOC_MAGIC, 7, __u32), &bitset);  //SPI_IOC_RD_CS_PIN
        printf("Get CS: %d\n", bitset);
*/
        src = dstBuff;
        acusz = 0; pktcnt = 0;
        while (acusz < fsize) {
            if ((fsize - acusz) > 61440) {
                txsz = 61440;
            } else {
                txsz = fsize - acusz;
            }
			
            send = tx_data(fm[arg2], rx_buff[0], src, 1, txsz, 1024*1024);
            acusz += send;
            printf("[%d] tx %d - %d\n", pktcnt, send, acusz);
            if (send != txsz) {
                printf("Error! spi data tx did not complete %d/%d \n", send, txsz);
                break;
            }
            src += send;

            pktcnt++;
        }
/*
        bitset = 0;
        ret = ioctl(fm[(arg2+1)%2], _IOW(SPI_IOC_MAGIC, 7, __u32), &bitset);  //SPI_IOC_WT_CS_PIN
        printf("Set CS: %d\n", bitset);
*/

        bitset = 1;
        ret = ioctl(fm[arg2], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_RD_CTL_PIN
        printf("Get spi%d RDY: %d\n", arg2, bitset);

        goto end;
    }
	
    if (sel == 18){ /* command mode test ex[18 1 ]*/
        int sid[2], pid, gid, size, pi, opsz;
        struct pipe_s pipe_t;
        char str[128], op, *stop_at;;

        char *shmtx, *dbgbuf[2], *rxbuf;
        char *tmptx, *pdbg ;
		
        shmtx  = mmap(NULL, 2, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
        dbgbuf[0] = mmap(NULL, 4*1024*1024, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
        dbgbuf[1] = mmap(NULL, 4*1024*1024, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
        if (!shmtx) goto end;
        if (!dbgbuf[0]) goto end;
        if (!dbgbuf[1]) goto end;

        //memset(shmtx, 0xaa, 2);
        shmtx[0] = 0xf0;         shmtx[1] = 0xf0;
        memset(dbgbuf[0], 0xf0, 4*1024*1024);
        memset(dbgbuf[1], 0xf0, 4*1024*1024);

        tmptx = shmtx;

        rxbuf = rx_buff[0];
  
        pipe(pipe_t.rt);

        // don't pull low RDY after every transmitting

        bits = 16;
        ret = ioctl(fm[0], 	SPI_IOC_WR_BITS_PER_WORD, &bits);
        printf("Set spi0 data wide: %d\n", bits);

        ret = ioctl(fm[1], 	SPI_IOC_WR_BITS_PER_WORD, &bits);
        printf("Set spi1 data wide: %d\n", bits);
		
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
        ret = ioctl(fm[0], SPI_IOC_WR_MODE, &mode);
        if (ret == -1) 
            pabort("can't set spi mode"); 
 
        ret = ioctl(fm[0], SPI_IOC_RD_MODE, &mode);
        if (ret == -1) 
            pabort("can't get spi mode"); 

/*                        ret = read(pipefs[0], lastaddr, 32); */
        pid = getpid();
        sid[0] = -1; sid[1] = -1;
        sid[0] = fork();
        if (!sid[0]) { // process 1
             gid = pid + sid[0] + sid[1];
             printf("[1]%d\n", gid);
             close(pipe_t.rt[0]);
             rxbuf[0] = 'p';
             pdbg = dbgbuf[0];
             while (1) {
		  //printf("!");
                msync(shmtx, 2, MS_SYNC);
		  *pdbg = 0;
		  pdbg +=1;

                memcpy(pdbg, shmtx, 2);
                pdbg += 2;

		  *pdbg = 0;
		  pdbg +=1;
				
                ret = tx_data(fm[0], &rxbuf[1], shmtx, 1, 2, 1024);
                //printf("[%d]%x, %x \n", gid, rxbuf[1], rxbuf[2]);
                write(pipe_t.rt[1], rxbuf, 3);
                if (rxbuf[2] == 0xaa) {
                    size = pdbg - dbgbuf[0];
                    sprintf(str, "s0s%.8d", size);
                    write(pipe_t.rt[1], str, 11);
		      pdbg = dbgbuf[0];
                }
                if (rxbuf[2] == 0xee) {
                    write(pipe_t.rt[1], "e", 1);
                    break;
                }
                //usleep(1);
             }
             close(pipe_t.rt[1]);
        } else {
            sid[1] = fork();
            if (!sid[1]) { // process 2
                 gid = pid + sid[0] + sid[1];
                 printf("[2]%d\n", gid);
                 close(pipe_t.rt[0]);
                 rxbuf[0] = 'p';
                 pdbg = dbgbuf[1];
                 while (1) {
	       	//printf("?");
	                msync(shmtx, 2, MS_SYNC);
			  *pdbg = 0;
			  pdbg +=1;

	                memcpy(pdbg, shmtx, 2);
       	         pdbg += 2;

			  *pdbg = 0;
			  pdbg +=1;
				
                     ret = tx_data(fm[0], &rxbuf[1], shmtx, 1, 2, 1024);
                     //printf("[%d]%x, %x \n", gid, rxbuf[1], rxbuf[2]);
                     write(pipe_t.rt[1], rxbuf, 3);
                     if (rxbuf[2] == 0xaa) {
                         size = pdbg - dbgbuf[1];
                         sprintf(str, "s1s%.8d", size);
                         write(pipe_t.rt[1], str, 11);
			    pdbg = dbgbuf[1];
                     }
                     if (rxbuf[2] == 0xee) {
                         write(pipe_t.rt[1], "e", 1);
                         break;
                     }
	              //usleep(1);
                 }
                 close(pipe_t.rt[1]);
            } else { // process 0
                gid = pid + sid[0] + sid[1];
                 printf("[0]%d\n", gid);
                close(pipe_t.rt[1]);
                op = 0;
                while (1) {
#if 0
                    ret = read(pipe_t.rt[0], rxbuf, 3);
                    printf("[%d]size:%d, %x %x %x \n", gid, ret, rxbuf[0], rxbuf[1], rxbuf[2] );
#else
                    ret = read(pipe_t.rt[0], &op, 1);
                    if (ret > 0) {
                        if (op == 'p') {
                               ret = read(pipe_t.rt[0], rxbuf, 2);
       	                 printf("[%d] 0x%.2x 0x%.2x \n", gid, rxbuf[0], rxbuf[1]);
	                        shmtx[0] = rxbuf[0];
              	          shmtx[1] = rxbuf[1];
                        }
                        else if (op == 's') {
                               ret = read(pipe_t.rt[0], rxbuf, 10);
                               rxbuf[10] = '\0';
       	                 printf("[%d] %c \"%s\" \n", gid, op, rxbuf);
                               if (rxbuf[0] == '0') pi = 0;
                               else if (rxbuf[0] == '1') pi = 1;                               
                               else goto end;
                               size = strtoul(&rxbuf[2], &stop_at, 10);
                               opsz = 16 - (size % 16);
                               ret = fwrite(dbgbuf[pi], 1, size, fp);
				   printf("[%d]file %d write size: %d\n", gid, fp, ret);
                               ret = fwrite("================", 1, opsz, fp);
				   printf("[%d]file %d write size: %d\n", gid, fp, ret);
                        }
                        else if (op == 'e') {
      	                     printf("[%d] %c \n", gid, op);
                            break;
                        } else {
                            printf("[?] op:%x \n", op);
                        }
                    }
#endif
                }
                close(pipe_t.rt[0]);
                kill(sid[0]);
                kill(sid[1]);
            }
        }

        goto end;
    }
    if (sel == 17){ /* command mode test ex[17 6]*/
	int lsz=0, cnt=0;
	char *ch;
	
	mode |= SPI_MODE_1;
        ret = ioctl(fd0, SPI_IOC_WR_MODE, &mode);    //?家Α 
        if (ret == -1) 
            pabort("can't set spi mode"); 
 
        ret = ioctl(fd0, SPI_IOC_RD_MODE, &mode);    //?家Α 
        if (ret == -1) 
            pabort("can't get spi mode"); 

	if (arg0 > 0)
		lsz = arg0;

	while (lsz) {
		ch = tx_buff[lsz%2];
		*ch = 0x30 + cnt;
              ret = tx_data(fd0, rx_buff[0], tx_buff[lsz%2], 1, 512, 1024*1024);
		if (ret != 512)
			break;
		lsz --;
		cnt++;
	}
		
        goto end;
    }
		
    if (sel == 16){ /* tx hold test ex[16 0 0]*/
	arg1 = arg1%2;
       bitset = arg0;
	ioctl(fm[arg1], _IOW(SPI_IOC_MAGIC, 13, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
	printf("Set spi%d Tx Hold: %d\n", arg1, bitset);
        goto end;
    }

    if (sel == 15){ /* single band data mode test ex[15 0 3]*/
        int wtsz;

        arg0 = arg0 % 2;

        mode &= ~SPI_MODE_3;
	 mode |= SPI_MODE_1;

        printf("select spi%d mode: 0x%x tx:0xf0\n", arg0, mode);

        /*
         * spi mode //?竚spi??家Α
         */ 
        ret = ioctl(fm[arg0], SPI_IOC_WR_MODE, &mode);    //?家Α 
        if (ret == -1) 
            pabort("can't set spi mode"); 
 
        ret = ioctl(fm[arg0], SPI_IOC_RD_MODE, &mode);    //?家Α 
        if (ret == -1) 
            pabort("can't get spi mode"); 

        if (!arg1) {
            arg1 = 1;
        }
        
        printf("sel 15 spi[%d] ready to receive data, size: 61440 num: %d \n", arg0, arg1);
        ret = tx_data(fm[arg0], rx_buff[0], tx_buff[0], arg1, 61440, 1024*1024);
        printf("[%d]rx %d\n", arg0, ret);
        wtsz = fwrite(rx_buff[0], 1, ret, fp);
        printf("write file %d size %d/%d \n", fp, wtsz, ret);
        goto end;
    }
    if (sel == 14){ /* dual band data mode test ex[14 1 2 5 1 30720]*/
        #define PKTSZ  30720 //61440
        int chunksize;
        if (arg4 == PKTSZ) chunksize = PKTSZ;
        else chunksize = 61440;

        printf("***chunk size: %d ***\n", chunksize);
		
        int pipefd[2];
        int pipefs[2];
        int pipefc[2];
        char *tbuff, *tmp, buf='c', *dstBuff, *dstmp;
        char lastaddr[48];
        int sz = 0, wtsz = 0, lsz = 0;
        char *addr;
        int pid = 0;
        int txhldsz = 0;
        //tbuff = malloc(TSIZE);
        tbuff = rx_buff[0];
        dstBuff = mmap(NULL, TSIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	 memset(dstBuff, 0x95, TSIZE);
        dstmp = dstBuff;

	switch(arg3) {
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
        /*
         * spi mode 
         */ 
        ret = ioctl(fm[0], SPI_IOC_WR_MODE, &mode);    //?家Α 
        if (ret == -1) 
            pabort("can't set spi mode"); 
        
        ret = ioctl(fm[0], SPI_IOC_RD_MODE, &mode);    //?家Α 
        if (ret == -1) 
            pabort("can't get spi mode"); 

        /*
         * spi mode 
         */ 
        ret = ioctl(fm[1], SPI_IOC_WR_MODE, &mode);    //?家Α 
        if (ret == -1) 
            pabort("can't set spi mode"); 
        
        ret = ioctl(fm[1], SPI_IOC_RD_MODE, &mode);    //?家Α 
        if (ret == -1) 
            pabort("can't get spi mode"); 

        bitset = 1;
        ret = ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        printf("Set spi0 data mode: %d\n", bitset);

        bitset = 1;
        ret = ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        printf("Set spi1 data mode: %d\n", bitset);

        bitset = 1;
        ret = ioctl(fm[0], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_RD_CTL_PIN
        printf("Get spi1 RDY: %d\n", bitset);

        bitset = 1;
        ret = ioctl(fm[1], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_RD_CTL_PIN
        printf("Get spi1 RDY: %d\n", bitset);

        if (tbuff)
            printf("%d bytes memory alloc succeed! [0x%.8x]\n", TSIZE, tbuff);
        else 
            goto end;

        tmp = tbuff;
        
        sz = 0;
        pipe(pipefd);
        pipe(pipefs);
        pipe(pipefc);
        
        pid = fork();
        
        if (pid) {
            pid = fork();
            if (pid) {
		uint8_t *cbuff;
		cbuff = tx_buff[1];
                printf("main process to monitor the p0 and p1 \n");
                close(pipefd[0]); // close the read-end of the pipe, I'm not going to use it
                close(pipefs[1]); // close the write-end of the pipe, I'm not going to use it
                close(pipefd[1]); // close the write-end of the pipe, thus sending EOF to the reader
                close(pipefs[0]); // close the read-end of the pipe
                close(pipefc[1]); // close the write-end of the pipe, thus sending EOF to the reader
                
                txhldsz = arg1;
                
                while (1) {
                    //sleep(3);					
                    ret = read(pipefc[0], &buf, 1); 
                    //printf("********//read %d, buf:%c//********\n", ret, buf);
                    if ((buf == '0') || (buf == '1')) {
				cbuff[wtsz] = buf;
                     	wtsz++;

			if (arg0) {						
                        if (wtsz == txhldsz) {
                            //printf("********//spi slave send signal to hold the data transmitting(%d/%d)//********\n", wtsz, txhldsz);

                            bitset = 1;
                            ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 13, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
                            //printf("Set spi0 Tx Hold: %d\n", bitset);

                            bitset = 1;
                            ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 13, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
                            //printf("Set spi1 Tx Hold: %d\n", bitset);

				if (arg2>0)
					lsz = arg2;
				else
					lsz = 10;

				while(lsz) {
              	              bitset = 0;
       	                     ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
	                            //printf("Set RDY: %d, dly:%d \n", bitset, arg2);
					usleep(1);
                            
                     	       bitset = 1;
              	              ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
       	                     //printf("Set RDY: %d\n", bitset);
					usleep(1);
					
					lsz --;
				}

              	     //         bitset = 0;
       	             //        ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
	             //               printf("Set RDY: %d\n", bitset);
                     //       	sleep(5);

                            bitset = 0;
                            ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 13, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
                            //printf("Set spi0 Tx Hold: %d\n", bitset);

                            bitset = 0;
                            ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 13, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
                            //printf("Set spi1 Tx Hold: %d\n", bitset);

                        }   
			   }
                    } else if (buf == 'e') {
                        ret = read(pipefc[0], lastaddr, 32); 
                        sz = atoi(lastaddr);
                        printf("********//main monitor, write count:%d, sz:%d str:%s//********\n", wtsz, sz, lastaddr);
				for (sz = 0; sz < wtsz; sz++) {
					printf("%c, ", cbuff[sz]);
					if (!((sz+1) % 16)) printf("\n");
				}
                        break;
                    } else {
                        printf("********//main monitor, get buff:%c, ERROR!!!!//********\n", buf);
                    }
                    
                }
                close(pipefc[0]); // close the read-end of the pipe

            } else {
                printf("p0 process pid:%d!\n", pid);
                char str[256] = "/root/p0.log";
                
                close(pipefd[0]); // close the read-end of the pipe, I'm not going to use it
                close(pipefs[1]); // close the write-end of the pipe, I'm not going to use it
                close(pipefc[0]); // close the read-end of the pipe
                
                while(1) {
                    ret = tx_data(fm[0], dstBuff, tx_buff[0], 1, chunksize, 1024*1024);
                    //ret = tx_data(fm[0], dstBuff, NULL, 1, chunksize, 1024*1024);
			//if (arg0) 
                    		printf("[p0]rx %d - %d\n", ret, wtsz++);
                    msync(dstBuff, ret, MS_SYNC);
/*
if (((dstBuff - dstmp) < 0x28B9005) && ((dstBuff - dstmp) > 0x28B8005)) {
	char *ch;
	ch = dstBuff;;
	printf("0x%.8x: \n",(uint32_t)(ch - dstmp));
	for (sel = 0; sel < 1024; sel++) {
		printf("%.2x ", *ch);
		if (!((sel + 1) % 16)) printf("\n");
		ch++;
	}
}
*/
			dstBuff += ret + chunksize;
                    if (ret != chunksize) {
                        dstBuff -= chunksize;
                        if (ret == 1) ret = 0;
                        lsz = ret;
                        write(pipefd[1], "e", 1); // send the content of argv[1] to the reader
                        sprintf(lastaddr, "%d", dstBuff);
                        printf("p0 write e  addr:%x str:%s ret:%d\n", dstBuff, lastaddr, ret);
                        write(pipefd[1], lastaddr, 32); 
                        break;
                    }
                    write(pipefd[1], "c", 1);
                    
                    /* send info to control process */
                    write(pipefc[1], "0", 1);
                    //printf("main process write c \n");    
                }
                
                wtsz = 0;
                while (1) {
                    ret = read(pipefs[0], &buf, 1); 
                    //printf("p0 read %d, buf:%c \n", ret, buf);
                    if (buf == 'c')
                        wtsz++; 
                    else if (buf == 'e') {
                        ret = read(pipefs[0], lastaddr, 32); 
                        addr = (char *)atoi(lastaddr);
                        printf("p0 process wait wtsz:%d, lastaddr:%x/%x\n", wtsz, addr, dstBuff);
                        break;
                    }
                }
                
                if (dstBuff > addr) {
                    printf("p0 memcpy from  %x to %x sz:%d\n", dstBuff-lsz, addr, lsz);
                    msync(dstBuff-lsz, lsz, MS_SYNC);
                    #if 0
                    memcpy(addr, dstBuff-lsz, lsz);
                    #else
                    memcpy(tbuff, dstBuff-lsz, lsz);
                    memcpy(addr, tbuff, lsz);
                    #endif
                    addr += lsz;
                    memset(addr, 0, chunksize);
                    sz = addr - dstmp;
                    
                    /* send info to control process */
                    write(pipefc[1], "e", 1);
                    sprintf(lastaddr, "%d", sz);
                    printf("p0 write e, sz:%x str:%s \n", sz, lastaddr);
                    write(pipefc[1], lastaddr, 32);

                    msync(dstmp, sz, MS_SYNC);
                    ret = fwrite(dstmp, 1, sz, fp);
                    printf("\np0 write file [%s] size %d/%d \n", path, sz, ret);
                }
                
                close(pipefd[1]); // close the write-end of the pipe, thus sending EOF to the reader
                close(pipefs[0]); // close the read-end of the pipe
                close(pipefc[0]); // close the read-end of the pipe
            }
        } else {
            char str[256] = "/root/p1.log";
            printf("p1 process pid:%d!\n", pid);

            close(pipefd[1]); // close the write-end of the pipe, I'm not going to use it
            close(pipefs[0]); // close the read-end of the pipe, I'm not going to use it
            close(pipefc[0]); // close the read-end of the pipe

            dstBuff += chunksize;
            while(1) {
                ret = tx_data(fm[1], dstBuff, tx_buff[0], 1, chunksize, 1024*1024);
                //ret = tx_data(fm[1], dstBuff, NULL, 1, chunksize, 1024*1024);
		//if (arg0)		
	               printf("[p1]rx %d - %d\n", ret, wtsz++);
                msync(dstBuff, ret, MS_SYNC);
/*
if (((dstBuff - dstmp) < 0x28B9005) && ((dstBuff - dstmp) > 0x28B8005)) {
	char *ch;
	ch = dstBuff;;
	printf("0x%.8x: \n", (uint32_t)(ch - dstmp));
	for (sel = 0; sel < 1024; sel++) {
		printf("%.2x ", *ch);
		if (!((sel + 1) % 16)) printf("\n");
		ch++;
	}
}
*/
                dstBuff += ret + chunksize;
                if (ret != chunksize) {
                    dstBuff -= chunksize;
                    if (ret == 1) ret = 0;
                    lsz = ret;
                    write(pipefs[1], "e", 1); // send the content of argv[1] to the reader
                    sprintf(lastaddr, "%d", dstBuff);
                    printf("p1 write e addr:%x str:%s ret:%d\n", dstBuff, lastaddr, ret);
                    write(pipefs[1], lastaddr, 32); // send the content of argv[1] to the reader
                    break;
                }
                write(pipefs[1], "c", 1); // send the content of argv[1] to the reader         
                
                /* send info to control process */
                write(pipefc[1], "1", 1);
                //printf("p1 process write c \n");
            }

            wtsz = 0;
            while (1) {
                ret = read(pipefd[0], &buf, 1); 
                //printf("p1 read %d, buf:%c  \n", ret, buf);
                if (buf == 'c')
                    wtsz++; 
                else if (buf == 'e') {
                    ret = read(pipefd[0], lastaddr, 32); 
                    addr = (char *)atoi(lastaddr);
                    printf("p1 process wtsz:%d, lastaddr:%x/%x\n",  wtsz, addr, dstBuff);
                    break;
                }
            }

            if (dstBuff > addr) {
                printf("p1 memcpy from  %x to %x sz:%d\n", dstBuff-lsz, addr, lsz);
                msync(dstBuff-lsz, lsz, MS_SYNC);
                #if 0
                memcpy(addr, dstBuff-lsz, lsz);
                #else
                memcpy(tbuff, dstBuff-lsz, lsz);
                memcpy(addr, tbuff, lsz);
                #endif
                addr += lsz;
                memset(addr, 0, chunksize);
                sz = addr - dstmp;
                
                /* send info to control process */
                write(pipefc[1], "e", 1);
                sprintf(lastaddr, "%d", sz);
                printf("p1 write e sz:%x str:%s \n", sz, lastaddr);
                write(pipefc[1], lastaddr, 32); 

                msync(dstmp, sz, MS_SYNC);
                ret = fwrite(dstmp, 1, sz, fp);
                printf("\np1 write file [%s] size %d/%d \n", path, sz, ret);
            }

            close(pipefd[0]); // close the read-end of the pipe
            close(pipefs[1]); // close the write-end    of the pipe, thus sending EOF to the
            close(pipefc[0]); // close the read-end of the pipe
        }

        munmap(dstmp, TSIZE);
        goto end;
    }

    if (sel == 13){ /* data mode test ex[13 1024 100 0]*/
	arg2 = arg2 % 2;
        data_process(rx_buff[0], tx_buff[0], fp, fm[arg2], arg0, arg1);
        goto end;
    }               

    if (sel == 12){ /* data mode test */
        int pid;
        pid = fork();
        if (pid) {
            printf("p1 %d created!\n", pid);
            pid = fork();
            if (pid)
                printf("p2 %d created!\n", pid);
            else
                while(1);
        } else {
            while(1);
        }

        goto end;
    }       
    if (sel == 11){ /* cmd mode test */
        tx[0] = 0xaa;
        tx[1] = 0x55;
        ret = tx_command(fd0, rx, tx, 512);
        printf("reply 0x%x 0x%x \n", tx[0], tx[1]);
        ret = chk_reply(rx, rxans, 512);
        printf("check receive data ret: %d\n", ret);
        while (1) {
                bitset = 1;
                ret = ioctl(fd1, _IOR(SPI_IOC_MAGIC, 7, __u32), &bitset);   //SPI_IOC_RD_CS_PIN
             printf("Get fd1 CS: %d\n", bitset);
             if (bitset == 0) break;
             sleep(3);
        }
        goto end;
    }
    if (sel == 2) { /*set RDY pin*/
        bitset = arg0;
        ret = ioctl(fm[arg1], _IOW(SPI_IOC_MAGIC, 7, __u32), &bitset);  //SPI_IOC_WR_CS_PIN
        printf("Set CS: %d\n", bitset);
        goto end;
    }

    if (sel == 3) { /*set RDY pin*/
        bitset = arg0;
        ret = ioctl(fm[arg1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        printf("Set RDY: %d\n", arg0);
        goto end;
    }
    if (sel == 4) { /* get RDY pin */
        bitset = 0;
        ret = ioctl(fm[arg1], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);  //SPI_IOC_RD_CTL_PIN
        printf("Get RDY: %d\n", bitset);
        goto end;
    }
    if (sel == 5) { /* get CS pin */
        bitset = 0;
        ret = ioctl(fm[arg1], _IOR(SPI_IOC_MAGIC, 7, __u32), &bitset);  //SPI_IOC_RD_CS_PIN
        printf("Get CS: %d\n", bitset);
        goto end;
    }
    if (sel == 6){/* set data mode test */
        bitset = arg0;
        ret = ioctl(fm[arg1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        printf("Set data mode: %d\n", arg0);
        goto end;
    }
    if (sel == 7) {/* get data mode test */
        bitset = 0;
        ret = ioctl(fm[arg1], _IOR(SPI_IOC_MAGIC, 8, __u32), &bitset);  //SPI_IOC_RD_DATA_MODE
        printf("Get data mode: %d\n", bitset);
        goto end;
    }
    if (sel == 8){ /* set slve ready */
        bitset = arg0;
        ret = ioctl(fm[arg1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
        printf("Set slve ready: %d\n", arg0);
        goto end;
    }
    if (sel == 9){ /* get slve ready */
        bitset = 0;
        ret = ioctl(fm[arg1], _IOR(SPI_IOC_MAGIC, 11, __u32), &bitset); //SPI_IOC_RD_SLVE_READY
        printf("Get slve ready: %d\n", bitset);
        goto end;
    }
    if (sel == 10){
        fp = find_save(path, data_save);
        if (!fp)
            printf("find save dst failed ret:%d\n", fp);
        else
            printf("find save dst succeed ret:%d\n", fp);
        goto end;
    }
    if (sel == 12){
        return;
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

    free(tx_buff[0]);
    free(rx_buff[0]);
    free(tx_buff[1]);
    free(rx_buff[1]);

    fclose(fp);
}

