#include "reactor/connect.h"
#include "reactor/CMD.h"
#include "send/fileSend.h"

#include <cstring>

int main(int argc, char *argv[])
{
  std::string ip = argv[1];
  int port = 8888;
  sockaddr_in addr;
  std::cout << "ip is " << ip << std::endl;
	int sockfd = Connect(ip, port, &addr);
  while(true) {
    char buf[kMaxLength];
    *(CMD*)buf = kResend;
    int num = 0;
    std::cin >> num;
    *(int*)(buf+4) = num;
    strcpy(buf+8, "testFile");

    sendto(sockfd, buf, 8+strlen(buf+8), 0, (sockaddr*)&addr, sizeof(addr));
  }
	return 0;
}
