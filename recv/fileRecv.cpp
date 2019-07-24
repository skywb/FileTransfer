#include "fileRecv.h"
#include "send/fileSend.h"
#include "reactor/CMD.h"
#include "util/multicastUtil.h"

#include <cstring>
#include <fcntl.h>
#include <sys/time.h>
#include <arpa/inet.h>

#include <iostream>
#include <vector>


///*
// * 从sockfd读内容， 判断是否为本文件， 写入文件
// * 计算平均传输时间， 设置定时器， 当超过两倍时间没有发送时， 发送丢包重传请求
// */
//void FileRecv (const int sockfd, const sockaddr_in* addr, std::unique_ptr<File>& file_uptr) {{{{
//  char buf[kBufSize];
//  int recv_len = 0;
//  //分别上一个包到来时间，本次到来时间， 差值， 以及平均时间
//  timeval pre_time, cur_time, sub_time, avg_time;
//  //设置默认等待时间
//  fd_set rd_fd;
//  //检查的包序号
//  int max_pack_num = 0, check_package_num = 0;
//  for (int i = 0; ; ++i) {
//    avg_time.tv_sec = 3;
//    avg_time.tv_usec = 0;
//    FD_ZERO(&rd_fd);
//    FD_SET(sockfd, &rd_fd);
//    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&avg_time, sizeof(timeval));
//    recv_len = recvfrom(sockfd, buf, File::kMaxLength, 0, NULL, 0);
//    if (recv_len != 0 && FD_ISSET(sockfd, &rd_fd)) {
//      //数据到来
//      std::cout << recv_len << std::endl;
//#if DEBUG
//      std::cout << "recv" << std::endl;
//#endif
//      if (recv_len < 0) {
//        /* TODO: 
//         * 校验文件 <22-07-19, 王彬> */
//        //接收错误
//        break;
//      }
//      buf[recv_len] = 0;
//      int pack_num = *(int*)(buf+kPackNumberBeg);
//#if DEBUG
//      std::cout << "pack_num = " <<  pack_num << std::endl;
//#endif
//      if (pack_num == 0) {
//        continue;
//      }
//      max_pack_num = std::max(pack_num, max_pack_num);
//      int file_name_len = *(int*)(buf+kFileNameLenBeg);
//      char file_name[File::kFileNameMaxLen+10];
//      /* TODO: 检查文件长度， 防止非法长度造成错误 <22-07-19, 王彬> */
//      strncpy(file_name, buf+kFileNameBeg, file_name_len);
//      //TODO:校验文件名
//      int data_len = *(buf+kFileDataLenBeg);
//      file_uptr->Write(pack_num, buf+kFileDataBeg, data_len);
//    }
//    /*: 检查之前的包是否到达 <22-07-19, 王彬> */
//    while (check_package_num <= file_uptr->File_max_packages() && (max_pack_num - check_package_num > 3 || recv_len == 0)) {
//      if (!file_uptr->Check_at_package_number(check_package_num)) {
//        //请求重发
//      ++check_package_num;
//      check_package_num = std::min(check_package_num, file_uptr->File_max_packages());
//#if DEBUG
//        std::cout << "请求重发" << std::endl;
//#endif
//      }
//      if (recv_len == 0) recv_len = 1;
//    }
//  }
//}}}}

bool FileRecv(std::string group_ip, int port, std::unique_ptr<File>& file_uptr) {
  sockaddr_in addr;
  //ip_mreq join_adr;
	int sockfd; //= socket(AF_INET, SOCK_DGRAM, 0);
  if (false == JoinGroup(&addr, &sockfd, group_ip, port)) {
    std::cout << "加入组播组失败" << std::endl;
    exit(1);
  }
	//addr.sin_port = htons(port);
	//addr.sin_family = AF_INET;
  //addr.sin_addr.s_addr = htonl(INADDR_ANY);
  //int opt = 1;
  //// sockfd为需要端口复用的套接字
  //setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));
  //if(bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
  //  std::cerr << "bind() error" << std::endl;
  //  exit(1);
  //}
  //join_adr.imr_multiaddr.s_addr = inet_addr(group_ip.c_str());
  //join_adr.imr_interface.s_addr = htonl(INADDR_ANY);
  //setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&join_adr, sizeof(join_adr));
  //接收缓冲区
  char buf[kBufSize];
  //接收的长度
  int recv_len = 0;
  //分别上一个包到来时间，本次到来时间， 差值， 以及平均时间
  timeval pre_time, cur_time, sub_time, avg_time;
  //设置默认等待时间
  fd_set rd_fd;
  //检查的包序号
  int max_pack_num = 0, check_package_num = 0;
  for (int i = 0; ; ++i) {
    avg_time.tv_sec = 3;
    avg_time.tv_usec = 0;
    FD_ZERO(&rd_fd);
    FD_SET(sockfd, &rd_fd);
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&avg_time, sizeof(timeval));
    recv_len = recvfrom(sockfd, buf, File::kMaxLength, 0, NULL, 0);
    if (recv_len != 0 && FD_ISSET(sockfd, &rd_fd)) {
      //数据到来
      std::cout << recv_len << std::endl;
#if DEBUG
      std::cout << "recv" << std::endl;
#endif
      if (recv_len < 0) {
        /* TODO: 
         * 校验文件 <22-07-19, 王彬> */
        //接收错误
        break;
      }
      buf[recv_len] = 0;
      int pack_num = *(int*)(buf+kPackNumberBeg);
#if DEBUG
      std::cout << "pack_num = " <<  pack_num << std::endl;
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
      int data_len = *(buf+kFileDataLenBeg);
      file_uptr->Write(pack_num, buf+kFileDataBeg, data_len);
    }
    /*: 检查之前的包是否到达 <22-07-19, 王彬> */
    while (check_package_num <= file_uptr->File_max_packages() && (max_pack_num - check_package_num > 3 || recv_len == 0)) {
      if (!file_uptr->Check_at_package_number(check_package_num)) {
        //请求重发
      ++check_package_num;
      check_package_num = std::min(check_package_num, file_uptr->File_max_packages());
#if DEBUG
        std::cout << "请求重发" << std::endl;
#endif
      }
      if (recv_len == 0) recv_len = 1;
    }
  }
  return true;
}

