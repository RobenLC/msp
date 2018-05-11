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
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0])) 

#define SPI_DISABLE (1)
 
#define SPI_AT_CPHA  0x02          /* clock phase */
#define SPI_AT_CPOL  0x01          /* clock polarity */
#define SPI_AT_MODE_0  (0|0)            /* (original MicroWire) */
#define SPI_AT_MODE_1  (0|SPI_AT_CPHA)
#define SPI_AT_MODE_2  (SPI_AT_CPOL|0)
#define SPI_AT_MODE_3  (SPI_AT_CPOL|SPI_AT_CPHA)
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

#define USB_META_SIZE 512
#define PT_BUF_SIZE (32768*3)
#define MAX_EVENTS (32)
#define EPOLLLT (0)
#define USB_SAVE_RESULT (1)

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

#define MIN_SECTOR_SIZE  (512)
#define RING_BUFF_NUM   (2000)//(6000/4) //(1024)
#define DATA_RX_SIZE RING_BUFF_NUM

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
#define OP_SAVE             0x32

/* usblp ioctls */
#define IOCNR_CONTI_READ_START  8
#define IOCNR_CONTI_READ_STOP    9
#define IOCNR_CONTI_READ_PROBE    10

/* Data requested by client. */
#define PRINT_ACCEL     (0x01)
#define PRINT_GYRO      (0x02)
#define PRINT_QUAT      (0x04)

#define ACCEL_ON        (0x01)
#define GYRO_ON         (0x02)

#define MPU9250
#define MPU6500
#define AK8963_SECONDARY
//#define AK89xx_SECONDARY

/*
#define INV_X_GYRO      (0x40)
#define INV_Y_GYRO      (0x20)
#define INV_Z_GYRO      (0x10)
#define INV_XYZ_GYRO    (INV_X_GYRO | INV_Y_GYRO | INV_Z_GYRO)
#define INV_XYZ_ACCEL   (0x08)
#define INV_XYZ_COMPASS (0x01)
*/
#define CALAB_DATA_BUFF_SIZE  (2048)

static int *totSalloc=0;

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

struct usbdev_s{
    struct usbhost_s *pushost1;
    struct usbhost_s *pushost2;
};

struct calab_data_s {
    int calab_done;
    int calab_count;
    int calab_total;
    int calab_still[CALAB_DATA_BUFF_SIZE];
    float calab_avg;
    float calab_div;
};

struct accelc_info_s {
    int accelc_status;
    double accelc_xradio;
    double accelc_yradio;
    double accelc_zradio;
    double accelc_frangdiv;
    double accelc_grange;
    float accelc_flsbthd;
    short   accelc_mid[4];
    struct calab_data_s accelc_xmax;
    struct calab_data_s accelc_xmin;
    struct calab_data_s accelc_xtmp;
    struct calab_data_s accelc_ymax;
    struct calab_data_s accelc_ymin;
    struct calab_data_s accelc_ytmp;
    struct calab_data_s accelc_zmax;
    struct calab_data_s accelc_zmin;
    struct calab_data_s accelc_ztmp;
};

struct gyroc_info_s {
    int gyroc_status;
    int gyroc_sdycnt;
    short   gyroc_mid[4];
    struct calab_data_s gyroc_zerox;
    struct calab_data_s gyroc_tmpx;
    struct calab_data_s gyroc_zeroy;
    struct calab_data_s gyroc_tmpy;
    struct calab_data_s gyroc_zeroz;
    struct calab_data_s gyroc_tmpz;
};

/* Hardware registers needed by driver. */
struct gyro_reg_s {
    unsigned char who_am_i;
    unsigned char rate_div;
    unsigned char lpf;
    unsigned char prod_id;
    unsigned char user_ctrl;
    unsigned char fifo_en;
    unsigned char gyro_cfg;
    unsigned char accel_cfg;
    unsigned char accel_cfg2;
    unsigned char lp_accel_odr;
    unsigned char motion_thr;
    unsigned char motion_dur;
    unsigned char fifo_count_h;
    unsigned char fifo_count_l;
    unsigned char fifo_r_w;
    unsigned char raw_gyro;
    unsigned char raw_accel;
    unsigned char temp;
    unsigned char int_enable;
    unsigned char dmp_int_status;
    unsigned char int_status;
    unsigned char accel_intel;
    unsigned char pwr_mgmt_1;
    unsigned char pwr_mgmt_2;
    unsigned char int_pin_cfg;
    unsigned char mem_r_w;
    unsigned char accel_offs;
    unsigned char i2c_mst;
    unsigned char bank_sel;
    unsigned char mem_start_addr;
    unsigned char prgm_start_h;
#if defined AK89xx_SECONDARY
    unsigned char s0_addr;
    unsigned char s0_reg;
    unsigned char s0_ctrl;
    unsigned char s1_addr;
    unsigned char s1_reg;
    unsigned char s1_ctrl;
    unsigned char s4_ctrl;
    unsigned char s0_do;
    unsigned char s1_do;
    unsigned char i2c_delay_ctrl;
    unsigned char raw_compass;
    /* The I2C_MST_VDDIO bit is in this register. */
    unsigned char yg_offs_tc;
#endif
};

/* Information specific to a particular device. */
struct hw_s {
    unsigned char addr;
    unsigned short max_fifo;
    unsigned char num_reg;
    unsigned short temp_sens;
    short temp_offset;
    unsigned short bank_size;
#if defined AK89xx_SECONDARY
    unsigned short compass_fsr;
#endif
};

/* When entering motion interrupt mode, the driver keeps track of the
 * previous state so that it can be restored at a later time.
 * TODO: This is tacky. Fix it.
 */
struct motion_int_cache_s {
    unsigned short gyro_fsr;
    unsigned short accel_fsr;
    unsigned short lpf;
    unsigned short sample_rate;
    unsigned short sensors_on;
    unsigned short fifo_sensors;
    unsigned int dmp_on;
};

/* Cached chip configuration data.
 * TODO: A lot of these can be handled with a bitmask.
 */
struct chip_cfg_s {
    /* Matches gyro_cfg >> 3 & 0x03 */
    unsigned int gyro_fsr;
    /* Matches accel_cfg >> 3 & 0x03 */
    unsigned int accel_fsr;
    /* Enabled sensors. Uses same masks as fifo_en, NOT pwr_mgmt_2. */
    unsigned int sensors;
    /* Matches config register. */
    unsigned int lpf;
    unsigned int clk_src;
    /* Sample rate, NOT rate divider. */
    unsigned int sample_rate;
    /* Matches fifo_en register. */
    unsigned int fifo_enable;
    /* Matches int enable register. */
    unsigned int int_enable;
    /* 1 if devices on auxiliary I2C bus appear on the primary. */
    unsigned int bypass_mode;
    /* 1 if half-sensitivity.
     * NOTE: This doesn't belong here, but everything else in hw_s is const,
     * and this allows us to save some precious RAM.
     */
    unsigned int accel_half;
    /* 1 if device in low-power accel-only mode. */
    unsigned int lp_accel_mode;
    /* 1 if interrupts are only triggered on motion events. */
    unsigned int int_motion_only;
    struct motion_int_cache_s cache;
    /* 1 for active low interrupts. */
    unsigned int active_low_int;
    /* 1 for latched interrupts. */
    unsigned int latched_int;
    /* 1 if DMP is enabled. */
    unsigned int dmp_on;
    /* Ensures that DMP will only be loaded once. */
    unsigned int dmp_loaded;
    /* Sampling rate used when DMP is enabled. */
    unsigned int dmp_sample_rate;
#ifdef AK89xx_SECONDARY
    /* Compass sample rate. */
    unsigned int compass_sample_rate;
    unsigned int compass_addr;
    short mag_sens_adj[4];
#endif
};

/* Information for self-test. */
struct test_s {
    unsigned long gyro_sens;
    unsigned long accel_sens;
    unsigned char reg_rate_div;
    unsigned char reg_lpf;
    unsigned char reg_gyro_fsr;
    unsigned char reg_accel_fsr;
    unsigned short wait_ms;
    unsigned char packet_thresh;
    float min_dps;
    float max_dps;
    float max_gyro_var;
    float min_g;
    float max_g;
    float max_accel_var;
#ifdef MPU6500
    float max_g_offset;
    unsigned short sample_wait_ms;
#endif
};

/* Gyro driver state variables. */
struct gyro_state_s {
    struct chip_cfg_s chip_cfg;
    const struct gyro_reg_s *reg;
    const struct hw_s *hw;
    const struct test_s *test;
};

/* Filter configurations. */
enum lpf_e {
    INV_FILTER_256HZ_NOLPF2 = 0,
    INV_FILTER_188HZ,
    INV_FILTER_98HZ,
    INV_FILTER_42HZ,
    INV_FILTER_20HZ,
    INV_FILTER_10HZ,
    INV_FILTER_5HZ,
    INV_FILTER_2100HZ_NOLPF,
    NUM_FILTER
};

/* Full scale ranges. */
enum gyro_fsr_e {
    INV_FSR_250DPS = 0,
    INV_FSR_500DPS,
    INV_FSR_1000DPS,
    INV_FSR_2000DPS,
    NUM_GYRO_FSR
};

/* Full scale ranges. */
enum accel_fsr_e {
    INV_FSR_2G = 0,
    INV_FSR_4G,
    INV_FSR_8G,
    INV_FSR_16G,
    NUM_ACCEL_FSR
};

/* Clock sources. */
enum clock_sel_e {
    INV_CLK_INTERNAL = 0,
    INV_CLK_PLL,
    NUM_CLK
};

/* Low-power accel wakeup rates. */
enum lp_accel_rate_e {
    INV_LPA_0_24HZ,
    INV_LPA_0_49HZ,
    INV_LPA_0_98HZ,
    INV_LPA_1_95HZ,
    INV_LPA_3_91HZ,
    INV_LPA_7_81HZ,
    INV_LPA_15_63HZ,
    INV_LPA_31_25HZ,
    INV_LPA_62_50HZ,
    INV_LPA_125HZ,
    INV_LPA_250HZ,
    INV_LPA_500HZ
};

#define BIT_I2C_MST_VDDIO   (0x80)
#define BIT_FIFO_EN         (0x40)
#define BIT_DMP_EN          (0x80)
#define BIT_FIFO_RST        (0x04)
#define BIT_DMP_RST         (0x08)
#define BIT_FIFO_OVERFLOW   (0x10)
#define BIT_DATA_RDY_EN     (0x01)
#define BIT_DMP_INT_EN      (0x02)
#define BIT_MOT_INT_EN      (0x40)
#define BITS_FSR            (0x18)
#define BITS_LPF            (0x07)
#define BITS_HPF            (0x07)
#define BITS_CLK            (0x07)
#define BIT_FIFO_SIZE_1024  (0x40)
#define BIT_FIFO_SIZE_2048  (0x80)
#define BIT_FIFO_SIZE_4096  (0xC0)
#define BIT_RESET           (0x80)
#define BIT_SLEEP           (0x40)
#define BIT_S0_DELAY_EN     (0x01)
#define BIT_S2_DELAY_EN     (0x04)
#define BITS_SLAVE_LENGTH   (0x0F)
#define BIT_SLAVE_BYTE_SW   (0x40)
#define BIT_SLAVE_GROUP     (0x10)
#define BIT_SLAVE_EN        (0x80)
#define BIT_I2C_READ        (0x80)
#define BITS_I2C_MASTER_DLY (0x1F)
#define BIT_AUX_IF_EN       (0x20)
#define BIT_ACTL            (0x80)
#define BIT_LATCH_EN        (0x20)
#define BIT_ANY_RD_CLR      (0x10)
#define BIT_BYPASS_EN       (0x02)
#define BITS_WOM_EN         (0xC0)
#define BIT_LPA_CYCLE       (0x20)
#define BIT_STBY_XA         (0x20)
#define BIT_STBY_YA         (0x10)
#define BIT_STBY_ZA         (0x08)
#define BIT_STBY_XG         (0x04)
#define BIT_STBY_YG         (0x02)
#define BIT_STBY_ZG         (0x01)
#define BIT_STBY_XYZA       (BIT_STBY_XA | BIT_STBY_YA | BIT_STBY_ZA)
#define BIT_STBY_XYZG       (BIT_STBY_XG | BIT_STBY_YG | BIT_STBY_ZG)
#define BIT_ACCL_FC_B       (0x08)
#define MPU9250_WHOAMI      (0x71)

#define SUPPORTS_AK89xx_HIGH_SENS   (0x10)
#define AK89xx_FSR                  (4915)

#define AKM_REG_WHOAMI      (0x00)

#define AKM_REG_ST1         (0x02)
#define AKM_REG_HXL         (0x03)
#define AKM_REG_ST2         (0x09)

#define AKM_REG_CNTL        (0x0A)
#define AKM_REG_ASTC        (0x0C)
#define AKM_REG_ASAX        (0x10)
#define AKM_REG_ASAY        (0x11)
#define AKM_REG_ASAZ        (0x12)

#define AKM_DATA_READY      (0x01)
#define AKM_DATA_OVERRUN    (0x02)
#define AKM_OVERFLOW        (0x80)
#define AKM_DATA_ERROR      (0x40)

#define AKM_BIT_SELF_TEST   (0x40)

#define AKM_POWER_DOWN          (0x00 | SUPPORTS_AK89xx_HIGH_SENS)
#define AKM_SINGLE_MEASUREMENT  (0x01 | SUPPORTS_AK89xx_HIGH_SENS)
#define AKM_FUSE_ROM_ACCESS     (0x0F | SUPPORTS_AK89xx_HIGH_SENS)
#define AKM_MODE_SELF_TEST      (0x08 | SUPPORTS_AK89xx_HIGH_SENS)

#define AKM_WHOAMI      (0x48)

const struct gyro_reg_s reg = {
    .who_am_i       = 0x75,
    .rate_div       = 0x19,
    .lpf            = 0x1A,
    .prod_id        = 0x0C,
    .user_ctrl      = 0x6A,
    .fifo_en        = 0x23,
    .gyro_cfg       = 0x1B,
    .accel_cfg      = 0x1C,
    .accel_cfg2     = 0x1D,
    .lp_accel_odr   = 0x1E,
    .motion_thr     = 0x1F,
    .motion_dur     = 0x20,
    .fifo_count_h   = 0x72,
    .fifo_count_l   = 0x73,
    .fifo_r_w       = 0x74,
    .raw_gyro       = 0x43,
    .raw_accel      = 0x3B,
    .temp           = 0x41,
    .int_enable     = 0x38,
    .dmp_int_status = 0x39,
    .int_status     = 0x3A,
    .accel_intel    = 0x69,
    .pwr_mgmt_1     = 0x6B,
    .pwr_mgmt_2     = 0x6C,
    .int_pin_cfg    = 0x37,
    .mem_r_w        = 0x6F,
    .accel_offs     = 0x77,
    .i2c_mst        = 0x24,
    .bank_sel       = 0x6D,
    .mem_start_addr = 0x6E,
    .prgm_start_h   = 0x70
#ifdef AK89xx_SECONDARY
    ,.raw_compass   = 0x49,
    .s0_addr        = 0x25,
    .s0_reg         = 0x26,
    .s0_ctrl        = 0x27,
    .s1_addr        = 0x28,
    .s1_reg         = 0x29,
    .s1_ctrl        = 0x2A,
    .s4_ctrl        = 0x34,
    .s0_do          = 0x63,
    .s1_do          = 0x64,
    .i2c_delay_ctrl = 0x67
#endif
};
const struct hw_s hw = {
    .addr           = 0x68,
    .max_fifo       = 1024,
    .num_reg        = 128,
    .temp_sens      = 321,
    .temp_offset    = 0,
    .bank_size      = 256
#if defined AK89xx_SECONDARY
    ,.compass_fsr    = AK89xx_FSR
#endif
};

const struct test_s test = {
    .gyro_sens      = 32768/250,
    .accel_sens     = 32768/2,  //FSR = +-2G = 16384 LSB/G
    .reg_rate_div   = 0,    /* 1kHz. */
    .reg_lpf        = 2,    /* 92Hz low pass filter*/
    .reg_gyro_fsr   = 0,    /* 250dps. */
    .reg_accel_fsr  = 0x0,  /* Accel FSR setting = 2g. */
    .wait_ms        = 200,   //200ms stabilization time
    .packet_thresh  = 200,    /* 200 samples */
    .min_dps        = 20.f,  //20 dps for Gyro Criteria C
    .max_dps        = 60.f, //Must exceed 60 dps threshold for Gyro Criteria B
    .max_gyro_var   = .5f, //Must exceed +50% variation for Gyro Criteria A
    .min_g          = .225f, //Accel must exceed Min 225 mg for Criteria B
    .max_g          = .675f, //Accel cannot exceed Max 675 mg for Criteria B
    .max_accel_var  = .5f,  //Accel must be within 50% variation for Criteria A
    .max_g_offset   = .5f,   //500 mg for Accel Criteria C
    .sample_wait_ms = 10    //10ms sample time wait
};

static struct gyro_state_s *st;

/* The sensors can be mounted onto the board in any orientation. The mounting
 * matrix seen below tells the MPL how to rotate the raw data from thei
 * driver(s).
 * TODO: The following matrices refer to the configuration on an internal test
 * board at Invensense. If needed, please modify the matrices to match the
 * chip-to-body matrix for your particular set up.
 */
static signed char gyro_orientation[9] = {-1, 0, 0,
                                           0,-1, 0,
                                           0, 0, 1};

enum packet_type_e {
    PACKET_TYPE_ACCEL,
    PACKET_TYPE_GYRO,
    PACKET_TYPE_QUAT,
    PACKET_TYPE_TAP,
    PACKET_TYPE_ANDROID_ORIENT,
    PACKET_TYPE_PEDO,
    PACKET_TYPE_MISC
};

#define INT_SRC_TAP             (0x01)
#define INT_SRC_ANDROID_ORIENT  (0x08)

#define DMP_FEATURE_SEND_ANY_GYRO   (DMP_FEATURE_SEND_RAW_GYRO | \
                                     DMP_FEATURE_SEND_CAL_GYRO)
                                     
#define GYRO_SF             (46850825LL * 200 / DMP_SAMPLE_RATE)
#define DMP_SAMPLE_RATE     (200)

//#define FIFO_CORRUPTION_CHECK
#ifdef FIFO_CORRUPTION_CHECK
#define QUAT_ERROR_THRESH       (1L<<24)
#define QUAT_MAG_SQ_NORMALIZED  (1L<<28)
#define QUAT_MAG_SQ_MIN         (QUAT_MAG_SQ_NORMALIZED - QUAT_ERROR_THRESH)
#define QUAT_MAG_SQ_MAX         (QUAT_MAG_SQ_NORMALIZED + QUAT_ERROR_THRESH)
#endif

struct dmp_s {
    void (*tap_cb)(unsigned char count, unsigned char direction);
    void (*android_orient_cb)(unsigned char orientation);
    unsigned short orient;
    unsigned short feature_mask;
    unsigned short fifo_rate;
    unsigned char packet_length;
};

static struct dmp_s dmp = {
    .tap_cb = NULL,
    .android_orient_cb = NULL,
    .orient = 0,
    .feature_mask = 0,
    .fifo_rate = 0,
    .packet_length = 0
};

struct rx_s {
    unsigned char header[3];
    unsigned char cmd;
};

struct hal_s {
    unsigned char sensors;
    unsigned char dmp_on;
    unsigned char wait_for_tap;
    unsigned char new_gyro;
    unsigned short report;
    unsigned short dmp_features;
    unsigned char motion_int_mode;
    struct rx_s rx;
};
static struct hal_s *hal;

#define MAX_PACKET_LENGTH (12)
#ifdef MPU6500
#define HWST_MAX_PACKET_LENGTH (1024)
#endif

/* These defines are copied from dmpDefaultMPU6050.c in the general MPL
 * releases. These defines may change for each DMP image, so be sure to modify
 * these values when switching to a new image.
 */
#define CFG_LP_QUAT             (2712)
#define END_ORIENT_TEMP         (1866)
#define CFG_27                  (2742)
#define CFG_20                  (2224)
#define CFG_23                  (2745)
#define CFG_FIFO_ON_EVENT       (2690)
#define END_PREDICTION_UPDATE   (1761)
#define CGNOTICE_INTR           (2620)
#define X_GRT_Y_TMP             (1358)
#define CFG_DR_INT              (1029)
#define CFG_AUTH                (1035)
#define UPDATE_PROP_ROT         (1835)
#define END_COMPARE_Y_X_TMP2    (1455)
#define SKIP_X_GRT_Y_TMP        (1359)
#define SKIP_END_COMPARE        (1435)
#define FCFG_3                  (1088)
#define FCFG_2                  (1066)
#define FCFG_1                  (1062)
#define END_COMPARE_Y_X_TMP3    (1434)
#define FCFG_7                  (1073)
#define FCFG_6                  (1106)
#define FLAT_STATE_END          (1713)
#define SWING_END_4             (1616)
#define SWING_END_2             (1565)
#define SWING_END_3             (1587)
#define SWING_END_1             (1550)
#define CFG_8                   (2718)
#define CFG_15                  (2727)
#define CFG_16                  (2746)
#define CFG_EXT_GYRO_BIAS       (1189)
#define END_COMPARE_Y_X_TMP     (1407)
#define DO_NOT_UPDATE_PROP_ROT  (1839)
#define CFG_7                   (1205)
#define FLAT_STATE_END_TEMP     (1683)
#define END_COMPARE_Y_X         (1484)
#define SKIP_SWING_END_1        (1551)
#define SKIP_SWING_END_3        (1588)
#define SKIP_SWING_END_2        (1566)
#define TILTG75_START           (1672)
#define CFG_6                   (2753)
#define TILTL75_END             (1669)
#define END_ORIENT              (1884)
#define CFG_FLICK_IN            (2573)
#define TILTL75_START           (1643)
#define CFG_MOTION_BIAS         (1208)
#define X_GRT_Y                 (1408)
#define TEMPLABEL               (2324)
#define CFG_ANDROID_ORIENT_INT  (1853)
#define CFG_GYRO_RAW_DATA       (2722)
#define X_GRT_Y_TMP2            (1379)

#define D_0_22                  (22+512)
#define D_0_24                  (24+512)

#define D_0_36                  (36)
#define D_0_52                  (52)
#define D_0_96                  (96)
#define D_0_104                 (104)
#define D_0_108                 (108)
#define D_0_163                 (163)
#define D_0_188                 (188)
#define D_0_192                 (192)
#define D_0_224                 (224)
#define D_0_228                 (228)
#define D_0_232                 (232)
#define D_0_236                 (236)

#define D_1_2                   (256 + 2)
#define D_1_4                   (256 + 4)
#define D_1_8                   (256 + 8)
#define D_1_10                  (256 + 10)
#define D_1_24                  (256 + 24)
#define D_1_28                  (256 + 28)
#define D_1_36                  (256 + 36)
#define D_1_40                  (256 + 40)
#define D_1_44                  (256 + 44)
#define D_1_72                  (256 + 72)
#define D_1_74                  (256 + 74)
#define D_1_79                  (256 + 79)
#define D_1_88                  (256 + 88)
#define D_1_90                  (256 + 90)
#define D_1_92                  (256 + 92)
#define D_1_96                  (256 + 96)
#define D_1_98                  (256 + 98)
#define D_1_106                 (256 + 106)
#define D_1_108                 (256 + 108)
#define D_1_112                 (256 + 112)
#define D_1_128                 (256 + 144)
#define D_1_152                 (256 + 12)
#define D_1_160                 (256 + 160)
#define D_1_176                 (256 + 176)
#define D_1_178                 (256 + 178)
#define D_1_218                 (256 + 218)
#define D_1_232                 (256 + 232)
#define D_1_236                 (256 + 236)
#define D_1_240                 (256 + 240)
#define D_1_244                 (256 + 244)
#define D_1_250                 (256 + 250)
#define D_1_252                 (256 + 252)
#define D_2_12                  (512 + 12)
#define D_2_96                  (512 + 96)
#define D_2_108                 (512 + 108)
#define D_2_208                 (512 + 208)
#define D_2_224                 (512 + 224)
#define D_2_236                 (512 + 236)
#define D_2_244                 (512 + 244)
#define D_2_248                 (512 + 248)
#define D_2_252                 (512 + 252)

#define CPASS_BIAS_X            (35 * 16 + 4)
#define CPASS_BIAS_Y            (35 * 16 + 8)
#define CPASS_BIAS_Z            (35 * 16 + 12)
#define CPASS_MTX_00            (36 * 16)
#define CPASS_MTX_01            (36 * 16 + 4)
#define CPASS_MTX_02            (36 * 16 + 8)
#define CPASS_MTX_10            (36 * 16 + 12)
#define CPASS_MTX_11            (37 * 16)
#define CPASS_MTX_12            (37 * 16 + 4)
#define CPASS_MTX_20            (37 * 16 + 8)
#define CPASS_MTX_21            (37 * 16 + 12)
#define CPASS_MTX_22            (43 * 16 + 12)
#define D_EXT_GYRO_BIAS_X       (61 * 16)
#define D_EXT_GYRO_BIAS_Y       (61 * 16) + 4
#define D_EXT_GYRO_BIAS_Z       (61 * 16) + 8
#define D_ACT0                  (40 * 16)
#define D_ACSX                  (40 * 16 + 4)
#define D_ACSY                  (40 * 16 + 8)
#define D_ACSZ                  (40 * 16 + 12)

#define FLICK_MSG               (45 * 16 + 4)
#define FLICK_COUNTER           (45 * 16 + 8)
#define FLICK_LOWER             (45 * 16 + 12)
#define FLICK_UPPER             (46 * 16 + 12)

#define D_AUTH_OUT              (992)
#define D_AUTH_IN               (996)
#define D_AUTH_A                (1000)
#define D_AUTH_B                (1004)

#define D_PEDSTD_BP_B           (768 + 0x1C)
#define D_PEDSTD_HP_A           (768 + 0x78)
#define D_PEDSTD_HP_B           (768 + 0x7C)
#define D_PEDSTD_BP_A4          (768 + 0x40)
#define D_PEDSTD_BP_A3          (768 + 0x44)
#define D_PEDSTD_BP_A2          (768 + 0x48)
#define D_PEDSTD_BP_A1          (768 + 0x4C)
#define D_PEDSTD_INT_THRSH      (768 + 0x68)
#define D_PEDSTD_CLIP           (768 + 0x6C)
#define D_PEDSTD_SB             (768 + 0x28)
#define D_PEDSTD_SB_TIME        (768 + 0x2C)
#define D_PEDSTD_PEAKTHRSH      (768 + 0x98)
#define D_PEDSTD_TIML           (768 + 0x2A)
#define D_PEDSTD_TIMH           (768 + 0x2E)
#define D_PEDSTD_PEAK           (768 + 0X94)
#define D_PEDSTD_STEPCTR        (768 + 0x60)
#define D_PEDSTD_TIMECTR        (964)
#define D_PEDSTD_DECI           (768 + 0xA0)

#define D_HOST_NO_MOT           (976)
#define D_ACCEL_BIAS            (660)

#define D_ORIENT_GAP            (76)

#define D_TILT0_H               (48)
#define D_TILT0_L               (50)
#define D_TILT1_H               (52)
#define D_TILT1_L               (54)
#define D_TILT2_H               (56)
#define D_TILT2_L               (58)
#define D_TILT3_H               (60)
#define D_TILT3_L               (62)

#define DMP_CODE_SIZE           (3062)

