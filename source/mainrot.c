#include <stdint.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <unistd.h> 
#include <math.h>
#include <sys/mman.h> 

struct bitmapHeader_s {
    char aspbmpMagic[4];
    int    aspbhSize;
    char aspbhReserve[4];
    int    aspbhRawoffset;
    int    aspbiSize;
    int    aspbiWidth;
    int    aspbiHeight;
    int    aspbiCPP;
    int    aspbiCompMethd;
    int    aspbiRawSize;
    int    aspbiResoluH;
    int    aspbiResoluV;
    int    aspbiNumCinCP;
    int    aspbiNumImpColor;
};

struct intMbs32_s{
    union {
        uint32_t n;
        uint8_t d[4];
    };
};

struct aspMetaDataviaUSB_s{
  unsigned char  ASP_MAGIC_ASPC[4];  //byte[4] "ASPC"
  unsigned char IMG_HIGH[2];                   // byte[6]
  unsigned char  WIDTH_RESERVE[5];    // byte[11]
  unsigned char IMG_WIDTH[2];                // byte[13] 
  unsigned char MINGS_USE[2];                 // byte[15]
  unsigned char PRI_O_SEC;                 // byte[16]
  unsigned char Scaned_Page[2];                            //byte[18]
  unsigned char BKNote_Total_Layers;                    //byte[19]
  unsigned char MUSE_RESERVE[16];   // byte[35]
  unsigned char BKNote_Slice_idx;   // current image slice index of this Bank Note                            //byte[36]
  unsigned char BKNote_Block_idx;   // current image block index of this Bank Note                            //byte[37]
  unsigned char MCROP_RESERVE[27];   // byte[64]
  
  struct intMbs32_s CROP_POS_1;        //byte[68]
  struct intMbs32_s CROP_POS_2;        //byte[72]
  struct intMbs32_s CROP_POS_3;        //byte[76]
  struct intMbs32_s CROP_POS_4;        //byte[80]
  struct intMbs32_s CROP_POS_5;        //byte[84]
  struct intMbs32_s CROP_POS_6;        //byte[88]
  struct intMbs32_s CROP_POS_7;        //byte[92]
  struct intMbs32_s CROP_POS_8;        //byte[96]
  struct intMbs32_s CROP_POS_9;        //byte[100]
  struct intMbs32_s CROP_POS_10;        //byte[104]
  struct intMbs32_s CROP_POS_11;        //byte[108]
  struct intMbs32_s CROP_POS_12;        //byte[112]
  struct intMbs32_s CROP_POS_13;        //byte[116]
  struct intMbs32_s CROP_POS_14;        //byte[120]
  struct intMbs32_s CROP_POS_15;        //byte[124]
  struct intMbs32_s CROP_POS_16;        //byte[128]
  struct intMbs32_s CROP_POS_17;        //byte[132]
  struct intMbs32_s CROP_POS_18;        //byte[136]
  unsigned char  Start_Pos_1st;         //byte[137]
  unsigned char  Start_Pos_2nd;        //byte[138]
  unsigned char  End_Pos_All;            //byte[139]
  unsigned char  Start_Pos_RSV;        //byte[140], not using for now
  unsigned char  YLine_Gap;               //byte[141]
  unsigned char  Start_YLine_No;       //byte[142]
  unsigned short YLines_Recorded;     //byte[144] 16bits
  struct intMbs32_s CROP_POS_F1;        //byte[148]
  struct intMbs32_s CROP_POS_F2;        //byte[152]
  struct intMbs32_s CROP_POS_F3;        //byte[156]
  struct intMbs32_s CROP_POS_F4;        //byte[160]
  unsigned char EPOINT_RESERVE1[64];         //byte[224]
  unsigned char ASP_MAGIC_YL[2];    //byte[226]
  unsigned short MPIONT_LEN;           //byte[228] 16bits  
  unsigned char EXTRA_POINT[4];    //byte[232]
};

static FILE *find_save(char *dst, char *tmple)
{
    int i;
    FILE *f;
    for (i =0; i < 1000; i++) {
        sprintf(dst, tmple, i);
        f = fopen(dst, "r");
        if (!f) {
            //printf("open file [%s]\n", dst);
            break;
        } else {
            //printf("open file [%s] succeed \n", dst);
            fclose(f);
        }
    }
    f = fopen(dst, "w");
    return f;
}

extern int rotateBMPMf(int *cropinfo, char *bmpsrc, char *rotbuff, int *pmreal, char *headbuff, int midx);
extern int dbgBitmapHeader(struct bitmapHeader_s *ph, int len);
extern int dbgMetaUsb(struct aspMetaDataviaUSB_s *pmetausb);
extern uint32_t msb2lsb32(struct intMbs32_s *msb);

