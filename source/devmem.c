#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
//#include <asm/page.h>
#define MODULE_NAME "/dev/mem"
int main(int argc,char **argv)
{
    char *ps8_page_start_addr;
    char *curAddr;
    int fd , r, pageSize;
    int idx=0;
    unsigned int u32_start_addr , u32_len , u32_page_seek_cur , data;
    if( ((argv[1][0] == 'R' || argv[1][0] == 'r') && argc == 4)  || 
        ((argv[1][0] == 'W' || argv[1][0] == 'w') && argc == 5))
    {
        if (argc == 4)
            printf("mem  <%c>  <addr:0x%s>   <%s>  \n", argv[1][0], argv[2], argv[3]);
        else
            printf("mem  <%c>  <addr:0x%s>   <%s:%s>  \n", argv[1][0], argv[2], argv[3], argv[4]);
    } else {
        printf("mem  <R/W>  <start addr>   <len>  <data to write> \n" );
        return 1;
    }
    
    pageSize = getpagesize();
    printf("[R] get page size: %d \n", pageSize);
    
    u32_start_addr = strtoul( argv[2] , NULL , 16 );
    printf("[R] get start addr: 0x%.8x \n", u32_start_addr);

    u32_len = strtoul( argv[3] , NULL , 10 );
    printf("[R] get len: %d \n", u32_len);
    
    u32_page_seek_cur = u32_start_addr % pageSize;
    printf("[R] page seek: %d \n", u32_page_seek_cur);
   
    fd = open(MODULE_NAME , O_RDWR);
  
    
    
    if( fd < 0 ) {
        fprintf( stderr , "open() %s errorn" , MODULE_NAME );
        return 1;
    } else {
        printf("[R] open [%s] succeed!!!! \n", MODULE_NAME);
    }
   
    printf("Start addr : 0x%.8x  , length : %d \n" , u32_start_addr - u32_page_seek_cur , u32_page_seek_cur + u32_len );
    
    sync();
    
    ps8_page_start_addr = mmap( 0 ,                                   // Start addr in file
                           u32_page_seek_cur + u32_len , // len
                           PROT_READ | PROT_WRITE , // mode
                           MAP_SHARED ,                  // flag
                           fd ,
                           u32_start_addr - u32_page_seek_cur ); // start addr in page system
  
    printf("[R] get addr: 0x%.8x \n", ps8_page_start_addr);
    
    if( MAP_FAILED == ps8_page_start_addr ) {
        printf("mmap() errorn" );
        close( fd );      
        return 1;
    }
  
    //while(1);
    
    switch( argv[1][0] ) {
    case 'R':
    case 'r':
        curAddr = ps8_page_start_addr + u32_page_seek_cur;
        for (idx=0; idx < u32_len; idx++) {
            printf("0x%.2x ", curAddr[idx]);
            if ((idx+1)%16 == 0) printf("\n");
        }
        printf("\n");
        break;
    case 'W':
    case 'w': 
        curAddr = ps8_page_start_addr + u32_page_seek_cur;
        data = strtoul( argv[4] , NULL , 16 );
        
        for (idx=0; idx < u32_len; idx++) {
            curAddr[idx] = data;
            printf("[R] 0x%.8x = 0x%.2x \n", &curAddr[idx], data);
        }
        
        break;
    default:
        fprintf( stderr , "Unknown R/W commnd %cn" , argv[1][0] );    
        break;        
    }
  
    munmap( ps8_page_start_addr , u32_len );
  
    close( fd );
  
    return 0;
}