static const unsigned char dmp_memory[DMP_CODE_SIZE] = {
    /* bank # 0 */
    0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x02, 0x00, 0x03, 0x00, 0x00,
    0x00, 0x65, 0x00, 0x54, 0xff, 0xef, 0x00, 0x00, 0xfa, 0x80, 0x00, 0x0b, 0x12, 0x82, 0x00, 0x01,
    0x03, 0x0c, 0x30, 0xc3, 0x0e, 0x8c, 0x8c, 0xe9, 0x14, 0xd5, 0x40, 0x02, 0x13, 0x71, 0x0f, 0x8e,
    0x38, 0x83, 0xf8, 0x83, 0x30, 0x00, 0xf8, 0x83, 0x25, 0x8e, 0xf8, 0x83, 0x30, 0x00, 0xf8, 0x83,
    0xff, 0xff, 0xff, 0xff, 0x0f, 0xfe, 0xa9, 0xd6, 0x24, 0x00, 0x04, 0x00, 0x1a, 0x82, 0x79, 0xa1,
    0x00, 0x00, 0x00, 0x3c, 0xff, 0xff, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x38, 0x83, 0x6f, 0xa2,
    0x00, 0x3e, 0x03, 0x30, 0x40, 0x00, 0x00, 0x00, 0x02, 0xca, 0xe3, 0x09, 0x3e, 0x80, 0x00, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00,
    0x00, 0x0c, 0x00, 0x00, 0x00, 0x0c, 0x18, 0x6e, 0x00, 0x00, 0x06, 0x92, 0x0a, 0x16, 0xc0, 0xdf,
    0xff, 0xff, 0x02, 0x56, 0xfd, 0x8c, 0xd3, 0x77, 0xff, 0xe1, 0xc4, 0x96, 0xe0, 0xc5, 0xbe, 0xaa,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x0b, 0x2b, 0x00, 0x00, 0x16, 0x57, 0x00, 0x00, 0x03, 0x59,
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1d, 0xfa, 0x00, 0x02, 0x6c, 0x1d, 0x00, 0x00, 0x00, 0x00,
    0x3f, 0xff, 0xdf, 0xeb, 0x00, 0x3e, 0xb3, 0xb6, 0x00, 0x0d, 0x22, 0x78, 0x00, 0x00, 0x2f, 0x3c,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x42, 0xb5, 0x00, 0x00, 0x39, 0xa2, 0x00, 0x00, 0xb3, 0x65,
    0xd9, 0x0e, 0x9f, 0xc9, 0x1d, 0xcf, 0x4c, 0x34, 0x30, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00,
    0x3b, 0xb6, 0x7a, 0xe8, 0x00, 0x64, 0x00, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* bank # 1 */
    0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0xfa, 0x92, 0x10, 0x00, 0x22, 0x5e, 0x00, 0x0d, 0x22, 0x9f,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00, 0xff, 0x46, 0x00, 0x00, 0x63, 0xd4, 0x00, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x04, 0xd6, 0x00, 0x00, 0x04, 0xcc, 0x00, 0x00, 0x04, 0xcc, 0x00, 0x00,
    0x00, 0x00, 0x10, 0x72, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x06, 0x00, 0x02, 0x00, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00, 0x64, 0x00, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x03, 0x00,
    0x00, 0x00, 0x00, 0x32, 0xf8, 0x98, 0x00, 0x00, 0xff, 0x65, 0x00, 0x00, 0x83, 0x0f, 0x00, 0x00,
    0xff, 0x9b, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0xb2, 0x6a, 0x00, 0x02, 0x00, 0x00,
    0x00, 0x01, 0xfb, 0x83, 0x00, 0x68, 0x00, 0x00, 0x00, 0xd9, 0xfc, 0x00, 0x7c, 0xf1, 0xff, 0x83,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x00, 0x64, 0x03, 0xe8, 0x00, 0x64, 0x00, 0x28,
    0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00, 0x00, 0x16, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
    0x00, 0x00, 0x10, 0x00, 0x00, 0x2f, 0x00, 0x00, 0x00, 0x00, 0x01, 0xf4, 0x00, 0x00, 0x10, 0x00,
    /* bank # 2 */
    0x00, 0x28, 0x00, 0x00, 0xff, 0xff, 0x45, 0x81, 0xff, 0xff, 0xfa, 0x72, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00, 0x05, 0x00, 0x05, 0xba, 0xc6, 0x00, 0x47, 0x78, 0xa2,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x14,
    0x00, 0x00, 0x25, 0x4d, 0x00, 0x2f, 0x70, 0x6d, 0x00, 0x00, 0x05, 0xae, 0x00, 0x0c, 0x02, 0xd0,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x1b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x64, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x1b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x0e,
    0x00, 0x00, 0x0a, 0xc7, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0xff, 0xff, 0xff, 0x9c,
    0x00, 0x00, 0x0b, 0x2b, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x64,
    0xff, 0xe5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* bank # 3 */
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x80, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x24, 0x26, 0xd3,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x10, 0x00, 0x96, 0x00, 0x3c,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0c, 0x0a, 0x4e, 0x68, 0xcd, 0xcf, 0x77, 0x09, 0x50, 0x16, 0x67, 0x59, 0xc6, 0x19, 0xce, 0x82,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0xd7, 0x84, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc7, 0x93, 0x8f, 0x9d, 0x1e, 0x1b, 0x1c, 0x19,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0x18, 0x85, 0x00, 0x00, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x67, 0x7d, 0xdf, 0x7e, 0x72, 0x90, 0x2e, 0x55, 0x4c, 0xf6, 0xe6, 0x88,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    /* bank # 4 */
    0xd8, 0xdc, 0xb4, 0xb8, 0xb0, 0xd8, 0xb9, 0xab, 0xf3, 0xf8, 0xfa, 0xb3, 0xb7, 0xbb, 0x8e, 0x9e,
    0xae, 0xf1, 0x32, 0xf5, 0x1b, 0xf1, 0xb4, 0xb8, 0xb0, 0x80, 0x97, 0xf1, 0xa9, 0xdf, 0xdf, 0xdf,
    0xaa, 0xdf, 0xdf, 0xdf, 0xf2, 0xaa, 0xc5, 0xcd, 0xc7, 0xa9, 0x0c, 0xc9, 0x2c, 0x97, 0xf1, 0xa9,
    0x89, 0x26, 0x46, 0x66, 0xb2, 0x89, 0x99, 0xa9, 0x2d, 0x55, 0x7d, 0xb0, 0xb0, 0x8a, 0xa8, 0x96,
    0x36, 0x56, 0x76, 0xf1, 0xba, 0xa3, 0xb4, 0xb2, 0x80, 0xc0, 0xb8, 0xa8, 0x97, 0x11, 0xb2, 0x83,
    0x98, 0xba, 0xa3, 0xf0, 0x24, 0x08, 0x44, 0x10, 0x64, 0x18, 0xb2, 0xb9, 0xb4, 0x98, 0x83, 0xf1,
    0xa3, 0x29, 0x55, 0x7d, 0xba, 0xb5, 0xb1, 0xa3, 0x83, 0x93, 0xf0, 0x00, 0x28, 0x50, 0xf5, 0xb2,
    0xb6, 0xaa, 0x83, 0x93, 0x28, 0x54, 0x7c, 0xf1, 0xb9, 0xa3, 0x82, 0x93, 0x61, 0xba, 0xa2, 0xda,
    0xde, 0xdf, 0xdb, 0x81, 0x9a, 0xb9, 0xae, 0xf5, 0x60, 0x68, 0x70, 0xf1, 0xda, 0xba, 0xa2, 0xdf,
    0xd9, 0xba, 0xa2, 0xfa, 0xb9, 0xa3, 0x82, 0x92, 0xdb, 0x31, 0xba, 0xa2, 0xd9, 0xba, 0xa2, 0xf8,
    0xdf, 0x85, 0xa4, 0xd0, 0xc1, 0xbb, 0xad, 0x83, 0xc2, 0xc5, 0xc7, 0xb8, 0xa2, 0xdf, 0xdf, 0xdf,
    0xba, 0xa0, 0xdf, 0xdf, 0xdf, 0xd8, 0xd8, 0xf1, 0xb8, 0xaa, 0xb3, 0x8d, 0xb4, 0x98, 0x0d, 0x35,
    0x5d, 0xb2, 0xb6, 0xba, 0xaf, 0x8c, 0x96, 0x19, 0x8f, 0x9f, 0xa7, 0x0e, 0x16, 0x1e, 0xb4, 0x9a,
    0xb8, 0xaa, 0x87, 0x2c, 0x54, 0x7c, 0xba, 0xa4, 0xb0, 0x8a, 0xb6, 0x91, 0x32, 0x56, 0x76, 0xb2,
    0x84, 0x94, 0xa4, 0xc8, 0x08, 0xcd, 0xd8, 0xb8, 0xb4, 0xb0, 0xf1, 0x99, 0x82, 0xa8, 0x2d, 0x55,
    0x7d, 0x98, 0xa8, 0x0e, 0x16, 0x1e, 0xa2, 0x2c, 0x54, 0x7c, 0x92, 0xa4, 0xf0, 0x2c, 0x50, 0x78,
    /* bank # 5 */
    0xf1, 0x84, 0xa8, 0x98, 0xc4, 0xcd, 0xfc, 0xd8, 0x0d, 0xdb, 0xa8, 0xfc, 0x2d, 0xf3, 0xd9, 0xba,
    0xa6, 0xf8, 0xda, 0xba, 0xa6, 0xde, 0xd8, 0xba, 0xb2, 0xb6, 0x86, 0x96, 0xa6, 0xd0, 0xf3, 0xc8,
    0x41, 0xda, 0xa6, 0xc8, 0xf8, 0xd8, 0xb0, 0xb4, 0xb8, 0x82, 0xa8, 0x92, 0xf5, 0x2c, 0x54, 0x88,
    0x98, 0xf1, 0x35, 0xd9, 0xf4, 0x18, 0xd8, 0xf1, 0xa2, 0xd0, 0xf8, 0xf9, 0xa8, 0x84, 0xd9, 0xc7,
    0xdf, 0xf8, 0xf8, 0x83, 0xc5, 0xda, 0xdf, 0x69, 0xdf, 0x83, 0xc1, 0xd8, 0xf4, 0x01, 0x14, 0xf1,
    0xa8, 0x82, 0x4e, 0xa8, 0x84, 0xf3, 0x11, 0xd1, 0x82, 0xf5, 0xd9, 0x92, 0x28, 0x97, 0x88, 0xf1,
    0x09, 0xf4, 0x1c, 0x1c, 0xd8, 0x84, 0xa8, 0xf3, 0xc0, 0xf9, 0xd1, 0xd9, 0x97, 0x82, 0xf1, 0x29,
    0xf4, 0x0d, 0xd8, 0xf3, 0xf9, 0xf9, 0xd1, 0xd9, 0x82, 0xf4, 0xc2, 0x03, 0xd8, 0xde, 0xdf, 0x1a,
    0xd8, 0xf1, 0xa2, 0xfa, 0xf9, 0xa8, 0x84, 0x98, 0xd9, 0xc7, 0xdf, 0xf8, 0xf8, 0xf8, 0x83, 0xc7,
    0xda, 0xdf, 0x69, 0xdf, 0xf8, 0x83, 0xc3, 0xd8, 0xf4, 0x01, 0x14, 0xf1, 0x98, 0xa8, 0x82, 0x2e,
    0xa8, 0x84, 0xf3, 0x11, 0xd1, 0x82, 0xf5, 0xd9, 0x92, 0x50, 0x97, 0x88, 0xf1, 0x09, 0xf4, 0x1c,
    0xd8, 0x84, 0xa8, 0xf3, 0xc0, 0xf8, 0xf9, 0xd1, 0xd9, 0x97, 0x82, 0xf1, 0x49, 0xf4, 0x0d, 0xd8,
    0xf3, 0xf9, 0xf9, 0xd1, 0xd9, 0x82, 0xf4, 0xc4, 0x03, 0xd8, 0xde, 0xdf, 0xd8, 0xf1, 0xad, 0x88,
    0x98, 0xcc, 0xa8, 0x09, 0xf9, 0xd9, 0x82, 0x92, 0xa8, 0xf5, 0x7c, 0xf1, 0x88, 0x3a, 0xcf, 0x94,
    0x4a, 0x6e, 0x98, 0xdb, 0x69, 0x31, 0xda, 0xad, 0xf2, 0xde, 0xf9, 0xd8, 0x87, 0x95, 0xa8, 0xf2,
    0x21, 0xd1, 0xda, 0xa5, 0xf9, 0xf4, 0x17, 0xd9, 0xf1, 0xae, 0x8e, 0xd0, 0xc0, 0xc3, 0xae, 0x82,
    /* bank # 6 */
    0xc6, 0x84, 0xc3, 0xa8, 0x85, 0x95, 0xc8, 0xa5, 0x88, 0xf2, 0xc0, 0xf1, 0xf4, 0x01, 0x0e, 0xf1,
    0x8e, 0x9e, 0xa8, 0xc6, 0x3e, 0x56, 0xf5, 0x54, 0xf1, 0x88, 0x72, 0xf4, 0x01, 0x15, 0xf1, 0x98,
    0x45, 0x85, 0x6e, 0xf5, 0x8e, 0x9e, 0x04, 0x88, 0xf1, 0x42, 0x98, 0x5a, 0x8e, 0x9e, 0x06, 0x88,
    0x69, 0xf4, 0x01, 0x1c, 0xf1, 0x98, 0x1e, 0x11, 0x08, 0xd0, 0xf5, 0x04, 0xf1, 0x1e, 0x97, 0x02,
    0x02, 0x98, 0x36, 0x25, 0xdb, 0xf9, 0xd9, 0x85, 0xa5, 0xf3, 0xc1, 0xda, 0x85, 0xa5, 0xf3, 0xdf,
    0xd8, 0x85, 0x95, 0xa8, 0xf3, 0x09, 0xda, 0xa5, 0xfa, 0xd8, 0x82, 0x92, 0xa8, 0xf5, 0x78, 0xf1,
    0x88, 0x1a, 0x84, 0x9f, 0x26, 0x88, 0x98, 0x21, 0xda, 0xf4, 0x1d, 0xf3, 0xd8, 0x87, 0x9f, 0x39,
    0xd1, 0xaf, 0xd9, 0xdf, 0xdf, 0xfb, 0xf9, 0xf4, 0x0c, 0xf3, 0xd8, 0xfa, 0xd0, 0xf8, 0xda, 0xf9,
    0xf9, 0xd0, 0xdf, 0xd9, 0xf9, 0xd8, 0xf4, 0x0b, 0xd8, 0xf3, 0x87, 0x9f, 0x39, 0xd1, 0xaf, 0xd9,
    0xdf, 0xdf, 0xf4, 0x1d, 0xf3, 0xd8, 0xfa, 0xfc, 0xa8, 0x69, 0xf9, 0xf9, 0xaf, 0xd0, 0xda, 0xde,
    0xfa, 0xd9, 0xf8, 0x8f, 0x9f, 0xa8, 0xf1, 0xcc, 0xf3, 0x98, 0xdb, 0x45, 0xd9, 0xaf, 0xdf, 0xd0,
    0xf8, 0xd8, 0xf1, 0x8f, 0x9f, 0xa8, 0xca, 0xf3, 0x88, 0x09, 0xda, 0xaf, 0x8f, 0xcb, 0xf8, 0xd8,
    0xf2, 0xad, 0x97, 0x8d, 0x0c, 0xd9, 0xa5, 0xdf, 0xf9, 0xba, 0xa6, 0xf3, 0xfa, 0xf4, 0x12, 0xf2,
    0xd8, 0x95, 0x0d, 0xd1, 0xd9, 0xba, 0xa6, 0xf3, 0xfa, 0xda, 0xa5, 0xf2, 0xc1, 0xba, 0xa6, 0xf3,
    0xdf, 0xd8, 0xf1, 0xba, 0xb2, 0xb6, 0x86, 0x96, 0xa6, 0xd0, 0xca, 0xf3, 0x49, 0xda, 0xa6, 0xcb,
    0xf8, 0xd8, 0xb0, 0xb4, 0xb8, 0xd8, 0xad, 0x84, 0xf2, 0xc0, 0xdf, 0xf1, 0x8f, 0xcb, 0xc3, 0xa8,
    /* bank # 7 */
    0xb2, 0xb6, 0x86, 0x96, 0xc8, 0xc1, 0xcb, 0xc3, 0xf3, 0xb0, 0xb4, 0x88, 0x98, 0xa8, 0x21, 0xdb,
    0x71, 0x8d, 0x9d, 0x71, 0x85, 0x95, 0x21, 0xd9, 0xad, 0xf2, 0xfa, 0xd8, 0x85, 0x97, 0xa8, 0x28,
    0xd9, 0xf4, 0x08, 0xd8, 0xf2, 0x8d, 0x29, 0xda, 0xf4, 0x05, 0xd9, 0xf2, 0x85, 0xa4, 0xc2, 0xf2,
    0xd8, 0xa8, 0x8d, 0x94, 0x01, 0xd1, 0xd9, 0xf4, 0x11, 0xf2, 0xd8, 0x87, 0x21, 0xd8, 0xf4, 0x0a,
    0xd8, 0xf2, 0x84, 0x98, 0xa8, 0xc8, 0x01, 0xd1, 0xd9, 0xf4, 0x11, 0xd8, 0xf3, 0xa4, 0xc8, 0xbb,
    0xaf, 0xd0, 0xf2, 0xde, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xd8, 0xf1, 0xb8, 0xf6,
    0xb5, 0xb9, 0xb0, 0x8a, 0x95, 0xa3, 0xde, 0x3c, 0xa3, 0xd9, 0xf8, 0xd8, 0x5c, 0xa3, 0xd9, 0xf8,
    0xd8, 0x7c, 0xa3, 0xd9, 0xf8, 0xd8, 0xf8, 0xf9, 0xd1, 0xa5, 0xd9, 0xdf, 0xda, 0xfa, 0xd8, 0xb1,
    0x85, 0x30, 0xf7, 0xd9, 0xde, 0xd8, 0xf8, 0x30, 0xad, 0xda, 0xde, 0xd8, 0xf2, 0xb4, 0x8c, 0x99,
    0xa3, 0x2d, 0x55, 0x7d, 0xa0, 0x83, 0xdf, 0xdf, 0xdf, 0xb5, 0x91, 0xa0, 0xf6, 0x29, 0xd9, 0xfb,
    0xd8, 0xa0, 0xfc, 0x29, 0xd9, 0xfa, 0xd8, 0xa0, 0xd0, 0x51, 0xd9, 0xf8, 0xd8, 0xfc, 0x51, 0xd9,
    0xf9, 0xd8, 0x79, 0xd9, 0xfb, 0xd8, 0xa0, 0xd0, 0xfc, 0x79, 0xd9, 0xfa, 0xd8, 0xa1, 0xf9, 0xf9,
    0xf9, 0xf9, 0xf9, 0xa0, 0xda, 0xdf, 0xdf, 0xdf, 0xd8, 0xa1, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xac,
    0xde, 0xf8, 0xad, 0xde, 0x83, 0x93, 0xac, 0x2c, 0x54, 0x7c, 0xf1, 0xa8, 0xdf, 0xdf, 0xdf, 0xf6,
    0x9d, 0x2c, 0xda, 0xa0, 0xdf, 0xd9, 0xfa, 0xdb, 0x2d, 0xf8, 0xd8, 0xa8, 0x50, 0xda, 0xa0, 0xd0,
    0xde, 0xd9, 0xd0, 0xf8, 0xf8, 0xf8, 0xdb, 0x55, 0xf8, 0xd8, 0xa8, 0x78, 0xda, 0xa0, 0xd0, 0xdf,
    /* bank # 8 */
    0xd9, 0xd0, 0xfa, 0xf8, 0xf8, 0xf8, 0xf8, 0xdb, 0x7d, 0xf8, 0xd8, 0x9c, 0xa8, 0x8c, 0xf5, 0x30,
    0xdb, 0x38, 0xd9, 0xd0, 0xde, 0xdf, 0xa0, 0xd0, 0xde, 0xdf, 0xd8, 0xa8, 0x48, 0xdb, 0x58, 0xd9,
    0xdf, 0xd0, 0xde, 0xa0, 0xdf, 0xd0, 0xde, 0xd8, 0xa8, 0x68, 0xdb, 0x70, 0xd9, 0xdf, 0xdf, 0xa0,
    0xdf, 0xdf, 0xd8, 0xf1, 0xa8, 0x88, 0x90, 0x2c, 0x54, 0x7c, 0x98, 0xa8, 0xd0, 0x5c, 0x38, 0xd1,
    0xda, 0xf2, 0xae, 0x8c, 0xdf, 0xf9, 0xd8, 0xb0, 0x87, 0xa8, 0xc1, 0xc1, 0xb1, 0x88, 0xa8, 0xc6,
    0xf9, 0xf9, 0xda, 0x36, 0xd8, 0xa8, 0xf9, 0xda, 0x36, 0xd8, 0xa8, 0xf9, 0xda, 0x36, 0xd8, 0xa8,
    0xf9, 0xda, 0x36, 0xd8, 0xa8, 0xf9, 0xda, 0x36, 0xd8, 0xf7, 0x8d, 0x9d, 0xad, 0xf8, 0x18, 0xda,
    0xf2, 0xae, 0xdf, 0xd8, 0xf7, 0xad, 0xfa, 0x30, 0xd9, 0xa4, 0xde, 0xf9, 0xd8, 0xf2, 0xae, 0xde,
    0xfa, 0xf9, 0x83, 0xa7, 0xd9, 0xc3, 0xc5, 0xc7, 0xf1, 0x88, 0x9b, 0xa7, 0x7a, 0xad, 0xf7, 0xde,
    0xdf, 0xa4, 0xf8, 0x84, 0x94, 0x08, 0xa7, 0x97, 0xf3, 0x00, 0xae, 0xf2, 0x98, 0x19, 0xa4, 0x88,
    0xc6, 0xa3, 0x94, 0x88, 0xf6, 0x32, 0xdf, 0xf2, 0x83, 0x93, 0xdb, 0x09, 0xd9, 0xf2, 0xaa, 0xdf,
    0xd8, 0xd8, 0xae, 0xf8, 0xf9, 0xd1, 0xda, 0xf3, 0xa4, 0xde, 0xa7, 0xf1, 0x88, 0x9b, 0x7a, 0xd8,
    0xf3, 0x84, 0x94, 0xae, 0x19, 0xf9, 0xda, 0xaa, 0xf1, 0xdf, 0xd8, 0xa8, 0x81, 0xc0, 0xc3, 0xc5,
    0xc7, 0xa3, 0x92, 0x83, 0xf6, 0x28, 0xad, 0xde, 0xd9, 0xf8, 0xd8, 0xa3, 0x50, 0xad, 0xd9, 0xf8,
    0xd8, 0xa3, 0x78, 0xad, 0xd9, 0xf8, 0xd8, 0xf8, 0xf9, 0xd1, 0xa1, 0xda, 0xde, 0xc3, 0xc5, 0xc7,
    0xd8, 0xa1, 0x81, 0x94, 0xf8, 0x18, 0xf2, 0xb0, 0x89, 0xac, 0xc3, 0xc5, 0xc7, 0xf1, 0xd8, 0xb8,
    /* bank # 9 */
    0xb4, 0xb0, 0x97, 0x86, 0xa8, 0x31, 0x9b, 0x06, 0x99, 0x07, 0xab, 0x97, 0x28, 0x88, 0x9b, 0xf0,
    0x0c, 0x20, 0x14, 0x40, 0xb0, 0xb4, 0xb8, 0xf0, 0xa8, 0x8a, 0x9a, 0x28, 0x50, 0x78, 0xb7, 0x9b,
    0xa8, 0x29, 0x51, 0x79, 0x24, 0x70, 0x59, 0x44, 0x69, 0x38, 0x64, 0x48, 0x31, 0xf1, 0xbb, 0xab,
    0x88, 0x00, 0x2c, 0x54, 0x7c, 0xf0, 0xb3, 0x8b, 0xb8, 0xa8, 0x04, 0x28, 0x50, 0x78, 0xf1, 0xb0,
    0x88, 0xb4, 0x97, 0x26, 0xa8, 0x59, 0x98, 0xbb, 0xab, 0xb3, 0x8b, 0x02, 0x26, 0x46, 0x66, 0xb0,
    0xb8, 0xf0, 0x8a, 0x9c, 0xa8, 0x29, 0x51, 0x79, 0x8b, 0x29, 0x51, 0x79, 0x8a, 0x24, 0x70, 0x59,
    0x8b, 0x20, 0x58, 0x71, 0x8a, 0x44, 0x69, 0x38, 0x8b, 0x39, 0x40, 0x68, 0x8a, 0x64, 0x48, 0x31,
    0x8b, 0x30, 0x49, 0x60, 0x88, 0xf1, 0xac, 0x00, 0x2c, 0x54, 0x7c, 0xf0, 0x8c, 0xa8, 0x04, 0x28,
    0x50, 0x78, 0xf1, 0x88, 0x97, 0x26, 0xa8, 0x59, 0x98, 0xac, 0x8c, 0x02, 0x26, 0x46, 0x66, 0xf0,
    0x89, 0x9c, 0xa8, 0x29, 0x51, 0x79, 0x24, 0x70, 0x59, 0x44, 0x69, 0x38, 0x64, 0x48, 0x31, 0xa9,
    0x88, 0x09, 0x20, 0x59, 0x70, 0xab, 0x11, 0x38, 0x40, 0x69, 0xa8, 0x19, 0x31, 0x48, 0x60, 0x8c,
    0xa8, 0x3c, 0x41, 0x5c, 0x20, 0x7c, 0x00, 0xf1, 0x87, 0x98, 0x19, 0x86, 0xa8, 0x6e, 0x76, 0x7e,
    0xa9, 0x99, 0x88, 0x2d, 0x55, 0x7d, 0xd8, 0xb1, 0xb5, 0xb9, 0xa3, 0xdf, 0xdf, 0xdf, 0xae, 0xd0,
    0xdf, 0xaa, 0xd0, 0xde, 0xf2, 0xab, 0xf8, 0xf9, 0xd9, 0xb0, 0x87, 0xc4, 0xaa, 0xf1, 0xdf, 0xdf,
    0xbb, 0xaf, 0xdf, 0xdf, 0xb9, 0xd8, 0xb1, 0xf1, 0xa3, 0x97, 0x8e, 0x60, 0xdf, 0xb0, 0x84, 0xf2,
    0xc8, 0xf8, 0xf9, 0xd9, 0xde, 0xd8, 0x93, 0x85, 0xf1, 0x4a, 0xb1, 0x83, 0xa3, 0x08, 0xb5, 0x83,
    /* bank # 10 */
    0x9a, 0x08, 0x10, 0xb7, 0x9f, 0x10, 0xd8, 0xf1, 0xb0, 0xba, 0xae, 0xb0, 0x8a, 0xc2, 0xb2, 0xb6,
    0x8e, 0x9e, 0xf1, 0xfb, 0xd9, 0xf4, 0x1d, 0xd8, 0xf9, 0xd9, 0x0c, 0xf1, 0xd8, 0xf8, 0xf8, 0xad,
    0x61, 0xd9, 0xae, 0xfb, 0xd8, 0xf4, 0x0c, 0xf1, 0xd8, 0xf8, 0xf8, 0xad, 0x19, 0xd9, 0xae, 0xfb,
    0xdf, 0xd8, 0xf4, 0x16, 0xf1, 0xd8, 0xf8, 0xad, 0x8d, 0x61, 0xd9, 0xf4, 0xf4, 0xac, 0xf5, 0x9c,
    0x9c, 0x8d, 0xdf, 0x2b, 0xba, 0xb6, 0xae, 0xfa, 0xf8, 0xf4, 0x0b, 0xd8, 0xf1, 0xae, 0xd0, 0xf8,
    0xad, 0x51, 0xda, 0xae, 0xfa, 0xf8, 0xf1, 0xd8, 0xb9, 0xb1, 0xb6, 0xa3, 0x83, 0x9c, 0x08, 0xb9,
    0xb1, 0x83, 0x9a, 0xb5, 0xaa, 0xc0, 0xfd, 0x30, 0x83, 0xb7, 0x9f, 0x10, 0xb5, 0x8b, 0x93, 0xf2,
    0x02, 0x02, 0xd1, 0xab, 0xda, 0xde, 0xd8, 0xf1, 0xb0, 0x80, 0xba, 0xab, 0xc0, 0xc3, 0xb2, 0x84,
    0xc1, 0xc3, 0xd8, 0xb1, 0xb9, 0xf3, 0x8b, 0xa3, 0x91, 0xb6, 0x09, 0xb4, 0xd9, 0xab, 0xde, 0xb0,
    0x87, 0x9c, 0xb9, 0xa3, 0xdd, 0xf1, 0xb3, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0xb0, 0x87, 0xa3, 0xa3,
    0xa3, 0xa3, 0xb2, 0x8b, 0xb6, 0x9b, 0xf2, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3,
    0xa3, 0xf1, 0xb0, 0x87, 0xb5, 0x9a, 0xa3, 0xf3, 0x9b, 0xa3, 0xa3, 0xdc, 0xba, 0xac, 0xdf, 0xb9,
    0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3,
    0xd8, 0xd8, 0xd8, 0xbb, 0xb3, 0xb7, 0xf1, 0xaa, 0xf9, 0xda, 0xff, 0xd9, 0x80, 0x9a, 0xaa, 0x28,
    0xb4, 0x80, 0x98, 0xa7, 0x20, 0xb7, 0x97, 0x87, 0xa8, 0x66, 0x88, 0xf0, 0x79, 0x51, 0xf1, 0x90,
    0x2c, 0x87, 0x0c, 0xa7, 0x81, 0x97, 0x62, 0x93, 0xf0, 0x71, 0x71, 0x60, 0x85, 0x94, 0x01, 0x29,
    /* bank # 11 */
    0x51, 0x79, 0x90, 0xa5, 0xf1, 0x28, 0x4c, 0x6c, 0x87, 0x0c, 0x95, 0x18, 0x85, 0x78, 0xa3, 0x83,
    0x90, 0x28, 0x4c, 0x6c, 0x88, 0x6c, 0xd8, 0xf3, 0xa2, 0x82, 0x00, 0xf2, 0x10, 0xa8, 0x92, 0x19,
    0x80, 0xa2, 0xf2, 0xd9, 0x26, 0xd8, 0xf1, 0x88, 0xa8, 0x4d, 0xd9, 0x48, 0xd8, 0x96, 0xa8, 0x39,
    0x80, 0xd9, 0x3c, 0xd8, 0x95, 0x80, 0xa8, 0x39, 0xa6, 0x86, 0x98, 0xd9, 0x2c, 0xda, 0x87, 0xa7,
    0x2c, 0xd8, 0xa8, 0x89, 0x95, 0x19, 0xa9, 0x80, 0xd9, 0x38, 0xd8, 0xa8, 0x89, 0x39, 0xa9, 0x80,
    0xda, 0x3c, 0xd8, 0xa8, 0x2e, 0xa8, 0x39, 0x90, 0xd9, 0x0c, 0xd8, 0xa8, 0x95, 0x31, 0x98, 0xd9,
    0x0c, 0xd8, 0xa8, 0x09, 0xd9, 0xff, 0xd8, 0x01, 0xda, 0xff, 0xd8, 0x95, 0x39, 0xa9, 0xda, 0x26,
    0xff, 0xd8, 0x90, 0xa8, 0x0d, 0x89, 0x99, 0xa8, 0x10, 0x80, 0x98, 0x21, 0xda, 0x2e, 0xd8, 0x89,
    0x99, 0xa8, 0x31, 0x80, 0xda, 0x2e, 0xd8, 0xa8, 0x86, 0x96, 0x31, 0x80, 0xda, 0x2e, 0xd8, 0xa8,
    0x87, 0x31, 0x80, 0xda, 0x2e, 0xd8, 0xa8, 0x82, 0x92, 0xf3, 0x41, 0x80, 0xf1, 0xd9, 0x2e, 0xd8,
    0xa8, 0x82, 0xf3, 0x19, 0x80, 0xf1, 0xd9, 0x2e, 0xd8, 0x82, 0xac, 0xf3, 0xc0, 0xa2, 0x80, 0x22,
    0xf1, 0xa6, 0x2e, 0xa7, 0x2e, 0xa9, 0x22, 0x98, 0xa8, 0x29, 0xda, 0xac, 0xde, 0xff, 0xd8, 0xa2,
    0xf2, 0x2a, 0xf1, 0xa9, 0x2e, 0x82, 0x92, 0xa8, 0xf2, 0x31, 0x80, 0xa6, 0x96, 0xf1, 0xd9, 0x00,
    0xac, 0x8c, 0x9c, 0x0c, 0x30, 0xac, 0xde, 0xd0, 0xde, 0xff, 0xd8, 0x8c, 0x9c, 0xac, 0xd0, 0x10,
    0xac, 0xde, 0x80, 0x92, 0xa2, 0xf2, 0x4c, 0x82, 0xa8, 0xf1, 0xca, 0xf2, 0x35, 0xf1, 0x96, 0x88,
    0xa6, 0xd9, 0x00, 0xd8, 0xf1, 0xff
};

static const unsigned short sStartAddress = 0x0400;

#ifdef AK89xx_SECONDARY
static int setup_compass(void);
#define MAX_COMPASS_SAMPLE_RATE (100)
#endif

static int shmem_dump(char *src, int size);
static int gyro_spi_write(uint8_t addr, unsigned short size, uint8_t *data);
static int gyro_spi_read(uint8_t addr, unsigned short size, uint8_t *data);
static int tx_data(int fd, uint8_t *rx_buff, uint8_t *tx_buff, int num, int pksz, int maxsz);
#define i2c_write(a, b, c, d)  gyro_spi_write(b, c, d)
#define i2c_read(a, b, c, d)  gyro_spi_read(b, c, d)
#define delay_ms(a)             usleep(a*1000)
#define get_curtime(a)             clock_gettime(CLOCK_REALTIME, a)
#define log_i                     printf
#define log_e                     printf
/**
 *  @brief      Enable latched interrupts.
 *  Any MPU register will clear the interrupt.
 *  @param[in]  enable  1 to enable, 0 to disable.
 *  @return     0 if successful.
 */
static int mpu_set_int_latched(unsigned char enable)
{
    unsigned char tmp;
    if (st->chip_cfg.latched_int == enable)
        return 0;

    if (enable)
        tmp = BIT_LATCH_EN | BIT_ANY_RD_CLR;
    else
        tmp = 0;
    if (st->chip_cfg.bypass_mode)
        tmp |= BIT_BYPASS_EN;
    if (st->chip_cfg.active_low_int)
        tmp |= BIT_ACTL;
    if (i2c_write(st->hw->addr, st->reg->int_pin_cfg, 1, &tmp))
        return -1;
    st->chip_cfg.latched_int = enable;
    return 0;
}

/**
 *  @brief      Turn specific sensors on/off.
 *  @e sensors can contain a combination of the following flags:
 *  \n INV_X_GYRO, INV_Y_GYRO, INV_Z_GYRO
 *  \n INV_XYZ_GYRO
 *  \n INV_XYZ_ACCEL
 *  \n INV_XYZ_COMPASS
 *  @param[in]  sensors    Mask of sensors to wake.
 *  @return     0 if successful.
 */
static int mpu_set_sensors(unsigned char sensors)
{
    unsigned char data=0;
    

    if (sensors & INV_XYZ_GYRO) {
        data = INV_CLK_PLL;
    } else if (sensors != 0) {
        data = 0;
    } else {
        data = BIT_SLEEP;
    }

    printf("mpu_set_sensors(1) data = %d, sensors = %d \n", data, sensors);
    
    if (i2c_write(st->hw->addr, st->reg->pwr_mgmt_1, 1, &data)) {
        st->chip_cfg.sensors = 0;
        return -1;
    }
    
    st->chip_cfg.clk_src = data & ~BIT_SLEEP;

    data = 0;
    if (!(sensors & INV_X_GYRO)) data |= BIT_STBY_XG;
    if (!(sensors & INV_Y_GYRO)) data |= BIT_STBY_YG;
    if (!(sensors & INV_Z_GYRO)) data |= BIT_STBY_ZG;
    if (!(sensors & INV_XYZ_ACCEL)) data |= BIT_STBY_XYZA;
    
    if (i2c_write(st->hw->addr, st->reg->pwr_mgmt_2, 1, &data)) {
        st->chip_cfg.sensors = 0;
        return -1;
    }

    /* Latched interrupts only used in LP accel mode. */
    if (sensors && (sensors != INV_XYZ_ACCEL)) mpu_set_int_latched(0);

    st->chip_cfg.sensors = sensors;
    st->chip_cfg.lp_accel_mode = 0;
    
    usleep(50000);
    return 0;
}

/**
 *  @brief      Enable/disable data ready interrupt.
 *  If the DMP is on, the DMP interrupt is enabled. Otherwise, the data ready
 *  interrupt is used.
 *  @param[in]  enable      1 to enable interrupt.
 *  @return     0 if successful.
 */
static int set_int_enable(unsigned char enable)
{
    unsigned char tmp;

    if (st->chip_cfg.dmp_on) {
        if (enable)
            tmp = BIT_DMP_INT_EN;
        else
            tmp = 0x00;
        if (i2c_write(st->hw->addr, st->reg->int_enable, 1, &tmp))
            return -1;
        st->chip_cfg.int_enable = tmp;
    } else {
        if (!st->chip_cfg.sensors)
            return -1;
        if (enable && st->chip_cfg.int_enable)
            return 0;
        if (enable)
            tmp = BIT_DATA_RDY_EN;
        else
            tmp = 0x00;
        if (i2c_write(st->hw->addr, st->reg->int_enable, 1, &tmp))
            return -1;
        st->chip_cfg.int_enable = tmp;
    }
    return 0;
}

/**
 *  @brief  Reset FIFO read/write pointers.
 *  @return 0 if successful.
 */
static int mpu_reset_fifo(void)
{
    unsigned char data;

    printf("mpu_reset_fifo()\n");

    if (!(st->chip_cfg.sensors))
        return -1;

    data = 0;
    if (i2c_write(st->hw->addr, st->reg->int_enable, 1, &data)) return -1;
    if (i2c_write(st->hw->addr, st->reg->fifo_en, 1, &data)) return -1;
    if (i2c_write(st->hw->addr, st->reg->user_ctrl, 1, &data)) return -1;

    if (st->chip_cfg.dmp_on) {
        data = BIT_FIFO_RST | BIT_DMP_RST;
        if (i2c_write(st->hw->addr, st->reg->user_ctrl, 1, &data)) return -1;
        
        delay_ms(50);

        data = BIT_DMP_EN | BIT_FIFO_EN;
        if (st->chip_cfg.sensors & INV_XYZ_COMPASS) {
            data |= BIT_AUX_IF_EN;
        }
        
        if (i2c_write(st->hw->addr, st->reg->user_ctrl, 1, &data)) return -1;
        
        if (st->chip_cfg.int_enable) {
            data = BIT_DMP_INT_EN;
        } else {
            data = 0;
        }
        
        if (i2c_write(st->hw->addr, st->reg->int_enable, 1, &data)) return -1;

        data = 0;
        if (i2c_write(st->hw->addr, st->reg->fifo_en, 1, &data)) return -1;
    }
    else {
        data = BIT_FIFO_RST;
        if (i2c_write(st->hw->addr, st->reg->user_ctrl, 1, &data)) return -1;

        if (st->chip_cfg.bypass_mode || !(st->chip_cfg.sensors & INV_XYZ_COMPASS)) {
            data = BIT_FIFO_EN;
        } else {
            data = BIT_FIFO_EN | BIT_AUX_IF_EN;
        }

        if (i2c_write(st->hw->addr, st->reg->user_ctrl, 1, &data)) return -1;

        usleep(50000);

        if (st->chip_cfg.int_enable) {
            data = BIT_DATA_RDY_EN;
        } else {
            data = 0;
        }
        
        if (i2c_write(st->hw->addr, st->reg->int_enable, 1, &data)) return -1;

        data = st->chip_cfg.fifo_enable;
        if (i2c_write(st->hw->addr, st->reg->fifo_en, 1, &data)) return -1;
    }
    return 0;
}

/**
 *  @brief      Select which sensors are pushed to FIFO.
 *  @e sensors can contain a combination of the following flags:
 *  \n INV_X_GYRO, INV_Y_GYRO, INV_Z_GYRO
 *  \n INV_XYZ_GYRO
 *  \n INV_XYZ_ACCEL
 *  @param[in]  sensors Mask of sensors to push to FIFO.
 *  @return     0 if successful.
 */
