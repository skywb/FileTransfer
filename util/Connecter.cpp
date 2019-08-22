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
  writeable_ = false;
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
  if (-1 == pipe(pipefd_)) {
    std::cout << "打开管道失败" << std::endl;
    return;
  }
  event_.data.fd = pipefd_[0];
  event_.events = EPOLLIN;
  epoll_ctl(epoll_root_, EPOLL_CTL_ADD, pipefd_[0], &event_);
  for (auto i = sockets.cbegin(); i != sockets.cend(); ++i) {
    event_.data.fd = *i;
    event_.events = EPOLLIN;
    epoll_ctl(epoll_root_, EPOLL_CTL_ADD, *i, &event_);
    event_.events = EPOLLOUT;
    epoll_ctl(epoll_root_, EPOLL_CTL_ADD, *i, &event_);
  }
  sockfd_iter = sockets.cbegin();
  writeable_ = true;
}

Connecter::~Connecter() {
  close(epoll_root_);
  for (auto i : sockets) {
    close(i);
  }
}

int Connecter::Recv(char* buf, int len) {
  std::lock_guard<std::mutex> lock_guard(lock_read_);
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
  std::lock_guard<std::mutex> lock_guard(lock_write_);
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
  epoll_event events[sockets.size() * 2+1];
  int cnt = 0;
  bool readable = false;
  bool writeable = false;
  while (true) {
    cnt = epoll_wait(epoll_root_, events, sockets.size()*2+1, time_millsec);
    if (cnt > 0) {
      for (int i=0; i<cnt; ++i) {
        if (events[i].data.fd == pipefd_[0]) {
          //需要监听可写事件
          char buf[10];
          int re = 1;
          while (re > 0) re = read(pipefd_[0], buf, 10);
          epoll_event event;
          for (auto i : sockets) {
            event.data.fd = i;
            event.events = EPOLLIN | EPOLLOUT;
            epoll_ctl(epoll_root_, EPOLL_CTL_MOD, i, &event);
          }
          continue;
        } 
        if (events[i].events & EPOLLOUT) {
          writeable = true;
          writeable_ = true;
          epoll_event event;
          for (auto i : sockets) {
            event.data.fd = i;
            event.events = EPOLLIN;
            epoll_ctl(epoll_root_, EPOLL_CTL_MOD, i, &event);
          }
        }
        if (events[i].events & EPOLLIN) {
          readable = true;
        }
      }
    } else {
      return Connecter::kOutTime;
    }
  }
  if (readable && writeable) return kAll;
  else if (readable) return kRead;
  else if (writeable) return kWrite;
  return kOutTime;
}



bool Connecter::AddListenWriteableEvent() {
  std::lock_guard<std::mutex> lock(lock_write_);
  writeable_ = false;
  write(pipefd_[1], "1", 1);
  return true;
}
