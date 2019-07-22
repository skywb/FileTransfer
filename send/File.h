#ifndef FILE_H_WTYE42R3
#define FILE_H_WTYE42R3

#include <iostream>
#include <vector>

//const int kFileDataMaxLength = 1000;
//const int kMaxLength = kFileDataMaxLength;
//const int kFileNameMaxLen = 100;

class File
{
public:
  static const int kFileDataMaxLength = 1000;
  static const int kMaxLength = kFileDataMaxLength;
  static const int kFileNameMaxLen = 100;
public:
  File (std::string file_path, int size=0);
  virtual ~File ();

  std::string file_name() { return file_name_; }
  int file_len() { return file_len_; }
  int file_max_packages() { return (file_len_+kMaxLength-1)/kFileDataMaxLength; }
  //判断是否可读
  //也可用于判断文件是否可用
  bool readable();
  bool writeable();
  //根据包号计算位置，读固定长度的内容
  int read(int pack_num, char* buf);
  //根据包号计算位置，写固定长度的内容
  void write(int pack_num, const char* data, int len);
  //文件校验， 返回缺失的包
  //返回为空表示没有包丢失
  std::vector<int> check();
private:
  std::string file_path_;   //文件存在的路径
  std::string file_name_;    //文件名
  int filefd_;    //文件描述符
  int file_len_;   //文件长度
  std::vector<bool> pack_is_recved_;
};

#endif /* end of include guard: FILE_H_WTYE42R3 */