static int mpu_configure_fifo(unsigned char sensors)
{
    unsigned char prev;
    int result = 0;

    /* Compass data isn't going into the FIFO. Stop trying. */
    sensors &= ~INV_XYZ_COMPASS;

    if (st->chip_cfg.dmp_on)
        return 0;
    else {
        if (!(st->chip_cfg.sensors))
            return -1;
        prev = st->chip_cfg.fifo_enable;
        st->chip_cfg.fifo_enable = sensors & st->chip_cfg.sensors;
        if (st->chip_cfg.fifo_enable != sensors)
            /* You're not getting what you asked for. Some sensors are
             * asleep.
             */
            result = -1;
        else
            result = 0;
        if (sensors || st->chip_cfg.lp_accel_mode)
            set_int_enable(1);
        else
            set_int_enable(0);
        if (sensors) {
            if (mpu_reset_fifo()) {
                st->chip_cfg.fifo_enable = prev;
                return -1;
            }
        }
    }

    return result;
}

/**
 *  @brief      Set digital low pass filter.
 *  The following LPF settings are supported: 188, 98, 42, 20, 10, 5.
 *  @param[in]  lpf Desired LPF setting.
 *  @return     0 if successful.
 */
static int mpu_set_lpf(unsigned short lpf)
{
    unsigned char data;

    if (!(st->chip_cfg.sensors))
        return -1;

    if (lpf >= 188)
        data = INV_FILTER_188HZ;
    else if (lpf >= 98)
        data = INV_FILTER_98HZ;
    else if (lpf >= 42)
        data = INV_FILTER_42HZ;
    else if (lpf >= 20)
        data = INV_FILTER_20HZ;
    else if (lpf >= 10)
        data = INV_FILTER_10HZ;
    else
        data = INV_FILTER_5HZ;

    if (st->chip_cfg.lpf == data)
        return 0;

    if (i2c_write(st->hw->addr, st->reg->lpf, 1, &data))
        return -1;

#ifdef MPU6500 //MPU6500 accel/gyro dlpf separately
    data = BIT_FIFO_SIZE_1024 | data;
    if (i2c_write(st->hw->addr, st->reg->accel_cfg2, 1, &data))
            return -1;
#endif

    st->chip_cfg.lpf = data;
    return 0;
}

/**
 *  @brief      Enter low-power accel-only mode.
 *  In low-power accel mode, the chip goes to sleep and only wakes up to sample
 *  the accelerometer at one of the following frequencies:
 *  \n MPU6050: 1.25Hz, 5Hz, 20Hz, 40Hz
 *  \n MPU6500: 0.24Hz, 0.49Hz, 0.98Hz, 1.95Hz, 3.91Hz, 7.81Hz, 15.63Hz, 31.25Hz, 62.5Hz, 125Hz, 250Hz, 500Hz
 *  \n If the requested rate is not one listed above, the device will be set to
 *  the next highest rate. Requesting a rate above the maximum supported
 *  frequency will result in an error.
 *  \n To select a fractional wake-up frequency, round down the value passed to
 *  @e rate.
 *  @param[in]  rate        Minimum sampling rate, or zero to disable LP
 *                          accel mode.
 *  @return     0 if successful.
 */
static int mpu_lp_accel_mode(unsigned short rate)
{
    unsigned char tmp[2];

#if defined MPU6500
    unsigned char data;
#endif

    if (!rate) {
        mpu_set_int_latched(0);
        tmp[0] = 0;
        tmp[1] = BIT_STBY_XYZG;
        if (i2c_write(st->hw->addr, st->reg->pwr_mgmt_1, 2, tmp))
            return -1;
        st->chip_cfg.lp_accel_mode = 0;
        return 0;
    }
    /* For LP accel, we automatically configure the hardware to produce latched
     * interrupts. In LP accel mode, the hardware cycles into sleep mode before
     * it gets a chance to deassert the interrupt pin; therefore, we shift this
     * responsibility over to the MCU.
     *
     * Any register read will clear the interrupt.
     */
    mpu_set_int_latched(1);
    
#if defined MPU6050
    tmp[0] = BIT_LPA_CYCLE;
    if (rate == 1) {
        tmp[1] = INV_LPA_1_25HZ;
        mpu_set_lpf(5);
    } else if (rate <= 5) {
        tmp[1] = INV_LPA_5HZ;
        mpu_set_lpf(5);
    } else if (rate <= 20) {
        tmp[1] = INV_LPA_20HZ;
        mpu_set_lpf(10);
    } else {
        tmp[1] = INV_LPA_40HZ;
        mpu_set_lpf(20);
    }
    tmp[1] = (tmp[1] << 6) | BIT_STBY_XYZG;
    if (i2c_write(st->hw->addr, st->reg->pwr_mgmt_1, 2, tmp))
        return -1;
#elif defined MPU6500
    /* Set wake frequency. */
    if (rate == 1)
    	data = INV_LPA_0_98HZ;
    else if (rate == 2)
    	data = INV_LPA_1_95HZ;
    else if (rate <= 5)
    	data = INV_LPA_3_91HZ;
    else if (rate <= 10)
    	data = INV_LPA_7_81HZ;
    else if (rate <= 20)
    	data = INV_LPA_15_63HZ;
    else if (rate <= 40)
    	data = INV_LPA_31_25HZ;
    else if (rate <= 70)
    	data = INV_LPA_62_50HZ;
    else if (rate <= 125)
    	data = INV_LPA_125HZ;
    else if (rate <= 250)
    	data = INV_LPA_250HZ;
    else
    	data = INV_LPA_500HZ;
        
    if (i2c_write(st->hw->addr, st->reg->lp_accel_odr, 1, &data))
        return -1;
    
    if (i2c_read(st->hw->addr, st->reg->accel_cfg2, 1, &data))
        return -1;
        
    data = data | BIT_ACCL_FC_B;
    if (i2c_write(st->hw->addr, st->reg->accel_cfg2, 1, &data))
            return -1;
            
    data = BIT_LPA_CYCLE;
    if (i2c_write(st->hw->addr, st->reg->pwr_mgmt_1, 1, &data))
        return -1;
#endif
    st->chip_cfg.sensors = INV_XYZ_ACCEL;
    st->chip_cfg.clk_src = 0;
    st->chip_cfg.lp_accel_mode = 1;
    mpu_configure_fifo(0);

    return 0;
}

/**
 *  @brief      Set compass sampling rate.
 *  The compass on the auxiliary I2C bus is read by the MPU hardware at a
 *  maximum of 100Hz. The actual rate can be set to a fraction of the gyro
 *  sampling rate.
 *
 *  \n WARNING: The new rate may be different than what was requested. Call
 *  mpu_get_compass_sample_rate to check the actual setting.
 *  @param[in]  rate    Desired compass sampling rate (Hz).
 *  @return     0 if successful.
 */
static int mpu_set_compass_sample_rate(unsigned short rate)
{
#ifdef AK89xx_SECONDARY
    unsigned char div;
    if (!rate || rate > st->chip_cfg.sample_rate || rate > MAX_COMPASS_SAMPLE_RATE)
        return -1;

    div = st->chip_cfg.sample_rate / rate - 1;
    if (i2c_write(st->hw->addr, st->reg->s4_ctrl, 1, &div))
        return -1;
    st->chip_cfg.compass_sample_rate = st->chip_cfg.sample_rate / (div + 1);
    return 0;
#else
    return -1;
#endif
}

/**
 *  @brief      Set sampling rate.
 *  Sampling rate must be between 4Hz and 1kHz.
 *  @param[in]  rate    Desired sampling rate (Hz).
 *  @return     0 if successful.
 */
static int mpu_set_sample_rate(unsigned short rate)
{
    unsigned char data;
    unsigned short minRate=0;
    
    if (!(st->chip_cfg.sensors))
        return -1;

    if (st->chip_cfg.dmp_on)
        return -1;
    else {
        if (st->chip_cfg.lp_accel_mode) {
            if (rate && (rate <= 40)) {
                /* Just stay in low-power accel mode. */
                mpu_lp_accel_mode(rate);
                return 0;
            }
            /* Requested rate exceeds the allowed frequencies in LP accel mode,
             * switch back to full-power mode.
             */
            mpu_lp_accel_mode(0);
        }
        if (rate < 4)
            rate = 4;
        else if (rate > 1000)
            rate = 1000;

        data = 1000 / rate - 1;
        if (i2c_write(st->hw->addr, st->reg->rate_div, 1, &data))
            return -1;

        st->chip_cfg.sample_rate = 1000 / (1 + data);

#ifdef AK89xx_SECONDARY
        if (st->chip_cfg.compass_sample_rate > MAX_COMPASS_SAMPLE_RATE) {
            minRate = MAX_COMPASS_SAMPLE_RATE;
        } else {
            minRate = st->chip_cfg.compass_sample_rate;
        }
        mpu_set_compass_sample_rate(minRate);
#endif

        /* Automatically set LPF to 1/2 sampling rate. */
        mpu_set_lpf(st->chip_cfg.sample_rate >> 1);
        return 0;
    }
}

/**
 *  @brief      Get sampling rate.
 *  @param[out] rate    Current sampling rate (Hz).
 *  @return     0 if successful.
 */
static int mpu_get_sample_rate(unsigned short *rate)
{
    if (st->chip_cfg.dmp_on)
        return -1;
    else
        rate[0] = st->chip_cfg.sample_rate;
    return 0;
}

/**
 *  @brief      Get the gyro full-scale range.
 *  @param[out] fsr Current full-scale range.
 *  @return     0 if successful.
 */
static int mpu_get_gyro_fsr(unsigned short *fsr)
{
    switch (st->chip_cfg.gyro_fsr) {
    case INV_FSR_250DPS:
        fsr[0] = 250;
        break;
    case INV_FSR_500DPS:
        fsr[0] = 500;
        break;
    case INV_FSR_1000DPS:
        fsr[0] = 1000;
        break;
    case INV_FSR_2000DPS:
        fsr[0] = 2000;
        break;
    default:
        fsr[0] = 0;
        break;
    }
    return 0;
}

/**
 *  @brief      Get the accel full-scale range.
 *  @param[out] fsr Current full-scale range.
 *  @return     0 if successful.
 */
static int mpu_get_accel_fsr(unsigned char *fsr)
{
    switch (st->chip_cfg.accel_fsr) {
    case INV_FSR_2G:
        fsr[0] = 2;
        break;
    case INV_FSR_4G:
        fsr[0] = 4;
        break;
    case INV_FSR_8G:
        fsr[0] = 8;
        break;
    case INV_FSR_16G:
        fsr[0] = 16;
        break;
    default:
        return -1;
    }
    if (st->chip_cfg.accel_half)
        fsr[0] <<= 1;
    return 0;
}

/**
 *  @brief      Write to the DMP memory.
 *  This function prevents I2C writes past the bank boundaries. The DMP memory
 *  is only accessible when the chip is awake.
 *  @param[in]  mem_addr    Memory location (bank << 8 | start address)
 *  @param[in]  length      Number of bytes to write.
 *  @param[in]  data        Bytes to write to memory.
 *  @return     0 if successful.
 */
static int mpu_write_mem(unsigned short mem_addr, unsigned short length,
        unsigned char *data)
{
    unsigned char tmp[2];

    if (!data)
        return -1;
    if (!st->chip_cfg.sensors)
        return -1;

    tmp[0] = (unsigned char)(mem_addr >> 8);
    tmp[1] = (unsigned char)(mem_addr & 0xFF);

    /* Check bank boundaries. */
    if (tmp[1] + length > st->hw->bank_size)
        return -1;

    if (i2c_write(st->hw->addr, st->reg->bank_sel, 2, tmp))
        return -1;
    if (i2c_write(st->hw->addr, st->reg->mem_r_w, length, data))
        return -1;
    return 0;
}

/**
 *  @brief      Read from the DMP memory.
 *  This function prevents I2C reads past the bank boundaries. The DMP memory
 *  is only accessible when the chip is awake.
 *  @param[in]  mem_addr    Memory location (bank << 8 | start address)
 *  @param[in]  length      Number of bytes to read.
 *  @param[out] data        Bytes read from memory.
 *  @return     0 if successful.
 */
static int mpu_read_mem(unsigned short mem_addr, unsigned short length,
        unsigned char *data)
{
    unsigned char tmp[2];

    if (!data)
        return -1;
    if (!st->chip_cfg.sensors)
        return -1;

    tmp[0] = (unsigned char)(mem_addr >> 8);
    tmp[1] = (unsigned char)(mem_addr & 0xFF);

    /* Check bank boundaries. */
    if (tmp[1] + length > st->hw->bank_size)
        return -1;

    if (i2c_write(st->hw->addr, st->reg->bank_sel, 2, tmp))
        return -1;
    if (i2c_read(st->hw->addr, st->reg->mem_r_w, length, data))
        return -1;
    return 0;
}

/**
 *  @brief      Load and verify DMP image.
 *  @param[in]  length      Length of DMP image.
 *  @param[in]  firmware    DMP code.
 *  @param[in]  start_addr  Starting address of DMP code memory.
 *  @param[in]  sample_rate Fixed sampling rate used when DMP is enabled.
 *  @return     0 if successful.
 */
static int mpu_load_firmware(unsigned short length, const unsigned char *firmware,
    unsigned short start_addr, unsigned short sample_rate)
{
    unsigned short ii;
    unsigned short this_write, min_len;
    /* Must divide evenly into st->hw->bank_size to avoid bank crossings. */
#define LOAD_CHUNK  (16)
    unsigned char cur[LOAD_CHUNK], tmp[2];

    if (st->chip_cfg.dmp_loaded)
        /* DMP should only be loaded once. */
        return -1;

    if (!firmware)
        return -1;
    for (ii = 0; ii < length; ii += this_write) {
    
        if (LOAD_CHUNK > (length - ii)) {
            min_len = (length - ii);
        } else {
            min_len = LOAD_CHUNK;
        }
        this_write = min_len;
        if (mpu_write_mem(ii, this_write, (unsigned char*)&firmware[ii]))
            return -1;
        if (mpu_read_mem(ii, this_write, cur))
            return -1;
        if (memcmp(firmware+ii, cur, this_write))
         {   
            return -2;
         }
    }

    /* Set program start address. */
    tmp[0] = start_addr >> 8;
    tmp[1] = start_addr & 0xFF;
    if (i2c_write(st->hw->addr, st->reg->prgm_start_h, 2, tmp))
        return -1;

    st->chip_cfg.dmp_loaded = 1;
    st->chip_cfg.dmp_sample_rate = sample_rate;
    return 0;
}

/**
 *  @brief  Load the DMP with this image.
 *  @return 0 if successful.
 */
static int dmp_load_motion_driver_firmware(void)
{
    return mpu_load_firmware(DMP_CODE_SIZE, dmp_memory, sStartAddress,
        DMP_SAMPLE_RATE);
}

/* These next two functions converts the orientation matrix (see
 * gyro_orientation) to a scalar representation for use by the DMP.
 * NOTE: These functions are borrowed from Invensense's MPL.
 */
static inline unsigned short inv_row_2_scale(const signed char *row)
{
    unsigned short b;

    if (row[0] > 0)
        b = 0;
    else if (row[0] < 0)
        b = 4;
    else if (row[1] > 0)
        b = 1;
    else if (row[1] < 0)
        b = 5;
    else if (row[2] > 0)
        b = 2;
    else if (row[2] < 0)
        b = 6;
    else
        b = 7;      // error
    return b;
}

static inline unsigned short inv_orientation_matrix_to_scalar(
    const signed char *mtx)
{
    unsigned short scalar;

    /*
       XYZ  010_001_000 Identity Matrix
       XZY  001_010_000
       YXZ  010_000_001
       YZX  000_010_001
       ZXY  001_000_010
       ZYX  000_001_010
     */

    scalar = inv_row_2_scale(mtx);
    scalar |= inv_row_2_scale(mtx + 3) << 3;
    scalar |= inv_row_2_scale(mtx + 6) << 6;


    return scalar;
}

/**
 *  @brief      Push gyro and accel orientation to the DMP.
 *  The orientation is represented here as the output of
 *  @e inv_orientation_matrix_to_scalar.
 *  @param[in]  orient  Gyro and accel orientation in body frame.
 *  @return     0 if successful.
 */
static int dmp_set_orientation(unsigned short orient)
{
    unsigned char gyro_regs[3], accel_regs[3];
    const unsigned char gyro_axes[3] = {DINA4C, DINACD, DINA6C};
    const unsigned char accel_axes[3] = {DINA0C, DINAC9, DINA2C};
    const unsigned char gyro_sign[3] = {DINA36, DINA56, DINA76};
    const unsigned char accel_sign[3] = {DINA26, DINA46, DINA66};

    gyro_regs[0] = gyro_axes[orient & 3];
    gyro_regs[1] = gyro_axes[(orient >> 3) & 3];
    gyro_regs[2] = gyro_axes[(orient >> 6) & 3];
    accel_regs[0] = accel_axes[orient & 3];
    accel_regs[1] = accel_axes[(orient >> 3) & 3];
    accel_regs[2] = accel_axes[(orient >> 6) & 3];

    /* Chip-to-body, axes only. */
    if (mpu_write_mem(FCFG_1, 3, gyro_regs))
        return -1;
    if (mpu_write_mem(FCFG_2, 3, accel_regs))
        return -1;

    memcpy(gyro_regs, gyro_sign, 3);
    memcpy(accel_regs, accel_sign, 3);
    if (orient & 4) {
        gyro_regs[0] |= 1;
        accel_regs[0] |= 1;
    }
    if (orient & 0x20) {
        gyro_regs[1] |= 1;
        accel_regs[1] |= 1;
    }
    if (orient & 0x100) {
        gyro_regs[2] |= 1;
        accel_regs[2] |= 1;
    }

    /* Chip-to-body, sign only. */
    if (mpu_write_mem(FCFG_3, 3, gyro_regs))
        return -1;
    if (mpu_write_mem(FCFG_7, 3, accel_regs))
        return -1;
    dmp.orient = orient;
    return 0;
}

/* Send data to the Python client application.
 * Data is formatted as follows:
 * packet[0]    = $
 * packet[1]    = packet type (see packet_type_e)
 * packet[2+]   = data
 */
static void send_packet(char packet_type, void *data)
{
#define MAX_BUF_LENGTH  (18)
    char buf[MAX_BUF_LENGTH], length;

    memset(buf, 0, MAX_BUF_LENGTH);
    buf[0] = '$';
    buf[1] = packet_type;

    if (packet_type == PACKET_TYPE_ACCEL || packet_type == PACKET_TYPE_GYRO) {
        short *sdata = (short*)data;
        buf[2] = (char)(sdata[0] >> 8);
        buf[3] = (char)sdata[0];
        buf[4] = (char)(sdata[1] >> 8);
        buf[5] = (char)sdata[1];
        buf[6] = (char)(sdata[2] >> 8);
        buf[7] = (char)sdata[2];
        length = 8;
    } else if (packet_type == PACKET_TYPE_QUAT) {
        long *ldata = (long*)data;
        buf[2] = (char)(ldata[0] >> 24);
        buf[3] = (char)(ldata[0] >> 16);
        buf[4] = (char)(ldata[0] >> 8);
        buf[5] = (char)ldata[0];
        buf[6] = (char)(ldata[1] >> 24);
        buf[7] = (char)(ldata[1] >> 16);
        buf[8] = (char)(ldata[1] >> 8);
        buf[9] = (char)ldata[1];
        buf[10] = (char)(ldata[2] >> 24);
        buf[11] = (char)(ldata[2] >> 16);
        buf[12] = (char)(ldata[2] >> 8);
        buf[13] = (char)ldata[2];
        buf[14] = (char)(ldata[3] >> 24);
        buf[15] = (char)(ldata[3] >> 16);
        buf[16] = (char)(ldata[3] >> 8);
        buf[17] = (char)ldata[3];
        length = 18;
    } else if (packet_type == PACKET_TYPE_TAP) {
        buf[2] = ((char*)data)[0];
        buf[3] = ((char*)data)[1];
        length = 4;
    } else if (packet_type == PACKET_TYPE_ANDROID_ORIENT) {
        buf[2] = ((char*)data)[0];
        length = 3;
    } else if (packet_type == PACKET_TYPE_PEDO) {
        long *ldata = (long*)data;
        buf[2] = (char)(ldata[0] >> 24);
        buf[3] = (char)(ldata[0] >> 16);
        buf[4] = (char)(ldata[0] >> 8);
        buf[5] = (char)ldata[0];
        buf[6] = (char)(ldata[1] >> 24);
        buf[7] = (char)(ldata[1] >> 16);
        buf[8] = (char)(ldata[1] >> 8);
        buf[9] = (char)ldata[1];
        length = 10;
    } else if (packet_type == PACKET_TYPE_MISC) {
        buf[2] = ((char*)data)[0];
        buf[3] = ((char*)data)[1];
        buf[4] = ((char*)data)[2];
        buf[5] = ((char*)data)[3];
        length = 6;
    }
    //cdcSendDataWaitTilDone((BYTE*)buf, length, CDC0_INTFNUM, 100);
}

static void tap_cb(unsigned char direction, unsigned char count)
{
    char data[2];
    data[0] = (char)direction;
    data[1] = (char)count;
    send_packet(PACKET_TYPE_TAP, data);
}

/**
 *  @brief      Register a function to be executed on a tap event.
 *  The tap direction is represented by one of the following:
 *  \n TAP_X_UP
 *  \n TAP_X_DOWN
 *  \n TAP_Y_UP
 *  \n TAP_Y_DOWN
 *  \n TAP_Z_UP
 *  \n TAP_Z_DOWN
 *  @param[in]  func    Callback function.
 *  @return     0 if successful.
 */
static int dmp_register_tap_cb(void (*func)(unsigned char, unsigned char))
{
    dmp.tap_cb = func;
    return 0;
}

static void android_orient_cb(unsigned char orientation)
{
    send_packet(PACKET_TYPE_ANDROID_ORIENT, &orientation);
}

/**
 *  @brief      Register a function to be executed on a android orientation event.
 *  @param[in]  func    Callback function.
 *  @return     0 if successful.
 */
static int dmp_register_android_orient_cb(void (*func)(unsigned char))
{
    dmp.android_orient_cb = func;
    return 0;
}

/**
 *  @brief      Calibrate the gyro data in the DMP.
 *  After eight seconds of no motion, the DMP will compute gyro biases and
 *  subtract them from the quaternion output. If @e dmp_enable_feature is
 *  called with @e DMP_FEATURE_SEND_CAL_GYRO, the biases will also be
 *  subtracted from the gyro output.
 *  @param[in]  enable  1 to enable gyro calibration.
 *  @return     0 if successful.
 */
static int dmp_enable_gyro_cal(unsigned char enable)
{
    if (enable) {
        unsigned char regs[9] = {0xb8, 0xaa, 0xb3, 0x8d, 0xb4, 0x98, 0x0d, 0x35, 0x5d};
        return mpu_write_mem(CFG_MOTION_BIAS, 9, regs);
    } else {
        unsigned char regs[9] = {0xb8, 0xaa, 0xaa, 0xaa, 0xb0, 0x88, 0xc3, 0xc5, 0xc7};
        return mpu_write_mem(CFG_MOTION_BIAS, 9, regs);
    }
}

/**
 *  @brief      Set tap threshold for a specific axis.
 *  @param[in]  axis    1, 2, and 4 for XYZ accel, respectively.
 *  @param[in]  thresh  Tap threshold, in mg/ms.
 *  @return     0 if successful.
 */
static int dmp_set_tap_thresh(unsigned char axis, unsigned short thresh)
{
    unsigned char tmp[4], accel_fsr;
    float scaled_thresh;
    unsigned short dmp_thresh, dmp_thresh_2;
    if (!(axis & TAP_XYZ) || thresh > 1600)
        return -1;

    scaled_thresh = (float)thresh / DMP_SAMPLE_RATE;

    mpu_get_accel_fsr(&accel_fsr);
    switch (accel_fsr) {
    case 2:
        dmp_thresh = (unsigned short)(scaled_thresh * 16384);
        /* dmp_thresh * 0.75 */
        dmp_thresh_2 = (unsigned short)(scaled_thresh * 12288);
        break;
    case 4:
        dmp_thresh = (unsigned short)(scaled_thresh * 8192);
        /* dmp_thresh * 0.75 */
        dmp_thresh_2 = (unsigned short)(scaled_thresh * 6144);
        break;
    case 8:
        dmp_thresh = (unsigned short)(scaled_thresh * 4096);
        /* dmp_thresh * 0.75 */
        dmp_thresh_2 = (unsigned short)(scaled_thresh * 3072);
        break;
    case 16:
        dmp_thresh = (unsigned short)(scaled_thresh * 2048);
        /* dmp_thresh * 0.75 */
        dmp_thresh_2 = (unsigned short)(scaled_thresh * 1536);
        break;
    default:
        return -1;
    }
    tmp[0] = (unsigned char)(dmp_thresh >> 8);
    tmp[1] = (unsigned char)(dmp_thresh & 0xFF);
    tmp[2] = (unsigned char)(dmp_thresh_2 >> 8);
    tmp[3] = (unsigned char)(dmp_thresh_2 & 0xFF);

    if (axis & TAP_X) {
        if (mpu_write_mem(DMP_TAP_THX, 2, tmp))
            return -1;
        if (mpu_write_mem(D_1_36, 2, tmp+2))
            return -1;
    }
    if (axis & TAP_Y) {
        if (mpu_write_mem(DMP_TAP_THY, 2, tmp))
            return -1;
        if (mpu_write_mem(D_1_40, 2, tmp+2))
            return -1;
    }
    if (axis & TAP_Z) {
        if (mpu_write_mem(DMP_TAP_THZ, 2, tmp))
            return -1;
        if (mpu_write_mem(D_1_44, 2, tmp+2))
            return -1;
    }
    return 0;
}

/**
 *  @brief      Set which axes will register a tap.
 *  @param[in]  axis    1, 2, and 4 for XYZ, respectively.
 *  @return     0 if successful.
 */
static int dmp_set_tap_axes(unsigned char axis)
{
    unsigned char tmp = 0;

    if (axis & TAP_X)
        tmp |= 0x30;
    if (axis & TAP_Y)
        tmp |= 0x0C;
    if (axis & TAP_Z)
        tmp |= 0x03;
    return mpu_write_mem(D_1_72, 1, &tmp);
}

/**
 *  @brief      Set minimum number of taps needed for an interrupt.
 *  @param[in]  min_taps    Minimum consecutive taps (1-4).
 *  @return     0 if successful.
 */
static int dmp_set_tap_count(unsigned char min_taps)
{
    unsigned char tmp;

    if (min_taps < 1)
        min_taps = 1;
    else if (min_taps > 4)
        min_taps = 4;

    tmp = min_taps - 1;
    return mpu_write_mem(D_1_79, 1, &tmp);
}

/**
 *  @brief      Set length between valid taps.
 *  @param[in]  time    Milliseconds between taps.
 *  @return     0 if successful.
 */
static int dmp_set_tap_time(unsigned short time)
{
    unsigned short dmp_time;
    unsigned char tmp[2];

    dmp_time = time / (1000 / DMP_SAMPLE_RATE);
    tmp[0] = (unsigned char)(dmp_time >> 8);
    tmp[1] = (unsigned char)(dmp_time & 0xFF);
    return mpu_write_mem(DMP_TAPW_MIN, 2, tmp);
}

/**
 *  @brief      Set max time between taps to register as a multi-tap.
 *  @param[in]  time    Max milliseconds between taps.
 *  @return     0 if successful.
 */
static int dmp_set_tap_time_multi(unsigned short time)
{
    unsigned short dmp_time;
    unsigned char tmp[2];

    dmp_time = time / (1000 / DMP_SAMPLE_RATE);
    tmp[0] = (unsigned char)(dmp_time >> 8);
    tmp[1] = (unsigned char)(dmp_time & 0xFF);
    return mpu_write_mem(D_1_218, 2, tmp);
}


/**
 *  @brief      Set shake rejection threshold.
 *  If the DMP detects a gyro sample larger than @e thresh, taps are rejected.
 *  @param[in]  sf      Gyro scale factor.
 *  @param[in]  thresh  Gyro threshold in dps.
 *  @return     0 if successful.
 */
static int dmp_set_shake_reject_thresh(long sf, unsigned short thresh)
{
    unsigned char tmp[4];
    long thresh_scaled = sf / 1000 * thresh;
    tmp[0] = (unsigned char)(((long)thresh_scaled >> 24) & 0xFF);
    tmp[1] = (unsigned char)(((long)thresh_scaled >> 16) & 0xFF);
    tmp[2] = (unsigned char)(((long)thresh_scaled >> 8) & 0xFF);
    tmp[3] = (unsigned char)((long)thresh_scaled & 0xFF);
    return mpu_write_mem(D_1_92, 4, tmp);
}

/**
 *  @brief      Set shake rejection time.
 *  Sets the length of time that the gyro must be outside of the threshold set
 *  by @e gyro_set_shake_reject_thresh before taps are rejected. A mandatory
 *  60 ms is added to this parameter.
 *  @param[in]  time    Time in milliseconds.
 *  @return     0 if successful.
 */
static int dmp_set_shake_reject_time(unsigned short time)
{
    unsigned char tmp[2];

    time /= (1000 / DMP_SAMPLE_RATE);
    tmp[0] = time >> 8;
    tmp[1] = time & 0xFF;
    return mpu_write_mem(D_1_90,2,tmp);
}

/**
 *  @brief      Set shake rejection timeout.
 *  Sets the length of time after a shake rejection that the gyro must stay
 *  inside of the threshold before taps can be detected again. A mandatory
 *  60 ms is added to this parameter.
 *  @param[in]  time    Time in milliseconds.
 *  @return     0 if successful.
 */
static int dmp_set_shake_reject_timeout(unsigned short time)
{
    unsigned char tmp[2];

    time /= (1000 / DMP_SAMPLE_RATE);
    tmp[0] = time >> 8;
    tmp[1] = time & 0xFF;
    return mpu_write_mem(D_1_88,2,tmp);
}

/**
 *  @brief      Generate 3-axis quaternions from the DMP.
 *  In this driver, the 3-axis and 6-axis DMP quaternion features are mutually
 *  exclusive.
 *  @param[in]  enable  1 to enable 3-axis quaternion.
 *  @return     0 if successful.
 */
static int dmp_enable_lp_quat(unsigned char enable)
{
    unsigned char regs[4];
    if (enable) {
        regs[0] = DINBC0;
        regs[1] = DINBC2;
        regs[2] = DINBC4;
        regs[3] = DINBC6;
    }
    else
        memset(regs, 0x8B, 4);

    mpu_write_mem(CFG_LP_QUAT, 4, regs);

    return mpu_reset_fifo();
}

/**
 *  @brief       Generate 6-axis quaternions from the DMP.
 *  In this driver, the 3-axis and 6-axis DMP quaternion features are mutually
 *  exclusive.
 *  @param[in]   enable  1 to enable 6-axis quaternion.
 *  @return      0 if successful.
 */
static int dmp_enable_6x_lp_quat(unsigned char enable)
{
    unsigned char regs[4];
    if (enable) {
        regs[0] = DINA20;
        regs[1] = DINA28;
        regs[2] = DINA30;
        regs[3] = DINA38;
    } else
        memset(regs, 0xA3, 4);

    mpu_write_mem(CFG_8, 4, regs);

    return mpu_reset_fifo();
}

/**
 *  @brief      Enable DMP features.
 *  The following \#define's are used in the input mask:
 *  \n DMP_FEATURE_TAP
 *  \n DMP_FEATURE_ANDROID_ORIENT
 *  \n DMP_FEATURE_LP_QUAT
 *  \n DMP_FEATURE_6X_LP_QUAT
 *  \n DMP_FEATURE_GYRO_CAL
 *  \n DMP_FEATURE_SEND_RAW_ACCEL
 *  \n DMP_FEATURE_SEND_RAW_GYRO
 *  \n NOTE: DMP_FEATURE_LP_QUAT and DMP_FEATURE_6X_LP_QUAT are mutually
 *  exclusive.
 *  \n NOTE: DMP_FEATURE_SEND_RAW_GYRO and DMP_FEATURE_SEND_CAL_GYRO are also
 *  mutually exclusive.
 *  @param[in]  mask    Mask of features to enable.
 *  @return     0 if successful.
 */
