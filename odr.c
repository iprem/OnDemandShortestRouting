#include "a3.h"
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include "hw_addrs.h"


#define RT struct routing_table
#define TRUE 	1
#define FALSE	0
#define ERROR	2
#define RREQ 0
#define RREP 1
#define APP_PAYLOAD 2
#define ROUTING_TABLE_SIZE 100
#define REVERSE_PATH_SIZE 100
/*
  Routing table is a combination of reverse route/forward route
*/

/* 
   Going to need beyond 10 records because we have combined backward routes with forward routes 
   When we need to send rrep backwards we need to trace back according to prev_hop.
   Thus we could have 9 different prev hop to 9 different vms....81ish records? 
*/


struct rreq_reverse_path
{
  
  /* reverse route fields */
  int b_id; /*broadcast id*/
  unsigned char prev_hop[6];
  int in_interface_index;	//Out going interface index

  /* presentation format */
  char src_addr[16]; 
  char dest_addr[16];

}r_paths[REVERSE_PATH_SIZE];

struct routing_table{
  /*convenience*/
  char ip [16];	   //IP address of vm

  /* presentation format */
  char dest_addr[16];

  /* forward route fields */
  unsigned char next_hop[6]; //Ethernet address  of next hop
  int out_interface_index;	//Out going interface index
  int hop_count;			//Number of hops to detination
  struct timespec timestamp;	//Time stamp
  
} vm[ROUTING_TABLE_SIZE];

struct rreq
{
  
  char src_addr[16];
  char dest_addr[16];
  
  //incremented for every new rreq
  int b_id;
  
  //incremented at each ODR
  int hop_count;

  /* Set to 1 when rreply is sent so that other nodes do not send rrep messages*/
  int rrep_flag;

  /* set to nonzero to force route rediscovery*/
  int discover_flag;
  
};

struct rrep
{
  
  /*network order */
  char src_addr[16];
  char dest_addr[16];
    
  //incremented at each ODR
  int hop_count;
  
};

struct app_payload
{

  ;


};


struct odr_message
{

  int type; 

  union msg 
  {
    struct rreq odr_rreq;
    struct rrep odr_rrep;
    struct app_payload odr_payload;
  }contents;

}odr_msg_rreq; /* our global, source, rreq */



int reached_destination(struct rreq * rreq);
void init_rpaths(struct rreq_reverse_path * rpath);
void flood_rreqs(int fd, char * buffer, struct sockaddr_ll* sockaddr_rreq);
unsigned char* get_source_ethaddr(unsigned char *buf, int index);
int dont_have_rreq(struct rreq_reverse_path * rpath, struct rreq* rreq);
void add_rpath(struct rreq_reverse_path * rpath, struct sockaddr_ll * sock_rreq, struct rreq * rreq);
struct rreq_reverse_path * get_rpath(struct rreq_reverse_path * r_paths, char * dest_addr);
void write_source_rrep(struct odr_message *rrep, char * recv_buf, char *send_buf, struct sockaddr_ll* recv_sockaddr);
void write_source_rreq(char *buffer, struct sockaddr_ll* sockaddr_rreq, char * dest_addr, int discover);
void write_forward_rrep(char * send_buf, struct sockaddr_ll * pk_rreq, struct rreq_reverse_path * r_paths);
void write_forward_rreq(char * buffer, struct sockaddr_ll* sockaddr_rreq, int rrep_sent);
void update_route_table_rrep(RT* vm, struct sockaddr_ll * sock_rrep,  struct rrep* rrep);
int setTimeStamp(RT *vm);
void print_eth_addr(unsigned char *addr);

