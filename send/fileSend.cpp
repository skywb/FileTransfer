#include "fileSend.h"
#include "send/File.h"
#include "util/multicastUtil.h"
#include "util/Connecter.h"
#include "send/FileSendControl.h"

#include <string.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>
#include <chrono>

/*
 * 文件的发送
 * 根据传入的组播地址和端口， 绑定之后发送文件
 * 会启动一个线程去监听客户端丢失的udp包， 最后检查所有的丢失的包，再次发送
 * 等待一定的时间，防止无效的报文到达
 * 获取所有网卡绑定的IP，并创建套接字
 * 想每个网卡发送相同的信息
 */
bool FileSend(std::string group_ip, 
    int port, std::unique_ptr<File>& file_uptr) {
/* TODO:  <17-08-19, yourname> */
  //Connecter con(group_ip, port);
  //LostPackageVec losts(file_uptr->File_max_packages());
  ////启动一个线程，监听丢失的包
  //std::thread listen(ListenLostPackageCallback, port+1, std::ref(losts), std::ref(con)); //修改端口， 参数： socket， addr, losts
  ////设置多播地址
  ////发送文件内容
  //for (int i = 0; i <= file_uptr->File_max_packages(); ++i) {
  //  SendFileDataAtPackNum(con, file_uptr, i, i); 
  //  if (!losts.isRunning()) {
  //    return false;
  //  }
  //  if (i % 300 == 0) {
  //    auto lose = losts.GetFileLostedPackage();
  //    if (!lose.empty()) {
  //      //重发
  //      for (auto j : lose) {
  //        //std::cout << "重发 package_num " << i << std::endl;
  //        if (j > i) break;
  //        SendFileDataAtPackNum(con, file_uptr, j, i);
  //      }
  //    }
  //  }
  //}
  ////检查所有的丢包情况， 并重发
  //int end_pack = file_uptr->File_max_packages();
  //while (!losts.ExitListen()) {
  //  auto lose = losts.GetFileLostedPackage();
  //  if (!lose.empty()) {
  //    //重发
  //    for (auto i : lose) {
  //      //std::cout << "重发 package_num " << i << std::endl;
  //      SendFileDataAtPackNum(con, file_uptr, i, end_pack);
  //    }
  //  }
  //}
  //listen.join();
  //return true;
}

//计算文件的长度， 想多播组发送包号为0的数据包
//数据包格式： 包号（4Bytes)文件名长度(4Bytes)文件名(小于100Bytes)
void SendFileMessage(Connecter& con, const std::unique_ptr<File>& file) {
  //发送文件信息
  Proto proto;
  proto.set_type(Proto::kNewFile);
  proto.set_package_number(0);
  proto.set_file_name(file->File_name());
  proto.set_file_len(file->File_len());
  con.Send(proto.buf(), proto.get_send_len());
}

/*
 * 发送一个数据包给对应的组播地址
 * 若为0号包， 则为文件信息
 * 若包号大于0， 则计算数据在文件的相应位置，并发送
 */
void SendFileDataAtPackNum(Connecter& con, const std::unique_ptr<File>& file, int package_numbuer, int ack_num) {
  if (package_numbuer == 0) {
    SendFileMessage(con, file); 
  } else if (package_numbuer > 0) {
    Proto proto;
    proto.set_type(Proto::kData);
    proto.set_package_number(package_numbuer);
    proto.set_ack_package_number(ack_num);
    proto.set_file_name(file->File_name());
    int re = file->Read(package_numbuer, proto.get_file_data_buf_ptr());
    if (re < 0) {
      std::cout << "read len < 0" << __FILE__ << __LINE__ << std::endl;
    }
    proto.set_file_data_len(re);
    //std::cout << "send pack " << proto.package_numbuer() << std::endl;
    if (-1 == con.Send(proto.buf(), proto.get_send_len())) {
      std::cout << "send error " << package_numbuer << std::endl;
    }
  } else {
    std::cout << "package_num < 0" << std::endl;
  }
}

/* 监听丢失的包
 * 保存到LostPackageVec中
 * 线程任务
 */
void ListenLostPackageCallback(LostPackageVec& losts, Connecter& con) {
/* TODO: 从con中读取数据，
 * 若为重传请求
 *  将包号保存到lostvec中， 并唤醒发送线程 send_cond_.notify_one
 * 若为心跳包， 更新心跳包时间 
 * 不可读直接再次等待条件变量 recv_cond_<17-08-19, yourname> */
  Proto proto;
  auto now = std::chrono::system_clock::now();
  while (losts.isRunning()) {
    while (-1 != con.Recv(proto.buf(), proto.BufSize())) {
      if (Proto::kReSend == proto.type()) {
        losts.AddFileLostedRecord(proto.package_numbuer());
      } else if (Proto::kAlive == proto.type()) {

      }
      now = std::chrono::system_clock::now();
    }
    if (!losts.WaitRecvable(now + std::chrono::seconds(3))) {
      losts.ExecRunning();
      return;
    }
  }
}


LostPackageVec::LostPackageVec (int package_count) : 
  package_count_(package_count),
  lost_(package_count_+1), running_(false) { }

LostPackageVec::~LostPackageVec () { 
  listen_thread_.join();
}

/*
 * 获取丢失的包的集合， 返回一个vector
 */
std::vector<int> LostPackageVec::GetFileLostedPackage() {
  std::unique_lock<std::mutex> lock(send_lock_);
  std::vector<int> res;
  send_cond_.wait(lock);
  for (int i = 0, cnt = 0; i <= package_count_ && cnt < 1000; ++i) {
    if (lost_[i]) {
      res.push_back(i);
      ++cnt;
      lost_[i] = false;
    } 
  }
  return res;
}

//添加一个丢失记录
void LostPackageVec::AddFileLostedRecord(int package_num) {
  std::lock_guard<std::mutex> lock(send_lock_);
  lost_[package_num] = true;
  send_cond_.notify_one();
}


bool LostPackageVec::isRunning() {
  std::lock_guard<std::mutex> lock(running_lock_);
  return  running_;
}
