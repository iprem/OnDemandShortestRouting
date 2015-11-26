#include "a3.h"
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include "hw_addrs.h"
#include <sys/select.h>

#define RT struct routing_table
#define TRUE 	1
#define FALSE	0
#define ERROR	2
#define RREQ 0
#define RREP 1
#define APP_PAYLOAD 2
#define ROUTING_TABLE_SIZE 10
#define REVERSE_PATH_SIZE 1000
#define SOURCE 1
#define INTERMEDIATE 0
#define penter() printf("Enter"); printf ("__FUNCTION__ = %s\n", __FUNCTION__);
#define pexit()  printf("Return"); printf ("__FUNCTION__ = %s\n", __FUNCTION__);

const char ipVM[10][16] = {
							"130.245.156.21",
							"130.245.156.22",
							"130.245.156.23",
							"130.245.156.24",
							"130.245.156.25",
							"130.245.156.26",
							"130.245.156.27",
							"130.245.156.28",
							"130.245.156.29",
							"130.245.156.20"	};



struct rreq_reverse_path
{
  
  /* reverse route fields */
  int b_id; /*broadcast id*/
  unsigned char prev_hop[6];
  int in_interface_index;	//Out going interface index
  int hop_count;

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
  
};

static RT vm[ROUTING_TABLE_SIZE];

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


int have_route(RT* entry);
int reached_destination(struct rreq * rreq);
void init_rpaths(struct rreq_reverse_path * rpath);
void flood_rreqs(int fd, char * buffer, struct sockaddr_ll* sockaddr_rreq);
unsigned char* get_source_ethaddr(unsigned char *buf, int index);
int dont_have_rreq(struct rreq_reverse_path * rpath, struct rreq* rreq);
void add_rpath(struct rreq_reverse_path * rpath, struct sockaddr_ll * sock_rreq, struct rreq * rreq);
struct rreq_reverse_path * get_rpath(struct rreq_reverse_path * r_paths, char * dest_addr);
void write_source_rrep(struct odr_message *rrep, char * recv_buf, char *send_buf, struct sockaddr_ll* recv_sockaddr);
void write_source_rreq(char *buffer, struct sockaddr_ll* sockaddr_rreq, char * dest_addr, int discover);
void write_forward_rrep(char * send_buf, char * recv_buf, struct sockaddr_ll *pk_rreq , struct rreq_reverse_path * r_paths);
void write_forward_rreq( char * send_buf, char * recv_buf, struct sockaddr_ll* sockaddr_rreq, int rrep_sent);
void update_route_table_rrep(RT* , struct sockaddr_ll * sock_rrep,  struct rrep* rrep);
void update_route_table_rreq(RT* , struct sockaddr_ll * sock_rreq,  struct rreq* rreq);
int setTimeStamp(RT *);
void print_eth_addr(unsigned char *addr);
void init_RoutingEntry( RT * );
void init_RoutingTable(RT *);
RT* get_route_entry(RT* , char *dest_addr );
void write_forward_rrep2(char * send_buf, struct sockaddr_ll * pk_rreq);
int isRoutingTableStale(RT *vm, struct timespec stale);
void delete_stale_entry(RT* entry);
int lower_hop_count(struct rreq_reverse_path* rpath, struct rreq* rreq);
int improve_route(RT* vm, struct rreq* rreq);
void remove_rpath(struct rreq_reverse_path * rpaths,  struct rreq * rreq);
int reconfirm_route(RT* vm, struct rreq* rreq, struct timespec staleness_param);
int new_neighbor_same_hops(struct routing_table * vm, struct rreq* rreq, struct sockaddr_ll* sock_rreq);
int reached_src(struct rrep * rrep);