int main(int argc, char **argv)
{

  int ud_sockfd = 0, pk_sockfd = 0,  source_port = 0, dest_port = 0;
  socklen_t len = sizeof(struct sockaddr_ll);
  //char *ds_odr_path = "/home/mliuzzi/odr_path";
  //char *ds_odr_path = "/users/mliuzzi/cse533/un_odr_path";
  char *ds_odr_path = "/home/mliuzzi/un_odr_path";
  struct sockaddr_un ds_odr, cliaddr;
  struct sockaddr_ll pk_rreq;
  struct odr_message* recv_odr_msg;
  struct odr_message odr_msg_rrep;
  char recv_buf[ETH_FRAME_LEN];
  char send_buf[ETH_FRAME_LEN];
  char own_ip[16];
  char *data = recv_buf + 14;
  

  /* Initialize structures to zero */
  memset(&odr_msg_rreq, 0, sizeof(struct odr_message));
  memset(vm, 0, sizeof(struct routing_table) * 10);
  memset(recv_buf, 0, ETH_FRAME_LEN);
  memset(send_buf, 0, ETH_FRAME_LEN);
  memset(&odr_msg_rrep, 0, sizeof(struct odr_message));
  init_sockaddr_un(&ds_odr, ds_odr_path);
  init_rpaths(r_paths);

  ud_sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
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
  //pk_sockfd = 1;
  
  //debug
  //force send rreq
  //other odr will just wait to receive 
  //arg 1 is flag to do it and arg2 is ip
  if (argc > 1)
    {
      printf("argc: %d \n", argc);
      if(strtol(argv[1], 0, 10) == 1)
	{
	  printf("argv[1]: %s", argv[1]);
	  write_source_rreq(send_buf, &pk_rreq, argv[2], 0);
	  printf("Return from write_source_rreq. \n");
	  flood_rreqs(pk_sockfd, send_buf, &pk_rreq );
	}
    }

  recvfrom(pk_sockfd, recv_buf, ETH_FRAME_LEN, 0, (SA *) &pk_rreq, &len);
  printf("Return from receivefrom prior to loop. \n" );
  while(1)
    {
      recv_odr_msg = data;
      if (recv_odr_msg->type == RREQ)
	{
	  printf("Received RREQ. \n" );

	  printf("Eth addr in pk_rreq: ");
	  print_eth_addr(pk_rreq.sll_addr);
	  printf("Eth addr in ethhdr: ");
	  struct ethhdr *temp;
	  temp = recv_buf;
	  print_eth_addr(temp->h_source);
	  
	  
	  memset(send_buf, 0, ETH_FRAME_LEN);
	  memcpy(send_buf, recv_buf, ETH_FRAME_LEN);
	  //not working right now
	  //xx
	  if(dont_have_rreq(r_paths, &(recv_odr_msg->contents.odr_rreq)))
	    {
	      printf("Did not have rreq \n" );
	      add_rpath(r_paths, &pk_rreq, &(recv_odr_msg->contents.odr_rreq));
	      memset(&pk_rreq, 0, sizeof(struct sockaddr_ll));
	      write_forward_rreq(send_buf, &pk_rreq, 0 );
	      flood_rreqs(pk_sockfd, send_buf, &pk_rreq);
	    }
	  /* Actually needs to be different logic 
	     you can not have an rreq and still be the destination */
	  if(reached_destination(&(recv_odr_msg->contents.odr_rreq)))
	    {
	      printf("Reached Destination \n" );
	      struct odr_message *test;
	      test = recv_buf + 14;
	      printf("Before write_source rep: %s \n" ,test->contents.odr_rreq.src_addr);
	      memset(send_buf, 0, ETH_FRAME_LEN);
	      write_source_rrep(&odr_msg_rrep, recv_buf, send_buf, &pk_rreq);

	      printf("After write_source rep: %s \n" ,test->contents.odr_rreq.src_addr);
	      printf("Eth addr in pk_rreq: ");
	      print_eth_addr(pk_rreq.sll_addr);
	      printf("Eth addr in ethhdr: ");
	      struct ethhdr *temp;
	      temp = send_buf;	      
	      print_eth_addr(temp->h_dest);
	      printf("src addr before sendto: %s \n", test->contents.odr_rrep.src_addr);
	      Sendto(pk_sockfd, send_buf, ETH_FRAME_LEN, 0, (SA*) &pk_rreq, sizeof(struct sockaddr_ll));
	      printf("Sent rrep! \n");
	    }
	}
      if(recv_odr_msg->type == RREP)
 	{
	  printf("Received RREP \n" );
	  findOwnIP(own_ip);
	  printf("Found IP \n");
	  printf("Own IP: %s \n", own_ip);
	  printf("rrep src addr: %s \n", recv_odr_msg->contents.odr_rrep.src_addr);
	  if(!strcmp(recv_odr_msg->contents.odr_rrep.src_addr, own_ip))
	    {
	      printf("Received RREP at src \n");
	      break;
	    }
	  update_route_table_rrep(vm, &pk_rreq, &(recv_odr_msg->contents.odr_rrep));
	  printf("After route table. \n");
	  write_forward_rrep(send_buf, &pk_rreq, r_paths);
	  printf("After route table. \n");
	}
      
      memset(recv_buf, 0, ETH_FRAME_LEN);
      recvfrom(pk_sockfd, recv_buf, ETH_FRAME_LEN, 0, (SA *) &pk_rreq, &len);
    }

  unlink(ds_odr_path);
  exit(0);

  }


