#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include "util/multicastUtil.h"

int main(int argc, char *argv[])
{
  int sockfd;
  sockaddr_in addr;
  if (false == Bind(&addr, &sockfd, std::string(), 8888)) {
    std::cout << "bind error " << strerror(errno) << std::endl;
  }
  auto ips = GetAllNetIP();
  for (auto i : ips) {
    if (false == JoinGroup(&sockfd, std::string("224.0.2.11"), i)) {
      std::cout << i << "JoinGroup error" << strerror(errno) << std::endl;
    }
  }
  addr.sin_addr.s_addr = inet_addr("224.0.2.11");
  addr.sin_port = 8888;
  addr.sin_family = AF_INET;
  while (true) {
    sendto(sockfd, "hello", 5, 0, (sockaddr*)&addr, sizeof(sockaddr_in));
    sleep(1);
  }
  return 0;
}
