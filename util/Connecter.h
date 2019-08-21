#ifndef CONNECTER_H_7LVEHFUI
#define CONNECTER_H_7LVEHFUI

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <list>
#include <queue>
#include <netinet/ip.h>
#include <sys/epoll.h>
#include <mutex>
#include <condition_variable>

class Connecter {
public:
  enum Type {
    kNull    = 0,
    kRead    = 1,
    kWrite   = 1 << 1,
    kAll     = kRead | kWrite,
    kOutTime = 1 << 2
  };
public:
  Connecter (std::string group_ip, int port);
  virtual ~Connecter ();

  /* 等待可读或者可写或者任意一个， 返回可读或者可写的Type
   * time_millsec为超时时间，若在time_millsec 毫秒内没有要等待的类型，则返回kOutTime
   * 若time_millsec 为-1 则阻塞等待
   */
  virtual Type Wait(Type type, int time_millsec);
  //非阻塞读， 没有数据返回-1
  virtual int Recv(char* buf, int len);
  /* 非阻塞发送数据, 无法发送的返回-1 */
  virtual int Send(char* buf, int len);
  virtual bool WaitReadable();
  virtual bool WaitWriteable();
  operator bool() {
    return true;
  }
private:
  //std::vector<int> sockets;
  std::list<int> sockets;
  std::list<int>::const_iterator sockfd_iter;
  int pipefd_[2];
  int epoll_root_;
  epoll_event event_;
  //std::mutex lock_;
  std::mutex lock_read_;
  std::condition_variable cond_read_;
  std::mutex lock_write_;
  std::condition_variable cond_write_;
  sockaddr_in addr_;
  std::unordered_map<uint32_t, bool> not_Listen_ip_;
};

#endif /* end of include guard: CONNECTER_H_7LVEHFUI */
