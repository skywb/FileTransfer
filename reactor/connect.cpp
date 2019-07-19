#include "connect.h"
#include "CMD.h"

#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <cstring>
void joinGroup();

int connect(std::string& ip, int& port, sockaddr_in* addr) {
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	addr->sin_port = htons(8888);
	addr->sin_family = AF_INET;
	inet_pton(AF_INET, ip.c_str(), &addr->sin_addr);
	const int kBufSize = 1000;
	char buf[kBufSize];
  CMD cmd = kUnavailable;
	*(CMD*)buf = kRequest;
	int re = sendto(sockfd, buf, sizeof(CMD), 0, (sockaddr*)addr, sizeof(*addr));
	if (-1 == re) {
    std::cerr << "sendto error" << std::endl;
		std::cout << strerror(errno) << std::endl;
	}
  re = recv(sockfd, buf, kBufSize, 0);
  buf[re] = 0;
	if (-1 == re) {
    std::cout << "recv失败" << std::endl;
    std::cout << strerror(errno) << std::endl;
  } else {
    buf[re] = 0;
    std::cout << buf << std::endl;
    ip = std::string(buf);
    port = 8888;
  }
	return sockfd;
}