static int dmp_enable_feature(unsigned short mask)
{
    int ret=0;
    unsigned char tmp[10];

    /* TODO: All of these settings can probably be integrated into the default
     * DMP image.
     */
    /* Set integration scale factor. */
    tmp[0] = (unsigned char)((GYRO_SF >> 24) & 0xFF);
    tmp[1] = (unsigned char)((GYRO_SF >> 16) & 0xFF);
    tmp[2] = (unsigned char)((GYRO_SF >> 8) & 0xFF);
    tmp[3] = (unsigned char)(GYRO_SF & 0xFF);
    mpu_write_mem(D_0_104, 4, tmp);

    /* Send sensor data to the FIFO. */
    tmp[0] = 0xA3;
    if (mask & DMP_FEATURE_SEND_RAW_ACCEL) {
        tmp[1] = 0xC0;
        tmp[2] = 0xC8;
        tmp[3] = 0xC2;
    } else {
        tmp[1] = 0xA3;
        tmp[2] = 0xA3;
        tmp[3] = 0xA3;
    }
    if (mask & DMP_FEATURE_SEND_ANY_GYRO) {
        tmp[4] = 0xC4;
        tmp[5] = 0xCC;
        tmp[6] = 0xC6;
    } else {
        tmp[4] = 0xA3;
        tmp[5] = 0xA3;
        tmp[6] = 0xA3;
    }
    tmp[7] = 0xA3;
    tmp[8] = 0xA3;
    tmp[9] = 0xA3;
    ret = mpu_write_mem(CFG_15,10,tmp);
    if (ret) {
        return -1;
    }

    /* Send gesture data to the FIFO. */
    if (mask & (DMP_FEATURE_TAP | DMP_FEATURE_ANDROID_ORIENT))
        tmp[0] = DINA20;
    else
        tmp[0] = 0xD8;
    ret = mpu_write_mem(CFG_27,1,tmp);
    if (ret) {
        return -2;
    }

    if (mask & DMP_FEATURE_GYRO_CAL) {
        ret = dmp_enable_gyro_cal(1);
        if (ret) {
            return -3;
        }

    } else {
        ret = dmp_enable_gyro_cal(0);
        if (ret) {
            return -4;
        }
    }

    if (mask & DMP_FEATURE_SEND_ANY_GYRO) {
        if (mask & DMP_FEATURE_SEND_CAL_GYRO) {
            tmp[0] = 0xB2;
            tmp[1] = 0x8B;
            tmp[2] = 0xB6;
            tmp[3] = 0x9B;
        } else {
            tmp[0] = DINAC0;
            tmp[1] = DINA80;
            tmp[2] = DINAC2;
            tmp[3] = DINA90;
        }
        ret = mpu_write_mem(CFG_GYRO_RAW_DATA, 4, tmp);
        if (ret) {
            return -5;
        }
    }

    ret = 0;
    if (mask & DMP_FEATURE_TAP) {
        /* Enable tap. */
        tmp[0] = 0xF8;
        ret |= mpu_write_mem(CFG_20, 1, tmp);
        ret |= dmp_set_tap_thresh(TAP_XYZ, 250);
        ret |= dmp_set_tap_axes(TAP_XYZ);
        ret |= dmp_set_tap_count(1);
        ret |= dmp_set_tap_time(100);
        ret |= dmp_set_tap_time_multi(500);

        ret |= dmp_set_shake_reject_thresh(GYRO_SF, 200);
        ret |= dmp_set_shake_reject_time(40);
        ret |= dmp_set_shake_reject_timeout(10);
    } else {
        tmp[0] = 0xD8;
        ret |= mpu_write_mem(CFG_20, 1, tmp);
    }
    if (ret) {
        return -6;
    }

    if (mask & DMP_FEATURE_ANDROID_ORIENT) {
        tmp[0] = 0xD9;
    } else
        tmp[0] = 0xD8;
    ret = mpu_write_mem(CFG_ANDROID_ORIENT_INT, 1, tmp);
    if (ret) {
        return -7;
    }

    if (mask & DMP_FEATURE_LP_QUAT) {
        ret = dmp_enable_lp_quat(1);
    } else {
        ret = dmp_enable_lp_quat(0);
    }
    if (ret) {
        return -8;
    }

    if (mask & DMP_FEATURE_6X_LP_QUAT) {
        ret = dmp_enable_6x_lp_quat(1);
    } else {
        ret = dmp_enable_6x_lp_quat(0);
    }
    if (ret) {
        return -9;
    }

    /* Pedometer is always enabled. */
    dmp.feature_mask = mask | DMP_FEATURE_PEDOMETER;
    ret = mpu_reset_fifo();
    if (ret) {
        return -10;
    }

    dmp.packet_length = 0;
    if (mask & DMP_FEATURE_SEND_RAW_ACCEL)
        dmp.packet_length += 6;
    if (mask & DMP_FEATURE_SEND_ANY_GYRO)
        dmp.packet_length += 6;
    if (mask & (DMP_FEATURE_LP_QUAT | DMP_FEATURE_6X_LP_QUAT))
        dmp.packet_length += 16;
    if (mask & (DMP_FEATURE_TAP | DMP_FEATURE_ANDROID_ORIENT))
        dmp.packet_length += 4;

    return 0;
}

/**
 *  @brief      Set DMP output rate.
 *  Only used when DMP is on.
 *  @param[in]  rate    Desired fifo rate (Hz).
 *  @return     0 if successful.
 */
static int dmp_set_fifo_rate(unsigned short rate)
{
    const unsigned char regs_end[12] = {DINAFE, DINAF2, DINAAB,
        0xc4, DINAAA, DINAF1, DINADF, DINADF, 0xBB, 0xAF, DINADF, DINADF};
    unsigned short div;
    unsigned char tmp[8];

    if (rate > DMP_SAMPLE_RATE)
        return -1;
    div = DMP_SAMPLE_RATE / rate - 1;
    tmp[0] = (unsigned char)((div >> 8) & 0xFF);
    tmp[1] = (unsigned char)(div & 0xFF);
    if (mpu_write_mem(D_0_22, 2, tmp))
        return -1;
    if (mpu_write_mem(CFG_6, 12, (unsigned char*)regs_end))
        return -1;

    dmp.fifo_rate = rate;
    return 0;
}

/**
 *  @brief      Set device to bypass mode.
 *  @param[in]  bypass_on   1 to enable bypass mode.
 *  @return     0 if successful.
 */
static int mpu_set_bypass(unsigned char bypass_on)
{
    unsigned char tmp;

    if (st->chip_cfg.bypass_mode == bypass_on)
        return 0;

    if (bypass_on) {
        if (i2c_read(st->hw->addr, st->reg->user_ctrl, 1, &tmp))
            return -1;
        tmp &= ~BIT_AUX_IF_EN;
        if (i2c_write(st->hw->addr, st->reg->user_ctrl, 1, &tmp))
            return -1;
        delay_ms(3);
        tmp = BIT_BYPASS_EN;
        if (st->chip_cfg.active_low_int)
            tmp |= BIT_ACTL;
        if (st->chip_cfg.latched_int)
            tmp |= BIT_LATCH_EN | BIT_ANY_RD_CLR;
        if (i2c_write(st->hw->addr, st->reg->int_pin_cfg, 1, &tmp))
            return -1;
    } else {
        /* Enable I2C master mode if compass is being used. */
        if (i2c_read(st->hw->addr, st->reg->user_ctrl, 1, &tmp))
            return -1;
        if (st->chip_cfg.sensors & INV_XYZ_COMPASS)
            tmp |= BIT_AUX_IF_EN;
        else
            tmp &= ~BIT_AUX_IF_EN;
        if (i2c_write(st->hw->addr, st->reg->user_ctrl, 1, &tmp))
            return -1;
        delay_ms(3);
        if (st->chip_cfg.active_low_int)
            tmp = BIT_ACTL;
        else
            tmp = 0;
        if (st->chip_cfg.latched_int)
            tmp |= BIT_LATCH_EN | BIT_ANY_RD_CLR;
        if (i2c_write(st->hw->addr, st->reg->int_pin_cfg, 1, &tmp))
            return -1;
    }
    st->chip_cfg.bypass_mode = bypass_on;
    return 0;
}

/**
 *  @brief      Enable/disable DMP support.
 *  @param[in]  enable  1 to turn on the DMP.
 *  @return     0 if successful.
 */
static int mpu_set_dmp_state(unsigned char enable)
{
    unsigned char tmp;
    if (st->chip_cfg.dmp_on == enable)
        return 0;

    if (enable) {
        if (!st->chip_cfg.dmp_loaded)
            return -1;
        /* Disable data ready interrupt. */
        set_int_enable(0);
        /* Disable bypass mode. */
        mpu_set_bypass(0);
        /* Keep constant sample rate, FIFO rate controlled by DMP. */
        mpu_set_sample_rate(st->chip_cfg.dmp_sample_rate);
        /* Remove FIFO elements. */
        tmp = 0;
        i2c_write(st->hw->addr, 0x23, 1, &tmp);
        st->chip_cfg.dmp_on = 1;
        /* Enable DMP interrupt. */
        set_int_enable(1);
        mpu_reset_fifo();
    } else {
        /* Disable DMP interrupt. */
        set_int_enable(0);
        /* Restore FIFO settings. */
        tmp = st->chip_cfg.fifo_enable;
        i2c_write(st->hw->addr, 0x23, 1, &tmp);
        st->chip_cfg.dmp_on = 0;
        mpu_reset_fifo();
    }
    return 0;
}

/**
 *  @brief      Get one unparsed packet from the FIFO.
 *  This function should be used if the packet is to be parsed elsewhere.
 *  @param[in]  length  Length of one FIFO packet.
 *  @param[in]  data    FIFO packet.
 *  @param[in]  more    Number of remaining packets.
 */
static int mpu_read_fifo_stream(unsigned short length, unsigned char *data, unsigned int *more)
{
    unsigned char tmp[2];
    unsigned short fifo_count;
    unsigned short target_read;
    if (!st->chip_cfg.dmp_on) {
        printf("[GSTEAM] dmp: %d \n", (int)st->chip_cfg.dmp_on);
        return -1;
    }
    if (!st->chip_cfg.sensors) {
        printf("[GSTEAM] sensors: %d \n", (int)st->chip_cfg.sensors);
        return -2;
    }

    if (i2c_read(st->hw->addr, st->reg->fifo_count_h, 1, &tmp[0]))
        return -3;

    if (i2c_read(st->hw->addr, st->reg->fifo_count_l, 1, &tmp[1]))
        return -3;
        
    fifo_count = (tmp[0] << 8) | tmp[1];
    printf("[GSTEAM] FIFO count: %hd read len:  %hd\n", fifo_count, length);    
    
    if (fifo_count < length) {
        more[0] = 0;
        return -4;
    } else {
        if (i2c_read(st->hw->addr, st->reg->fifo_r_w, length, data)) return -7;
    }
    
    #if 1
    if (fifo_count > (st->hw->max_fifo >> 1)) {
        /* FIFO is 50% full, better check overflow bit. */
        #if 1
        if (i2c_read(st->hw->addr, st->reg->int_status, 1, tmp))
            return -5;

        if (tmp[0] & BIT_FIFO_OVERFLOW) {
            printf("[GSTEAM] FIFO overflow status: 0x%x \n", tmp[0]);
            //delay_ms(1000);
            
            mpu_reset_fifo();
            return -6;
        }
        #endif
    }
    #endif
    
    more[0] = fifo_count / length - 1;
    
    //printf("[GSTEAM] count: %d, length: %d, more: %d\n", (int)fifo_count, (int)length, (int)more[0]);
    return 0;
}

/**
 *  @brief      Decode the four-byte gesture data and execute any callbacks.
 *  @param[in]  gesture Gesture data from DMP packet.
 *  @return     0 if successful.
 */
static int decode_gesture(unsigned char *gesture)
{
    unsigned char tap, android_orient;

    android_orient = gesture[3] & 0xC0;
    tap = 0x3F & gesture[3];

    if (gesture[1] & INT_SRC_TAP) {
        unsigned char direction, count;
        direction = tap >> 3;
        count = (tap % 8) + 1;
        if (dmp.tap_cb)
            dmp.tap_cb(direction, count);
    }

    if (gesture[1] & INT_SRC_ANDROID_ORIENT) {
        if (dmp.android_orient_cb)
            dmp.android_orient_cb(android_orient >> 6);
    }

    return 0;
}

/**
 *  @brief      Get one packet from the FIFO.
 *  If @e sensors does not contain a particular sensor, disregard the data
 *  returned to that pointer.
 *  \n @e sensors can contain a combination of the following flags:
 *  \n INV_X_GYRO, INV_Y_GYRO, INV_Z_GYRO
 *  \n INV_XYZ_GYRO
 *  \n INV_XYZ_ACCEL
 *  \n INV_WXYZ_QUAT
 *  \n If the FIFO has no new data, @e sensors will be zero.
 *  \n If the FIFO is disabled, @e sensors will be zero and this function will
 *  return a non-zero error code.
 *  @param[out] gyro        Gyro data in hardware units.
 *  @param[out] accel       Accel data in hardware units.
 *  @param[out] quat        3-axis quaternion data in hardware units.
 *  @param[out] timestamp   Timestamp in milliseconds.
 *  @param[out] sensors     Mask of sensors read from FIFO.
 *  @param[out] more        Number of remaining packets.
 *  @return     0 if successful.
 */
static int dmp_read_fifo(short *gyro, short *accel, unsigned int *quat,
    struct timespec *timestamp, short *sensors, unsigned int *more)
{
    int ret=0;
    unsigned char fifo_data[HWST_MAX_PACKET_LENGTH];
    unsigned char ii = 0;

    /* TODO: sensors[0] only changes when dmp_enable_feature is called. We can
     * cache this value and save some cycles.
     */
    sensors[0] = 0;

    memset(fifo_data, 0, HWST_MAX_PACKET_LENGTH);
    
    /* Get a packet. */
    //printf("BEG dmp_read_fifo() read size: %d \n", dmp.packet_length);
    ret = mpu_read_fifo_stream(dmp.packet_length, fifo_data, more);
    //printf("END dmp_read_fifo() more: %d ret = %d feature = 0x%x\n", *more, ret, dmp.feature_mask);

    if (ret) {
        return (ret * 10 + -1);
    } 
    
    /* Parse DMP packet. */
    if (dmp.feature_mask & (DMP_FEATURE_LP_QUAT | DMP_FEATURE_6X_LP_QUAT)) {
#ifdef FIFO_CORRUPTION_CHECK
        long quat_q14[4], quat_mag_sq;
#endif
        quat[0] = ((long)fifo_data[0] << 24) | ((long)fifo_data[1] << 16) |
            ((long)fifo_data[2] << 8) | fifo_data[3];
        quat[1] = ((long)fifo_data[4] << 24) | ((long)fifo_data[5] << 16) |
            ((long)fifo_data[6] << 8) | fifo_data[7];
        quat[2] = ((long)fifo_data[8] << 24) | ((long)fifo_data[9] << 16) |
            ((long)fifo_data[10] << 8) | fifo_data[11];
        quat[3] = ((long)fifo_data[12] << 24) | ((long)fifo_data[13] << 16) |
            ((long)fifo_data[14] << 8) | fifo_data[15];
        ii += 16;
#ifdef FIFO_CORRUPTION_CHECK
        /* We can detect a corrupted FIFO by monitoring the quaternion data and
         * ensuring that the magnitude is always normalized to one. This
         * shouldn't happen in normal operation, but if an I2C error occurs,
         * the FIFO reads might become misaligned.
         *
         * Let's start by scaling down the quaternion data to avoid long long
         * math.
         */
        quat_q14[0] = quat[0] >> 16;
        quat_q14[1] = quat[1] >> 16;
        quat_q14[2] = quat[2] >> 16;
        quat_q14[3] = quat[3] >> 16;
        quat_mag_sq = quat_q14[0] * quat_q14[0] + quat_q14[1] * quat_q14[1] +
            quat_q14[2] * quat_q14[2] + quat_q14[3] * quat_q14[3];
        if ((quat_mag_sq < QUAT_MAG_SQ_MIN) ||
            (quat_mag_sq > QUAT_MAG_SQ_MAX)) {
            /* Quaternion is outside of the acceptable threshold. */

            printf("Quaternion is outside of the acceptable threshold. \n");
            //usleep(1000000);
            
            mpu_reset_fifo();
            sensors[0] = 0;
            return -1;
        }
#endif
        sensors[0] |= INV_WXYZ_QUAT;
    }

    if (dmp.feature_mask & DMP_FEATURE_SEND_RAW_ACCEL) {
        accel[0] = ((short)fifo_data[ii+0] << 8) | fifo_data[ii+1];
        accel[1] = ((short)fifo_data[ii+2] << 8) | fifo_data[ii+3];
        accel[2] = ((short)fifo_data[ii+4] << 8) | fifo_data[ii+5];
        ii += 6;
        sensors[0] |= INV_XYZ_ACCEL;
    }

    if (dmp.feature_mask & DMP_FEATURE_SEND_ANY_GYRO) {
        gyro[0] = ((short)fifo_data[ii+0] << 8) | fifo_data[ii+1];
        gyro[1] = ((short)fifo_data[ii+2] << 8) | fifo_data[ii+3];
        gyro[2] = ((short)fifo_data[ii+4] << 8) | fifo_data[ii+5];
        ii += 6;
        sensors[0] |= INV_XYZ_GYRO;
    }

    /* Gesture data is at the end of the DMP packet. Parse it and call
     * the gesture callbacks (if registered).
     */
    if (dmp.feature_mask & (DMP_FEATURE_TAP | DMP_FEATURE_ANDROID_ORIENT))
        decode_gesture(fifo_data + ii);

    //get_curtime(timestamp);
    return 0;
}

/**
 *  @brief      Get one packet from the FIFO.
 *  If @e sensors does not contain a particular sensor, disregard the data
 *  returned to that pointer.
 *  \n @e sensors can contain a combination of the following flags:
 *  \n INV_X_GYRO, INV_Y_GYRO, INV_Z_GYRO
 *  \n INV_XYZ_GYRO
 *  \n INV_XYZ_ACCEL
 *  \n If the FIFO has no new data, @e sensors will be zero.
 *  \n If the FIFO is disabled, @e sensors will be zero and this function will
 *  return a non-zero error code.
 *  @param[out] gyro        Gyro data in hardware units.
 *  @param[out] accel       Accel data in hardware units.
 *  @param[out] timestamp   Timestamp in milliseconds.
 *  @param[out] sensors     Mask of sensors read from FIFO.
 *  @param[out] more        Number of remaining packets.
 *  @return     0 if successful.
 */
static int mpu_read_fifo(short *gyro, short *accel, struct timespec *timestamp,
        unsigned char *sensors, unsigned int *more)
{
static long nsLast=0;
    /* Assumes maximum packet size is gyro (6) + accel (6). */
    unsigned char tmp[2], sflag=0;
    unsigned short index = 0, fifo_count1;

    int packet_size=0, fifo_count=0, read_count=0, readsz=0;
    char *data=0, pbuf[1024];

    //pbuf = malloc(1024);

    if (!pbuf) {
        printf("Error: allocate memory for read buffer failed!!!");
        return -1;
    }

    data = pbuf;
    
    if (st->chip_cfg.dmp_on) {
        printf("[GMPU] error!!! DMP is on (%d)!!!\n", st->chip_cfg.dmp_on);
        //return -1;
    }

    if (!st->chip_cfg.sensors) {
        return -2;
    }
    if (!st->chip_cfg.fifo_enable) {
        return -3;
    }

    if (st->chip_cfg.fifo_enable & INV_X_GYRO) {
        packet_size += 2;
        sflag |= INV_X_GYRO;
    }
    if (st->chip_cfg.fifo_enable & INV_Y_GYRO) {
        packet_size += 2;
        sflag |= INV_Y_GYRO;
    }
    if (st->chip_cfg.fifo_enable & INV_Z_GYRO) {
        packet_size += 2;
        sflag |= INV_Z_GYRO;
    }
    if (st->chip_cfg.fifo_enable & INV_XYZ_ACCEL) {
        packet_size += 6;
        sflag |= INV_XYZ_ACCEL;
    }


    if (i2c_read(st->hw->addr, st->reg->fifo_count_h, 2, data)) {
        return -4;
    }

    fifo_count = (data[0] << 8) | data[1];
        
    //log_i("FIFO count: %hd\n", fifo_count);
    
#if 0
    if (i2c_read(st->hw->addr, st->reg->fifo_count_h, 1, &tmp[0]))
        return -3;

    if (i2c_read(st->hw->addr, st->reg->fifo_count_l, 1, &tmp[1]))
        return -3;

    fifo_count1 = (tmp[0] << 8) | tmp[1]; 
        
    log_i("FIFO count: %hd, %hd\n", fifo_count, fifo_count1);
#endif

    #if 1
    if (fifo_count > (st->hw->max_fifo >> 1)) {
        /* FIFO is 50% full, better check overflow bit. */
        get_curtime(timestamp);
        log_i("FIFO count: %hd (%ld) \n", fifo_count, timestamp->tv_nsec - nsLast);
        if (i2c_read(st->hw->addr, st->reg->int_status, 1, data))
            return -5;
        if (data[0] & BIT_FIFO_OVERFLOW) {
            mpu_reset_fifo();
            return -6;
        }
    }
    #endif

    if (fifo_count < packet_size) {
        return 0;
    } else {
        read_count = fifo_count / packet_size;
        readsz = read_count * packet_size;
    }

    if (i2c_read(st->hw->addr, st->reg->fifo_r_w, readsz, pbuf)) {
        return -7;
    }
        
    *more = read_count;

    while (read_count) {
        if (sflag & INV_XYZ_ACCEL) {
            accel[0] = (data[index+0] << 8) | data[index+1];
            accel[1] = (data[index+2] << 8) | data[index+3];
            accel[2] = (data[index+4] << 8) | data[index+5];
            index += 6;
        }
        if (sflag & INV_X_GYRO) {
            gyro[0] = (data[index+0] << 8) | data[index+1];
            index += 2;
        }
        if (sflag & INV_Y_GYRO) {
            gyro[1] = (data[index+0] << 8) | data[index+1];
            index += 2;
        }
        if (sflag & INV_Z_GYRO) {
            gyro[2] = (data[index+0] << 8) | data[index+1];
            index += 2;
        }

        data += packet_size;
        accel += 3;
        gyro += 3;
        index = 0;
        read_count --;
    }

    *sensors = sflag;
    
    //get_curtime(timestamp);
    //nsLast = timestamp->tv_nsec;

    return 0;
}

/**
 *  @brief      Get the current DLPF setting.
 *  @param[out] lpf Current LPF setting.
 *  0 if successful.
 */
static int mpu_get_lpf(unsigned short *lpf)
{
    switch (st->chip_cfg.lpf) {
    case INV_FILTER_188HZ:
        lpf[0] = 188;
        break;
    case INV_FILTER_98HZ:
        lpf[0] = 98;
        break;
    case INV_FILTER_42HZ:
        lpf[0] = 42;
        break;
    case INV_FILTER_20HZ:
        lpf[0] = 20;
        break;
    case INV_FILTER_10HZ:
        lpf[0] = 10;
        break;
    case INV_FILTER_5HZ:
        lpf[0] = 5;
        break;
    case INV_FILTER_256HZ_NOLPF2:
    case INV_FILTER_2100HZ_NOLPF:
    default:
        lpf[0] = 0;
        break;
    }
    return 0;
}

/**
 *  @brief      Get current FIFO configuration.
 *  @e sensors can contain a combination of the following flags:
 *  \n INV_X_GYRO, INV_Y_GYRO, INV_Z_GYRO
 *  \n INV_XYZ_GYRO
 *  \n INV_XYZ_ACCEL
 *  @param[out] sensors Mask of sensors in FIFO.
 *  @return     0 if successful.
 */
static int mpu_get_fifo_config(unsigned char *sensors)
{
    sensors[0] = st->chip_cfg.fifo_enable;
    return 0;
}

/**
 *  @brief      Set the gyro full-scale range.
 *  @param[in]  fsr Desired full-scale range.
 *  @return     0 if successful.
 */
static int mpu_set_gyro_fsr(unsigned short fsr)
{
    unsigned char data;

    if (!(st->chip_cfg.sensors))
        return -1;

    switch (fsr) {
    case 250:
        data = INV_FSR_250DPS << 3;
        break;
    case 500:
        data = INV_FSR_500DPS << 3;
        break;
    case 1000:
        data = INV_FSR_1000DPS << 3;
        break;
    case 2000:
        data = INV_FSR_2000DPS << 3;
        break;
    default:
        return -1;
    }

    if (st->chip_cfg.gyro_fsr == (data >> 3))
        return 0;
    if (i2c_write(st->hw->addr, st->reg->gyro_cfg, 1, &data))
        return -1;
    st->chip_cfg.gyro_fsr = data >> 3;
    return 0;
}

/**
 *  @brief      Set the accel full-scale range.
 *  @param[in]  fsr Desired full-scale range.
 *  @return     0 if successful.
 */
static int mpu_set_accel_fsr(unsigned char fsr)
{
    unsigned char data;

    if (!(st->chip_cfg.sensors))
        return -1;

    switch (fsr) {
    case 2:
        data = INV_FSR_2G << 3;
        break;
    case 4:
        data = INV_FSR_4G << 3;
        break;
    case 8:
        data = INV_FSR_8G << 3;
        break;
    case 16:
        data = INV_FSR_16G << 3;
        break;
    default:
        return -1;
    }

    if (st->chip_cfg.accel_fsr == (data >> 3))
        return 0;
    if (i2c_write(st->hw->addr, st->reg->accel_cfg, 1, &data))
        return -1;
    st->chip_cfg.accel_fsr = data >> 3;
    return 0;
}

/**
 *  @brief      Get gyro sensitivity scale factor.
 *  @param[out] sens    Conversion from hardware units to dps.
 *  @return     0 if successful.
 */
int mpu_get_gyro_sens(float *sens)
{
    switch (st->chip_cfg.gyro_fsr) {
    case INV_FSR_250DPS:
        sens[0] = 131.0f;
        break;
    case INV_FSR_500DPS:
        sens[0] = 65.5f;
        break;
    case INV_FSR_1000DPS:
        sens[0] = 32.8f;
        break;
    case INV_FSR_2000DPS:
        sens[0] = 16.4f;
        break;
    default:
        return -1;
    }
    return 0;
}

/**
 *  @brief      Get accel sensitivity scale factor.
 *  @param[out] sens    Conversion from hardware units to g's.
 *  @return     0 if successful.
 */
int mpu_get_accel_sens(unsigned int *sens)
{
    switch (st->chip_cfg.accel_fsr) {
    case INV_FSR_2G:
        sens[0] = 16384;
        break;
    case INV_FSR_4G:
        sens[0] = 8092;
        break;
    case INV_FSR_8G:
        sens[0] = 4096;
        break;
    case INV_FSR_16G:
        sens[0] = 2048;
        break;
    default:
        return -1;
    }
    if (st->chip_cfg.accel_half)
        sens[0] >>= 1;
    return 0;
}

/**
 *  @brief      Enters LP accel motion interrupt mode.
 *  The behaviour of this feature is very different between the MPU6050 and the
 *  MPU6500. Each chip's version of this feature is explained below.
 *
 *  \n The hardware motion threshold can be between 32mg and 8160mg in 32mg
 *  increments.
 *
 *  \n Low-power accel mode supports the following frequencies:
 *  \n 1.25Hz, 5Hz, 20Hz, 40Hz
 *
 *  \n MPU6500:
 *  \n Unlike the MPU6050 version, the hardware does not "lock in" a reference
 *  sample. The hardware monitors the accel data and detects any large change
 *  over a short period of time.
 *
 *  \n The hardware motion threshold can be between 4mg and 1020mg in 4mg
 *  increments.
 *
 *  \n MPU6500 Low-power accel mode supports the following frequencies:
 *  \n 0.24Hz, 0.49Hz, 0.98Hz, 1.95Hz, 3.91Hz, 7.81Hz, 15.63Hz, 31.25Hz, 62.5Hz, 125Hz, 250Hz, 500Hz
 *
 *  \n\n NOTES:
 *  \n The driver will round down @e thresh to the nearest supported value if
 *  an unsupported threshold is selected.
 *  \n To select a fractional wake-up frequency, round down the value passed to
 *  @e lpa_freq.
 *  \n The MPU6500 does not support a delay parameter. If this function is used
 *  for the MPU6500, the value passed to @e time will be ignored.
 *  \n To disable this mode, set @e lpa_freq to zero. The driver will restore
 *  the previous configuration.
 *
 *  @param[in]  thresh      Motion threshold in mg.
 *  @param[in]  time        Duration in milliseconds that the accel data must
 *                          exceed @e thresh before motion is reported.
 *  @param[in]  lpa_freq    Minimum sampling rate, or zero to disable.
 *  @return     0 if successful.
 */
static int mpu_lp_motion_interrupt(unsigned short thresh, unsigned char time,
    unsigned char lpa_freq)
{
    unsigned char ch=0;
#if defined MPU6500
    unsigned char data[3];
#endif
    if (lpa_freq) {
#if defined MPU6500
    	unsigned char thresh_hw;

        /* 1LSb = 4mg. */
        if (thresh > 1020)
            thresh_hw = 255;
        else if (thresh < 4)
            thresh_hw = 1;
        else
            thresh_hw = thresh >> 2;
#endif

        if (!time)
            /* Minimum duration must be 1ms. */
            time = 1;

#if defined MPU6500
        if (lpa_freq > 500)
            /* At this point, the chip has not been re-configured, so the
             * function can safely exit.
             */
            return -1;
#endif

        if (!st->chip_cfg.int_motion_only) {
            /* Store current settings for later. */
            if (st->chip_cfg.dmp_on) {
                mpu_set_dmp_state(0);
                st->chip_cfg.cache.dmp_on = 1;
            } else
                st->chip_cfg.cache.dmp_on = 0;
            mpu_get_gyro_fsr(&st->chip_cfg.cache.gyro_fsr);
            
            mpu_get_accel_fsr(&ch);
            st->chip_cfg.cache.accel_fsr = ch;
            
            mpu_get_lpf(&st->chip_cfg.cache.lpf);
            mpu_get_sample_rate(&st->chip_cfg.cache.sample_rate);
            st->chip_cfg.cache.sensors_on = st->chip_cfg.sensors;
            
            mpu_get_fifo_config(&ch);
            st->chip_cfg.cache.fifo_sensors = ch;
        }

#if defined MPU6500
        /* Disable hardware interrupts. */
        set_int_enable(0);

        /* Enter full-power accel-only mode, no FIFO/DMP. */
        data[0] = 0;
        data[1] = 0;
        data[2] = BIT_STBY_XYZG;
        if (i2c_write(st->hw->addr, st->reg->user_ctrl, 3, data))
            goto lp_int_restore;

        /* Set motion threshold. */
        data[0] = thresh_hw;
        if (i2c_write(st->hw->addr, st->reg->motion_thr, 1, data))
            goto lp_int_restore;

        /* Set wake frequency. */
        if (lpa_freq == 1)
            data[0] = INV_LPA_0_98HZ;
        else if (lpa_freq == 2)
            data[0] = INV_LPA_1_95HZ;
        else if (lpa_freq <= 5)
            data[0] = INV_LPA_3_91HZ;
        else if (lpa_freq <= 10)
            data[0] = INV_LPA_7_81HZ;
        else if (lpa_freq <= 20)
            data[0] = INV_LPA_15_63HZ;
        else if (lpa_freq <= 40)
            data[0] = INV_LPA_31_25HZ;
        else if (lpa_freq <= 70)
            data[0] = INV_LPA_62_50HZ;
        else if (lpa_freq <= 125)
            data[0] = INV_LPA_125HZ;
        else if (lpa_freq <= 250)
            data[0] = INV_LPA_250HZ;
        else
            data[0] = INV_LPA_500HZ;
        if (i2c_write(st->hw->addr, st->reg->lp_accel_odr, 1, data))
            goto lp_int_restore;

        /* Enable motion interrupt (MPU6500 version). */
        data[0] = BITS_WOM_EN;
        if (i2c_write(st->hw->addr, st->reg->accel_intel, 1, data))
            goto lp_int_restore;
            
        /* Bypass DLPF ACCEL_FCHOICE_B=1*/
        data[0] = BIT_ACCL_FC_B | 0x01;
        if (i2c_write(st->hw->addr, st->reg->accel_cfg2, 1, data))
            goto lp_int_restore;

        /* Enable interrupt. */
        data[0] = BIT_MOT_INT_EN;
        if (i2c_write(st->hw->addr, st->reg->int_enable, 1, data))
            goto lp_int_restore;
        
        /* Enable cycle mode. */
        data[0] = BIT_LPA_CYCLE;
        if (i2c_write(st->hw->addr, st->reg->pwr_mgmt_1, 1, data))
            goto lp_int_restore;
            
        st->chip_cfg.int_motion_only = 1;
        return 0;
#endif
    } else {
        /* Don't "restore" the previous state if no state has been saved. */
        int ii;
        char *cache_ptr = (char*)&st->chip_cfg.cache;
        for (ii = 0; ii < sizeof(st->chip_cfg.cache); ii++) {
            if (cache_ptr[ii] != 0)
                goto lp_int_restore;
        }
        /* If we reach this point, motion interrupt mode hasn't been used yet. */
        return -1;
    }
lp_int_restore:
    /* Set to invalid values to ensure no I2C writes are skipped. */
    st->chip_cfg.gyro_fsr = 0xFF;
    st->chip_cfg.accel_fsr = 0xFF;
    st->chip_cfg.lpf = 0xFF;
    st->chip_cfg.sample_rate = 0xFFFF;
    st->chip_cfg.sensors = 0xFF;
    st->chip_cfg.fifo_enable = 0xFF;
    st->chip_cfg.clk_src = INV_CLK_PLL;
    mpu_set_sensors(st->chip_cfg.cache.sensors_on);
    mpu_set_gyro_fsr(st->chip_cfg.cache.gyro_fsr);
    mpu_set_accel_fsr(st->chip_cfg.cache.accel_fsr);
    mpu_set_lpf(st->chip_cfg.cache.lpf);
    mpu_set_sample_rate(st->chip_cfg.cache.sample_rate);
    mpu_configure_fifo(st->chip_cfg.cache.fifo_sensors);

    if (st->chip_cfg.cache.dmp_on)
        mpu_set_dmp_state(1);

#ifdef MPU6500
    /* Disable motion interrupt (MPU6500 version). */
    data[0] = 0;
    if (i2c_write(st->hw->addr, st->reg->accel_intel, 1, data))
        goto lp_int_restore;
#endif

    st->chip_cfg.int_motion_only = 0;
    return 0;
}

