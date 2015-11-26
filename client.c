#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "unp.h"
#include "a3.h"

void init_sockaddr_un(struct sockaddr_un *addr, char * path);

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


void client_debug_send(char* map, char * dest_ip);

int main(int argc, char **argv)
{
  int fd = 0,  err = 0, sockfd = 0, one = 1;
  //null terminated
  //char ds_cli_path[] = "client_sock_XXXXXX";
  //char* ds_odr_path = "/home/mliuzzi/odr_path";
  // Paths for testing are uncommented
  char *ds_odr_path = "/home/mliuzzi/un_odr_path1";
  // has newline
  int vm_choice;
	char ip[16];
  struct sockaddr_un cliaddr, ds_odr;

  /*
  fd = mkstemp(ds_cli_path);
  if(fd == -1)
    {
      perror("Could not create temp file");
      exit(1);
    }
  */
	int flag = 0;

	while (1){
  	printf("Enter server node now from 1 to 10 \n");
  	scanf("%d", &vm_choice);
  	printf("You have chosen: %d", vm_choice);
	
		if((vm_choice > 10) || (vm_choice < 1) ){
			printf("Invalid choice of vm. Please try again\n");
			continue;
		}
		
  	sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
  	Setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

  	init_sockaddr_un(&cliaddr, tempnam("./", "ud_"));
  	printf("Filename: %s", cliaddr.sun_path);
  	Bind(sockfd, (SA *) &cliaddr, sizeof(cliaddr));

  	init_sockaddr_un(&ds_odr, ds_odr_path);
  	Connect(sockfd, (SA *) & ds_odr, sizeof(struct sockaddr_un));
		
		sprintf(ip, "130.245.156.%d", (vm_choice%10)+20);
		
  	msg_send(sockfd, ip, 2039, "1", flag);
  	client_debug_send(ipVM, ip);

  /*
    CLOSE PROGRAM
  */

  if ((err = close(fd)))
      perror("Error: ");
      

  //unlink(ds_cli_path);  
  unlink(cliaddr.sun_path);  
  exit(0);
}

void
client_debug_send(char* map, char * dest_ip)
{
  char own_ip[16];
  int i, vm_src = 0 , vm_dest = 0;
  findOwnIP(own_ip);

  //vm1 through vm10
  for(i = 1; i < 11; i++)
    {
      if(!strcmp(dest_ip, map))
	{
	  vm_dest = i;
	  printf("vm dest: %d \n", vm_dest);
	}
      
      if(!strcmp(own_ip, map))
	{
	  vm_src = i;
	  printf("vm dest: %d \n", vm_src);
	}

      map = map+16;
      
    }

  if(vm_dest && vm_src)
    {
      printf("Client at node vm:%d sending request to client at node vm: %d \n", vm_src, vm_dest);
    }
  
	else
    printf("Could not find client or dest in map \n");

}
