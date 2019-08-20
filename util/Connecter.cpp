#include "Connecter.h"
#include "send/fileSend.h"
#include "multicastUtil.h"
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <unistd.h>
#include <fcntl.h>

static void SetNoBlock(int sockfd) {
  auto flags = fcntl(sockfd, F_GETFL, 0);
  fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

Connecter::Connecter (std::string group_ip, int port) {
  addr_.sin_addr.s_addr = inet_addr(group_ip.c_str());
  addr_.sin_family = AF_INET;
  addr_.sin_port = htons(port);
  auto ip_vec = GetAllNetIP();
  auto addr = new sockaddr_in();
  for (int i = 0; i < ip_vec.size(); ++i) {
    int sockfd;
    not_Listen_ip_[inet_addr(ip_vec[i].c_str())] = true;
    //绑定成功则加入组播地址， 否则忽略该ip
    if (Bind(addr, &sockfd, group_ip, port)) {            //Linux 接  发      Win  绑定失败
      if (true == JoinGroup(&sockfd, group_ip, ip_vec[i])) {
        std::cout << ip_vec[i] << "加入组播成功" << std::endl;
        SetNoBlock(sockfd);
        sockets.push_back(sockfd);
      }
    }
  }
  delete addr;
  epoll_root_ = epoll_create(10);

  for (auto i = sockets.cbegin(); i != sockets.cend(); ++i) {
    event_.data.fd = *i;
    event_.events = EPOLLIN;
    epoll_ctl(epoll_root_, EPOLL_CTL_ADD, *i, &event_);
    event_.events = EPOLLOUT;
    epoll_ctl(epoll_root_, EPOLL_CTL_ADD, *i, &event_);
  }
  sockfd_iter = sockets.cbegin();
}

Connecter::~Connecter() {
  close(epoll_root_);
  for (auto i : sockets) {
    close(i);
  }
}

int Connecter::Recv(char* buf, int len) {
  std::lock_guard<std::mutex> lock_guard(lock_);
  sockaddr_in addr;
  socklen_t addr_len = sizeof(addr);
  if (sockets.size() <= 0) return -1;
  for (int i=0; i<sockets.size(); ++i) {
    if (sockfd_iter == sockets.cend()) sockfd_iter = sockets.cbegin();
    int re = recvfrom(*sockfd_iter, buf, len, 0, (sockaddr*)&addr, &addr_len);
    ++sockfd_iter;
    if (re > 0) return re;
  }
  return -1;
}

int Connecter::Send(char* buf, int len) {
  std::lock_guard<std::mutex> lock_guard(lock_);
  int cnt = 0;
  for (auto i : sockets) {
    int re = sendto(i, buf, len, 0, (sockaddr*)&addr_, sizeof(sockaddr_in));
    if (re > 0) {
      cnt++;
    }
  }
  if (cnt == 0) cnt = -1;
  return cnt;
}

Connecter::Type Connecter::Wait(Type type, int time_millsec) {
  epoll_event events[sockets.size() * 2];
  int cnt = epoll_wait(epoll_root_, events, sockets.size()*2, time_millsec);
  bool readable = false;
  bool writeable = false;
  for (int i=0; i<cnt; ++i) {
    if (events[i].events == EPOLLOUT) writeable = true;
    else if (events[i].events == EPOLLIN) readable = true;
  }
  if (readable && writeable) return kAll;
  else if (readable) return kRead;
  else if (writeable) return kWrite;
  return kOutTime;
}
