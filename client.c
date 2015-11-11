#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "unp.h"
#include "a3.h"

void init_sockaddr_un(struct sockaddr_un *addr, char * path);

int main(int argc, char **argv)
{
  int fd = 0,  err = 0, sockfd = 0, one = 1;
  //null terminated
  //char ds_cli_path[] = "client_sock_XXXXXX";
  //char* ds_odr_path = "/home/mliuzzi/odr_path";
  // Paths for testing are uncommented
  char *ds_odr_path = "/users/mliuzzi/cse533/un_odr_path";
  // has newline
  char vm_choice[10] = "";
  struct sockaddr_un cliaddr, ds_odr;

  /*
  fd = mkstemp(ds_cli_path);
  if(fd == -1)
    {
      perror("Could not create temp file");
      exit(1);
    }
  */

  printf("Enter server node now \n");
  fgets(vm_choice, 10, stdin);
  printf("You have chosen: %s", vm_choice);

  sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
  Setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

  //init_sockaddr_un(&cliaddr, ds_cli_path);
  init_sockaddr_un(&cliaddr, tempnam("./", "ud_"));
  printf("Filename: %s", cliaddr.sun_path);
  Bind(sockfd, (SA *) &cliaddr, sizeof(cliaddr));

  init_sockaddr_un(&ds_odr, ds_odr_path);

  dg_cli(stdin, sockfd, (SA *) &ds_odr, sizeof(ds_odr));

  //msg_send(sockfd, recv_buf, )


  /*
    CLOSE PROGRAM
  */

  if ((err = close(fd)))
      perror("Error: ");
      

  //unlink(ds_cli_path);  
  unlink(cliaddr.sun_path);  
  exit(0);
}