#ifdef AK89xx_SECONDARY
/* This initialization is similar to the one in ak8975.c. */
static int setup_compass(void)
{
    unsigned char data[4], akm_addr;

    mpu_set_bypass(1);

    /* Find compass. Possible addresses range from 0x0C to 0x0F. */
    for (akm_addr = 0x0C; akm_addr <= 0x0F; akm_addr++) {
        int result;
        result = i2c_read(akm_addr, AKM_REG_WHOAMI, 1, data);
        if (!result && (data[0] == AKM_WHOAMI))
            break;
    }

    if (akm_addr > 0x0F) {
        /* TODO: Handle this case in all compass-related functions. */
        log_e("Compass not found.\n");
        return -1;
    }

    st->chip_cfg.compass_addr = akm_addr;

    data[0] = AKM_POWER_DOWN;
    if (i2c_write(st->chip_cfg.compass_addr, AKM_REG_CNTL, 1, data))
        return -1;
    delay_ms(1);

    data[0] = AKM_FUSE_ROM_ACCESS;
    if (i2c_write(st->chip_cfg.compass_addr, AKM_REG_CNTL, 1, data))
        return -1;
    delay_ms(1);

    /* Get sensitivity adjustment data from fuse ROM. */
    if (i2c_read(st->chip_cfg.compass_addr, AKM_REG_ASAX, 3, data))
        return -1;
    st->chip_cfg.mag_sens_adj[0] = (long)data[0] + 128;
    st->chip_cfg.mag_sens_adj[1] = (long)data[1] + 128;
    st->chip_cfg.mag_sens_adj[2] = (long)data[2] + 128;

    data[0] = AKM_POWER_DOWN;
    if (i2c_write(st->chip_cfg.compass_addr, AKM_REG_CNTL, 1, data))
        return -1;
    delay_ms(1);

    mpu_set_bypass(0);

    /* Set up master mode, master clock, and ES bit. */
    data[0] = 0x40;
    if (i2c_write(st->hw->addr, st->reg->i2c_mst, 1, data))
        return -1;

    /* Slave 0 reads from AKM data registers. */
    data[0] = BIT_I2C_READ | st->chip_cfg.compass_addr;
    if (i2c_write(st->hw->addr, st->reg->s0_addr, 1, data))
        return -1;

    /* Compass reads start at this register. */
    data[0] = AKM_REG_ST1;
    if (i2c_write(st->hw->addr, st->reg->s0_reg, 1, data))
        return -1;

    /* Enable slave 0, 8-byte reads. */
    data[0] = BIT_SLAVE_EN | 8;
    if (i2c_write(st->hw->addr, st->reg->s0_ctrl, 1, data))
        return -1;

    /* Slave 1 changes AKM measurement mode. */
    data[0] = st->chip_cfg.compass_addr;
    if (i2c_write(st->hw->addr, st->reg->s1_addr, 1, data))
        return -1;

    /* AKM measurement mode register. */
    data[0] = AKM_REG_CNTL;
    if (i2c_write(st->hw->addr, st->reg->s1_reg, 1, data))
        return -1;

    /* Enable slave 1, 1-byte writes. */
    data[0] = BIT_SLAVE_EN | 1;
    if (i2c_write(st->hw->addr, st->reg->s1_ctrl, 1, data))
        return -1;

    /* Set slave 1 data. */
    data[0] = AKM_SINGLE_MEASUREMENT;
    if (i2c_write(st->hw->addr, st->reg->s1_do, 1, data))
        return -1;

    /* Trigger slave 0 and slave 1 actions at each sample. */
    data[0] = 0x03;
    if (i2c_write(st->hw->addr, st->reg->i2c_delay_ctrl, 1, data))
        return -1;

#ifdef MPU9150
    /* For the MPU9150, the auxiliary I2C bus needs to be set to VDD. */
    data[0] = BIT_I2C_MST_VDDIO;
    if (i2c_write(st->hw->addr, st->reg->yg_offs_tc, 1, data))
        return -1;
#endif

    return 0;
}
#endif

/* Handle sensor on/off combinations. */
static void setup_gyro(void)
{
    unsigned char mask = 0;
    if (hal->sensors & ACCEL_ON)
        mask |= INV_XYZ_ACCEL;
    if (hal->sensors & GYRO_ON)
        mask |= INV_XYZ_GYRO;
    /* If you need a power transition, this function should be called with a
     * mask of the sensors still enabled. The driver turns off any sensors
     * excluded from this mask.
     */
    mpu_set_sensors(mask);
    if (!hal->dmp_on)
        mpu_configure_fifo(mask);
}

#ifdef AK89xx_SECONDARY
static int compass_self_test(void)
{
    unsigned char tmp[6];
    unsigned char tries = 10;
    int result = 0x07;
    short data;

    mpu_set_bypass(1);

    tmp[0] = AKM_POWER_DOWN;
    if (i2c_write(st->chip_cfg.compass_addr, AKM_REG_CNTL, 1, tmp))
        return 0x07;
    tmp[0] = AKM_BIT_SELF_TEST;
    if (i2c_write(st->chip_cfg.compass_addr, AKM_REG_ASTC, 1, tmp))
        goto AKM_restore;
    tmp[0] = AKM_MODE_SELF_TEST;
    if (i2c_write(st->chip_cfg.compass_addr, AKM_REG_CNTL, 1, tmp))
        goto AKM_restore;

    do {
        delay_ms(10);
        if (i2c_read(st->chip_cfg.compass_addr, AKM_REG_ST1, 1, tmp))
            goto AKM_restore;
        if (tmp[0] & AKM_DATA_READY)
            break;
    } while (tries--);
    if (!(tmp[0] & AKM_DATA_READY))
        goto AKM_restore;

    if (i2c_read(st->chip_cfg.compass_addr, AKM_REG_HXL, 6, tmp))
        goto AKM_restore;

    result = 0;
#if defined MPU9150
    data = (short)(tmp[1] << 8) | tmp[0];
    if ((data > 100) || (data < -100))
        result |= 0x01;
    data = (short)(tmp[3] << 8) | tmp[2];
    if ((data > 100) || (data < -100))
        result |= 0x02;
    data = (short)(tmp[5] << 8) | tmp[4];
    if ((data > -300) || (data < -1000))
        result |= 0x04;
#elif defined MPU9250
    data = (short)(tmp[1] << 8) | tmp[0];
    if ((data > 200) || (data < -200))  
        result |= 0x01;
    data = (short)(tmp[3] << 8) | tmp[2];
    if ((data > 200) || (data < -200))  
        result |= 0x02;
    data = (short)(tmp[5] << 8) | tmp[4];
    if ((data > -800) || (data < -3200))  
        result |= 0x04;
#endif
AKM_restore:
    tmp[0] = 0 | SUPPORTS_AK89xx_HIGH_SENS;
    i2c_write(st->chip_cfg.compass_addr, AKM_REG_ASTC, 1, tmp);
    tmp[0] = SUPPORTS_AK89xx_HIGH_SENS;
    i2c_write(st->chip_cfg.compass_addr, AKM_REG_CNTL, 1, tmp);
    mpu_set_bypass(0);
    return result;
}
#endif


#ifdef MPU6500
#define REG_6500_XG_ST_DATA     0x0
#define REG_6500_XA_ST_DATA     0xD
static const unsigned short mpu_6500_st_tb[256] = {
	2620,2646,2672,2699,2726,2753,2781,2808, //7
	2837,2865,2894,2923,2952,2981,3011,3041, //15
	3072,3102,3133,3165,3196,3228,3261,3293, //23
	3326,3359,3393,3427,3461,3496,3531,3566, //31
	3602,3638,3674,3711,3748,3786,3823,3862, //39
	3900,3939,3979,4019,4059,4099,4140,4182, //47
	4224,4266,4308,4352,4395,4439,4483,4528, //55
	4574,4619,4665,4712,4759,4807,4855,4903, //63
	4953,5002,5052,5103,5154,5205,5257,5310, //71
	5363,5417,5471,5525,5581,5636,5693,5750, //79
	5807,5865,5924,5983,6043,6104,6165,6226, //87
	6289,6351,6415,6479,6544,6609,6675,6742, //95
	6810,6878,6946,7016,7086,7157,7229,7301, //103
	7374,7448,7522,7597,7673,7750,7828,7906, //111
	7985,8065,8145,8227,8309,8392,8476,8561, //119
	8647,8733,8820,8909,8998,9088,9178,9270,
	9363,9457,9551,9647,9743,9841,9939,10038,
	10139,10240,10343,10446,10550,10656,10763,10870,
	10979,11089,11200,11312,11425,11539,11654,11771,
	11889,12008,12128,12249,12371,12495,12620,12746,
	12874,13002,13132,13264,13396,13530,13666,13802,
	13940,14080,14221,14363,14506,14652,14798,14946,
	15096,15247,15399,15553,15709,15866,16024,16184,
	16346,16510,16675,16842,17010,17180,17352,17526,
	17701,17878,18057,18237,18420,18604,18790,18978,
	19167,19359,19553,19748,19946,20145,20347,20550,
	20756,20963,21173,21385,21598,21814,22033,22253,
	22475,22700,22927,23156,23388,23622,23858,24097,
	24338,24581,24827,25075,25326,25579,25835,26093,
	26354,26618,26884,27153,27424,27699,27976,28255,
	28538,28823,29112,29403,29697,29994,30294,30597,
	30903,31212,31524,31839,32157,32479,32804,33132
};


static int accel_6500_self_test(long *bias_regular, long *bias_st, int debug)
{
    int i, result = 0, otp_value_zero = 0;
    float accel_st_al_min, accel_st_al_max;
    float st_shift_cust[3], st_shift_ratio[3], ct_shift_prod[3], accel_offset_max;
    unsigned char regs[3];
    if (i2c_read(st->hw->addr, REG_6500_XA_ST_DATA, 3, regs)) {
    	if(debug)
    		log_i("Reading OTP Register Error.\n");
    	return 0x07;
    }
    if(debug)
    	log_i("Accel OTP:%d, %d, %d\n", regs[0], regs[1], regs[2]);
	for (i = 0; i < 3; i++) {
		if (regs[i] != 0) {
			ct_shift_prod[i] = mpu_6500_st_tb[regs[i] - 1];
			ct_shift_prod[i] *= 65536.f;
			ct_shift_prod[i] /= test.accel_sens;
		}
		else {
			ct_shift_prod[i] = 0;
			otp_value_zero = 1;
		}
	}
	if(otp_value_zero == 0) {
		if(debug)
			log_i("ACCEL:CRITERIA A\n");
		for (i = 0; i < 3; i++) {
			st_shift_cust[i] = bias_st[i] - bias_regular[i];
			if(debug) {
				log_i("Bias_Shift=%7.4f, Bias_Reg=%7.4f, Bias_HWST=%7.4f\r\n",
						st_shift_cust[i]/1.f, bias_regular[i]/1.f,
						bias_st[i]/1.f);
				log_i("OTP value: %7.4f\r\n", ct_shift_prod[i]/1.f);
			}

			st_shift_ratio[i] = st_shift_cust[i] / ct_shift_prod[i] - 1.f;

			if(debug)
				log_i("ratio=%7.4f, threshold=%7.4f\r\n", st_shift_ratio[i]/1.f,
							test.max_accel_var/1.f);

			if (fabs(st_shift_ratio[i]) > test.max_accel_var) {
				if(debug)
					log_i("ACCEL Fail Axis = %d\n", i);
				result |= 1 << i;	//Error condition
			}
		}
	}
	else {
		/* Self Test Pass/Fail Criteria B */
		accel_st_al_min = test.min_g * 65536.f;
		accel_st_al_max = test.max_g * 65536.f;

		if(debug) {
			log_i("ACCEL:CRITERIA B\r\n");
			log_i("Min MG: %7.4f\r\n", accel_st_al_min/1.f);
			log_i("Max MG: %7.4f\r\n", accel_st_al_max/1.f);
		}

		for (i = 0; i < 3; i++) {
			st_shift_cust[i] = bias_st[i] - bias_regular[i];

			if(debug)
				log_i("Bias_shift=%7.4f, st=%7.4f, reg=%7.4f\n", st_shift_cust[i]/1.f, bias_st[i]/1.f, bias_regular[i]/1.f);
			if(st_shift_cust[i] < accel_st_al_min || st_shift_cust[i] > accel_st_al_max) {
				if(debug)
					log_i("Accel FAIL axis:%d <= 225mg or >= 675mg\n", i);
				result |= 1 << i;	//Error condition
			}
		}
	}

	if(result == 0) {
	/* Self Test Pass/Fail Criteria C */
		accel_offset_max = test.max_g_offset * 65536.f;
		if(debug)
			log_i("Accel:CRITERIA C: bias less than %7.4f\n", accel_offset_max/1.f);
		for (i = 0; i < 3; i++) {
			if(fabs(bias_regular[i]) > accel_offset_max) {
				if(debug)
					log_i("FAILED: Accel axis:%d = %d > 500mg\n", i, bias_regular[i]);
				result |= 1 << i;	//Error condition
			}
		}
	}

    return result;
}

static int gyro_6500_self_test(long *bias_regular, long *bias_st, int debug)
{
    int i, result = 0, otp_value_zero = 0;
    float gyro_st_al_max;
    float st_shift_cust[3], st_shift_ratio[3], ct_shift_prod[3], gyro_offset_max;
    unsigned char regs[3];

    if (i2c_read(st->hw->addr, REG_6500_XG_ST_DATA, 3, regs)) {
    	if(debug)
    		log_i("Reading OTP Register Error.\n");
        return 0x07;
    }

    if(debug)
    	log_i("Gyro OTP:%d, %d, %d\r\n", regs[0], regs[1], regs[2]);

	for (i = 0; i < 3; i++) {
		if (regs[i] != 0) {
			ct_shift_prod[i] = mpu_6500_st_tb[regs[i] - 1];
			ct_shift_prod[i] *= 65536.f;
			ct_shift_prod[i] /= test.gyro_sens;
		}
		else {
			ct_shift_prod[i] = 0;
			otp_value_zero = 1;
		}
	}

	if(otp_value_zero == 0) {
		if(debug)
			log_i("GYRO:CRITERIA A\n");
		/* Self Test Pass/Fail Criteria A */
		for (i = 0; i < 3; i++) {
			st_shift_cust[i] = bias_st[i] - bias_regular[i];

			if(debug) {
				log_i("Bias_Shift=%7.4f, Bias_Reg=%7.4f, Bias_HWST=%7.4f\r\n",
						st_shift_cust[i]/1.f, bias_regular[i]/1.f,
						bias_st[i]/1.f);
				log_i("OTP value: %7.4f\r\n", ct_shift_prod[i]/1.f);
			}

			st_shift_ratio[i] = st_shift_cust[i] / ct_shift_prod[i];

			if(debug)
				log_i("ratio=%7.4f, threshold=%7.4f\r\n", st_shift_ratio[i]/1.f,
							test.max_gyro_var/1.f);

			if (fabs(st_shift_ratio[i]) < test.max_gyro_var) {
				if(debug)
					log_i("Gyro Fail Axis = %d\n", i);
				result |= 1 << i;	//Error condition
			}
		}
	}
	else {
		/* Self Test Pass/Fail Criteria B */
		gyro_st_al_max = test.max_dps * 65536.f;

		if(debug) {
			log_i("GYRO:CRITERIA B\r\n");
			log_i("Max DPS: %7.4f\r\n", gyro_st_al_max/1.f);
		}

		for (i = 0; i < 3; i++) {
			st_shift_cust[i] = bias_st[i] - bias_regular[i];

			if(debug)
				log_i("Bias_shift=%7.4f, st=%7.4f, reg=%7.4f\n", st_shift_cust[i]/1.f, bias_st[i]/1.f, bias_regular[i]/1.f);
			if(st_shift_cust[i] < gyro_st_al_max) {
				if(debug)
					log_i("GYRO FAIL axis:%d greater than 60dps\n", i);
				result |= 1 << i;	//Error condition
			}
		}
	}

	if(result == 0) {
	/* Self Test Pass/Fail Criteria C */
		gyro_offset_max = test.min_dps * 65536.f;
		if(debug)
			log_i("Gyro:CRITERIA C: bias less than %7.4f\n", gyro_offset_max/1.f);
		for (i = 0; i < 3; i++) {
			if(fabs(bias_regular[i]) > gyro_offset_max) {
				if(debug)
					log_i("FAILED: Gyro axis:%d = %d > 20dps\n", i, bias_regular[i]);
				result |= 1 << i;	//Error condition
			}
		}
	}
    return result;
}

static int get_st_6500_biases(long *gyro, long *accel, unsigned char hw_test, int debug)
{
    unsigned char data[HWST_MAX_PACKET_LENGTH];
    unsigned char packet_count, ii;
    unsigned short fifo_count;
    int s = 0, read_size = 0, ind;

    data[0] = 0x01;
    data[1] = 0;
    if (i2c_write(st->hw->addr, st->reg->pwr_mgmt_1, 2, data))
        return -1;
    delay_ms(200);
    data[0] = 0;
    if (i2c_write(st->hw->addr, st->reg->int_enable, 1, data))
        return -1;
    if (i2c_write(st->hw->addr, st->reg->fifo_en, 1, data))
        return -1;
    if (i2c_write(st->hw->addr, st->reg->pwr_mgmt_1, 1, data))
        return -1;
    if (i2c_write(st->hw->addr, st->reg->i2c_mst, 1, data))
        return -1;
    if (i2c_write(st->hw->addr, st->reg->user_ctrl, 1, data))
        return -1;
    data[0] = BIT_FIFO_RST | BIT_DMP_RST;
    if (i2c_write(st->hw->addr, st->reg->user_ctrl, 1, data))
        return -1;
    delay_ms(15);
    data[0] = st->test->reg_lpf;
    if (i2c_write(st->hw->addr, st->reg->lpf, 1, data))
        return -1;
    data[0] = st->test->reg_rate_div;
    if (i2c_write(st->hw->addr, st->reg->rate_div, 1, data))
        return -1;
    if (hw_test)
        data[0] = st->test->reg_gyro_fsr | 0xE0;
    else
        data[0] = st->test->reg_gyro_fsr;
    if (i2c_write(st->hw->addr, st->reg->gyro_cfg, 1, data))
        return -1;

    if (hw_test)
        data[0] = st->test->reg_accel_fsr | 0xE0;
    else
        data[0] = test.reg_accel_fsr;
    if (i2c_write(st->hw->addr, st->reg->accel_cfg, 1, data))
        return -1;

    delay_ms(test.wait_ms);  //wait 200ms for sensors to stabilize

    /* Enable FIFO */
    data[0] = BIT_FIFO_EN;
    if (i2c_write(st->hw->addr, st->reg->user_ctrl, 1, data))
        return -1;
    data[0] = INV_XYZ_GYRO | INV_XYZ_ACCEL;
    if (i2c_write(st->hw->addr, st->reg->fifo_en, 1, data))
        return -1;

    //initialize the bias return values
    gyro[0] = gyro[1] = gyro[2] = 0;
    accel[0] = accel[1] = accel[2] = 0;

    if(debug)
    	log_i("Starting Bias Loop Reads\n");

    //start reading samples
    while (s < test.packet_thresh) {
    	delay_ms(test.sample_wait_ms); //wait 10ms to fill FIFO
		if (i2c_read(st->hw->addr, st->reg->fifo_count_h, 2, data))
			return -1;
		fifo_count = (data[0] << 8) | data[1];
		packet_count = fifo_count / MAX_PACKET_LENGTH;
		if ((test.packet_thresh - s) < packet_count)
		            packet_count = test.packet_thresh - s;
		read_size = packet_count * MAX_PACKET_LENGTH;

		//burst read from FIFO
		if (i2c_read(st->hw->addr, st->reg->fifo_r_w, read_size, data))
						return -1;
		ind = 0;
		for (ii = 0; ii < packet_count; ii++) {
			short accel_cur[3], gyro_cur[3];
			accel_cur[0] = ((short)data[ind + 0] << 8) | data[ind + 1];
			accel_cur[1] = ((short)data[ind + 2] << 8) | data[ind + 3];
			accel_cur[2] = ((short)data[ind + 4] << 8) | data[ind + 5];
			accel[0] += (long)accel_cur[0];
			accel[1] += (long)accel_cur[1];
			accel[2] += (long)accel_cur[2];
			gyro_cur[0] = (((short)data[ind + 6] << 8) | data[ind + 7]);
			gyro_cur[1] = (((short)data[ind + 8] << 8) | data[ind + 9]);
			gyro_cur[2] = (((short)data[ind + 10] << 8) | data[ind + 11]);
			gyro[0] += (long)gyro_cur[0];
			gyro[1] += (long)gyro_cur[1];
			gyro[2] += (long)gyro_cur[2];
			ind += MAX_PACKET_LENGTH;
		}
		s += packet_count;
    }

    if(debug)
    	log_i("Samples: %d\n", s);

    //stop FIFO
    data[0] = 0;
    if (i2c_write(st->hw->addr, st->reg->fifo_en, 1, data))
        return -1;

    gyro[0] = (long)(((long long)gyro[0]<<16) / test.gyro_sens / s);
    gyro[1] = (long)(((long long)gyro[1]<<16) / test.gyro_sens / s);
    gyro[2] = (long)(((long long)gyro[2]<<16) / test.gyro_sens / s);
    accel[0] = (long)(((long long)accel[0]<<16) / test.accel_sens / s);
    accel[1] = (long)(((long long)accel[1]<<16) / test.accel_sens / s);
    accel[2] = (long)(((long long)accel[2]<<16) / test.accel_sens / s);
    /* remove gravity from bias calculation */
    if (accel[2] > 0L)
        accel[2] -= 65536L;
    else
        accel[2] += 65536L;


    if(debug) {
    	log_i("Accel offset data HWST bit=%d: %7.4f %7.4f %7.4f\r\n", hw_test, accel[0]/65536.f, accel[1]/65536.f, accel[2]/65536.f);
    	log_i("Gyro offset data HWST bit=%d: %7.4f %7.4f %7.4f\r\n", hw_test, gyro[0]/65536.f, gyro[1]/65536.f, gyro[2]/65536.f);
    }

    return 0;
}
/**
 *  @brief      Trigger gyro/accel/compass self-test for MPU6500/MPU9250
 *  On success/error, the self-test returns a mask representing the sensor(s)
 *  that failed. For each bit, a one (1) represents a "pass" case; conversely,
 *  a zero (0) indicates a failure.
 *
 *  \n The mask is defined as follows:
 *  \n Bit 0:   Gyro.
 *  \n Bit 1:   Accel.
 *  \n Bit 2:   Compass.
 *
 *  @param[out] gyro        Gyro biases in q16 format.
 *  @param[out] accel       Accel biases (if applicable) in q16 format.
 *  @param[in]  debug       Debug flag used to print out more detailed logs. Must first set up logging in Motion Driver.
 *  @return     Result mask (see above).
 */
static int mpu_run_6500_self_test(long *gyro, long *accel, unsigned char debug)
{
    const unsigned char tries = 2;
    long gyro_st[3], accel_st[3];
    unsigned char accel_result, gyro_result;
#ifdef AK89xx_SECONDARY
    unsigned char compass_result;
#endif
    int ii;

    int result;
    unsigned char accel_fsr, fifo_sensors, sensors_on;
    unsigned short gyro_fsr, sample_rate, lpf;
    unsigned char dmp_was_on;



    if(debug)
    	log_i("Starting MPU6500 HWST!\r\n");

    if (st->chip_cfg.dmp_on) {
        mpu_set_dmp_state(0);
        dmp_was_on = 1;
    } else
        dmp_was_on = 0;

    /* Get initial settings. */
    mpu_get_gyro_fsr(&gyro_fsr);
    mpu_get_accel_fsr(&accel_fsr);
    mpu_get_lpf(&lpf);
    mpu_get_sample_rate(&sample_rate);
    sensors_on = st->chip_cfg.sensors;
    mpu_get_fifo_config(&fifo_sensors);

    if(debug)
    	log_i("Retrieving Biases\r\n");

    for (ii = 0; ii < tries; ii++)
        if (!get_st_6500_biases(gyro, accel, 0, debug))
            break;
    if (ii == tries) {
        /* If we reach this point, we most likely encountered an I2C error.
         * We'll just report an error for all three sensors.
         */
        if(debug)
        	log_i("Retrieving Biases Error - possible I2C error\n");

        result = 0;
        goto restore;
    }

    if(debug)
    	log_i("Retrieving ST Biases\n");

    for (ii = 0; ii < tries; ii++)
        if (!get_st_6500_biases(gyro_st, accel_st, 1, debug))
            break;
    if (ii == tries) {

        if(debug)
        	log_i("Retrieving ST Biases Error - possible I2C error\n");

        /* Again, probably an I2C error. */
        result = 0;
        goto restore;
    }

    accel_result = accel_6500_self_test(accel, accel_st, debug);
    if(debug)
    	log_i("Accel Self Test Results: %d\n", accel_result);

    gyro_result = gyro_6500_self_test(gyro, gyro_st, debug);
    if(debug)
    	log_i("Gyro Self Test Results: %d\n", gyro_result);

    result = 0;
    if (!gyro_result)
        result |= 0x01;
    if (!accel_result)
        result |= 0x02;

#ifdef AK89xx_SECONDARY
    compass_result = compass_self_test();
    if(debug)
    	log_i("Compass Self Test Results: %d\n", compass_result);
    if (!compass_result)
        result |= 0x04;
#else
    result |= 0x04;
#endif
restore:
	if(debug)
		log_i("Exiting HWST\n");
	/* Set to invalid values to ensure no I2C writes are skipped. */
	st->chip_cfg.gyro_fsr = 0xFF;
	st->chip_cfg.accel_fsr = 0xFF;
	st->chip_cfg.lpf = 0xFF;
	st->chip_cfg.sample_rate = 0xFFFF;
	st->chip_cfg.sensors = 0xFF;
	st->chip_cfg.fifo_enable = 0xFF;
	st->chip_cfg.clk_src = INV_CLK_PLL;
	mpu_set_gyro_fsr(gyro_fsr);
	mpu_set_accel_fsr(accel_fsr);
	mpu_set_lpf(lpf);
	mpu_set_sample_rate(sample_rate);
	mpu_set_sensors(sensors_on);
	mpu_configure_fifo(fifo_sensors);

	if (dmp_was_on)
		mpu_set_dmp_state(1);

	return result;
}
#endif

static int mpu_read_6500_gyro_bias(long *gyro_bias) {
	unsigned char data[6];
	if (i2c_read(st->hw->addr, 0x13, 2, &data[0]))
		return -1;
	if (i2c_read(st->hw->addr, 0x15, 2, &data[2]))
		return -1;
	if (i2c_read(st->hw->addr, 0x17, 2, &data[4]))
		return -1;
	gyro_bias[0] = ((long)data[0]<<8) | data[1];
	gyro_bias[1] = ((long)data[2]<<8) | data[3];
	gyro_bias[2] = ((long)data[4]<<8) | data[5];
	return 0;
}

/**
 *  @brief      Push biases to the gyro bias 6500/6050 registers.
 *  This function expects biases relative to the current sensor output, and
 *  these biases will be added to the factory-supplied values. Bias inputs are LSB
 *  in +-1000dps format.
 *  @param[in]  gyro_bias  New biases.
 *  @return     0 if successful.
 */
static int mpu_set_gyro_bias_reg(long *gyro_bias)
{
    unsigned char data[6] = {0, 0, 0, 0, 0, 0};
    long gyro_reg_bias[3] = {0, 0, 0};
    int i=0;
    
    if(mpu_read_6500_gyro_bias(gyro_reg_bias))
        return -1;

    for(i=0;i<3;i++) {
        gyro_reg_bias[i]-= gyro_bias[i];
    }
    
    data[0] = (gyro_reg_bias[0] >> 8) & 0xff;
    data[1] = (gyro_reg_bias[0]) & 0xff;
    data[2] = (gyro_reg_bias[1] >> 8) & 0xff;
    data[3] = (gyro_reg_bias[1]) & 0xff;
    data[4] = (gyro_reg_bias[2] >> 8) & 0xff;
    data[5] = (gyro_reg_bias[2]) & 0xff;
    
    if (i2c_write(st->hw->addr, 0x13, 2, &data[0]))
        return -1;
    if (i2c_write(st->hw->addr, 0x15, 2, &data[2]))
        return -1;
    if (i2c_write(st->hw->addr, 0x17, 2, &data[4]))
        return -1;
    return 0;
}

/**
 *  @brief      Read biases to the accel bias 6500 registers.
 *  This function reads from the MPU6500 accel offset cancellations registers.
 *  The format are G in +-8G format. The register is initialized with OTP 
 *  factory trim values.
 *  @param[in]  accel_bias  returned structure with the accel bias
 *  @return     0 if successful.
 */
static int mpu_read_6500_accel_bias(long *accel_bias) {
	unsigned char data[6];
	if (i2c_read(st->hw->addr, 0x77, 2, &data[0]))
		return -1;
	if (i2c_read(st->hw->addr, 0x7A, 2, &data[2]))
		return -1;
	if (i2c_read(st->hw->addr, 0x7D, 2, &data[4]))
		return -1;
	accel_bias[0] = ((long)data[0]<<8) | data[1];
	accel_bias[1] = ((long)data[2]<<8) | data[3];
	accel_bias[2] = ((long)data[4]<<8) | data[5];
	return 0;
}

/**
 *  @brief      Push biases to the accel bias 6500 registers.
 *  This function expects biases relative to the current sensor output, and
 *  these biases will be added to the factory-supplied values. Bias inputs are LSB
 *  in +-8G format.
 *  @param[in]  accel_bias  New biases.
 *  @return     0 if successful.
 */
static int mpu_set_accel_bias_6500_reg(const long *accel_bias) {
    unsigned char data[6] = {0, 0, 0, 0, 0, 0};
    long accel_reg_bias[3] = {0, 0, 0};

    if(mpu_read_6500_accel_bias(accel_reg_bias))
        return -1;

    // Preserve bit 0 of factory value (for temperature compensation)
    accel_reg_bias[0] -= (accel_bias[0] & ~1);
    accel_reg_bias[1] -= (accel_bias[1] & ~1);
    accel_reg_bias[2] -= (accel_bias[2] & ~1);

    data[0] = (accel_reg_bias[0] >> 8) & 0xff;
    data[1] = (accel_reg_bias[0]) & 0xff;
    data[2] = (accel_reg_bias[1] >> 8) & 0xff;
    data[3] = (accel_reg_bias[1]) & 0xff;
    data[4] = (accel_reg_bias[2] >> 8) & 0xff;
    data[5] = (accel_reg_bias[2]) & 0xff;

    if (i2c_write(st->hw->addr, 0x77, 2, &data[0]))
        return -1;
    if (i2c_write(st->hw->addr, 0x7A, 2, &data[2]))
        return -1;
    if (i2c_write(st->hw->addr, 0x7D, 2, &data[4]))
        return -1;

    return 0;
}
static inline void run_self_test(void)
{
    int result;
    char test_packet[4] = {0};
    long gyro[3], accel[3];
    unsigned char i = 0;

    result = mpu_run_6500_self_test(gyro, accel, 1);
    if (result == 0x7) {
        /* Test passed. We can trust the gyro data here, so let's push it down
         * to the DMP.
         */
        for(i = 0; i<3; i++) {
        	gyro[i] = (long)(gyro[i] * 32.8f); //convert to +-1000dps
        	accel[i] *= 2048.f; //convert to +-16G
        	accel[i] = accel[i] >> 16;
        	gyro[i] = (long)(gyro[i] >> 16);
        }

        mpu_set_gyro_bias_reg(gyro);

        mpu_set_accel_bias_6500_reg(accel);
    }

    /* Report results. */
    test_packet[0] = 't';
    test_packet[1] = result;
    send_packet(PACKET_TYPE_MISC, test_packet);
}

/**
 *  @brief      Specify when a DMP interrupt should occur.
 *  A DMP interrupt can be configured to trigger on either of the two
 *  conditions below:
 *  \n a. One FIFO period has elapsed (set by @e mpu_set_sample_rate).
 *  \n b. A tap event has been detected.
 *  @param[in]  mode    DMP_INT_GESTURE or DMP_INT_CONTINUOUS.
 *  @return     0 if successful.
 */
static int dmp_set_interrupt_mode(unsigned char mode)
{
    const unsigned char regs_continuous[11] =
        {0xd8, 0xb1, 0xb9, 0xf3, 0x8b, 0xa3, 0x91, 0xb6, 0x09, 0xb4, 0xd9};
    const unsigned char regs_gesture[11] =
        {0xda, 0xb1, 0xb9, 0xf3, 0x8b, 0xa3, 0x91, 0xb6, 0xda, 0xb4, 0xda};

    switch (mode) {
    case DMP_INT_CONTINUOUS:
        return mpu_write_mem(CFG_FIFO_ON_EVENT, 11,
            (unsigned char*)regs_continuous);
    case DMP_INT_GESTURE:
        return mpu_write_mem(CFG_FIFO_ON_EVENT, 11,
            (unsigned char*)regs_gesture);
    default:
        return -1;
    }
}

/**
 *  @brief      Get current step count.
 *  @param[out] count   Number of steps detected.
 *  @return     0 if successful.
 */
static int dmp_get_pedometer_step_count(unsigned long *count)
{
    unsigned char tmp[4];
    if (!count)
        return -1;

    if (mpu_read_mem(D_PEDSTD_STEPCTR, 4, tmp))
        return -1;

    count[0] = ((unsigned long)tmp[0] << 24) | ((unsigned long)tmp[1] << 16) |
        ((unsigned long)tmp[2] << 8) | tmp[3];
    return 0;
}

/**
 *  @brief      Overwrite current step count.
 *  WARNING: This function writes to DMP memory and could potentially encounter
 *  a race condition if called while the pedometer is enabled.
 *  @param[in]  count   New step count.
 *  @return     0 if successful.
 */
static int dmp_set_pedometer_step_count(unsigned long count)
{
    unsigned char tmp[4];

    tmp[0] = (unsigned char)((count >> 24) & 0xFF);
    tmp[1] = (unsigned char)((count >> 16) & 0xFF);
    tmp[2] = (unsigned char)((count >> 8) & 0xFF);
    tmp[3] = (unsigned char)(count & 0xFF);
    return mpu_write_mem(D_PEDSTD_STEPCTR, 4, tmp);
}

/**
 *  @brief      Get duration of walking time.
 *  @param[in]  time    Walk time in milliseconds.
 *  @return     0 if successful.
 */
static int dmp_get_pedometer_walk_time(unsigned long *time)
{
    unsigned char tmp[4];
    if (!time)
        return -1;

    if (mpu_read_mem(D_PEDSTD_TIMECTR, 4, tmp))
        return -1;

    time[0] = (((unsigned long)tmp[0] << 24) | ((unsigned long)tmp[1] << 16) |
        ((unsigned long)tmp[2] << 8) | tmp[3]) * 20;
    return 0;
}

/**
 *  @brief      Overwrite current walk time.
 *  WARNING: This function writes to DMP memory and could potentially encounter
 *  a race condition if called while the pedometer is enabled.
 *  @param[in]  time    New walk time in milliseconds.
 */
static int dmp_set_pedometer_walk_time(unsigned long time)
{
    unsigned char tmp[4];

    time /= 20;

    tmp[0] = (unsigned char)((time >> 24) & 0xFF);
    tmp[1] = (unsigned char)((time >> 16) & 0xFF);
    tmp[2] = (unsigned char)((time >> 8) & 0xFF);
    tmp[3] = (unsigned char)(time & 0xFF);
    return mpu_write_mem(D_PEDSTD_TIMECTR, 4, tmp);
}

/**
 *  @brief      Get DMP output rate.
 *  @param[out] rate    Current fifo rate (Hz).
 *  @return     0 if successful.
 */
static int dmp_get_fifo_rate(unsigned short *rate)
{
    rate[0] = dmp.fifo_rate;
    return 0;
}

