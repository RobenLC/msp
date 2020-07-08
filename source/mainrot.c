#include <stdint.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <unistd.h> 
#include <math.h>
#include <sys/mman.h> 
#include <time.h>
#define GHP_EN (1)
#if GHP_EN
#include <jpeglib.h>
#include <jerror.h>
//#include <turbojpeg.h>
//#include <GL/gl.h>
//#include <GL/glext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
//#include <GLES2/gl2platform.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
//#include <GL/glx.h>
#endif

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

extern int rotateBMPMf(char *rotbuff, char *headbuff, int *cropinfo, char *bmpsrc, int *pmreal, int pattern, int midx);
extern int dbgBitmapHeader(struct bitmapHeader_s *ph, int len);
extern int dbgMetaUsb(struct aspMetaDataviaUSB_s *pmetausb);
extern uint32_t msb2lsb32(struct intMbs32_s *msb);


static int jpeg2rgbm(unsigned char *pjpg, int jpgsz, char *prgb, int rgbsz, int *getW, int * getH, int bpp)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr err;
 
    JSAMPARRAY samplebuffer;
    JDIMENSION offsetx, cropW, skipf, skipb;
    int row_stride = 0;
    char* tmpbuff = NULL;
    int rgb_size, clrsp=0;

    //printf("[JPG] jpeg2rgbm enter \n"); 
    
    if (!pjpg) {
        printf("[JPG] jpg buffer is null \n");
        return -1;
    }
    
    if (!prgb)
    {
        printf("[JPG] output buff is null \n");
        return -2;
    }
    
    cinfo.err = jpeg_std_error(&err);
    
    #if 1 /* fast decoding */
    cinfo.do_fancy_upsampling=TRUE;
    cinfo.dct_method=JDCT_FASTEST;
    #endif

    jpeg_create_decompress(&cinfo);
    //printf("[JPG] jpeg_create_decompress. \n"); 

    #if 1 /* fast decoding */
    cinfo.do_fancy_upsampling=TRUE;
    cinfo.dct_method=JDCT_FASTEST;
    #endif

    jpeg_mem_src(&cinfo, pjpg, jpgsz);
    //printf("[JPG] jpeg_mem_src size: %d \n", jpgsz); 
    
    //shmem_dump(pjpg, 512);
     
    jpeg_read_header(&cinfo, TRUE);
    //printf("[JPG] jpeg_read_header. bpp: %d output_components: %d \n", bpp, cinfo.output_components); 
    cinfo.output_components = bpp / 8;
    
    clrsp = (bpp==8) ? JCS_GRAYSCALE:JCS_RGB;
    
    cinfo.out_color_space = clrsp;//JCS_GRAYSCALE;//JCS_RGB;//JCS_GRAYSCALE; //JCS_YCbCr;

    #if 1 /* fast decoding */
    cinfo.do_fancy_upsampling=TRUE;
    cinfo.dct_method=JDCT_FASTEST;
    #endif

    jpeg_start_decompress(&cinfo);
    //printf("[JPG] jpeg_start_decompress. \n"); 
     
    #if 0
    offsetx = cinfo.output_width / 4;
    cropW = cinfo.output_width / 2;
    printf("[JPG] jpeg_crop_scanline. %d, %d S\n", offsetx, cropW); 
    jpeg_crop_scanline(&cinfo, &offsetx, &cropW);
    printf("[JPG] jpeg_crop_scanline. %d, %d E\n", offsetx, cropW); 
    #endif
    
    //row_stride = cinfo.output_width * cinfo.output_components;
    row_stride = ((cinfo.output_width * bpp + 31) / 32) * 4;;
    
    *getW = cinfo.output_width;
    *getH = cinfo.output_height;
    
    //printf("[JPG] jpeg_read_header. width: %d height: %d row_stride: %d jpeg_skip_scanlines S1\n", cinfo.output_width, cinfo.output_height, row_stride); 
    //skipf = 100;
    //skipb = 200;
    //jpeg_skip_scanlines(&cinfo, skipf);
    //printf("[JPG] jpeg_read_header. width: %d height: %d row_stride: %d jpeg_skip_scanlines E1\n", cinfo.output_width, cinfo.output_height, row_stride); 
     
    rgb_size = row_stride * cinfo.output_height;
    if (rgbsz < rgb_size) {
        printf("[JPG] output buff size %d is wrong should be %d \n", rgbsz, rgb_size);
        return -3;
    }
    
    //printf("[JPG] output buff size: %d, bmp size: %d \n", rgbsz, rgb_size);
    
    samplebuffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

    #if 0
    printf("[JPG] debug: rgb_size: %d, raw size: %d w: %d h: %d row_stride: %d \n", rgb_size,
                cinfo.image_width*cinfo.image_height*cinfo.output_components,
                cinfo.image_width, 
                cinfo.image_height,
                row_stride);
    #endif
                
    tmpbuff = prgb;
    while (cinfo.output_scanline < (cinfo.output_height))
    {
        jpeg_read_scanlines(&cinfo, samplebuffer, 1);
        memcpy(tmpbuff, samplebuffer[0], row_stride);
        tmpbuff += row_stride;
    }
 
    //printf("[JPG] jpeg_read_header. width: %d height: %d row_stride: %d jpeg_skip_scanlines S2\n", cinfo.output_width, cinfo.output_height, row_stride); 
    //jpeg_skip_scanlines(&cinfo, skipb);
    //printf("[JPG] jpeg_read_header. width: %d height: %d row_stride: %d jpeg_skip_scanlines E2\n", cinfo.output_width, cinfo.output_height, row_stride); 
    
    jpeg_finish_decompress(&cinfo);

    //printf("[JPG] jpeg_read_header. width: %d height: %d row_stride: %d jpeg_skip_scanlines FINISH\n", cinfo.output_width, cinfo.output_height, row_stride); 
    
    jpeg_destroy_decompress(&cinfo);
    
    return 0;
}

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
        printf("Error!!! read file size not math size: %d ret: %d \n", len, ret);
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
    *raw = 0;

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
        printf("Error!!! read meta failed!!! ret: %d \n", ret);
        err = -3 + ret*10;
        goto Error;
    }

    Error:

    return err;
}

