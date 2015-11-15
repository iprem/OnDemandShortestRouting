#include "a3.h"
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include "hw_addrs.h"

void flood_rreqs(int fd);
void write_rreq(int fd, char *haddr, int index);

/**********************************************/
#define RT struct routing_table
#define TRUE 	1
#define FALSE	0
#define ERROR	2


struct routing_table{

	in_addr_t ip;				//IP address of vm
	char next_hop[20];			//Ethernet address  of next hop
	int out_interface_index;	//Out going interface index
	int num_hops_destn;			//Number of hops to detination
	struct timespec timestamp;	//Time stamp

} vm[10];

/* Check if routing table is stale */
int isRoutingTableStale(RT *vm, struct timespec stale){
	
	struct timespec curtime;
		
	if ( clock_gettime(CLOCK_REALTIME, curtime) == -1 ){
		printf("Error: Unable to get current time");
		return ERROR;
	}

	/*Assuming staleness is only in seconds*/
	if( curtime.tv_sec > (vm->timestamp.tv_sec + stale.tv_sec) )
		return TRUE;
	
	else		
		return FALSE;
	
}

/*Set timestamp for a routing table */
int setTimeStamp(RT *vm){
	
	if(clock_settime(CLOCK_REALTIME, vm->timestamp) == -1){
		printf("Failed to set timestamp for destination IP: %s", vm->ip);
		return FALSE;
	}

	return TRUE;

}

/*Checks for all conditions if flooding is required */
int isFloodingRequired(RT *vm, struct timespec stale, struct rreq *req){
	
	char own_ip[16];
	findOwnIP(own_ip);
	
    // Got to test this first condition
	if ( strcmp(own_ip, (req->dest_addr)) == 0 )
		return FALSE;

	if ( isRoutingTableStale(vm, stale) == TRUE )
		return TRUE;

	if ( req-> rrep_flag == 1 )
		return TRUE;

	if ( (strcmp(ip, req->dest_addr) != 0) )
		return TRUE;
	else 
		if ( (vm->next_hop == NULL) && (vm->num_hop_destn > -1) )
			return TRUE;
		else 
			return FALSE;

}
/*****************************************************************/


struct rreq
{
  
  /*network order */
  in_addr_t source_addr;
  in_addr_t dest_addr;
  
  //incremented for every new rreq
  int broadcast_id;
  
  //incremented at each ODR
  int hop_count;
  
  int rrep_flag;
  
}odr_rreq;


struct rrep
{
  
  /*network order */
  in_addr_t source_addr;
  in_addr_t dest_addr;
  
  //incremented for every new rreq
  int broadcast_id;
  
  //incremented at each ODR
  int hop_count;
  
}odr_rrep;

int main(int argc, char **argv)
{

  int ud_sockfd = 0, pk_sockfd = 0,  source_port = 0, dest_port = 0;
  //char *ds_odr_path = "/home/mliuzzi/odr_path";
  //char *ds_odr_path = "/users/mliuzzi/cse533/un_odr_path";
  char *ds_odr_path = "/home/mliuzzi/un_odr_path";
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

  //Bind(ud_sockfd, (SA*) &ds_odr, SUN_LEN(&ds_odr));

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
  int valid_addr =0, i = 0;
  char *eth0 = "eth0";
    
  for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next)
    {

      valid_addr = 0;
      // All zeroes used to indicate invalid address
      // i.e. if it's the loopback interface
      // so only set prflag
      i = 0;
      do {
	if (hwa->if_haddr[i] != '\0') 
	  {
	    if(strcmp(hwa->if_name, eth0))
	    {
		valid_addr = 1;
		break;
	    }
	  }
      } while (++i < IF_HADDR);
      
      if(valid_addr)
	{
	  write_rreq(fd, hwa->if_haddr, hwa->if_index);
	}
    }
}


/*

  @fd packet socket descriptor to write to
  @haddr source mac address (obtained by get_hw_addr())
  @index of interface corresponding to haddr (obtained by get_hw_addr())

 */
void write_rreq(int fd, char * hwaddr, int index){

  struct sockaddr_ll rreq;
  struct ethhdr* hdr;
  char *buffer;
  //ethernet header is 14 bytes
  char *data;
  unsigned char dest_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  int i = 0;
  char *ptr;

  buffer = calloc(1, ETH_FRAME_LEN);
  hdr = buffer;
  data = buffer + 14;

  //test data
  for(i; i < 10; i++)
    {
      data[i] = 'a' + i;
    }

  memset(&rreq, 0, sizeof(struct sockaddr_ll));

  rreq.sll_family = AF_PACKET;
  rreq.sll_ifindex = index;
  rreq.sll_halen = ETH_ALEN;
  rreq.sll_addr[0] = 0xff;
  rreq.sll_addr[1] = 0xff;
  rreq.sll_addr[2] = 0xff;
  rreq.sll_addr[3] = 0xff;
  rreq.sll_addr[4] = 0xff;
  rreq.sll_addr[5] = 0xff;
  rreq.sll_addr[6] = 0x00;
  rreq.sll_addr[7] = 0x00;

  memcpy(hdr->h_dest, dest_mac, ETH_ALEN);
  memcpy(hdr->h_source, hwaddr, ETH_ALEN);
  hdr->h_proto = htons(PROT_MAGIC);

  Sendto(fd, buffer, ETH_FRAME_LEN, 0, (SA *) &rreq, sizeof(rreq));

}