int main(int argc, char **argv)
{

  int ud_sockfd = 0, pk_sockfd = 0,  source_port = 0, dest_port = 0, source_flag = 0, discover = 0, sent_rrep = 0, send_application_payload = 0;
  fd_set rset;
  socklen_t len = sizeof(struct sockaddr_ll);
  //char *ds_odr_path = "/home/mliuzzi/odr_path";
  //char *ds_odr_path = "/users/mliuzzi/cse533/un_odr_path";
  char *ds_odr_path = "/home/mliuzzi/un_odr_path";
  struct sockaddr_un ds_odr, cliaddr;
  struct sockaddr_ll pk_rreq;
  struct odr_message* recv_odr_msg;
  struct odr_message odr_msg_rrep;
  struct msg_send_struct* msg_send_ptr;
  struct timespec staleness_param;
  RT* vm_entry_iter = NULL;
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
  memset(&staleness_param, 0, sizeof(struct timespec));
  init_sockaddr_un(&ds_odr, ds_odr_path);
  init_rpaths(r_paths);
  init_RoutingTable(vm);

  //set our staleness parameter
  //staleness_param.tv_sec = strtol(argv[1], 0, 10);
  staleness_param.tv_sec = 10;


  ud_sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
  Bind(ud_sockfd, (SA*) &ds_odr, SUN_LEN(&ds_odr));

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

  /*  if (argc > 1)
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
  */


  recv_odr_msg = data;
  while(1)
    {
      FD_ZERO(&rset);
      FD_SET(ud_sockfd, &rset);
      FD_SET(pk_sockfd, &rset);
      int max_fd = pk_sockfd > ud_sockfd ? pk_sockfd : ud_sockfd; 
      select(max_fd + 1, &rset, NULL, NULL, NULL);
      //source
      if(FD_ISSET(ud_sockfd, &rset))
	{
	  printf("Received client msg \n");
	  //XX
	  //We need to know who it's from but for now just deal with one client
	  recvfrom(ud_sockfd, recv_buf, ETH_FRAME_LEN, 0, NULL, NULL);
	  msg_send_ptr = recv_buf;
	  vm_entry_iter = get_route_entry(vm, msg_send_ptr->dest_ip);
	  if(have_route(vm_entry_iter))
	    {
	      printf("ODR: has a route \n.");
	      if(isRoutingTableStale(vm_entry_iter, staleness_param))
		{
		  printf("Route is stale \n.");
		  delete_stale_entry(vm_entry_iter);
		  write_source_rreq(send_buf, &pk_rreq, msg_send_ptr->dest_ip, msg_send_ptr->flag);
		  flood_rreqs(pk_sockfd, send_buf, &pk_rreq);
		}
	      else if(msg_send_ptr -> flag == DISCOVER)
		{
		  printf("ODR: Discover bit is set \n");
		  delete_stale_entry(vm_entry_iter);
		  write_source_rreq(send_buf, &pk_rreq, msg_send_ptr->dest_ip, msg_send_ptr->flag);
		  flood_rreqs(pk_sockfd, send_buf, &pk_rreq);
		}
	      else
		{
		  printf("ODR: has route and can send");
		  send_application_payload = 1;
 		}
	    }
	  else if(!have_route(vm_entry_iter))
	    {
	      printf("ODR: No route \n");
	      printf("ODR SOURCE RREQ: %s \n", msg_send_ptr->dest_ip);
	      write_source_rreq(send_buf, &pk_rreq, msg_send_ptr->dest_ip, msg_send_ptr->flag);
	      flood_rreqs(pk_sockfd, send_buf, &pk_rreq);
	    }	  
	}
      //Intermediate OR Destination node
      if(FD_ISSET(pk_sockfd, &rset))
	{
	  printf("Received packet msg \n");
	  recvfrom(pk_sockfd, recv_buf, ETH_FRAME_LEN, 0, (SA *) &pk_rreq, &len);
	  if(recv_odr_msg->type == RREQ)
	    {
	      // REACHED DESTINATION
	      // First rreq OR lower hop count
	      if(reached_destination(&(recv_odr_msg->contents.odr_rreq)) && 
		 ((dont_have_rreq(r_paths, &(recv_odr_msg->contents.odr_rreq)) || lower_hop_count(r_paths, &(recv_odr_msg->contents.odr_rreq)))))
		{
		  printf("Reached destination AND Don't have rreq OR lower_hop_count rreq recieved \n");
		  add_rpath(r_paths, &pk_rreq, &(recv_odr_msg->contents.odr_rreq));
		  
		  printf("Added rpath \n");
		  write_source_rrep(&odr_msg_rrep, recv_buf, send_buf, &pk_rreq);
		  printf("Wrote source rrep");
		  Sendto(pk_sockfd, send_buf, ETH_FRAME_LEN, 0, (SA*) &pk_rreq, sizeof(struct sockaddr_ll));
		  sent_rrep = 1;
		}	

	      // INTERMEDIATE NODE
	      // New rreq OR if route has a lower hop count...
	      else if(dont_have_rreq(r_paths, &(recv_odr_msg->contents.odr_rreq)) || lower_hop_count(r_paths, &(recv_odr_msg->contents.odr_rreq)))
		{
		  printf("Don't have rreq OR received rreq with lower hop count...\n");
		  vm_entry_iter = get_route_entry(vm, recv_odr_msg->contents.odr_rreq.dest_addr);
		  // HAVE ROUTE
		  if(have_route(vm_entry_iter))
		    {
		      printf("Have route...\n");
		      //if stale or force discover
		      if(isRoutingTableStale(vm_entry_iter, staleness_param) || 
			 recv_odr_msg->contents.odr_rreq.discover_flag == DISCOVER)
			{
			  printf("Route stale or DISCOVER enabled \n");
			  delete_stale_entry(vm_entry_iter);
			  //will remove if it exists or return silently
			  remove_rpath(r_paths, &(recv_odr_msg->contents.odr_rreq));
			  add_rpath(r_paths, &pk_rreq, &(recv_odr_msg->contents.odr_rreq));
			  memset(&pk_rreq, 0, sizeof(struct sockaddr_ll));
			  write_forward_rreq(send_buf, recv_buf, &pk_rreq, recv_odr_msg->contents.odr_rreq.rrep_flag);
			  flood_rreqs(pk_sockfd, send_buf, &pk_rreq);
			}
		      //else we have a valid route and can reply with a rrep
		      else
			{
			  if(!recv_odr_msg->contents.odr_rreq.rrep_flag)
			    {
			      printf("Have valid route \n");
			      write_source_rrep(&odr_msg_rrep, recv_buf, send_buf, &pk_rreq);
			      printf("Wrote rrep \n");
			      Sendto(pk_sockfd, send_buf, ETH_FRAME_LEN, 0, (SA*) &pk_rreq, sizeof(struct sockaddr_ll));
			      printf("Sent rrep \n");
			      sent_rrep = 1;
			    }
			}
		    }
		  // NO ROUTE
		  else
		    {
		      printf("No route \n");
		      add_rpath(r_paths, &pk_rreq, &(recv_odr_msg->contents.odr_rreq));
		      memset(&pk_rreq, 0, sizeof(struct sockaddr_ll));
		      write_forward_rreq(send_buf, recv_buf,  &pk_rreq, recv_odr_msg->contents.odr_rreq.rrep_flag);
		      flood_rreqs(pk_sockfd, send_buf, &pk_rreq);
		    }
		}
	      /*
		Does the RREQ improve our knowledge of other nodes? 

		Two cases:
		If we sent an rrep from this node AND the rreq we received improves / is a new route
		we have to flood.
		
		If we didn't send an rrep then the flooding is taking care of above and all we have to do is
		update the route table

	      */
	      
	      //If we sent a rrep above we have to copy our recv buf to the send buf because the send buf will contain an rrep instead of an rreq
	      memset(send_buf, 0, ETH_FRAME_LEN);
	      memcpy(recv_buf, send_buf, ETH_FRAME_LEN);

	      
	      //update_route_table_rreq only needs to be done if improve the route
	      //so do it here independent of all sending
	      //improve route covers case that there is no route
	      if(improve_route(vm, &(recv_odr_msg->contents.odr_rreq)))
		{
		  printf("Improved route discovered...\n");
		  if(sent_rrep)
		    {
		      printf("Sent rrep already...\n");
		      update_route_table_rreq(vm, &pk_rreq, &(recv_odr_msg->contents.odr_rreq));
		      //need to set sent_rrep based on whether or not a rrep was sent from this node
		      write_forward_rreq(send_buf, recv_buf, &pk_rreq, sent_rrep);
		      flood_rreqs(pk_sockfd, send_buf, &pk_rreq);
		    }
		  //Since we haven't sent a rrep from this node the rreq will continue to be flooded
		  //All we have to do is update our own routing table
		  else
		    {
		      printf("Did not send rrep already...\n");
		      update_route_table_rreq(vm, &pk_rreq, &(recv_odr_msg->contents.odr_rreq));
		    }
		}
	      else if(reconfirm_route(vm, &(recv_odr_msg->contents.odr_rreq), staleness_param))
		{
		  printf("Reconfirm existing route...\n");
		  vm_entry_iter = get_route_entry(vm, recv_odr_msg->contents.odr_rreq.dest_addr);
		  setTimeStamp(vm_entry_iter);
		}

	      else if(new_neighbor_same_hops(vm, &(recv_odr_msg->contents.odr_rreq), &pk_rreq))
		{
		  printf("New neighbor same hops \n");
		  if(sent_rrep)
		    {
		      printf("Sent rrep already... \n");
		      update_route_table_rreq(vm, &pk_rreq, &(recv_odr_msg->contents.odr_rreq));
		      //need to set sent_rrep based on whether or not a rrep was sent from this node
		      write_forward_rreq(send_buf, recv_buf, &pk_rreq, sent_rrep);
		      flood_rreqs(pk_sockfd, send_buf, &pk_rreq);
		    }
		  else
		    {
		      printf("Didnt send rrep already... \n");
		      update_route_table_rreq(vm, &pk_rreq, &(recv_odr_msg->contents.odr_rreq));
		    }
		}
	    }
	  if(recv_odr_msg->type == RREP)
	    {
	      printf("Receive RREP... \n");
	      if(reached_src(&recv_odr_msg->contents.odr_rrep))
		{
		  printf("Reached src... \n");
		  update_route_table_rrep(vm, &pk_rreq, &(recv_odr_msg->contents.odr_rrep));
		  send_application_payload = 1;
		}
	      else
		{
		  printf("Did not reach src...forwarding rrep.... \n");
		  update_route_table_rrep(vm, &pk_rreq, &(recv_odr_msg->contents.odr_rrep));
		  write_forward_rrep(send_buf, recv_buf,  &pk_rreq, r_paths);
		  Sendto(pk_sockfd, send_buf, ETH_FRAME_LEN, 0, (SA*) &pk_rreq, sizeof(struct sockaddr_ll));
		}
	    }
	}
      
      //reset everything
      send_application_payload = 0;
      sent_rrep = 0;
      memset(send_buf, 0, ETH_FRAME_LEN);
      memset(recv_buf, 0, ETH_FRAME_LEN);
    }

  /*
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
	      //Update routing table for incoming RREQ
	      update_route_table_rreq(vm, &pk_rreq, &(recv_odr_msg->contents.odr_rrep));
	      memset(&pk_rreq, 0, sizeof(struct sockaddr_ll));
	      write_forward_rreq(send_buf, &pk_rreq, 0 );
	      flood_rreqs(pk_sockfd, send_buf, &pk_rreq);
		  
	    }
	    // Actually needs to be different logic 
	    //you can not have an rreq and still be the destination 
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
	  memset(send_buf, 0, ETH_FRAME_LEN);
	  write_forward_rrep(send_buf, recv_buf,  &pk_rreq, r_paths);
	  printf("After forward rrep \n");
	  Sendto(pk_sockfd, send_buf, ETH_FRAME_LEN, 0, (SA*) &pk_rreq, sizeof(struct sockaddr_ll));
	}
      
      memset(recv_buf, 0, ETH_FRAME_LEN);
      recvfrom(pk_sockfd, recv_buf, ETH_FRAME_LEN, 0, (SA *) &pk_rreq, &len);
    }
*/

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
write_forward_rrep(char * send_buf, char * recv_buf, struct sockaddr_ll *pk_rreq , struct rreq_reverse_path * r_paths)
{
  struct odr_message *recv_odr_msg_rrep = recv_buf + 14;
  struct rrep *rrep = &(recv_odr_msg_rrep->contents.odr_rrep);
  struct ethhdr *new_send_hdr = (struct ethhdr*) recv_buf;
  struct rreq_reverse_path *rrep_rpath;
  unsigned char src_addr[6];

  rrep_rpath = get_rpath(r_paths, rrep->dest_addr);

  printf("forward_rrep DestnAddr: %s\n", rrep->dest_addr);
  
  recv_odr_msg_rrep->contents.odr_rrep.hop_count++;

  printf("passed hopcount \n");
  memcpy(pk_rreq->sll_addr, rrep_rpath->prev_hop, ETH_ALEN);
  print_eth_addr(pk_rreq->sll_addr);
  printf("passed addr \n");

  pk_rreq->sll_ifindex =  rrep_rpath->in_interface_index;
  printf("sll_ifindex: %d \n", pk_rreq->sll_ifindex);
  printf("passed index  \n");
  memcpy(new_send_hdr->h_dest, rrep_rpath->prev_hop, ETH_ALEN);
  print_eth_addr(new_send_hdr->h_dest);
  printf("passed dest  \n");
  memcpy(new_send_hdr->h_source, get_source_ethaddr(src_addr , rrep_rpath->in_interface_index), ETH_ALEN);
  print_eth_addr(new_send_hdr->h_source);
  printf("passed h_source  \n");

  new_send_hdr->h_proto = htons(PROT_MAGIC);

  //modify recv_buf in place
  //copy to send_buf
  memcpy(send_buf, recv_buf, ETH_FRAME_LEN);
    
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

  memset(rpath, 0, REVERSE_PATH_SIZE * sizeof(struct rreq_reverse_path));
  
  for(i = 0; i < REVERSE_PATH_SIZE; i++)
    {
      rpath->b_id = -1;
      rpath++;
    }
}

/*
  Has this rreq gotten to us in less hops than a previous rreq?
*/

int
lower_hop_count(struct rreq_reverse_path* rpath, struct rreq* rreq)
{

  int i;
  for(i = 0; i < REVERSE_PATH_SIZE; i++)
   {
     printf("rreq->src_addr: %s \n", rreq->src_addr);
     printf("rpath->src_addr: %s \n", rpath->src_addr);
     if((!strcmp(rreq->src_addr, rpath->src_addr)) && (rreq->b_id == rpath->b_id))
       {
	 if(rpath->hop_count < rreq->hop_count)
	   {
	     printf("Return");
	     printf ("__FUNCTION__ = %s\n", __FUNCTION__);
	     return TRUE;
	   }
	 else
	   {
	     printf("Return");
	     printf ("__FUNCTION__ = %s\n", __FUNCTION__);
	     return FALSE;
	   }
       }
     rpath++;
   }

 printf("Return");
 printf ("__FUNCTION__ = %s\n", __FUNCTION__);
 return FALSE;
}

int
dont_have_rreq(struct rreq_reverse_path * rpath, struct rreq* rreq)
{
  int i;
  for(i = 0; i < REVERSE_PATH_SIZE; i++)
    {
      if((!strcmp(rreq->src_addr, rpath->src_addr)) && (rreq->b_id == rpath->b_id))
	{
	  printf("Return");
	  printf ("__FUNCTION__ = %s\n", __FUNCTION__);
	  return FALSE;
	}

    }

  printf("Return");
  printf ("__FUNCTION__ = %s\n", __FUNCTION__);
  return TRUE;
}


void
remove_rpath(struct rreq_reverse_path * rpaths,  struct rreq * rreq)
{
  int i;
  for(i = 0; i < ROUTING_TABLE_SIZE; i++)
    {
      if(rpaths->b_id == rreq->b_id && !strcmp(rpaths->src_addr, rreq->src_addr))
	{
	  memset(rpaths, 0, sizeof(struct rreq_reverse_path));
	  rpaths->b_id = -1;
	  return;
	}
      rpaths++;
    }
}


void
add_rpath(struct rreq_reverse_path * rpaths, struct sockaddr_ll * sock_rreq, struct rreq * rreq)
{

  printf ("__FUNCTION__ = %s\n", __FUNCTION__);
  int i;
  for(i = 0; i < ROUTING_TABLE_SIZE; i++)
    {
      if(rpaths->b_id == -1)
	{
	  rpaths->b_id = rreq->b_id;
	  printf("Reverse Path b_id: %d \n", rpaths->b_id);
	  memcpy(rpaths->src_addr, rreq->src_addr, 16);
	  printf("Reverse Path src_addr: %s \n", rpaths->src_addr);
	  memcpy(rpaths->dest_addr, rreq->dest_addr, 16);
	  memcpy(rpaths->prev_hop, sock_rreq->sll_addr, ETH_ALEN);
	  printf("Reverse Path prev_hop:\n");
	  print_eth_addr(rpaths->prev_hop);
	  rpaths->in_interface_index = sock_rreq->sll_ifindex;
	  printf("Reverse Path index: %d \n", rpaths->in_interface_index);
	  rpaths->hop_count = rreq->hop_count;
	  printf("Rpath hop count: %d \n", rpaths->hop_count);
	  break;
	}
      rpaths++;
    }
}

 
void 
delete_stale_entry(RT* entry)
{
  memset(entry->next_hop, 0, 6);
  entry->out_interface_index = -1;
  entry->hop_count = -1;
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

int
have_route(RT* entry)
{
  if(entry)
    {
      if(entry->next_hop[0] != 0)
	{
	  return TRUE;
	}
    }
  
  return FALSE;
      
}

struct rreq_reverse_path *
get_rpath(struct rreq_reverse_path * r_paths, char * dest_addr)
{
  int i;
  for(i = 0; i < REVERSE_PATH_SIZE; i++)
    {
      if(!strcmp(r_paths->dest_addr, dest_addr))
	return r_paths;

      r_paths++;
    }
  return NULL;
}

int
new_neighbor_same_hops(struct routing_table * vm, struct rreq* rreq, struct sockaddr_ll* sock_rreq)
{
  //route to src_addr
  struct routing_table *entry  = get_route_entry(vm, rreq->src_addr);
  if(entry)
    {
      //if new address
      if(memcmp(entry->next_hop, sock_rreq->sll_addr, 6))
	{
	  if(entry->hop_count == rreq->hop_count)
	    {
	      return TRUE;
	    }
	}
    }

  return FALSE;
}

 
int
reconfirm_route(RT* vm, struct rreq* rreq, struct timespec staleness_param)
{
  RT* entry = NULL;
  entry = get_route_entry(vm, rreq->dest_addr);
  if(entry)
    {
      if (!isRoutingTableStale(vm, staleness_param))
	{
	  if(entry->hop_count == rreq->hop_count)
	    return TRUE;
	}
    }

  return FALSE;
}


/*
  Improve route will also cover the case that we DON'T have a route i.e. the vm->hop_count == -1

  Should always have a route by the time we get to improve_route. If we don't, return FALSE because something
  must have gone wrong.
 */
  
int
improve_route(RT* vm, struct rreq* rreq)
{
  int i;
  for(i = 0; i < ROUTING_TABLE_SIZE; i++)
    {
      if(!strcmp(vm->dest_addr, rreq->src_addr))
	{
	  if(vm->hop_count > rreq->hop_count || vm->hop_count == -1)
	    return TRUE;
	  else
	    return FALSE;
	}
    }
  return FALSE;
}

/*
//XX
//We need to check if it is better than the previous route...??
Set forward pointers in response to rrep

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
/*
  Set forward ptrs in response to a rreq
  Notice that we compare to rreq->src_addr here
  We want to update our table if the route to the rreq source is better than the one we knew

 */
void update_route_table_rreq(RT* vm, struct sockaddr_ll* sock_rreq,  struct rreq* rreq)
{

  RT* entry; 
  entry = get_route_entry(vm, rreq->src_addr);
  if(entry->hop_count > rreq->hop_count || entry->hop_count == -1)
    {
      //set forward address
      memcpy(entry->next_hop, sock_rreq->sll_addr, ETH_ALEN);

      //set interface index
      entry->out_interface_index = sock_rreq->sll_ifindex;

      //set hop count
      entry->hop_count = rreq->hop_count;

      setTimeStamp(entry);


    }
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
	}
    }
}


