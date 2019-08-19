#ifndef FILESEND_H_HMY0LJA6
#define FILESEND_H_HMY0LJA6

#include "File.h"
#include "send/FileSendControl.h"
#include "util/Connecter.h"

#include <unistd.h>
#include <netinet/ip.h>
#include <string>
#include <memory>
#include <mutex>
#include <condition_variable>

const int kTTL = 64;

//文件丢包记录， 用于保存所有丢失的包
class LostPackageVec {
public:
  LostPackageVec (int package_count);
  virtual ~LostPackageVec ();
  //获取丢失包的集合
  std::vector<int> GetFileLostedPackage(int max_num);
  //添加一个丢失记录
  void AddFileLostedRecord(int package_num);

  void NoticeRead() {
    std::unique_lock<std::mutex> lock(recv_lock_);
    recv_cond_.notify_one();
  }

  void RegestListenCallback(void func(LostPackageVec& , Connecter& ), Connecter& con) {
    std::thread th(func, std::ref(*this), std::ref(con));
    listen_thread_.swap(th);
    std::lock_guard<std::mutex> lock(running_lock_);
    running_ = true;
  }
  bool WaitRecvable(std::chrono::time_point<std::chrono::system_clock> time) {
    std::unique_lock<std::mutex> lock(recv_lock_);
    if (std::cv_status::timeout == recv_cond_.wait_until(lock, time)) return false;
    return isRunning();
  }
  bool isRunning();
  void ExecRunning() {
    std::lock_guard<std::mutex> lock(running_lock_);
    running_ = false;
  }

private:
  std::thread listen_thread_;
  int package_count_;    //包数量
  std::vector<bool> lost_;  //丢失记录
  std::mutex send_lock_;
  std::mutex recv_lock_;
  std::condition_variable send_cond_;   //唤醒正在发送端重发数据包
  std::condition_variable recv_cond_;   //唤醒正在发送端重发数据包
  std::mutex running_lock_;
  bool running_;
};

//发送文件, 参数分别为可用的组播地址和端口， 以及一个文件
bool FileSend(std::string group_ip, int port, std::unique_ptr<File>& file_uptr);

//发送文件描述信息的数据包， 即0号数据包
void SendFileMessage(Connecter& con, const std::unique_ptr<File>& file);

//发送包号为package_number的数据包
bool SendFileDataAtPackNum(Connecter& con, const std::unique_ptr<File>& file, int package_numbuer, int ack_num);

//监听接收端丢失的包   --- 线程回调函数
void ListenLostPackageCallback(LostPackageVec& lost, Connecter& con);

#endif /* end of include guard: FILESEND_H_HMY0LJA6 */
