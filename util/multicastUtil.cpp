#include "multicastUtil.h"

#include <arpa/inet.h>
#include <cstring>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <ifaddrs.h>  


bool JoinGroup (sockaddr_in* addr, int* sockfd, std::string group_ip, int port, std::string netcard_ip){
  ip_mreq join_adr;
	*sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	addr->sin_port = htons(port);
	addr->sin_family = AF_INET;
  addr->sin_addr.s_addr =inet_addr(netcard_ip.c_str());
  //int opt = 1;
  //// sockfd为需要端口复用的套接字
  //setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));
  if(bind(*sockfd, (sockaddr *)addr, sizeof(*addr)) == -1) {
    //std::cout << netcard_ip << std::endl;
    //std::cerr << "bind() error in file " << __FILE__ << std::endl;
    //std::cout << strerror(errno) << std::endl;
    *sockfd = -1;
    close(*sockfd);
    return false;
    //exit(1);
  }
  join_adr.imr_multiaddr.s_addr = inet_addr(group_ip.c_str());
  join_adr.imr_interface.s_addr = inet_addr(netcard_ip.c_str());
  setsockopt(*sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&join_adr, sizeof(join_adr));
  unsigned long if_addr = inet_addr(netcard_ip.c_str());
  int ret = setsockopt(*sockfd, IPPROTO_IP, IP_MULTICAST_IF, (const char*)&if_addr, sizeof(if_addr));
  addr->sin_addr.s_addr = inet_addr(group_ip.c_str());
}

//返回所有网卡IP列表
std::vector<std::string> GetAllNetIP() {
    struct sockaddr_in *sin = NULL;
    struct ifaddrs *ifa = NULL, *ifList;
    //所有网卡地址列表
    std::vector<std::string> res;
    if (getifaddrs(&ifList) < 0) {
      return res;
    }

    for (ifa = ifList; ifa != NULL; ifa = ifa->ifa_next) {
        if(ifa->ifa_addr->sa_family == AF_INET) {
            //printf("\n>>> interfaceName: %s\n", ifa->ifa_name);
            sin = (struct sockaddr_in *)ifa->ifa_addr;
            //printf(">>> ipAddress: %s\n", inet_ntoa(sin->sin_addr));
            res.push_back(std::string(inet_ntoa(sin->sin_addr)));
            //sin = (struct sockaddr_in *)ifa->ifa_dstaddr;
            //printf(">>> broadcast: %s\n", inet_ntoa(sin->sin_addr));
            //sin = (struct sockaddr_in *)ifa->ifa_netmask;
            //printf(">>> subnetMask: %s\n", inet_ntoa(sin->sin_addr));
        }
    }
    freeifaddrs(ifList);
    return res;
}

