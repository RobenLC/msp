#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 

int main(int argc, char *argv[])
{
    int sockfd = 0, n = 0, i = 0;
    char recvBuff[1024];
    char sendBuff[1024];
    char *pi = 0;
    struct sockaddr_in serv_addr; 

    if(argc < 2)
    {
        printf("\n Usage: %s <ip of server> \n",argv[0]);
        return 1;
    }
    
    if (argc > 2) {
    		pi = argv[2];
    }
		
		printf(" start: \n ");
		if (pi) {
				i = 0;
				while (*pi != '\0'){
						sendBuff[i] = *pi;
						printf("%c", sendBuff[i]);
						pi++; i++;
				}
		}
		else {
				for (i = 0; i < 1024; i++) {
						sendBuff[i] = 0x41;
						printf("%c", sendBuff[i]);
				}
		}
		printf("\n end");
		
    memset(recvBuff, '0',sizeof(recvBuff));
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    } 

    memset(&serv_addr, '0', sizeof(serv_addr)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000); 

    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    } 

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\n Error : Connect Failed \n");
       return 1;
    } 

		n = write(sockfd, sendBuff, i-1);
		printf("\n Send %d %c to server \n", n, 0x41);

    while ( (n = read(sockfd, recvBuff, sizeof(recvBuff)-1)) > 0)
    {
        recvBuff[n] = 0;
        if(fputs(recvBuff, stdout) == EOF)
        {
            printf("\n Error : Fputs error\n");
        }
    } 

    if(n < 0)
    {
        printf("\n Read error \n");
    } 

    return 0;
}