/*

  @send_buf send_buffer to hold modified recv_buf
  @pk_rreq assumption is that family, sll_halen set correctly from receivefrom. Only thing we have to substitute
           is new address and index from rpaths
  @vm routing table which needs to be traversed for prev_hop

*/

void
write_forward_rrep(char * send_buf, struct sockaddr_ll *pk_rreq , struct rreq_reverse_path * r_paths)
{
  struct odr_message *forward_odr_msg_rrep = (struct odr_message * ) send_buf + 14;
  struct rrep * rrep = &forward_odr_msg_rrep->contents.odr_rrep;
  struct ethhdr *send_hdr = (struct ethhdr*) send_buf;
  struct rreq_reverse_path *rrep_rpath;
  unsigned char src_addr[6];

  rrep_rpath = get_rpath(r_paths, rrep->dest_addr);
  
  forward_odr_msg_rrep->contents.odr_rrep.hop_count++;
  
  memcpy(pk_rreq->sll_addr, rrep_rpath->prev_hop, ETH_ALEN);
  pk_rreq->sll_ifindex =  rrep_rpath->in_interface_index;

  memcpy(send_hdr->h_dest, rrep_rpath->prev_hop, ETH_ALEN);
  memcpy(send_hdr->h_source, get_source_ethaddr(src_addr , rrep_rpath->in_interface_index), ETH_ALEN);
  send_hdr->h_proto = htons(PROT_MAGIC);
    
}

unsigned char * 
get_source_ethaddr(unsigned char *buf, int index)
{

  struct hwa_info *hwa, *hwahead;
  int i;

  i = 0;
  for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next)
    {
      // All zeroes used to indicate invalid address
      // i.e. if it's the loopback interface
      // so only set prflag
      if (hwa->if_index == index) 
	{
	  memcpy(buf, hwa->if_haddr, ETH_ALEN);
	  return buf;
	}
      i++;
    }
  
  return NULL;

}

void
init_rpaths(struct rreq_reverse_path * rpath)
{
  int i;
  for(i = 0; i < REVERSE_PATH_SIZE; i++)
    {
      rpath->b_id = -1;
      rpath++;
    }
}


int
dont_have_rreq(struct rreq_reverse_path * rpath, struct rreq* rreq)
{
  int i;
  for(i = 0; i < REVERSE_PATH_SIZE; i++)
    {
      if((!strcmp(rreq->src_addr, rpath->src_addr)) && (rreq->b_id == rpath->b_id))
	return TRUE;

    }

  return FALSE;
}

void
add_rpath(struct rreq_reverse_path * rpaths, struct sockaddr_ll * sock_rreq, struct rreq * rreq)
{
  int i;
  for(i = 0; i < ROUTING_TABLE_SIZE; i++)
    {
      if(rpaths->b_id == -1)
	{
	  rpaths->b_id = rreq->b_id;
	  memcpy(rpaths->src_addr, rreq->src_addr, 16);
	  memcpy(rpaths->prev_hop, sock_rreq->sll_addr, ETH_ALEN);
	  rpaths->in_interface_index = sock_rreq->sll_ifindex;
	}
      rpaths++;
    }
}

RT* 
get_route_entry(RT* vm, char *dest_addr )
{
  int i;
  for(i = 0; i < ROUTING_TABLE_SIZE; i++)
    {
      if(!strcmp(vm->dest_addr, dest_addr))
	{
	  return vm;
	}
      vm++;
    }
    return NULL;
}

struct rreq_reverse_path *
get_rpath(struct rreq_reverse_path * r_paths, char * dest_addr)
{
  int i;
  for(i = 0; i < REVERSE_PATH_SIZE; i++)
    {
      if(!strcmp(r_paths->dest_addr, dest_addr))
	return r_paths;


    }

  return NULL;
}

/*

@vm routing table
@sock_rrep the sockaddr_ll from receivefrom which contains the information we need to setup the forward path
@rrep the rrep which we will use to find the correct routing table entry and update the hop count
@return: will fail silently..

*/

void update_route_table_rrep(RT* vm, struct sockaddr_ll* sock_rrep,  struct rrep* rrep)
{

  RT* entry; 
  entry = get_route_entry(vm, rrep->dest_addr);
 
  //set forward address
  memcpy(entry->next_hop, sock_rrep->sll_addr, ETH_ALEN);
  //set interface index
  entry->out_interface_index = sock_rrep->sll_ifindex;
  //set hop count
  entry->hop_count = rrep->hop_count;
  
  setTimeStamp(entry);

}