/**
 *  @brief      Initialize hardware.
 *  Initial configuration:\n
 *  Gyro FSR: +/- 2000DPS\n
 *  Accel FSR +/- 2G\n
 *  DLPF: 42Hz\n
 *  FIFO rate: 50Hz\n
 *  Clock source: Gyro PLL\n
 *  FIFO: Disabled.\n
 *  Data ready interrupt: Disabled, active low, unlatched.
 *  @param[in]  int_param   Platform-specific parameters to interrupt API.
 *  @return     0 if successful.
 */
static int gyro_init(void)
{
    unsigned char data[6];

    /* Reset device. */
    data[0] = BIT_RESET;
    if (i2c_write(st->hw->addr, st->reg->pwr_mgmt_1, 1, data))
        return -1;
    delay_ms(100);

    /* Wake up chip. */
    data[0] = 0x00;
    if (i2c_write(st->hw->addr, st->reg->pwr_mgmt_1, 1, data))
        return -1;

   st->chip_cfg.accel_half = 0;

#ifdef MPU6500
    /* MPU6500 shares 4kB of memory between the DMP and the FIFO. Since the
     * first 3kB are needed by the DMP, we'll use the last 1kB for the FIFO.
     */
    data[0] = BIT_FIFO_SIZE_1024;
    if (i2c_write(st->hw->addr, st->reg->accel_cfg2, 1, data))
        return -1;
#endif

    /* Set to invalid values to ensure no I2C writes are skipped. */
    st->chip_cfg.sensors = 0xFF;
    st->chip_cfg.gyro_fsr = 0xFF;
    st->chip_cfg.accel_fsr = 0xFF;
    st->chip_cfg.lpf = 0xFF;
    st->chip_cfg.sample_rate = 0xFFFF;
    st->chip_cfg.fifo_enable = 0xFF;
    st->chip_cfg.bypass_mode = 0xFF;

    /* mpu_set_sensors always preserves this setting. */
    st->chip_cfg.clk_src = INV_CLK_PLL;
    /* Handled in next call to mpu_set_bypass. */
    st->chip_cfg.active_low_int = 1;
    st->chip_cfg.latched_int = 0;
    st->chip_cfg.int_motion_only = 0;
    st->chip_cfg.lp_accel_mode = 0;
    memset(&st->chip_cfg.cache, 0, sizeof(st->chip_cfg.cache));
    st->chip_cfg.dmp_on = 0;
    st->chip_cfg.dmp_loaded = 0;
    st->chip_cfg.dmp_sample_rate = 0;

    if (mpu_set_gyro_fsr(1000))
        return -1;
    if (mpu_set_accel_fsr(2))
        return -1;
    if (mpu_set_lpf(42))
        return -1;
    if (mpu_set_sample_rate(50))
        return -1;
    if (mpu_configure_fifo(0))
        return -1;


    /* Already disabled by setup_compass. */
    if (mpu_set_bypass(0))
        return -1;

    mpu_set_sensors(0);
    return 0;
}

static int handle_input(char c)
{
    const unsigned char header[3] = "inv";
    unsigned long pedo_packet[2];

    /* Read incoming byte and check for header.
     * Technically, the MSP430 USB stack can handle more than one byte at a
     * time. This example allows for easily switching to UART if porting to a
     * different microcontroller.
     */


    if (hal->rx.header[0] == header[0]) {
        if (hal->rx.header[1] == header[1]) {
            if (hal->rx.header[2] == header[2]) {
                memset(&hal->rx.header, 0, sizeof(hal->rx.header));
                hal->rx.cmd = c;
            } else if (c == header[2])
                hal->rx.header[2] = c;
            else
                memset(&hal->rx.header, 0, sizeof(hal->rx.header));
        } else if (c == header[1])
            hal->rx.header[1] = c;
        else
            memset(&hal->rx.header, 0, sizeof(hal->rx.header));
    } else if (c == header[0])
        hal->rx.header[0] = header[0];
    if (!hal->rx.cmd)
        return -1;

    printf("[GHAND] cmd:%c, ch:%c \n", hal->rx.cmd, c);
    
    switch (hal->rx.cmd) {
    /* These commands turn the hardware sensors on/off. */
    case '8':
        if (!hal->dmp_on) {
            /* Accel and gyro need to be on for the DMP features to work
             * properly.
             */
            hal->sensors ^= ACCEL_ON;
            setup_gyro();
        }
        break;
    case '9':
        if (!hal->dmp_on) {
            hal->sensors ^= GYRO_ON;
            setup_gyro();
        }
        break;
    /* The commands start/stop sending data to the client. */
    case 'a':
        hal->report ^= PRINT_ACCEL;
        break;
    case 'g':
        hal->report ^= PRINT_GYRO;
        break;
    case 'q':
        hal->report ^= PRINT_QUAT;
        break;
    /* The hardware self test can be run without any interaction with the
     * MPL since it's completely localized in the gyro driver. Logging is
     * assumed to be enabled; otherwise, a couple LEDs could probably be used
     * here to display the test results.
     */
    case 't':
        run_self_test();
        break;
    /* Depending on your application, sensor data may be needed at a faster or
     * slower rate. These commands can speed up or slow down the rate at which
     * the sensor data is pushed to the MPL.
     *
     * In this example, the compass rate is never changed.
     */
    case '1':
        if (hal->dmp_on)
            dmp_set_fifo_rate(10);
        else
            mpu_set_sample_rate(10);
        break;
    case '2':
        if (hal->dmp_on)
            dmp_set_fifo_rate(20);
        else
            mpu_set_sample_rate(20);
        break;
    case '3':
        if (hal->dmp_on)
            dmp_set_fifo_rate(40);
        else
            mpu_set_sample_rate(40);
        break;
    case '4':
        if (hal->dmp_on)
            dmp_set_fifo_rate(50);
        else
            mpu_set_sample_rate(50);
        break;
    case '5':
        if (hal->dmp_on)
            dmp_set_fifo_rate(100);
        else
            mpu_set_sample_rate(100);
        break;
    case '6':
        if (hal->dmp_on)
            dmp_set_fifo_rate(200);
        else
            mpu_set_sample_rate(200);
        break;
	case ',':
        /* Set hardware to interrupt on gesture event only. This feature is
         * useful for keeping the MCU asleep until the DMP detects as a tap or
         * orientation event.
         */
        dmp_set_interrupt_mode(DMP_INT_GESTURE);
        break;
    case '.':
        /* Set hardware to interrupt periodically. */
        dmp_set_interrupt_mode(DMP_INT_CONTINUOUS);
        break;
    case '7':
        /* Reset pedometer. */
        dmp_set_pedometer_step_count(0);
        dmp_set_pedometer_walk_time(0);
        break;
    case 'f':
        /* Toggle DMP. */
        if (hal->dmp_on) {
            unsigned short dmp_rate;
            hal->dmp_on = 0;
            mpu_set_dmp_state(0);
            /* Restore FIFO settings. */
            mpu_configure_fifo(INV_XYZ_ACCEL | INV_XYZ_GYRO);
            /* When the DMP is used, the hardware sampling rate is fixed at
             * 200Hz, and the DMP is configured to downsample the FIFO output
             * using the function dmp_set_fifo_rate. However, when the DMP is
             * turned off, the sampling rate remains at 200Hz. This could be
             * handled in inv_mpu.c, but it would need to know that
             * inv_mpu_dmp_motion_driver.c exists. To avoid this, we'll just
             * put the extra logic in the application layer.
             */
            dmp_get_fifo_rate(&dmp_rate);
            mpu_set_sample_rate(dmp_rate);
        } else {
            unsigned short sample_rate;
            hal->dmp_on = 1;
            /* Both gyro and accel must be on. */
            hal->sensors |= ACCEL_ON | GYRO_ON;
            mpu_set_sensors(INV_XYZ_ACCEL | INV_XYZ_GYRO);
            mpu_configure_fifo(INV_XYZ_ACCEL | INV_XYZ_GYRO);
            /* Preserve current FIFO rate. */
            mpu_get_sample_rate(&sample_rate);
            dmp_set_fifo_rate(sample_rate);
            mpu_set_dmp_state(1);
        }
        break;
    case 'm':
        /* Test the motion interrupt hardware feature. */
		hal->motion_int_mode = 1;
        break;
    case 'p':
        /* Read current pedometer count. */
        dmp_get_pedometer_step_count(pedo_packet);
        dmp_get_pedometer_walk_time(pedo_packet + 1);
        send_packet(PACKET_TYPE_PEDO, pedo_packet);
        break;
    case 'x':
        //msp430_reset();
        break;
    case 'v':
        /* Toggle LP quaternion.
         * The DMP features can be enabled/disabled at runtime. Use this same
         * approach for other features.
         */
        hal->dmp_features ^= DMP_FEATURE_6X_LP_QUAT;
        dmp_enable_feature(hal->dmp_features);
        break;
    default:
        return -2;
        break;
    }
    hal->rx.cmd = 0;

    return 0;
}

static inline float calab_abs_f(float f) 
{
    if (f < 0) {
        return -f;
    } else {
        return f;
    }
}

static inline double calab_abs_lf(double lf) 
{
    if (lf < 0) {
        return -lf;
    } else {
        return lf;
    }
}

static int calab_get_div_short(int *ary, int size, float avg, float *div)
{
    int ix=0;
    double fval, fcal, fdiv=0, fsiz, fret;

    if (!ary) return -1;
    if (size <= 0) return -2;
    if (!div) return -4;

    for (ix = 0; ix < size; ix++) {
        fval = ary[ix];
        fcal = calab_abs_lf(avg - fval);
        fdiv += fcal;
        //printf("%.4f - %.4lf = %.4lf (total: %.4lf)\n", avg, fval, fcal, fdiv);
    }

    fsiz = size;

    fret = fdiv / fsiz;
    *div = (float)fret;

    return 0;
}

static int calab_find_steady(struct calab_data_s *pcd, int val, int lsb)
{
    int max=0, ret=0;
    float avg, famt, fcnt;
    int cnt=0, amt=0;
    cnt = pcd->calab_count;
    max = CALAB_DATA_BUFF_SIZE;

    amt = pcd->calab_total;
    amt += val;
    pcd->calab_still[cnt] = val;
    pcd->calab_total = amt;

    cnt += 1;
    pcd->calab_count = cnt;

    if (cnt == max) {
       fcnt = cnt;
       famt = amt;
       avg = famt / fcnt;

       pcd->calab_avg = avg;

       ret = calab_get_div_short(pcd->calab_still, pcd->calab_count, pcd->calab_avg, &pcd->calab_div);
       if(ret) {
           printf("error!!!find div failed ret:%d !!!\n", ret);
           return -1;
       } else {
           printf("succeed to find steady val:%f div = %f\n", pcd->calab_avg, pcd->calab_div);
           pcd->calab_done = 1;
           return pcd->calab_count;
       }
    } else {
        //printf("continue to find div ret:%d, val: %d count: %d, amount: %d\n", ret, val, pcd->calab_count, pcd->calab_total);
    }

    return 0;
}

static int calab_accel(struct accelc_info_s *pacc, short *dacc, unsigned int lsb)
{
#define ACCEL_MAX 32768
    int ret=0;
    int acx=0, acy=0, acz=0;
    int cnt=0, amt=0, avg=0;
    float flsbthd;
    double flsbrang, range, rdiv, frangdiv;
    struct calab_data_s *pcdt, *pdmax, *pdmin;

    if ((pacc->accelc_status & 0x7) == 0x7) {    
        return pacc->accelc_status;
    }

    flsbthd = pacc->accelc_flsbthd;
    frangdiv = pacc->accelc_frangdiv;
    flsbrang = pacc->accelc_grange;
    
    acx = dacc[0];
    ret = 0;
    if (!(pacc->accelc_status & 0x1)) {
        ret = calab_find_steady(&pacc->accelc_xtmp, acx, lsb);
    }
    if (ret > 0) {
        //printf("\nCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC\n");
        pcdt = &pacc->accelc_xtmp;
        pdmax =& pacc->accelc_xmax;
        pdmin = &pacc->accelc_xmin;

        //printf("accelx div = %f, threshold = %f \n", pcdt->calab_div, flsbthd);

        if (pcdt->calab_div > flsbthd) {
            printf("accelx div = %f > %f \n", pcdt->calab_div, flsbthd);
        }
        else if (!pdmax->calab_done) {
            memcpy(pdmax, pcdt, sizeof(struct calab_data_s));
        } else {
            if (pdmax->calab_avg < pcdt->calab_avg) {
                memcpy(pdmax, pcdt, sizeof(struct calab_data_s));            
            } else {
                if (!pdmin->calab_done) {
                    memcpy(pdmin, pcdt, sizeof(struct calab_data_s));            
                } else {
                    if (pdmin->calab_avg > pcdt->calab_avg) {
                        memcpy(pdmin, pcdt, sizeof(struct calab_data_s));            
                    }
                }
            }
        }

        if ((pdmax->calab_done) && (pdmin->calab_done)) {
            range = pdmax->calab_avg - pdmin->calab_avg;
            rdiv = calab_abs_lf(flsbrang - range);
            if (rdiv < frangdiv) {
                printf("calab get x accel range = %lf, div = (%.2lf / %.2lf) \n", range, rdiv, frangdiv);              
                pacc->accelc_status |= 0x1;

                pacc->accelc_xradio = flsbrang / range;
            } else {
                printf("calab get x accel range = %.4lf but target = %.4lf\n", range, flsbrang);
            }
        } else if (pdmax->calab_done) {
            printf("calab get max x accel = %f \n", pdmax->calab_avg);
        } else if (pdmin->calab_done) {
            printf("calab get min x accel = %f \n", pdmin->calab_avg);
        }
        
        //printf("\nCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC\n");
        memset(pcdt, 0, sizeof(struct calab_data_s));
    }

    
    acy = dacc[1];
    ret = 0;
    if (!(pacc->accelc_status & 0x2)) {    
        ret = calab_find_steady(&pacc->accelc_ytmp, acy, lsb);
    }
    if (ret > 0) {
        //printf("\nCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC\n");
        pcdt = &pacc->accelc_ytmp;
        pdmax =& pacc->accelc_ymax;
        pdmin = &pacc->accelc_ymin;

        //printf("accelx div = %f, threshold = %f \n", pcdt->calab_div, flsbthd);

        if (pcdt->calab_div > flsbthd) {
            printf("accelx div = %f > %f \n", pcdt->calab_div, flsbthd);
        }
        else if (!pdmax->calab_done) {
            memcpy(pdmax, pcdt, sizeof(struct calab_data_s));
        } else {
            if (pdmax->calab_avg < pcdt->calab_avg) {
                memcpy(pdmax, pcdt, sizeof(struct calab_data_s));            
            } else {
                if (!pdmin->calab_done) {
                    memcpy(pdmin, pcdt, sizeof(struct calab_data_s));            
                } else {
                    if (pdmin->calab_avg > pcdt->calab_avg) {
                        memcpy(pdmin, pcdt, sizeof(struct calab_data_s));            
                    }
                }
            }
        }
        if ((pdmax->calab_done) && (pdmin->calab_done)) {
            range = pdmax->calab_avg - pdmin->calab_avg;
            rdiv = calab_abs_lf(flsbrang - range);
            if (rdiv < frangdiv) {
                printf("calab get y accel range = %lf \n", range);              
                pacc->accelc_status |= 0x2;
                
                pacc->accelc_yradio = flsbrang / range;
            } else {
                printf("calab get y accel range = %.4lf but target = %.4lf\n", range, flsbrang);
            }
        } else if (pdmax->calab_done) {
            printf("calab get max y accel = %f \n", pdmax->calab_avg);
        } else if (pdmin->calab_done) {
            printf("calab get min y accel = %f \n", pdmin->calab_avg);
        }
        
        //printf("\nCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC\n");
        memset(pcdt, 0, sizeof(struct calab_data_s));
    }

    acz = dacc[2];
    ret = 0;
    if (!(pacc->accelc_status & 0x4)) {    
        ret = calab_find_steady(&pacc->accelc_ztmp, acz, lsb);
    }
    if (ret > 0) {
        //printf("\nCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC\n");
        pcdt = &pacc->accelc_ztmp;
        pdmax =& pacc->accelc_zmax;
        pdmin = &pacc->accelc_zmin;

        //printf("accelx div = %f, threshold = %f \n", pcdt->calab_div, flsbthd);

        if (pcdt->calab_div > flsbthd) {
            printf("accelx div = %f > %f \n", pcdt->calab_div, flsbthd);
        }
        else if (!pdmax->calab_done) {
            memcpy(pdmax, pcdt, sizeof(struct calab_data_s));
        } else {
            if (pdmax->calab_avg < pcdt->calab_avg) {
                memcpy(pdmax, pcdt, sizeof(struct calab_data_s));            
            } else {
                if (!pdmin->calab_done) {
                    memcpy(pdmin, pcdt, sizeof(struct calab_data_s));            
                } else {
                    if (pdmin->calab_avg > pcdt->calab_avg) {
                        memcpy(pdmin, pcdt, sizeof(struct calab_data_s));            
                    }
                }
            }
        }

        if ((pdmax->calab_done) && (pdmin->calab_done)) {
            range = pdmax->calab_avg - pdmin->calab_avg;
            rdiv = calab_abs_lf(flsbrang - range);
            if (rdiv < frangdiv) {
                printf("calab get z accel range = %lf \n", range);              
                pacc->accelc_status |= 0x4;

                pacc->accelc_zradio = flsbrang / range;
            } else {
                printf("calab get z accel range = %.4lf but target = %.4lf\n", range, flsbrang);
            }
        } else if (pdmax->calab_done) {
            printf("calab get max z accel = %f \n", pdmax->calab_avg);
        } else if (pdmin->calab_done) {
            printf("calab get min z accel = %f \n", pdmin->calab_avg);
        }
        //printf("\nCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC\n");
        memset(pcdt, 0, sizeof(struct calab_data_s));
    }    

    return 0;
}

static int calab_gyro(struct gyroc_info_s *pgyc, short *dgyo, float lsb)
{
    int ret=0;
    int grx=0, gry=0, grz=0;
    float flsbthd=0;
    struct calab_data_s *pgyt;
    
    flsbthd = lsb / 5.0;

    //printf("calab gyro\n");

/*
    if ((pgyc->gyroc_status & 0x7) == 0x7) {
        return 0x7;
    }
*/

    ret = 0;
    if (!(pgyc->gyroc_status & 0x1)) {
        grx = dgyo[0];
        ret = calab_find_steady(&pgyc->gyroc_tmpx, grx, (int)lsb);
    }

    if (ret > 0) {
        pgyt = &pgyc->gyroc_tmpx;

        if (pgyt->calab_div < flsbthd) {
            printf("gyro x avg: %f, div = %f < %f \n", pgyt->calab_avg, pgyt->calab_div, flsbthd);
            pgyc->gyroc_status |= 0x1;

            memcpy(&pgyc->gyroc_zerox, pgyt, sizeof(struct calab_data_s));
        } else {
            printf("gyro x avg: %f, div = %f > %f \n", pgyt->calab_avg, pgyt->calab_div, flsbthd);
        }
        
        memset(pgyt, 0, sizeof(struct calab_data_s));
    }

    ret = 0;
    if (!(pgyc->gyroc_status & 0x2)) {
        gry = dgyo[1];
        ret = calab_find_steady(&pgyc->gyroc_tmpy, gry, (int)lsb);
    }

    if (ret > 0) {
        pgyt = &pgyc->gyroc_tmpy;

        if (pgyt->calab_div < flsbthd) {
            printf("gyro y avg: %f, div = %f < %f \n", pgyt->calab_avg, pgyt->calab_div, flsbthd);
            pgyc->gyroc_status |= 0x2;

            memcpy(&pgyc->gyroc_zeroy, pgyt, sizeof(struct calab_data_s));
        } else {
            printf("gyro y avg: %f, div = %f > %f \n", pgyt->calab_avg, pgyt->calab_div, flsbthd);
        }

        memset(pgyt, 0, sizeof(struct calab_data_s));
    }

    ret = 0;
    if (!(pgyc->gyroc_status & 0x4)) {
        grz = dgyo[2];
        ret = calab_find_steady(&pgyc->gyroc_tmpz, grz, (int)lsb);
    }

    if (ret > 0) {
        pgyt = &pgyc->gyroc_tmpz;

        if (pgyt->calab_div < flsbthd) {
            printf("gyro z avg: %f, div = %f < %f \n", pgyt->calab_avg, pgyt->calab_div, flsbthd);
            pgyc->gyroc_status |= 0x4;

            memcpy(&pgyc->gyroc_zeroz, pgyt, sizeof(struct calab_data_s));
        } else {
            printf("gyro z avg: %f, div = %f > %f \n", pgyt->calab_avg, pgyt->calab_div, flsbthd);
        }

        memset(pgyt, 0, sizeof(struct calab_data_s));
    }
    
    return pgyc->gyroc_status;
}

static int dmp_gyro_shift(short xr, short yr, short zr, float lsb)
{
    double dxr, dyr, dzr, dlsb;

    if (lsb <= 0) return -1;

    dxr = xr;
    dyr = yr;
    dzr = zr;
    dlsb = lsb;

    dxr = dxr / dlsb;
    dyr = dyr / dlsb;
    dzr = dzr / dlsb;

    printf("gyro:[%.2lf, %.2lf, %.2lf] (lsb:%.1f)\n", dxr, dyr, dzr, lsb);
    
    return 0;
}

static int dmp_accel_shift(short xr, short yr, short zr, unsigned int lsb)
{
    double dxa, dya, dza, dlsb;

    if (lsb <= 0) return -1;

    dxa = xr;
    dya = yr;
    dza = zr;
    dlsb = lsb;

    dxa = dxa/ dlsb;
    dya = dya / dlsb;
    dza = dza / dlsb;

    printf("accel:[%.2lf, %.2lf, %.2lf] (lsb:%d)\n", dxa, dya, dza, lsb);
    
    return 0;
}

#define SEC_LEN 512
static void pabort(const char *s) 
{ 
    perror(s); 
    abort(); 
} 
 
static const char *device = "/dev/spidev32765.0"; 
char data_path[256] = "/mnt/mmc2/usb/greenhill_05.jpg"; 
static uint8_t mode; 
static uint8_t bits = 8; 
static uint32_t speed = 1000000; 
static uint16_t delay; 
static uint16_t command = 0; 
static uint8_t loop = 0; 
static int fd = 0;

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

static int gyro_spi_write(uint8_t addr, unsigned short size, uint8_t *data)
{
    int ret=0;
    uint8_t tx8[2] = {0, 0};
    uint8_t rx8[2] = {0, 0};
    char *rtbuf;
    if (size == 0) return -1;
    if (!data) return -2;

    //printf("\ngyro_spi_write(): size: %d \n", size);

    if (size == 1) {
        tx8[0] = addr;
        tx8[1] = data[0];

        ret = tx_data(fd, rx8, tx8, 1, 2, 1024);
        //printf("[gyroWreg] (0x%.8x) tx: 0x%.2x-0x%.2x rx: 0x%.2x-0x%.2x ret:%d\n", fd, tx8[0], tx8[1], rx8[0], rx8[1], ret);
    } else {
        rtbuf = malloc(size+1);
        if (!rtbuf) return -3;

        memcpy(&rtbuf[1], data, size);
        
        rtbuf[0] = addr;

        //ret = tx_data(fd, rx8, tx8, 1, 1, 1024);
        //printf("[gyroWtreg] tx: 0x%.2x rx: 0x%.2x ret:%d\n", tx8[0], rx8[0], ret);

        ret = tx_data(fd, rtbuf, rtbuf, 1, size+1, size+1);      
        
        //printf("[gyroWdat] tx: \n");
        //shmem_dump(data, size);
        //printf("[gyroWdat] rx: \n");
        //shmem_dump(rtbuf, size);

        free(rtbuf);
    }

    return 0;
}

static int gyro_spi_read(uint8_t addr, unsigned short size, uint8_t *data)
{
    int ret=0;
    uint8_t tx8[2] = {0, 0};
    uint8_t rx8[2] = {0, 0};
    char *rtbuf;
    if (size == 0) return -1;
    if (!data) return -2;

    //printf("\ngyro_spi_read(): size: %d \n", size);
    
    if (size == 1) {
        tx8[0] = addr | 0x80;

        ret = tx_data(fd, rx8, tx8, 1, 2, 1024);
        //printf("[gyroRdreg] (0x%.8x) tx: 0x%.2x-0x%.2x rx: 0x%.2x-0x%.2x ret:%d\n", fd, tx8[0], tx8[1], rx8[0], rx8[1], ret);

        data[0] = rx8[1];
    } else {
        rtbuf = malloc(size+1);
        if (!rtbuf) return -3;

        memset(rtbuf, 0, size+1);
        rtbuf[0] = addr | 0x80;

        //ret = tx_data(fd, rx8, tx8, 1, 1, 1024);
        //printf("[gyroRreg] tx: 0x%.2x rx: 0x%.2x ret:%d\n", tx8[0], rx8[0], ret);

        ret = tx_data(fd, rtbuf, rtbuf, 1, size+1, size+1);      
        
        //printf("[gyroRdat] rx: \n");
        //shmem_dump(rtbuf, size+1);
        memcpy(data, &rtbuf[1], size);
        
        free(rtbuf);
    }

    return 0;
}

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
            fclose(f);
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
    memset(tr, 0, sizeof(struct spi_ioc_transfer));
    
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

  //printf("tx/rx len: %d/%d\n", ret, pksz);

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
            mode |= SPI_AT_CPHA; 
            break; 
        case 'O':   //??体┦ 
            mode |= SPI_AT_CPOL; 
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

