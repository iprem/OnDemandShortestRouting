#include "a3.h"


int main(int argc, char **argv)
{

  int ud_sockfd = 0, source_port = 0, dest_port = 0;
  //char *ds_odr_path = "/home/mliuzzi/odr_path";
  char *ds_odr_path = "/users/mliuzzi/cse533/un_odr_path";
  struct sockaddr_un ds_odr, cliaddr;
  struct sockaddr_ll pk_rreq;
  char recv_buf[4096];
  char source_ip[40];
  char dest_ip[40];
  char msg [] = "I'm here!";

  ud_sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
  init_sockaddr_un(&ds_odr, ds_odr_path);

  Bind(ud_sockfd, (SA*) &ds_odr, SUN_LEN(&ds_odr));

  //msg_recv(ud_sockfd, recv_buf, source_ip, source_port );
  //msg_send(ud_sockfd, dest_ip, dest_port, msg, 0);
  
  /*
    Receive a messsage from local socket
  */
  
  dg_echo(ud_sockfd, (SA*) &cliaddr, sizeof(cliaddr));

  

  /*
    Create packet socket
  */

  pk_sockfd = Socket(AF_PACKET, SOCK_RAW, htons(PROT_MAGIC));


  unlink(ds_odr_path);
  exit(0);


}


int write_rreq(int fd){

  struct sockaddr_ll rreq;
  struct ethhdr hdr;

  memset(&rreq, 0, sizeof(struct sockaddr_ll));
  rreq.sll_family = AF_PACKET;
  // XX
  rreq.sll_ifindex = ??;
  rreq.sll_halen = ETH_ALEN;
  rreq.sll_addr = {0xff:ff:ff:ff:ff:ff};
  

}
