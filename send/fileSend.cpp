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
  Connecter con(group_ip, port);
  LostPackageVec losts(file_uptr->File_max_packages());
  //启动一个线程，监听丢失的包
  std::thread listen(ListenLostPackageCallback, port+1, std::ref(losts), std::ref(con)); //修改端口， 参数： socket， addr, losts
  //设置多播地址
  //发送文件内容
  for (int i = 0; i <= file_uptr->File_max_packages(); ++i) {
    SendFileDataAtPackNum(con, file_uptr, i); 
    if (!losts.isRunning()) {
      return false;
    }
  }
  std::cout << "发送完毕, 开始校验" << std::endl;
  //检查所有的丢包情况， 并重发
  while (!losts.ExitListen()) {
    auto lose = losts.GetFileLostedPackage();
    if (!lose.empty()) {
      //重发
      for (auto i : lose) {
        std::cout << "重发 package_num " << i << std::endl;
        SendFileDataAtPackNum(con, file_uptr, i);
      }
    }
  }
  listen.join();
  return true;
}

//计算文件的长度， 想多播组发送包号为0的数据包
//数据包格式： 包号（4Bytes)文件名长度(4Bytes)文件名(小于100Bytes)
void SendFileMessage(Connecter& con, const std::unique_ptr<File>& file) {
  //发送文件信息
  Proto proto;
  proto.set_type(Proto::kNewFile);
  char buf[kBufSize];
  //*(int*)(buf+kPackNumberBeg) = (int)0;
  proto.set_package_number(0);
  //*(buf+kFileNameLenBeg) = file->File_name().size();
  proto.set_file_name(file->File_name());
  //char file_name[100];
  //strcpy(file_name, file->File_name().c_str());
  //::strncpy(buf+kFileNameBeg, file_name, File::kFileNameMaxLen);
  //*(int*)(buf+kFileLenBeg) = file->File_len();
  proto.set_file_len(file->File_len());
  int len = 0;
  //proto.buf(Proto::kNewFile, buf, len);
  con.Send(proto.buf(), proto.get_send_len());
}

/*
 * 发送一个数据包给对应的组播地址
 * 若为0号包， 则为文件信息
 * 若包号大于0， 则计算数据在文件的相应位置，并发送
 */
void SendFileDataAtPackNum(Connecter& con, const std::unique_ptr<File>& file, int package_numbuer) {
  if (package_numbuer == 0) {
    SendFileMessage(con, file); 
  } else if (package_numbuer > 0) {
    //char buf[kBufSize];
    Proto proto;
    proto.set_type(Proto::kData);
    //*(int*)(buf+kPackNumberBeg) = package_numbuer;
    proto.set_package_number(package_numbuer);
    //*(int*)(buf+kFileNameLenBeg) = file->File_name().size();
    //strncpy(buf+kFileNameBeg, file->File_name().c_str(), File::kFileNameMaxLen);
    proto.set_file_name(file->File_name());
    int re = file->Read(package_numbuer, proto.get_file_data_buf_ptr());
    if (re < 0) {
      //read error
      std::cout << "read len < 0" << __FILE__ << __LINE__ << std::endl;
    }
    //std::cout << "file Read re is " << re << std::endl;
    //*(int*)(buf+kFileDataLenBeg) = re;
    //int len = re+kFileDataBeg;
    //buf[kFileDataBeg+re] = 0;
    proto.set_file_data_len(re);
    //proto.set_file_data(buf);
    //std::cout << buf+kFileDataBeg << std::endl;
    std::cout << "send pack " << proto.package_numbuer() << std::endl;
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
void ListenLostPackageCallback(int port, LostPackageVec& losts, Connecter& con) {
  //char buf[kBufSize];
  auto time_pre = std::chrono::system_clock::now();
  Proto proto;
  while (losts.isRunning()) {
    int re = con.Recv(proto.buf(), BUFSIZ, 1000);
    if (re > 0) {
      std::cout << "recv " << re << " Bytes  " << *(Proto::Type*)(proto.buf()) << std::endl;
      if (Proto::kReSend == proto.type()) {
        time_pre = std::chrono::system_clock::now();
        std::cout << "recved pack resend request " << proto.package_numbuer() << std::endl;
        losts.AddFileLostedRecord(proto.package_numbuer());
      }
      if (Proto::kAlive == proto.type()) {
        time_pre = std::chrono::system_clock::now();
      }
    }
    if (time_pre + std::chrono::seconds(2) <= std::chrono::system_clock::now()) {
      std::cout << "all client quit" << std::endl;
      losts.ExecRunning();
    }
  }
}


LostPackageVec::LostPackageVec (int package_count) : 
  package_count_(package_count),
  lost_(package_count_+1), running_(true) {
}

LostPackageVec::~LostPackageVec () { }

/*
 * 获取丢失的包的集合， 返回一个vector
 */
std::vector<int> LostPackageVec::GetFileLostedPackage() {
  std::lock_guard<std::mutex> lock(lock_);
  std::vector<int> res;
  for (int i = 0; i <= package_count_; ++i) {
    if (lost_[i]) {
      res.push_back(i);
      lost_[i] = false;
    } 
  }
  return res;
}

//添加一个丢失记录
void LostPackageVec::AddFileLostedRecord(int package_num) {
  std::lock_guard<std::mutex> lock(lock_);
  lost_[package_num] = true;
  cond_.notify_one();
}

/* 尝试退出子线程, 若没有数据包
 * 若没有数据包，则退出子线程，并返回true
 * 否则返回false
 */
bool LostPackageVec::ExitListen() {
  if (!isRunning()) return true;
  std::unique_lock<std::mutex> lock(lock_);
  auto t = std::chrono::system_clock::now();
  t += std::chrono::seconds(5);
  std::cout << "sleepping..." << std::endl;
  if (cond_.wait_until(lock, t) == std::cv_status::timeout) {
      running_ = false;
      return true;
  }
  return false;
}

bool LostPackageVec::isRunning() {
  std::lock_guard<std::mutex> lock(lock_);
  return  running_;
}
