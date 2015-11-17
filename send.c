#include "a3.h"

void msg_send(int sockfd, char* dest_ip, int dest_port, char* msg, int flag)
{

	char buff[MAXLINE+1];
	
	/* Format of buff --> destn_ip dest_port flag message */
	strcat(buff,dest_ip);
	strcat(buff," ");
	sprintf(buff,"%d %d ",dest_port,flag);
	strcat(buff,msg);
	
	Send( sockfd, buff, strlen(buff)+1, 0 );
	
	printf("SEND_MSG: %s\n", buff);	

}
