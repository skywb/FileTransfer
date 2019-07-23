#include "recv/fileRecv.h"
#include "reactor/connect.h"

#include <arpa/inet.h>

int main(int argc, char *argv[])
{
  sockaddr_in addr;
  ip_mreq join_adr;
  std::string ip = "224.0.0.11";
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	addr.sin_port = htons(8888);
	addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  if(bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    printf("bind() error");

  //int time_live = 64;
  //setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, 
  //           (void *)&time_live, sizeof(time_live));

  join_adr.imr_multiaddr.s_addr = inet_addr(ip.c_str());
  join_adr.imr_interface.s_addr = htonl(INADDR_ANY);
  setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&join_adr, sizeof(join_adr));
  auto file = std::make_unique<File> ("recvFile", 1000, true);
  FileRecv(sockfd, &addr, file);
  return 0;
}
