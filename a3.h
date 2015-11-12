#include "unp.h"

#define PROT_MAGIC 0x4d70

void msg_send(int sockfd, char* dest_ip, int dest_port, char* msg, int flag);
void msg_recv(int sockfd, char* msg, char* source_ip, int* source_port);
void findHostName(char *ip, char *host);

void inline init_sockaddr_un(struct sockaddr_un *addr, char * path )
{ 
  memset(addr, 0, sizeof(struct sockaddr_un));
  addr->sun_family = AF_LOCAL;

  /*Before copying path, we can check if sizeof(path) fits into sun_path*/
  strncpy(addr->sun_path, path, strlen(path));
}

void findHostName(char *ip, char *host){
	
	struct hostent *he;
	struct in_addr ipv4addr;

	inet_pton(AF_INET, ip, &ipv4addr);
	if( (he = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AF_INET)) == NULL )
		printf("Error: gethostbyaddr\n");
	
	strcpy(host, he->h_name);

}