static int doRot2BMP(char *rotraw, char *rothead, int *cropinfo, char *bmpraw, int *mreal)
{
    int err=0, ret=0;
    
    //printf("cropinfo[4]: (%d, %d, %d, %d) \n", cropinfo[0], cropinfo[1], cropinfo[2], cropinfo[3]);    
    //printf("mreal[8]: (%d, %d, %d, %d, %d, %d, %d, %d) \n", mreal[0], mreal[1], mreal[2], mreal[3], 
    //    mreal[4], mreal[5], mreal[6], mreal[7]);    

    //memset(rotraw, 0xff, cropinfo[2] * cropinfo[3]);
    //memcpy(rothead, bmpraw, 1078);
    //memcpy(&bheader->aspbmpMagic[2], bmpraw, sizeof(struct bitmapHeader_s) - 2);
    //dbgBitmapHeader(bheader, sizeof(struct bitmapHeader_s) - 2);
    
    rotateBMPMf(rotraw, rothead, cropinfo, bmpraw+1078, mreal, 0xa5, 0);

    return 0;
}

static int saveRot2BMP(char *raw, char *head, int ntd, int *dat, int idx)
{
    int ret=0, err=0, len=0;
    char ptfileInfo[] = "/home/root/rotate/rot_%d_%d_%d_%.2d";
    char filetail[] = "_%.3d.bmp";
    char ptfileSave[256]={0};
    char dumpath[256]={0};
    FILE *fdump=0;
    int abuf_size=0, bhlen=0, bmph=0;
    struct bitmapHeader_s *bheader=0;

    sprintf(ptfileSave, ptfileInfo, ntd, dat[0], dat[1], idx);
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
    } else {
        bmph = bheader->aspbiHeight;
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

static int get_image_in_jpg(char **retjpg, int *retw, int *reth, int *retsize, int *pmreal, FILE *f)
{
    int image_size=0, mul=0, tail=0, shf=0, ix=0, jpglen=0, exlen=0, yllen=0, val=0, mtlen=0;
    unsigned char *pt=0, *pmeta=0, *pexmt=0, *jpgbuf=0;
    char filename[256]={0};
    int ret=0;
    int size=0, imgw=0, imgh=0;
    int utmp, crod;
    struct aspMetaDataviaUSB_s *pusbmeta=0;
    
    if (!f) {
        printf("file is null \n");
        return -1;
    }

    ret |= fseek(f, 0, SEEK_END);

    size = ftell(f);

    ret |= fseek(f, 0, SEEK_SET);

    if (ret) {
        return -3;
    }

    jpgbuf = malloc(size);
    if (!jpgbuf) {
        return -4;
    }
    
    image_size = fread(jpgbuf, 1, size, f);
    if ((image_size < 0) || (image_size != size)) {
        printf("read file %s error ret=%d \r\n", filename, image_size);
        fclose(f);
        return -5;
    }
    
    fclose(f);
    
    tail = size % 512;
    mul = (size - tail) / 512;

    printf("read file: [%s] size: %d, mul: %d, tail: %d \n", filename, size, mul, tail);

    for (ix=0; ix < mul; ix++) {
        shf = (mul - ix) * 512;
        pt = jpgbuf + shf;
        
        if (pt[0] == 'A') {
            if ((pt[1] == 'S') && (pt[2] == 'P') && (pt[3] == 'C')) {
                pmeta = pt;
                break;
            }
        }
        
        /*
        else {
            sprintf_f(rs->logs,"%d. [0x%.8x]: 0x%.2x \n", ix, shf, pt[0]);
            print_f(rs->plogs, "fIle", rs->logs);
        }
        */
    }

    if (!pmeta) {
        printf("Error!!! can't find meta search count: %d \n", ix);
        return -6;
    }
    
    jpglen = pmeta - jpgbuf;

    val = size - jpglen;

    tail = val % 16;
    mul = (val - tail) / 16;
    
    for (ix=0; ix < (mul - 1); ix++) {
        shf = (ix + 1) * 16;
        pt = pmeta + shf;

        if ((pt[0] == 'Y') && (pt[1] == 'L')) {
            pexmt = pt;
        }
    }

    if (!pexmt) return -7;
    
    mtlen = pexmt - pmeta;
    exlen = size - jpglen - mtlen;
    
    pusbmeta = (struct aspMetaDataviaUSB_s *)pmeta;
    dbgMetaUsb(pusbmeta);
    
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
    
    pt = (unsigned char *)&(pusbmeta->YLines_Recorded);
    val = (pt[0] << 8) | pt[1];
    yllen = (val * 4) + 4;

    val = (pexmt[2] << 8) | pexmt[3];
    val += 4;

    printf("mass len: %d m_len: %d rec_len: %d meta_len: %d jpeg_len: %d updn: %d\n", exlen, val, yllen, mtlen, jpglen, pusbmeta->PRI_O_SEC); 

    if ((exlen != yllen) || (exlen != val)) {
        printf("Error!!! extra len not match, len: %d should be %d or %d \n", exlen, yllen, val);
        return -8;
    }

    //memcpy(decmeta->aspDcData->mfourData, pmeta, mtlen - 64);
    //memcpy(decexmt->aspDcData->mfourData, pexmt, exlen);

    imgw = (pusbmeta->IMG_WIDTH[1] << 8) | pusbmeta->IMG_WIDTH[0];
    imgh = pusbmeta->IMG_HIGH[0] | (pusbmeta->IMG_HIGH[1] << 8);

    *retw = imgw;
    *reth = imgh;
    *retsize = jpglen;
    *retjpg = jpgbuf;
    
    printf("open image (w=%d h=%d size=%d)\r\n", imgw, imgh, jpglen);

    return 0;
}

static int bitmapHeaderSetupRvs(struct bitmapHeader_s *ph, int clr, int w, int h, int dpi, int flen) 
{
    int rawoffset=0, totsize=0, numclrp=0, calcuraw=0, rawsize=0;
    float resH=0, resV=0, ratio=39.27, fval=0;
    uint32_t cmpl=0xffffffff;

    if (!w) return -1;
    if (!h) return -2;
    if (!dpi) return -3;
    if (!flen) return -4;
    memset(ph, 0, sizeof(struct bitmapHeader_s));

    if (clr == 8) {
        numclrp = 256;
        rawoffset = 1078;
        //numclrp = 0;
        //rawoffset = 54;
        calcuraw = w * h;
    }
    else if (clr == 24) {
        numclrp = 0;
        rawoffset = 54;
        calcuraw = w * h * 3;
    } else {
        printf("[BMP] reset header ERROR!!! color bits is %d \n", clr);
        return -5;
    }

    if (calcuraw != flen) {
        //printf("[BMP] WARNNING!!! raw size %d is wrong, should be %d x %d x %d= %d \n", flen, w, h, clr / 8, calcuraw);
        if (flen > calcuraw) {
            rawsize = calcuraw;
        } else {
            rawsize = flen;
        }
    } else {
        rawsize = calcuraw;
    }

    totsize = rawsize + rawoffset;
    
    fval = dpi;
    resH = fval * ratio;
    fval = dpi;
    resV = fval * ratio;
    
    ph->aspbmpMagic[2] = 'B';
    ph->aspbmpMagic[3] = 'M';       
    ph->aspbhSize = totsize; // file total size
    ph->aspbhRawoffset = rawoffset; // header size include color table 54 + 1024 = 1078
    ph->aspbiSize = 40;
    ph->aspbiWidth = w; // W
    cmpl = (cmpl - h) + 1;
    ph->aspbiHeight = cmpl; // H
    //ph->aspbiHeight = h; // H
    ph->aspbiCPP = 1;
    //ph->aspbiCPP = 0;
    ph->aspbiCPP |= clr << 16;  // 8 or 24
    ph->aspbiCompMethd = 0;
    ph->aspbiRawSize = rawsize; // size of raw
    ph->aspbiResoluH = (int)resH; // dpi x 39.27
    ph->aspbiResoluV = (int)resV; // dpi x 39.27
    ph->aspbiNumCinCP = numclrp;  // 24bit is 0, 8bit is 256
    ph->aspbiNumImpColor = 0;

    return 0;
}

static int bitmapColorTableSetupd(char *p)
{
    int i=0;
    char val=0, *dst=0;
    if (!p) return -1;

    dst = p;

    for (i = 0; i < 256; i++) {
        dst[i*4+0] = val;
        dst[i*4+1] = val;
        dst[i*4+2] = val;
        dst[i*4+3] = 0;

        val++;
    }

    return 0;
}

static int time_diffm(struct timespec *s, struct timespec *e, int unit)
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

    if (diff == 0) {
        diff = 1;
    }

    return diff;
}

