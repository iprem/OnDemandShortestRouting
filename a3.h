#include "unp.h"
#include "sockaddr_util.h"

#define PROT_MAGIC 0x4d70

void msg_send(int sockfd, char* dest_ip, int dest_port, char* msg, int flag);
void msg_recv(int sockfd, char* msg, char* source_ip, int* source_port);

