#include "fileRecv.h"
#include "send/fileSend.h"
#include "reactor/CMD.h"

#include <cstring>
#include <fcntl.h>
#include <sys/time.h>

#include <iostream>
#include <vector>


//void FileRecv(const int recv_sock, const sockaddr_in* addr, std::string save_path) {/*{{{*/
//  int file_fd = -1;
//  int str_len = 0;
//  char buf[kBufSize];
//  char filename[kBufSize];
//
//  std::vector<bool> pack_is_recved;
//  while (true) {
//    str_len = recvfrom(recv_sock, buf, kMaxLength, 0, NULL, 0);
//#if DEBUG
//    std::cout << "recv" << std::endl;
//#endif
//    if(str_len < 0) {
//      /* TODO: 
//       * 校验文件 <22-07-19, 王彬> */
//      break;
//    }
//    buf[str_len] = 0;
//    int pack_num = *buf;
//    strncpy(filename, buf+sizeof(pack_num), kFileNameMaxLen);
//    int file_length = 0;
//    int max_pack_num = 0;
//    /*  TODO
//     * 收到包号为0的应该先判断这个文件是否已经接收过
//     * 没有则创建新的文件
//     */
//    if(pack_num == 0) {
//      /* TODO: 开一个多线程接收文件 <18-07-19, wangbin> */
//      char file_name[100];
//      file_length = *(buf+sizeof(pack_num));
//      //映射数组, 每位表示对应序号的包已经接收到， 包号从1开始，最后一个可能是不完整包， 所以要多一个， 即要多两个空位
//      max_pack_num = file_length/kMaxLength;
//      if (max_pack_num*kMaxLength < file_length) {
//        ++max_pack_num;
//      }
//      pack_is_recved.clear();
//      for (int i = 0; i < max_pack_num; ++i) {
//        pack_is_recved.push_back(false);	
//      }
//      strcpy(file_name, buf+sizeof(pack_num)+sizeof(file_length));
//      std::cout << "new file : " << file_name << std::endl;
//      file_fd = open(file_name, O_CREAT | O_RDWR, 0777);
//      lseek(file_fd, file_length, SEEK_END);
//      write(file_fd, "a", 1);
//      continue;
//    } else {
//      /* TODO: 查看属于哪个文件的包， 分给对应的线程 <18-07-19, wangbin> */
//      lseek(file_fd, (pack_num-1)*kMaxLength, SEEK_SET);
//      pack_is_recved[pack_num] = true;
//      int write_re = write(file_fd, buf+2, str_len-2);
//      if (write_re == -1) {
//        std::cout << file_fd << std::endl;
//        std::cout << strerror(errno) << std::endl;
//      }
//      if (pack_num*kMaxLength >= file_length) {
//          //check file
//        while (true) {
//          bool ok = true;
//          for (int i = 0; i < max_pack_num; ++i) {
//            if (true != pack_is_recved[i]) {
//              ok = false;
//              //重新请求
//              *(CMD*)buf = kRequest;
//              int re = sendto(recv_sock, buf, sizeof(CMD), 0, (sockaddr*)addr, sizeof(*addr));
//              if (-1 == re) {
//                std::cerr << "sendto error" << std::endl;
//                std::cerr << strerror(errno) << std::endl;
//              }
//            }
//          }
//          if (ok) { break; }
//
//          if(pack_num == 0) {
//            continue;
//          } else {
//            lseek(file_fd, (pack_num-1)*kMaxLength, SEEK_SET);
//            pack_is_recved[pack_num] = true;
//            int write_re = write(file_fd, buf+2, str_len-2);
//            if (write_re == -1) {
//              std::cout << file_fd << std::endl;
//              std::cout << strerror(errno) << std::endl;
//            }
//          }
//        }
//      }
//    }
//  }
//  close(recv_sock);
//}/*}}}*/

/*
 * 从sockfd读内容， 判断是否为本文件， 写入文件
 * 计算平均传输时间， 设置定时器， 当超过两倍时间没有发送时， 发送丢包重传请求
 */
void FileRecv (const int sockfd, const sockaddr_in* addr, std::unique_ptr<File>& file_uptr) {
  char buf[kBufSize];
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
      std::cout << pack_num << std::endl;
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
}
