/*
 * Copyright (C) 2020 Aspect Microsystems, Corp. All rights reserved.
 *
 * Author: Leo Chen <leoc@aspectmicrosystems.com.tw>
 *
 * License terms: GNU General Public License v2
 */
 
#include <stdint.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <unistd.h> 
#include <math.h>
#include <sys/mman.h> 

#define CFLOAT double

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

struct aspRectObj{
    CFLOAT aspRectLU[2];
    CFLOAT aspRectRU[2];    
    CFLOAT aspRectLD[2];
    CFLOAT aspRectRD[2];    
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

uint32_t msb2lsb32(struct intMbs32_s *msb)
{
    uint32_t lsb=0;
    int i=0;

    while (i < 4) {
        lsb = lsb << 8;
        
        lsb |= msb->d[i];
        
        //printf("[%d] :0x%.2x <- 0x%.2x \n", i, lsb & 0xff, msb->d[i]);
        
        i++;
    }

    //printf("msb2lsb32() msb:0x%.8x -> lsb:0x%.8x \n", msb->n, lsb);
    
    return lsb;
}

int dbgMetaUsb(struct aspMetaDataviaUSB_s *pmetausb) 
{
#define VERB_INFO_USB (0)
    char *pch=0;
    uint32_t head=0;
    int ix=0;

    head = (uint32_t)pmetausb->ASP_MAGIC_ASPC;
    
    msync(pmetausb, sizeof(struct aspMetaDataviaUSB_s), MS_SYNC);
    
    printf("[METAU]********************************************\n");
    
    printf("[METAU](%.3d) ASP_MAGIC_ASPC: 0x%.2x 0x%.2x 0x%.2x 0x%.2x\n", (uint32_t)pmetausb->ASP_MAGIC_ASPC - head, pmetausb->ASP_MAGIC_ASPC[0], pmetausb->ASP_MAGIC_ASPC[1], 
                  pmetausb->ASP_MAGIC_ASPC[2], pmetausb->ASP_MAGIC_ASPC[3]);

    printf("[METAU](%.3d) IMG_HIGH: 0x%.2x 0x%.2x (%d)   \n",(uint32_t)&(pmetausb->IMG_HIGH)  - head, pmetausb->IMG_HIGH[0], pmetausb->IMG_HIGH[1], 
                   pmetausb->IMG_HIGH[0] | (pmetausb->IMG_HIGH[1] << 8)); 

    #if VERB_INFO_USB
    for (ix=0; ix < 5; ix++) {
    printf("[METAU](%.3d) WIDTH_RESERVE[%d]: 0x%.2x    \n",(uint32_t)&(pmetausb->WIDTH_RESERVE[ix])  - head, ix, pmetausb->WIDTH_RESERVE[ix]); 
    }
    #endif

    printf("[METAU](%.3d) IMG_WIDTH: 0x%.2x 0x%.2x (%d)   \n",(uint32_t)&(pmetausb->IMG_WIDTH)  - head, pmetausb->IMG_WIDTH[0], pmetausb->IMG_WIDTH[1], 
                   pmetausb->IMG_WIDTH[0] | (pmetausb->IMG_WIDTH[1] << 8)); 

    printf("[METAU](%.3d) MINGS_USE: 0x%.2x 0x%.2x   \n",(uint32_t)&(pmetausb->MINGS_USE) - head, pmetausb->MINGS_USE[0], pmetausb->MINGS_USE[1]); 

    printf("[METAU](%.3d) PRI_O_SEC: 0x%.2x    \n",(uint32_t)&(pmetausb->PRI_O_SEC) - head, pmetausb->PRI_O_SEC); 

    printf("[METAU](%.3d) Scaned_Page: %d    \n",(uint32_t)&(pmetausb->Scaned_Page) - head, (pmetausb->Scaned_Page[0] << 8) | pmetausb->Scaned_Page[1]); 

    printf("[METAU](%.3d) BKNote_Total_Layers: %d    \n",(uint32_t)&(pmetausb->BKNote_Total_Layers) - head, pmetausb->BKNote_Total_Layers); 

    printf("[METAU](%.3d) BKNote_Slice_idx: %d    \n",(uint32_t)&(pmetausb->BKNote_Slice_idx) - head, pmetausb->BKNote_Slice_idx); 
    
    printf("[METAU](%.3d) BKNote_Block_idx: %d    \n",(uint32_t)&(pmetausb->BKNote_Block_idx) - head, pmetausb->BKNote_Block_idx); 

    #if VERB_INFO_USB
    for (ix=0; ix < 27; ix++) {
    printf("[METAU](%.3d) MCROP_RESERVE[%d]: 0x%.2x    \n",(uint32_t)&(pmetausb->MCROP_RESERVE[ix]) - head, ix, pmetausb->MCROP_RESERVE[ix]); 
    }
    #endif

    printf("[METAU](%.3d) CROP_POSX_01: %d, %d\n", (uint32_t)(&pmetausb->CROP_POS_1) - head, msb2lsb32(&pmetausb->CROP_POS_1) >> 16, msb2lsb32(&pmetausb->CROP_POS_1) & 0xffff);                      //byte[68]
    printf("[METAU](%.3d) CROP_POSX_02: %d, %d\n", (uint32_t)&pmetausb->CROP_POS_2 - head, msb2lsb32(&pmetausb->CROP_POS_2) >> 16, msb2lsb32(&pmetausb->CROP_POS_2) & 0xffff);                      //byte[72]
    printf("[METAU](%.3d) CROP_POSX_03: %d, %d\n", (uint32_t)&pmetausb->CROP_POS_3 - head, msb2lsb32(&pmetausb->CROP_POS_3) >> 16, msb2lsb32(&pmetausb->CROP_POS_3) & 0xffff);                      //byte[76]
    printf("[METAU](%.3d) CROP_POSX_04: %d, %d\n", (uint32_t)&pmetausb->CROP_POS_4 - head, msb2lsb32(&pmetausb->CROP_POS_4) >> 16, msb2lsb32(&pmetausb->CROP_POS_4) & 0xffff);                      //byte[80]
    printf("[METAU](%.3d) CROP_POSX_05: %d, %d\n", (uint32_t)&pmetausb->CROP_POS_5 - head, msb2lsb32(&pmetausb->CROP_POS_5) >> 16, msb2lsb32(&pmetausb->CROP_POS_5) & 0xffff);                      //byte[84]
    printf("[METAU](%.3d) CROP_POSX_06: %d, %d\n", (uint32_t)&pmetausb->CROP_POS_6 - head, msb2lsb32(&pmetausb->CROP_POS_6) >> 16, msb2lsb32(&pmetausb->CROP_POS_6) & 0xffff);                      //byte[88]
    printf("[METAU](%.3d) CROP_POSX_07: %d, %d\n", (uint32_t)&pmetausb->CROP_POS_7 - head, msb2lsb32(&pmetausb->CROP_POS_7) >> 16, msb2lsb32(&pmetausb->CROP_POS_7) & 0xffff);                      //byte[92]
    printf("[METAU](%.3d) CROP_POSX_08: %d, %d\n", (uint32_t)&pmetausb->CROP_POS_8 - head, msb2lsb32(&pmetausb->CROP_POS_8) >> 16, msb2lsb32(&pmetausb->CROP_POS_8) & 0xffff);                      //byte[96]
    printf("[METAU](%.3d) CROP_POSX_09: %d, %d\n", (uint32_t)&pmetausb->CROP_POS_9 - head, msb2lsb32(&pmetausb->CROP_POS_9) >> 16, msb2lsb32(&pmetausb->CROP_POS_9) & 0xffff);                      //byte[100]
    printf("[METAU](%.3d) CROP_POSX_10: %d, %d\n", (uint32_t)&pmetausb->CROP_POS_10 - head, msb2lsb32(&pmetausb->CROP_POS_10) >> 16, msb2lsb32(&pmetausb->CROP_POS_10) & 0xffff);                      //byte[104]
    printf("[METAU](%.3d) CROP_POSX_11: %d, %d\n", (uint32_t)&pmetausb->CROP_POS_11 - head, msb2lsb32(&pmetausb->CROP_POS_11) >> 16, msb2lsb32(&pmetausb->CROP_POS_11) & 0xffff);                      //byte[108]
    printf("[METAU](%.3d) CROP_POSX_12: %d, %d\n", (uint32_t)&pmetausb->CROP_POS_12 - head, msb2lsb32(&pmetausb->CROP_POS_12) >> 16, msb2lsb32(&pmetausb->CROP_POS_12) & 0xffff);                      //byte[112]
    printf("[METAU](%.3d) CROP_POSX_13: %d, %d\n", (uint32_t)&pmetausb->CROP_POS_13 - head, msb2lsb32(&pmetausb->CROP_POS_13) >> 16, msb2lsb32(&pmetausb->CROP_POS_13) & 0xffff);                      //byte[116]
    printf("[METAU](%.3d) CROP_POSX_14: %d, %d\n", (uint32_t)&pmetausb->CROP_POS_14 - head, msb2lsb32(&pmetausb->CROP_POS_14) >> 16, msb2lsb32(&pmetausb->CROP_POS_14) & 0xffff);                      //byte[120]
    printf("[METAU](%.3d) CROP_POSX_15: %d, %d\n", (uint32_t)&pmetausb->CROP_POS_15 - head, msb2lsb32(&pmetausb->CROP_POS_15) >> 16, msb2lsb32(&pmetausb->CROP_POS_15) & 0xffff);                      //byte[124]
    printf("[METAU](%.3d) CROP_POSX_16: %d, %d\n", (uint32_t)&pmetausb->CROP_POS_16 - head, msb2lsb32(&pmetausb->CROP_POS_16) >> 16, msb2lsb32(&pmetausb->CROP_POS_16) & 0xffff);                      //byte[128]
    printf("[METAU](%.3d) CROP_POSX_17: %d, %d\n", (uint32_t)&pmetausb->CROP_POS_17 - head, msb2lsb32(&pmetausb->CROP_POS_17) >> 16, msb2lsb32(&pmetausb->CROP_POS_17) & 0xffff);                      //byte[132]
    printf("[METAU](%.3d) CROP_POSX_18: %d, %d\n", (uint32_t)&pmetausb->CROP_POS_18 - head, msb2lsb32(&pmetausb->CROP_POS_18) >> 16, msb2lsb32(&pmetausb->CROP_POS_18) & 0xffff);                      //byte[136]
    printf("[METAU](%.3d) YLine_Gap: %.d      \n", (uint32_t)&pmetausb->YLine_Gap - head, pmetausb->YLine_Gap); 
    printf("[METAU](%.3d) Start_YLine_No: %d      \n", (uint32_t)&pmetausb->Start_YLine_No - head, pmetausb->Start_YLine_No); 
    pch = (char *)&pmetausb->YLines_Recorded;
    printf("[METAU](%.3d) YLines_Recorded: %d      \n", (uint32_t)&pmetausb->YLines_Recorded - head, (pch[0] << 8) | pch[1]); 

    printf("[METAU](%.3d) CROP_POSX_F01: %d, %d\n", (uint32_t)(&pmetausb->CROP_POS_F1) - head, msb2lsb32(&pmetausb->CROP_POS_F1) >> 16, msb2lsb32(&pmetausb->CROP_POS_F1) & 0xffff);                      //byte[148]
    printf("[METAU](%.3d) CROP_POSX_F02: %d, %d\n", (uint32_t)&pmetausb->CROP_POS_F2 - head, msb2lsb32(&pmetausb->CROP_POS_F2) >> 16, msb2lsb32(&pmetausb->CROP_POS_F2) & 0xffff);                      //byte[152]
    printf("[METAU](%.3d) CROP_POSX_F03: %d, %d\n", (uint32_t)&pmetausb->CROP_POS_F3 - head, msb2lsb32(&pmetausb->CROP_POS_F3) >> 16, msb2lsb32(&pmetausb->CROP_POS_F3) & 0xffff);                      //byte[156]
    printf("[METAU](%.3d) CROP_POSX_F04: %d, %d\n", (uint32_t)&pmetausb->CROP_POS_F4 - head, msb2lsb32(&pmetausb->CROP_POS_F4) >> 16, msb2lsb32(&pmetausb->CROP_POS_F4) & 0xffff);                      //byte[160]
    
    printf("[METAU](%.3d) EPOINT_RESERVE1      \n", (uint32_t)pmetausb->EPOINT_RESERVE1 - head); 

    printf("[METAU](%.3d) ASP_MAGIC_YL: 0x%.2x 0x%.2x\n", (uint32_t)pmetausb->ASP_MAGIC_YL - head, pmetausb->ASP_MAGIC_YL[0], pmetausb->ASP_MAGIC_YL[1]);

    printf("[METAU](%.3d) MPIONT_LEN: %d      \n", (uint32_t)&pmetausb->MPIONT_LEN - head, pmetausb->MPIONT_LEN); 

    printf("[METAU](%.3d) EXTRA_POINT[4]: 0x%.2x, 0x%.2x, 0x%.2x, 0x%.2x  \n", (uint32_t)pmetausb->EXTRA_POINT - head, pmetausb->EXTRA_POINT[0], pmetausb->EXTRA_POINT[1], pmetausb->EXTRA_POINT[2], pmetausb->EXTRA_POINT[3]);


    printf("[METAU]********************************************\n");
    return 0;
}

int dbgBitmapHeader(struct bitmapHeader_s *ph, int len) 
{

    msync(ph, sizeof(struct bitmapHeader_s), MS_SYNC);

    printf("[BMP]********************************************\n");

    printf("[BMP]debug print bitmap header length: %d\n", len);

    printf("[BMP]MAGIC NUMBER: [%c] [%c] \n",ph->aspbmpMagic[2], ph->aspbmpMagic[3]);         

    printf("[BMP]FILE TOTAL LENGTH: [%d] \n",ph->aspbhSize);                                                 // mod

    printf("[BMP]HEADER TOTAL LENGTH: [%d] \n",ph->aspbhRawoffset);          

    printf("[BMP]INFO HEADER LENGTH: [%d] \n",ph->aspbiSize);          

    printf("[BMP]WIDTH: [%d] \n",ph->aspbiWidth);                                                          // mod

    printf("[BMP]HEIGHT: [%d] \n",ph->aspbiHeight);                                                        // mod
    
    printf("[BMP]NUM OF COLOR PLANES: [%d] \n",ph->aspbiCPP & 0xffff);          

    printf("[BMP]BITS PER PIXEL: [%d] \n",ph->aspbiCPP >> 16);          

    printf("[BMP]COMPRESSION METHOD: [%d] \n",ph->aspbiCompMethd);          

    printf("[BMP]SIZE OF RAW: [%d] \n",ph->aspbiRawSize);                                            // mod

    printf("[BMP]HORIZONTAL RESOLUTION: [%d] \n",ph->aspbiResoluH);          

    printf("[BMP]VERTICAL RESOLUTION: [%d] \n",ph->aspbiResoluV);          

    printf("[BMP]NUM OF COLORS IN CP: [%d] \n",ph->aspbiNumCinCP);          

    printf("[BMP]NUM OF IMPORTANT COLORS: [%d] \n",ph->aspbiNumImpColor);          

    printf("[BMP]********************************************\n");

    return 0;
}

static inline char* getPixel(char *rawCpy, int dx, int dy, int rowsz, int bitset) 
{
    return (rawCpy + dx * bitset + dy * rowsz);
}
           
static void* aspMemalloc(int size, int midx) 
{
    void *pt=0;

    pt = malloc(size);
    //printf("<<<<<<<<  aspMemalloc [0x%.8x] %d  %d \n", (uint32_t)pt, size, midx);
    return pt;
}

static void aspFree(void *p, int pidx)
{
    //printf("<<<<<<<<  aspFree 0x%.8x \n", (uint32_t)p);
    if (p) {
        free(p);
    }
}

static CFLOAT aspMin(CFLOAT d1, CFLOAT d2) 
{
    if (d1 < d2) return d1;
    else return d2;
}

static int aspMaxInt(int d1, int d2) 
{
    if (d1 > d2) return d1;
    else return d2;
}

static int aspMinInt(int d1, int d2) 
{
    if (d1 < d2) return d1;
    else return d2;
}


static int getVectorFromP(CFLOAT *vec, CFLOAT *p1, CFLOAT *p2)
{
    CFLOAT a1, b, a2;
    CFLOAT x1, y1, x2, y2;

    if (vec == 0) return -1;
    if (p1 == 0) return -2;
    if (p2 == 0) return -3;

    x1 = p1[0];
    y1 = p1[1];

    x2 = p2[0];
    y2 = p2[1];


    vec[2] = 0;
#if CROP_CALCU_DETAIL
    printf("[vectP] input - p1 = (%lf, %lf), p2 = (%lf, %lf)\n", x1, y1, x2, y2);
#endif
    if (x1 == x2) {
    	if (y1 == y2) {
    	    return -4;
    	} else {
    	    a1 = 1;
    	    a2 = 1;
    	    b = -x1;
    	    vec[2] = 1;
    	}
    } else {
        b = ((x2 * y1) - (x1 * y2)) / (x2 - x1);

        if (x1 == 0) {
            a1 = 0;
        } else {
            a1 = ((x1 * y2) - (x1 * y1)) / ((x1 * x2) - (x1 * x1));
        }

        if (x2 == 0) {
            a2 = 0;
        } else {
            a2 = ((x2 * y2) - (x2 * y1)) / ((x2 * x2) - (x2 * x1));
        }
    }
#if CROP_CALCU_DETAIL
    printf("[vectP] output - a = %lf/%lf, b = %lf\n", a1, a2, b);
#endif

    if (a1 == 0) {
        vec[0] = a2;
    } else {
        vec[0] = a1;
    }

    vec[1] = b;

    return 0;

}

static int getCross(CFLOAT *v1, CFLOAT *v2, CFLOAT *pt)
{
    CFLOAT a1, a2, b1, b2, c1, c2;
    CFLOAT x, y;

    if (v1 == 0) return -1;
    if (v2 == 0) return -2;
    if (pt == 0) return -3;

    a1 = v1[0];
    b1 = v1[1];
    c1 = v1[2];

    a2 = v2[0];
    b2 = v2[1];
    c2 = v2[2];
#if CROP_CALCU_DETAIL
    printf("[Cross] input - v1 = (%lf, %lf, %lf) v2 = (%lf, %lf, %lf)\n", a1, b1, c1, a2, b2, c2);
#endif
    if (a1 == a2) return -4;

    if ((c1 == 1) && (c2 == 1)) {
        return -5;
    } else if (c1 == 1) {
        x = -(b1/a1);
        y = a2 * x + b2;
    } else if (c2 == 1) {
        x = -(b2/a2);
        y = a1 * x + b1;
    } else {
        y = ((a1 * b2) - (a2 * b1)) / (a1 - a2);
        x = (b2 - b1) / (a1 - a2);
    }
#if CROP_CALCU_DETAIL
    printf("[Cross] output - pt = (%lf, %lf)\n", x, y);
#endif
    pt[0] = x;
    pt[1] = y;
    
    return 0;
}

static int calcuRotateCoordinates(int *outi, CFLOAT *out, CFLOAT *in, CFLOAT *angle) 
{
    CFLOAT r=0;
    CFLOAT x1, y1;
    CFLOAT x2, y2;
    CFLOAT cosA, sinA;

    if (!out) return -1;
    if (!in) return -2;
    if (!outi) return -3;
    if (!angle) return -4;

    //printf("calcu rotate input :%lf, %lf, cos:%lf sin:%lf\n", in[0], in[1], angle[0], angle[1]);

    x1 = in[0];
    y1 = in[1];

    cosA = angle[0];
    sinA = angle[1];

    //r = angle * M_PI / piAngle;
    //x2 = x1*cos(r) - y1*sin(r);
    //y2 = x1*sin(r) + y1*cos(r);
    
    x2 = x1*cosA - y1*sinA;
    y2 = x1*sinA + y1*cosA;
    
    out[0] = x2;
    out[1] = y2;

    outi[0] = (int)(x2);
    outi[1] = (int)(y2);

    //printf("calcu rotate input :%lf, %lf, output: (%d, %d) (%lf, %lf) \n", in[0], in[1], outi[0], outi[1], out[0], out[1]);
    
    return 0;
}

static CFLOAT getAngle(CFLOAT *pSrc, CFLOAT *p1, CFLOAT *p2)
{
    CFLOAT angle = 0.0f;
    
    if ((p1[0] == p2[0]) && (p1[1] == p2[1])) return -1;
    if ((p1[0] == pSrc[0]) && (p1[1] == pSrc[1])) return -1;
    if ((p2[0] == pSrc[0]) && (p2[1] == pSrc[1])) return -1;
    
    CFLOAT va_x = p1[0] - pSrc[0];
    CFLOAT va_y = p1[1] - pSrc[1];

    CFLOAT vb_x = p2[0] - pSrc[0];
    CFLOAT vb_y = p2[1] - pSrc[1];

    CFLOAT productValue = (va_x * vb_x) + (va_y * vb_y);  
    CFLOAT va_val = sqrt(va_x * va_x + va_y * va_y);  
    CFLOAT vb_val = sqrt(vb_x * vb_x + vb_y * vb_y);  
    CFLOAT cosValue = productValue / (va_val * vb_val);      

    //printf("[AnG] cos: %.2lf \n", cosValue);

    if(cosValue < -1 && cosValue > -2) {
        cosValue = -1;
    } else if (cosValue > 1 && cosValue < 2) {
        cosValue = 1;
    }

    angle = acos(cosValue) * 180.0 / M_PI; 
    
    return angle;
}

static void dbgprintRect(struct aspRectObj *rect)
{
    
    printf("[DBG] LU[%.2lf, %.2lf] LD[%.2lf, %.2lf] RD[%.2lf, %.2lf] RU[%.2lf, %.2lf]\n", 
        rect->aspRectLU[0], rect->aspRectLU[1], 
        rect->aspRectLD[0], rect->aspRectLD[1], 
        rect->aspRectRD[0], rect->aspRectRD[1], 
        rect->aspRectRU[0], rect->aspRectRU[1]);
}

static int getRectVectorFromV(CFLOAT *vec, CFLOAT *p, CFLOAT *vecIn)
{
    CFLOAT a=0, b=0, c=0, aIn=0, cIn=0;
    CFLOAT x, y;

    if (vec == 0) return -1;
    if (p == 0) return -2;
    if (vecIn == 0) return -3;

    aIn = vecIn[0];
    cIn = vecIn[2];
    x = p[0];
    y = p[1];

    if (aIn == 0) {
        c = 1;
    } else if (cIn == 1) {
        a = 0;
    } else {
        a = -1 / aIn;
    }
    
    if (c == 1) {
        a = 1;
        b = -x;
    } else {
        b = y - (a * x);
    }
    vec[0] = a;
    vec[1] = b;
    vec[2] = c;

#if CROP_CALCU_DETAIL
    printf("[vectRV] output - a = %f, b = %f, c = %f\n", a, b, c);
#endif

    return 0;
}

static void setRectPoint(struct aspRectObj *pRectin, CFLOAT edwidth, CFLOAT edheight, CFLOAT *pst) 
{
    pRectin->aspRectLD[0] = pst[0];
    pRectin->aspRectLD[1] = pst[1];
    
    pRectin->aspRectLU[0] = pst[0];
    pRectin->aspRectLU[1] = pst[1] + edheight;

    pRectin->aspRectRU[0] = pst[0] + edwidth;
    pRectin->aspRectRU[1] = pst[1] + edheight;

    pRectin->aspRectRD[0] = pst[0] + edwidth;
    pRectin->aspRectRD[1] = pst[1];

}

static inline int getPtTran(CFLOAT *ptout, CFLOAT *rangle, CFLOAT *offset, CFLOAT *ptin)
{
    CFLOAT pLU[2];
    int ret=0, LUt[2];
    
    pLU[0] = ptin[0] + offset[0];
    pLU[1] = ptin[1] + offset[1];
    
    ret = calcuRotateCoordinates(LUt, ptout, pLU, rangle);
    
    //printf("[PT] in:(%4.2lf, %4.2lf), out:(%4.2lf, %4.2lf) ret: %d\n", ptin[0], ptin[1], ptout[0], ptout[1], ret);

    return ret;
}

#define LOG_GETRECTRAN_EN (0)
static int getRectTran(struct aspRectObj *pRectin, CFLOAT dg, CFLOAT *offset, struct aspRectObj *pRectout)
{
    int ret=0;
    CFLOAT piAngle = 180.0, thacos=0, thasin=0, rangle[2], theta=0;
    CFLOAT pLU[2], pLD[2], pRU[2], pRD[2];
    CFLOAT *LUn, *RUn, *LDn, *RDn;
    int LUt[2], RUt[2], LDt[2], RDt[2];

    theta = dg;

    theta = theta * M_PI / piAngle;

    thacos = cos(theta);
    thasin = sin(theta);
    
    rangle[0] = thacos;
    rangle[1] = thasin;

    #if LOG_GETRECTRAN_EN
    printf("[CHR] getRectTran() degree: %4.2lf, offset: (%4.2lf, %4.2lf) \n", dg, offset[0], offset[1]);
    #endif
    
    ret = getPtTran(pRectout->aspRectLU, rangle, offset, pRectin->aspRectLU);
    #if LOG_GETRECTRAN_EN
    printf("[CHR] LU: (%4.2lf, %4.2lf) ->  (%4.2lf, %4.2lf) ret: %d\n", 
        pRectin->aspRectLU[0], pRectin->aspRectLU[1], pRectout->aspRectLU[0], pRectout->aspRectLU[1], ret);
    #endif
    
    ret = getPtTran(pRectout->aspRectLD, rangle, offset, pRectin->aspRectLD);
    #if LOG_GETRECTRAN_EN
    printf("[CHR] LD: (%4.2lf, %4.2lf) ->  (%4.2lf, %4.2lf) ret: %d\n", 
        pRectin->aspRectLD[0], pRectin->aspRectLD[1], pRectout->aspRectLD[0], pRectout->aspRectLD[1], ret);
    #endif
        
    ret = getPtTran(pRectout->aspRectRD, rangle, offset, pRectin->aspRectRD);
    #if LOG_GETRECTRAN_EN
    printf("[CHR] RD: (%4.2lf, %4.2lf) ->  (%4.2lf, %4.2lf) ret: %d\n", 
        pRectin->aspRectRD[0], pRectin->aspRectRD[1], pRectout->aspRectRD[0], pRectout->aspRectRD[1], ret);
    #endif
        
    ret = getPtTran(pRectout->aspRectRU, rangle, offset, pRectin->aspRectRU);
    #if LOG_GETRECTRAN_EN
    printf("[CHR] RU: (%4.2lf, %4.2lf) ->  (%4.2lf, %4.2lf) ret: %d\n", 
        pRectin->aspRectRU[0], pRectin->aspRectRU[1], pRectout->aspRectRU[0], pRectout->aspRectRU[1], ret);
    #endif

    return 0;
}

#define LOG_ROTORI_DBG  (0)
static int findRectOrient(struct aspRectObj *pRout, struct aspRectObj *pRin)
{
    CFLOAT minH=0, minV=0, offsetH=0, offsetV=0;
    CFLOAT LUn[2], RUn[2], LDn[2], RDn[2];
    CFLOAT pLU[2], pRU[2], pLD[2], pRD[2];
    
    int LUt[2], RUt[2], LDt[2], RDt[2];
    int maxhint=0, maxvint=0, minhint=0, minvint=0, rowsize=0, rawszNew=0;
    
    if (!pRin) {
        return -1;
    }

    if (!pRout) {
        return -2;
    }

    memcpy(LUn, pRin->aspRectLU, sizeof(CFLOAT) * 2);
    memcpy(RUn, pRin->aspRectRU, sizeof(CFLOAT) * 2);
    memcpy(LDn, pRin->aspRectLD, sizeof(CFLOAT) * 2);
    memcpy(RDn, pRin->aspRectRD, sizeof(CFLOAT) * 2);
    
    minH = aspMin(LUn[0], RUn[0]);
    minH = aspMin(minH, LDn[0]);
    minH = aspMin(minH, RDn[0]);

    minV = aspMin(LUn[1], RUn[1]);
    minV = aspMin(minV, LDn[1]);
    minV = aspMin(minV, RDn[1]);

    offsetH = 0 - minH;
    offsetV = 0 - minV;

    #if 0
    LUn[0] += offsetH;
    LUn[1] += offsetV;

    RUn[0] += offsetH;
    RUn[1] += offsetV;

    LDn[0] += offsetH;
    LDn[1] += offsetV;

    RDn[0] += offsetH;
    RDn[1] += offsetV;
    #endif

    LUt[0] = (int)round(LUn[0]);
    LUt[1] = (int)round(LUn[1]);

    RUt[0] = (int)round(RUn[0]);
    RUt[1] = (int)round(RUn[1]);

    LDt[0] = (int)round(LDn[0]);
    LDt[1] = (int)round(LDn[1]);

    RDt[0] = (int)round(RDn[0]);
    RDt[1] = (int)round(RDn[1]);
    
    #if LOG_ROTORI_DBG
    printf("[ORT] bound: LUn: %lf, %lf -> %d, %d\n", LUn[0], LUn[1], LUt[0], LUt[1]);
    printf("[ORT] bound: RUn: %lf, %lf -> %d, %d \n", RUn[0], RUn[1], RUt[0], RUt[1]);
    printf("[ORT] bound: LDn: %lf, %lf -> %d, %d \n", LDn[0], LDn[1], LDt[0], LDt[1]);
    printf("[ORT] bound: RDn: %lf, %lf -> %d, %d \n", RDn[0], RDn[1], RDt[0], RDt[1]);
    #endif
    
    maxhint= aspMaxInt(LUt[0], RUt[0]);
    maxhint = aspMaxInt(maxhint, LDt[0]);
    maxhint = aspMaxInt(maxhint, RDt[0]);

    maxvint = aspMaxInt(LUt[1], RUt[1]);
    maxvint = aspMaxInt(maxvint, LDt[1]);
    maxvint = aspMaxInt(maxvint, RDt[1]);

    minhint= aspMinInt(LUt[0], RUt[0]);
    minhint = aspMinInt(minhint, LDt[0]);
    minhint = aspMinInt(minhint, RDt[0]);

    minvint = aspMinInt(LUt[1], RUt[1]);
    minvint = aspMinInt(minvint, LDt[1]);
    minvint = aspMinInt(minvint, RDt[1]);

    #if LOG_ROTORI_DBG    
    printf("[ORT] maxh: %d, minh: %d, maxv: %d, minv: %d \n", maxhint, minhint, maxvint, minvint);
    #endif

    pLU[0] = -1;
    pLU[1] = -1;
    pRU[0] = -1;
    pRU[1] = -1;
    pLD[0] = -1;
    pLD[1] = -1;
    pRD[0] = -1;
    pRD[1] = -1;

    if (minhint == LUt[0]) {
    
        #if LOG_ROTORI_DBG    
        printf("[ORT] LU =  %d, %d match minhint: %d !!!left - 0\n", LUt[0], LUt[1], minhint);
        #endif

        if (minvint == LUt[1]) {

            #if LOG_ROTORI_DBG    
            printf("[ORT] LU =  %d, %d match minvint: %d !!!left - 0\n", LUt[0], LUt[1], minvint);
            #endif
        
            pLD[0] = LUn[0];
            pLD[1] = LUn[1];

            #if LOG_ROTORI_DBG    
            printf("[ORT] set PLD = %lf, %lf\n", pLD[0], pLD[1]);
            #endif

        } else {
            if (maxvint == LUt[1]) {

                #if LOG_ROTORI_DBG    
                printf("[ORT] LU =  %d, %d match maxvint: %d !!!left - 0\n", LUt[0], LUt[1], maxvint);
                #endif

                pLU[0] = LUn[0];
                pLU[1] = LUn[1];

                #if LOG_ROTORI_DBG    
                printf("[ORT] set PLU = %lf, %lf\n", pLU[0], pLU[1]);
                #endif

            }
        }

        if ((maxvint == RUt[1]) || (maxvint == LUt[1]) || (minvint == LDt[1]) || (minvint == RDt[1])) {
                if (RUt[0] >= LDt[0]) {
                    pLU[0] = LUn[0];
                    pLU[1] = LUn[1];

                    pLD[0] = LDn[0];
                    pLD[1] = LDn[1];
                } else if (RUt[0] < LDt[0]) {
                    pLU[0] = RUn[0];
                    pLU[1] = RUn[1];

                    pLD[0] = LUn[0];
                    pLD[1] = LUn[1];
                } else {
                    printf("[ORT] WARNING!! LU =  %d, %d not match!!!left - 1\n", LUt[0], LUt[1]);
                }
        }

        
    }
    
    if (minhint == RUt[0]) {

        #if LOG_ROTORI_DBG    
        printf("[ORT] RU =  %d, %d match minhint: %d !!!left - 0\n", RUt[0], RUt[1], minhint);
        #endif

        if (minvint == RUt[1]) {

            #if LOG_ROTORI_DBG    
            printf("[ORT] RU =  %d, %d match minvint: %d !!!left - 0\n", RUt[0], RUt[1], minvint);
            #endif
            
            pLD[0] = RUn[0];
            pLD[1] = RUn[1];

            #if LOG_ROTORI_DBG    
            printf("[ORT] set PLD = %lf, %lf\n", pLD[0], pLD[1]);
            #endif
        } else {
            if (maxvint == RUt[1]) {

                #if LOG_ROTORI_DBG    
                printf("[ORT] RU =  %d, %d match maxvint: %d !!!left - 0\n", RUt[0], RUt[1], maxvint);
                #endif
                
                pLU[0] = RUn[0];
                pLU[1] = RUn[1];

                #if LOG_ROTORI_DBG    
                printf("[ORT] set PLU = %lf, %lf\n", pLU[0], pLU[1]);
                #endif
            }
        }

        if ((maxvint == RDt[1]) || (maxvint == RUt[1]) || (minvint == LUt[1]) || (minvint == LDt[1])) {
                if (RDt[0] >= LUt[0]) {
                    pLU[0] = RUn[0];
                    pLU[1] = RUn[1];
        
                    pLD[0] = LUn[0];
                    pLD[1] = LUn[1];
                } else if (RDt[0] < LUt[0]) {
                    pLU[0] = RDn[0];
                    pLU[1] = RDn[1];
        
                    pLD[0] = RUn[0];
                    pLD[1] = RUn[1];
                } else {
                    printf("[ORT] WARNING!! RU =  %d, %d not match!!!left - 1\n", RUt[0], RUt[1]);
                }
        }
    }
        
    if (minhint == LDt[0]) {

        #if LOG_ROTORI_DBG    
        printf("[ORT] LD =  %d, %d match minhint: %d !!!left - 0\n", LDt[0], LDt[1], minhint);
        #endif
        
        if (minvint == LDt[1]) {

            #if LOG_ROTORI_DBG    
            printf("[ORT] LD =  %d, %d match minvint: %d !!!left - 0\n", LDt[0], LDt[1], minvint);
            #endif

            pLD[0] = LDn[0];
            pLD[1] = LDn[1];

            #if LOG_ROTORI_DBG    
            printf("[ORT] set PLD = %lf, %lf\n", pLD[0], pLD[1]);
            #endif
        } else {
            if (maxvint == LDt[1]) {
                #if LOG_ROTORI_DBG    
                printf("[ORT] LD =  %d, %d match maxvint: %d !!!left - 0\n", LDt[0], LDt[1], maxvint);
                #endif

                pLU[0] = LDn[0];
                pLU[1] = LDn[1];

                #if LOG_ROTORI_DBG    
                printf("[ORT] set PLU = %lf, %lf\n", pLU[0], pLU[1]);
                #endif

            }
        }

        if ((maxvint == LUt[1]) || (maxvint == LDt[1]) || (minvint == RDt[1]) || (minvint == RUt[1])) {
                if (LUt[0] >= RDt[0]) {
                    pLU[0] = LDn[0];
                    pLU[1] = LDn[1];
        
                    pLD[0] = RDn[0];
                    pLD[1] = RDn[1];
                } else if (LUt[0] < RDt[0]) {
                    pLU[0] = LUn[0];
                    pLU[1] = LUn[1];
        
                    pLD[0] = LDn[0];
                    pLD[1] = LDn[1];
                } else {
                    printf("[ORT] WARNING!! LD =  %d, %d not match!!!left - 1\n", LDt[0], LDt[1]);
                }
        }
    }
        
    if (minhint == RDt[0]) {

        #if LOG_ROTORI_DBG    
        printf("[ORT] RD =  %d, %d match minhint: %d !!!left - 0\n", RDt[0], RDt[1], minhint);
        #endif

        if (minvint == RDt[1]) {

            #if LOG_ROTORI_DBG    
            printf("[ORT] RD =  %d, %d match minvint: %d !!!left - 0\n", RDt[0], RDt[1], minvint);
            #endif

            pLD[0] = RDn[0];
            pLD[1] = RDn[1];                    

            #if LOG_ROTORI_DBG    
            printf("[ORT] set PLD = %lf, %lf\n", pLD[0], pLD[1]);
            #endif
        } else {
            if (maxvint == RDt[1]) {

                #if LOG_ROTORI_DBG    
                printf("[ORT] RD =  %d, %d match maxvint: %d !!!left - 0\n", RDt[0], RDt[1], maxvint);
                #endif

                pLU[0] = RDn[0];
                pLU[1] = RDn[1];

                #if LOG_ROTORI_DBG    
                printf("[ORT] set PLU = %lf, %lf\n", pLU[0], pLU[1]);
                #endif
            }
        }

        if ((maxvint == LDt[1]) || (maxvint == RDt[1]) || (minvint == RUt[1]) || (minvint == LUt[1])) {
                if (LDt[0] >= RUt[0]) {
                    pLU[0] = RDn[0];
                    pLU[1] = RDn[1];
        
                    pLD[0] = RUn[0];
                    pLD[1] = RUn[1];
                } else if (LDt[0] < RUt[0]) {
                    pLU[0] = LDn[0];
                    pLU[1] = LDn[1];
        
                    pLD[0] = RDn[0];
                    pLD[1] = RDn[1];
                } else {
                    printf("[ORT] WARNING!! RD =  %d, %d not match!!!left - 1\n", RDt[0], RDt[1]);
                }
        }
    }

    if (maxhint == LUt[0]) {

        #if LOG_ROTORI_DBG    
        printf("[ORT] LU =  %d, %d match maxhint: %d !!!right - 0\n", LUt[0], LUt[1], maxhint);
        #endif

        if (minvint == LUt[1]) {
        
            #if LOG_ROTORI_DBG    
            printf("[ORT] LU =  %d, %d match minvint: %d !!!right - 0\n", LUt[0], LUt[1], minvint);
            #endif

            pRD[0] = LUn[0];
            pRD[1] = LUn[1];

            #if LOG_ROTORI_DBG    
            printf("[ORT] set PLD = %lf, %lf\n", pRD[0], pRD[1]);
            #endif

        } else {
            if (maxvint == LUt[1]) {

                #if LOG_ROTORI_DBG    
                printf("[ORT] LU =  %d, %d match maxvint: %d !!!right - 0\n", LUt[0], LUt[1], maxvint);
                #endif

                pRU[0] = LUn[0];
                pRU[1] = LUn[1];

                #if LOG_ROTORI_DBG    
                printf("[ORT] set PRU = %lf, %lf\n", pRU[0], pRU[1]);
                #endif

            }
        }
        
        if ((maxvint == LDt[1]) || (maxvint == LUt[1]) || (minvint == RUt[1]) || (minvint == RDt[1])) {
                if (RUt[0] <= LDt[0]) {
                    pRU[0] = LDn[0];
                    pRU[1] = LDn[1];

                    pRD[0] = LUn[0];
                    pRD[1] = LUn[1];
                } else if (RUt[0] > LDt[0]) {
                    pRU[0] = LUn[0];
                    pRU[1] = LUn[1];

                    pRD[0] = RUn[0];
                    pRD[1] = RUn[1];
                } else {
                    printf("[ORT] WARNING!! LU =  %d, %d not match!!!right - 1\n", LUt[0], LUt[1]);
                }
        }
    }
    
    if (maxhint == RUt[0]) {
    
        #if LOG_ROTORI_DBG    
        printf("[ORT] RU =  %d, %d match maxhint: %d !!!right - 0\n", RUt[0], RUt[1], maxhint);
        #endif

        if (minvint == RUt[1]) {

            #if LOG_ROTORI_DBG    
            printf("[ORT] RU =  %d, %d match minvint: %d !!!right - 0\n", RUt[0], RUt[1], minvint);
            #endif

            pRD[0] = RUn[0];
            pRD[1] = RUn[1];

            #if LOG_ROTORI_DBG    
            printf("[ORT] set PLD = %lf, %lf\n", pRD[0], pRD[1]);
            #endif

        } else {
            if (maxvint == RUt[1]) {

                #if LOG_ROTORI_DBG    
                printf("[ORT] RU =  %d, %d match maxvint: %d !!!right - 0\n", RUt[0], RUt[1], maxvint);
                #endif

                pRU[0] = RUn[0];
                pRU[1] = RUn[1];

                #if LOG_ROTORI_DBG    
                printf("[ORT] set PRU = %lf, %lf\n", pRU[0], pRU[1]);
                #endif

            }
        }

        if ((maxvint == LUt[1]) || (maxvint == RUt[1]) || (minvint == RDt[1]) || (minvint == LDt[1])) {
                if (RDt[0] <= LUt[0]) {
                    pRU[0] = LUn[0];
                    pRU[1] = LUn[1];
        
                    pRD[0] = RUn[0];
                    pRD[1] = RUn[1];
                } else if (RDt[0] > LUt[0]) {
                    pRU[0] = RUn[0];
                    pRU[1] = RUn[1];
        
                    pRD[0] = RDn[0];
                    pRD[1] = RDn[1];
                } else {
                    printf("[ORT] WARNING!! RU =  %d, %d not match!!!right - 1\n", RUt[0], RUt[1]);
                }
        }
    }
        
    if (maxhint == LDt[0]) {

        #if LOG_ROTORI_DBG    
        printf("[ORT] LD =  %d, %d match maxhint: %d !!!right - 0\n", LDt[0], LDt[1], maxhint);
        #endif

        if (minvint == LDt[1]) {

            #if LOG_ROTORI_DBG    
            printf("[ORT] LD =  %d, %d match minvint: %d !!!right - 0\n", LDt[0], LDt[1], minvint);
            #endif

            pRD[0] = LDn[0];
            pRD[1] = LDn[1];                  

            #if LOG_ROTORI_DBG    
            printf("[ORT] set PLD = %lf, %lf\n", pRD[0], pRD[1]);
            #endif

        } else {
            if (maxvint == LDt[1]) {

                #if LOG_ROTORI_DBG    
                printf("[ORT] LD =  %d, %d match maxvint: %d !!!right - 0\n", LDt[0], LDt[1], maxvint);
                #endif

                pRU[0] = LDn[0];
                pRU[1] = LDn[1];

                #if LOG_ROTORI_DBG    
                printf("[ORT] set PRU = %lf, %lf\n", pRU[0], pRU[1]);
                #endif

            }
        }
        
        if ((maxvint == RDt[1]) || (maxvint == LDt[1]) || (minvint == LUt[1]) || (minvint == RUt[1])) {
                if (LUt[0] <= RDt[0]) {
                    pRU[0] = RDn[0];
                    pRU[1] = RDn[1];
        
                    pRD[0] = LDn[0];
                    pRD[1] = LDn[1];
                } else if (LUt[0] > RDt[0]) {
                    pRU[0] = LDn[0];
                    pRU[1] = LDn[1];
        
                    pRD[0] = LUn[0];
                    pRD[1] = LUn[1];
                } else {
                    printf("[ORT] WARNING!! LD =  %d, %d not match!!!right - 1\n", LDt[0], LDt[1]);
                }
        }
    }
            
    if (maxhint == RDt[0]) {

        #if LOG_ROTORI_DBG    
        printf("[ORT] RD =  %d, %d match maxhint: %d !!!right - 0\n", RDt[0], RDt[1], maxhint);
        #endif

        if (minvint == RDt[1]) {

            #if LOG_ROTORI_DBG    
            printf("[ORT] RD =  %d, %d match minvint: %d !!!right - 0\n", RDt[0], RDt[1], minvint);
            #endif

            pRD[0] = RDn[0];
            pRD[1] = RDn[1];                    

            #if LOG_ROTORI_DBG    
            printf("[ORT] set PLD = %lf, %lf\n", pRD[0], pRD[1]);
            #endif

        } else {
            if (maxvint == RDt[1]) {

                #if LOG_ROTORI_DBG    
                printf("[ORT] RD =  %d, %d match maxvint: %d !!!right - 0\n", RDt[0], RDt[1], maxvint);
                #endif

                pRU[0] = RDn[0];
                pRU[1] = RDn[1];

                #if LOG_ROTORI_DBG    
                printf("[ORT] set PRU = %lf, %lf\n", pRU[0], pRU[1]);
                #endif

            }
        }
        
        if ((maxvint == RUt[1]) || (maxvint == RDt[1]) || (minvint == LDt[1]) || (minvint == LUt[1])) {
                if (LDt[0] <= RUt[0]) {
                    pRU[0] = RUn[0];
                    pRU[1] = RUn[1];
        
                    pRD[0] = RDn[0];
                    pRD[1] = RDn[1];
                } else if (LDt[0] > RUt[0]) {
                    pRU[0] = RDn[0];
                    pRU[1] = RDn[1];
        
                    pRD[0] = LDn[0];
                    pRD[1] = LDn[1];
                } else {
                    printf("[ORT] WARNING!! RD =  %d, %d not match!!!right - 1\n", RDt[0], RDt[1]);
                }
        }
    }

    memcpy(pRout->aspRectLU, pLU, sizeof(CFLOAT) * 2);
    memcpy(pRout->aspRectRU, pRU, sizeof(CFLOAT) * 2);
    memcpy(pRout->aspRectLD, pLD, sizeof(CFLOAT) * 2);
    memcpy(pRout->aspRectRD, pRD, sizeof(CFLOAT) * 2);
    
    return 0;
}


#define LOG_RECTOFFSET_TP_EN (0)
static CFLOAT getRectOffsetTP(struct aspRectObj *pRectout, struct aspRectObj *pRectin, struct aspRectObj *pRectorg, CFLOAT *offset)
{
    CFLOAT offsetH, offsetV;
    CFLOAT *pLU, *pLD, *pRD, *pRU;
    CFLOAT *LUn, *RUn, *LDn, *RDn;
    CFLOAT divH=0, divV=0, diff=0;
    CFLOAT minH, minV;

    pLU = pRectout->aspRectLU;
    pRU = pRectout->aspRectRU;
    pLD = pRectout->aspRectLD;
    pRD = pRectout->aspRectRD;
    
    LUn = pRectin->aspRectLU;
    RUn = pRectin->aspRectRU;
    LDn = pRectin->aspRectLD;
    RDn = pRectin->aspRectRD;

    offsetH = LUn[0] - pRectorg->aspRectLU[0];
    offsetV = LUn[1] - pRectorg->aspRectLU[1];

    #if LOG_RECTOFFSET_TP_EN
    printf("[offsetTp] select LU(%4.2lf, %4.2lf) LD(%4.2lf, %4.2lf) RD(%4.2lf, %4.2lf) RU(%4.2lf, %4.2lf) offH: %.2lf offV: %.2lf \n", 
        pLU[0], pLU[1], pLD[0], pLD[1], pRD[0], pRD[1], pRU[0], pRU[1], offsetH, offsetV);
    #endif

    pLU[0] = LUn[0] - offsetH;
    pLU[1] = LUn[1] - offsetV;

    pLD[0] = LDn[0] - offsetH;
    pLD[1] = LDn[1] - offsetV;

    pRD[0] = RDn[0] - offsetH;
    pRD[1] = RDn[1] - offsetV;

    pRU[0] = RUn[0] - offsetH;
    pRU[1] = RUn[1] - offsetV;

    #if LOG_RECTOFFSET_TP_EN
    printf("[offsetTp] simulate LU(%4.2lf, %4.2lf) LD(%4.2lf, %4.2lf) RD(%4.2lf, %4.2lf) RU(%4.2lf, %4.2lf) offH: %.2lf offV: %.2lf \n", 
        pLU[0], pLU[1], pLD[0], pLD[1], pRD[0], pRD[1], pRU[0], pRU[1], offsetH, offsetV);
    #endif

    diff = fabs(pLU[0] - pRectorg->aspRectLU[0]);
    divH += diff;

    diff = fabs(pLD[0] - pRectorg->aspRectLD[0]);
    divH += diff;

    diff = fabs(pRD[0] - pRectorg->aspRectRD[0]);
    divH += diff;

    diff = fabs(pRU[0] - pRectorg->aspRectRU[0]);
    divH += diff;

    diff = fabs(pLU[1] - pRectorg->aspRectLU[1]);
    divV += diff;

    diff = fabs(pLD[1] - pRectorg->aspRectLD[1]);
    divV += diff;

    diff = fabs(pRD[1] - pRectorg->aspRectRD[1]);
    divV += diff;

    diff = fabs(pRU[1] - pRectorg->aspRectRU[1]);
    divV += diff;

    diff = (divH + divV) / 2.0;

    offset[0] = offsetH;
    offset[1] = offsetV;
    
    return diff;
}

#define LOG_RECTOFFSET_DN_EN (0)
static CFLOAT getRectOffsetDn(struct aspRectObj *pRectout, struct aspRectObj *pRectin, struct aspRectObj *pRectorg, CFLOAT *offset)
{
    CFLOAT offsetH, offsetV;
    CFLOAT *pLU, *pLD, *pRD, *pRU;
    CFLOAT *LUn, *RUn, *LDn, *RDn;
    CFLOAT divH=0, divV=0, diff=0;
    CFLOAT minH, minV;

    pLU = pRectout->aspRectLU;
    pRU = pRectout->aspRectRU;
    pLD = pRectout->aspRectLD;
    pRD = pRectout->aspRectRD;
    
    LUn = pRectin->aspRectLU;
    RUn = pRectin->aspRectRU;
    LDn = pRectin->aspRectLD;
    RDn = pRectin->aspRectRD;
    
    offsetH = LDn[0] - pRectorg->aspRectLD[0];
    offsetV = LDn[1] - pRectorg->aspRectLD[1];

    pLU[0] = LUn[0] - offsetH;
    pLU[1] = LUn[1] - offsetV;

    pLD[0] = LDn[0] - offsetH;
    pLD[1] = LDn[1] - offsetV;

    pRD[0] = RDn[0] - offsetH;
    pRD[1] = RDn[1] - offsetV;

    pRU[0] = RUn[0] - offsetH;
    pRU[1] = RUn[1] - offsetV;

    #if LOG_RECTOFFSET_DN_EN
    printf("[offsetDn] simulate LU(%4.2lf, %4.2lf) LD(%4.2lf, %4.2lf) RD(%4.2lf, %4.2lf) RU(%4.2lf, %4.2lf) offH: %.2lf offV: %.2lf \n", 
        pLU[0], pLU[1], pLD[0], pLD[1], pRD[0], pRD[1], pRU[0], pRU[1], offsetH, offsetV);
    #endif

    diff = fabs(pLU[0] - pRectorg->aspRectLU[0]);
    divH += diff;

    diff = fabs(pLD[0] - pRectorg->aspRectLD[0]);
    divH += diff;

    diff = fabs(pRD[0] - pRectorg->aspRectRD[0]);
    divH += diff;

    diff = fabs(pRU[0] - pRectorg->aspRectRU[0]);
    divH += diff;

    diff = fabs(pLU[1] - pRectorg->aspRectLU[1]);
    divV += diff;

    diff = fabs(pLD[1] - pRectorg->aspRectLD[1]);
    divV += diff;

    diff = fabs(pRD[1] - pRectorg->aspRectRD[1]);
    divV += diff;

    diff = fabs(pRU[1] - pRectorg->aspRectRU[1]);
    divV += diff;

    diff = (divH + divV) / 2.0;

    offset[0] = offsetH;
    offset[1] = offsetV;
    
    return diff;
}

#define LOG_RECTOFFSET_LF_EN (0)
static CFLOAT getRectOffsetLf(struct aspRectObj *pRectout, struct aspRectObj *pRectin, struct aspRectObj *pRectorg, CFLOAT *offset)
{
    CFLOAT offsetH, offsetV;
    CFLOAT *pLU, *pLD, *pRD, *pRU;
    CFLOAT *LUn, *RUn, *LDn, *RDn;
    CFLOAT divH=0, divV=0, diff=0;
    CFLOAT minH, minV;

    pLU = pRectout->aspRectLU;
    pRU = pRectout->aspRectRU;
    pLD = pRectout->aspRectLD;
    pRD = pRectout->aspRectRD;
    
    LUn = pRectin->aspRectLU;
    RUn = pRectin->aspRectRU;
    LDn = pRectin->aspRectLD;
    RDn = pRectin->aspRectRD;
    
    offsetH = LUn[0] - pRectorg->aspRectLU[0];
    offsetV = LUn[1] - pRectorg->aspRectLU[1];

    pLU[0] = LUn[0] - offsetH;
    pLU[1] = LUn[1] - offsetV;

    pLD[0] = LDn[0] - offsetH;
    pLD[1] = LDn[1] - offsetV;

    pRD[0] = RDn[0] - offsetH;
    pRD[1] = RDn[1] - offsetV;

    pRU[0] = RUn[0] - offsetH;
    pRU[1] = RUn[1] - offsetV;

    #if LOG_RECTOFFSET_LF_EN
    printf("[offsetLf] simulate LU(%4.2lf, %4.2lf) LD(%4.2lf, %4.2lf) RD(%4.2lf, %4.2lf) RU(%4.2lf, %4.2lf) offH: %.2lf offV: %.2lf \n", 
        pLU[0], pLU[1], pLD[0], pLD[1], pRD[0], pRD[1], pRU[0], pRU[1], offsetH, offsetV);
    #endif

    diff = fabs(pLU[0] - pRectorg->aspRectLU[0]);
    divH += diff;

    diff = fabs(pLD[0] - pRectorg->aspRectLD[0]);
    divH += diff;

    diff = fabs(pRD[0] - pRectorg->aspRectRD[0]);
    divH += diff;

    diff = fabs(pRU[0] - pRectorg->aspRectRU[0]);
    divH += diff;

    diff = fabs(pLU[1] - pRectorg->aspRectLU[1]);
    divV += diff;

    diff = fabs(pLD[1] - pRectorg->aspRectLD[1]);
    divV += diff;

    diff = fabs(pRD[1] - pRectorg->aspRectRD[1]);
    divV += diff;

    diff = fabs(pRU[1] - pRectorg->aspRectRU[1]);
    divV += diff;

    diff = (divH + divV) / 2.0;

    offset[0] = offsetH;
    offset[1] = offsetV;
    
    return diff;
}

#define LOG_RECTOFFSET_RT_EN (0)
static CFLOAT getRectOffsetRt(struct aspRectObj *pRectout, struct aspRectObj *pRectin, struct aspRectObj *pRectorg, CFLOAT *offset)
{
    CFLOAT offsetH, offsetV;
    CFLOAT *pLU, *pLD, *pRD, *pRU;
    CFLOAT *LUn, *RUn, *LDn, *RDn;
    CFLOAT divH=0, divV=0, diff=0;
    CFLOAT minH, minV;

    pLU = pRectout->aspRectLU;
    pRU = pRectout->aspRectRU;
    pLD = pRectout->aspRectLD;
    pRD = pRectout->aspRectRD;
    
    LUn = pRectin->aspRectLU;
    RUn = pRectin->aspRectRU;
    LDn = pRectin->aspRectLD;
    RDn = pRectin->aspRectRD;
    
    offsetH = RUn[0] - pRectorg->aspRectRU[0];
    offsetV = RUn[1] - pRectorg->aspRectRU[1];

    pLU[0] = LUn[0] - offsetH;
    pLU[1] = LUn[1] - offsetV;

    pLD[0] = LDn[0] - offsetH;
    pLD[1] = LDn[1] - offsetV;

    pRD[0] = RDn[0] - offsetH;
    pRD[1] = RDn[1] - offsetV;

    pRU[0] = RUn[0] - offsetH;
    pRU[1] = RUn[1] - offsetV;

    #if LOG_RECTOFFSET_RT_EN
    printf("[offsetRt] simulate LU(%4.2lf, %4.2lf) LD(%4.2lf, %4.2lf) RD(%4.2lf, %4.2lf) RU(%4.2lf, %4.2lf) offH: %.2lf offV: %.2lf \n", 
        pLU[0], pLU[1], pLD[0], pLD[1], pRD[0], pRD[1], pRU[0], pRU[1], offsetH, offsetV);
    #endif

    diff = fabs(pLU[0] - pRectorg->aspRectLU[0]);
    divH += diff;

    diff = fabs(pLD[0] - pRectorg->aspRectLD[0]);
    divH += diff;

    diff = fabs(pRD[0] - pRectorg->aspRectRD[0]);
    divH += diff;

    diff = fabs(pRU[0] - pRectorg->aspRectRU[0]);
    divH += diff;

    diff = fabs(pLU[1] - pRectorg->aspRectLU[1]);
    divV += diff;

    diff = fabs(pLD[1] - pRectorg->aspRectLD[1]);
    divV += diff;

    diff = fabs(pRD[1] - pRectorg->aspRectRD[1]);
    divV += diff;

    diff = fabs(pRU[1] - pRectorg->aspRectRU[1]);
    divV += diff;

    diff = (divH + divV) / 2.0;

    offset[0] = offsetH;
    offset[1] = offsetV;
    
    return diff;
}

#define LOG_RECTALIGN_RT_EN (1)
static CFLOAT getRectAlignRT(struct aspRectObj *pRectin, CFLOAT *p1, CFLOAT *p2, struct aspRectObj *pRectout)
{
    int fArea=-1, ret=0;
    CFLOAT dh=0, dv=0, dg=-1;
    CFLOAT plf[2]={0};
    CFLOAT piAngle = 180.0, thacos=0, thasin=0, rangle[2], theta=0;
    CFLOAT *pLU, *pLD, *pRU, *pRD;
    CFLOAT *LUn, *RUn, *LDn, *RDn;
    int LUt[2], RUt[2], LDt[2], RDt[2], dht, dvt, p1t[2];

    
    dh = p2[0] - p1[0];
    dv = p2[1] - p1[1];

    if ((dh > 0) && (dh < 0.01)) {
        dh = 0.0;
    }

    if ((dh < 0) && (dh > -0.01)) {
        dh = 0.0;
    }

    if ((dv > 0) && (dv < 0.01)) {
        dv = 0.0;
    }

    if ((dv < 0) && (dv > -0.01)) {
        dv = 0.0;
    }
    
    if ((dh == 0.0) && (dv == 0.0)) {
        return -1;
    }
    
    #if LOG_RECTALIGN_RT_EN
    printf("[RectAlignRT] p1: (%.2lf, %.2lf) p2: (%.2lf, %.2lf) dh: %.2lf dv: %.2lf \n", p1[0], p1[1], p2[0], p2[1], dh, dv);
    #endif

    if (dh == 0.0) {
        if (dv > 0.0) {
            dg = 90.0;
        } else {
            dg = 270.0;
        }
    }
    else if (dv == 0.0) {
        if (dh > 0.0) {
            dg = 0.0;
        } else {
            dg = 180.0;
        }
    } 
    else {
        plf[0] = p1[0];
        plf[1] = p1[1] - 200.0;
        dg = getAngle(p1, plf, p2);

        if ((dh > 0.0) && (dv > 0.0)) {
            dg += 0.0;
            //dg = 360.0 - dg;
        }
        else if ((dh < 0.0) && (dv > 0.0)) {
            //dg += 0.0;
            dg = 360.0 - dg;
        }
        else if ((dh < 0.0) && (dv < 0.0)) {
            //dg += 0.0;
            dg = 360.0 - dg;
        }
        else if ((dh > 0.0) && (dv < 0.0)) {
            dg += 0.0;
            //dg = 360.0 - dg;
        }
    }

    #if LOG_RECTALIGN_RT_EN
    printf("[RectAlignRT] p1: (%.2lf, %.2lf) p2: (%.2lf, %.2lf) plf: (%.2lf, %.2lf) angle: %.2lf \n", p1[0], p1[1], p2[0], p2[1], plf[0], plf[1], dg);
    #endif

    theta = 360.0 - dg;

    theta = theta * M_PI / piAngle;

    thacos = cos(theta);
    thasin = sin(theta);
    
    rangle[0] = thacos;
    rangle[1] = thasin;

    pLU = pRectin->aspRectLU;
    pRU = pRectin->aspRectRU;
    pLD = pRectin->aspRectLD;
    pRD = pRectin->aspRectRD;

    LUn = pRectout->aspRectLU;
    RUn = pRectout->aspRectRU;
    LDn = pRectout->aspRectLD;
    RDn = pRectout->aspRectRD;
    
    ret = calcuRotateCoordinates(LUt, LUn, pLU, rangle);
    #if LOG_RECTALIGN_RT_EN
    printf("[RectAlignRT] pLU: (%4.2lf, %4.2lf) -> (%4.2lf, %4.2lf) ret: %d\n", pLU[0], pLU[1], LUn[0], LUn[1], ret);
    #endif
    
    ret = calcuRotateCoordinates(RUt, RUn, pRU, rangle);
    #if LOG_RECTALIGN_RT_EN
    printf("[RectAlignRT] pRU: (%4.2lf, %4.2lf) -> (%4.2lf, %4.2lf) ret: %d\n", pRU[0], pRU[1], RUn[0], RUn[1], ret);
    #endif
    
    ret = calcuRotateCoordinates(LDt, LDn, pLD, rangle);
    #if LOG_RECTALIGN_RT_EN
    printf("[RectAlignRT] pLD: (%4.2lf, %4.2lf) -> (%4.2lf, %4.2lf) ret: %d\n", pLD[0], pLD[1], LDn[0], LDn[1], ret);
    #endif
    
    ret = calcuRotateCoordinates(RDt, RDn, pRD, rangle);
    #if LOG_RECTALIGN_RT_EN
    printf("[RectAlignRT] pRD: (%4.2lf, %4.2lf) -> (%4.2lf, %4.2lf) ret: %d\n", pRD[0], pRD[1], RDn[0], RDn[1], ret);
    printf("[RectAlignRT] LU(%d, %d) LD(%d, %d) RD(%d, %d) RU(%d, %d) \n", LUt[0], LUt[1], LDt[0], LDt[1], RDt[0], RDt[1], RUt[0], RUt[1]);
    #endif

    ret = calcuRotateCoordinates(p1t, plf, p1, rangle);
    #if LOG_RECTALIGN_RT_EN
    printf("[RectAlignRT] p1: (%4.2lf, %4.2lf) -> (%4.2lf, %4.2lf) ret: %d\n", p1[0], p1[1], plf[0], plf[1], ret);
    printf("[RectAlignRT] (%d, %d) (%d, %d) (%d, %d) (%d, %d) \n", LUt[0], LUt[1], LDt[0], LDt[1], RDt[0], RDt[1], RUt[0], RUt[1]);
    #endif
    
    return dg;
}

#define LOG_RECTALIGN_LF_EN (1)
static CFLOAT getRectAlignLF(struct aspRectObj *pRectin, CFLOAT *p1, CFLOAT *p2, struct aspRectObj *pRectout)
{
    int fArea=-1, ret=0;
    CFLOAT dh=0, dv=0, dg=-1;
    CFLOAT plf[2]={0};
    CFLOAT piAngle = 180.0, thacos=0, thasin=0, rangle[2], theta=0;
    CFLOAT *pLU, *pLD, *pRU, *pRD;
    CFLOAT *LUn, *RUn, *LDn, *RDn;
    int LUt[2], RUt[2], LDt[2], RDt[2], dht, dvt, p1t[2];

    
    dh = p2[0] - p1[0];
    dv = p2[1] - p1[1];

    if ((dh > 0) && (dh < 0.01)) {
        dh = 0.0;
    }

    if ((dh < 0) && (dh > -0.01)) {
        dh = 0.0;
    }

    if ((dv > 0) && (dv < 0.01)) {
        dv = 0.0;
    }

    if ((dv < 0) && (dv > -0.01)) {
        dv = 0.0;
    }
    
    if ((dh == 0.0) && (dv == 0.0)) {
        return -1;
    }
    
    #if LOG_RECTALIGN_LF_EN
    printf("[RectAlignLF] p1: (%.2lf, %.2lf) p2: (%.2lf, %.2lf) dh: %.2lf dv: %.2lf \n", p1[0], p1[1], p2[0], p2[1], dh, dv);
    #endif

    if (dh == 0.0) {
        if (dv > 0.0) {
            dg = 90.0;
        } else {
            dg = 270.0;
        }
    }
    else if (dv == 0.0) {
        if (dh > 0.0) {
            dg = 0.0;
        } else {
            dg = 180.0;
        }
    } 
    else {
        plf[0] = p1[0];
        plf[1] = p1[1] + 200.0;
        dg = getAngle(p1, plf, p2);

        if ((dh > 0.0) && (dv > 0.0)) {
            dg = 360.0 - dg;
            //dg += 0.0;
        }
        else if ((dh < 0.0) && (dv > 0.0)) {
            //dg = 360.0 - dg;
            dg += 0.0;
        }
        else if ((dh < 0.0) && (dv < 0.0)) {
            //dg = 360.0 - dg;
            dg += 0.0;
        }
        else if ((dh > 0.0) && (dv < 0.0)) {
            dg = 360.0 - dg;
            //dg += 0.0;
        }
    }

    #if LOG_RECTALIGN_LF_EN
    printf("[RectAlignLF] p1: (%.2lf, %.2lf) p2: (%.2lf, %.2lf) plf: (%.2lf, %.2lf) angle: %.2lf \n", p1[0], p1[1], p2[0], p2[1], plf[0], plf[1], dg);
    #endif

    theta = 360.0 - dg;

    theta = theta * M_PI / piAngle;

    thacos = cos(theta);
    thasin = sin(theta);
    
    rangle[0] = thacos;
    rangle[1] = thasin;

    pLU = pRectin->aspRectLU;
    pRU = pRectin->aspRectRU;
    pLD = pRectin->aspRectLD;
    pRD = pRectin->aspRectRD;

    LUn = pRectout->aspRectLU;
    RUn = pRectout->aspRectRU;
    LDn = pRectout->aspRectLD;
    RDn = pRectout->aspRectRD;
    
    ret = calcuRotateCoordinates(LUt, LUn, pLU, rangle);
    #if LOG_RECTALIGN_LF_EN
    printf("[RectAlignLF] pLU: (%4.2lf, %4.2lf) -> (%4.2lf, %4.2lf) ret: %d\n", pLU[0], pLU[1], LUn[0], LUn[1], ret);
    #endif
    
    ret = calcuRotateCoordinates(RUt, RUn, pRU, rangle);
    #if LOG_RECTALIGN_LF_EN
    printf("[RectAlignLF] pRU: (%4.2lf, %4.2lf) -> (%4.2lf, %4.2lf) ret: %d\n", pRU[0], pRU[1], RUn[0], RUn[1], ret);
    #endif
    
    ret = calcuRotateCoordinates(LDt, LDn, pLD, rangle);
    #if LOG_RECTALIGN_LF_EN
    printf("[RectAlignLF] pLD: (%4.2lf, %4.2lf) -> (%4.2lf, %4.2lf) ret: %d\n", pLD[0], pLD[1], LDn[0], LDn[1], ret);
    #endif
    
    ret = calcuRotateCoordinates(RDt, RDn, pRD, rangle);
    #if LOG_RECTALIGN_LF_EN
    printf("[RectAlignLF] pRD: (%4.2lf, %4.2lf) -> (%4.2lf, %4.2lf) ret: %d\n", pRD[0], pRD[1], RDn[0], RDn[1], ret);
    printf("[RectAlignLF] LU(%d, %d) LD(%d, %d) RD(%d, %d) RU(%d, %d) \n", LUt[0], LUt[1], LDt[0], LDt[1], RDt[0], RDt[1], RUt[0], RUt[1]);
    #endif

    ret = calcuRotateCoordinates(p1t, plf, p1, rangle);
    #if LOG_RECTALIGN_LF_EN
    printf("[RectAlignLF] p1: (%4.2lf, %4.2lf) -> (%4.2lf, %4.2lf) ret: %d\n", p1[0], p1[1], plf[0], plf[1], ret);
    printf("[RectAlignLF] (%d, %d) (%d, %d) (%d, %d) (%d, %d) \n", LUt[0], LUt[1], LDt[0], LDt[1], RDt[0], RDt[1], RUt[0], RUt[1]);
    #endif
    
    return dg;
}

#define LOG_RECTALIGN_DN_EN (0)
static CFLOAT getRectAlignDN(struct aspRectObj *pRectin, CFLOAT *p1, CFLOAT *p2, struct aspRectObj *pRectout)
{
    int fArea=-1, ret=0;
    CFLOAT dh=0, dv=0, dg=-1;
    CFLOAT plf[2]={0};
    CFLOAT piAngle = 180.0, thacos=0, thasin=0, rangle[2], theta=0;
    CFLOAT *pLU, *pLD, *pRU, *pRD;
    CFLOAT *LUn, *RUn, *LDn, *RDn;
    int LUt[2], RUt[2], LDt[2], RDt[2], dht, dvt, p1t[2];

    
    dh = p2[0] - p1[0];
    dv = p2[1] - p1[1];

    if ((dh > 0) && (dh < 0.01)) {
        dh = 0.0;
    }

    if ((dh < 0) && (dh > -0.01)) {
        dh = 0.0;
    }

    if ((dv > 0) && (dv < 0.01)) {
        dv = 0.0;
    }

    if ((dv < 0) && (dv > -0.01)) {
        dv = 0.0;
    }
    
    if ((dh == 0.0) && (dv == 0.0)) {
        return -1;
    }
    
    #if LOG_RECTALIGN_DN_EN
    printf("[RectAlignDN] p1: (%.2lf, %.2lf) p2: (%.2lf, %.2lf) dh: %.2lf dv: %.2lf \n", p1[0], p1[1], p2[0], p2[1], dh, dv);
    #endif

    if (dh == 0.0) {
        if (dv > 0.0) {
            dg = 90.0;
        } else {
            dg = 270.0;
        }
    }
    else if (dv == 0.0) {
        if (dh > 0.0) {
            dg = 0.0;
        } else {
            dg = 180.0;
        }
    } 
    else {
        plf[0] = p1[0] - 200.0;
        plf[1] = p1[1];
        dg = getAngle(p1, plf, p2);

        if ((dh > 0.0) && (dv > 0.0)) {
            //dg += 0.0;
            dg = 360.0 - dg;
        }
        else if ((dh < 0.0) && (dv > 0.0)) {
            //dg += 0.0;
            dg = 360.0 - dg;
        }
        else if ((dh < 0.0) && (dv < 0.0)) {
            dg += 0.0;
            //dg = 360.0 - dg;
        }
        else if ((dh > 0.0) && (dv < 0.0)) {
            dg += 0.0;
            //dg = 360.0 - dg;
        }
    }

    #if LOG_RECTALIGN_DN_EN
    printf("[RectAlignDN] p1: (%.2lf, %.2lf) p2: (%.2lf, %.2lf) plf: (%.2lf, %.2lf) angle: %.2lf \n", p1[0], p1[1], p2[0], p2[1], plf[0], plf[1], dg);
    #endif

    theta = 360.0 - dg;

    theta = theta * M_PI / piAngle;

    thacos = cos(theta);
    thasin = sin(theta);
    
    rangle[0] = thacos;
    rangle[1] = thasin;

    pLU = pRectin->aspRectLU;
    pRU = pRectin->aspRectRU;
    pLD = pRectin->aspRectLD;
    pRD = pRectin->aspRectRD;

    LUn = pRectout->aspRectLU;
    RUn = pRectout->aspRectRU;
    LDn = pRectout->aspRectLD;
    RDn = pRectout->aspRectRD;
    
    ret = calcuRotateCoordinates(LUt, LUn, pLU, rangle);
    #if LOG_RECTALIGN_DN_EN
    printf("[RectAlignDN] pLU: (%4.2lf, %4.2lf) -> (%4.2lf, %4.2lf) ret: %d\n", pLU[0], pLU[1], LUn[0], LUn[1], ret);
    #endif
    
    ret = calcuRotateCoordinates(RUt, RUn, pRU, rangle);
    #if LOG_RECTALIGN_DN_EN
    printf("[RectAlignDN] pRU: (%4.2lf, %4.2lf) -> (%4.2lf, %4.2lf) ret: %d\n", pRU[0], pRU[1], RUn[0], RUn[1], ret);
    #endif
    
    ret = calcuRotateCoordinates(LDt, LDn, pLD, rangle);
    #if LOG_RECTALIGN_DN_EN
    printf("[RectAlignDN] pLD: (%4.2lf, %4.2lf) -> (%4.2lf, %4.2lf) ret: %d\n", pLD[0], pLD[1], LDn[0], LDn[1], ret);
    #endif
    
    ret = calcuRotateCoordinates(RDt, RDn, pRD, rangle);
    #if LOG_RECTALIGN_DN_EN
    printf("[RectAlignDN] pRD: (%4.2lf, %4.2lf) -> (%4.2lf, %4.2lf) ret: %d\n", pRD[0], pRD[1], RDn[0], RDn[1], ret);
    printf("[RectAlignDN] LU(%d, %d) LD(%d, %d) RD(%d, %d) RU(%d, %d) \n", LUt[0], LUt[1], LDt[0], LDt[1], RDt[0], RDt[1], RUt[0], RUt[1]);
    #endif

    ret = calcuRotateCoordinates(p1t, plf, p1, rangle);
    #if LOG_RECTALIGN_DN_EN
    printf("[RectAlignDN] p1: (%4.2lf, %4.2lf) -> (%4.2lf, %4.2lf) ret: %d\n", p1[0], p1[1], plf[0], plf[1], ret);
    printf("[RectAlignDN] (%d, %d) (%d, %d) (%d, %d) (%d, %d) \n", LUt[0], LUt[1], LDt[0], LDt[1], RDt[0], RDt[1], RUt[0], RUt[1]);
    #endif
    
    return dg;
}

#define LOG_RECTALIGN_TP_EN (0)
static CFLOAT getRectAlignTP(struct aspRectObj *pRectin, CFLOAT *p1, CFLOAT *p2, struct aspRectObj *pRectout)
{
    int fArea=-1, ret=0;
    CFLOAT dh=0, dv=0, dg=-1;
    CFLOAT plf[2]={0};
    CFLOAT piAngle = 180.0, thacos=0, thasin=0, rangle[2], theta=0;
    CFLOAT *pLU, *pLD, *pRU, *pRD;
    CFLOAT *LUn, *RUn, *LDn, *RDn;
    int LUt[2], RUt[2], LDt[2], RDt[2], dht, dvt, p1t[2];

    
    dh = p2[0] - p1[0];
    dv = p2[1] - p1[1];

    if ((dh > 0) && (dh < 0.01)) {
        dh = 0.0;
    }

    if ((dh < 0) && (dh > -0.01)) {
        dh = 0.0;
    }

    if ((dv > 0) && (dv < 0.01)) {
        dv = 0.0;
    }

    if ((dv < 0) && (dv > -0.01)) {
        dv = 0.0;
    }
    
    if ((dh == 0.0) && (dv == 0.0)) {
        return -1;
    }
    
    #if LOG_RECTALIGN_TP_EN
    printf("[RectAlignTP] p1: (%.2lf, %.2lf) p2: (%.2lf, %.2lf) dh: %.2lf dv: %.2lf \n", p1[0], p1[1], p2[0], p2[1], dh, dv);
    #endif

    if (dh == 0.0) {
        if (dv > 0.0) {
            dg = 90.0;
        } else {
            dg = 270.0;
        }
    }
    else if (dv == 0.0) {
        if (dh > 0.0) {
            dg = 0.0;
        } else {
            dg = 180.0;
        }
    } 
    else {
        plf[0] = p1[0] + 200.0;
        plf[1] = p1[1];
        dg = getAngle(p1, plf, p2);

        if ((dh > 0.0) && (dv > 0.0)) {
            dg += 0.0;
            //dg = 360.0 - dg;
        }
        else if ((dh < 0.0) && (dv > 0.0)) {
            dg += 0.0;
            //dg = 360.0 - dg;
        }
        else if ((dh < 0.0) && (dv < 0.0)) {
            //dg += 0.0;
            dg = 360.0 - dg;
        }
        else if ((dh > 0.0) && (dv < 0.0)) {
            //dg += 0.0;
            dg = 360.0 - dg;
        }
    }

    #if LOG_RECTALIGN_TP_EN
    printf("[RectAlignTP] p1: (%.2lf, %.2lf) p2: (%.2lf, %.2lf) plf: (%.2lf, %.2lf) angle: %.2lf \n", p1[0], p1[1], p2[0], p2[1], plf[0], plf[1], dg);
    #endif

    theta = 360.0 - dg;

    theta = theta * M_PI / piAngle;

    thacos = cos(theta);
    thasin = sin(theta);
    
    rangle[0] = thacos;
    rangle[1] = thasin;

    pLU = pRectin->aspRectLU;
    pRU = pRectin->aspRectRU;
    pLD = pRectin->aspRectLD;
    pRD = pRectin->aspRectRD;

    LUn = pRectout->aspRectLU;
    RUn = pRectout->aspRectRU;
    LDn = pRectout->aspRectLD;
    RDn = pRectout->aspRectRD;
    
    ret = calcuRotateCoordinates(LUt, LUn, pLU, rangle);
    #if LOG_RECTALIGN_TP_EN
    printf("[RectAlignTP] pLU: (%4.2lf, %4.2lf) -> (%4.2lf, %4.2lf) ret: %d\n", pLU[0], pLU[1], LUn[0], LUn[1], ret);
    #endif
    
    ret = calcuRotateCoordinates(RUt, RUn, pRU, rangle);
    #if LOG_RECTALIGN_TP_EN
    printf("[RectAlignTP] pRU: (%4.2lf, %4.2lf) -> (%4.2lf, %4.2lf) ret: %d\n", pRU[0], pRU[1], RUn[0], RUn[1], ret);
    #endif
    
    ret = calcuRotateCoordinates(LDt, LDn, pLD, rangle);
    #if LOG_RECTALIGN_TP_EN
    printf("[RectAlignTP] pLD: (%4.2lf, %4.2lf) -> (%4.2lf, %4.2lf) ret: %d\n", pLD[0], pLD[1], LDn[0], LDn[1], ret);
    #endif
    
    ret = calcuRotateCoordinates(RDt, RDn, pRD, rangle);
    #if LOG_RECTALIGN_TP_EN
    printf("[RectAlignTP] pRD: (%4.2lf, %4.2lf) -> (%4.2lf, %4.2lf) ret: %d\n", pRD[0], pRD[1], RDn[0], RDn[1], ret);
    printf("[RectAlignTP] LU(%d, %d) LD(%d, %d) RD(%d, %d) RU(%d, %d) \n", LUt[0], LUt[1], LDt[0], LDt[1], RDt[0], RDt[1], RUt[0], RUt[1]);
    #endif

    ret = calcuRotateCoordinates(p1t, plf, p1, rangle);
    #if LOG_RECTALIGN_TP_EN
    printf("[RectAlignTP] p1: (%4.2lf, %4.2lf) -> (%4.2lf, %4.2lf) ret: %d\n", p1[0], p1[1], plf[0], plf[1], ret);
    printf("[RectAlignTP] (%d, %d) (%d, %d) (%d, %d) (%d, %d) \n", LUt[0], LUt[1], LDt[0], LDt[1], RDt[0], RDt[1], RUt[0], RUt[1]);
    #endif
    
    return dg;
}

#define LOG_ROTRECT_MF_EN (0)
static int getRotRectPointMf(int *cropinfo, struct aspRectObj *pRectroi, CFLOAT *pdeg, int oldRowsz, int bpp, struct aspRectObj *pRectin, int pidx) 
{
#define DF_IMG_W (2000)
#define DF_IMG_H (700)
    int ret=0, err=0, bitset=0, dx=0, dy=0, ix=0, ic=0;
    int LUt[2], RUt[2], LDt[2], RDt[2];
    CFLOAT piAngle = 180.0, thacos=0, thasin=0, rangle[2], theta=0;
    CFLOAT *pLU, *pLD, *pRU, *pRD;
    CFLOAT pT1[4], pT2[4], pT5[2];
    CFLOAT *LUn, *RUn, *LDn, *RDn;
    CFLOAT d12, d23, d34, d41;
    CFLOAT v12, v23, v34, v41;
    CFLOAT o12[2], o23[2], o34[2], o41[2];
    CFLOAT vmin, vmin1;
    CFLOAT pfound[2];
    CFLOAT ptStart[4][2], dgs[4], *offsets[4], ptEnd[4][4], correct[2], ptShift[4][2];
    CFLOAT srhnum[4][2]={0}, srhlen[4][2]={0}, srhran[4][2]={0};
    int edwhA[2]={0}, edwhB[2]={0};
    
    int ptreal[2];
    struct aspRectObj *pRectout12=0, *pRectout23=0, *pRectout34=0, *pRectout41=0, *pRectorg=0;
    struct aspRectObj *pRectorgi=0, *pRectorgv=0, *pRectorgc=0, *pRectorgk=0, *pRectorgcOut=0, *pRectorgkOut=0;
    struct aspRectObj *pRectout12R=0, *pRectout23R=0, *pRectout34R=0, *pRectout41R=0;
    struct aspRectObj *pRectout12Ro=0, *pRectout23Ro=0, *pRectout34Ro=0, *pRectout41Ro=0;
    struct aspRectObj *pRectTga=0;
    char *src=0;
    char rgb[4][3];
    char *rgbtga[4];
    char *rgbdiff[4];
    int srhcntA[2]={0}, srhcntB[2]={0};
    CFLOAT srhcntmax[4][2]={0}, fdiff=0.0, vdiff=0.0;
    int srhtotal=0;
    int idxA=0, idxB=0;
    int side=0;
    
    bitset = bpp / 8;
    
    pLU = pRectin->aspRectLU;
    pRU = pRectin->aspRectRU;
    pLD = pRectin->aspRectLD;
    pRD = pRectin->aspRectRD;
    
    pRectout12 = aspMemalloc(sizeof(struct aspRectObj), pidx);
    pRectout23 = aspMemalloc(sizeof(struct aspRectObj), pidx);
    pRectout34 = aspMemalloc(sizeof(struct aspRectObj), pidx);
    pRectout41 = aspMemalloc(sizeof(struct aspRectObj), pidx);
    pRectorg = aspMemalloc(sizeof(struct aspRectObj), pidx);
    pRectorgi = aspMemalloc(sizeof(struct aspRectObj), pidx);
    pRectorgv = aspMemalloc(sizeof(struct aspRectObj), pidx);
    pRectorgc = aspMemalloc(sizeof(struct aspRectObj), pidx);
    pRectorgk = aspMemalloc(sizeof(struct aspRectObj), pidx);
    pRectorgcOut = aspMemalloc(sizeof(struct aspRectObj), pidx);
    pRectorgkOut = aspMemalloc(sizeof(struct aspRectObj), pidx);
    
    pRectout12R = aspMemalloc(sizeof(struct aspRectObj), pidx);
    pRectout23R = aspMemalloc(sizeof(struct aspRectObj), pidx);
    pRectout34R = aspMemalloc(sizeof(struct aspRectObj), pidx);
    pRectout41R = aspMemalloc(sizeof(struct aspRectObj), pidx);

    pRectout12Ro = aspMemalloc(sizeof(struct aspRectObj), pidx);
    pRectout23Ro = aspMemalloc(sizeof(struct aspRectObj), pidx);
    pRectout34Ro = aspMemalloc(sizeof(struct aspRectObj), pidx);
    pRectout41Ro = aspMemalloc(sizeof(struct aspRectObj), pidx);

    pRectTga = aspMemalloc(sizeof(struct aspRectObj), pidx);

    edwhA[0] = DF_IMG_W;
    edwhA[1] = DF_IMG_H;
    pT1[0] = (CFLOAT)cropinfo[0];
    pT1[1] = (CFLOAT)(edwhA[1]  - (cropinfo[1] + cropinfo[3]));
    pT1[2] = (CFLOAT)cropinfo[2];
    pT1[3] = (CFLOAT)cropinfo[3];

    edwhB[0] = DF_IMG_W;
    edwhB[1] = DF_IMG_H;
    pT2[0] = (CFLOAT)cropinfo[0];
    pT2[1] = (CFLOAT)(edwhB[1] - (cropinfo[1] + cropinfo[3]));
    pT2[2] = (CFLOAT)cropinfo[2];
    pT2[3] = (CFLOAT)cropinfo[3];

    #if LOG_ROTRECT_MF_EN
    printf("get A side pos %.2lf, %.2lf, %.2lf, %.2lf w: %4d h: %4d !!! \n", pT1[0], pT1[1], pT1[2], pT1[3], edwhA[0], edwhA[1]);
    printf("get B side pos %.2lf, %.2lf, %.2lf, %.2lf w: %4d h: %4d  !!! \n", pT2[0], pT2[1], pT2[2], pT2[3], edwhB[0], edwhB[1]);
    #endif
    
    pRectorg->aspRectLU[0] = (CFLOAT)1;
    pRectorg->aspRectLU[1] = (CFLOAT)edwhA[1];

    pRectorg->aspRectLD[0] = (CFLOAT)1;
    pRectorg->aspRectLD[1] = (CFLOAT)1;
    
    pRectorg->aspRectRD[0] = (CFLOAT)edwhA[0];
    pRectorg->aspRectRD[1] = (CFLOAT)1;

    pRectorg->aspRectRU[0] = (CFLOAT)edwhA[0];
    pRectorg->aspRectRU[1] = (CFLOAT)edwhA[1];

    #if LOG_ROTRECT_MF_EN
    printf("pLU:(%.2lf, %.2lf) pRU:(%.2lf, %.2lf) pLD:(%.2lf, %.2lf) pRD:(%.2lf, %.2lf) w:%d h:%d \n", pLU[0], pLU[1], pRU[0], pRU[1], pLD[0], pLD[1], pRD[0], pRD[1], edwhA[0], edwhA[1]);
    #endif
    
    d12 = getRectAlignTP(pRectin, pRectin->aspRectLD, pRectin->aspRectLU, pRectout12);
    #if LOG_ROTRECT_MF_EN    
    printf(" d12: %.2lf aspRectLU:(%.2lf, %.2lf) aspRectLD:(%.2lf, %.2lf) \n", d12, pRectin->aspRectLU[0], pRectin->aspRectLU[1], pRectin->aspRectLD[0], pRectin->aspRectLD[1]);
    #endif
    
    d23 = getRectAlignDN(pRectin, pRectin->aspRectRD, pRectin->aspRectLD, pRectout23);
    #if LOG_ROTRECT_MF_EN
    printf(" d23: %.2lf aspRectLD:(%.2lf, %.2lf) aspRectRD:(%.2lf, %.2lf) \n", d23, pRectin->aspRectLD[0], pRectin->aspRectLD[1], pRectin->aspRectRD[0], pRectin->aspRectRD[1]);
    #endif
    
    d34 = getRectAlignTP(pRectin, pRectin->aspRectRU, pRectin->aspRectRD, pRectout34);
    #if LOG_ROTRECT_MF_EN
    printf(" d34: %.2lf aspRectRD:(%.2lf, %.2lf) aspRectRU:(%.2lf, %.2lf) \n", d34, pRectin->aspRectRD[0], pRectin->aspRectRD[1], pRectin->aspRectRU[0], pRectin->aspRectRU[1]);
    #endif
    
    d41 = getRectAlignTP(pRectin, pRectin->aspRectLU, pRectin->aspRectRU, pRectout41);
    #if LOG_ROTRECT_MF_EN
    printf(" d41: %.2lf aspRectRU:(%.2lf, %.2lf) aspRectLU:(%.2lf, %.2lf) \n", d41, pRectin->aspRectRU[0], pRectin->aspRectRU[1], pRectin->aspRectLU[0], pRectin->aspRectLU[1]);
    #endif

    msync(pRectout12, sizeof(struct aspRectObj), MS_SYNC);
    msync(pRectout23, sizeof(struct aspRectObj), MS_SYNC);
    msync(pRectout34, sizeof(struct aspRectObj), MS_SYNC);
    msync(pRectout41, sizeof(struct aspRectObj), MS_SYNC);
    
    findRectOrient(pRectout12R, pRectout12);
    findRectOrient(pRectout23R, pRectout23);
    findRectOrient(pRectout34R, pRectout34);
    findRectOrient(pRectout41R, pRectout41);

    #if LOG_ROTRECT_MF_EN
    dbgprintRect(pRectout12);
    dbgprintRect(pRectout12R);


    dbgprintRect(pRectout23);
    dbgprintRect(pRectout23R);


    dbgprintRect(pRectout34);
    dbgprintRect(pRectout34R);


    dbgprintRect(pRectout41);
    dbgprintRect(pRectout41R);

    dbgprintRect(pRectorg);
    #endif

    memcpy(pRectout12Ro, pRectin, sizeof(struct aspRectObj));
    memcpy(pRectout23Ro, pRectin, sizeof(struct aspRectObj));
    memcpy(pRectout34Ro, pRectin, sizeof(struct aspRectObj));
    memcpy(pRectout41Ro, pRectin, sizeof(struct aspRectObj));
    
    v12 = getRectOffsetLf(pRectout12Ro, pRectout12R, pRectorg, o12);
    v23 = getRectOffsetDn(pRectout23Ro, pRectout23R, pRectorg, o23);
    v34 = getRectOffsetRt(pRectout34Ro, pRectout34R, pRectorg, o34);
    v41 = getRectOffsetTP(pRectout41Ro, pRectout41R, pRectorg, o41);

    if (pRectin->aspRectLU[1] > pRectin->aspRectRU[1]) {
        vmin = aspMin(v41, v12);
    } else {
        vmin = aspMin(v41, v34);
    }

    vdiff = 0;
    if (vmin < v41) {
        vdiff = v41 - vmin;
    }

    if (vdiff < 50.0) {
        vmin = v41;
    }
   
    #if LOG_ROTRECT_MF_EN
    printf(" min: %.4lf v12:%.4lf v23:%.4lf v34:%.4lf v41:%.4lf vdiff:%.4lf\n", vmin, v12, v23, v34, v41, vdiff);
    printf(" v12: %.2lf o12:(%.2lf, %.2lf) d12: %.2lf \n", v12, o12[0], o12[1], d12);
    printf(" v23: %.2lf o23:(%.2lf, %.2lf) d23: %.2lf \n", v23, o23[0], o23[1], d23);
    printf(" v34: %.2lf o34:(%.2lf, %.2lf) d34: %.2lf \n", v34, o34[0], o34[1], d34);
    printf(" v41: %.2lf o41:(%.2lf, %.2lf) d41: %.2lf \n", v41, o41[0], o41[1], d41);
    #endif

    if (vmin == v41) {
        #if LOG_ROTRECT_MF_EN
        printf(" v41 \n");
        #endif
        
        memcpy(&ptEnd[0][0], &pT1[0], 4*sizeof(CFLOAT));
        memcpy(&ptEnd[1][0], &pT2[0], 4*sizeof(CFLOAT));
        memcpy(&ptEnd[2][0], &pT1[0], 4*sizeof(CFLOAT));
        memcpy(&ptEnd[3][0], &pT2[0], 4*sizeof(CFLOAT));
        
        dgs[0] = d41;
        dgs[1] = d41;
        dgs[2] = d23;
        dgs[3] = d23;

        offsets[0] = o41;
        offsets[1] = o41;
        offsets[2] = o23;
        offsets[3] = o23;

        ptreal[0] = 0;
        ptreal[1] = 0;
    }
    else {
        if (pRectin->aspRectLU[1] > pRectin->aspRectRU[1]) {
            if (vmin == v12) {
                #if LOG_ROTRECT_MF_EN
                printf(" v12 \n");
                #endif
                
                memcpy(&ptEnd[0][0], &pT1[0], 4*sizeof(CFLOAT));
                memcpy(&ptEnd[1][0], &pT2[0], 4*sizeof(CFLOAT));
                memcpy(&ptEnd[2][0], &pT1[0], 4*sizeof(CFLOAT));
                memcpy(&ptEnd[3][0], &pT2[0], 4*sizeof(CFLOAT));
                
                dgs[0] = d12;
                dgs[1] = d12;
                dgs[2] = d34;
                dgs[3] = d34;
            
                offsets[0] = o12;
                offsets[1] = o12;
                offsets[2] = o34;
                offsets[3] = o34;
            
                ptreal[0] = 0;
                ptreal[1] = 0;
            } 
            else if (vmin == v34) {
                #if LOG_ROTRECT_MF_EN
                printf(" v34 \n");
                #endif

                memcpy(&ptEnd[0][0], &pT1[0], 4*sizeof(CFLOAT));
                memcpy(&ptEnd[1][0], &pT2[0], 4*sizeof(CFLOAT));
                memcpy(&ptEnd[2][0], &pT1[0], 4*sizeof(CFLOAT));
                memcpy(&ptEnd[3][0], &pT2[0], 4*sizeof(CFLOAT));
                
                dgs[0] = d34;
                dgs[1] = d34;
                dgs[2] = d12;
                dgs[3] = d12;
            
                offsets[0] = o34;
                offsets[1] = o34;
                offsets[2] = o12;
                offsets[3] = o12;

                ptreal[0] = 0;
                ptreal[1] = 0;
            
            }
            else {
                #if LOG_ROTRECT_MF_EN
                printf(" v23 \n");
                #endif
                
                memcpy(&ptEnd[0][0], &pT1[0], 4*sizeof(CFLOAT));
                memcpy(&ptEnd[1][0], &pT2[0], 4*sizeof(CFLOAT));
                memcpy(&ptEnd[2][0], &pT1[0], 4*sizeof(CFLOAT));
                memcpy(&ptEnd[3][0], &pT2[0], 4*sizeof(CFLOAT));
                
                dgs[0] = d23;
                dgs[1] = d23;
                dgs[2] = d41;
                dgs[3] = d41;
            
                offsets[0] = o23;
                offsets[1] = o23;
                offsets[2] = o41;
                offsets[3] = o41;
            
                ptreal[0] = 0;
                ptreal[1] = 0;
            }
        }
        else {
            if (vmin == v34) {
                #if LOG_ROTRECT_MF_EN
                printf(" v34 \n");
                #endif

                memcpy(&ptEnd[0][0], &pT1[0], 4*sizeof(CFLOAT));
                memcpy(&ptEnd[1][0], &pT2[0], 4*sizeof(CFLOAT));
                memcpy(&ptEnd[2][0], &pT1[0], 4*sizeof(CFLOAT));
                memcpy(&ptEnd[3][0], &pT2[0], 4*sizeof(CFLOAT));
                
                dgs[0] = d34;
                dgs[1] = d34;
                dgs[2] = d12;
                dgs[3] = d12;
            
                offsets[0] = o34;
                offsets[1] = o34;
                offsets[2] = o12;
                offsets[3] = o12;
            
                ptreal[0] = 0;
                ptreal[1] = 0;
            
            }
            else if (vmin == v12) {
            
                #if LOG_ROTRECT_MF_EN
                printf(" v12 \n");
                #endif

                memcpy(&ptEnd[0][0], &pT1[0], 4*sizeof(CFLOAT));
                memcpy(&ptEnd[1][0], &pT2[0], 4*sizeof(CFLOAT));
                memcpy(&ptEnd[2][0], &pT1[0], 4*sizeof(CFLOAT));
                memcpy(&ptEnd[3][0], &pT2[0], 4*sizeof(CFLOAT));
                
                dgs[0] = d12;
                dgs[1] = d12;
                dgs[2] = d34;
                dgs[3] = d34;
            
                offsets[0] = o12;
                offsets[1] = o12;
                offsets[2] = o34;
                offsets[3] = o34;

                ptreal[0] = 0;
                ptreal[1] = 0;
            } 
            else {
                #if LOG_ROTRECT_MF_EN
                printf(" v23 \n");
                #endif
                
                memcpy(&ptEnd[0][0], &pT1[0], 4*sizeof(CFLOAT));
                memcpy(&ptEnd[1][0], &pT2[0], 4*sizeof(CFLOAT));
                memcpy(&ptEnd[2][0], &pT1[0], 4*sizeof(CFLOAT));
                memcpy(&ptEnd[3][0], &pT2[0], 4*sizeof(CFLOAT));
                
                dgs[0] = d23;
                dgs[1] = d23;
                dgs[2] = d41;
                dgs[3] = d41;
            
                offsets[0] = o23;
                offsets[1] = o23;
                offsets[2] = o41;
                offsets[3] = o41;
            
                ptreal[0] = 0;
                ptreal[1] = 0;
            }
        }
    }


    ix = 0;

    setRectPoint(pRectorgk, ptEnd[ix][2] - 1, ptEnd[ix][3] - 1, &ptEnd[ix][0]);
    getRectTran(pRectorgk, dgs[ix], offsets[ix], pRectroi);

    *pdeg = dgs[ix];

    #if LOG_ROTRECT_MF_EN
    dbgprintRect(pRectroi);
    #endif

    aspFree(pRectout12, pidx);
    aspFree(pRectout23, pidx);
    aspFree(pRectout34, pidx);
    aspFree(pRectout41, pidx);
    aspFree(pRectorg, pidx);
    aspFree(pRectorgi, pidx);
    aspFree(pRectorgv, pidx);
    aspFree(pRectorgc, pidx);
    aspFree(pRectorgk, pidx);
    aspFree(pRectorgcOut, pidx);
    aspFree(pRectorgkOut, pidx);
    aspFree(pRectout12R, pidx);
    aspFree(pRectout23R, pidx);
    aspFree(pRectout34R, pidx);
    aspFree(pRectout41R, pidx);
    aspFree(pRectout12Ro, pidx);
    aspFree(pRectout23Ro, pidx);
    aspFree(pRectout34Ro, pidx);
    aspFree(pRectout41Ro, pidx);
    aspFree(pRectTga, pidx);
    
    #if LOG_ROTRECT_MF_EN
    printf("tran end ! !\n");       
    #endif
    
    return 0;
}

#define LOG_ROTMF_DBG  (0)
/**
 * rotateBMPMf - get a small rectangle from a big rectangle located in a bmp 
 * @@input parameter:
 * @cropinfo: info of target rectangle
 *    cropinfo[0]: x 
 *    cropinfo[1]: y
 *    cropinfo[2]: width
 *    cropinfo[3]: height
 * @bmpsrc: memory address of raw image for BMP
 * @pmreal: four coordinates of scaned image comes from croping algorithm 
 *    pmreal[0]: x of point 1
 *    pmreal[1]: y of point 1
 *    pmreal[2]: x of point 2
 *    pmreal[3]: y of point 2
 *    pmreal[4]: x of point 3
 *    pmreal[5]: y of point 3
 *    pmreal[6]: x of point 4
 *    pmreal[7]: y of point 4
 *    <example>
 *        int mreal[8], utmp, crod;
 *        utmp = msb2lsb32(&pusbmeta->CROP_POS_F1);
 *        crod = utmp & 0xffff;
 *        utmp = utmp >> 16;
 *        mreal[0] = utmp;
 *        mreal[1] = crod;
 *        
 *        utmp = msb2lsb32(&pusbmeta->CROP_POS_F2);
 *        crod = utmp & 0xffff;
 *        utmp = utmp >> 16;
 *        mreal[2] = utmp;
 *        mreal[3] = crod;
 *        
 *        utmp = msb2lsb32(&pusbmeta->CROP_POS_F3);
 *        crod = utmp & 0xffff;
 *        utmp = utmp >> 16;
 *        mreal[4] = utmp;
 *        mreal[5] = crod;
 *        
 *        utmp = msb2lsb32(&pusbmeta->CROP_POS_F4);
 *        crod = utmp & 0xffff;
 *        utmp = utmp >> 16;
 *        mreal[6] = utmp;
 *        mreal[7] = crod;
 * @pattern: the byte value you want to place in image if the transforming coordinates out of boundary
 * @midx: no matter if outside mothership
 *
 * @@output parameter:
 * @rotbuff: memory address to save raw image which is the result of rotate and crop rectangle
 * @headbuff: memory address of BMP header for bmpsrc
 *
 */
int rotateBMPMf(char *rotbuff, char *headbuff, int *cropinfo, char *bmpsrc, int *pmreal, int pattern, int midx)
{
#define UNIT_DEG (1000.0)
#define MIN_P  (100.0)
#define BMP_8_BIT_HEAD_SIZE (1078)
#define V_FLIP_EN (0)

    char *addr=0, *srcbuf=0, *ph, *rawCpy, *rawSrc, *rawTmp, *rawdest=0;
    int ret, bitset, len=0, totsz=0, lstsz=0, cnt=0, acusz=0, err=0, rvs=0;
    int rawsz=0, oldWidth=0, oldHeight=0, oldRowsz=0, oldTot=0;
    char ch;
    struct bitmapHeader_s *bheader;
    CFLOAT LU[2], RU[2], LD[2], RD[2];
    CFLOAT LUn[2], RUn[2], LDn[2], RDn[2];
    int LUt[2], RUt[2], LDt[2], RDt[2], drawCord[4], bmpScale[4], oldScale[4];
    int sdot[2], ddot[2];
    CFLOAT rangle[2], thacos=0, thasin=0, theta, piAngle = 180.0;
    CFLOAT minH=0, minV=0, offsetH=0, offsetV=0;
    int maxhint=0, maxvint=0, minhint=0, minvint=0, rowsize=0, rawszNew=0;
    int bpp=0, ix=0, iy=0, dx=0, dy=0, outi[2], id=0, ixd=0, iyn=0, ixn=0, offsetCal=0;
    CFLOAT ind[4], outd[2], fx=0, fy=0, tx=0, ty=0;
    CFLOAT *tars, *tarc;
    char gdat[3];
    char *dst=0, *src=0;
    char *paintcolr=0;

    int *crsAry, crsASize, expCAsize;
    CFLOAT linLU[3], linRU[3], linLD[3], linRD[3], linPal[3], linCrs[3];
    CFLOAT pLU[2], pRU[2], pLD[2], pRD[2], pal[2], par[2], pt[2];
    CFLOAT plm[2], prm[2], plc[2], prc[2], pn[2], ptop[2];
    CFLOAT maxhf=0, maxvf=0, minhf=0, minvf=0;
    CFLOAT imgw=0.0, imgh=0.0, imgdeg=0.0;
    int ublen=0, ubret=0, ubrst=0;
    uint32_t val=0;
    int cxm, cxn;
    int deg=0;
    struct aspRectObj *pRectin=0, *pRectROI=0, *pRectinR=0;

    paintcolr = aspMemalloc(sizeof(char) * 4, midx); 
    pRectin = aspMemalloc(sizeof(struct aspRectObj), midx);
    pRectROI = aspMemalloc(sizeof(struct aspRectObj), midx);
    pRectinR = aspMemalloc(sizeof(struct aspRectObj), midx);

    memset(paintcolr, pattern&0xff, sizeof(char) * 4);
    
    srcbuf = bmpsrc;

    /* check header */
    //shmem_dump(srcbuf, 512);

    /* rotate */
    bheader = aspMemalloc(sizeof(struct bitmapHeader_s), midx);
    memset(bheader, 0, sizeof(struct bitmapHeader_s));

    ph = &bheader->aspbmpMagic[2];
    memcpy(ph, headbuff, sizeof(struct bitmapHeader_s) - 2);
    
    #if LOG_ROTMF_DBG    
    dbgBitmapHeader(bheader, len);
    #endif
    rawsz = bheader->aspbiRawSize;
    oldWidth = bheader->aspbiWidth;
    if (bheader->aspbiHeight < 0) {
        bheader->aspbiHeight = 0 - bheader->aspbiHeight;
        rvs = 1;
    }
    oldHeight = bheader->aspbiHeight;
    bpp = bheader->aspbiCPP >> 16;
    oldRowsz = ((bpp * oldWidth + 31) / 32) * 4;

    rawCpy = srcbuf;

    #if LOG_ROTMF_DBG    
    printf("rotate raw offset: %d \n", bheader->aspbhRawoffset); 
    #endif
    
    imgw = (CFLOAT)bheader->aspbiWidth;
    imgh = (CFLOAT)bheader->aspbiHeight;

    cxm = pmreal[0];
    cxn = pmreal[1];

    #if LOG_ROTMF_DBG    
    printf("rot meta get F1: (%d, %d) \n", cxm, cxn); 
    #endif

    LD[0] = (CFLOAT)cxm;
    LD[1] = (CFLOAT)cxn;

    cxm = pmreal[2];
    cxn = pmreal[3];

    #if LOG_ROTMF_DBG    
    printf("rot meta get F2: (%d, %d) \n", cxm, cxn); 
    #endif          

    RD[0] = (CFLOAT)cxm;
    RD[1] = (CFLOAT)cxn;

    cxm = pmreal[4];
    cxn = pmreal[5];

    #if LOG_ROTMF_DBG    
    printf("rot meta get F3: (%d, %d) \n", cxm, cxn); 
    #endif

    RU[0] = (CFLOAT)cxm;
    RU[1] = (CFLOAT)cxn;

    cxm = pmreal[6];
    cxn = pmreal[7];

    #if LOG_ROTMF_DBG    
    printf("rot meta get F4: (%d, %d) \n", cxm, cxn); 
    #endif          

    LU[0] = (CFLOAT)cxm;
    LU[1] = (CFLOAT)cxn;    
    
    memcpy(pRectin->aspRectLU, LU, sizeof(CFLOAT)*2);
    memcpy(pRectin->aspRectLD, LD, sizeof(CFLOAT)*2);
    memcpy(pRectin->aspRectRD, RD, sizeof(CFLOAT)*2);
    memcpy(pRectin->aspRectRU, RU, sizeof(CFLOAT)*2);

    findRectOrient(pRectinR, pRectin);

    #if LOG_ROTMF_DBG    
    dbgprintRect(pRectin);
    #endif
    
    ret = getRotRectPointMf(cropinfo, pRectROI, &imgdeg, oldRowsz, bpp, pRectinR, midx);
    if (ret == 0) {
        memcpy(LU, pRectROI->aspRectLU, sizeof(CFLOAT)*2);
        memcpy(LD, pRectROI->aspRectLD, sizeof(CFLOAT)*2);
        memcpy(RD, pRectROI->aspRectRD, sizeof(CFLOAT)*2);
        memcpy(RU, pRectROI->aspRectRU, sizeof(CFLOAT)*2);
    } else {
        LU[0] = 0;
        LU[1] = bheader->aspbiHeight-1;

        RU[0] = bheader->aspbiWidth-1;
        RU[1] = bheader->aspbiHeight-1;        

        LD[0] = 0;
        LD[1] = 0;

        RD[0] = bheader->aspbiWidth-1;
        RD[1] = 0;

        err = ret;
    }

    #if LOG_ROTMF_DBG    
    printf("getRotRectPointMf: LUn: %lf, %lf\n", LU[0], LU[1]);
    printf("getRotRectPointMf: LDn: %lf, %lf \n", LD[0], LD[1]);
    printf("getRotRectPointMf: RDn: %lf, %lf \n", RD[0], RD[1]);
    printf("getRotRectPointMf: RUn: %lf, %lf \n", RU[0], RU[1]);
    printf("getRotRectPointMf: degree: %.2lf \n", imgdeg);
    #endif
    
    deg = (int)(imgdeg * UNIT_DEG);
    deg = 0 - deg;

    theta = (CFLOAT)deg;
    theta = theta / UNIT_DEG;

    theta = theta * M_PI / piAngle;

    thacos = cos(theta);
    thasin = sin(theta);
    
    rangle[0] = thacos;
    rangle[1] = thasin;
    
    calcuRotateCoordinates(LUt, LUn, LU, rangle);
    calcuRotateCoordinates(RUt, RUn, RU, rangle);
    calcuRotateCoordinates(LDt, LDn, LD, rangle);
    calcuRotateCoordinates(RDt, RDn, RD, rangle);
    
    #if LOG_ROTMF_DBG
    printf("rotate: LU: %.2lf, %.2lf -> %3d, %3d\n", LU[0], LU[1], LUt[0], LUt[1]);
    printf("rotate: LD: %.2lf, %.2lf -> %3d, %3d \n", LD[0], LD[1], LDt[0], LDt[1]);
    printf("rotate: RD: %.2lf, %.2lf -> %3d, %3d \n", RD[0], RD[1], RDt[0], RDt[1]);
    printf("rotate: RU: %.2lf, %.2lf -> %3d, %3d \n", RU[0], RU[1], RUt[0], RUt[1]);
    #endif
    
    minH = aspMin(LUn[0], RUn[0]);
    minH = aspMin(minH, LDn[0]);
    minH = aspMin(minH, RDn[0]);

    minV = aspMin(LUn[1], RUn[1]);
    minV = aspMin(minV, LDn[1]);
    minV = aspMin(minV, RDn[1]);
    
    offsetH = 0 - minH;
    offsetV = 0 - minV;

    LUn[0] += offsetH;
    LUn[1] += offsetV;

    RUn[0] += offsetH;
    RUn[1] += offsetV;

    LDn[0] += offsetH;
    LDn[1] += offsetV;

    RDn[0] += offsetH;
    RDn[1] += offsetV;

    LUt[0] = (int)round(LUn[0]);
    LUt[1] = (int)round(LUn[1]);

    RUt[0] = (int)round(RUn[0]);
    RUt[1] = (int)round(RUn[1]);

    LDt[0] = (int)round(LDn[0]);
    LDt[1] = (int)round(LDn[1]);

    RDt[0] = (int)round(RDn[0]);
    RDt[1] = (int)round(RDn[1]);
    
    #if LOG_ROTMF_DBG    
    printf("bound: LUn: %lf, %lf -> %d, %d\n", LUn[0], LUn[1], LUt[0], LUt[1]);
    printf("bound: RUn: %lf, %lf -> %d, %d \n", RUn[0], RUn[1], RUt[0], RUt[1]);
    printf("bound: LDn: %lf, %lf -> %d, %d \n", LDn[0], LDn[1], LDt[0], LDt[1]);
    printf("bound: RDn: %lf, %lf -> %d, %d \n", RDn[0], RDn[1], RDt[0], RDt[1]);
    #endif
    
    maxhint= aspMaxInt(LUt[0], RUt[0]);
    maxhint = aspMaxInt(maxhint, LDt[0]);
    maxhint = aspMaxInt(maxhint, RDt[0]);

    maxvint = aspMaxInt(LUt[1], RUt[1]);
    maxvint = aspMaxInt(maxvint, LDt[1]);
    maxvint = aspMaxInt(maxvint, RDt[1]);

    minhint= aspMinInt(LUt[0], RUt[0]);
    minhint = aspMinInt(minhint, LDt[0]);
    minhint = aspMinInt(minhint, RDt[0]);

    minvint = aspMinInt(LUt[1], RUt[1]);
    minvint = aspMinInt(minvint, LDt[1]);
    minvint = aspMinInt(minvint, RDt[1]);
    
    rowsize = ((bpp * (maxhint + 1) + 31) / 32) * 4;
    rawszNew = rowsize * (maxvint+1);

    bheader->aspbhSize = bheader->aspbhRawoffset + rawszNew;
    bheader->aspbiWidth = maxhint+1;
    bheader->aspbiHeight = maxvint+1;
    bheader->aspbiRawSize = rawszNew;

    ubrst = 512 - (bheader->aspbhSize % 512);
    
    #if LOG_ROTMF_DBG    
    printf("raw dest size: %d!!!\n", rawszNew);
    #endif
    
    rawdest = rotbuff;
    if (rawdest) {
        #if LOG_ROTMF_DBG    
        printf("get raw dest size: %d succeed!!!\n", rawszNew);
        #endif
    } else {
        printf("allocate raw dest size: %d failed!!!\n", rawszNew);
        return -1;
    }
    
    rawSrc = rawdest;

    #if LOG_ROTMF_DBG    
    printf("maxh: %d, minh: %d, maxv: %d, minv: %d \n", maxhint, minhint, maxvint, minvint);
    #endif

    pLU[0] = -1;
    pLU[1] = -1;
    pRU[0] = -1;
    pRU[1] = -1;
    pLD[0] = -1;
    pLD[1] = -1;
    pRD[0] = -1;
    pRD[1] = -1;

    if (minhint == LUt[0]) {
    
        #if LOG_ROTMF_DBG    
        printf("LU =  %d, %d match minhint: %d !!!left - 0\n", LUt[0], LUt[1], minhint);
        #endif

        if (minvint == LUt[1]) {

            #if LOG_ROTMF_DBG    
            printf("LU =  %d, %d match minvint: %d !!!left - 0\n", LUt[0], LUt[1], minvint);
            #endif
        
            pLD[0] = LUn[0];
            pLD[1] = LUn[1];

            #if LOG_ROTMF_DBG    
            printf("set PLD = %lf, %lf\n", pLD[0], pLD[1]);
            #endif

        } else {
            if (maxvint == LUt[1]) {

                #if LOG_ROTMF_DBG    
                printf("LU =  %d, %d match maxvint: %d !!!left - 0\n", LUt[0], LUt[1], maxvint);
                #endif

                pLU[0] = LUn[0];
                pLU[1] = LUn[1];

                #if LOG_ROTMF_DBG    
                printf("set PLU = %lf, %lf\n", pLU[0], pLU[1]);
                #endif
            }
        }
        
        if ((maxvint == RUt[1]) && (pLU[1] == -1)) {
            if ((minvint == LDt[1]) && (pLD[1] == -1)) {
                if (RUt[0] >= LDt[0]) {
                    pLU[0] = LUn[0];
                    pLU[1] = LUn[1];

                    pLD[0] = LDn[0];
                    pLD[1] = LDn[1];
                    
                    #if LOG_ROTMF_DBG    
                    printf("set PLU = %lf, %lf - 3\n", pLU[0], pLU[1]);
                    #endif

                    #if LOG_ROTMF_DBG    
                    printf("set PLD = %lf, %lf - 3 \n", pLD[0], pLD[1]);
                    #endif

                } else if (RUt[0] < LDt[0]) {
                    pLU[0] = RUn[0];
                    pLU[1] = RUn[1];

                    pLD[0] = LUn[0];
                    pLD[1] = LUn[1];

                    #if LOG_ROTMF_DBG    
                    printf("set PLU = %lf, %lf - 3\n", pLU[0], pLU[1]);
                    #endif

                    #if LOG_ROTMF_DBG    
                    printf("set PLD = %lf, %lf - 3 \n", pLD[0], pLD[1]);
                    #endif

                } else {
                    printf("WARNING!! LU =  %d, %d not match!!!left - 1\n", LUt[0], LUt[1]);
                }
            } else {
                printf("WARNING!! LU =  %d, %d not match!!! left - 2\n", LUt[0], LUt[1]);
            }
        }
    }
    
    if (minhint == RUt[0]) {

        #if LOG_ROTMF_DBG    
        printf("RU =  %d, %d match minhint: %d !!!left - 0\n", RUt[0], RUt[1], minhint);
        #endif

        if (minvint == RUt[1]) {

            #if LOG_ROTMF_DBG    
            printf("RU =  %d, %d match minvint: %d !!!left - 0\n", RUt[0], RUt[1], minvint);
            #endif
            
            pLD[0] = RUn[0];
            pLD[1] = RUn[1];

            #if LOG_ROTMF_DBG    
            printf("set PLD = %lf, %lf\n", pLD[0], pLD[1]);
            #endif
        } else {
            if (maxvint == RUt[1]) {

                #if LOG_ROTMF_DBG    
                printf("RU =  %d, %d match maxvint: %d !!!left - 0\n", RUt[0], RUt[1], maxvint);
                #endif
                
                pLU[0] = RUn[0];
                pLU[1] = RUn[1];

                #if LOG_ROTMF_DBG    
                printf("set PLU = %lf, %lf\n", pLU[0], pLU[1]);
                #endif               
            }
        }

        if ((maxvint == RDt[1]) && (pLU[1] == -1)) {
            if ((minvint == LUt[1]) && (pLD[1] == -1)) {
                if (RDt[0] >= LUt[0]) {
                    pLU[0] = RUn[0];
                    pLU[1] = RUn[1];
        
                    pLD[0] = LUn[0];
                    pLD[1] = LUn[1];

                    #if LOG_ROTMF_DBG    
                    printf("set PLU = %lf, %lf - 3\n", pLU[0], pLU[1]);
                    #endif

                    #if LOG_ROTMF_DBG    
                    printf("set PLD = %lf, %lf - 3 \n", pLD[0], pLD[1]);
                    #endif
                    
                } else if (RDt[0] < LUt[0]) {
                    pLU[0] = RDn[0];
                    pLU[1] = RDn[1];
        
                    pLD[0] = RUn[0];
                    pLD[1] = RUn[1];

                    #if LOG_ROTMF_DBG    
                    printf("set PLU = %lf, %lf - 3\n", pLU[0], pLU[1]);
                    #endif

                    #if LOG_ROTMF_DBG    
                    printf("set PLD = %lf, %lf - 3 \n", pLD[0], pLD[1]);
                    #endif
                    
                } else {
                    printf("WARNING!! RU =  %d, %d not match!!!left - 1\n", RUt[0], RUt[1]);
                }
            } else {
                printf("WARNING!! RU =  %d, %d not match!!!left - 2\n", RUt[0], RUt[1]);
            }
        }
    }
        
    if (minhint == LDt[0]) {

        #if LOG_ROTMF_DBG    
        printf("LD =  %d, %d match minhint: %d !!!left - 0\n", LDt[0], LDt[1], minhint);
        #endif
        
        if (minvint == LDt[1]) {

            #if LOG_ROTMF_DBG    
            printf("LD =  %d, %d match minvint: %d !!!left - 0\n", LDt[0], LDt[1], minvint);
            #endif

            pLD[0] = LDn[0];
            pLD[1] = LDn[1];

            #if LOG_ROTMF_DBG    
            printf("set PLD = %lf, %lf\n", pLD[0], pLD[1]);
            #endif
        } else {
            if (maxvint == LDt[1]) {
                #if LOG_ROTMF_DBG    
                printf("LD =  %d, %d match maxvint: %d !!!left - 0\n", LDt[0], LDt[1], maxvint);
                #endif

                pLU[0] = LDn[0];
                pLU[1] = LDn[1];

                #if LOG_ROTMF_DBG    
                printf("set PLU = %lf, %lf\n", pLU[0], pLU[1]);
                #endif                                
            }
        }

        if ((maxvint == LUt[1]) && (pLU[1] == -1)) {
            if ((minvint == RDt[1]) && (pLD[1] == -1)) {
                if (LUt[0] >= RDt[0]) {
                    pLU[0] = LDn[0];
                    pLU[1] = LDn[1];
        
                    pLD[0] = RDn[0];
                    pLD[1] = RDn[1];

                    #if LOG_ROTMF_DBG    
                    printf("set PLU = %lf, %lf - 3\n", pLU[0], pLU[1]);
                    #endif

                    #if LOG_ROTMF_DBG    
                    printf("set PLD = %lf, %lf - 3 \n", pLD[0], pLD[1]);
                    #endif
                } else if (LUt[0] < RDt[0]) {
                    pLU[0] = LUn[0];
                    pLU[1] = LUn[1];
        
                    pLD[0] = LDn[0];
                    pLD[1] = LDn[1];

                    #if LOG_ROTMF_DBG    
                    printf("set PLU = %lf, %lf - 3\n", pLU[0], pLU[1]);
                    #endif

                    #if LOG_ROTMF_DBG    
                    printf("set PLD = %lf, %lf - 3 \n", pLD[0], pLD[1]);
                    #endif
                } else {
                    printf("WARNING!! LD =  %d, %d not match!!!left - 1\n", LDt[0], LDt[1]);
                }
            } else {
                printf("WARNING!! LD =  %d, %d not match!!!left - 2\n", LDt[0], LDt[1]);
            }
        }
    }
        
    if (minhint == RDt[0]) {

        #if LOG_ROTMF_DBG    
        printf("RD =  %d, %d match minhint: %d !!!left - 0\n", RDt[0], RDt[1], minhint);
        #endif

        if (minvint == RDt[1]) {

            #if LOG_ROTMF_DBG    
            printf("RD =  %d, %d match minvint: %d !!!left - 0\n", RDt[0], RDt[1], minvint);
            #endif

            pLD[0] = RDn[0];
            pLD[1] = RDn[1];                    

            #if LOG_ROTMF_DBG    
            printf("set PLD = %lf, %lf\n", pLD[0], pLD[1]);
            #endif
        } else {
            if (maxvint == RDt[1]) {

                #if LOG_ROTMF_DBG    
                printf("RD =  %d, %d match maxvint: %d !!!left - 0\n", RDt[0], RDt[1], maxvint);
                #endif

                pLU[0] = RDn[0];
                pLU[1] = RDn[1];

                #if LOG_ROTMF_DBG    
                printf("set PLU = %lf, %lf\n", pLU[0], pLU[1]);
                #endif
            }
        }

        if ((maxvint == LDt[1]) && (pLU[1] == -1)) {
            if ((minvint == RUt[1]) && (pLD[1] == -1)) {
                if (LDt[0] >= RUt[0]) {
                    pLU[0] = RDn[0];
                    pLU[1] = RDn[1];
        
                    pLD[0] = RUn[0];
                    pLD[1] = RUn[1];

                    #if LOG_ROTMF_DBG    
                    printf("set PLU = %lf, %lf - 3\n", pLU[0], pLU[1]);
                    #endif

                    #if LOG_ROTMF_DBG    
                    printf("set PLD = %lf, %lf - 3 \n", pLD[0], pLD[1]);
                    #endif
                } else if (LDt[0] < RUt[0]) {
                    pLU[0] = LDn[0];
                    pLU[1] = LDn[1];
        
                    pLD[0] = RDn[0];
                    pLD[1] = RDn[1];

                    #if LOG_ROTMF_DBG    
                    printf("set PLU = %lf, %lf - 3\n", pLU[0], pLU[1]);
                    #endif

                    #if LOG_ROTMF_DBG    
                    printf("set PLD = %lf, %lf - 3 \n", pLD[0], pLD[1]);
                    #endif
                } else {
                    printf("WARNING!! RD =  %d, %d not match!!!left - 1\n", RDt[0], RDt[1]);
                }
            } else {
                printf("WARNING!! RD =  %d, %d not match!!!left - 2\n", RDt[0], RDt[1]);
            }
        }
    }

    if (maxhint == LUt[0]) {

        #if LOG_ROTMF_DBG    
        printf("LU =  %d, %d match maxhint: %d !!!right - 0\n", LUt[0], LUt[1], maxhint);
        #endif

        if (minvint == LUt[1]) {
        
            #if LOG_ROTMF_DBG    
            printf("LU =  %d, %d match minvint: %d !!!right - 0\n", LUt[0], LUt[1], minvint);
            #endif

            pRD[0] = LUn[0];
            pRD[1] = LUn[1];

            #if LOG_ROTMF_DBG    
            printf("set PRD = %lf, %lf\n", pRD[0], pRD[1]);
            #endif

        } else {
            if (maxvint == LUt[1]) {

                #if LOG_ROTMF_DBG    
                printf("LU =  %d, %d match maxvint: %d !!!right - 0\n", LUt[0], LUt[1], maxvint);
                #endif

                pRU[0] = LUn[0];
                pRU[1] = LUn[1];

                #if LOG_ROTMF_DBG    
                printf("set PRU = %lf, %lf\n", pRU[0], pRU[1]);
                #endif
            }
        }
        
        if ((maxvint == LDt[1]) && (pRU[1] == -1)) {
            if ((minvint == RUt[1]) && (pRD[1] == -1)) {
                if (RUt[0] <= LDt[0]) {
                    pRU[0] = LDn[0];
                    pRU[1] = LDn[1];

                    pRD[0] = LUn[0];
                    pRD[1] = LUn[1];

                    #if LOG_ROTMF_DBG    
                    printf("set PRU = %lf, %lf - 3\n", pRU[0], pRU[1]);
                    #endif

                    #if LOG_ROTMF_DBG    
                    printf("set PRD = %lf, %lf - 3\n", pRD[0], pRD[1]);
                    #endif
                
                } else if (RUt[0] > LDt[0]) {
                    pRU[0] = LUn[0];
                    pRU[1] = LUn[1];

                    pRD[0] = RUn[0];
                    pRD[1] = RUn[1];

                    #if LOG_ROTMF_DBG    
                    printf("set PRU = %lf, %lf - 3\n", pRU[0], pRU[1]);
                    #endif

                    #if LOG_ROTMF_DBG    
                    printf("set PRD = %lf, %lf - 3\n", pRD[0], pRD[1]);
                    #endif
                    
                } else {
                    printf("WARNING!! LU =  %d, %d not match!!!right - 1\n", LUt[0], LUt[1]);
                }
            } else {
                printf("WARNING!! LU =  %d, %d not match!!!right - 2\n", LUt[0], LUt[1]);
            }
        }
    }
    
    if (maxhint == RUt[0]) {
    
        #if LOG_ROTMF_DBG    
        printf("RU =  %d, %d match maxhint: %d !!!right - 0\n", RUt[0], RUt[1], maxhint);
        #endif

        if (minvint == RUt[1]) {

            #if LOG_ROTMF_DBG    
            printf("RU =  %d, %d match minvint: %d !!!right - 0\n", RUt[0], RUt[1], minvint);
            #endif

            pRD[0] = RUn[0];
            pRD[1] = RUn[1];

            #if LOG_ROTMF_DBG    
            printf("set PRD = %lf, %lf\n", pRD[0], pRD[1]);
            #endif

        } else {
            if (maxvint == RUt[1]) {

                #if LOG_ROTMF_DBG    
                printf("RU =  %d, %d match maxvint: %d !!!right - 0\n", RUt[0], RUt[1], maxvint);
                #endif

                pRU[0] = RUn[0];
                pRU[1] = RUn[1];

                #if LOG_ROTMF_DBG    
                printf("set PRU = %lf, %lf\n", pRU[0], pRU[1]);
                #endif
            }
        }
        
        if ((maxvint == LUt[1]) && (pRU[1] == -1)) {
            if ((minvint == RDt[1]) && (pRD[1] == -1)) {
                if (RDt[0] <= LUt[0]) {
                    pRU[0] = LUn[0];
                    pRU[1] = LUn[1];
        
                    pRD[0] = RUn[0];
                    pRD[1] = RUn[1];

                    #if LOG_ROTMF_DBG    
                    printf("set PRU = %lf, %lf - 3\n", pRU[0], pRU[1]);
                    #endif

                    #if LOG_ROTMF_DBG    
                    printf("set PRD = %lf, %lf - 3\n", pRD[0], pRD[1]);
                    #endif
                } else if (RDt[0] > LUt[0]) {
                    pRU[0] = RUn[0];
                    pRU[1] = RUn[1];
        
                    pRD[0] = RDn[0];
                    pRD[1] = RDn[1];

                    #if LOG_ROTMF_DBG    
                    printf("set PRU = %lf, %lf - 3\n", pRU[0], pRU[1]);
                    #endif

                    #if LOG_ROTMF_DBG    
                    printf("set PRD = %lf, %lf - 3\n", pRD[0], pRD[1]);
                    #endif
                } else {
                    printf("WARNING!! RU =  %d, %d not match!!!right - 1\n", RUt[0], RUt[1]);
                }
            } else {
                printf("WARNING!! RU =  %d, %d not match!!!right - 2\n", RUt[0], RUt[1]);
            }
        }
    }
        
    if (maxhint == LDt[0]) {

        #if LOG_ROTMF_DBG    
        printf("LD =  %d, %d match maxhint: %d !!!right - 0\n", LDt[0], LDt[1], maxhint);
        #endif

        if (minvint == LDt[1]) {

            #if LOG_ROTMF_DBG    
            printf("LD =  %d, %d match minvint: %d !!!right - 0\n", LDt[0], LDt[1], minvint);
            #endif

            pRD[0] = LDn[0];
            pRD[1] = LDn[1];                  

            #if LOG_ROTMF_DBG    
            printf("set PRD = %lf, %lf\n", pRD[0], pRD[1]);
            #endif

        } else {
            if (maxvint == LDt[1]) {

                #if LOG_ROTMF_DBG    
                printf("LD =  %d, %d match maxvint: %d !!!right - 0\n", LDt[0], LDt[1], maxvint);
                #endif

                pRU[0] = LDn[0];
                pRU[1] = LDn[1];

                #if LOG_ROTMF_DBG    
                printf("set PRU = %lf, %lf\n", pRU[0], pRU[1]);
                #endif
            }
        }

        if ((maxvint == RDt[1]) && (pRU[1] == -1)) {
            if ((minvint == LUt[1]) && (pRD[1] == -1)) {
                if (LUt[0] <= RDt[0]) {
                    pRU[0] = RDn[0];
                    pRU[1] = RDn[1];
        
                    pRD[0] = LDn[0];
                    pRD[1] = LDn[1];

                    #if LOG_ROTMF_DBG    
                    printf("set PRU = %lf, %lf - 3\n", pRU[0], pRU[1]);
                    #endif

                    #if LOG_ROTMF_DBG    
                    printf("set PRD = %lf, %lf - 3\n", pRD[0], pRD[1]);
                    #endif
                } else if (LUt[0] > RDt[0]) {
                    pRU[0] = LDn[0];
                    pRU[1] = LDn[1];
        
                    pRD[0] = LUn[0];
                    pRD[1] = LUn[1];

                    #if LOG_ROTMF_DBG    
                    printf("set PRU = %lf, %lf - 3\n", pRU[0], pRU[1]);
                    #endif

                    #if LOG_ROTMF_DBG    
                    printf("set PRD = %lf, %lf - 3\n", pRD[0], pRD[1]);
                    #endif
                } else {
                    printf("WARNING!! LD =  %d, %d not match!!!right - 1\n", LDt[0], LDt[1]);
                }
            } else {
                printf("WARNING!! LD =  %d, %d not match!!!right - 2\n", LDt[0], LDt[1]);
            }
        }
    }
            
    if (maxhint == RDt[0]) {

        #if LOG_ROTMF_DBG    
        printf("RD =  %d, %d match maxhint: %d !!!right - 0\n", RDt[0], RDt[1], maxhint);
        #endif

        if (minvint == RDt[1]) {

            #if LOG_ROTMF_DBG    
            printf("RD =  %d, %d match minvint: %d !!!right - 0\n", RDt[0], RDt[1], minvint);
            #endif

            pRD[0] = RDn[0];
            pRD[1] = RDn[1];                    

            #if LOG_ROTMF_DBG    
            printf("set PRD = %lf, %lf\n", pRD[0], pRD[1]);
            #endif

        } else {
            if (maxvint == RDt[1]) {

                #if LOG_ROTMF_DBG    
                printf("RD =  %d, %d match maxvint: %d !!!right - 0\n", RDt[0], RDt[1], maxvint);
                #endif

                pRU[0] = RDn[0];
                pRU[1] = RDn[1];

                #if LOG_ROTMF_DBG    
                printf("set PRU = %lf, %lf\n", pRU[0], pRU[1]);
                #endif
            }
        }
        
        if ((maxvint == RUt[1]) && (pRU[1] == -1)) {
            if ((minvint == LDt[1]) && (pRD[1] == -1)) {
                if (LDt[0] <= RUt[0]) {
                    pRU[0] = RUn[0];
                    pRU[1] = RUn[1];
        
                    pRD[0] = RDn[0];
                    pRD[1] = RDn[1];
                    
                    #if LOG_ROTMF_DBG    
                    printf("set PRU = %lf, %lf - 3\n", pRU[0], pRU[1]);
                    #endif

                    #if LOG_ROTMF_DBG    
                    printf("set PRD = %lf, %lf - 3\n", pRD[0], pRD[1]);
                    #endif
                } else if (LDt[0] > RUt[0]) {
                    pRU[0] = RDn[0];
                    pRU[1] = RDn[1];
        
                    pRD[0] = LDn[0];
                    pRD[1] = LDn[1];
                    
                    #if LOG_ROTMF_DBG    
                    printf("set PRU = %lf, %lf - 3\n", pRU[0], pRU[1]);
                    #endif

                    #if LOG_ROTMF_DBG    
                    printf("set PRD = %lf, %lf - 3\n", pRD[0], pRD[1]);
                    #endif
                } else {
                    printf("WARNING!! RD =  %d, %d not match!!!right - 1\n", RDt[0], RDt[1]);
                }
            }
            else {
                printf("WARNING!! RD =  %d, %d not match!!!right - 2\n", RDt[0], RDt[1]);
            }
        }
    }

    LUt[0] = (int)round(pLU[0]*MIN_P);
    LUt[1] = (int)round(pLU[1]*MIN_P);

    RUt[0] = (int)round(pRU[0]*MIN_P);
    RUt[1] = (int)round(pRU[1]*MIN_P);

    LDt[0] = (int)round(pLD[0]*MIN_P);
    LDt[1] = (int)round(pLD[1]*MIN_P);

    RDt[0] = (int)round(pRD[0]*MIN_P);
    RDt[1] = (int)round(pRD[1]*MIN_P);

    pLU[0] = (double)LUt[0];
    pLU[1] = (double)LUt[1];
    pRU[0] = (double)RUt[0];
    pRU[1] = (double)RUt[1];
    pLD[0] = (double)LDt[0];
    pLD[1] = (double)LDt[1];
    pRD[0] = (double)RDt[0];
    pRD[1] = (double)RDt[1];

    pLU[0] = pLU[0]/MIN_P;
    pLU[1] = pLU[1]/MIN_P;
    pRU[0] = pRU[0]/MIN_P;
    pRU[1] = pRU[1]/MIN_P;
    pLD[0] = pLD[0]/MIN_P;
    pLD[1] = pLD[1]/MIN_P;
    pRD[0] = pRD[0]/MIN_P;
    pRD[1] = pRD[1]/MIN_P; 
    
    #if LOG_ROTMF_DBG
    printf("align: PLU: %lf, %lf \n", pLU[0], pLU[1]);
    printf("align: PRU: %lf, %lf \n", pRU[0], pRU[1]);
    printf("align: PLD: %lf, %lf \n", pLD[0], pLD[1]);
    printf("align: PRD: %lf, %lf \n", pRD[0], pRD[1]);
    #endif

    if (pLU[1] > pRU[1]) {
        #if LOG_ROTMF_DBG
        printf("PLU[1]: %lf  > PRU[1]: %lf \n", pLU[1], pRU[1]);
        #endif

        getVectorFromP(linLU, pLD, pLU);
        getVectorFromP(linLD, pLD, pRD);
        getVectorFromP(linRD, pRD, pRU);
        getVectorFromP(linRU, pRU, pLU);

        #if LOG_ROTMF_DBG
        printf("lineLU:(%lf, %lf, %lf) = pLU:(%lf, %lf) <-> pLD:(%lf, %lf)  \n", linLU[0], linLU[1], linLU[2], pLU[0], pLU[1], pLD[0], pLD[1]);
        printf("linLD:(%lf, %lf, %lf) = pLD:(%lf, %lf) <-> pRD:(%lf, %lf)  \n", linLD[0], linLD[1], linLD[2], pLD[0], pLD[1], pRD[0], pRD[1]);
        printf("linRD:(%lf, %lf, %lf) = pRD:(%lf, %lf) <-> pRU:(%lf, %lf)  \n", linRD[0], linRD[1], linRD[2], pRD[0], pRD[1], pRU[0], pRU[1]);
        printf("linRU:(%lf, %lf, %lf) = pRU:(%lf, %lf) <-> pLU:(%lf, %lf)  \n", linRU[0], linRU[1], linRU[2], pRU[0], pRU[1], pLU[0], pLU[1]);
        #endif

        plm[0] = pLD[0];
        plm[1] = pLD[1];
        
        prm[0] = pRU[0];
        prm[1] = pRU[1];
    }
    else {
        #if LOG_ROTMF_DBG
        printf("PLU[1]: %lf  <= PRU[1]: %lf \n", pLU[1], pRU[1]);
        #endif

        getVectorFromP(linLU, pRU, pLU);
        getVectorFromP(linLD, pLU, pLD);
        getVectorFromP(linRD, pLD, pRD);
        getVectorFromP(linRU, pRD, pRU);

        #if LOG_ROTMF_DBG
        printf("lineLU:(%lf, %lf, %lf) = pRU:(%lf, %lf) <-> pLU:(%lf, %lf)  \n", linLU[0], linLU[1], linLU[2], pRU[0], pRU[1], pLU[0], pLU[1]);
        printf("linLD:(%lf, %lf, %lf) = pLU:(%lf, %lf) <-> pLD:(%lf, %lf)  \n", linLD[0], linLD[1], linLD[2], pLU[0], pLU[1], pLD[0], pLD[1]);
        printf("linRD:(%lf, %lf, %lf) = pLD:(%lf, %lf) <-> pRD:(%lf, %lf)  \n", linRD[0], linRD[1], linRD[2], pLD[0], pLD[1], pRD[0], pRD[1]);
        printf("linRU:(%lf, %lf, %lf) = pRU:(%lf, %lf) <-> pLU:(%lf, %lf)  \n", linRU[0], linRU[1], linRU[2], pRD[0], pRD[1], pRU[0], pRU[1]);
        #endif
        
        plm[0] = pLU[0];
        plm[1] = pLU[1];
        
        prm[0] = pRD[0];
        prm[1] = pRD[1];
    }

    ret = getCross(linLD, linLU, plc);

    ret = getCross(linRD, linRU, prc);

    ret = getCross(linRU, linLU, ptop);

    ret= getCross(linRD, linLD, pn);

    
    #if LOG_ROTMF_DBG
    printf("test linLD cross linLU.  left %lf, %lf ret: %d \n", plc[0], plc[1], ret);
    printf("test linRD cross linRU.  right %lf, %lf ret: %d \n", prc[0], prc[1], ret);
    printf("test linRU cross linLU.  top %lf, %lf ret: %d \n", ptop[0], ptop[1], ret);
    printf("test linRD cross linLD.  down %lf, %lf ret: %d \n", pn[0], pn[1], ret);
    #endif

    pal[0] = 100;
    pal[1] = 100;

    par[0] = 100;
    par[1] = 1000;
    getVectorFromP(linPal, par, pal);        

    #if LOG_ROTMF_DBG
    printf("linPal:(%lf, %lf, %lf) = par:(%lf, %lf) <-> pal:(%lf, %lf)  \n", linPal[0], linPal[1], linPal[2], par[0], par[1], pal[0], pal[1]);
    #endif

    expCAsize = maxvint-minvint+1;
    len = 3*sizeof(int);
    crsAry = aspMemalloc(expCAsize*len, midx);

    #if LOG_ROTMF_DBG
    printf("bf trim r: %lf, l: %lf, top: %lf down: %lf \n", prm[1], plm[1], ptop[1], pn[1]);
    #endif
    
    pt[0] = 200.0;
    fy = ptop[1] - plm[1];
    if (fy < 1.0) {
        plm[1] = ptop[1];
    }

    fy = ptop[1] - prm[1];
    if (fy < 1.0) {
        prm[1] = ptop[1];
    }

    fy = plm[1] - pn[1];
    if (fy < 1.0) {
        plm[1] = pn[1];
    }

    fy = prm[1] - pn[1];
    if (fy < 1.0) {
        prm[1] = pn[1];
    }

    #if LOG_ROTMF_DBG
    printf("after trim r: %lf, l: %lf, top: %lf down: %lf \n", prm[1], plm[1], ptop[1], pn[1]);
    #endif

    for (iy=minvint, ix=0; ix < expCAsize; iy++, ix++) {
        pt[1] = (CFLOAT)iy;
        getRectVectorFromV(linCrs, pt, linPal);
        #if LOG_ROTMF_DBG
        printf("linCrs:(%lf, %lf, %lf) = pt:(%lf, %lf)  \n", linCrs[0], linCrs[1], linCrs[2], pt[0], pt[1]);
        #endif
        if (pt[1] > plm[1]) {
            getCross(linCrs, linLU, plc);
        } else {
            getCross(linCrs, linLD, plc);
        }
        if (pt[1] > prm[1]) {
            getCross(linCrs, linRU, prc);
        } else {
            getCross(linCrs, linRD, prc);
        }
        crsAry[ix*3+0] = iy;
        crsAry[ix*3+1] = (int)round(plc[0]);
        crsAry[ix*3+2] = (int)round(prc[0]);
    }



    /*
    for (ix=0; ix < (maxvint-minvint+1); ix++) {
        printf("list %d. %d, %d, %d (%d)\n", ix, crsAry[ix*3+0], crsAry[ix*3+1], crsAry[ix*3+2], crsAry[ix*3+2] - crsAry[ix*3+1]);
    }
    */
    
    #if LOG_ROTMF_DBG    
    printf("new bitmap H/V = %d /%d, rowsize: %d, rawsize: %d, buffused: %d, sizeof crsArry: %d\n", maxhint, maxvint, rowsize, rawszNew, totsz, expCAsize);
    #endif
    
    /* retate raw data */
    oldScale[0] = oldRowsz;
    oldScale[1] = oldHeight;
    oldScale[2] = rawsz;
    oldScale[3] = bpp;        

    bmpScale[0] = rowsize;
    bmpScale[1] = maxvint;        
    bmpScale[2] = rawszNew;
    bmpScale[3] = bpp;

    rawTmp = rawSrc;
    //memset(rawTmp, 0, rawszNew);
    
    /* reverse to fill the rotate image */
    theta = (CFLOAT) (360*UNIT_DEG - deg);
    theta = theta / UNIT_DEG;
    
    #if LOG_ROTMF_DBG    
    printf("reverse rotate angle = %lf \n", theta);
    #endif

    theta = theta * M_PI / piAngle;

    thacos = cos(theta);
    thasin = sin(theta);

    lstsz = 0;
    totsz = bheader->aspbhSize;
    bpp = bpp / 8;

    msync(crsAry, expCAsize*3*4, MS_SYNC);
    
    cnt = 0;
    
    for (id=0; id < expCAsize; id++) {
        iy = crsAry[id*3+0];
        ix = crsAry[id*3+1];
        ixd = crsAry[id*3+2]; 

        fy = (CFLOAT)iy;
        fy -= offsetV;
        
        for (;ix <= ixd; ix++) {       

            fx = (CFLOAT)ix;

            fx -= offsetH;

            dx = (int) round(fx*thacos - fy*thasin);
            dy = (int) round(fx*thasin + fy*thacos);

            cnt++;
            bitset = bpp;
            
            if ((dx < 0) || (dy < 0) || (dx >= oldWidth) || (dy >= oldHeight)) {
                src = paintcolr;
                //printf("ob dx: %d, dy: %d, ix: %d, iy: %d colr: 0x%.2x\n", dx, dy, ix, iy, src[0]);
                //continue;
            } else {
                src = getPixel(rawCpy, dx, dy, oldRowsz, bitset);
            }

            #if V_FLIP_EN
            if (rvs) {
                //printf("(%d, %d) \n", ix, expCAsize - iy);
                dst = getPixel(rawTmp, ix, expCAsize - iy - 1, rowsize, bitset);
            } else {
                dst = getPixel(rawTmp, ix, iy, rowsize, bitset);
            }
            #else
            //printf("(%d, %d) %d\n", ix, iy, expCAsize - 1 - iy);
            dst = getPixel(rawTmp, ix, iy, rowsize, bitset);
            #endif
            
            cnt = 0;
            while (bitset > 0) {
                *dst = *src;

                bitset --;
                cnt++;
                src++;
                dst++;

                if (cnt > 2) break;
            }
        }
    }

    msync(rawTmp, totsz, MS_SYNC); 

    #if !V_FLIP_EN
    if (rvs) {
        bheader->aspbiHeight = 0 - bheader->aspbiHeight;;
    }
    #endif

    #if LOG_ROTMF_DBG
    dbgBitmapHeader(bheader, sizeof(struct bitmapHeader_s) - 2);
    #endif
    
    memcpy(headbuff, ph, sizeof(struct bitmapHeader_s) - 2);
    
    aspFree(pRectin, midx);
    aspFree(pRectROI, midx);
    aspFree(pRectinR, midx);
    aspFree(bheader, midx);
    aspFree(crsAry, midx);
    
    return err; 
}


