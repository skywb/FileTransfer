#ifndef CONNECTER_H_7LVEHFUI
#define CONNECTER_H_7LVEHFUI

#include <string>
#include <vector>
#include <map>
#include <netinet/ip.h>
#include <sys/epoll.h>
#include <mutex>


class Connecter
{
public:
  Connecter (std::string group_ip, int port);
  virtual ~Connecter ();
  int Recv(char* buf, int len, int timeout);
  int Send(char* buf, int len);
private:
  std::vector<int> sockets;
  int epoll_root_;
  epoll_event event_;
  std::mutex lock_;
  sockaddr_in addr_;
};

#endif /* end of include guard: CONNECTER_H_7LVEHFUI */
