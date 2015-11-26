

#include "unp.h"
#include "a3.h"
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>


int msg_recv(int sockfd, char* msg, char* source_ip, int* source_port)
{

	static int pipefd[2];
	char buff[ETH_FRAME_LEN];
	int bytes_read;
	char ipaddr[16], message[20];
	struct msg_send_struct* ourmsg = buff;
	
	/* Read line */
	bytes_read = Read(sockfd, buff, sizeof(buff));
	
	memcpy(msg, ourmsg->msg, 512);
	*source_ip = ourmsg->dest_ip;
	*source_port = 666;
	
	if( bytes_read < 0 )
	  printf("msg_recv() error");
	
	return bytes_read;
}
