#include <stdint.h> 
#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <getopt.h> 
#include <fcntl.h> 
#include <sys/ioctl.h> 
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
static void tx_command(
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
}

static void tx_data(int fd, uint8_t *rx_buff, uint8_t *tx_buff, int ex_size, int num)
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
            data_path = optarg;
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
    uint32_t bitset;
    int sel;
    int fd, ret; 
    int buffsize;
    uint8_t *tx_buff, *rx_buff;
    
    fd = open(device, O_RDWR);  //ゴ???ゅン 
    if (fd < 0) 
        pabort("can't open device"); 
    
    if (argc > 1) {
        printf(" [1]:%s \n", argv[1]);
        sel = atoi(argv[1]);
    }

    buffsize = 1*1024*1024;
    tx_buff = malloc(buffsize);
    if (tx_buff) {
        printf(" tx buff alloc success!!\n");
    }
    rx_buff = malloc(buffsize);
    if (rx_buff) {
        printf(" rx buff alloc success!!\n");
    }
    
    if (sel == 3) {
        ret = ioctl(fd, _IOW(SPI_IOC_MAGIC, 7, __u32), &bits);   //SPI_IOC_REL_CTL_PIN
        if (ret == -1) 
            pabort("can't SPI_IOC_REL_CTL_PIN"); 
        return;
    }
    if (sel == 4) {
        ret = ioctl(fd, _IOR(SPI_IOC_MAGIC, 11, __u32), &bits);	//SPI_IOC_RD_SLVE_READY _IOR(SPI_IOC_MAGIC, 11, __u32)
        if (ret == -1) 
            pabort("can't SPI_IOC_RD_SLVE_READY"); 
	printf("rd slve rdy %d\n",bits);
        return;
    }
    if (sel == 5) {
	bits = 1;
        ret = ioctl(fd, _IOR(SPI_IOC_MAGIC, 11, __u32), &bits);	//SPI_IOC_RD_SLVE_READY _IOR(SPI_IOC_MAGIC, 11, __u32)
	if (bits == 1) bits = 0;
	else bits = 1;
        ret = ioctl(fd, _IOW(SPI_IOC_MAGIC, 11, __u32), &bits);	//SPI_IOC_WD_SLVE_READY _IOW(SPI_IOC_MAGIC, 11, __u32)
        if (ret == -1) 
            pabort("can't SPI_IOC_WD_SLVE_READY"); 
	printf("WD slve rdy %d\n",bits);
        return;
    }
    if (sel == 6){/* 6 slave rx cmd */
	uint8_t *p8;
	uint32_t slv = 0;
	uint32_t pin = 1;
	uint32_t rdy = 1;
	uint32_t dat = 1;
	bitset = 1;
	ret = ioctl(fd, _IOW(SPI_IOC_MAGIC, 8, __u32), &dat);   //SPI_IOC_WR_DATA_MODE
	if (ret) printf("set data mode error\n");
	else printf("set data mode: %d\n", dat);
	ret = ioctl(fd, _IOR(SPI_IOC_MAGIC, 10, __u32), &slv);	//SPI_IOC_RD_SLVE_MODE
	printf("slv: %d \n", slv);
	printf("rx one cmd \n");
	ret = ioctl(fd, _IOW(SPI_IOC_MAGIC, 6, __u32), &pin);	//SPI_IOC_WD_CTL_PIN
	if (ret) printf("set ctl pin error\n");
	else printf("set ctl pin: %d\n", pin);
        ret = ioctl(fd, _IOW(SPI_IOC_MAGIC, 11, __u32), &rdy);	//SPI_IOC_WD_SLVE_READY _IOW(SPI_IOC_MAGIC, 11, __u32)
	if (ret) printf("set slve rdy error\n");
	else printf("set slve rdy: %d\n", rdy);
	tx_command(fd, rx_buff, tx_buff, 2);
	p8 = rx_buff;
	printf("rx 0x%x 0x%x \n", p8[0], p8[1]);
	return;
    }
    if (sel == 7) {/* 7 master tx cmd */
	uint8_t *p8;
	uint32_t pin = 0;
	ret = ioctl(fd, _IOR(SPI_IOC_MAGIC, 6, __u32), &pin);	//SPI_IOC_RD_CTL_PIN
	if (ret) 
        	pabort("can't SPI_IOC_RD_SLVE_READY"); 
	printf("slve ctl pin: %d\n",pin);
	pin = 1;
	ret = ioctl(fd, _IOW(SPI_IOC_MAGIC, 6, __u32), &pin);	//SPI_IOC_WD_CTL_PIN
	if (ret) printf("set ctl pin error\n");
	else printf("set ctl pin: %d\n", pin);
	tx_buff[0] = 0xa8;
	tx_buff[1] = 0x52;
	printf("tx cmd 0x%x 0x%x \n", tx_buff[0], tx_buff[1]);
	tx_command(fd, rx_buff, tx_buff, 2);
	p8 = rx_buff;
	printf("rx 0x%x 0x%x \n", p8[0], p8[1]);
	
        ret = ioctl(fd, _IOR(SPI_IOC_MAGIC, 6, __u32), &pin);	//SPI_IOC_RD_CTL_PIN
        if (ret) 
            pabort("can't SPI_IOC_RD_SLVE_READY"); 
	printf("slve ctl pin: %d\n",pin);

	return;
    }
    if (sel == 8){
	uint8_t *p8;
	uint32_t pin = 0;
	ret = ioctl(fd, _IOW(SPI_IOC_MAGIC, 6, __u32), &pin);	//SPI_IOC_WD_CTL_PIN
	if (ret) printf("set ctl pin error\n");
	else printf("set ctl pin: %d\n", pin);

	tx_buff[0] = 0xaa;
	tx_buff[1] = 0xff;
	printf("tx cmd 0x%x 0x%x \n", tx_buff[0], tx_buff[1]);
	tx_command(fd, rx_buff, tx_buff, 2);
	p8 = rx_buff;
	printf("rx 0x%x 0x%x \n", p8[0], p8[1]);
	return;
    }
    if (sel == 9){
	return;
    }
    if (sel == 10){
	return;
    }
    if (sel == 11){
	return;
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
}
#else
int main(int argc, char *argv[]) 
{ 
    int ret = 0; 
    int fd; 
      uint8_t cmd_tx[16] = {0x53, 0x80,};
      uint8_t cmd_rx[16] = {0,};
      int cmd_size = 2;
      uint8_t *tx_buff, *rx_buff;
      FILE *fpd;
      int fsize, buffsize;
        
    uint32_t getbit;

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

    uint32_t bitset;
    bitset = 0;
    ioctl(fd, _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WD_CTL_PIN
    
    /* spi work sequence start here */
    uint32_t temp32;
    uint32_t stage;
    stage = 1;
    printf(" \n*****[%d]*****\n", stage++);
    /* 1. pull down ctl_pin to notice master */
    bitset = 1;
    ioctl(fd, _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WD_CTL_PIN
    
    printf(" \n*****[%d]*****\n", stage++);
    /* 2. reply request */

    uint32_t getcmd;
    int cntdown;
    getcmd = 0;
    temp32 = 1;
    cmd_tx[0] = 0;
    cmd_tx[1] = 0;
    while (1) {
        tx_command(fd, cmd_rx, cmd_tx, 2);
        getcmd = cmd_rx[0] | (cmd_rx[1] << 8);
        
        if (getcmd == 0x0729) {
            ioctl(fd, _IOW(SPI_IOC_MAGIC, 7, __u32), &bits);   //SPI_IOC_REL_CTL_PIN
            usleep(10);
            cntdown = 10000;
            getbit = 2;
            while(cntdown) {
                ioctl(fd, _IOR(SPI_IOC_MAGIC, 6, __u32), &getbit);   //SPI_IOC_RD_CTL_PIN
                if (getbit == 0)
                    break;
                cntdown--;
                usleep(1);
            }
            
            while(cntdown) {
                ioctl(fd, _IOR(SPI_IOC_MAGIC, 6, __u32), &getbit);   //SPI_IOC_RD_CTL_PIN    
                if (getbit == 1)
                    break;
                cntdown --;
                usleep(1);
            }
            if (cntdown)
                break;
        }
        
        command = getcmd;               
        cmd_tx[0] = (command >> 4) & 0xff;
        cmd_tx[1] = ((command >> 12) & 0x0f) | ((command << 4) & 0xf0);
        printf(" tx:%x %x \n", cmd_tx[1], cmd_tx[0]);
        
        if (temp32 != getcmd) {
            printf(" get status:0x%x\n", getcmd);
            temp32 = getcmd;
        }
        printf(" get status:0x%x\n", getcmd);
    }
    
    printf(" \n*****[%d]*****\n", stage++);
    /* 4. enable data mode and pull low to wait for data mode */
    bitset = 1;
    ioctl(fd, _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    bitset = 0;
    ioctl(fd, _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WD_CTL_PIN
    
    printf(" \n*****[%d]*****\n", stage++);
    /* 5. prepare the buffer for data tx mode */
    /* prepare transmitting */
    /* Tx/Rx buffer alloc */       
    buffsize = 1*1024*1024;
    tx_buff = malloc(buffsize);
    if (tx_buff) {
        printf(" tx buff alloc success!!\n");
    }
    rx_buff = malloc(buffsize);
    if (rx_buff) {
        printf(" rx buff alloc success!!\n");
    }
    
    printf(" \n*****[%d]*****\n", stage++);
    /* 7. do the transmiting */
    #define P_SIZE (1024 * 289)
    int count = 0;
    int usize = 0;
    uint8_t *ptr;
    ptr = rx_buff;
    
    getbit = 1;
    temp32 = 2;
    while (usize < 883882) {
        count++;
        tx_data(fd, ptr, tx_buff, P_SIZE, 2);     
        usize += P_SIZE;
        ptr += P_SIZE;
        printf(" %d r:%d \n", count, usize);
    }
    
    bitset = 0;
    ioctl(fd, _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    bitset = 1;
    ioctl(fd, _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WD_CTL_PIN
    
    printf(" \n*****[%d]*****\n", stage++);
    /* 8. save the rx_buff into FILE */
    /* open target file which will be transmitted */
    fsize = usize;
    printf(" open file %s \n", data_path);
    fpd = fopen(data_path, "w");
       
    if (!fpd) {
        printf(" %s file open failed \n", data_path);
        return ret;
    }
    printf(" %s file open succeed \n", data_path);
    /* write Rx buffer into file */
    fwrite(rx_buff, 1, fsize, fpd);
    printf(" [%s] size: %d \n", data_path, fsize);

    printf(" \n*****[%d]*****\n", stage++);
    /* 9. check status for return of command mode */
    getcmd = 0;
    temp32 = 1;
    cmd_tx[0] = 0;
    cmd_tx[1] = 0;
    while (1) {
        tx_command(fd, cmd_rx, cmd_tx, 2);
        getcmd = cmd_rx[0] | (cmd_rx[1] << 8);
        
        getbit = 2;
        ioctl(fd, _IOR(SPI_IOC_MAGIC, 6, __u32), &getbit);   //SPI_IOC_RD_CTL_PIN
        if (getbit == 0) {
            cntdown = 1000;
            while(cntdown) {
                getbit = 2;
                ioctl(fd, _IOR(SPI_IOC_MAGIC, 6, __u32), &getbit);   //SPI_IOC_RD_CTL_PIN    
                if (getbit == 1)
                    break;
                cntdown --;
                usleep(10);
            }
            if (cntdown)
                break;
        }
        
        command = getcmd;               
        cmd_tx[0] = (command >> 4) & 0xff;
        cmd_tx[1] = ((command >> 12) & 0x0f) | ((command << 4) & 0xf0);
        printf(" tx:%x %x \n", cmd_tx[1], cmd_tx[0]);
        
        if (temp32 != getcmd) {
            printf(" get status:0x%x\n", getcmd);
            temp32 = getcmd;
        }
    }

    printf(" \n*****[%d]*****\n", stage++);
    
    fclose(fpd);
    free(tx_buff);
    free(rx_buff);
         
    close(fd);  //???? 
 
    return ret; 
} 
#endif
