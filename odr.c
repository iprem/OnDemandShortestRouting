#include "a3.h"
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include "hw_addrs.h"

void flood_rreqs(int fd);
void write_rreq(int fd, char *haddr, int index);

int main(int argc, char **argv)
{

  int ud_sockfd = 0, pk_sockfd = 0,  source_port = 0, dest_port = 0;
  //char *ds_odr_path = "/home/mliuzzi/odr_path";
  char *ds_odr_path = "/users/mliuzzi/cse533/un_odr_path";
  struct sockaddr_un ds_odr, cliaddr;
  struct sockaddr_ll pk_rreq;
  struct hwa_info* hwa_head = NULL;
  struct hwa_info* hwa = NULL;
  
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
  

  //dg_echo(ud_sockfd, (SA*) &cliaddr, sizeof(cliaddr));
  

  /*
    Create packet socket
  */


  pk_sockfd = Socket(AF_PACKET, SOCK_RAW, htons(PROT_MAGIC));
  flood_rreqs(pk_sockfd);

  unlink(ds_odr_path);
  exit(0);


  }


//XX
//Does a hardware address show up multiple times if it has alias ip addrs? 

/*
  
  Write a route request message to each interface


*/
void flood_rreqs(int fd)
{

  struct hwa_info *hwa, *hwahead;   

  for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) 
    {   
      do {
	if (hwa->if_haddr[i] != '\0') {                                                                       
	  prflag = 1;                                                                                  
	  break;                                                                                       
	}                                                                                          
      } while (++i < IF_HADDR);

      if(prflag)
	{
	  write_rreq(fd, hwa->if_haddr, hwa->if_index);
	}
    }
}

void write_rreq(int fd, char * haddr, int index){

  struct sockaddr_ll rreq;
  struct ethhdr* hdr;
  char *buffer ;
  //ethernet header is 14 bytes
  char *data;
  int i = 0;

  buffer = calloc(1, ETH_FRAME_LEN);
  hdr = buffer;
  data = buffer[14];

  //test data
  for(i; i < 10; i++)
    {
      datap[i] = 'a' + i;
    }
  
  memset(&rreq, 0, sizeof(struct sockaddr_ll));
  rreq.sll_family = AF_PACKET;
  // XX
  rreq.sll_ifindex = index;
  rreq.sll_halen = ETH_ALEN;
  rreq.sll_addr = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

  
  hdr->h_dest = {0xff, 0xff, 0xff, 0x:ff, 0xff, 0xff};
  hdr->h_source = haddr;
  hdr->h_proto = PROT_MAGIC;


  Sendto(fd, buffer, ETH_FRAME_LEN, 0, (SA *) &rreq, sizeof(rreq));
}
