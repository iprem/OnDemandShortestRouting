#ifndef A3_HW_H
#define A3_HW_H

#include "unp.h"
#include "sockaddr_util.h"
#include "hw_addrs.h"
#include <time.h>

#define PROT_MAGIC 0x4d70
#define DISCOVER 1

void msg_send(int sockfd, char* dest_ip, int dest_port, char* msg, int flag);
int msg_recv(int sockfd, char* msg, char* source_ip, int* source_port);
void findHostName(char *ip, char *host);
void findOwnIP(char * own_ip);

struct msg_send_struct
{
  char dest_ip[16];
  int dest_port;
  char msg[512];
  int flag;
};


#endif
