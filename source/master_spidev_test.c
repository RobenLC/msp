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
static const char *data_path = "/root/tx/rx.jpg"; 
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

static void tx_data(int fd, uint8_t *rx_buff, uint8_t *tx_buff, int ex_size)
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
            data_path = optarg;
            break;
        default:    //???? 
            print_usage(argv[0]); 
            break; 
        } 
    } 
} 
#if 0
int main(int argc, char *argv[]) 
{ 
    uint32_t bitset;
    int sel;
    int fd, ret; 
    
    fd = open(device, O_RDWR);  //ゴ???ゅン 
    if (fd < 0) 
        pabort("can't open device"); 
    
    if (argc > 1) {
        printf(" [1]:%s \n", argv[1]);
        sel = atoi(argv[1]);
    }
 
    if (sel == 3) {
        ret = ioctl(fd, _IOW(SPI_IOC_MAGIC, 7, __u32), &bits);   //SPI_IOC_REL_CTL_PIN
        if (ret == -1) 
            pabort("can't SPI_IOC_REL_CTL_PIN"); 
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
    
    uint32_t temp32;
    uint32_t stage;
    stage = 1;
    printf(" \n*****[%d]*****\n", stage++);
    /* 1. check control_pin ready or not */
    uint32_t getbit;
    getbit = 1;
    temp32 = 2;
    while (getbit == 1) {
        ret = ioctl(fd, _IOR(SPI_IOC_MAGIC, 6, __u32), &getbit);   //SPI_IOC_RD_CTL_PIN
        if (ret == -1) 
            pabort("can't SPI_IOC_RD_CTL_PIN"); 
        if (temp32 != getbit) {
            printf("wait for slave power up, ctl_pin = %d \n", getbit);
            temp32 = getbit;
        }
    }
    printf(" \n*****[%d]*****\n", stage++);
    /* 2. send request status command and check */
    command = 0x5380;
    if (command) {
        cmd_tx[0] = command & 0xff;
        cmd_tx[1] = (command >> 8) & 0xff;
        printf(" tx:%x %x \n", cmd_tx[1], cmd_tx[0]);
    }
    
    uint32_t getcmd;
    getcmd = 0;
    temp32 = 1;
    while (getcmd != 0x5380) {
        usleep(1000);
        tx_command(fd, cmd_rx, cmd_tx, cmd_size);
        getcmd = cmd_rx[0] | (cmd_rx[1] << 8);
        if (temp32 != getcmd) {
            printf(" get status:0x%x\n", getcmd);
            temp32 = getcmd;
        }
    }

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
       
    /* open target file which will be transmitted */
    printf(" open file %s \n", data_path);
    fpd = fopen(data_path, "r");
       
    if (!fpd) {
        printf(" %s file open failed \n", data_path);
        return ret;
    }
    printf(" %s file open succeed \n", data_path);
    /* read the file into Tx buffer */
    fsize = fread(tx_buff, 1, buffsize, fpd);
    printf(" [%s] size: %d \n", data_path, fsize);
    
    printf(" \n*****[%d]*****\n", stage++); 
    /* 3. send request transmitting command and check */
    /* to be done */
    command = 0x7290;
    if (command) {
        cmd_tx[0] = command & 0xff;
        cmd_tx[1] = (command >> 8) & 0xff;
        printf(" tx:%x %x \n", cmd_tx[1], cmd_tx[0]);
    }

    getcmd = 0;
    temp32 = 1;
    while (getcmd != 0x7290) {
        usleep(1000);
        tx_command(fd, cmd_rx, cmd_tx, cmd_size);
        getcmd = cmd_rx[0] | (cmd_rx[1] << 8);
        if (temp32 != getcmd) {
            printf(" get status:0x%x\n", getcmd);
            temp32 = getcmd;
        }
    }

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
    uint32_t bitset;
    
    count = 0;
    uint8_t *ptr;
    usize = fsize;
    ptr = tx_buff;
    while (usize) {
        count++;
        usleep(1);
        
        //bitset = 0;
        //ioctl(fd, _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WD_CTL_PIN
        //usleep(1);
        //bitset = 1;
        //ioctl(fd, _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WD_CTL_PIN
        printf(" %d r:%d ",count, usize);
        tx_data(fd, rx_buff, ptr, PKT_SIZE);
        
        ptr += PKT_SIZE;
        if (usize < PKT_SIZE)
            usize = 0;
        else
            usize -= PKT_SIZE;
    }
    bitset = 0;
    ioctl(fd, _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WD_CTL_PIN
    
    printf(" \n*****[%d]*****\n", stage++);
    /* 6. pull down the control_pin to notice the end of transmitting */
    
    bitset = 0;
    ioctl(fd, _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WD_CTL_PIN
    
    printf(" \n*****[%d]*****\n", stage++);
    /* 7. request status to back to command mode */
    command = 0x5380;
    if (command) {
        cmd_tx[0] = command & 0xff;
        cmd_tx[1] = (command >> 8) & 0xff;
        printf(" tx:%x %x \n", cmd_tx[1], cmd_tx[0]);
    }
    
    getcmd = 0;
    temp32 = 1;
    while (getcmd != 0x5380) {
        usleep(1000);
        tx_command(fd, cmd_rx, cmd_tx, cmd_size);
        getcmd = cmd_rx[0] | (cmd_rx[1] << 8);
        if (temp32 != getcmd) {
            printf(" get status:0x%x\n", getcmd);
            temp32 = getcmd;
        }
    }
    
    printf(" \n*****[%d]*****\n", stage++);
    /* 8. release the ctl_pin */
    ret = ioctl(fd, _IOW(SPI_IOC_MAGIC, 7, __u32), &bits);   //SPI_IOC_REL_CTL_PIN
    if (ret == -1) 
        pabort("can't SPI_IOC_REL_CTL_PIN"); 
   
    printf(" \n*****[%d]*****\n", stage++);
    
    fclose(fpd);
    free(tx_buff);
    free(rx_buff);
         
    close(fd);  //???? 
 
    return ret; 
} 
#endif