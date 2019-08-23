#include <iostream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>


#include "send/fileSend.h"

class MockConnecter : public Connecter
{
public:
  MockConnecter () : Connecter("224.0.2.10", 8888) {}
  virtual ~MockConnecter () {}
public:
  int Recv(char* buf, int len) override {
    if (len_ == -1) return -1;
    len = len_;
    memcpy(buf, buf_, len);
    len_ = -1;
    return len;
  }
  void set_buf(char* buf, int len) {
    if (len == -1) {
      len_ = -1;
      return ;
    }
    len_ = len;
    memcpy(buf_, buf, len);
  }
  int Send(char* buf, int len) override {
    return -1;
  }
protected:
  int len_;
  char* buf_[BUFSIZ];
  
};

void MockListenPackageCallback(LostPackageVec& losts, Connecter& con) {
  char buf[1000];
  int len = 0;
  len = con.Recv(buf, 1000);
  usleep(100);
  return;
}

class TestLostPackageVec : public testing::Test
{
public:
  TestLostPackageVec () : vec(1000) {}

protected:
  LostPackageVec vec;
  MockConnecter con;
};

TEST_F(TestLostPackageVec, TestRunning) {
  ASSERT_EQ(vec.isRunning(), false);
  //vec.RegestListenCallback(MockListenPackageCallback, con);
  EXPECT_EQ(vec.isRunning(), true);
}

TEST_F(TestLostPackageVec, TestListenLostPackCallback) {
  ASSERT_EQ(vec.isRunning(), false);
  con.set_buf(nullptr, -1);
  //vec.RegestListenCallback(ListenLostPackageCallback, con);
  EXPECT_EQ(vec.isRunning(), true);
  auto res = vec.GetFileLostedPackage(1000);
  EXPECT_EQ(res.size(), 1000);
  Proto proto;
  proto.set_type(Proto::kReSend);
  proto.set_package_number(1);
  con.set_buf(proto.buf(), proto.get_send_len());
  //vec.NoticeRead();
  usleep(10);
  res = vec.GetFileLostedPackage(1000);
  ASSERT_EQ(vec.isRunning(), true);
  ASSERT_EQ(res.size(), 1);
  EXPECT_EQ(res[0], 1);
  sleep(5);
  EXPECT_EQ(vec.isRunning(), false);
}