int reached_src(struct rrep * rrep)
{
  char own_ip[16];
  findOwnIP(own_ip);
    
  if (!strcmp(rrep->src_addr, own_ip))
    return TRUE;
  else
    return FALSE;


}


int reached_destination(struct rreq * rreq)
{
  char own_ip[16];
  findOwnIP(own_ip);
  

  if(!strcmp(rreq->dest_addr, own_ip))
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
  memcpy(rrep->contents.odr_rrep.src_addr, rreq->contents.odr_rreq.src_addr, 16);

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
  char own_ip[16];

  /* Set ethernet frame ptrs */
  hdr = (struct ethhdr *) buffer;
  data = buffer + 14;

  findOwnIP(own_ip);

  /* Initialize RREQ */
  odr_msg_rreq.type = 0;
  memcpy(odr_msg_rreq.contents.odr_rreq.src_addr, own_ip, 16);
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


void write_forward_rreq( char * send_buf, char * recv_buf, struct sockaddr_ll* sockaddr_rreq, int rrep_sent)
{

  struct odr_message *forward_odr_msg_rreq;

  memset(send_buf, 0, ETH_FRAME_LEN);
  memcpy(send_buf, recv_buf, ETH_FRAME_LEN);

  /* Set ethernet frame ptrs */
  forward_odr_msg_rreq = (struct odr_message *) send_buf + 14;
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
  
  
  if(clock_gettime(CLOCK_REALTIME, &vm->timestamp) == -1){
    perror("setTimeStamp: ");
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
	if (strcmp(own_ip, (req->dest_addr)) == 0 )
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

/* 
   Initialize Routing Table for all nodes 
*/

void init_RoutingTable(RT *vm){

  int i;
  
  for ( i = 0; i < 10; i++ )
    {
      strcpy( vm[i].dest_addr, ipVM[i]);
      init_RoutingEntry(&vm[i]);
      setTimeStamp(&vm[i]);
    }
}

/* Initialize Routing Table for any particular VM */
void init_RoutingEntry( RT *vm_node ){

	vm_node->next_hop[0] = 0;
	vm_node->out_interface_index = -1;
	vm_node->hop_count = -1;

}

