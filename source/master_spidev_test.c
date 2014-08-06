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
 
static const char *device = "/dev/spidev32765.0"; 
static const char *data_path = "/root/tx/1.jpg"; 
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

    struct spi_ioc_transfer tr = {  //?���}��l��spi_ioc_transfer?���^ 
        .tx_buf = (unsigned long)tx, 
        .rx_buf = (unsigned long)rx, 
        .len = BUFF_SIZE,
        .delay_usecs = delay, 
        .speed_hz = speed, 
        .bits_per_word = bits, 
    }; 
    
    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);   //ioctl�q?�ާ@,???�u 
    if (ret < 1) 
        pabort("can't send spi message"); 
 
    errcnt = 0; i = 0;
    for (ret = 0; ret < BUFF_SIZE; ret++) { //���L����??? 
        if (!(ret % 6))     //6??�u?�@�L���L 
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
  //  for (ret = 0; ret < ex_size; ret++) { //���L����??? 
  //      if (!(ret % 6))     //6??�u?�@�L���L 
  //          puts(""); 
	//			tg = (ret - 0) & 0xff;
	//			if (rx[ret] != tg) {
	//    		errcnt++;
	//    		i = 1;
	//			}
  //      printf("%.2X:%.2X/%d ", rx[ret], tg, i); 
	//			i  = 0;
  //  } 
  //  puts(""); 
  //  printf(" error count: %d\n", errcnt);
	puts("");
}
#endif
static void print_usage(const char *prog)   //?????���L?�U�H�� 
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
        static const struct option lopts[] = {  //??�R�O�� 
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
        case 'D':   //??�W 
       			printf(" -D %s \n", optarg);
            device = optarg; 
            break; 
        case 's':   //�t�v 
        		printf(" -s %s \n", optarg);
            speed = atoi(optarg); 
            break; 
        case 'd':   //��??? 
            delay = atoi(optarg); 
            break; 
        case 'b':   //�C�r�t�h�֦� 
            bits = atoi(optarg); 
            break; 
        case 'l':   //�^�e�Ҧ� 
            mode |= SPI_LOOP; 
            break; 
        case 'H':   //??�ۦ� 
            mode |= SPI_CPHA; 
            break; 
        case 'O':   //??��� 
            mode |= SPI_CPOL; 
            break; 
        case 'L':   //lsb �̧C���Ħ� 
            mode |= SPI_LSB_FIRST; 
            break; 
        case 'C':   //��?��?�� 
            mode |= SPI_CS_HIGH; 
            break; 
        case '3':   //3???�Ҧ� 
            mode |= SPI_3WIRE; 
            break; 
        case 'N':   //?��? 
            mode |= SPI_NO_CS; 
            break; 
        case 'R':   //?��ԧC?������?�u?? 
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
        default:    //??��?? 
            print_usage(argv[0]); 
            break; 
        } 
    } 
} 
 
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
		
    parse_opts(argc, argv); //�ѪR????��?? 
 
    fd = open(device, O_RDWR);  //��???��� 
    if (fd < 0) 
        pabort("can't open device"); 
 
    /*
     * spi mode //?�mspi??�Ҧ�
     */ 
    ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);    //?�Ҧ� 
    if (ret == -1) 
        pabort("can't set spi mode"); 
 
    ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);    //?�Ҧ� 
    if (ret == -1) 
        pabort("can't get spi mode"); 
 
    /*
     * bits per word    //?�m�C?�r�t�h�֦�
     */ 
    ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);   //? �C?�r�t�h�֦� 
    if (ret == -1) 
        pabort("can't set bits per word"); 
 
    ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);   //? �C?�r�t�h�֦� 
    if (ret == -1) 
        pabort("can't get bits per word"); 
 
    /*
     * max speed hz     //?�m�t�v
     */ 
    ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);   //?�t�v 
    if (ret == -1) 
        pabort("can't set max speed hz"); 
 
    ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);   //?�t�v 
    if (ret == -1) 
        pabort("can't get max speed hz"); 
    //���L�Ҧ�,�C�r�h�֦�M�t�v�H�� 
    printf("spi mode: %d\n", mode); 
    printf("bits per word: %d\n", bits); 
    printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000); 

		if (command) {
			cmd_tx[0] = command & 0xff;
			cmd_tx[1] = (command >> 8) & 0xff;
			printf("\n tx:%x %x \n", cmd_tx[0], cmd_tx[1]);
		}
		
		//tx_command(fd, cmd_rx, cmd_tx, cmd_size);
   	
   	buffsize = 1*1024*1024;
   	tx_buff = malloc(buffsize);
   	if (tx_buff) {
   		printf(" tx buff alloc success!!\n");
   	}
   	rx_buff = malloc(buffsize);
   	if (rx_buff) {
   		printf(" rx buff alloc success!!\n");
   	}
   	
   	printf(" open file %s \n", data_path);
   	//
   	fpd = fopen(data_path, "r");
   	
   	if (!fpd) {
   		printf(" %s file open failed \n", data_path);
   		return ret;
   	}
   	printf(" %s file open succeed \n", data_path);
   	
   	fsize = fread(tx_buff, 1, buffsize, fpd);
   	
   	printf(" [%s] size: %d \n", data_path, fsize);
    //printf("\n rx:%x %x \n", cmd_rx[0], cmd_rx[1]);
    	//transfer(fd); 
    int usize;
    uint8_t *ptr;
    usize = fsize;
    ptr = tx_buff;

    while (usize) {
    	printf(" remain:%d ", usize);
    	tx_data(fd, rx_buff, ptr, 1024);
    	
    	ptr += 1024;
    	if (usize < 1024)
    		usize = 0;
    	else
    		usize -= 1024;
    	
    }
    
    fclose(fpd);
 		free(tx_buff);
 		free(rx_buff);
 		
    close(fd);  //???? 
 
    return ret; 
} 
