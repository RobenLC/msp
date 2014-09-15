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

static void pabort(const char *s) 
{ 
    perror(s); 
    abort(); 
} 
 
static const char *device = "/dev/spidev32765.0"; 
char data_path[256] = "/root/tx/1.jpg"; 
static uint8_t mode; 
static uint8_t bits = 8; 
static uint32_t speed = 20000000; 
static uint16_t delay; 
static uint16_t command = 0; 
static uint8_t loop = 0; 
 
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
static char spi1[] = "/dev/spidev32765.0"; 
static char spi0[] = "/dev/spidev32765.1"; 
    uint32_t bitset;
    int sel, arg0, arg1, arg2;
    int fd, ret; 
    arg0 = 0;
	arg1 = 0;
	arg2 = 0;

	/* scanner default setting */
	mode |= SPI_CPHA;     
 
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
	
        uint8_t *tx_buff, *rx_buff;
        FILE *fpd;
        int fsize, buffsize;
        buffsize = 16*1024*1024;
        tx_buff = malloc(buffsize);
        if (tx_buff) {
            printf(" tx buff alloc success!!\n");
        }
        rx_buff = malloc(buffsize);
        if (rx_buff) {
            printf(" rx buff alloc success!!\n");
        }

	if (((sel == 9) ||(sel == 11)) && (argc > 3))
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
    if (sel == 12) { /* command mode */
	int pid = 0;
	pid = fork();
	printf("pid: %d \n", pid);
	
	goto end;

	}
    if (sel == 11) { /* dual channel data mode ex:[11 50 file_path 61440]*/
	#define TSIZE (128*1024*1024)
	#define PKTSZ  61440

	if (arg0)
		speed = arg0 * 1000000;
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
	
        bitset = 0;
        ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL

        bitset = 1;
        ioctl(fm[1], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL

        int pksize = 1024;
        int pknum = 60;
        int trunksz, remainsz, pkcnt;
	char *srcBuff, *srctmp;
	char * tbuff;
	int pid;
	if ((arg2) && (arg2 <= PKTSZ) && !(arg2%pksize) && !(PKTSZ%arg2)) {
		pksize = arg2;
	}
	pknum = PKTSZ / pksize;
	printf("pksize:%d pknum:%d \n", pksize, pknum);
	struct tms time;
	struct timespec curtime;
	unsigned long long cur, tnow, lnow, past, tbef, lpast;
	
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
        printf("Set RDY pin: %d\n", bitset);

	pid = fork();
	printf("pid: %d \n", pid);

	if (pid) {
		clock_gettime(CLOCK_REALTIME, &curtime);
		cur = curtime.tv_sec;
		tnow = curtime.tv_nsec;
		lnow = cur * 1000000000+tnow;
		printf("[p0] enter %d t:%llu %llu %llu\n", remainsz,cur,tnow, lnow/1000000);
		while (1) {
	
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
			srcBuff += ret + PKTSZ;

			pkcnt++;

			if (remainsz <= 0) break;
		}

	}else {
		remainsz -= trunksz;
		
		clock_gettime(CLOCK_REALTIME, &curtime);
		cur = curtime.tv_sec;
		tnow = curtime.tv_nsec;
		lnow = cur * 1000000000+tnow;
		printf("[p1] enter %d t:%llu %llu %llu\n", remainsz,cur,tnow, lnow/1000000);
		

		
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

			ret = tx_data(fm[1], NULL, srcBuff, pknum, pksize, 1024*1024);
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
			srcBuff += ret + PKTSZ;
		}

	}


	clock_gettime(CLOCK_REALTIME, &curtime);
	past = curtime.tv_sec;
	tbef = curtime.tv_nsec;		
	lpast = past * 1000000000+tbef;	

	printf("time cose: %llu s, bandwidth: %llu Mbits/s \n", (lpast - lnow)/1000000000, ((fsize*8)/((lpast - lnow)/1000000000)) /1000000 );
	
	sleep(3);

        bitset = 1;
        ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        printf("[%d]Set RDY pin: %d cnt:%d\n", pid, bitset,pkcnt);

	sleep(1);
	
        bitset = 0;
        ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        printf("[%d]Set RDY pin: %d cnt:%d\n", pid, bitset, pkcnt);

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
		
    if (sel == 9) { /* tx data use dual band [9 100 filename] */

        int pksize = 1024;
        int pknum = arg0;
        int trunksz, remainsz, pkcnt;
	char * tbuff;
        trunksz = pknum * pksize;
        remainsz = fsize;
        pkcnt = 0;
	tbuff = tx_buff;

        bitset = 1;
        ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        printf("Set RDY pin: %d\n", bitset);

        while (1) {
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
			remainsz -= trunksz;
		}
	        ret = tx_data(fm[pkcnt%2], NULL, tbuff, pknum, pksize, 1024*1024);
       	 //printf("tx ret:%d fd%d\n", ret, pkcnt%2);

		tbuff += pknum * pksize;
		
		 pkcnt++;
		 if (!remainsz) break;
        }

	sleep(3);

        bitset = 1;
        ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        printf("Set RDY pin: %d\n", bitset);

        bitset = 0;
        ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
        printf("Set RDY pin: %d\n", bitset);

        goto end;
    }
	
    if (sel == 8) { /* tx data */
        ret = tx_data(fm[arg2], rx_buff, tx_buff, arg0, arg1, 1024*1024);
        printf("tx len: %d num: %d fd%d pksz:%d\n", ret, arg0, arg2, arg1);
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
    
    if (sel == 7) { /* open device node */
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
