#include "Connecter.h"
#include "multicastUtil.h"
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>

Connecter::Connecter (std::string group_ip, int port) {
  std::cout << "hello world" << std::endl;
  sleep(1); 
  addr_.sin_addr.s_addr = inet_addr(group_ip.c_str());
  addr_.sin_family = AF_INET;
  addr_.sin_port = htons(port);
  //sockets.push_back(socket(AF_INET, SOCK_DGRAM, 0));
  auto ip_vec = GetAllNetIP();
  auto addr = new sockaddr_in();
  std::cout << ip_vec.size() << std::endl;
  for (int i = 0; i < ip_vec.size(); ++i) {
    int sockfd;
    //int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (Bind(addr, &sockfd, ip_vec[i], port)) {             //Linux 不接 发  Win   发  接
    //if (Bind(addr, &sockfd, group_ip, port)) {            //Linux 接  发      Win  绑定失败
    //if (Bind(addr, &sockfd, std::string(), port)) {       //Linux 接 发     Win 接  不发
      if (true == JoinGroup(&sockfd, group_ip, ip_vec[i])) {
        sockets.push_back(sockfd);
      }
    } else {
      std::cout << ip_vec[i] << " 绑定失败" << std::endl;
    }
  }
  delete addr;
  epoll_root_ = epoll_create(10);
  for (int i = 0; i < sockets.size(); ++i) {
    event_.data.fd = sockets[i];
    event_.events = EPOLLIN;
    epoll_ctl(epoll_root_, EPOLL_CTL_ADD, sockets[i], &event_);
  }
}

Connecter::~Connecter() {
}

int Connecter::Recv(char* buf, int len, int timeout) {
  int cnt = epoll_wait(epoll_root_, &event_, 1, timeout);
  std::lock_guard<std::mutex> lock_guard(lock_);
  if (cnt > 0) {
    return recvfrom(event_.data.fd, buf, len, 0, nullptr, nullptr);
  } else {
    return -1;
  }
}

int Connecter::Send(char* buf, int len) {
  std::lock_guard<std::mutex> lock_guard(lock_);
  for (auto i : sockets) {
    //int re = sendto(i, buf, len, 0, (sockaddr*)&addr_, sizeof(sockaddr_in));
    connect(i, (sockaddr*)&addr_, sizeof(sockaddr_in));
    int re = send(i, buf, len, 0);
    if (re == -1) {
      std::cout << inet_ntoa(addr_.sin_addr) << " 发送失败" << std::endl;
      std::cout << strerror(errno) << std::endl;
    } else {
      std::cout << inet_ntoa(addr_.sin_addr) << " port " << htons(addr_.sin_port) << " 发送" << re << "字节" << std::endl;
    }
  }
  return 0;
}
  
