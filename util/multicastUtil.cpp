#include "multicastUtil.h"

#include <arpa/inet.h>
#include <cstring>


bool JoinGroup (sockaddr_in* addr, int* sockfd, std::string group_ip, int port){
  ip_mreq join_adr;
	*sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	addr->sin_port = htons(port);
	addr->sin_family = AF_INET;
  addr->sin_addr.s_addr = htonl(INADDR_ANY);
  int opt = 1;
  // sockfd为需要端口复用的套接字
  setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));
  if(bind(*sockfd, (sockaddr *)addr, sizeof(*addr)) == -1) {
    std::cerr << "bind() error" << std::endl;
    std::cout << strerror(errno) << std::endl;
    exit(1);
  }
  join_adr.imr_multiaddr.s_addr = inet_addr(group_ip.c_str());
  join_adr.imr_interface.s_addr = htonl(INADDR_ANY);
  setsockopt(*sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&join_adr, sizeof(join_adr));
  return true;
}

