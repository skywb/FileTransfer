#include "fileRecv.h"

#include <cstring>
#include <fcntl.h>

#include <iostream>
#include <vector>

void FileRecv(int recv_sock, std::string save_path = std::string()) {
  int file_fd = -1;
  int str_len = 0;
  char buf[kMaxLength+20];
  while (true) {
    str_len = recvfrom(recv_sock, buf, kMaxLength, 0, NULL, 0);
    if(str_len < 0) break;
    buf[str_len] = 0;
    int pack_num = *buf;
    int file_length = 0;
    std::vector<bool> pack_is_recved;
    int max_pack_num = 0;
    if(pack_num == 0) {
      /* TODO: 开一个多线程接收文件 <18-07-19, wangbin> */
      char file_name[100];
      file_length = *(buf+2);
      //映射数组, 每位表示对应序号的包已经接收到， 包号从1开始，最后一个可能是不完整包， 所以要多一个， 即要多两个空位
      max_pack_num = file_length/kMaxLength;
      if (max_pack_num*kMaxLength < file_length) {
        ++max_pack_num;
      }
      for (int i = 0; i < max_pack_num; ++i) {
        pack_is_recved.push_back(false);	
      }
      strcpy(file_name, buf+4);
      std::cout << "new file : " << file_name << std::endl;
      file_fd = open(file_name, O_CREAT | O_RDWR, 0777);
      lseek(file_fd, file_length, SEEK_END);
      write(file_fd, "a", 1);
      continue;
    } else {
      /* TODO: 查看属于哪个文件的包， 分给对应的线程 <18-07-19, wangbin> */
      lseek(file_fd, (pack_num-1)*kMaxLength, SEEK_SET);
      pack_is_recved[pack_num] = true;
      int write_re = write(file_fd, buf+2, str_len-2);
      if (write_re == -1) {
        std::cout << file_fd << std::endl;
        std::cout << strerror(errno) << std::endl;
      }
      if (pack_num*kMaxLength >= file_length) {
        //check file
        for (int i = 0; i < max_pack_num; ++i) {
          if (true != pack_is_recved[i]) {
            //重新请求
          }
        }

      }

    }
  }
  close(recv_sock);
}

