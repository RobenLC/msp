
#include <stdint.h> 
#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>

char ccarray[28] = {'a', 'b', 'c', 'd', 'e',      // 0  4
                                'f', 'g', 'h', 'i', 'j',        // 5  9
                                'k', 'l', 'm', 'n', 'o',      // 10 14
                                'p', 'q', 'r', 's', 't',       //  15 19
                                'u', 'v', 'w', 'x', 'y',     // 20  24
                                'z', '_', '.'};             // 25 27
                                
int iiarrya[16] = {0x1f, 0x2e, 0x3d, 0x4c, 0x5b, 
                           0x6a, 0x79, 0x88, 0x97, 0xa6,
                           0xb5, 0xc4, 0xd3, 0xe2, 0xf1, 0x00};

static int ch2stridx(char *ccarry, char ch, int size) 
{
    int ix=0, rt=-1;
    char tc=0;

    while (ix < size) {
         tc = ccarry[ix];
         if (tc == ch) {
             rt = ix;
             break;
         }

         ix++;
    }

    return rt;
}

static int ccarr2str(char *str, char *ccarry, int *arry, int size)
{
    int cnt=0, id=0;

    while (cnt < size) {
        id = arry[cnt];
        str[cnt] = ccarry[id];

        cnt++;
    }
    
    return cnt;
}

