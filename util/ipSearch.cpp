#include "ipSearch.h"

#include <arpa/inet.h>
#include <cstring>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string>

std::string Ip_search() {
  int                 sockfd;
  struct sockaddr_in  sin;
  struct ifreq        ifr;

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd == -1) {
    perror("socket error");
    exit(1);
  }
  strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);      //Interface name

  if (ioctl(sockfd, SIOCGIFADDR, &ifr) == 0) {    //SIOCGIFADDR 获取interface address
    memcpy(&sin, &ifr.ifr_addr, sizeof(ifr.ifr_addr));
    return inet_ntoa(sin.sin_addr);
  }
  return std::string();
}
