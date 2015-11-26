/* Server Program for Assignment 3 CSE 533: Network Programming */
/*Date: 11th Nov, 2015*/


#include "unp.h"
#include "a3.h"
#include "hw_addrs.h"
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>


#define SUN_PATH "/home/mliuzzi/server_addr"	/*Define sun_path here*/
#define MSG_SIZE ETH_FRAME_LEN

/* Canonical IP Address of all the VMs of interface eth0 */
/*Not required as i am using Get_hw_addrs to find IP on eth0*/
/*
char ipVM[10][16] = {
							"130.245.156.21",
							"130.245.156.22",
							"130.245.156.23",
							"130.245.156.24",
							"130.245.156.25",
							"130.245.156.26",
							"130.245.156.27",
							"130.245.156.28",
							"130.245.156.29",
							"130.245.156.20"	}; */

//void findHostName(char *, char *); */Defined this function in a3.h */
void findServerIP(char *);				

int main(int argc, char **argv){
  struct sockaddr_un ds_odr;
  struct sockaddr_un servaddr;
  int sockfd, port;
  time_t ticks;
  char buff[ETH_FRAME_LEN], msg[ETH_FRAME_LEN], client_ip[16], server_ip[16], server_host[16], client_host[16];
  char *ds_odr_path = "/home/mliuzzi/un_odr_path1";
	sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
	
	unlink(SUN_PATH);

	bzero(&servaddr, sizeof(servaddr));
	init_sockaddr_un(&servaddr, SUN_PATH);
	init_sockaddr_un(&ds_odr, ds_odr_path);
	
	Bind(sockfd, (SA *)&servaddr, sizeof(servaddr));
	Connect(sockfd, (SA *) & ds_odr, sizeof(struct sockaddr_un));
	
	findServerIP(server_ip);
	server_ip[strlen(server_ip)] = 0;
	//printf("Server IP: %s %d\n",server_ip,strlen(server_ip));
	if(server_ip == NULL)	err_quit("Couldn't determine server IP\n");
	findHostName(server_ip, server_host);
	system("clear");
	printf("\n****************Starting SERVER*******************\n\n");
	printf("Server IP Address at node %s: %s\n", server_host, server_ip);

	while(1){
		
	  struct msg_send_struct* server_msg;
	  server_msg = msg;

	  printf("Before msg recv...\n");
	  msg_recv(sockfd, msg, client_ip, &port);	
	  ticks = time(NULL);
	  memset(msg, 0, 512);
	  snprintf(msg, 512, "%.24s\r\n",ctime(&ticks));
	  printf("Current time: %s\n", msg);
	  printf("\nServer at node %s responding to request from %s\n", server_host, client_host);
	  port = 666;
	  msg_send(sockfd, client_ip, port, msg, 0);
	  break;
	  
	}
	
	unlink(SUN_PATH);
	
	exit(0);

}

/*Find VM */
/*
void FindHostName(char *ip, char *host){
	
	struct hostent *he;
	struct in_addr ipv4addr;

	//for(int i = 0; i < 10; i++)
	//	if(	strcmp(ipVM[i], ip) == 0 )
	//		return (i+1);
	//

	inet_pton(AF_INET, ip, &ipv4addr);
	he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
	
	strcpy(host,he->h_name);
}*/


/*Find IP address of server */
void findServerIP(char * server_ip){
	
	struct hwa_info	*hwa, *hwahead;
	struct sockaddr	*sa;
	//char   *ptr;

	for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) {
		
		//printf("%s :%s", hwa->if_name, ((hwa->ip_alias) == IP_ALIAS) ? " (alias)\n" : "\n");
		
		if(strcmp("eth0",hwa->if_name) == 0)
			if ( (sa = hwa->ip_addr) != NULL){
				strcpy(server_ip,Sock_ntop_host(sa, sizeof(*sa)));
				//printf("IP: %s\n",Sock_ntop_host(sa, sizeof(*sa)), server_ip);
				break;
			}
	}

	free_hwa_info(hwahead);


}
