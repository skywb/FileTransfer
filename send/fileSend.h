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
  std::vector<int> GetFileLostedPackage();
  //添加一个丢失记录
  void AddFileLostedRecord(int package_num);
  /* 尝试退出子线程, 若没有数据包
   * 若没有数据包，则退出子线程，并返回true
   * 否则返回false
   */
  bool ExitListen();
  //检查是否正在运行
  bool isRunning();
  void ExecRunning() {
    std::lock_guard<std::mutex> lock(lock_);
    running_ = false;
  }

private:
  int package_count_;    //包数量
  std::vector<bool> lost_;  //丢失记录
  std::mutex lock_;
  std::condition_variable cond_;   //唤醒正在发送端重发数据包
  bool running_;
};

//发送文件, 参数分别为可用的组播地址和端口， 以及一个文件
bool FileSend(std::string group_ip, int port, std::unique_ptr<File>& file_uptr);

//发送文件描述信息的数据包， 即0号数据包
void SendFileMessage(Connecter& con, const std::unique_ptr<File>& file);

//发送包号为package_number的数据包
void SendFileDataAtPackNum(Connecter& con, const std::unique_ptr<File>& file, int package_numbuer, int ack_num);

//监听接收端丢失的包   --- 线程回调函数
void ListenLostPackageCallback(int port, LostPackageVec& losts, Connecter& con);

#endif /* end of include guard: FILESEND_H_HMY0LJA6 */
