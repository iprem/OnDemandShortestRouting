
#include "unp.h"
#include "a3.h"

msg_recv(int sockfd, char* msg, char* source_ip, int* source_port)
{
	
	char buff[MAXLINE+1];
	int bytes_read;
	char ipaddr[16], message[20];
	
	/* Read line */
	bytes_read = Read(sockfd, buff, sizeof(buff));

	if( bytes_read < 0 )
		printf("msg_recv() error");
	
	/* Fragment line into sub-components based on spaces*/
	sscanf(buff, "%s %d %s", ipaddr, &source_port, message);

	ipaddr[strlen(ipaddr)]	= 0;	

	printf("IP Addr: %s\n", ipaddr);
	printf("Port Number: %d\n", source_port);
	printf("Msg: %s\n", msg);

	strcpy(ipaddr, source_ip);
	message[strlen(message)] = 0;
	strcpy(msg, message);

}
