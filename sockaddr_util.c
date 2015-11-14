#include "sockaddr_util.h"
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>


void init_sockaddr_un(struct sockaddr_un *addr, char * path )
{ 
  memset(addr, 0, sizeof(struct sockaddr_un));
  addr->sun_family = AF_LOCAL;
  strncpy(addr->sun_path, path, strlen(path));
}


/*

Recv should be nonzero if the sockaddr_ll is from a recv call

*/
void print_sockaddr_ll(struct sockaddr_ll *addr, int recv)
{ 
  int i = 0;
  char *ptr;
  ptr = addr->sll_addr;

  printf("Interface Num: %d \n",  addr->sll_ifindex);
  if(recv)
    printf("Interface Source Addr: ",  addr->sll_addr);
  else
    printf("Interface Dest Addr: ",  addr->sll_addr);
    
  for(i = 0; i < 6; i++)
    {
      printf("%.2x%s", *ptr++ & 0xff, (i == 5) ? " " : ":");
    }
}
