#include "fileRecv.h"
#include "send/fileSend.h"
#include "reactor/CMD.h"
#include "util/multicastUtil.h"
#include "util/Connecter.h"

#include <cstring>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include <iostream>
#include <vector>
#include <thread>


///*
// * 从sockfd读内容， 判断是否为本文件， 写入文件
// * 计算平均传输时间， 设置定时器， 当超过两倍时间没有发送时， 发送丢包重传请求
// */
bool FileRecv(std::string group_ip, int port, std::unique_ptr<File>& file_uptr) {
  Connecter con(group_ip, port);
  //接收缓冲区
  char buf[kBufSize];
  //接收的长度
  int recv_len = 0;
  //分别上一个包到来时间，本次到来时间， 差值， 以及平均时间
  //timeval pre_time, cur_time, sub_time, avg_time;
  ////设置默认等待时间
  //fd_set rd_fd;
  ////检查的包序号
  int max_pack_num = 0, check_package_num = 0;
  for (int i = 0; ; ++i) {
    recv_len = con.Recv(buf, kBufSize, 3000);
    if (recv_len > 0) {
      //数据到来
      buf[recv_len] = 0;
      int pack_num = *(int*)(buf+kPackNumberBeg);
#if DEBUG
      std::cout << "pack_num = " <<  pack_num   << "len is " << recv_len << " " << __FILE__ << __LINE__<< std::endl;
#endif
      if (pack_num == 0) {
        continue;
      }
      max_pack_num = std::max(pack_num, max_pack_num);
      int file_name_len = *(int*)(buf+kFileNameLenBeg);
      char file_name[File::kFileNameMaxLen+10];
      /* TODO: 检查文件长度， 防止非法长度造成错误 <22-07-19, 王彬> */
      strncpy(file_name, buf+kFileNameBeg, file_name_len);
      //TODO:校验文件名
      int data_len = *(int*)(buf+kFileDataLenBeg);
      std::cout << "data len is " << data_len << std::endl;
      std::cout << buf+kFileDataBeg << std::endl;
      file_uptr->Write(pack_num, buf+kFileDataBeg, data_len);
    }
    /*: 检查之前的包是否到达 <22-07-19, 王彬> */
    std::cout << (check_package_num <= file_uptr->File_max_packages()) << " " << file_uptr->Check_at_package_number(check_package_num) << std::endl;
    while (check_package_num <= file_uptr->File_max_packages() 
        && file_uptr->Check_at_package_number(check_package_num)) {
      ++check_package_num;
    }
    if (check_package_num > file_uptr->File_max_packages()) break;
    if (max_pack_num - check_package_num > 3 || recv_len <= 0) { //请求重发
      check_package_num = std::min(check_package_num, file_uptr->File_max_packages());
    }
    std::cout << "check_package_num " << check_package_num << " max_pack_num " << file_uptr->File_max_packages() << std::endl;
  }
  return true;
}

