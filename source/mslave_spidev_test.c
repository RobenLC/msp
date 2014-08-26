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
    
static void pabort(const char *s) 
{ 
    perror(s); 
    abort(); 
} 
 
static const char *device = "/dev/spidev32765.0"; 
static const char *data_path = "/root/rx/1-1.jpg"; 
static uint8_t mode; 
static uint8_t bits = 8; 
static uint32_t speed = 1000000; 
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
  if (ret < 1)
      pabort("can't send spi message");

  printf("tx/rx len: %d\n", ret);

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
        case 'D':   //??名 
                printf(" -D %s \n", optarg);
            device = optarg; 
            break; 
        case 's':   //速率 
              printf(" -s %s \n", optarg);
            speed = atoi(optarg); 
            break; 
        case 'd':   //延??? 
            delay = atoi(optarg); 
            break; 
        case 'b':   //每字含多少位 
            bits = atoi(optarg); 
            break; 
        case 'l':   //回送模式 
            mode |= SPI_LOOP; 
            break; 
        case 'H':   //??相位 
            mode |= SPI_CPHA; 
            break; 
        case 'O':   //??极性 
            mode |= SPI_CPOL; 
            break; 
        case 'L':   //lsb 最低有效位 
            mode |= SPI_LSB_FIRST; 
            break; 
        case 'C':   //片?高?平 
            mode |= SPI_CS_HIGH; 
            break; 
        case '3':
            mode |= SPI_3WIRE; 
            break; 
        case 'N':   //?片? 
            mode |= SPI_NO_CS; 
            break; 
        case 'R':   //?机拉低?平停止?据?? 
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
        default:    //??的?? 
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
			printf("open file [%s] failed \n", dst);
			break;
		} else
			printf("open file [%s] succeed \n", dst);
	}
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
static char data_save[] = "/root/rx/%d.jpg"; 
static char path[256];

    uint32_t bitset;
    int sel, arg0, arg1 = 0;
    int fd, ret; 
    int buffsize;
    uint8_t *tx_buff[2], *rx_buff[2];
    FILE *fp;
    
    fd = open(device, O_RDWR);  //打???文件 
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
	
    buffsize = 5*1024*1024;
    tx_buff[0] = malloc(buffsize);
    if (tx_buff[0]) {
        printf(" tx buff 0 alloc success!!\n");
    }
	memset(tx_buff[0], 0, buffsize);
    rx_buff[0] = malloc(buffsize);
    if (rx_buff[0]) {
        printf(" rx buff 0 alloc success!!\n");
    }
	memset(rx_buff[0], 0, buffsize);
    tx_buff[1] = malloc(buffsize);
    if (tx_buff[1]) {
        printf(" tx buff 1 alloc success!!\n");
    }
	memset(tx_buff[1], 0, buffsize);
    rx_buff[1] = malloc(buffsize);
    if (rx_buff[1]) {
        printf(" rx buff 1 alloc success!!\n");
    }
	memset(rx_buff[1], 0, buffsize);
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

	fp = find_save(path, data_save);
	if (!fp)
		printf("find save dst failed ret:%d\n", fp);
	else
		printf("find save dst succeed ret:%d\n", fp);

        int fm[2] = {fd0, fd1};
		
	char rxans[512];
	char tx[512];
	char rx[512];
	int i;
	for (i = 0; i < 512; i++) {
		rxans[i] = i & 0x95;
		tx[i] = i & 0x95;
	}

    	if (sel == 14){ /* dual band data mode test ex[14]*/
		#define TSIZE (32*1024*1024)
		#define PKTSZ  61440
		int pipefd[2];
		int pipefs[2];
		char *tbuff, *tmp, buf='c', *dstBuff, *dstmp;
		char lastaddr[48];
		int sz, wtsz, lsz;
		char *addr;
		int pid;
		//tbuff = malloc(TSIZE);
		tbuff = rx_buff[0];
		dstBuff = mmap(NULL, TSIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
		dstmp = dstBuff;

		if (tbuff)
			printf("%d bytes memory alloc succeed! [0x%.8x]\n", TSIZE, tbuff);
		else 
			goto end;

		tmp = tbuff;
		
		sz = 0;
		pipe(pipefd);
		pipe(pipefs);

		pid = fork();
		
		if (pid) {
			printf("main process !\n", pid);
			sleep(3);
			char str[256] = "/root/p0.log";
			
			close(pipefd[0]); // close the read-end of the pipe, I'm not going to use it
			close(pipefs[1]); // close the write-end of the pipe, I'm not going to use it
			   
			while(1) {
				ret = tx_data(fm[0], dstBuff, tx_buff[0], 1, PKTSZ, 1024*1024);
				//printf("[p0]%d rx %d\n", fd0, ret);
				msync(dstBuff, ret, MS_SYNC);
				dstBuff += ret + PKTSZ;
				if (ret != PKTSZ) {
					dstBuff -= PKTSZ;
					if (ret == 1) ret = 0;
					lsz = ret;
					write(pipefd[1], "e", 1); // send the content of argv[1] to the reader
					sprintf(lastaddr, "%d", dstBuff);
					printf("write e addr:%x str:%s \n", dstBuff, lastaddr);
					write(pipefd[1], lastaddr, 32); 
					break;
				}
				write(pipefd[1], "c", 1); // send the content of argv[1] to the reader
				//printf("main process write c \n");	
			}

			wtsz = 0;
			while (1) {
				ret = read(pipefs[0], &buf, 1); 
				printf("read %d, buf:%c \n", ret, buf);
				if (buf == 'c')
					wtsz++;	
				else if (buf == 'e') {
					ret = read(pipefs[0], lastaddr, 32); 
					addr = (char *)atoi(lastaddr);
					printf("main process wait wtsz:%d, lastaddr:%x/%x\n", wtsz, addr, dstBuff);
					break;
				}
			}

			if (dstBuff > addr) {
				printf("memcpy from %x to %x sz:%d\n", dstBuff-lsz, addr, lsz);
				msync(dstBuff-lsz, lsz, MS_SYNC);
				#if 0
				memcpy(addr, dstBuff-lsz, lsz);
				#else
				memcpy(tbuff, dstBuff-lsz, lsz);
				memcpy(addr, tbuff, lsz);
				#endif
				addr += lsz;
				memset(addr, 0, PKTSZ);
				sz = addr - dstmp;
				msync(dstmp, sz, MS_SYNC);
				ret = fwrite(dstmp, 1, sz, fp);
				printf("write file size %d/%d \n", sz, ret);
			}
			
			close(pipefd[1]); // close the write-end of the pipe, thus sending EOF to the reader
           		close(pipefs[0]); // close the read-end of the pipe

		} else {
			char str[256] = "/root/p1.log";
			printf("p1 process created!\n");
			sleep(3);

			close(pipefd[1]); // close the write-end of the pipe, I'm not going to use it
			close(pipefs[0]); // close the read-end of the pipe, I'm not going to use it
			
			dstBuff += PKTSZ;
			while(1) {
				ret = tx_data(fm[1], dstBuff, tx_buff[0], 1, PKTSZ, 1024*1024);
				//printf("[p1]%d rx %d\n", fd1, ret);
				msync(dstBuff, ret, MS_SYNC);
				dstBuff += ret + PKTSZ;
				if (ret != PKTSZ) {
					dstBuff -= PKTSZ;
					if (ret == 1) ret = 0;
					lsz = ret;
			       	write(pipefs[1], "e", 1); // send the content of argv[1] to the reader
					sprintf(lastaddr, "%d", dstBuff);
					printf("write e addr:%x str:%s \n", dstBuff, lastaddr);
					write(pipefs[1], lastaddr, 32); // send the content of argv[1] to the reader
					break;
				}
				write(pipefs[1], "c", 1); // send the content of argv[1] to the reader
				//printf("p1 process write c \n");
			}

			wtsz = 0;
			while (1) {
				ret = read(pipefd[0], &buf, 1); 
				printf("read %d, buf:%c \n", ret, buf);
				if (buf == 'c')
					wtsz++;	
				else if (buf == 'e') {
					ret = read(pipefd[0], lastaddr, 32); 
					addr = (char *)atoi(lastaddr);
					printf("p1 process wait wtsz:%d, lastaddr:%x/%x\n", wtsz, addr, dstBuff);
					break;
				}
			}

			if (dstBuff > addr) {
				printf("memcpy from %x to %x sz:%d\n", dstBuff-lsz, addr, lsz);
				msync(dstBuff-lsz, lsz, MS_SYNC);
				#if 0
				memcpy(addr, dstBuff-lsz, lsz);
				#else
				memcpy(tbuff, dstBuff-lsz, lsz);
				memcpy(addr, tbuff, lsz);
				#endif
				addr += lsz;
				memset(addr, 0, PKTSZ);
				sz = addr - dstmp;
				msync(dstmp, sz, MS_SYNC);
				ret = fwrite(dstmp, 1, sz, fp);
				printf("write file size %d/%d \n", sz, ret);
			}

           		close(pipefd[0]); // close the read-end of the pipe
		       close(pipefs[1]); // close the write-end of the pipe, thus sending EOF to the
	
		}

		munmap(dstmp, TSIZE);
		goto end;
	}			
    	if (sel == 13){ /* data mode test ex[13 1024 100]*/
		data_process(rx_buff[0], tx_buff[0], fp, fm[1], arg0, arg1);
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
	        ret = ioctl(fd1, _IOR(SPI_IOC_MAGIC, 7, __u32), &bitset);	//SPI_IOC_RD_CS_PIN
       	 printf("Get fd1 CS: %d\n", bitset);
		 if (bitset == 0) break;
		 sleep(3);
	}
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
        ret = ioctl(fm[arg1], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);	//SPI_IOC_RD_CTL_PIN
        printf("Get RDY: %d\n", bitset);
	goto end;
    }
    if (sel == 5) { /* get CS pin */
        bitset = 0;
        ret = ioctl(fm[arg1], _IOR(SPI_IOC_MAGIC, 7, __u32), &bitset);	//SPI_IOC_RD_CS_PIN
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
        ret = ioctl(fm[arg1], _IOR(SPI_IOC_MAGIC, 8, __u32), &bitset);	//SPI_IOC_RD_DATA_MODE
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
        ret = ioctl(fm[arg1], _IOR(SPI_IOC_MAGIC, 11, __u32), &bitset);	//SPI_IOC_RD_SLVE_READY
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

