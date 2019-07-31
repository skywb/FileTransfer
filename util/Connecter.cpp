#include "Connecter.h"
#include "multicastUtil.h"
#include <arpa/inet.h>

Connecter::Connecter (std::string group_ip, int port) {
  auto ip_vec = GetAllNetIP();
  for (int i = 0; i < ip_vec.size(); ++i) {
    auto addr = new sockaddr_in();
    int sockfd;
    if (false == JoinGroup(addr, &sockfd, group_ip, port, ip_vec[i])) {
      //std::cout << "加入组播组失败" << std::endl;
    } else {
      addrs_.push_back(std::make_pair(sockfd, addr));
    }
  }
  epoll_root_ = epoll_create(10);
  int sock_cnt = addrs_.size();
  for (int i = 0; i < sock_cnt; ++i) {
    event_.data.fd = addrs_[i].first;
    event_.events = EPOLLIN;
    epoll_ctl(epoll_root_, EPOLL_CTL_ADD, addrs_[i].first, &event_);
  }
}

Connecter::~Connecter() {
  for (auto i : addrs_) {
    delete i.second;
  }
}

int Connecter::Recv(char* buf, int len, int timeout) {
  //for (auto i : addrs_) {
  //  std::cout << inet_ntoa(i.second->sin_addr) << std::endl;
  //}
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
  for (auto i : addrs_) {
    int re = sendto(i.first, buf, len, 0, (sockaddr*)i.second, sizeof(sockaddr_in));
    if (re == -1) {
      std::cout << inet_ntoa(i.second->sin_addr) << " 发送失败" << std::endl;
    } else {
      std::cout << inet_ntoa(i.second->sin_addr) << " port " << htons(i.second->sin_port) << " 发送" << re << "字节" << std::endl;
    }
  }
  return 0;
}
  
