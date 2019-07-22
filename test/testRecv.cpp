#include "recv/fileRecv.h"
#include "reactor/connect.h"

#include <arpa/inet.h>

int main(int argc, char *argv[])
{
  sockaddr_in addr;
  std::string ip = "223.0.0.11";
  //int port = atoi(argv[2]);
  //int sockfd = Connect(ip, port, &addr);
  //std::cout << "加入成功" << std::endl;
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	addr.sin_port = htons(8888);
	addr.sin_family = AF_INET;
	inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
  int time_live = 64;
  setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, 
             (void *)&time_live, sizeof(time_live));
  FileRecv(sockfd, &addr);

  return 0;
}
