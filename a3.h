#include "unp.h"

#define PROT_MAGIC 0x4d70

void msg_send(int sockfd, char* dest_ip, int dest_port, char* msg, int flag);
void msg_recv(int sockfd, char* msg, char* source_ip, int* source_port);

void inline init_sockaddr_un(struct sockaddr_un *addr, char * path )
{ 
  memset(addr, 0, sizeof(struct sockaddr_un));
  addr->sun_family = AF_LOCAL;
  strncpy(addr->sun_path, path, strlen(path));
}
