#ifndef FILE_H_WTYE42R3
#define FILE_H_WTYE42R3

#include <iostream>
#include <vector>
#include <mutex>
#include <boost/uuid/uuid.hpp>

/* 保存文件的基本信息
 * 提供文件的读写功能
 */
class File
{
public:
  static const int kFileDataMaxLength = 5000;
  static const int kFileNameMaxLen = 100;
  enum Stat {
    kSendend = 1,
    kClientExec = 2,
    kNetError = 3,
    kRecvend = 4
  };
public:
  File (std::string file_path, boost::uuids::uuid uuid, int size=0, bool newFile = false);
  virtual ~File ();
  std::string File_name() { return file_name_; }
  std::string File_path() { return file_path_; }
  int File_len() { return file_len_; }
  int File_max_packages() { return (file_len_+kFileDataMaxLength-1)/kFileDataMaxLength; }
  bool Readable();            //判断是否可读, 也可用于判断文件是否可用
  bool Writeable();           //判断文件是否可写
  int Read(int pack_num, char* buf);       //根据包号计算位置，读固定长度的内容
  void Write(int pack_num, const char* data, int len);   //根据包号计算位置，写固定长度的内容
  void set_Stat(Stat stat) {
    std::lock_guard<std::mutex> lock(lock_);
    stat_ = stat;
  }
  Stat Stat() { 
    std::lock_guard<std::mutex> lock(lock_);
    return stat_;
  }
  /*文件校验， 返回缺失的包
   *返回为空表示没有包丢失
   */
  std::vector<int> Check();
  bool Check_at_package_number(int package_num);   //检查对应的包号是否已经到达
  const boost::uuids::uuid& UUID() { return uuid_; }
private:
  std::mutex lock_;
  std::string file_path_;   //文件存在的路径
  boost::uuids::uuid uuid_;
  std::string file_name_;    //文件名
  int filefd_;    //文件描述符
  int file_len_;   //文件长度
  std::vector<bool> pack_is_recved_;   //数据包已经到达状态的标记
  enum Stat stat_;
};

#endif /* end of include guard: FILE_H_WTYE42R3 */