static int iiarr2num(int id, int *arry, int ndec) 
{
    int val=-1, ix=0, sum=0, iu=0, dec=1;
    int tran[17] = {-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

    if (id < 0) return -2;
    if (id > 15) return -3;
    if (!arry) return -4;

    iu = 16 - id;
    ix = tran[id];
    
    //printf("iu: %d ix: %d \n", iu, ix);
    
    if (ix < 0) {
        val = arry[iu] & 0x0f;
    } else {
        sum = (arry[ix] + arry[iu-1]) & 0xf;
        
        //printf("sum: %d arry[ix]: 0x%.2x arry[iu-1]: 0x%.2x \n", sum, arry[ix], arry[iu-1]);
        
        if (!sum) {
            val = arry[iu - 1] & 0x0f;
        }
    }

    while (ndec) {
        dec = dec * tran[11]; 
        ndec --;
    }

    //printf("val: %d dec: %d \n", val, dec);

    return (val * dec);
}

int n123(int n1, int n2, int n3) 
{
    int num = 0;

    num += iiarr2num(n1, iiarrya, 2);
    num += iiarr2num(n2, iiarrya, 1);
    num += iiarr2num(n3, iiarrya, 0);

    return num;
}

int printfpath(void)
{
    char expth[32];
    char path[32];// = "/tmp/m.bin";
    char slash = '0' - 1;
    char dot = slash - 1;
    int ida[32], ret=0, ic=0, is=0;

    memset(expth, 0x0, 32);
    memset(path, 0x0, 32);    
    is = 0;
    expth[is] = slash;
    is++;
    
    ic = 0;
    ret = ch2stridx(ccarray, 't', 26);
    if (ret > 0) {
        ida[ic] = ret;
    } else {
        ida[ic] = 26;
    }

    ic++;
    ret = ch2stridx(ccarray, 'm', 26);
    if (ret > 0) {
        ida[ic] = ret;
    } else {
        ida[ic] = 26;
    }
    
    ic++;
    ret = ch2stridx(ccarray, 'p', 26);
    if (ret > 0) {
        ida[ic] = ret;
    } else {
        ida[ic] = 26;
    }

    ccarr2str(&expth[is], ccarray, ida, 3);

    is = is+ic+1;
    expth[is] = slash;
    is++;

    ic = 0;
    ret = ch2stridx(ccarray, 'm', 26);
    if (ret > 0) {
        ida[ic] = ret;
    } else {
        ida[ic] = 26;
    }

    ic++;
    ret = ch2stridx(ccarray, '.', 28);
    if (ret > 0) {
        ida[ic] = ret;
    } else {
        ida[ic] = 26;
    }

    ic++;
    ret = ch2stridx(ccarray, 'b', 26);
    if (ret > 0) {
        ida[ic] = ret;
    } else {
        ida[ic] = 26;
    }

    ic++;
    ret = ch2stridx(ccarray, 'i', 26);
    if (ret > 0) {
        ida[ic] = ret;
    } else {
        ida[ic] = 26;
    }

    ic++;
    ret = ch2stridx(ccarray, 'n', 26);
    if (ret > 0) {
        ida[ic] = ret;
    } else {
        ida[ic] = 26;
    }

    ccarr2str(&expth[is], ccarray, ida, 5);
    is = is + ic + 1;
    
    expth[is] = '\0';

    printf("expth:[%s] path:[%s] ic:%d is:%d\n", expth, path, ic, is);

    return is;
}

static int doSysCmd(char *sCommand)
{
#define BIGBUFLEN 2048
#define BUFLEN 128

    int ret=0, wct=0, n=0, totlog=0;
    FILE *fpRead = 0;
    char retBuff[BUFLEN], *pch=0, *p0=0;
    char bigBuff[BIGBUFLEN], *pBig=0;

    memset(bigBuff, 0, BIGBUFLEN);
    memset(retBuff, 0, BUFLEN);

    printf("doSystemCmd() [%s]\n", sCommand);
    fpRead = popen(sCommand, "r");
    //sleep(1);
    
    if (!fpRead) return -1;

    //pBig = bigBuff;
    pch = fgets(retBuff, BUFLEN , fpRead);
    while (pch) {
    
        if (pch) {
            //printf("sCommand: \n[%s] - 1\n\n", retBuff);
            //p0 = strstr(pch, "\n");
            //if (p0) *p0 = '\0';

            //n = strlen(pch);
            //totlog += n;
            //if (totlog > BIGBUFLEN) break;

            //strncpy(pBig, pch, n);
            //pBig += n;
            
            //printf("sCommand: [%s] \n", pch);
            //shmem_dump(retBuff, BUFLEN);
        } else {
            wct++;
            printf("sCommand: %d...\n", wct);
            if (wct > 3) {
                //break;
            }
        }
        
        //sleep(1);
        //memset(retBuff, 0, BUFLEN);

        pch = fgets(retBuff, BUFLEN , fpRead);
    }

    printf("scmd: [%s] \n", bigBuff);
            
    pclose(fpRead);

    return 0;
}

static inline int gethexint(char ch)
{
    int lw = -1;
    switch(ch) {
    case '0': 
        lw = 0;
        break; 
    case '1': 
        lw = 1;
        break;
    case '2': 
        lw = 2;
        break;
    case '3': 
        lw = 3;
        break;
    case '4': 
        lw = 4;
        break;
    case '5': 
        lw = 5;
        break;
    case '6': 
        lw = 6;
        break;
    case '7': 
        lw = 7;
        break;
    case '8': 
        lw = 8;
        break;
    case '9': 
        lw = 9;
        break;
    case 'A': 
        lw = 0xa;
        break;
    case 'B': 
        lw = 0xb;
        break;
    case 'C': 
        lw = 0xc;
        break;
    case 'D': 
        lw = 0xd;
        break;
    case 'E': 
        lw = 0xe;
        break;
    case 'F': 
        lw = 0xf;
    default:
        break;
    }

    return lw;
}

static int hex2bin(char *dst, char *src, int size)
{
    char hexmap[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    char ch=0, lw=0, hi=0;
    int i=0, sz=0;
    if (!dst) return -1;
    if (!src) return -2;
    if (!size) return -3;
    //printf("[B2H] start size: %d\n", size);

    sz = size / 2;
    for (i=0; i < sz; i++) {
        ch = src[i*2+1];

        lw = gethexint(ch);

        ch = src[i*2+0];

        hi = gethexint(ch);

        if ((lw < 0) || (hi < 0)) return -1;
        dst[i] = (lw & 0xf) | ((hi & 0xf) << 4);
                
        //printf("[b2h] %d. 0x%.2x, [%c-%c] \n", i, ch, dst[i*2+0], dst[i*2+1]);
    }
    //printf("[B2H] end idx: %d \n", i);

    return 0;
}

static int bin2hex(char *dst, char *src, int size)
{
    char hexmap[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    char ch=0;
    int i=0, lw, hi;
    if (!dst) return -1;
    if (!src) return -2;
    if (!size) return -3;
    //printf("[B2H] start size: %d\n", size);
    
    for (i=0; i < size; i++) {
        ch = src[i];
        lw = ch & 0xf;
        hi = ch >> 4;
        
        dst[i*2+1] = hexmap[lw];
        dst[i*2+0] = hexmap[hi];

        //printf("[b2h] %d. 0x%.2x, [%c-%c] \n", i, ch, dst[i*2+0], dst[i*2+1]);
    }
    //printf("[B2H] end idx: %d \n", i);

    return 0;
}

int filehex2bin( char *binfile, char *hexfile)
{
    FILE *fb=0, *fx=0;
    int ret=0, hexlen=0;
    char *ptbin=0, *pthex=0;

    fx = fopen(hexfile, "r");

    ret = fseek(fx, 0, SEEK_END);
    if (ret) {
        printf(" file seek failed!! ret:%d \n", ret);
        return -1;
    } 

    hexlen = ftell(fx);
    printf(" file [%s] size: %d \n", hexfile, hexlen);

    ret = fseek(fx, 0, SEEK_SET);
    if (ret) {
        printf(" file seek failed!! ret:%d \n", ret);
        return -2;
    }

    pthex = malloc(hexlen);
    if (!pthex) {
        printf(" pthex malloc failed ret: %d size:%d \n", pthex, hexlen);
        return -3;
    }

    ret = fread(pthex, 1, hexlen, fx);
    if (ret != hexlen) {
        printf(" read bin failed ret: %d size: %d\n", ret, hexlen);

        free(pthex);
        return -4;
    }
    
    fclose(fx);
    
    ptbin = malloc(hexlen/2);
    if (!ptbin) {
        printf(" ptbin malloc failed ret: %d size: %d\n", ptbin, hexlen/2);

        free(pthex);
        return -5;
    }
    
    ret = hex2bin(ptbin, pthex, hexlen);

    fb = fopen(binfile, "w");
    if (fb) {
        fwrite(ptbin, 1, hexlen/2, fb);
        fflush(fb);
        fclose(fb);
        printf("filehex2bin save to [%s] size:%d\n", binfile, hexlen/2);
    } else {
        printf("filehex2bin open [%s] failed !!!\n", binfile);
    }

    free(ptbin);
    free(pthex);
    
    return 0;
}

int filebin2hex(char *hexfile, char *binfile)
{
    FILE *fb=0, *fx=0;
    int ret=0, binlen=0;
    char *ptbin=0, *pthex=0;

    fb = fopen(binfile, "r");

    ret = fseek(fb, 0, SEEK_END);
    if (ret) {
        printf(" file seek failed!! ret:%d \n", ret);
        return -1;
    } 

    binlen = ftell(fb);
    printf(" file [%s] size: %d \n", binfile, binlen);

    ret = fseek(fb, 0, SEEK_SET);
    if (ret) {
        printf(" file seek failed!! ret:%d \n", ret);
        return -2;
    }

    ptbin = malloc(binlen);
    if (!ptbin) {
        printf(" ptbin malloc failed ret: %d size:%d \n", ptbin, binlen);
        return -3;
    }

    ret = fread(ptbin, 1, binlen, fb);
    if (ret != binlen) {
        printf(" read bin failed ret: %d size: %d\n", ret, binlen);

        free(ptbin);
        return -4;
    }
    
    fclose(fb);
    
    pthex = malloc(binlen*2);
    if (!pthex) {
        printf(" pthex malloc failed ret: %d size: %d\n", pthex, binlen);

        free(ptbin);
        return -5;
    }
    
    ret = bin2hex(pthex, ptbin, binlen);

    fx = fopen(hexfile, "w");
    if (fx) {
        fwrite(pthex, 1, binlen*2, fx);
        fflush(fx);
        fclose(fx);
        printf("bin2hex save to [%s] size:%d\n", hexfile, binlen*2);
    } else {
        printf("bin2hex open [%s] failed !!!\n", hexfile);
    }

    free(ptbin);
    free(pthex);
    
    return 0;
}

int main(int argc, char *argv[]) 
{
    int vcnt=0, value=0, n=0;
    int ret=0;
    char syscmd[128];
    char filename[128];
    char hexname[64] = "/root/m.hex";
    char binname[64] = "/root/m.bin";
    char *recgv[10];

    printf("hello argc: %d \n", argc);
    
    vcnt = argc - 1;
    if (vcnt > 10) vcnt = 10;

    memset(recgv, 0, sizeof(char *) * 10);
    while (vcnt) {
        //printf("input[%d]: %s\n", vcnt, argv[vcnt]);
        recgv[vcnt-1] = argv[vcnt];
        
        vcnt--;
    }

    for (n=0; n < 10; n++) {
        if (recgv[n]) {
            printf("[%d]: %s \n", n, recgv[n]);
        }
    }

    if ((recgv[0]) && (recgv[1])) {
        if (strcmp(recgv[0], "bin2hex") == 0) {
            memset(filename, 0, 128);

            sprintf(filename, "%s", recgv[1]);

            printf("bin2hex filename: [%s] \n", filename);

            ret = filebin2hex(hexname, filename);

            printf("filebin2hex ret: %d \n", ret);

            ret = filehex2bin(binname, hexname);

            printf("filehex2bin ret: %d \n", ret);            
        }
    }

    printf("n123: %d \n", n123(1, 2, 3));
    
    printfpath();


    sprintf(syscmd, "cp /root/mothership.bin /tmp/m.bin");
    doSysCmd(syscmd);
    
    #if 0
    sprintf(syscmd, "/tmp/m.bin 3 2>&1");
    doSysCmd(syscmd);
    #endif
    
    return 0;
}

