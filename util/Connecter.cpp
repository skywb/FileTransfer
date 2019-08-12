#include "Connecter.h"
#include "send/fileSend.h"
#include "multicastUtil.h"
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>

Connecter::Connecter (std::string group_ip, int port) {
  addr_.sin_addr.s_addr = inet_addr(group_ip.c_str());
  addr_.sin_family = AF_INET;
  addr_.sin_port = htons(port);
  auto ip_vec = GetAllNetIP();
  auto addr = new sockaddr_in();
  for (int i = 0; i < ip_vec.size(); ++i) {
    int sockfd;
    not_Listen_ip_[inet_addr(ip_vec[i].c_str())] = true;
//#ifdef __WIN32__
//    if (Bind(addr, &sockfd, ip_vec[i], port)) {             //Linux 不接 发  Win   发  接
//#elif __LINUX__
//#else
    //绑定成功则加入组播地址， 否则忽略该ip
    if (Bind(addr, &sockfd, group_ip, port)) {            //Linux 接  发      Win  绑定失败
//#endif
      if (true == JoinGroup(&sockfd, group_ip, ip_vec[i])) {
        std::cout << ip_vec[i] << "加入组播成功" << std::endl;
        sockets.push_back(sockfd);
      }
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
  close(epoll_root_);
  for (auto i : sockets) {
    close(i);
  }
}

int Connecter::Recv(char* buf, int len, int timeout) {
    while (true) {
        int cnt = epoll_wait(epoll_root_, &event_, 1, timeout);
        std::lock_guard<std::mutex> lock_guard(lock_);
        if (cnt > 0) {
            sockaddr_in addr;
            socklen_t addr_len = sizeof(addr);
            int re = recvfrom(event_.data.fd, buf, len, 0, (sockaddr*)&addr, &addr_len);
            if (re == -1) {
                perror("error in Recv");
            }
            if (not_Listen_ip_[addr.sin_addr.s_addr]) continue;
            return re;
        } else {
            if (cnt == -1) {
                std::cout << strerror(errno) << std::endl;
                return -1;
            }
            return -1;
        }
    }
    return 0;
}

int Connecter::Send(char* buf, int len) {
  std::lock_guard<std::mutex> lock_guard(lock_);
  for (auto i : sockets) {
    int re = sendto(i, buf, len, 0, (sockaddr*)&addr_, sizeof(sockaddr_in));
    if (re == -1) {
      std::cout << inet_ntoa(addr_.sin_addr) << " 发送失败" << std::endl;
      std::cout << strerror(errno) << std::endl;
      return -1;
    } else {
      //std::cout << inet_ntoa(addr_.sin_addr) << " port " << htons(addr_.sin_port) << " 发送" << re << "字节" << std::endl;
    }
  }
  return 0;
}
  
