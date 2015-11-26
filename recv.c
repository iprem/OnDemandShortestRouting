

#include "unp.h"
#include "a3.h"

int msg_recv(int sockfd, char* msg, char* source_ip, int* source_port)
{
	
	static int pipefd[2];
	char buff[MAXLINE+1];
	int bytes_read;
	char ipaddr[16], message[20];
	
	
	/* Read line */
	bytes_read = Read(sockfd, msg, sizeof(buff));
	
	
	if( bytes_read < 0 )
	  printf("msg_recv() error");
	
	return bytes_read;
}
