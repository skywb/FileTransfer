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
#include <glog/logging.h>

/*
 * 发送文件数据的回调函数
 * 参数：
 *  con: 连接器，用于连接一个组播组
 *  lost: 丢包记录
 *  file_uptr: 文件类指针，用于操作文件
 * 从lost中获取需要重发的数据包号， 并发送
 * 根据文件接收状态设置File的stat
 */
static void SendFileDataCallback(Connecter& con, LostPackageVec& lost, std::unique_ptr<File>& file_uptr) {
  int max_send_pack = 1;
  while (lost.isRunning()) {
    auto res = lost.GetFileLostedPackage(1000);
    for (auto i : res) {
      if (i > max_send_pack) max_send_pack = i;
      if (!SendFileDataAtPackNum(con, file_uptr, i, max_send_pack)) {
        con.AddListenWriteableEvent();
        break;
      }
    }
  }
  if (max_send_pack >= file_uptr->File_max_packages())
    file_uptr->set_Stat(File::kSendend);
  else 
    file_uptr->set_Stat(File::kClientExec);
}

/*
 * 监听需要重传的请求
 *  con: 连接器，用于连接一个组播组
 *  lost: 丢包记录
 * 将接收到的重传请求记录到lost中，并发送通知，唤醒重传的线程
 * 当超时3秒中没有收到心跳包（重传请求也认为是心跳包）则认为接收端已经退出
 * 并返回
 */
static void ListenLostPackage(Connecter& con, LostPackageVec& lost) {
  auto heart_time = std::chrono::system_clock::now();
  Proto proto;
  while (true) {
    auto event = con.Wait(Connecter::kRead, 3000);
    if (event == Connecter::kOutTime) {
      if (heart_time + std::chrono::milliseconds(3) < std::chrono::system_clock::now()) {
        lost.ExitRunning();
        LOG(INFO) << "超时没有心跳包，退出发送程序";
        break;
      }
    }
    if (event & Connecter::kWrite) {
      lost.AddFileLostedRecord(0);
    }
    if (event & Connecter::kRead) {
      int res = con.Recv(proto.buf(), proto.BufSize());
      if (res == -1) {
        continue;
      }
      switch (proto.type()) {
        case Proto::kReSend:
          lost.AddFileLostedRecord(proto.package_numbuer());
          //不需要break; 因为kReSend也是心跳包， 也需要执行kAlive的代码
        case Proto::kAlive:
          heart_time = std::chrono::system_clock::now();
          break;
        default:
          LOG(WARNING) << "异常协议头， 值：" << proto.type();
      }
    }
  }
  return ;
}

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
  std::thread send_data_th(SendFileDataCallback, 
      std::ref(con), std::ref(losts), std::ref(file_uptr));
  ListenLostPackage(con, losts);
  send_data_th.join();
  return file_uptr->Stat() == File::kSendend;
}

/*
 * 发送一个数据包给对应的组播地址
 * 若为0号包， 则为文件信息
 * 若包号大于0， 则计算数据在文件的相应位置，并发送
 */
bool SendFileDataAtPackNum(Connecter& con, 
    const std::unique_ptr<File>& file,
    int package_numbuer, int ack_num) {
  if (package_numbuer > ack_num) ack_num = package_numbuer;
  if (package_numbuer == 0) {
    //SendFileMessage(con, file); 
    return false;
  } else if (package_numbuer > 0) {
    Proto proto;
    proto.set_type(Proto::kData);
    proto.set_package_number(package_numbuer);
    proto.set_ack_package_number(ack_num);
    proto.set_file_name(file->File_name());
    int re = file->Read(package_numbuer, proto.get_file_data_buf_ptr());
    if (re < 0) {
      LOG(ERROR) << "读文件失败" <<  __FILE__ << __LINE__;
      return false;
    }
    proto.set_file_data_len(re);
    //std::cout << "send pack " << proto.package_numbuer() << std::endl;
    if (-1 == con.Send(proto.buf(), proto.get_send_len())) {
      return false;
    }
  } else {
    std::cout << "package_num < 0" << std::endl;
  }
  return true;
}

LostPackageVec::LostPackageVec (int package_count) : 
  package_count_(package_count),
  lost_(package_count_+1, true), lost_num_(package_count_),
  send_pack_beg_(1), running_(true) { }

LostPackageVec::~LostPackageVec () { 
}

/*
 * 获取丢失的包的集合， 返回一个vector
 */
std::vector<int> LostPackageVec::GetFileLostedPackage(int max_num) {
  std::vector<int> res;
  if (!isRunning()) return res;
  std::unique_lock<std::mutex> lock(lost_pack_lock_);
  while(isRunning() && lost_num_ <= 0) {
    lost_pack_cond_.wait(lock);

  }
  max_num = std::min(max_num, package_count_);
  if (send_pack_beg_ > package_count_) send_pack_beg_ = 1;
  for (int cnt=0; send_pack_beg_ <= package_count_ && cnt < max_num; ++send_pack_beg_) {
    if (lost_[send_pack_beg_]) {
      res.push_back(send_pack_beg_);
      ++cnt;
      lost_[send_pack_beg_] = false;
    } 
  }
  lost_num_ -= res.size();
  if (lost_num_ < 0) lost_num_ = 0;
  return res;
}

//添加一个丢失记录
void LostPackageVec::AddFileLostedRecord(int package_num) {
  if (!isRunning()) return;
  std::lock_guard<std::mutex> lock(lost_pack_lock_);
  if (package_num < 1 || package_num > package_count_) {
    lost_pack_cond_.notify_one();
    return;
  }
  if (lost_[package_num] == false) {
    lost_[package_num] = true;
    ++lost_num_;
  }
  lost_pack_cond_.notify_one();
}


bool LostPackageVec::isRunning() {
  std::lock_guard<std::mutex> lock(running_lock_);
  return  running_;
}