//XX
//Does a hardware address show up multiple times if it has alias ip addrs?

/*

  Write a route request message to each interface


@fd socket to send message out on
@buffer ethernet frame to send 
@sockaddr_rreq sockaddr_ll struct containing dest ethernet addr, family, protocol, interface index, and addr length

*/

void flood_rreqs(int fd, char * buffer, struct sockaddr_ll* sockaddr_rreq)
{

  struct hwa_info *hwa, *hwahead;
  struct ethhdr* hdr;
  int valid_addr =0, i = 0;
  char *eth0 = "eth0";

  printf("In flood rreqs. \n");

  hdr = (struct ethhdr *) buffer;

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
	    printf("Name: %s", hwa->if_name);
	    if(strcmp(hwa->if_name, eth0))
	      {
		valid_addr = 1;
		break;
	      }
	  }
      } while (++i < IF_HADDR);
 

      //fill out ethernet source addr and ethernet interface index 
      //send 
      if(valid_addr)
	{
	  sockaddr_rreq->sll_ifindex = hwa->if_index;
	  memcpy(hdr->h_source, hwa->if_haddr, ETH_ALEN);
	  Sendto(fd, buffer, ETH_FRAME_LEN, 0,(SA *) sockaddr_rreq, sizeof(struct sockaddr_ll));
	  printf("Flooded rreq \n");
	}
    }
}

int reached_destination(struct rreq * rreq)
{
  char own_ip[16];
  findOwnIP(own_ip);
  printf("reached destination addr: %s \n", rreq->dest_addr);
  printf("reached destination addr src addr: %s \n", rreq->src_addr);
    
  if (!strcmp(rreq->dest_addr, own_ip))
    return TRUE;
  else
    return FALSE;


}

void print_eth_addr(unsigned char *addr)
{
  int i;
  for(i = 0; i < 6; i++)
    {
      printf("%.2x%s", *addr++ & 0xff, (i == 5) ? " " : ":");
    }
  
  printf("\n");

}

void write_source_rrep(struct odr_message *rrep, char * recv_buf, char *send_buf, struct sockaddr_ll* recv_sockaddr)
{

  struct hwa_info *hwa, *hwahead;
  struct odr_message* rreq;
  struct ethhdr* recv_hdr;
  struct ethhdr* send_hdr;
  char* data;

  recv_hdr = (struct ethhdr *) recv_buf;
  send_hdr = (struct ethhdr *) send_buf;
  
  //recv_buf data
  rreq = recv_buf + 14;
  //send_buf data
  data = send_buf + 14;
  //set rrep source addr to rreq source addr
  printf("Before memcpy Write source rrep src_addr, rreq: %c \n", rreq->contents.odr_rreq.src_addr[0]);
  printf("Before memcpy Write source rrep src_addr, rreq: %c \n", rreq->contents.odr_rreq.src_addr[1]);
  printf("Before memcpy Write source rrep src_addr, rreq: %c \n", rreq->contents.odr_rreq.src_addr[2]);
  memcpy(rrep->contents.odr_rrep.src_addr, rreq->contents.odr_rreq.src_addr, 16);
  printf("Write source rrep src_addr, rrep: %s \n", rrep->contents.odr_rrep.src_addr);
  printf("Write source rrep src_addr, rreq: %s \n", rreq->contents.odr_rreq.src_addr);
  

  //set rrep dest addr to rreq dest addr
  memcpy(rrep->contents.odr_rrep.dest_addr, rreq->contents.odr_rreq.dest_addr, 16);
  
  rrep->type = RREP;
  rrep->contents.odr_rrep.hop_count = 1;
  
  //previous node ethernet address which is our new destination is contained in the recv_sockaddr
  memcpy(send_hdr->h_dest, recv_sockaddr->sll_addr, ETH_ALEN);
  
  //find source addr that matches received index
  for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next)
    {
      if (hwa->if_index == recv_sockaddr->sll_ifindex)
	{
	  memcpy(send_hdr->h_source, hwa->if_haddr, ETH_ALEN);
	}
    } 

  send_hdr->h_proto = htons(PROT_MAGIC);
  
  memcpy(data, rrep, sizeof(struct odr_message));
  
}


/*
  Fill buffer with ethernet frame containing RREQ

  @buffer char buffer[ETH_FRAME_LEN] that has been zeroed
  @sockaddr_rreq sockaddr_ll struct for our rreq
  @source_eth source mac address (obtained by get_hw_addr())
  @dest_addr destination ip address in presentation format
  @index of interface corresponding to haddr (obtained by get_hw_addr())
  @discover nonzero to force rediscover
 */
