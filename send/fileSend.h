#ifndef FILESEND_H_HMY0LJA6
#define FILESEND_H_HMY0LJA6

#include "File.h"
#include "util/Connecter.h"

#include <unistd.h>
#include <netinet/ip.h>
#include <string>
#include <memory>
#include <mutex>
#include <condition_variable>

const int kPackNumberBeg  = 0;
const int kFileNameLenBeg = kPackNumberBeg+sizeof(int);
const int kFileNameBeg    = kFileNameLenBeg+sizeof(int);

//文件长度   ----   0号包
const int kFileLenBeg    = kFileNameBeg+File::kFileNameMaxLen;
// 非0号包
const int kFileDataLenBeg    = kFileNameBeg+File::kFileNameMaxLen;
const int kFileDataBeg    = kFileDataLenBeg+sizeof(int);

const int kBufSize = kFileDataBeg+File::kFileDataMaxLength+10;
const int kTTL = 64;

class LostPackageVec
{
public:
  LostPackageVec (int package_count);
  virtual ~LostPackageVec ();
  //获取丢失包的集合
  std::vector<int> GetFileLostedPackage();
  //添加一个丢失记录
  void AddFileLostedRecord(int package_num);
  //尝试退出子线程, 若没有数据包
  //若没有数据包，则退出子线程，并返回true
  //否则返回false
  bool ExitListen();
  bool isRunning();

private:
  int package_count_;    //包数量
  std::vector<bool> lost_;  //丢失记录
  std::mutex lock_;
  std::condition_variable cond_;
  bool running;
};

//const int kMaxLength = 1000;
bool FileSend(std::string group_ip, int port, std::unique_ptr<File>& file_uptr);
//需要提供一个已经绑定过的地址结构和socket套接字， 以及一个文件
//void SendFileMessage(int sockfd, sockaddr_in *addr, const std::unique_ptr<File>& file);
void SendFileMessage(Connecter& con, const std::unique_ptr<File>& file);
//需要提供一个已经绑定过的地址结构和socket套接字， 以及一个文件
//void SendFileDataAtPackNum(int sockfd, sockaddr_in *addr, const std::unique_ptr<File>& file, int package_numbuer);
void SendFileDataAtPackNum(Connecter& con, const std::unique_ptr<File>& file, int package_numbuer);
//计算需要多少个数据包    --- 弃用
//int  CalculateMaxPages(std::string file_path);
//监听接收端丢失的包   --- 线程回调函数
void ListenLostPackage(int port, LostPackageVec& losts, Connecter& con);

#endif /* end of include guard: FILESEND_H_HMY0LJA6 */
