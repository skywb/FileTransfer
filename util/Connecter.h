#ifndef CONNECTER_H_7LVEHFUI
#define CONNECTER_H_7LVEHFUI

#include <string>
#include <vector>
#include <map>
#include <netinet/ip.h>
#include <sys/epoll.h>
#include <mutex>

class Connecter {
public:
  Connecter (std::string group_ip, int port);
  virtual ~Connecter ();
  /* 接收消息， 写到缓冲区中， 最大写了长度，
   * 设置超时退出， 设为-1则阻塞等待
   */
  int Recv(char* buf, int len, int timeout);
  /* 发送数据 */
  int Send(char* buf, int len);
private:
  std::vector<int> sockets;
  int epoll_root_;
  epoll_event event_;
  std::mutex lock_;
  sockaddr_in addr_;
  std::map<uint32_t, bool> not_Listen_ip_;
};

#endif /* end of include guard: CONNECTER_H_7LVEHFUI */
