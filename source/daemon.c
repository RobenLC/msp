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

#define TSIZE (128*1024*1024)
#define TCP_WIN_SZ  16777216

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

}
int sockserver(char *pBuf, int vsize, int pipefd1)
{
    int listenfd = 0, connfd = 0, n = 0, i = 0;
    struct sockaddr_in serv_addr; 
    FILE *fp=NULL;
    int sz;
    char *pb;

    char sendBuff[2048];
    char recvBuff[1025];
    time_t ticks; 

    struct timespec curtime;
    unsigned long long cur, tnow, lnow, past, tbef, lpast;

    clock_gettime(CLOCK_REALTIME, &curtime);
    printf("time measurement %llu, %llu \n", curtime.tv_sec, curtime.tv_nsec);

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
		fp = fopen ("Log.txt", "w+");

    while(1)
    {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL); 
				n = read(connfd, recvBuff, 1024);
				fprintf(fp, "\n Receive %d char \n", n);
				fprintf(fp, " start len: %d\n \"", n);
				for (i = 0; i < n; i++) {
						if (i < 1024)
								fprintf(fp, "%c", recvBuff[i]);
				}
				
				fprintf(fp, "\"\n end\n\n");
				recvBuff[i+1] = '\0';
				
        ticks = time(NULL);
        snprintf(sendBuff, sizeof(sendBuff), "%.24s+ \"%s\" \r\n", ctime(&ticks), recvBuff);
	printf("recv: %s\n", recvBuff);

    clock_gettime(CLOCK_REALTIME, &curtime);
    cur = curtime.tv_sec;
    tnow = curtime.tv_nsec;
    lnow = cur * 1000000000+tnow;
    printf("[p0] enter %d t:%llu %llu %llu\n", sz, cur,tnow, lnow/1000000);
#if 0
    write(connfd, pBuf, vsize);
#else
	while(vsize) {
            //printf("remain %d bytes \r", vsize);

            if (TCP_WIN_SZ > vsize) {
               write(connfd, pBuf, vsize);
               write(pipefd1, "e", 1); // send the content of argv[1] to the reader
		 break;	   
            } else {
               write(connfd, pBuf, TCP_WIN_SZ);
               write(pipefd1, "c", 1);
            }

           vsize -= TCP_WIN_SZ;
           pBuf += TCP_WIN_SZ;
       }
#endif
       //restore
       vsize = sz;
       pBuf = pb;

    clock_gettime(CLOCK_REALTIME, &curtime);
    past = curtime.tv_sec;
    tbef = curtime.tv_nsec;		
    lpast = past * 1000000000+tbef;	
    
    printf("time cost: %llu s, bandwidth: %llu Mbits/s \n", (lpast - lnow)/1000000000, ((vsize*8)/((lpast - lnow)/1000000000)) /1000000 );
	

	//printf("\n%d bytes tx finished\n", vsize);
	
	fprintf(fp, " %s\n ", sendBuff);
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

int main(int argc, char* argv[])
{
             int i = 0, sz;
             char *fpath;
             printf("argc:%d ", argc);
             while(i < argc) {
                 if (argv[i])
                     printf("[%d]:%s ", i, argv[i]);
                 i++;
             }

             if (argc > 1) {
                 //printf("[%d]:%s\n", 1, argv[1]);
                 fpath = argv[1];
             }



		pid_t process_id = 0;
		pid_t sid = 0;
		// Create child process
		process_id = fork();
		// Indication of fork() failure
		if (process_id < 0)
		{
				printf("fork failed!\n");
				// Return failure in exit status
				exit(1);
		}
		// PARENT PROCESS. Need to kill it.
		if (process_id > 0)
		{
				printf("process_id of child process %d \n", process_id);
				// return success in exit status
				exit(0);
		}
              int pipefd[2];
              pipe(pipefd);

		// Create child's child process
		process_id = fork();
		if (process_id < 0)
		{
				printf("fork failed!\n");
				// Return failure in exit status
				exit(1);
		}
		if (process_id == 0) {
			printf("monitor process \n");
			close(pipefd[1]); // close the write-end of the pipe, I'm not going to use it
			monitor(pipefd[0]);
			close(pipefd[0]); // close the read-end of the pipe
                     exit(1);
		}

		printf("socketserver process \n");
		close(pipefd[0]); // close the read-end of the pipe, I'm not going to use it
		//unmask the file mode
		umask(0);
		//set new session
		sid = setsid();
		if(sid < 0)
		{
				// Return failure
				exit(1);
		}
		// Change the current working directory to root.
		chdir("/mnt/mmc2");
		// Close stdin. stdout and stderr
		//close(STDIN_FILENO);
		//close(STDOUT_FILENO);
		//close(STDERR_FILENO);

		char *srcBuff = NULL;
		int buffsize;
			
		buffsize = TSIZE;
              srcBuff = malloc(buffsize);
              if (srcBuff) {
                  printf(" buff alloc success!!\n");
              } else {
                  printf(" buff alloc failed!!\n");
                  goto end;
              }
			  
              sz = find_open(fpath, srcBuff);
              if (sz < 0) {printf("file open failed ret:%d \n", sz); goto end;}

		sockserver(srcBuff, sz, pipefd[1]);
		// Open a log file in write mode.
		//fp = fopen ("Log.txt", "w+");
		//while (1)
		//{
		//		//Dont block context switches, let the process sleep for some time
		//		sleep(1);
		//		fprintf(fp, "Logging info...\n");
		//		fflush(fp);
		//		// Implement and call some function that does core work for this daemon.
		//}
		//fclose(fp);
		close(pipefd[1]); // close the write-end of the pipe, thus sending EOF to the reader
		end:

		free(srcBuff);
              printf("daemonfs end \n");
		exit(1);
		return (0);
}
