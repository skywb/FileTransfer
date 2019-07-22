#include "File.h"

#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>

File::File (std::string file_path, int size) : file_path_(file_path), file_len_(size) {
  filefd_ = ::open(file_path.c_str(), O_RDWR | O_CREAT, 0777);
  auto pos = file_path.find_last_of('/');
  file_path_ = file_path.substr(0, pos);
  file_name_ = file_path.substr(pos+1); 
  if (filefd_ == -1) {
    //文件打开失败
#if DEBUG
    std::cout << "文件打开失败 " << __FILE__  << " : " << __LINE__ << std::endl;
#endif
  }
  lseek(filefd_, 0, SEEK_SET);
  if (file_len_ == 0) {
    file_len_ = lseek(filefd_, 0, SEEK_END);
  }
  pack_is_recved_.resize((file_len_+kMaxLength-1)/kMaxLength, false);
}

File::~File() {
  close(filefd_);
  filefd_ = -1;
}
//判断文件是否可读
bool File::readable() {
  int val = fcntl(filefd_, F_GETFL, 0);
  if (val == -1) {
    //error
  }
  return ((val & O_ACCMODE) & (O_RDONLY | O_RDWR));
}
//判断文件是否可写
bool File::writeable() {
  int val = fcntl(filefd_, F_GETFL, 0);
  if (val == -1) {
    //error
  }
  return ((val & O_ACCMODE) & (O_WRONLY | O_RDWR));
}
/*
 * 根据包号计算读取的位置， 并读取固定长度的文件内容
 * 固定长度为kMaxLength
 */
int File::read(int pack_num, char* buf) {
  if (!readable()) {
    throw std::string("can't read");
  } 
  lseek(filefd_, (pack_num-1)*kMaxLength, SEEK_SET); 
  return read(filefd_, buf);
}
void File::write(int pack_num, const char* data, int len) {
  if (!writeable()) {
    throw std::string("can't write");
  }
  pack_is_recved_[pack_num] = true;
  lseek(filefd_, (pack_num-1)*kMaxLength, SEEK_SET);
  write(filefd_, data, len);
}


std::vector<int> File::check() {
  std::vector<int> res;
  for (int i = 0; i < pack_is_recved_.size(); ++i) {
    if (!pack_is_recved_[i]) res.push_back(i);
  }
  /* TODO: 检查文件完整性 <22-07-19, 王彬> */
  return res;
}