void write_source_rreq(char *buffer, struct sockaddr_ll* sockaddr_rreq, char * dest_addr, int discover){

  struct ethhdr* hdr;
  //ethernet header is 14 bytes
  char *data;
  unsigned char dest_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  int i = 0;
  char own_ip[16];

  /* Set ethernet frame ptrs */
  hdr = (struct ethhdr *) buffer;
  data = buffer + 14;

  findOwnIP(own_ip);

  /* Initialize RREQ */
  odr_msg_rreq.type = 0;
  memcpy(odr_msg_rreq.contents.odr_rreq.src_addr, own_ip, 16);
  printf("Write source rreq, own_ip: %s \n", own_ip);
  printf("Write source rreq, own_ip: %c \n", own_ip[15]);
  printf("Write source rreq, rreq: %s \n", odr_msg_rreq.contents.odr_rreq.src_addr);
  memcpy(&odr_msg_rreq.contents.odr_rreq.dest_addr, dest_addr, 16);
  odr_msg_rreq.contents.odr_rreq.b_id++;
  odr_msg_rreq.contents.odr_rreq.hop_count = 0;
  odr_msg_rreq.contents.odr_rreq.rrep_flag = 0;
  odr_msg_rreq.contents.odr_rreq.discover_flag = discover;

  memset(sockaddr_rreq, 0, sizeof(struct sockaddr_ll));
  sockaddr_rreq->sll_family = AF_PACKET;
  //sockaddr_rreq->sll_ifindex = index;
  sockaddr_rreq->sll_halen = ETH_ALEN;
  sockaddr_rreq->sll_addr[0] = 0xff;
  sockaddr_rreq->sll_addr[1] = 0xff;
  sockaddr_rreq->sll_addr[2] = 0xff;
  sockaddr_rreq->sll_addr[3] = 0xff;
  sockaddr_rreq->sll_addr[4] = 0xff;
  sockaddr_rreq->sll_addr[5] = 0xff;
  sockaddr_rreq->sll_addr[6] = 0x00;
  sockaddr_rreq->sll_addr[7] = 0x00;

  memcpy(hdr->h_dest, dest_mac, ETH_ALEN);
  //memcpy(hdr->h_source, source_eth, ETH_ALEN);
  hdr->h_proto = htons(PROT_MAGIC);

  memcpy(data, &odr_msg_rreq, sizeof(odr_msg_rreq));

}


void write_forward_rreq(char * buffer, struct sockaddr_ll* sockaddr_rreq, int rrep_sent)
{

  struct odr_message *forward_odr_msg_rreq;

  /* Set ethernet frame ptrs */
  forward_odr_msg_rreq = (struct odr_message *) buffer + 14;
  forward_odr_msg_rreq->contents.odr_rreq.hop_count++;
  forward_odr_msg_rreq->contents.odr_rreq.rrep_flag = rrep_sent;

  memset(sockaddr_rreq, 0, sizeof(struct sockaddr_ll));
  sockaddr_rreq->sll_family = AF_PACKET;
  sockaddr_rreq->sll_halen = ETH_ALEN;
  sockaddr_rreq->sll_addr[0] = 0xff;
  sockaddr_rreq->sll_addr[1] = 0xff;
  sockaddr_rreq->sll_addr[2] = 0xff;
  sockaddr_rreq->sll_addr[3] = 0xff;
  sockaddr_rreq->sll_addr[4] = 0xff;
  sockaddr_rreq->sll_addr[5] = 0xff;
  sockaddr_rreq->sll_addr[6] = 0x00;
  sockaddr_rreq->sll_addr[7] = 0x00;


}



/* Check if routing table is stale */
int isRoutingTableStale(RT *vm, struct timespec stale){
	
	struct timespec curtime;
	
	if ( clock_gettime(CLOCK_REALTIME, &curtime) == -1 ){
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
	
  if(clock_settime(CLOCK_REALTIME, &vm->timestamp) == -1){
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
	  {
	    printf("Reached destination! \n");
	    return FALSE;
	  }
	if ( isRoutingTableStale(vm, stale) == TRUE )
		return TRUE;

	if ( req-> rrep_flag == 1 )
		return TRUE;

	if ( (strcmp(own_ip, req->dest_addr) != 0) )
		return TRUE;
	else 
		if ( (vm->next_hop == NULL) && (vm->hop_count > -1) )
			return TRUE;
		else 
			return FALSE;


}
