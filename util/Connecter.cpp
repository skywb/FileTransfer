#include "Connecter.h"
#include "multicastUtil.h"
#include <arpa/inet.h>
#include <cstring>

Connecter::Connecter (std::string group_ip, int port) {
  auto ip_vec = GetAllNetIP();
  for (int i = 0; i < ip_vec.size(); ++i) {
    auto addr = new sockaddr_in();
    int sockfd;
    if (true == JoinGroup(addr, &sockfd, group_ip, port, ip_vec[i])) {
      std::cout << ip_vec[i] << " 绑定成功" << std::endl;
      addrs_.push_back(std::make_pair(sockfd, addr));
    } else {
      delete  addr;
    }
  }
  epoll_root_ = epoll_create(10);
  int sock_cnt = addrs_.size();
  for (int i = 0; i < sock_cnt; ++i) {
    event_.data.fd = addrs_[i].first;
    event_.events = EPOLLIN;
    epoll_ctl(epoll_root_, EPOLL_CTL_ADD, addrs_[i].first, &event_);
    std::cout << inet_ntoa(addrs_[i].second->sin_addr)  << " 加入成功"<< std::endl;
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
  std::cout << "epoll wait return cnt = " << cnt << std::endl;
  std::lock_guard<std::mutex> lock_guard(lock_);
  if (cnt > 0) {
    std::cout << "epoll return " << std::endl;
    return recvfrom(event_.data.fd, buf, len, 0, nullptr, nullptr);
  } else {
    return -1;
  }
}
int Connecter::Send(char* buf, int len) {
  std::lock_guard<std::mutex> lock_guard(lock_);
  for (auto i : addrs_) {
    i.second->sin_addr.s_addr = inet_addr("224.0.2.11");
    int re = sendto(i.first, buf, len, 0, (sockaddr*)i.second, sizeof(sockaddr_in));
    if (re == -1) {
      std::cout << inet_ntoa(i.second->sin_addr) << " 发送失败" << std::endl;
    } else {
      std::cout << inet_ntoa(i.second->sin_addr) << " port " << htons(i.second->sin_port) << " 发送" << re << "字节" << std::endl;
    }
  }
  return 0;
}
  
