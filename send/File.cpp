#include "File.h"

#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

File::File (std::string file_path, int size, bool newFile) : file_path_(file_path), file_len_(size) {
  if (newFile) {
    filefd_ = ::open(file_path.c_str(), O_RDWR | O_CREAT, 0777);
    lseek(filefd_, file_len_-1, SEEK_SET);
    ::write(filefd_, "0", 1);
  } else {
    filefd_ = ::open(file_path_.c_str(), O_RDONLY);
  }
  auto pos = file_path.find_last_of('/');
  file_path_ = file_path.substr(0, pos+1);
  file_name_ = file_path.substr(pos+1); 
  if (filefd_ == -1) {
    //文件打开失败
#if DEBUG
    std::cout << "文件打开失败 " << __FILE__  << " : " << __LINE__ << std::endl;
    std::cout << strerror(errno) << std::endl;
#endif
  }
  lseek(filefd_, 0, SEEK_SET);
  if (file_len_ == 0) {
    file_len_ = lseek(filefd_, 0, SEEK_END);
  }
  pack_is_recved_.resize((file_len_+kFileDataMaxLength-1)/kFileDataMaxLength, false);
  pack_is_recved_[0] = true;
}

File::~File() {
  close(filefd_);
  filefd_ = -1;
}
//判断文件是否可读
bool File::Readable() {
  int val = fcntl(filefd_, F_GETFL, 0);
  if (val == -1) {
    //error
  }
  return ((val & O_ACCMODE) == O_RDONLY) || ((val & O_ACCMODE) == O_RDWR);
}
//判断文件是否可写
bool File::Writeable() {
  int val = fcntl(filefd_, F_GETFL, 0);
  if (val == -1) {
    //error
  }
  return ((val & O_ACCMODE) == O_WRONLY) || ((val & O_ACCMODE) == O_RDWR);
}
/*
 * 根据包号计算读取的位置， 并读取固定长度的文件内容
 * 固定长度为kMaxLength
 */
int File::Read(int pack_num, char* buf) {
  if (!Readable()) {
#if DEBUG
    std::cout << "无法读" << std::endl;
#endif
    throw std::string("can't read");
  } 
  lseek(filefd_, (pack_num-1)*kFileDataMaxLength, SEEK_SET); 
  return ::read(filefd_, buf, kFileDataMaxLength);
}
void File::Write(int pack_num, const char* data, int len) {
  if (!Writeable()) {
    throw std::string("can't write");
  }
  pack_is_recved_[pack_num] = true;
  std::cout << pack_num << " recived in FileWrite" << std::endl;
  lseek(filefd_, (pack_num-1)*kFileDataMaxLength, SEEK_SET);
  ::write(filefd_, data, len);
}


std::vector<int> File::Check() {
  std::vector<int> res;
  for (int i = 0; i < pack_is_recved_.size(); ++i) {
    if (!pack_is_recved_[i]) res.push_back(i);
  }
  /* TODO: 检查文件完整性 <22-07-19, 王彬> */
  return res;
}

bool File::Check_at_package_number(int package_num) {

  return pack_is_recved_[package_num];
}
