#include "a3.h"


void msg_send(int sockfd, char* dest_ip, int dest_port, char* msg, int flag)
{

  struct msg_send_struct formatted_msg;
  char buff[ETH_FRAME_LEN];
  
  memset(buff, 0, ETH_FRAME_LEN);
  memcpy(formatted_msg.dest_ip, dest_ip, 16);
  formatted_msg.dest_port = dest_port;
  formatted_msg.flag = flag;
  
  memcpy(buff, &formatted_msg, sizeof(struct msg_send_struct));
  
  Send(sockfd, buff, ETH_FRAME_LEN, 0);
  
}
