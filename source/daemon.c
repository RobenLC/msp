#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h> 
#include <sys/mman.h> 
#include <sys/times.h> 

#define TSIZE (128*1024*1024)
#define TCP_WIN_SZ  16777216
#if 1
int find_open(char *path, char *srcBuff)
{

    FILE *f=NULL;
    int fsize;

    if (!path) {printf("path error! \n"); return (-1);}
    if (!srcBuff) {printf("path error! \n"); return (-2);}

    f = fopen(path, "r");
    if (!f) {
        printf("open [%s] failed ret:%d\n", path, f);
        return (-3);
    } else
        printf("open [%s] succeed ret:%d\n", path, f);
	
    fsize = fread(srcBuff, 1, TSIZE, f);
    printf(" [%s] size: %d, read to memory\n", path, fsize);

    fclose(f);

    return fsize;
}

int monitor(int pipefd0) 
{
	char buf;
	int csize;

	csize = 0;
	/*
	while(1) {
		read(pipefd0, &buf, 1); 
		if (buf == 'c') {
			printf("tx size: %d \n", csize);
			csize+= TCP_WIN_SZ;
		} else if (buf == 'e') {
			printf("\ntotal tx size: %d \n", csize);
			csize = 0;
		}
	}
       */
    int listenfd = 0, connfd = 0, n = 0, i = 0;
    struct sockaddr_in serv_addr; 
    FILE *fp=NULL;

    char sendBuff[2048];
    char *recvBuff;
    time_t ticks; 

    recvBuff = malloc(1024*1024);
    if (!recvBuff) return (-1);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000); 

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

    listen(listenfd, 10); 

		// Open a log file in write mode.
    fp = fopen ("Log0.txt", "w+");

    while(1)
    {
        printf("-\n");
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL); 
        n = 1;
        while (n > 0) {

        n = read(connfd, recvBuff, 1024);
        fprintf(fp, "\n Receive %d char \n", n);
        fprintf(fp, " start len: %d\n \"", n);
        for (i = 0; i < n; i++) {
            if (i < 1024)
                fprintf(fp, "%c", recvBuff[i]);
        }
				
        fprintf(fp, "\"\n end\n\n");
        recvBuff[n] = '\0';

        printf("0 recv %d bytes [%s]\n", n, recvBuff);
        
        }

        write(connfd, recvBuff, n);

//        fprintf(fp, " %s\n ", sendBuff);
        close(connfd);
        sleep(1);
        
        //Dont block context switches, let the process sleep for some time
        fprintf(fp, "Logging info...\n");
        fflush(fp);
        // Implement and call some function that does core work for this daemon.

    }
end:
    fclose(fp);
}
#if 0
int sockserver(char *pBuf, int vsize, int pipefd1)
{
    int listenfd = 0, connfd = 0, n = 0, i = 0;
    struct sockaddr_in serv_addr; 
    FILE *fp=NULL;
    int sz;
    char *pb;

    char sendBuff[2048];
    char *recvBuff;
    time_t ticks; 

    recvBuff = malloc(1024*1024);
    if (!recvBuff) return (-1);

    sz = vsize;
    pb = pBuf;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000); 

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

    listen(listenfd, 10); 

		// Open a log file in write mode.
	fp = fopen ("/root/Log.txt", "w+");
    if (!fp) printf("open log failed \n");
    else printf("open log ok %d\n", fp);

    while(1)
    {
        printf("+\n");
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL); 

        n = read(connfd, recvBuff, 1024);
        fprintf(fp, "\n Receive %d char \n", n);
        fprintf(fp, " start len: %d\n \"", n);
        for (i = 0; i < n; i++) {
            if (i < 1024)
                fprintf(fp, "%x", recvBuff[i]);
        }
				
        fprintf(fp, "\"\n end\n\n");
        recvBuff[n] = '\0';

        printf("1 recv [%s] \n", n);

        close(connfd);
        sleep(1);
        
        //Dont block context switches, let the process sleep for some time
        fprintf(fp, "Logging info...\n");
        fflush(fp);
        // Implement and call some function that does core work for this daemon.

    }
err:
    fclose(fp);
}
#endif
#endif
int main(int argc, char* argv[])
{
    int uid;

    uid = fork();
    if (uid == 0) {
        monitor(uid);
        exit(1);
    }
    int i = 0, sz, acusz;
    char *fpath;

	struct tms time;
	struct timespec curtime;
	unsigned long long cur, tnow, lnow, past, tbef, lpast;


    int listenfd = 0, connfd = 0, n = 0;
    struct sockaddr_in serv_addr; 
    FILE *fp=NULL;

    char *recvBuff, *save, *tmps;
    time_t ticks; 

    recvBuff = malloc(1024*1024);
    if (!recvBuff) return (-1);

    save = mmap(NULL, 64*1024*1024, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    if (!save) return (-1);
    tmps = save;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(6000); 

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 
    listen(listenfd, 10); 

    // Open a log file in write mode.
    fp = fopen ("/mnt/mmc2/recvfromApp.bin", "w");
    if (!fp) {
        printf("open log failed \n");
        return (-2);
    }
    else {
        printf("open log ok %d\n", fp);
    }


    while(1)
    {
        printf("+\n");
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL); 

		clock_gettime(CLOCK_REALTIME, &curtime);
		cur = curtime.tv_sec;
		tnow = curtime.tv_nsec;
		lnow = cur * 1000000000+tnow;
		printf("[p0] enter %d t:%llu %llu %llu\n", acusz,cur,tnow, lnow/1000000);

        save = tmps;
        acusz = 0;
        n = 1;
        while (n > 0) {
            n = read(connfd, recvBuff, 61440);
            if (n < 61440) printf("recv %d\n", n);

            memcpy(save, recvBuff, n);
            save += n;
            acusz += n;

            //printf("recv %d %d\n", n, acusz);
        }

	clock_gettime(CLOCK_REALTIME, &curtime);
	past = curtime.tv_sec;
	tbef = curtime.tv_nsec;		
	lpast = past * 1000000000+tbef;	

	printf("time cose: %llu s, bandwidth: %llu Mbits/s \n", (lpast - lnow)/1000000000, ((acusz*8)/((lpast - lnow)/1000000000)) /1000000 );
	
        //Dont block context switches, let the process sleep for some time
        msync(tmps, acusz, MS_SYNC);
		
        printf("write file %d size: %d\n", fp, acusz);
		
        fwrite(tmps, 1, acusz, fp);
        fflush(fp);
        // Implement and call some function that does core work for this daemon.
        close(connfd);
		
    }

    fclose(fp);
    free(recvBuff);
    munmap(tmps, 64*1024*1024);
    printf("daemonfs end \n");
    exit(1);
    return (0);
}
