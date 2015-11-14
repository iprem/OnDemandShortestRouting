#include "a3.h"
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>

#include <stdio.h>

int main(int argc, char **argv)
{
  int pk_sockfd = 0, len = 0;
  char buffer[ETH_FRAME_LEN];
  struct sockaddr_ll rreq;


  memset(&rreq, 0, sizeof(rreq));
  memset(buffer, 0, ETH_FRAME_LEN);
  pk_sockfd = Socket(AF_PACKET, SOCK_RAW, htons(PROT_MAGIC));

  len = sizeof(rreq);
  recvfrom(pk_sockfd, buffer, ETH_FRAME_LEN, 0, &rreq, &len);
  printf("Returned length: %d \n", len);
  printf("sll_hatype: %d \n", rreq.sll_hatype);

  print_sockaddr_ll(&rreq, 1);
  printf("Ethframe msg: %s", (buffer + 14) );
}
