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

int sockserver(void)
{
    int listenfd = 0, connfd = 0, n = 0, i = 0;
    struct sockaddr_in serv_addr; 
		FILE *fp= NULL;

    char sendBuff[2048];
    char recvBuff[1025];
    time_t ticks; 

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
        write(connfd, sendBuff, strlen(sendBuff)); 
				fprintf(fp, " %s\n ", sendBuff);
				
        close(connfd);
        sleep(1);
        
				//Dont block context switches, let the process sleep for some time
				fprintf(fp, "Logging info...\n");
				fflush(fp);
				// Implement and call some function that does core work for this daemon.

    }
    fclose(fp);

}

int main(int argc, char* argv[])
{
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
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		
		sockserver();
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
		return (0);
}
