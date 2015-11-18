#include "unp.h"
#include "sockaddr_util.h"
#include "hw_addrs.h"
#include <time.h>

#define PROT_MAGIC 0x4d70

void msg_send(int sockfd, char* dest_ip, int dest_port, char* msg, int flag);
void msg_recv(int sockfd, char* msg, char* source_ip, int* source_port);
void findHostName(char *ip, char *host);
void findOwnIP(char * own_ip);


void findHostName(char *ip, char *host)
{
	
  struct hostent *he;
  struct in_addr ipv4addr;
  
  inet_pton(AF_INET, ip, &ipv4addr);
  if( (he = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AF_INET)) == NULL )
    printf("Error: gethostbyaddr\n");
	
  strcpy(host, he->h_name);
  
}

void findOwnIP(char * own_ip){
	
	struct hwa_info	*hwa, *hwahead;
	struct sockaddr	*sa;

	for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) {
		
		if(strcmp("eth0",hwa->if_name) == 0)
		  {
			if ( (sa = hwa->ip_addr) != NULL)
			  {
			    strcpy(own_ip,Sock_ntop_host(sa, sizeof(*sa)));
			    break;
			  }
		  }
		else
		  printf("Error: No IP address found for eth0\n");
	}

	free_hwa_info(hwahead);

}

