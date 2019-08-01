#include <iostream>
#include <arpa/inet.h>
#include "util/multicastUtil.h"

int main(int argc, char *argv[])
{
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  sockaddr_in addr;
  addr.sin_addr.s_addr = inet_addr("172.18.146.35");
  addr.sin_port = htons(8888);
  addr.sin_family = AF_INET;


  JoinGroup(&sockfd, std::string("224.0.2.11"), "172.18.146.35");

  char buf[1000];
  while (true) {
    int re = recv(sockfd, buf, 1000, 0);
    std::cout << buf << std::endl;
  }
  return 0;
}