static void* aspSalloc(int slen)
{
    int tot=0;
    char *p=0;
    
    tot = *totSalloc;
    tot += slen;
    printf("*******************  salloc size: %d / %d\n", slen, tot);
    *totSalloc = tot;
    
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

    for (idx=0; idx <RING_BUFF_NUM; idx++) {
        memset(pp->pp[idx], 0x00, SPI_TRUNK_SZ);
        msync(pp->pp[idx], SPI_TRUNK_SZ, MS_SYNC);
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

#define DBG_USB_HS 0
#define USB_HS_SAVE_RESULT 0
static int usb_host(struct usbhost_s *puhs, char *strpath)
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
#if 0
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
            
            msync(pMta, USB_META_SIZE, MS_SYNC);
            memcpy(ptm, pMta, sizeof(struct aspMetaData_s));

            printf("[HS] get meta magic number: 0x%.2x, 0x%.2x !!!\n", meta.ASP_MAGIC[0], meta.ASP_MAGIC[1]);

#if 0 /* remove ready */    
            insert_cbw(CBW, CBW_CMD_READY, OP_Hand_Scan, OPSUB_USB_Scan);
            usb_send(CBW, usbid, 31);
#endif

            insert_cbw(CBW, CBW_CMD_SEND_OPCODE, OP_META, OP_META_Sub1);
            usb_send(CBW, usbid, 31);
    
            usb_send(ptm, usbid, USB_META_SIZE);
            shmem_dump(ptm, USB_META_SIZE);
                
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
                //printf("[%s] usb read %d / %d!!", strpath, recvsz, len);
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

            puhs->pushcnt = tcnt;
    
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

#if 1
int main(int argc, char *argv[]) 
{ 
#if SPI_DISABLE
static char spi0[] = ""; 
static char spi1[] = ""; 
#else
static char spi0[] = "/dev/spidev32765.0"; 
static char spi1[] = "/dev/spidev32766.0"; 
#endif
    uint32_t bitset;
    int sel, arg0 = 0, arg1 = 0, arg2 = 0, arg3 = 0, arg4 = 0;
    int ret; 

    arg0 = 0;
	arg1 = 0;
	arg2 = 0;
    /* scanner default setting */
    mode &= ~SPI_AT_MODE_3;
    mode |= SPI_AT_MODE_3;

     printf("mode initial: 0x%x\n", mode & SPI_AT_MODE_3);
         
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
		//goto end;
        }
        else 
            printf("open device[%s]\n", spi0); 

        fd1 = open(spi1, O_RDWR);
        if (fd1 < 0) {
                printf("can't open device[%s]\n", spi1); 
		//goto end;
		fd1 = 0;
        }
        else 
            printf("open device[%s]\n", spi1); 

        fd = fd0;

    /*
     * spi0 mode 
     */ 
    ret = ioctl(fd0, SPI_IOC_WR_MODE, &mode);    //?家Α 
    if (ret == -1) 
        printf("can't set spi mode"); 
 
    ret = ioctl(fd0, SPI_IOC_RD_MODE, &mode);    //?家Α 
    if (ret == -1) 
        printf("can't get spi mode"); 

   /*
     * spi1 mode
     */ 
    ret = ioctl(fd1, SPI_IOC_WR_MODE, &mode);    //?家Α 
    if (ret == -1) 
        printf("can't set spi mode"); 
 
    ret = ioctl(fd1, SPI_IOC_RD_MODE, &mode);    //?家Α 
    if (ret == -1) 
        printf("can't get spi mode"); 

    printf("spi%d mode:0x%x \n", 0, mode);


       int fm[2] = {fd0, fd1};
	char rxans[512];
	char tx[512];
	char rx[512];
	int i;
	for (i = 0; i < 512; i++) {
		rxans[i] = i & 0x95;
		tx[i] = i & 0x95;
	}
	
#if !SPI_DISABLE
    bits = 8;
    ret = ioctl(fm[0], SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) pabort("can't set bits per word");  
    ret = ioctl(fm[0], SPI_IOC_RD_BITS_PER_WORD, &bits); 
    if (ret == -1) pabort("can't get bits per word"); 
#endif

#define DBG_27_DV     0
#define DBG_27_EPOL  0
#define USB_HS_SAVE_RESULT_DV 0
    if (sel == 27){ /* usb printer test usb scam */
        //#define PT_BUF_SIZE (512)
        struct pollfd ptfd[1];
        static char strMetaPath[] = "/mnt/mmc2/usb/meta.bin";
        static char ptdevpath[] = "/dev/g_printer";
        static char pthostpath1[] = "/dev/usb/lp0";
        static char pthostpath2[] = "/dev/usb/lp1";
        char ptfilepath[128];
        char *ptrecv, *ptsend, *palloc=0;
        int ptret=0, recvsz=0, acusz=0, wrtsz=0, maxsz=0, sendsz=0, lastsz=0;
        int cntTx=0, usCost=0, bufsize=0, seqtx=0, retry=0, msCost=0;
        double throughput=0.0;
        struct timespec tstart, tend;

        struct aspMetaData_s *metaRx = 0;
        char *metaPt=0, *addrd=0;
        struct shmem_s *usbTx=0, *usbTxd=0, *usbCur=0;
        int pipeTx[2], pipeRx[2], pipRet=0, lens=0, pipeTxd[2], pipeRxd[2];
        char chq=0, chd=0;
        struct usbhost_s *pushost=0, *pushostd=0, *puscur=0;
        int *piptx=0, *piprx=0;
        int cntLp0=0, cntLp1=0, cntLpx=0;

#if USB_HS_SAVE_RESULT_DV
    FILE *fsave=0;
    char *pImage=0, *ptmp=0;
    static char ptfileSave[] = "/mnt/sdcard/usb/img_rx_%.3d.jpg";
    int saveSize=0;
#endif

        if (arg0 > 0) {
            bufsize = arg0;        
        }
        
        bufsize = PT_BUF_SIZE;
        
        printf(" recv buff size:[%d] \n", bufsize);
        
        totSalloc = (int *)mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
        if (!totSalloc) goto end;
        
        pushost = (struct usbhost_s *)malloc(sizeof(struct usbhost_s));
        usbTx = (struct shmem_s *)aspSalloc(sizeof(struct shmem_s));

        usbTx->pp = memory_init(&usbTx->slotn, DATA_RX_SIZE*bufsize, bufsize); // 150MB 
        if (!usbTx->pp) goto end;
        usbTx->r = (struct ring_p *)aspSalloc(sizeof(struct ring_p));
        usbTx->totsz = DATA_RX_SIZE*bufsize;
        usbTx->chksz = bufsize;
        usbTx->svdist = 8;

        pushostd = (struct usbhost_s *)malloc(sizeof(struct usbhost_s));
        usbTxd = (struct shmem_s *)aspSalloc(sizeof(struct shmem_s));

        usbTxd->pp = memory_init(&usbTxd->slotn, DATA_RX_SIZE*bufsize, bufsize); // 150MB
        if (!usbTxd->pp) goto end;
        usbTxd->r = (struct ring_p *)aspSalloc(sizeof(struct ring_p));
        usbTxd->totsz = DATA_RX_SIZE*bufsize;
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
        
        char csw[13] = {0x55, 0x53, 0x42, 0x43, 0x11, 0x22, 0x33, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00};
        char endTran[64] = {};
        uint8_t cmd=0, opc=0, dat=0;
        uint32_t usbentsRx=0, usbentsTx=0, getents=0;

        int thrid[5];
        
        struct epoll_event eventRx, eventTx, getevents[MAX_EVENTS];
        int usbfd=0, epollfd=0, uret=0, ifx=0, rxfd=0, txfd=0;

        usbfd = open(ptdevpath, O_RDWR);
        if (usbfd <= 0) {
            printf("can't open device[%s]\n", ptdevpath); 
            close(usbfd);
            goto end;
        }
        else {
            printf("open device[%s]\n", ptdevpath); 
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
        if (ret == -1)
        {
            perror ("epoll_ctl");
            printf("spoll set ctl failed errno: %ds\n", errno);
        }
        
        ptrecv = malloc(PT_BUF_SIZE);
        if (ptrecv) {
            printf(" recv buff alloc succeed! size: %d \n", bufsize);
        } else {
            printf(" recv buff alloc failed! size: %d \n", bufsize);
            goto end;
        }
        
        palloc = ptsend;
        
        thrid[0] = fork();
        if (!thrid[0]) {
            printf("fork pid: %d\n", thrid[0]);

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

#if 0 /* buff all and send to pc */
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
                    if ((opc == 0x05) && (!pushostd->pushcnt)) {

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

                        pushost->pushcnt = cntLp0;

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

                        pushostd->pushcnt = cntLp1;
                        
                    }

                    if ((opc == 0x04) && (!pushost->pushcnt)) {
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

                        pushost->pushcnt = cntLp0;
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
                        //puscur = 0;
                        
                        ring_buf_init(usbCur);
                    } else {
                        continue;
                    }

                }
#else /* send to pc in the mean time of recv */
                if ((cmd == 0x12) && ((opc == 0x04) || (opc == 0x05))) {

                        //usbCur = puscur->pushring;
                        //piptx = puscur->pushtx;
                        //piprx = puscur->pushrx; 
#if USB_HS_SAVE_RESULT_DV
            fsave = find_save(ptfilepath, ptfileSave);
            if (!fsave) {
                printf("[DV] find save [%s] failed!!! \n", ptfileSave);
                goto end;    
            }
            saveSize = 120*1024*1024;
            pImage = malloc(saveSize);
            if (pImage) {
                ptmp = pImage;
            } else {
                printf("alloc memory failed!! size: %d \n", saveSize);
            }
#endif

                    while (1) {       
#if DBG_27_DV
                        printf("[DV] addrd: 0x%.8x \n", addrd);
#endif
                        

                        while (addrd == 0) {
#if 1
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
#if 0                   
                            puscur->pushcnt --;

                            if (puscur->pushcnt == 0) {
                                printf("[DV] the last trunk size is %d \n", lens);
                            }
#else
                            if (chq == 'E') {
                                printf("[DV] the last trunk size is %d %d/%d\n", lens, cntTx, puscur->pushcnt);
                            }
#endif
                        }

                        if (cntTx == 0) {
                            clock_gettime(CLOCK_REALTIME, &tstart);
                            printf("[DV] start time %llu ms \n", time_get_ms(&tstart));
                        }    


#if USB_HS_SAVE_RESULT_DV
            //wrtsz = fwrite(addrd, 1, lens, fsave);
            //printf("[DV] usb write file %d / %d !!!\n", wrtsz, lens);
            wrtsz = lens;
            memcpy(ptmp, addrd, lens);
            ptmp += lens;
#endif

#if 0
            sendsz = wrtsz;
#else
                        sendsz = write(usbfd, addrd, lens);
#endif
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
#if 1
                                if (chq == 'E') break;
#else
                                if (puscur->pushcnt == 0) break;
#endif
                            } else {
                                lens -= sendsz;
                                addrd += sendsz;
                                continue;
                            }
                        }
                                
                        cntTx ++;

                    }

                    if ((chq == 'E') && (addrd == 0)) {

                        clock_gettime(CLOCK_REALTIME, &tend);
                        printf("[DV] end time %llu ms \n", time_get_ms(&tend));
                        usCost = test_time_diff(&tstart, &tend, 1000);
                        //msCost = test_time_diff(&tstart, &tend, 1000000);
                        throughput = acusz*8.0 / usCost*1.0;
                        printf("[DV] usb throughput: %d bytes / %d ms = %lf MBits\n", acusz, usCost / 1000, throughput);

#if USB_HS_SAVE_RESULT_DV
            wrtsz = fwrite(pImage, 1, cntTx*PT_BUF_SIZE, fsave);
            sync();
            fclose(fsave);
            free(pImage);

            printf("[DV] save file size: %d, ret: %d \n", cntTx*PT_BUF_SIZE, wrtsz);
#endif

                        cntTx = 0;
                        cmd = 0;
                        ring_buf_init(usbCur);
                    } else {
                        continue;
                    }
                }
#endif
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
                    recvsz = read(usbfd, ptrecv, USB_META_SIZE);
#if DBG_27_DV
                    printf("[DV] usb RX size: %d / %d \n====================\n", recvsz, USB_META_SIZE); 
#endif              
                    if (recvsz < 0) {
                        //usbentsRx = 0;
                        break;
                    }

                    dat = 0;
                    
                    if (recvsz != USB_META_SIZE) {
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
                                    puscur = pushost;                    

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
                                    puscur = pushost;                    

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
                                else if (puscur == pushost) {
                                    puscur = pushostd;
                                    
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
                
                    printf("[DVF] meta usb host _1_ start, PID[%d] \n", thrid[2]);
                    ret = usb_host(pushost, pthostpath1);
                    printf("[DVF] meta usb host _1_ end, PID[%d] ret: %d \n", thrid[2], ret);
                    exit(0);
                    goto  end;
                } else {
                    printf("[PID] create thread 2 id: %d exit\n", thrid[2]);
                    printf("[DVF] meta usb host _1_ PID[%d] \n", thrid[2]);
                }
            
                thrid[4] = fork();
                if (!thrid[4]) {
                
                    printf("[DVF] meta usb host _2_ start, PID[%d] \n", thrid[4]);
                    ret = usb_host(pushostd, pthostpath2);
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

        close(usbfd);
        free(palloc);
        goto end;
    }
    if (sel == 26){ /* usb printer test usb scam */
        //#define PT_BUF_SIZE (512)
        struct pollfd ptfd[1];
        static char ptdevpath[] = "/dev/g_printer";
        static char samplePath[] = "/mnt/mmc2/usb/handmade_o.jpg";
        static char *ptfileSend;
        //static char ptfileSend[] = "/mnt/mmc2/usb/greenhill_05.jpg";
        char ptfilepath[128];
        char *ptrecv, *ptsend, *palloc=0;
        int ptret=0, recvsz=0, acusz=0, wrtsz=0, maxsz=0, sendsz=0, lastsz=0;
        int cntTx=0, usCost=0, bufsize=0, seqtx=0, retry=0;
        double throughput=0.0;
        FILE *fsave=0, *fsend=0;
        struct timespec tstart, tend;
        char csw[13] = {0x55, 0x53, 0x42, 0x43, 0x11, 0x22, 0x33, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00};
        char endTran[64] = {};
        uint8_t cmd=0, opc=0, dat=0;
        uint32_t usbentsRx=0, usbentsTx=0, getents=0;

        int thrid[3];
        
        struct epoll_event eventRx, eventTx, getevents[MAX_EVENTS];
        int usbfd=0, epollfd=0, uret=0, ifx=0, rxfd=0, txfd=0;

        usbfd =  open(ptdevpath, O_RDWR);
        if (usbfd < 0) {
            printf("can't open device[%s]\n", ptdevpath); 
            close(usbfd);
            goto end;
        }
        else {
            printf("open device[%s]\n", ptdevpath); 
        }
        
        usb_nonblock_set(usbfd);
#if 1
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
        if (ret == -1)
        {
            perror ("epoll_ctl");
            printf("spoll set ctl failed errno: %ds\n", errno);
        }
        
/*
        eventTx.data.fd = usbfd;
        eventTx.events = EPOLLOUT | EPOLLET;
        ret = epoll_ctl (epollfd, EPOLL_CTL_ADD, usbfd, &eventTx);
        if (ret == -1)
        {
            perror ("epoll_ctl");
            printf("spoll set ctl failed errno: %ds\n", errno);
        }
*/
#endif

        ptrecv = malloc(PT_BUF_SIZE);
        if (ptrecv) {
            printf(" recv buff alloc succeed! size: %d \n", maxsz);
        } else {
            printf(" recv buff alloc failed! size: %d \n", maxsz);
            goto end;
        }

        if (argc > 2) {
            printf(" input file: %s \n", argv[2]);
            ptfileSend = argv[2];
        } else {
            ptfileSend = samplePath;
        }
        
        printf(" open file [%s] \n", ptfileSend);
        fsend = fopen(ptfileSend, "r");
       
        if (!fsend) {
            printf(" [%s] file open failed \n", ptfileSend);
            goto end;
        }   
        printf(" [%s] file open succeed \n", ptfileSend);

        
        bufsize = PT_BUF_SIZE;
        
        printf(" recv buff size:[%d] \n", bufsize);

        ptret = fseek(fsend, 0, SEEK_END);
        if (ptret) {
            printf(" file seek failed!! ret:%d \n", ptret);
            goto end;
        }

        maxsz = ftell(fsend);
        printf(" file [%s] size: %d \n", ptfileSend, maxsz);
        
        ptsend = malloc(maxsz);
        if (ptsend) {
            printf(" send buff alloc succeed! size: %d \n", maxsz);
        } else {
            printf(" send buff alloc failed! size: %d \n", maxsz);
            goto end;
        }

        ptret = fseek(fsend, 0, SEEK_SET);
        if (ptret) {
            printf(" file seek failed!! ret:%d \n", ptret);
            goto end;
        }
        
        ptret = fread(ptsend, 1, maxsz, fsend);
        printf(" output file read size: %d/%d \n", ptret, maxsz);

        palloc = ptsend;
        
        fclose(fsend);
        
#if 1
#if 0
        cntTx = 0;
        while (1) {
            printf("epoll wait the very first request %d/%d \n", usbentsRx, usbentsTx);
            cntTx++;
/*
            recvsz = read(usbfd, ptrecv, 31);
            printf("usb RX size: %d / %d \n====================\n", recvsz, 31); 
            shmem_dump(ptrecv, recvsz);

            wrtsz = write(usbfd, csw, 13);
            printf("usb TX size: %d \n====================\n", wrtsz); 
            shmem_dump(csw, wrtsz);
*/

            uret = epoll_wait(epollfd, getevents, MAX_EVENTS, 1000);
            if (uret < 0) {
                perror("epoll_wait");
                printf("nonblock failed errno: %d ret: %d\n", errno, uret);
            } else if (uret == 0) {
                printf("nonblock failed errno: %d ret: %d\n", errno, uret);
            } else {
                for(ifx =0; ifx < MAX_EVENTS; ifx++) {
                    if (getevents[ifx].events & EPOLLIN) {
                        //printf("[%d] IN \n", 0);
                        //usbentsRx = 1;
                        break;
                    }
                    if (getevents[ifx].events & EPOLLOUT) {
                        //printf("[%d] OUT \n", 0);
                        /* do nothing */
                    }
                }

                if (getevents[ifx].data.fd == usbfd) {   
                    break;
                }
            }
        }
#endif

        thrid[0] = fork();
        if (!thrid[0]) {

        lastsz = 0;
        cntTx = 0;
        ifx = 0;
        while (1) {
            //printf("Poll cmd: 0x%.2x, opc: 0x%.2x, dat: 0x%.2x, lastsz: %d, cnt: %d \n", cmd, opc, dat, lastsz, cntTx);

            uret = epoll_wait(epollfd, getevents, MAX_EVENTS, 1000);
            if (uret < 0) {
                perror("epoll_wait");
                printf("nonblock failed errno: %d ret: %d\n", errno, uret);
            } else if (uret == 0) {
                //printf("nonblock timeout errno: %d ret: %d\n", errno, uret);
            } else {

                //for(ifx =0; ifx < MAX_EVENTS; ifx++) {
                    rxfd = getevents[0].data.fd;
                    getents = getevents[0].events;

                    //printf("%d. fd: %d, usbfd: %d, evt: 0x%x ret: %d\n", ifx, rxfd, usbfd, getevents[ifx].events, uret);
                    
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
                //}
            }
            
            //printf("epoll %d/%d ret: %d \n", usbentsRx, usbentsTx, uret);

            if (usbentsRx == 1) {
                //rxfd = getevents[ifx].data.fd;
                //printf("Rx cmd: 0x%.2x, opc: 0x%.2x, dat: 0x%.2x, lastsz: %d, cnt: %d \n", cmd, opc, dat, lastsz, cntTx);

                while (1) {
                if ((opc == 0x4c) && (dat == 0x01)) {
                    recvsz = read(usbfd, ptrecv, USB_META_SIZE);
                    printf("usb RX size: %d / %d \n====================\n", recvsz, USB_META_SIZE); 
                    if (recvsz < 0) {
                        //usbentsRx = 0;
                        break;
                    }
                    shmem_dump(ptrecv, recvsz);
                    
                    dat = 0;
                    
                    break;
                } else {
                    recvsz = read(usbfd, ptrecv, 31);
                    printf("usb RX size: %d / %d \n====================\n", recvsz, 31); 
                    if (recvsz < 0) {
                        usbentsRx = 0;
                        break;
                    }
                    shmem_dump(ptrecv, recvsz);
                    
                    if ((ptrecv[0] == 0x55) && (ptrecv[1] == 0x53) && (ptrecv[2] == 0x42)) {
                        cmd = ptrecv[15];
                        opc = ptrecv[16];
                        dat = ptrecv[17];
                        printf("usb get cmd: 0x%.2x opc: 0x%.2x, dat: 0x%.2x \n",cmd, opc, dat);       
            
                        if (cmd == 0x11) {
                            if ((opc == 0x4c) && (dat == 0x01)) {
                                continue;
                            }
                            else if ((opc == 0x04) && (dat == 0x85)) {
                                break;
                            }
                            else if ((opc == 0x05) && (dat == 0x85)) {
                                break;
                            }
                            else {
                                continue;
                            }
                        }
                        else if (cmd == 0x12) {
                            if (opc == 0x04) {
                                break;
                            }
                            else if (opc == 0x05) {
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
            
            if (usbentsTx == 1) {
                //printf("Tx cmd: 0x%.2x, opc: 0x%.2x, dat: 0x%.2x, lastsz: %d, cnt: %d \n", cmd, opc, dat, lastsz, cntTx);
                
                if ((cmd == 0x12) && ((opc == 0x04) || (opc == 0x05))) {
                    printf("start to send image size: %d \n", maxsz);
                    lastsz = maxsz;        
                    ptsend = palloc;
                    cntTx = 0;
                    
                    cmd = 0;
                }
                else if (cmd == 0x11) {
                    seqtx++;
                    csw[12] = seqtx;

                    wrtsz = 0;
                    retry = 0;
                    while (1) {
                        wrtsz = write(usbfd, csw, 13);
                        printf("usb TX size: %d \n====================\n", wrtsz); 
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
            
                while (lastsz > 0) {
                    if (lastsz > bufsize) {
                        wrtsz = bufsize;
                    } else {
                        wrtsz = lastsz;
                        if (wrtsz == 1) {
                            wrtsz = 2;
                        }
                    }
                    
            
                    sendsz = write(usbfd, ptsend, wrtsz);
                    if (sendsz < 0) {
                        //printf("usb send ret: %d !!!\n", sendsz);
                        usbentsTx = 0;
                        break;
                    }  

                    if (cntTx == 0) {
                        clock_gettime(CLOCK_REALTIME, &tstart);
                    }    

#if USB_TX_LOG
                    printf("usb TX size: %d, ret: %d \n", wrtsz, sendsz);
#endif

                    //printf("delay 1ms...");
                    //usleep(1000); 
                    
            
                    ptsend += sendsz;
                    acusz += sendsz;
                    lastsz = lastsz - sendsz;
                    cntTx ++;

                    if (lastsz == 0) {
                        break;
                    }
            
                    //printf("usb send %d/%d to usb total %d last %d\n", sendsz, wrtsz, acusz, lastsz);    
                }
            
                if ((lastsz == 0) && (cntTx >0)) {


/*
                    sendsz = 0;
                    retry = 0;
                    while (1) {
                        sendsz = write(usbfd, ptsend, 13);
                        if (sendsz > 0) {
                            printf("usb send end last size = %d \n====================\n", sendsz);
                            break;
                        }
                        retry++;
                        if (retry > 32768) {
                            break;
                        }
                    }
*/

                    clock_gettime(CLOCK_REALTIME, &tend);
            
                    usCost = test_time_diff(&tstart, &tend, 1000);
                    throughput = maxsz*8.0 / usCost*1.0;
                    printf("usb throughput: %d bytes / %d ms = %lf MBits\n", maxsz, usCost / 1000, throughput);
            
                    cmd = 0;
                    cntTx = 0;                    
                    
                    //usbentsTx = 0;
                }
            
            }
        }

        }
        else {
            cntTx = 0;

            ptfd[0].events = POLLOUT | POLLIN;
            while(1) {
                cntTx++;
                ptret = poll(ptfd, 1, -1);
                printf("poll return %d evt: 0x%.2x - %d\n", ptret, ptfd[0].revents, cntTx);

                sleep(5);
            }
            
            /*

            while(1) {
                cntTx++;
                printf("[2] wait %ds \n", cntTx);
                sleep(1);
            }
            */
        }
#else   
        ptfd[0].fd = usbfd;
        if (ptfd[0].fd < 0) {
            printf("can't open device[%s]\n", ptdevpath); 
            close(ptfd[0].fd);
            goto end;
        }
        else {
            printf("open device[%s]\n", ptdevpath); 
        }
            
        ptfd[0].events = POLLOUT | POLLIN;
        while(1) {
            ptret = poll(ptfd, 1, -1);
            //printf("\n====================\npoll return %d evt: 0x%.2x\n", ptret, ptfd[0].revents);
            
            if (ptret < 0) {
                 printf("poll return %d \n", ptret);
                 goto end;
            }

            if (ptfd[0].revents & POLLOUT) {
                //wrtsz = write(ptfd[0].fd, csw, 1);
                //printf("usb TX NULL \n====================\n"); 
                //shmem_dump(csw, wrtsz);
            }

            if (ptfd[0].revents & POLLIN) {
                printf("usb RX break \n====================\n"); 
                break;
            }
        }
        cntTx = 1;

        while (1) {
            /* send ready */
            //memset(ptrecv, 0, PT_BUF_SIZE);
            ptfd[0].events = POLLIN;

            ptret = poll(ptfd, 1, -1);
            printf("\n====================\npoll return %d evt: 0x%.2x\n", ptret, ptfd[0].revents);
            
            if (ptret < 0) {
                 printf("poll return %d \n", ptret);
                 goto end;
            }

            if (ptfd[0].revents & POLLIN) {
                if ((opc == 0x4c) && (dat == 0x01)) {
                    recvsz = read(ptfd[0].fd, ptrecv, 513);
                    printf("usb RX size: %d \n====================\n", recvsz); 
                    shmem_dump(ptrecv, recvsz);
                    dat = 0;
                } else {
                    recvsz = read(ptfd[0].fd, ptrecv, 31);
                    printf("usb RX size: %d \n====================\n", recvsz); 
                    shmem_dump(ptrecv, recvsz);
/*
                    if ((ptrecv[0] == 0x55) && (ptrecv[1] == 0x53) && (ptrecv[2] == 0x42)) {
                        if ((ptrecv[15] == 0x08) && (ptrecv[16] == 0x0b) && (ptrecv[17] == 0x85)) {
                            //break;
                        }
                    }
*/
                    if ((ptrecv[0] == 0x55) && (ptrecv[1] == 0x53) && (ptrecv[2] == 0x42)) {
                         cmd = ptrecv[15];
                         opc = ptrecv[16];
                         dat = ptrecv[17];
                         printf("usb get opc: 0x%.2x, dat: 0x%.2x \n", opc, dat);       

                         if (cmd == 0x11) {
                             if ((opc == 0x4c) && (dat == 0x01)) {
                                 continue;
                             } else if ((opc == 0x04) && (dat == 0x85)) {
                             } else {
                                 continue;
                             }
                         } else if (cmd == 0x12) {
                             if (opc == 0x04) {
                             } else {
                                 continue;
                             }
                         } else {
                             continue;
                         }
                     }            
                     else if (recvsz == 512) {
                         cmd = 0;
                         opc = 0;
                         dat = 0;
                     }
                }
            }

            ptfd[0].events = POLLOUT;

            ptret = poll(ptfd, 1, -1);
            printf("\n====================\npoll return %d evt: 0x%.2x\n", ptret, ptfd[0].revents);
            
            if (ptret < 0) {
                 printf("poll return %d \n", ptret);
                 goto end;
            }

            if (ptfd[0].revents & POLLOUT) {
                printf("cmd: 0x%.2x, opc: 0x%.2x \n", cmd, opc);
                
                if ((cmd == 0x12) && (opc == 0x04)) {
                    printf("start to send image size: %d \n", maxsz);

                    lastsz = maxsz;        
                    ptsend = palloc;
                    while(1) {
                        if (lastsz > bufsize) {
                            wrtsz = bufsize;
                        } else {
                            wrtsz = lastsz;
                            if (wrtsz == 1) {
                                wrtsz = 2;
                            }
                        }

                        sendsz = write(ptfd[0].fd, ptsend, wrtsz);
                        printf("usb TX size: %d, ret: %d \n", wrtsz, sendsz);
                        
                        if (sendsz < 0) {
                            printf("usb send ret: %d, error!!!", sendsz);
                            break;
                        }

                        if (cntTx == 0) {
                            clock_gettime(CLOCK_REALTIME, &tstart);
                        }

                        ptsend += sendsz;
                        acusz += sendsz;
                        lastsz = lastsz - sendsz;
                        cntTx ++;

                        //printf("usb send %d/%d to usb total %d last %d\n", sendsz, wrtsz, acusz, lastsz);

                        if (lastsz == 0) {
                            sendsz = write(ptfd[0].fd, ptsend, 64);
                            printf("usb send end last size = %d \n====================\n", sendsz);
                            clock_gettime(CLOCK_REALTIME, &tend);
                            break;
                        }
                    }


                    usCost = test_time_diff(&tstart, &tend, 1000);
                    throughput = maxsz*8.0 / usCost*1.0;
                    printf("usb throughput: %d bytes / %d us = %lf MBits\n", maxsz, usCost, throughput);
                
                }
                else {
                    cntTx++;
                    csw[12] = cntTx;
                    wrtsz = write(ptfd[0].fd, csw, 13);
                    printf("usb TX size: %d \n====================\n", wrtsz); 
                    shmem_dump(csw, wrtsz);
                }
            }
            
            //STAGE_DELAY;
        }
#endif
#if 0
        /* reply META CSW */
        //memset(ptrecv, 0, PT_BUF_SIZE);
        ptfd[0].events = POLLOUT;

        ptret = poll(ptfd, 1, -1);
        printf("poll return %d \n", ptret);
            
        if (ptret < 0) {
             printf("poll return %d \n", ptret);
             goto end;
        }

        csw[12] = 1;
        
        if (ptret && (ptfd[0].revents & POLLOUT)) {
            wrtsz = write(ptfd[0].fd, csw, 13);
            printf("usb write size: %d - 3\n", wrtsz); 
            shmem_dump(csw, wrtsz);
        }

        /* reply CSW */
        //memset(ptrecv, 0, PT_BUF_SIZE);
        ptfd[0].events = POLLOUT;

        ptret = poll(ptfd, 1, -1);
        printf("poll return %d \n", ptret);
            
        if (ptret < 0) {
             printf("poll return %d \n", ptret);
             goto end;
        }
        
        csw[12] = 1;

        if (ptret && (ptfd[0].revents & POLLOUT)) {
            wrtsz = write(ptfd[0].fd, csw, 13);
            printf("usb write size: %d - 3\n", wrtsz); 
            shmem_dump(csw, wrtsz);
        }
#endif

        close(usbfd);
        free(palloc);
        goto end;
    }
    if (sel == 25){ /* usb printer write access */
        struct pollfd ptfd[1];
        static char ptdevpath[] = "/dev/g_printer";
        static char ptfileSave[] = "/mnt/mmc2/usb/image%.3d.jpg";
        char ptfilepath[128];
        char *ptrecv, *ptbuf=0;
        int ptret=0, recvsz=0, acusz=0, wrtsz=0;
        int cntRecv=0, usCost=0, bufsize=0;
        int bufmax=0, bufidx=0, printlog=0;
        double throughput=0.0;
        struct timespec tstart, tend;
        char firstTrunk[32] = {0x55, 0x53, 0x42, 0x43, 0x74, 0x34, 0x33, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        
        ptfd[0].fd = open(ptdevpath, O_WRONLY);
        if (ptfd[0].fd < 0) {
            printf("can't open device[%s]\n", ptdevpath); 
            close(ptfd[0].fd);
            goto end;
        }
        else {
            printf("open device[%s]\n", ptdevpath); 
        }

        if (arg0 > 0) {
            bufsize = arg0;
        } else {
            bufsize = PT_BUF_SIZE;
        }

        if (bufsize > PT_BUF_SIZE) {
            bufsize = PT_BUF_SIZE;
        }

        printf("usb write size[%d]\n", bufsize); 
        
        ptbuf = malloc(PT_BUF_SIZE);
        ptrecv = ptbuf;
        
        wrtsz = 32;
        acusz = PT_BUF_SIZE;
        while (acusz > 0) {
            if (acusz > wrtsz) {
                recvsz = wrtsz; 
            } else {
                recvsz = acusz; 
            }
            memcpy(ptrecv, firstTrunk, recvsz);

            acusz -= recvsz;
            ptrecv += recvsz;
        }

        //shmem_dump(ptbuf, PT_BUF_SIZE);

        if (arg1 > 0) {
            printlog = arg1;
        }

        ptfd[0].events = POLLOUT;

        ptrecv = ptbuf;
        while(1) {
            ptret = poll(ptfd, 1, -1);
            printf("usb poll ret: %d \n", ptret);
            if (ptret < 0) {
                printf("usb poll failed ret: %d\n", ptret);
                break;
            }

            if (ptret && (ptfd[0].revents & POLLOUT)) {

                recvsz = write(ptfd[0].fd, ptrecv, bufsize);
                printf("usb write ret: %d \n", recvsz);

                break;
            }                
            
        }

        close(ptfd[0].fd);
        free(ptbuf);

        goto end;
    }
    
    if (sel == 24){ /* usb printer read access */
        //#define PT_BUF_SIZE (512)
        struct pollfd ptfd[1];
        static char ptdevpath[] = "/dev/g_printer";
        static char ptfileSend[] = "/mnt/mmc2/usb/send001.jpg";
        char ptfilepath[128];
        char *ptrecv, *ptsend, *palloc=0;
        int ptret=0, recvsz=0, acusz=0, wrtsz=0, maxsz=0, sendsz=0, lastsz=0;
        int cntTx=0, usCost=0, bufsize=0;
        double throughput=0.0;
        FILE *fsave=0, *fsend=0;
        struct timespec tstart, tend;
        
        ptfd[0].fd = open(ptdevpath, O_RDWR);
        if (ptfd[0].fd < 0) {
            printf("can't open device[%s]\n", ptdevpath); 
            close(ptfd[0].fd);
            goto end;
        }
        else {
            printf("open device[%s]\n", ptdevpath); 
        }

        ptfd[0].events = POLLIN;

        if (arg0 > 0) {
            wrtsz = arg0;        
        } else {
            wrtsz = PT_BUF_SIZE;
        }
        
        printf(" recv size:[%d] \n", wrtsz);
        palloc = malloc(PT_BUF_SIZE);
        ptrecv = palloc;

        while(1) {

            ptret = poll(ptfd, 1, -1);

            if (ptret < 0) {
                printf("poll return %d \n", ptret);
                break;
            }

            if (ptret && (ptfd[0].revents & POLLIN)) {
                recvsz = read(ptfd[0].fd, ptrecv, wrtsz);
                
                printf("usb recv ret: %d ", recvsz);
            
                break;
            }
            
        }

        shmem_dump(ptrecv, recvsz);
        
        close(ptfd[0].fd);
        free(palloc);

        goto end;
    }

#define GSENSOR_TIME_LOG (0)

    if (sel == 23){ /* usb printer write access */
        //#define PT_BUF_SIZE (512)
        struct pollfd ptfd[1];
        static char ptdevpath[] = "/dev/g_printer";
        static char ptfileSend[] = "/mnt/mmc2/usb/send001.jpg";
        char ptfilepath[128];
        char *ptrecv, *ptsend, *palloc=0;
        int ptret=0, recvsz=0, acusz=0, wrtsz=0, maxsz=0, sendsz=0, lastsz=0;
        int cntTx=0, usCost=0, bufsize=0;
        double throughput=0.0;
        FILE *fsave=0, *fsend=0;
        struct timespec tstart, tend;
        
        ptfd[0].fd = open(ptdevpath, O_RDWR);
        if (ptfd[0].fd < 0) {
            printf("can't open device[%s]\n", ptdevpath); 
            close(ptfd[0].fd);
            goto end;
        }
        else {
            printf("open device[%s]\n", ptdevpath); 
        }

        ptfd[0].events = POLLOUT;

        printf(" open file [%s] \n", ptfileSend);
        fsend = fopen(ptfileSend, "r");
       
        if (!fsend) {
            printf(" [%s] file open failed \n", ptfileSend);
            goto end;
        }   
        printf(" [%s] file open succeed \n", ptfileSend);

        if (arg0 > 0) {
            bufsize = arg0;        
        } else {
            bufsize = PT_BUF_SIZE;
        }
        printf(" recv buff size:[%d] \n", bufsize);

        ptret = fseek(fsend, 0, SEEK_END);
        if (ptret) {
            printf(" file seek failed!! ret:%d \n", ptret);
            goto end;
        }

        maxsz = ftell(fsend);
        printf(" file [%s] size: %d \n", ptfileSend, maxsz);
        
        ptsend = malloc(maxsz);
        if (ptsend) {
            printf(" send buff alloc succeed! size: %d \n", maxsz);
        } else {
            printf(" send buff alloc failed! size: %d \n", maxsz);
            goto end;
        }

        ptret = fseek(fsend, 0, SEEK_SET);
        if (ptret) {
            printf(" file seek failed!! ret:%d \n", ptret);
            goto end;
        }
        
        ptret = fread(ptsend, 1, maxsz, fsend);
        printf(" output file read size: %d/%d \n", ptret, maxsz);

        fclose(fsend);

        lastsz = maxsz;        
        palloc = ptsend;
        while(1) {

            ptret = poll(ptfd, 1, -1);

            if (ptret < 0) {
                printf("poll return %d \n", ptret);
                break;
            }

            if (ptret && (ptfd[0].revents & POLLOUT)) {
                if (lastsz > bufsize) {
                    wrtsz = bufsize;
                } else {
                    wrtsz = lastsz;
                    if (wrtsz == 1) {
                        wrtsz = 2;
                    }
                }
                
                sendsz = write(ptfd[0].fd, ptsend, wrtsz);
                
                if (sendsz < 0) {
                    printf("usb send ret: %d, error!!!", sendsz);
                    break;
                }

                if (cntTx == 0) {
                    clock_gettime(CLOCK_REALTIME, &tstart);
                }
                
                ptsend += sendsz;
                acusz += sendsz;
                lastsz = lastsz - sendsz;
                cntTx ++;
                
                //printf("usb send %d/%d to usb total %d last %d\n", sendsz, wrtsz, acusz, lastsz);

                if (lastsz == 0) {
                    sendsz = write(ptfd[0].fd, ptsend, 1);
                    printf("usb send end last size = %d \n", sendsz);
                    clock_gettime(CLOCK_REALTIME, &tend);
                    break;
                }
            }
            
        }

        usCost = test_time_diff(&tstart, &tend, 1000);

        throughput = maxsz*8.0 / usCost*1.0;
        
        printf("usb throughput: %d bytes / %d us = %lf MBits\n", maxsz, usCost, throughput);
        
        close(ptfd[0].fd);
        free(palloc);
        goto end;
    }

    if (sel == 22){ /* usb printer read access */
        struct pollfd ptfd[1];
        static char ptdevpath[] = "/dev/usb/lp0";
        static char ptfileSave[] = "/mnt/mmc2/usb/image%.3d.jpg";
        char ptfilepath[128];
        char *ptrecv, *ptbuf=0;
        int ptret=0, recvsz=0, acusz=0, wrtsz=0;
        int cntRecv=0, usCost=0, bufsize=0;
        int bufmax=0, bufidx=0, printlog=0;
        double throughput=0.0;
        struct timespec tstart, tend;
        FILE *fsave=0;
        
        ptfd[0].fd = open(ptdevpath, O_RDWR);
        if (ptfd[0].fd < 0) {
            printf("can't open device[%s]\n", ptdevpath); 
            close(ptfd[0].fd);
            goto end;
        }
        else {
            printf("open device[%s]\n", ptdevpath); 
        }

        if (arg0 > 0) {
            bufsize = arg0;
        } else {
            bufsize = PT_BUF_SIZE;
        }

        if (arg1 > 0) {
            printlog = arg1;
        }
        
        printf("usb recv buff size:[%d]\n", bufsize); 
        
        ptfd[0].events = POLLIN;
        fsave = find_save(ptfilepath, ptfileSave);
        if (!fsave) {
            goto end;    
        }

        printf("usb find save [%s] \n", ptfilepath);

        bufmax = 128*1024*1024;
        ptbuf = malloc(bufmax);  //1024*32768
        printf("usb allocate memory size:[%d] \n", bufmax);

        ptrecv = ptbuf;
        while(1) {
            //ptret = read();
            //ptret = poll(ptfd, 1, -1);
            //printf("usb poll ret: %d \n", ptret);
            //if (ptret < 0) {
                //printf("usb poll failed ret: %d\n", ptret);
                //break;
            //}

            //if (ptret && (ptfd[0].revents & POLLIN)) {
                recvsz = read(ptfd[0].fd, ptrecv, bufsize);

                //printf("usb recv ret: %d \n", recvsz);
                if (recvsz <= 1) {
                    switch (recvsz) {
                        case 1:
                            printf("usb recv ret: %d, break!!!", recvsz);
                            break;
                        case 0:
                            continue;
                            break;
                        default:
                            printf("usb recv ret: %d, error!!!", recvsz);
                            break;
                    }
                    break;
                }
                
                /*
                if (!recvsz) {
                    continue;
                }
                */
                
                /*
                if (cntRecv == 0) {
                    clock_gettime(CLOCK_REALTIME, &tstart);
                }
                */

                //wrtsz = fwrite(ptrecv, 1, recvsz, fsave);
                wrtsz = recvsz;

                //sync();
                
                acusz += wrtsz;
                //ptrecv += wrtsz;

                
                
                if (printlog) {
                    //cntRecv ++;
                    printf("usb r %d w %d tot %d \n", recvsz, wrtsz, acusz);
                }
                
/*
                if (recvsz < bufsize) {
                    printf("usb recv end last recv = %d \n", recvsz);
                    clock_gettime(CLOCK_REALTIME, &tend);
                    break;
                }
*/
            //}

            
        }

        //usCost = test_time_diff(&tstart, &tend, 1000);

        //throughput = acusz*8.0 / usCost*1.0;
        
        //printf("usb throughput: %d bytes / %d us = %lf MBits\n", acusz, usCost, throughput);

        printf("write file size [%d] \n", acusz);

        /*
        ptrecv = ptbuf;
        while (acusz) {
            if (acusz > 32768) {
                wrtsz = 32768;
            } else {
                wrtsz = acusz;
            }
            
            recvsz = fwrite(ptrecv, 1, wrtsz, fsave);
            if (recvsz <= 0) {
                break;
            }
            
            acusz -= recvsz;
            ptrecv += recvsz;

            //printf("write file [%d] \n", acusz);
        }
        */
        sync();

        close(ptfd[0].fd);
        fclose(fsave);
        free(ptbuf);
        goto end;
    }
    
    if (sel == 21){ /* usb printer read access throughput */
        struct pollfd ptfd[1];
        static char ptdevpath[] = "/dev/usb/lp0";
        static char ptfileSave[] = "/mnt/mmc2/usb/image%.3d.jpg";
        char ptfilepath[128];
        char *ptrecv, *ptbuf=0;
        int ptret=0, recvsz=0, acusz=0, wrtsz=0;
        int cntRecv=0, usCost=0, bufsize=0;
        int bufmax=0, bufidx=0;
        double throughput=0.0;
        struct timespec tstart, tend;
        FILE *fsave=0;
        
        ptfd[0].fd = open(ptdevpath, O_RDWR);
        if (ptfd[0].fd < 0) {
            printf("can't open device[%s]\n", ptdevpath); 
            close(ptfd[0].fd);
            goto end;
        }
        else {
            printf("open device[%s]\n", ptdevpath); 
        }

        if (arg0 > 0) {
            bufsize = arg0;
        } else {
            bufsize = PT_BUF_SIZE;
        }
        printf("usb recv buff size:[%d]\n", bufsize); 
        
        ptfd[0].events = POLLIN;
        fsave = find_save(ptfilepath, ptfileSave);
        if (!fsave) {
            goto end;    
        }

        printf("usb find save [%s] \n", ptfilepath);

        bufmax = 128*1024*1024;
        ptbuf = malloc(bufmax);  //1024*32768
        printf("usb allocate memory size:[%d] \n", bufmax);

        ptrecv = ptbuf;
        while(1) {
            //ptret = read();
            ptret = poll(ptfd, 1, -1);
            //printf("usb poll ret: %d \n", ptret);
            if (ptret < 0) {
                printf("usb poll failed ret: %d\n", ptret);
                break;
            }

            if (ptret && (ptfd[0].revents & POLLIN)) {
                recvsz = read(ptfd[0].fd, ptrecv, bufsize);

                //printf("usb recv ret: %d \n", recvsz);
                if (recvsz < 0) {
                    printf("usb recv ret: %d, error!!!", recvsz);
                    break;
                }

                if (!recvsz) {
                    continue;
                }

                if (cntRecv == 0) {
                    clock_gettime(CLOCK_REALTIME, &tstart);
                }

                //wrtsz = fwrite(ptrecv, 1, recvsz, fsave);
                wrtsz = recvsz;

                acusz += wrtsz;
                ptrecv += wrtsz;
                cntRecv ++;
                //printf("usb recv %d write %d to file total %d \n", recvsz, wrtsz, acusz);

                if (recvsz < bufsize) {
                    printf("usb recv end last recv = %d \n", recvsz);
                    clock_gettime(CLOCK_REALTIME, &tend);
                    break;
                }
            }
            
        }

        usCost = test_time_diff(&tstart, &tend, 1000);

        throughput = acusz*8.0 / usCost*1.0;
        
        printf("usb throughput: %d bytes / %d us = %lf MBits\n", acusz, usCost, throughput);

        printf("write file size [%d] \n", acusz);
        ptrecv = ptbuf;
        while (acusz) {
            if (acusz > 32768) {
                wrtsz = 32768;
            } else {
                wrtsz = acusz;
            }
            
            recvsz = fwrite(ptrecv, 1, wrtsz, fsave);
            if (recvsz <= 0) {
                break;
            }
            
            acusz -= recvsz;
            ptrecv += recvsz;

            //printf("write file [%d] \n", acusz);
        }

        sync();

        close(ptfd[0].fd);
        fclose(fsave);
        free(ptbuf);
        goto end;
    }
    
    if (sel == 20){ /* command mode test ex[20 ]*/ /* gyro init */
        /* Starting sampling rate. */
        #define DEFAULT_DMP_HZ  (200)
        #define DEFAULT_MPU_HZ  (1000)
        hal = (struct hal_s *)mmap(NULL, sizeof(struct hal_s), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
        if (!hal) {
            printf("malloc for hal failed \n");
            goto end;  
        }
        
        st = (struct gyro_state_s *)mmap(NULL, sizeof(struct gyro_state_s), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
        if (!st) {
            printf("malloc for hal failed \n");
            goto end;  
        }

        memset(st, 0, sizeof(struct gyro_state_s));
        st->reg = &reg;
        st->hw = &hw;
        st->test = &test;
        
        int ret=0;
        unsigned char accel_fsr;
        unsigned short gyro_rate, gyro_fsr;
        unsigned short orient;
        struct timespec gyrotime, gyrotdif[2];
        int newgyro = 0;

        clock_gettime(CLOCK_REALTIME, &gyrotime);
        printf("gyrotime: sec:%llu, nsec:%llu \n", gyrotime.tv_sec, gyrotime.tv_nsec);

        ret = gyro_init();
        printf("gyro_init() ret = %d \n", ret);
        
        /* Wake up all sensors. */
        ret = mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL);
        if (ret) printf("mpu_set_sensors() ret = %d \n", ret);
        /* Push both gyro and accel data into the FIFO. */
        ret = mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL);
        if (ret) printf("mpu_configure_fifo() ret = %d \n", ret);
        ret = mpu_set_sample_rate(DEFAULT_MPU_HZ);
        if (ret) printf("mpu_set_sample_rate() ret = %d \n", ret);
        /* Read back configuration in case it was set improperly. */
        ret = mpu_get_sample_rate(&gyro_rate);
        ret = mpu_get_gyro_fsr(&gyro_fsr);
        ret = mpu_get_accel_fsr(&accel_fsr);

        printf("gyro_rate = %d \n", gyro_rate);
        printf("gyro_fsr = %d \n", gyro_fsr);
        printf("accel_fsr = %d \n", accel_fsr);

        /* Initialize HAL state variables. */
        memset(hal, 0, sizeof(hal));
        hal->sensors = ACCEL_ON | GYRO_ON;
        hal->report = PRINT_ACCEL | PRINT_GYRO;

        //run_self_test();

#if 0
        /* To initialize the DMP:
         * 1. Call dmp_load_motion_driver_firmware(). This pushes the DMP image in
         *    inv_mpu_dmp_motion_driver.h into the MPU memory.
         * 2. Push the gyro and accel orientation matrix to the DMP.
         * 3. Register gesture callbacks. Don't worry, these callbacks won't be
         *    executed unless the corresponding feature is enabled.
         * 4. Call dmp_enable_feature(mask) to enable different features.
         * 5. Call dmp_set_fifo_rate(freq) to select a DMP output rate.
         * 6. Call any feature-specific control functions.
         *
         * To enable the DMP, just call mpu_set_dmp_state(1). This function can
         * be called repeatedly to enable and disable the DMP at runtime.
         *
         * The following is a short summary of the features supported in the DMP
         * image provided in inv_mpu_dmp_motion_driver.c:
         * DMP_FEATURE_LP_QUAT: Generate a gyro-only quaternion on the DMP at
         * 200Hz. Integrating the gyro data at higher rates reduces numerical
         * errors (compared to integration on the MCU at a lower sampling rate).
         * DMP_FEATURE_6X_LP_QUAT: Generate a gyro/accel quaternion on the DMP at
         * 200Hz. Cannot be used in combination with DMP_FEATURE_LP_QUAT.
         * DMP_FEATURE_TAP: Detect taps along the X, Y, and Z axes.
         * DMP_FEATURE_ANDROID_ORIENT: Google's screen rotation algorithm. Triggers
         * an event at the four orientations where the screen should rotate.
         * DMP_FEATURE_GYRO_CAL: Calibrates the gyro data after eight seconds of
         * no motion.
         * DMP_FEATURE_SEND_RAW_ACCEL: Add raw accelerometer data to the FIFO.
         * DMP_FEATURE_SEND_RAW_GYRO: Add raw gyro data to the FIFO.
         * DMP_FEATURE_SEND_CAL_GYRO: Add calibrated gyro data to the FIFO. Cannot
         * be used in combination with DMP_FEATURE_SEND_RAW_GYRO.
         */
        ret = dmp_load_motion_driver_firmware();
        if (ret) {
            printf("dmp_load_motion_driver_firmware() ret = %d \n", ret);
            goto end;  
        }

        orient = inv_orientation_matrix_to_scalar(gyro_orientation);
        ret = dmp_set_orientation(orient);
        if (ret) {
            printf("dmp_set_orientation() ret = %d \n", ret);
            goto end;  
        }
        
        dmp_register_tap_cb(tap_cb);
        dmp_register_android_orient_cb(android_orient_cb);

        /*
         * Known Bug -
         * DMP when enabled will sample sensor data at 200Hz and output to FIFO at the rate
         * specified in the dmp_set_fifo_rate API. The DMP will then sent an interrupt once
         * a sample has been put into the FIFO. Therefore if the dmp_set_fifo_rate is at 25Hz
         * there will be a 25Hz interrupt from the MPU device.
         *
         * There is a known issue in which if you do not enable DMP_FEATURE_TAP
         * then the interrupts will be at 200Hz even if fifo rate
         * is set at a different rate. To avoid this issue include the DMP_FEATURE_TAP
         */
        hal->dmp_features = DMP_FEATURE_6X_LP_QUAT | DMP_FEATURE_TAP |
            DMP_FEATURE_ANDROID_ORIENT | DMP_FEATURE_SEND_RAW_ACCEL | DMP_FEATURE_SEND_CAL_GYRO |
            DMP_FEATURE_GYRO_CAL;
        
        ret = dmp_enable_feature(hal->dmp_features);
        if (ret) {
            printf("dmp_enable_feature() ret = %d \n", ret);
            goto end;  
        }

        dmp_set_fifo_rate(DEFAULT_DMP_HZ);
        mpu_set_dmp_state(1);

        
        hal->dmp_on = 1;
        //hal->motion_int_mode = 1;
#endif

        char ch;
        
        unsigned int acclsb=0;
        float gyrolsb=0;

        struct accelc_info_s *pacclc=0;
        struct gyroc_info_s *pgyroc=0;

        float flsbthd;
        double flsbrang, frangdiv;

        pacclc = mmap(NULL, sizeof(struct accelc_info_s), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
        //pacclc = malloc(sizeof(struct accelc_info_s));
        if (!pacclc) {
            printf("malloc for accelc failed!!! \n");
        }
        memset(pacclc, 0, sizeof(struct accelc_info_s));
        
        pgyroc = mmap(NULL, sizeof(struct gyroc_info_s), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
        //pgyroc = malloc(sizeof(struct gyroc_info_s));
        if (!pgyroc) {
            printf("malloc for pgyroc failed!!! \n");
        }
        memset(pgyroc, 0, sizeof(struct gyroc_info_s));
        
        ret = mpu_get_gyro_sens(&gyrolsb);
        if (ret) {
            gyrolsb = -1;
        }
        ret = mpu_get_accel_sens(&acclsb);
        if (ret) {
            acclsb = -1;
        }

        flsbthd = acclsb;
        flsbthd = flsbthd / 100.0;
        flsbrang = acclsb * 2;
        frangdiv = flsbrang / 50.0;

        pacclc->accelc_frangdiv = frangdiv;
        pacclc->accelc_flsbthd = flsbthd;
        pacclc->accelc_grange = flsbrang;
        
#define ACCEL_CALAB_DONE (1)
#if ACCEL_CALAB_DONE
    #define ACCEL_MID_X      (-6019)
    #define ACCEL_MID_Y      (-11488)
    #define ACCEL_MID_Z      (-3195)
    #define ACCEL_RADIO_X  (1.001162)
    #define ACCEL_RADIO_Y  (0.999877)
    #define ACCEL_RADIO_Z  (0.994124)
#endif

#if ACCEL_CALAB_DONE                            
        pacclc->accelc_mid[0] = ACCEL_MID_X;
        pacclc->accelc_mid[1] = ACCEL_MID_Y;
        pacclc->accelc_mid[2] = ACCEL_MID_Z;
        pacclc->accelc_xradio = ACCEL_RADIO_X;
        pacclc->accelc_yradio = ACCEL_RADIO_Y;
        pacclc->accelc_zradio = ACCEL_RADIO_Z;
#endif

#define GSENSOR_CALAB  (1)

#if 1 //GSENSOR_CALAB /* calabrate */
        int bufsize=0;
        short *gybuf, *acbuf;
        short *gyro, *accel;
        unsigned char sensors=0;
        short midx=0, midy=0, midz=0;
        short mgyx=0, mgyy=0, mgyz=0;
        double dx, dy, dz, dq, dg, dlsb;
        double radx=0, rady=0, radz=0;
        double gdx=0, gdy=0, gdz=0;
        double dist=0, gv=980.665, cuv=0, dt=0.001, dgs=0;

        uint32_t procede=0, steady_count=0;

        int d_ms=0, more=0;

        dgs = gyrolsb;
        bufsize = 1024;
        gybuf = malloc(bufsize);
        if (!gybuf) {
            printf("allocate memory for gyro failed!!! \n");
        }
        
        acbuf = malloc(bufsize);
        if (!acbuf) {
            printf("allocate memory for accel failed!!! \n");
        }

        procede = hal->report;

#if ACCEL_CALAB_DONE
        midx = pacclc->accelc_mid[0];
        midy = pacclc->accelc_mid[1];
        midz = pacclc->accelc_mid[2] ;
        radx = pacclc->accelc_xradio;
        rady = pacclc->accelc_yradio;
        radz = pacclc->accelc_zradio;
#endif

        while (1) {
            /* This function gets new data from the FIFO. The FIFO can contain
             * gyro, accel, both, or neither. The sensors parameter tells the
             * caller which data fields were actually populated with new data.
             * For example, if sensors == INV_XYZ_GYRO, then the FIFO isn't
             * being filled with accel data. The more parameter is non-zero if
             * there are leftover packets in the FIFO.
             */

#if GSENSOR_TIME_LOG
            clock_gettime(CLOCK_REALTIME, &gyrotdif[0]);
#endif

            usleep(16000);
            
#if GSENSOR_TIME_LOG                
            clock_gettime(CLOCK_REALTIME, &gyrotime);
            d_ms = test_time_diff(&gyrotdif[0], &gyrotime, 1000000);
            printf("    time sleep: %d ms\n", d_ms);
#endif                

#if GSENSOR_TIME_LOG
            clock_gettime(CLOCK_REALTIME, &gyrotime);
            //clock_gettime(CLOCK_REALTIME, &gyrotdif[0]);
            d_ms = test_time_diff(&gyrotdif[1], &gyrotime, 1000);
            printf("    time delay: %d us\n", d_ms);
#endif

            more = 0;
            sensors = 0;
            //memset(gybuf, 0, bufsize);
            //memset(acbuf, 0, bufsize);
            ret = mpu_read_fifo(gybuf, acbuf, &gyrotdif[1], &sensors, &more);
            if (ret) {
                printf("warning!!! mpu_read_fifo ret %d \n", ret);
            }

            //printf("    more: %d\n", more);
#if 1
            accel = acbuf;
            gyro = gybuf;
            while (more) {
                if ((sensors & INV_XYZ_ACCEL) && (procede& PRINT_ACCEL)) {
                    //printf("accel:[%.5hd, %.5hd, %.5hd] ms:%llu %d MPU\n", accel[0], accel[1], accel[2], time_get_ms(&gyrotime), acclsb);
                    //send_packet(PACKET_TYPE_ACCEL, accel);
                    if (midx == 0) {
                        ret = calab_accel(pacclc, accel, acclsb);
                        if (ret == 0x7) {
                            midx = (pacclc->accelc_xmax.calab_avg + pacclc->accelc_xmin.calab_avg) /2;
                            pacclc->accelc_mid[0] = midx;

                            midy = (pacclc->accelc_ymax.calab_avg + pacclc->accelc_ymin.calab_avg) /2;
                            pacclc->accelc_mid[1] = midy;

                            midz = (pacclc->accelc_zmax.calab_avg + pacclc->accelc_zmin.calab_avg) /2;
                            pacclc->accelc_mid[2] = midz;

                            radx = pacclc->accelc_xradio;
                            rady = pacclc->accelc_yradio;
                            radz = pacclc->accelc_zradio;

                            printf("Ymin: %f, Ymax: %f \n", pacclc->accelc_ymin.calab_avg, pacclc->accelc_ymax.calab_avg);
                            printf("Xmin: %f, Xmax: %f \n", pacclc->accelc_xmin.calab_avg, pacclc->accelc_xmax.calab_avg);
                            printf("Zmin: %f, Zmax: %f \n", pacclc->accelc_zmin.calab_avg, pacclc->accelc_zmax.calab_avg);
                            printf("accel calab succeed: xmid = %hd, ymid = %hd, zmid = %hd (%lf, %lf, %lf)\n", midx, midy, midz, pacclc->accelc_xradio, pacclc->accelc_yradio, pacclc->accelc_zradio);
                        }
                    } else {
                        dx = accel[0]-midx;
                        dy = accel[1]-midy;
                        dz = accel[2]-midz;
                
                        dx = dx * radz;
                        dy = dy * rady;
                        dz = dz * radz;
                
                        dq = dx*dx+dy*dy+dz*dz;
                
                        dlsb = acclsb;
                        
                        dg = sqrt(dq);
                        //printf("accel calab g(%.4lf) x = %.5lf, y = %.5lf, z = %.5lf g = %.1lf\n", dg/dlsb, pacclc->accelc_xradio, pacclc->accelc_yradio, pacclc->accelc_zradio, dg);
                        //printf("g: %.4lf \n", dg);
                
                        dq = dg/dlsb;
                        dg = 1;
                        
                        dq = calab_abs_lf(dg - dq);

                        
                        if ((dq < 0.02) && (cuv == 0)) {
                            if (dq < 0.01) {
                                gdx = dx;
                                gdy = dy;
                                gdz = dz;

                                steady_count++;

                                if (steady_count > 20) {
                                    cuv = 0;
                                    //steady_count = 0;
                                    //printf("[steady] \n");  
                                }

                                //printf("[steady] g = (%.2lf, %.2lf, %.2lf) \n", gdx, gdy, gdz);  
                            } else {

                                if (dist > 0) {
                                    printf("[down] dist = %.4lf \n", dist);  
                                }

                                if (cuv == 0) {
                                    dist = 0;
                                }
                            }

                            if (steady_count > 20) {
                                ret = calab_gyro(pgyroc, gyro, gyrolsb);
                                if (ret == 0x7) {
                                    pgyroc->gyroc_mid[0] = (short)pgyroc->gyroc_zerox.calab_avg;
                                    pgyroc->gyroc_mid[1] = (short)pgyroc->gyroc_zeroy.calab_avg;
                                    pgyroc->gyroc_mid[2] = (short)pgyroc->gyroc_zeroz.calab_avg;
                                    printf("gyro calibration done!!!(%.2f, %.2f, %.2f) \n", pgyroc->gyroc_zerox.calab_avg, pgyroc->gyroc_zeroy.calab_avg, pgyroc->gyroc_zeroz.calab_avg);
                                    printf("gyro calibration done!!!(%hd, %hd, %hd) \n", pgyroc->gyroc_mid[0], pgyroc->gyroc_mid[1], pgyroc->gyroc_mid[2]);
                                    pgyroc->gyroc_status = 0;
                                } else {
                                    //printf("calab gyro ret: %d\n", ret);
                                }
                            }
                        }
                        else {
                            if (dq < 0.02) {
                                steady_count++;
                            } else {
                                steady_count = 0;
                            }

                            dx = dx - gdx;
                            dy = dy - gdy;
                            dz = dz - gdz;

                            dq = dq * gv;

                            dist += 0.5 * dq * dt * dt + cuv * dt;

                            cuv = cuv + dq * dt;

                            if (steady_count > 20) {
                                cuv = 0;
                                steady_count = 0;
                            }

                            //printf("[move] g: %.4lf (%.2lf, %.2lf, %.2lf) \n", dq, dx, dy, dz);
                            //printf("[move] d: %.4lf v: %.4lf, a: %.4lf\n", dist, cuv, dq);
                            //printf("[move] v: %.4lf\n", cuv);
                        }
                        
                        //printf("accel calab x = %hd, y = %hd, z = %hd \n", accel[0], accel[1], accel[2]);
                    }
                }
                
                if ((sensors & INV_XYZ_GYRO) && (procede & PRINT_GYRO)) {
                    //printf("gyro:[%.5hd, %.5hd, %.5hd] \n");
                    //send_packet(PACKET_TYPE_GYRO, gyro);
                    mgyx = pgyroc->gyroc_mid[0];
                    mgyy = pgyroc->gyroc_mid[1];
                    mgyz = pgyroc->gyroc_mid[2];

                    if (mgyx != 0) {
                        dx = gyro[0]-mgyx;
                        dy = gyro[1]-mgyy;
                        dz = gyro[2]-mgyz;

                        //printf("gyro:[%.5hd, %.5hd, %.5hd] \n");
                        //printf("[gyro] %.2lf, %.2lf, %.2lf \n", dx, dy, dz);
                        
                        dx = dx / dgs;
                        dy = dy / dgs;
                        dz = dz / dgs;

                        printf("[gyro] %.3lf, %.3lf, %.3lf \n", dx, dy, dz);
                    }
                }

                accel += 3;
                gyro += 3;
                more --;
            }
#endif

#if GSENSOR_TIME_LOG
            clock_gettime(CLOCK_REALTIME, &gyrotdif[1]);

            d_ms = test_time_diff(&gyrotime, &gyrotdif[1], 1000);
            printf("    time comsuming: %d us\n", d_ms);
#endif

        }
#else
        while (1) {
            clock_gettime(CLOCK_REALTIME, &gyrotime);

            //ch = getchar();
            
            printf("[gyrotime] sec:%d, ms:%llu, ch:%c\n", gyrotime.tv_sec, time_get_ms(&gyrotime), ch);

            //usleep(1200000);
            if (ch)
                /* A byte has been received via USB. See handle_input for a list of
                 * valid commands.
                 */
                /*
                ret = handle_input(ch);
                if (ret) {
                    printf("handle_input failed, ret: %d \n", ret);
                }
                */
#if 0
            if (hal->motion_int_mode) {
                /* Enable motion interrupt. */
	    		mpu_lp_motion_interrupt(500, 1, 5);
                     usleep(600000);
                     hal->new_gyro = 1;
                /* Restore the previous sensor configuration. */
                     mpu_lp_motion_interrupt(0, 0, 0);
                    //hal->motion_int_mode = 0;
             }
#endif
            
            hal->new_gyro = 1;
            newgyro = hal->new_gyro;

            while (!newgyro) {
                msync(hal, sizeof(struct hal_s), MS_SYNC);
            
                //printf("[gyro] sensors:0x%x, new_gyro:0x%x\n", hal->sensors, hal->new_gyro);            
                hal->new_gyro = 1;
                newgyro = hal->new_gyro;
            }
            
            if (!hal->sensors || !hal->new_gyro) {
                /* Put the MSP430 to sleep until a timer interrupt or data ready
                 * interrupt is detected.
                 */
                //__bis_SR_register(LPM0_bits + GIE);
                printf("[gyro] continue!!! sensors:0x%x, new_gyro:0x%x\n", hal->sensors, hal->new_gyro);
                continue;
            }
        
            if (hal->new_gyro && hal->dmp_on) {
                short gyro[3], accel[3], sensors;
                unsigned char more;
                unsigned int quat[4];
                double q[4], pitch, roll, yaw;
                double q30 = 1073741824.0;
                //long q30 = 2.0;
                float pose[3];
                double r2d = 57.2957;
                /* This function gets new data from the FIFO when the DMP is in
                 * use. The FIFO can contain any combination of gyro, accel,
                 * quaternion, and gesture data. The sensors parameter tells the
                 * caller which data fields were actually populated with new data.
                 * For example, if sensors == (INV_XYZ_GYRO | INV_WXYZ_QUAT), then
                 * the FIFO isn't being filled with accel data.
                 * The driver parses the gesture data to determine if a gesture
                 * event has occurred; on an event, the application will be notified
                 * via a callback (assuming that a callback function was properly
                 * registered). The more parameter is non-zero if there are
                 * leftover packets in the FIFO.
                 */
                ret = dmp_read_fifo(gyro, accel, quat, &gyrotdif[0], &sensors, &more);
                if (ret != 0 && ret != -41) {
                    printf("warning!!! dmp_read_fifo ret %d, sensors: 0x%.8x\n", ret, sensors);
                }

                if (!more)
                    hal->new_gyro = 0;
                /* Gyro and accel data are written to the FIFO by the DMP in chip
                 * frame and hardware units. This behavior is convenient because it
                 * keeps the gyro and accel outputs of dmp_read_fifo and
                 * mpu_read_fifo consistent.
                 */
                if (sensors & INV_XYZ_GYRO && hal->report & PRINT_GYRO) {
                    //printf("    INV_XYZ_GYRO\n");

                    //printf("gyro:[%.5hd, %.5hd, %.5hd] s:%d, ms:%llu DMP\n", gyro[0], gyro[1], gyro[2], gyrotime.tv_sec, time_get_ms(&gyrotime));
                    dmp_gyro_shift(gyro[0], gyro[1], gyro[2], gyrolsb);
                    //send_packet(PACKET_TYPE_GYRO, gyro);
                }
                if (sensors & INV_XYZ_ACCEL && hal->report & PRINT_ACCEL) {
                    //printf("    INV_XYZ_ACCEL\n");
                    printf("accel:[%.5hd, %.5hd, %.5hd] ms:%llu lsb:%d DMP\n", accel[0], accel[1], accel[2], time_get_ms(&gyrotime), acclsb);
                    //dmp_accel_shift(accel[0], accel[1], accel[2], acclsb);
                    //send_packet(PACKET_TYPE_ACCEL, accel);
                }
                /* Unlike gyro and accel, quaternions are written to the FIFO in
                 * the body frame, q30. The orientation is set by the scalar passed
                 * to dmp_set_orientation during initialization.
                 */
                if (sensors & INV_WXYZ_QUAT && hal->report & PRINT_QUAT) {
                    //printf("    INV_WXYZ_QUAT\n");
                    //printf("[gyrotime] sec:%d, ms:%llu\n", gyrotime.tv_sec, time_get_ms(&gyrotime));

                    //printf("quat:[%d, %d, %d, %d] - 1\n", quat[0], quat[1], quat[2], quat[3]);
                    
                    q[0] = (double)quat[0] / q30;
                    q[1] = (double)quat[1] / q30;
                    q[2] = (double)quat[2] / q30;
                    q[3] = (double)quat[3] / q30;
                    
                    //printf("quat:[%lf, %lf, %lf, %lf] - 2\n", q[0], q[1], q[2], q[3]);
                    
                    //pitch  = asin(-2 * q[1] * q[2] + 2 * q[0]* q[2])* 57.3; // pitch
                    //roll = atan2(2 * q[2] * q[3] + 2 * q[0] * q[1], -2 * q[1] * q[1] - 2 * q[2]* q[2] + 1)* 57.3; // roll
                    //yaw = atan2(2*(q[1]*q[2] + q[0]*q[3]), -2 * q[2]*q[2] - 2 * q[3]*q[3] + 1) * 57.3;//yaw

                    yaw = atan2(2*q[2]*q[0]-2*q[1]*q[3] , 1 - 2*q[2]*q[2] - 2*q[3]*q[3]);
                    pitch = asin(2*q[1]*q[2] + 2*q[3]*q[0]);
                    roll = atan2(2*q[1]*q[0]-2*q[2]*q[3] , 1 - 2*q[1]*q[1] - 2*q[3]*q[3]);
                    
                    pose[0] = pitch * r2d;
                    pose[1] = roll * r2d;
                    pose[2] = yaw * r2d;

                    printf("pose:[%3.1f, %3.1f, %3.1f] s:%d, ms:%llu \n", pose[0], pose[1], pose[2], gyrotime.tv_sec, time_get_ms(&gyrotime));
                    
                    //send_packet(PACKET_TYPE_QUAT, quat);
                }
            }
            else if (hal->new_gyro) {
                short gyro[3], accel[3];
                unsigned char sensors, more;
                /* This function gets new data from the FIFO. The FIFO can contain
                 * gyro, accel, both, or neither. The sensors parameter tells the
                 * caller which data fields were actually populated with new data.
                 * For example, if sensors == INV_XYZ_GYRO, then the FIFO isn't
                 * being filled with accel data. The more parameter is non-zero if
                 * there are leftover packets in the FIFO.
                 */
                ret = mpu_read_fifo(gyro, accel, &gyrotdif[1], &sensors, &more);
                if (ret) {
                    printf("warning!!! mpu_read_fifo ret %d \n", ret);
                }

                if (!more)
                    hal->new_gyro = 0;
                if (sensors & INV_XYZ_GYRO && hal->report & PRINT_GYRO) {
                    printf("gyro:[%.5hd, %.5hd, %.5hd] ms:%llu %.1f MPU\n", gyro[0], gyro[1], gyro[2], time_get_ms(&gyrotime), gyrolsb);
                    //send_packet(PACKET_TYPE_GYRO, gyro);
                }
                if (sensors & INV_XYZ_ACCEL && hal->report & PRINT_ACCEL) {
                    printf("accel:[%.5hd, %.5hd, %.5hd] ms:%llu %d MPU\n", accel[0], accel[1], accel[2], time_get_ms(&gyrotime), acclsb);
                    //send_packet(PACKET_TYPE_ACCEL, accel);
                }
            }
        }

#endif

        goto end;
    }
    
    if (sel == 19){ /* command mode test ex[19 startsec secNum filename.bin clusterSize]*/ /* OP_SDWT */
#define FAT_TKSZ 4096

        int clustSize = 0;
        int startSec = 0, secNum = 0;
        int startAddr = 0, bLength = 0;
        char diskpath[128];
        FILE *dkf = 0;
        char *outbuf, *total=0;
        int ret = 0, len = 0, cnt = 0, acusz = 0, max = 0;
        int sLen = 0;

        startSec = arg0;
        secNum = arg1;
        clustSize = arg3;

        if (clustSize == 0) {
            clustSize = 512;
        }

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
            if (acusz > clustSize) {
                sLen = clustSize;
                acusz -= sLen;
            } else {
                sLen = acusz;
                acusz = 0;
            }

            msync(outbuf, sLen, MS_SYNC);
            len = tx_data(fm[0], outbuf, outbuf, 1, sLen, 1024*1024);
            printf("[%d]Send %d/%d bytes!!\n", cnt, len, sLen);

            cnt++;
            outbuf += len;
    	 }
/*
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
*/        
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
/*
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
*/

        printf("end!!!\n");
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
        uint8_t std[4] = {OP_STSEC_0, OP_STSEC_1, OP_STSEC_2, OP_STSEC_3};
        uint8_t ln[4] = {OP_STLEN_0, OP_STLEN_1, OP_STLEN_2, OP_STLEN_3};
        uint8_t staddr = 0, stlen = 0, btidx = 0;

        //tx8[0] = op[arg0];
        tx8[0] = arg0 & 0xff;
        tx8[1] = arg1;
/*
        if (arg0 > ARRY_MAX) {
            printf("Error!! Index overflow!![%d]\n", arg0);
            goto end;
        }
*/
        btidx = arg2 % 4;
        if (op[arg0] == OP_STSEC_0) {
            staddr = arg1 & (0xff << (8 * btidx));
            
            tx8[0] = std[btidx];
            tx8[1] = staddr;
            
            printf("start secter: %.8x, send:%.2x \n", arg1, staddr);
        }

        if (op[arg0] == OP_STLEN_0) {
            stlen = arg1 & (0xff << (8 * btidx));
            tx8[0] = ln[btidx];
            tx8[1] = stlen;
            
            printf("secter length: %.8x, send:%.2x\n", arg1, stlen);
        }

        bitset = 1;
        ioctl(fm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
        printf("Set spi 0 slave ready: %d\n", bitset);

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

        bitset = 0;
        ret = ioctl(fm[arg0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
        printf("Set spi%d data mode: %d\n", 0, bitset);
        
        bitset = 1;
        ioctl(fm[arg0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
        printf("Set spi 0 slave ready: %d\n", bitset);

        ret = ioctl(fm[arg0], SPI_IOC_WR_BITS_PER_WORD, &bits);
        if (ret == -1) 
            pabort("can't set bits per word"); 
 
        ret = ioctl(fm[arg0], SPI_IOC_RD_BITS_PER_WORD, &bits); 
        if (ret == -1) 
            pabort("can't get bits per word"); 

        if (bits == 16) {
            ret = tx_data_16(fm[arg0], rx16, tx16, 1, arg1, SPI_TRUNK_SZ);
            int i=0;
            printf("\n%d.", i);
            tmp16 = rx16;
            for (i = 0; i < ret; i+=2) {
                if (((i % 16) == 0) && (i != 0)) printf("\n%d.", i);
                printf("0x%.4x ", *tmp16);
                tmp16++;
            }
            
            printf("\n");

            i = 0;
            printf("\n%d.", i);
            tmp8 = (uint8_t *)rx16;
            for (i = 0; i < ret; i+=1) {
                if (((i % 16) == 0) && (i != 0)) printf("\n%d.", i);
                printf("0x%.2x ", *tmp8);
                tmp8++;
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

        mode &= ~SPI_AT_MODE_3;
        mode |= SPI_AT_MODE_1;

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

        mode &= ~SPI_AT_MODE_3;
        mode |= SPI_AT_MODE_1;

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
	mode &= ~SPI_AT_MODE_3;

	switch(arg0) {
		case 1:
    			mode |= SPI_AT_MODE_1;
			break;
		case 2:
    			mode |= SPI_AT_MODE_2;
			break;
		case 3:
    			mode |= SPI_AT_MODE_3;
			break;
		default:
    			mode |= SPI_AT_MODE_0;
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
    if (rx_buff) free(rx_buff);
    if (tx_buff) free(tx_buff);
    if (fd0) close(fd0);
    if (fd1) close(fd1);
    if (fpd) fclose(fpd);
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
