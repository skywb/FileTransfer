#ifndef FILESEND_H_HMY0LJA6
#define FILESEND_H_HMY0LJA6

#include "File.h"

#include <unistd.h>
#include <netinet/ip.h>
#include <string>
#include <memory>
#include <mutex>

const int kPackNumberBeg  = 0;
const int kFileNameLenBeg = sizeof(kPackNumberBeg);
const int kFileNameBeg    = kFileNameLenBeg+sizeof(kFileNameLenBeg);

//文件长度
const int kFileLenBeg    = kFileNameBeg+sizeof(kFileNameBeg);

const int kFileDataLenBeg    = kFileNameBeg+sizeof(kFileNameBeg)+File::kFileNameMaxLen;
const int kFileDataBeg    = kFileNameBeg+sizeof(kFileDataLenBeg);



const int kBufSize = kFileDataBeg+kFileDataMaxLength+10;
const int kTTL = 64;

//const int kMaxLength = 1000;
bool FileSend(std::string group_id, int port, std::string file_path);
//需要提供一个已经绑定过的地址结构和socket套接字， 以及一个文件
void SendFileMessage(int sockfd, sockaddr_in *addr, const std::unique_ptr<File>& file);
//需要提供一个已经绑定过的地址结构和socket套接字， 以及一个文件
void SendFileDataAtPackNum(int sockfd, sockaddr_in *addr, const std::unique_ptr<File>& file, int package_numbuer);
//计算需要多少个数据包
int  CalculateMaxPages(std::string file_path);

void ListenLostPackage(int port, LostPackageVec& losts);


class LostPackageVec
{
public:
  LostPackageVec (int package_count);
  virtual ~LostPackageVec ();
  //获取丢失包的集合
  std::vector<int> GetFileLostedPackage();
  //添加一个丢失记录
  void AddFileLostedRecord(int package_num);

private:
  int package_count_;    //包数量
  std::vector<bool> lost_;  //丢失记录
  std::mutex lock_;
};



#endif /* end of include guard: FILESEND_H_HMY0LJA6 */