#define DUMP_ROT_BMP (1)
int main(int argc, char *argv[]) 
{
    char filepath[256];
    int len=0, ix=0, ret=0, err=0, rvs=0;
    
    printf("input argc: %d \n", argc);

    while (ix < argc) {
        printf("[%d]: %s \n", ix, argv[ix]);

        ix++;
    }

    if (argc > 1) {
        len = strlen(argv[1]);

        memcpy(filepath, argv[1], len);
        filepath[len] = '\0';

        printf("get filepath: %s \n", filepath);
    } else {
        err = -1;
        goto end;
    }

    FILE *f=0;

    f = fopen(filepath, "r");

    if (!f) {
        err = -2;
        goto end;
    }

    fseek(f, 0, SEEK_END);

    len = ftell(f);

    printf("file [%s] size: %d \n", filepath, len);

    fseek(f, 0, SEEK_SET);

    if (len <= 0) {
        err = -3;
        goto end;
    }


    char *bmphead=0, *bmpraw=0;
    
    bmphead = malloc(1080);
    bmpraw = malloc(len);

    if ((!bmphead) || (!bmpraw)) {
        err = -4;
        goto end;
    }

    ret = fread(bmpraw, 1, len, f);
    printf("read file %d ret: %d \n", len, ret);

    memcpy(bmphead, bmpraw, 1078);

    fclose(f);

    int size=0, tail=0, mul=0, shf=0;
    char *pt=0, *pmeta=0;
    struct aspMetaDataviaUSB_s * pusbmeta = (struct aspMetaDataviaUSB_s *)pmeta;
    
    size = len;

    tail = size % 512;
    mul = (size - tail) / 512;

    printf("read file: [%s] size: %d, mul: %d, tail: %d \n", filepath, size, mul, tail);

    for (ix=0; ix < mul; ix++) {
        shf = (mul - ix) * 512;
        pt = bmpraw + shf;
        
        if (pt[0] == 'A') {
            if ((pt[1] == 'S') && (pt[2] == 'P') && (pt[3] == 'C')) {
                pmeta = pt;
                break;
            }
        }
        /*
        else {
            printf("%d. [0x%.8x]: 0x%.2x \n", ix, shf, pt[0]);
        }
        */
    }

    if (!pmeta) {
        printf("Error!!! can't find meta search count: %d \n", ix);
        err = -6;
        goto end;
    }

    pusbmeta = (struct aspMetaDataviaUSB_s *)pmeta;
    dbgMetaUsb(pusbmeta);

    int cropinfo[8];
    char *rotdst=0;

    cropinfo[0] = 78;
    cropinfo[1] = 159;
    cropinfo[2] = 200;
    cropinfo[3] = 50;
    cropinfo[4] = 1140;
    cropinfo[5] = 551;

    int mreal[8], utmp, crod;
    utmp = msb2lsb32(&pusbmeta->CROP_POS_F1);
    crod = utmp & 0xffff;
    utmp = utmp >> 16;
    mreal[0] = utmp;
    mreal[1] = crod;
    
    utmp = msb2lsb32(&pusbmeta->CROP_POS_F2);
    crod = utmp & 0xffff;
    utmp = utmp >> 16;
    mreal[2] = utmp;
    mreal[3] = crod;       
    utmp = msb2lsb32(&pusbmeta->CROP_POS_F3);
    crod = utmp & 0xffff;
    utmp = utmp >> 16;
    mreal[4] = utmp;
    mreal[5] = crod;

    utmp = msb2lsb32(&pusbmeta->CROP_POS_F4);
    crod = utmp & 0xffff;
    utmp = utmp >> 16;
    mreal[6] = utmp;
    mreal[7] = crod;

    char *rotbuff=0;

    rotbuff = malloc(200 * 50);
    if (!rotbuff) {
        err = -7;
        goto end;

    }

    memset(rotbuff, 0xaa, 200 * 50);

    struct bitmapHeader_s *bheader=0;
    bheader = malloc(sizeof (struct bitmapHeader_s));
    if (!bheader) {
        err = -5;
        goto end;
    }

    memcpy(&bheader->aspbmpMagic[2], bmphead, sizeof(struct bitmapHeader_s) - 2);
    dbgBitmapHeader(bheader, sizeof(struct bitmapHeader_s) - 2);

    if (bheader->aspbiHeight < 0) {
        bheader->aspbiHeight = 0 - bheader->aspbiHeight;
        memcpy(bmphead, &bheader->aspbmpMagic[2], sizeof(struct bitmapHeader_s) - 2);    
        rvs = 1;
    }
    
    rotateBMPMf(cropinfo, bmpraw + 1078, rotbuff, mreal, bmphead, 0);

    #if DUMP_ROT_BMP
    static char ptfileSave[] = "/home/root/rotate/rot_%.3d.bmp";
    char dumpath[256]={0};
    FILE *fdump=0;
    int abuf_size=0, bhlen=0;

    printf("show result bmp header: \n");

    memcpy(&bheader->aspbmpMagic[2], bmphead, sizeof(struct bitmapHeader_s) - 2);
    dbgBitmapHeader(bheader, sizeof(struct bitmapHeader_s) - 2);

    abuf_size = bheader->aspbiWidth * bheader->aspbiHeight;
    bhlen = bheader->aspbhRawoffset;

    if (rvs) {
        bheader->aspbiHeight = 0 - bheader->aspbiHeight;
        memcpy(bmphead, &bheader->aspbmpMagic[2], sizeof(struct bitmapHeader_s) - 2);
    }
    
    fdump = find_save(dumpath, ptfileSave);
    if (fdump) {
        printf("find save bmp [%s] succeed!!! \n", dumpath);
    }

    ret = fwrite((char*)bmphead, 1, bhlen, fdump);
    printf("write [%s] size: %d / %d !!! \n", dumpath, ret, bhlen);

    ret = fwrite((char*)rotbuff, 1, abuf_size, fdump);
    printf("write [%s] size: %d / %d !!! \n", dumpath, ret, abuf_size);
    

    fflush(fdump);
    fclose(fdump);
    sync();

    #endif

    
    
    end:

    printf("end err: %d \n", err);

    if (bheader) free(bheader);
    if (bmpraw) free(bmpraw);
    if (bmphead) free(bmphead);
    if (rotbuff) free(rotbuff);
    
    
    return err;
    
}


