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

extern int rotateBMPMf(char *rotbuff, char *headbuff, int *cropinfo, char *bmpsrc, int *pmreal, int midx);
extern int dbgBitmapHeader(struct bitmapHeader_s *ph, int len);
extern int dbgMetaUsb(struct aspMetaDataviaUSB_s *pmetausb);
extern uint32_t msb2lsb32(struct intMbs32_s *msb);

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

static int shmem_dump(char *src, int size)
{
    char str[128];
    int inc;
    if (!src) return -1;

    inc = 0;
    printf("memdump[0x%.8x] sz%d: \n", (uint32_t)src, size);
    while (inc < size) {
        printf("%.2x ", *src);

        if (!((inc+1) % 16)) {
            printf(" %d \n", inc+1);
        }
        inc++;
        src++;
    }

    printf("\n");

    return inc;
}

static int readBMPmeta(int *pmreal,char **raw, FILE *file, int len)
{
    int err=0, ret=0, ix=0;
    char *bmpraw=0;
    int size=0, tail=0, mul=0, shf=0;
    char *pt=0, *pmeta=0;
    int utmp, crod;
    struct aspMetaDataviaUSB_s *pusbmeta=0;

    bmpraw = malloc(len);
    if (!bmpraw) {
        err = -1;
        goto ErrEnd;
    }

    ret = fread(bmpraw, 1, len, file);

    if (ret != len) {
        err = -2;
        goto ErrEnd;
    }

    size = len;

    tail = size % 512;
    mul = (size - tail) / 512;

    printf("read file size: %d, mul: %d, tail: %d \n", size, mul, tail);

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
        err = -3;
        goto ErrEnd;
    }

    pusbmeta = (struct aspMetaDataviaUSB_s *)pmeta;
    //dbgMetaUsb(pusbmeta);
            
    utmp = msb2lsb32(&pusbmeta->CROP_POS_F1);
    crod = utmp & 0xffff;
    utmp = utmp >> 16;
    pmreal[0] = utmp;
    pmreal[1] = crod;
    
    utmp = msb2lsb32(&pusbmeta->CROP_POS_F2);
    crod = utmp & 0xffff;
    utmp = utmp >> 16;
    pmreal[2] = utmp;
    pmreal[3] = crod;       
    utmp = msb2lsb32(&pusbmeta->CROP_POS_F3);
    crod = utmp & 0xffff;
    utmp = utmp >> 16;
    pmreal[4] = utmp;
    pmreal[5] = crod;

    utmp = msb2lsb32(&pusbmeta->CROP_POS_F4);
    crod = utmp & 0xffff;
    utmp = utmp >> 16;
    pmreal[6] = utmp;
    pmreal[7] = crod;

    *raw = bmpraw;

    return 0;
    
    ErrEnd:

    if (bmpraw) free(bmpraw);

    return err;
}

static int readRotFile(int *pmreal,char **raw, FILE *f)
{
    int err=0, len=0, ret=0;
    
    if (!f) {
        err = -1;
        goto Error;
    }

    fseek(f, 0, SEEK_END);

    len = ftell(f);

    //printf("file [%s] size: %d \n", filepath, len);

    fseek(f, 0, SEEK_SET);

    if (len <= 0) {
        err = -2;
        goto Error;
    }
    
    ret = readBMPmeta(pmreal, raw, f, len);
    if (ret) {
        err = -3 + ret*10;
        goto Error;
    }

    Error:

    if (f) fclose(f);

    return err;
}

static int doRot2BMP(char *rotraw, char *rothead, int *cropinfo, FILE *f)
{
    int err=0, ret=0;
    char *rotbuf=0, *bmpraw=0;
    int mreal[8]={0};
    struct bitmapHeader_s bheader[1]={0};
    
    printf("cropinfo[4]: (%d, %d, %d, %d) \n", cropinfo[0], cropinfo[1], cropinfo[2], cropinfo[3]);    

    rotbuf = rotraw;
    
    ret = readRotFile(mreal, &bmpraw, f);
    if (ret) {
        err = -2 + ret*10;
        goto End;
    }

    memset(rotbuf, 0xff, cropinfo[2] * cropinfo[3]);
    
    memcpy(rothead, bmpraw, 1078);

    memcpy(&bheader->aspbmpMagic[2], bmpraw, sizeof(struct bitmapHeader_s) - 2);
    dbgBitmapHeader(bheader, sizeof(struct bitmapHeader_s) - 2);
    
    rotateBMPMf(rotbuf, rothead, cropinfo, bmpraw+1078, mreal, 0);

    free(bmpraw);

    return 0;

    End:

    if (rotbuf) free(rotbuf);
    if (bmpraw) free(bmpraw);

    return err;
}

