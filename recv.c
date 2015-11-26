/* msg_recv() function waits for 3 seconds to receive data and then returns -1 */

#include "unp.h"
#include "a3.h"
#define TIMEOUT 3 /*in seconds*/

int pipefd[2];

void recvfrom_alarm(int signo);

int msg_recv(int sockfd, char* msg, char* source_ip, int* source_port)
{
       
	char buff[MAXLINE+1];
	int bytes_read;
	char ipaddr[16], message[20];
	int maxfd, n;
	fd_set rset;

	Pipe(pipefd);
	maxfd = max(sockfd,pipefd[0]) + 1;

	FD_ZERO(&rset);
	
	alarm(TIMEOUT);		/*Set the alarm*/
	
	for(;;) {

		FD_SET(pipefd[0], &rset);
		FD_SET(sockfd, &rset);

		if( (n = select(maxfd, &rset, NULL, NULL, NULL)) <0 ){
			if(errno == EINTR)
				continue;
		else
			err_sys("\nselect error\n");
		}

		if(FD_ISSET(sockfd, &rset)){

			/* Read line */
			bytes_read = Read(sockfd, msg, sizeof(buff));
			
			alarm(0);	/* Reset Alarm */
	
			if( bytes_read < 0 )
				printf("msg_recv() error");

			return bytes_read;

		}
		
		if(FD_ISSET(pipefd[0], &rset)){
			
			Read(pipefd[0], &n, 1);		/*timer expired*/
			return -1;
		}
			
	}
}

void recvfrom_alarm(int signo){
	
  Write(pipefd[1],"",1);	/*Write one null byte data to pipe*/
  return;
  
}
