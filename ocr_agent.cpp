


#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string>       // std::string
#include <sstream>      // std::stringstream


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <semaphore.h>
#include "bkjob.h"


extern "C" {
#include "log.h"
}

void ocr_log_i(const char* fmt, ...);
using namespace std;

int bkocr_up( void * img_buf, int img_size, char *out_buf, int o_size, void *p_model );
int bkocr_dw( void * img_buf, int img_size, char *out_buf, int o_size, void *p_model );
void *bkocr_load_model( int model_type );

//*****************************************************************************
//
//*****************************************************************************
//
extern "C" void *BKOCR_Check( void * img_buf, int img_size, char *out_buf, int buf_size )
{
    static void *p_model=NULL;

    if( p_model == NULL )
        p_model = bkocr_load_model( 0 );
    if( p_model == NULL )
    {
        ocr_log_i( "Load OCR model fail !!!\n" );
        return 0;
    }
    bkocr_up( img_buf, img_size, out_buf, buf_size, p_model );
    return 0;
}

//*****************************************************************************
//  BKOCR_Check6 will load upA_model_hv.txt ( vertical flip )
//*****************************************************************************
extern "C" void *BKOCR_Check6( void * img_buf, int img_size, char *out_buf, int buf_size )
{
    static void *p_model=NULL;

    if( p_model == NULL )
        p_model = bkocr_load_model( 0 );

    if( p_model == NULL )
    {
        ocr_log_i( "Load OCR model fail !!!\n" );
        return 0;
    }
    bkocr_up( img_buf, img_size, out_buf, buf_size, p_model );
    return 0;
}

//*****************************************************************************
//  BKOCR_Check7 will load dwA_model_hv.txt ( horizontal flip )
//*****************************************************************************
extern "C" void *BKOCR_Check7( void * img_buf, int img_size, char *out_buf, int buf_size )
{
    static void *p_model=NULL;

    if( p_model == NULL )
        p_model = bkocr_load_model( 1 );

    if( p_model == NULL )
    {
        ocr_log_i( "Load OCR model fail !!!\n" );
        return 0;
    }
    bkocr_dw( img_buf, img_size, out_buf, buf_size, p_model );
    return 0;

}

//*****************************************************************************
//
//*****************************************************************************

extern "C" void *ocr_agent_start( void * arg )
{
    int ret;
    int i1;
    char *fname;
    int fh;
    t_chan_param *param = (t_chan_param *)arg;
    char ocr_result[20];
    int out_fh=0;
    static void *p_model=NULL;

    ocr_log_i( "+%s\n", __func__ );
    if( p_model == NULL )
        p_model = bkocr_load_model( 0 );

    for( i1=1; i1< param->argc; i1++ )
    {
        fname = (char*)param->argv[i1];

        ocr_log_i( "  %s\n", fname );
        fh = open( fname, O_RDONLY );
        if( fh < 0 )
        {
            ocr_log_i( "open file error %s errno=%d\n",fname, errno);
            continue;
        }

        struct stat st;
        fstat(fh, &st);
        int img_size = st.st_size;

        do
        {
            void *img_buf = malloc(img_size);
            if( img_buf==NULL )
                break;
            int read_size = read( fh, img_buf, img_size );

            bkocr_up( img_buf, img_size,ocr_result, 20, p_model);
            free( img_buf );
//            log_i( "input file = %s size = %d result=%s\n", param->argv[i1], read_size, ocr_result );
//            log_dump( "output=", ocr_result, sizeof(ocr_result));

            if( out_fh==0 )
            {
                out_fh = open( "bmpocr_out.txt", O_WRONLY | O_CREAT, 0644);
            }
            write( out_fh, ocr_result, strlen(ocr_result));

        } while(0);
        close( fh );
    }
    close( out_fh );
    ocr_log_i( "-%s\n", __func__ );
    return 0;
}


void ocr_log_i(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

//*****************************************************************************
//
//*****************************************************************************