static int saveRot2BMP(char *raw, char *head, int *dat, int idx)
{
    int ret=0, err=0, len=0;
    char ptfileInfo[] = "/home/root/rotate/rot_%d_%d_%d_%.2d";
    char filetail[] = "_%.3d.bmp";
    char ptfileSave[256]={0};
    char dumpath[256]={0};
    FILE *fdump=0;
    int abuf_size=0, bhlen=0, bmph=0;
    struct bitmapHeader_s *bheader=0;

    sprintf(ptfileSave, ptfileInfo, dat[0], dat[1], dat[2], idx);
    strcat(ptfileSave, filetail);

    //printf("find file [%s] org[%s] \n", ptfileSave, ptfileInfo);
    
    bheader = malloc(sizeof (struct bitmapHeader_s));
    if (!bheader) {
        err = -3;
        goto end;
    }
    
    memcpy(&bheader->aspbmpMagic[2], head, sizeof(struct bitmapHeader_s) - 2);
    printf("show result bmp header: \n");
    dbgBitmapHeader(bheader, sizeof(struct bitmapHeader_s) - 2);

    if (bheader->aspbiHeight < 0) {
        bmph = 0 - bheader->aspbiHeight;
    }
    abuf_size = bheader->aspbiWidth * bmph;
    bhlen = bheader->aspbhRawoffset;
    
    fdump = find_save(dumpath, ptfileSave);
    if (fdump) {
        printf("find save bmp [%s] succeed!!! \n", dumpath);
    }

    ret = fwrite((char*)head, 1, bhlen, fdump);
    printf("write [%s] size: %d / %d !!! \n", dumpath, ret, bhlen);

    ret = fwrite((char*)raw, 1, abuf_size, fdump);
    printf("write [%s] size: %d / %d !!! \n", dumpath, ret, abuf_size);

    //shmem_dump(rotbuff, 512);

    fflush(fdump);
    fclose(fdump);
    sync();

    end:

    if (bheader) free(bheader);
    
    return err;
}

#define DUMP_ROT_BMP (1)
int main(int argc, char *argv[]) 
{
    char filepath[256];
    char bnotefile[128]="/home/root/banknote/ASP_%d_%.2d.bmp";
    int data[3][5]={{100, 83, 332, 200, 50},{500, 141, 334, 200, 50},{1000, 177, 330, 200, 50}};
    int len=0, ret=0, err=0, cnt=0, nt=0, ntd=0;
    FILE *f=0;
    int cropinfo[8];
    char *rotraw=0, *rothead=0;

    printf("input argc: %d config: dump(%d)\n", argc, DUMP_ROT_BMP);
    /*
    while (ix < argc) {
        printf("[%d]: %s \n", ix, argv[ix]);

        ix++;
    }
    */

    cropinfo[0] = 168;
    cropinfo[1] = 389 - 50;
    cropinfo[2] = 200;
    cropinfo[3] = 50;

    rothead = malloc(1080);
    if (!rothead) {
        err = -4;
        goto end;
    }

    for (nt=0; nt < 3; nt++) {
        ntd = data[nt][0];
        memcpy(cropinfo, &data[nt][1], sizeof(int) * 4);
        
        printf("[NTD_%.4d]: %.3d, %.3d, %.3d, %.3d \n", ntd, cropinfo[0], cropinfo[1], cropinfo[2], cropinfo[3]);

        len = cropinfo[2]*cropinfo[3];
        rotraw = malloc(len);
        if (!rotraw) {
            err = -5;
            goto end;
        }
        
        
        for (cnt=1; cnt < 99; cnt++) {
            sprintf(filepath, bnotefile, ntd, cnt);        

            f = fopen(filepath, "r");
            if (!f) {
                printf("get file [%s] failed!! \n", filepath);
                break;
            } else {
                printf("get file [%s] !! \n", filepath);
                memset(rotraw, 0xff, len);

                ret = doRot2BMP(rotraw, rothead, cropinfo, f);
                if (ret) {
                    err = -6 + ret*10;
                    goto end;
                }

                #if DUMP_ROT_BMP
                ret = saveRot2BMP(rotraw, rothead, &data[nt][0], cnt);
                if (ret) {
                    err = -7 + ret*10;
                    goto end;
                }
                #endif
            }
        }

        free(rotraw);
        rotraw = 0;
        
    }   

    end:

    printf("end err: %d \n", err);

    if (rotraw) free(rotraw);
    if (rothead) free(rothead);
    
    return err;
    
}


