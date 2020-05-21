#include <stdint.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <unistd.h> 
#include <math.h>

#define CFLOAT double

#define LOG_ROTMF_DBG  (0)
static int rotateBMPMf(int *cropinfo, char *bmpsrc, char *rotbuff, int *pmreal, char *headbuff, int midx)
{
#define UNIT_DEG (1000.0)
#define MIN_P  (100.0)

    char *addr=0, *srcbuf=0, *ph, *rawCpy, *rawSrc, *rawTmp, *rawdest=0;
    int ret, bitset, len=0, totsz=0, lstsz=0, cnt=0, acusz=0, err=0;
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

    pRectin = aspMemalloc(sizeof(struct aspRectObj), midx);
    pRectROI = aspMemalloc(sizeof(struct aspRectObj), midx);
    pRectinR = aspMemalloc(sizeof(struct aspRectObj), midx);
    
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
    
    dbgprintRect(pRectin);
            
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
    memset(rawTmp, 0, rawszNew);
    
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

            if ((dx < 0) || (dy < 0) || (dx >= oldWidth) || (dy >= oldHeight)) {
                continue;
            }

            bitset = bpp / 8;
            src = getPixel(rawCpy, dx, dy, oldRowsz, bitset);
            dst = getPixel(rawTmp, ix, iy, rowsize, bitset);

            
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

    msync(rawdest, totsz, MS_SYNC);

    #if LOG_ROTMF_DBG
    dbgBitmapHeader(bheader, sizeof(struct bitmapHeader_s) - 2);
    #endif
    memcpy(headbuff, ph, sizeof(struct bitmapHeader_s) - 2);
    
    return err; 
}


