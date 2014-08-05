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
 
#define	SPI_CPHA	0x01			/* clock phase */
#define	SPI_CPOL	0x02			/* clock polarity */
#define	SPI_MODE_0	(0|0)			/* (original MicroWire) */
#define	SPI_MODE_1	(0|SPI_CPHA)
#define	SPI_MODE_2	(SPI_CPOL|0)
#define	SPI_MODE_3	(SPI_CPOL|SPI_CPHA)
#define	SPI_CS_HIGH	0x04			/* chipselect active high? */
#define	SPI_LSB_FIRST	0x08			/* per-word bits-on-wire */
#define	SPI_3WIRE	0x10			/* SI/SO signals shared */
#define	SPI_LOOP	0x20			/* loopback mode */
#define	SPI_NO_CS	0x40			/* 1 dev/bus, no chipselect */
#define	SPI_READY	0x80			/* slave pulls low to pause */
#define	SPI_TX_DUAL	0x100			/* transmit with 2 wires */
#define	SPI_TX_QUAD	0x200			/* transmit with 4 wires */
#define	SPI_RX_DUAL	0x400			/* receive with 2 wires */
#define	SPI_RX_QUAD	0x800			/* receive with 4 wires */
#define BUFF_SIZE  2048

static void pabort(const char *s) 
{ 
    perror(s); 
    abort(); 
} 
 
static const char *device = "/dev/spidev1.1"; 
static uint8_t mode; 
static uint8_t bits = 8; 
static uint32_t speed = 1000000; 
static uint16_t delay; 
 
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
         "  -3 --3wire    SI/SO signals shared\n"); 
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
 
        c = getopt_long(argc, argv, "D:s:d:b:lHOLC3NR", lopts, NULL); 
 
        if (c == -1) 
            break; 
 
        switch (c) { 
        case 'D':   //?? 
            device = optarg; 
            break; 
        case 's':   //硉瞯 
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
        default:    //???? 
            print_usage(argv[0]); 
            break; 
        } 
    } 
} 
 
int main(int argc, char *argv[]) 
{ 
    int ret = 0; 
    int fd; 
 
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
 
    	transfer(fd);   //???? 
 
    close(fd);  //???? 
 
    return ret; 
} 
