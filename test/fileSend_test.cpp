#include "util/Connecter.h"
#include "send/fileSend.h"
#include <iostream>
#include <gtest/gtest.h>
#include <unistd.h>
#include <fcntl.h>

class MockConnecter : public Connecter {
public:
  MockConnecter () : Connecter("224.0.2.10", 8888) {
    pipe(fd_);
    epoll_fd_ = epoll_create(2);
    epoll_event event;
    event.data.fd = fd_[0];
    event.events = EPOLLIN;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd_[0], &event);
    auto flags = fcntl(fd_[0], F_GETFL, 0);
    fcntl(fd_[0], F_SETFL, flags | O_NONBLOCK);
  }
  virtual ~MockConnecter () {
  }
  virtual Type Wait(Type type, int time_millsec) override;
  virtual int Recv(char* buf, int len) override;
  virtual int Send(char* buf, int len) override;
  Type wait_return_type_;
  int fd_[2];
  int epoll_fd_;
};

Connecter::Type MockConnecter::Wait(Type type, int time_millsec) {
  epoll_event events[10];
  int cnt = epoll_wait(epoll_fd_, events, 10, time_millsec);
  for (int i = 0; i < cnt; ++i) {
    if (events[i].data.fd == fd_[0]) {
      return wait_return_type_;
    } else {
      std::cout << "异常fd" << std::endl;
    }
  }
  return kOutTime;
}
int MockConnecter::Recv(char* buf, int len) { 
  int res = read(fd_[0], buf, len);
  return res;
}

int MockConnecter::Send(char* buf, int len) {
  return -1;
}

class MockLostPackageVec : public LostPackageVec
{
public:
  MockLostPackageVec () : LostPackageVec(100) { 
    
  }
  virtual ~MockLostPackageVec ();

private:
  /* data */
};


class TestFileSend
{
public:
  TestFileSend ();
  virtual ~TestFileSend ();

protected:
  MockConnecter con;
  MockLostPackageVec vec;
  Connecter con_;
  LostPackageVec vec_;
};

TEST_F(TestFileSend, TestListenLostPackage) {
  std::thread th(ListenLostPackage, std::ref(con_), std::ref(vec_));

}