static int mipc_read(int *pipes, int size, char *ptr) 
{
    int ret=0;

    ret = read(pipes[0], ptr, size);

    return ret;
}

static int mipc_write(int *pipes, int size, char *ptr) 
{
    int ret=0;

    ret = write(pipes[1], ptr, size);

    return ret;
}

#define DUMP_ROT_BMP (1)
int main(int argc, char *argv[]) 
{
    char filepath[256];
    char bnotefile[128]="/home/root/banknote/ASP_%d_%.2d_%.6d_org.bmp";
    char bnotejpg[128]="/home/root/banknote/full_H%.3d.jpg";
    int data[3][5]={{100, 83, 332, 200, 50},{500, 141, 334, 200, 50},{1000, 177, 330, 200, 50}};
    int len=0, ret=0, err=0, cnt=0, nt=0, ntd=0, ix=0;
    FILE *f=0;
    int cropinfo[8]={0};
    char *rotraw=0, *rothead=0;

    printf("input argc: %d config: dump(%d) jpg\n", argc, DUMP_ROT_BMP);

    while (ix < argc) {
        printf("[%d]: %s \n", ix, argv[ix]);

        ix++;
    }

    rothead = malloc(1080);
    if (!rothead) {
        err = -4;
        goto end;
    }

    if (argc > 2) {
        ntd = atoi(argv[1]);
        cnt = atoi(argv[2]);

        for(nt=0; nt < 3; nt++) {
            if (data[nt][0] == ntd) break;
        }

        if (data[nt][0] == ntd) {
            memcpy(cropinfo, &data[nt][1], sizeof(int) * 4);
        } else {
            printf("error input value: %d \n", ntd);
            goto end;
        }
        
        printf("ntd: %d idx: %d (%d, %d, %d, %d, %d, %d)\n", ntd, cnt, cropinfo[0], cropinfo[1], cropinfo[2], cropinfo[3], cropinfo[4], cropinfo[5]);

        sprintf(filepath, bnotejpg, cnt);
        f = fopen(filepath, "r");

        if (!f) {
            printf("get file [%s] failed!! \n", filepath);
            err = -2;
            goto end;
        } else {
            char *buffjpg=0, *buffraw=0, *buffhead=0, *pclortable=0;
            int jpgw=0, jpgh=0, jret=0, rawsz=0, jpgsz=0, decw=0, dech=0;
            int treal[8]={0}, tmCost=0;
            struct bitmapHeader_s *ph=0;
            struct timespec jpgS, jpgE;         
            int pid=0;
            
            jret = get_image_in_jpg(&buffjpg, &jpgw, &jpgh, &jpgsz, treal, f);
            if (jret) {
                if (buffjpg) free(buffjpg);
                fclose(f);
                f = 0;
                printf("get jpg file failed ret: %d \n", jret);
                goto end;
            }

            fclose(f);
            f = 0;
            
            printf("get jpg file w:%d h:%d jpgsz: %d\n", jpgw, jpgh, jpgsz);
            rawsz = jpgw * jpgh;
            buffhead = malloc(rawsz+4+1078); 
            buffraw = buffhead + 4 + 1078;
            if (!buffraw) {
                err = -3;
                free(buffjpg);
                goto end;
            }

            #define PS_NUM (4)

            //pid = fork();
            int pipeswt[PS_NUM+1][2];
            int pipesrd[PS_NUM+1][2];
            int pids[PS_NUM];
            int pmslf=-1;
            int mret=0, infolen=0;
            char pinfo[32]={0};

            memset(pids, 0xff, sizeof(int) * PS_NUM);

            for (ix=0; ix < PS_NUM+1; ix++) {
                pipe(pipeswt[ix]);
                pipe(pipesrd[ix]);
            }

            for (ix=0; ix < PS_NUM; ix++) {
                pids[ix] = fork();
                if (pids[ix] == 0) {
                    break;
                }
            }

            for (ix=0; ix< PS_NUM; ix++) {
                printf("%d. %d \n", ix, pids[ix]);

                if (pids[ix] == 0) {
                    pmslf = ix+1;
                    break;
                }
            }

            if (ix == PS_NUM) {
                pmslf = 0;
            }

            printf("[P%d] init \n", pmslf);

            if (pmslf == 0) {
                for (ix=1; ix < PS_NUM+1; ix++) {
                    mret = mipc_read(pipesrd[ix], 1, pinfo);

                    infolen = strlen(pinfo);
                    pinfo[infolen] = '\0';
                    printf("[P%d] %d. get [%s] info from slave infolen: %d \n", pmslf, ix, pinfo, infolen);
                }

                sleep(5);
            } 
            else if (pmslf > 0) {
                sprintf(pinfo, "%d", pmslf);
                mret = mipc_write(pipesrd[pmslf], 1, pinfo);

                infolen = strlen(pinfo);
                pinfo[infolen] = '\0';
                printf("[P%d] send [%s] info to master infolen: %d \n", pmslf, pinfo, infolen);

                
                mret = mipc_read(pipeswt[pmslf], 1, pinfo);

                infolen = strlen(pinfo);
                pinfo[infolen] = '\0';
                printf("[P%d] get [%s] from master infolen: %d\n", pmslf, pinfo, infolen);
            } 
            else {
                printf("[P%d] Error!!!  \n", pmslf);
                goto end;
            }

            clock_gettime(CLOCK_REALTIME, &jpgS);
                                        
            jret = jpeg2rgbm(buffjpg, jpgsz, buffraw, rawsz, &decw, &dech, 8);
            if (jret) {
                free(buffjpg);
                free(buffraw);
                err = -4;
                goto end;
            }

            clock_gettime(CLOCK_REALTIME, &jpgE);
            tmCost = time_diffm(&jpgS, &jpgE, 1000);                                                
            printf("[P%d] doing the JPG decoder succeed ret: %d w: %d h: %d cost: %d.%d ms\n", pmslf, err, decw, dech, tmCost/1000, tmCost%1000);
            
            len = cropinfo[2]*cropinfo[3];
            rotraw = malloc(len);
            if (!rotraw) {
                err = -5;
                goto end;
            }
            
            //printf("get file [%s] !! \n", filepath);
            memset(rotraw, 0xff, len);

            ph = (struct bitmapHeader_s *)(buffhead + 2);
            bitmapHeaderSetupRvs(ph, 8, decw, dech, 200, rawsz);
            pclortable = buffhead + 2 + sizeof(struct bitmapHeader_s);
            bitmapColorTableSetupd(pclortable);

            memcpy(rothead, buffhead + 2, 1078);
            
            clock_gettime(CLOCK_REALTIME, &jpgS);
            
            ret = doRot2BMP(rotraw, rothead, cropinfo, buffhead+4, treal);
            if (ret) {
                err = -6 + ret*10;
                goto end;
            }
            
            clock_gettime(CLOCK_REALTIME, &jpgE);
            tmCost = time_diffm(&jpgS, &jpgE, 1000);                                                
            printf("[P%d] rotate ret: %d w: %d h: %d cost: %d.%d ms\n", pmslf, ret, cropinfo[2], cropinfo[3], tmCost/1000, tmCost%1000);
            
            #if DUMP_ROT_BMP
            ret = saveRot2BMP(rotraw, rothead, ntd, cropinfo, cnt);
            if (ret) {
                err = -7 + ret*10;
                goto end;
            }
            #endif

            if (pmslf == 0) {
                for (ix=1; ix < PS_NUM+1; ix++) {
                    mret = mipc_write(pipeswt[ix], 1, "s");
                }
            }
            
            free(buffjpg);
            buffjpg = 0;

            free(buffhead);
            buffhead = 0;

            free(rotraw);
            rotraw = 0;
        }

        goto end;
    }

    for (nt=0; nt < 3; nt++) {
        ntd = data[nt][0];
        memcpy(cropinfo, &data[nt][1], sizeof(int) * 4);
        cropinfo[4] = 0;
        cropinfo[5] = 0;
        
        printf("[NTD_%.4d]: %.3d, %.3d, %.3d, %.3d, %.3d, %.3d \n", ntd, cropinfo[0], cropinfo[1], cropinfo[2], cropinfo[3], cropinfo[4], cropinfo[5]);

        len = cropinfo[2]*cropinfo[3];
        rotraw = malloc(len);
        if (!rotraw) {
            err = -5;
            goto end;
        }
        
        ix=100000;
        
        for (cnt=1; cnt < 12; cnt++) {
            //printf("cnt: %d \n", cnt);
            for (; ix < 250000; ix++) {
                sprintf(filepath, bnotefile, ntd, cnt, ix);
                //if ((ix%100000) == 0) printf("[%s] ix: %d \n", filepath, ix);
                f = fopen(filepath, "r");
                if (f) break;
            }
            
            if (!f) {
                printf("get file [%s] failed!! \n", filepath);
                break;
            } else {
                printf("get file [%s] !! \n", filepath);
                memset(rotraw, 0xff, len);

                //ret = doRot2BMP(rotraw, rothead, cropinfo, f);
                if (ret) {
                    err = -6 + ret*10;
                    goto end;
                }

                #if DUMP_ROT_BMP
                ret = saveRot2BMP(rotraw, rothead, ntd, cropinfo, cnt);
                if (ret) {
                    err = -7 + ret*10;
                    goto end;
                }
                #endif
            }

            fclose(f);
            f = 0;
        }

        free(rotraw);
        rotraw = 0;
        
    }   

    end:

    printf("end err: %d \n", err);

    if (rotraw) free(rotraw);
    if (rothead) free(rothead);
    if (f) fclose(f);
    
    return err;
    
}


