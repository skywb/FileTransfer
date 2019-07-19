#include "fileSend.h"

#include "reactor/Reactor.h"

#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <iostream>

/*
 * 打开文件， 计算文件的长度
 * 需要的数据包个数  =  文件长度/一个数据包中最大的数据长度(kMaxLength)
 * 返回需要的数据包个数
 */
int  CalculateMaxPages(std::string file_path) {
  int file_fd = -1;
  file_fd = open(file_path.c_str(), O_RDONLY);
  if (file_fd == -1) {
    std::cerr << "open() error" << std::endl;
    std::cerr << strerror(errno) << std::endl;
    exit(1);
  }
  lseek(file_fd, 0, SEEK_SET);
  int file_len = lseek(file_fd, 0, SEEK_END);
  close(file_fd);
  //计算需要总的数据包个数
  int max_pack_num = file_len/1000;
  if (max_pack_num * 10000 < file_len) {
    max_pack_num++;
  }
  return max_pack_num;
}

bool FileSend(std::string group_ip, 
              int port, std::string file_path) {
  //加入多播组
  int send_sock;
  send_sock = socket(AF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in mul_addr;
  memset(&mul_addr, 0, sizeof(mul_addr));
  mul_addr.sin_family = AF_INET;
  mul_addr.sin_addr.s_addr = inet_addr(group_ip.c_str());
  mul_addr.sin_port = htons(port);
  int time_live = kTTL;
  setsockopt(send_sock, IPPROTO_IP, IP_MULTICAST_TTL, 
             (void *)&time_live, sizeof(time_live));
  //发送文件信息   --- 0号数据包
  int max_pack_num = 
    SendFileMessage(send_sock, &mul_addr, file_path);
  //发送文件内容
  int file_fd = -1;
  file_fd = open(file_path.c_str(), O_RDONLY);
  if (file_fd == -1) {
    std::cerr << "open() error" << std::endl;
    std::cerr << strerror(errno) << std::endl;
    exit(1);
  }
  lseek(file_fd, 0, SEEK_SET);
  char buf[kMaxLength+20];
  //buf[0] = 0;
  for (int package_number = 1; 
      package_number <= max_pack_num;
      ++package_number) {
    *buf = package_number;
    int re = read(file_fd, buf+sizeof(package_number), kMaxLength);
    if (re == -1) {
      //读错误
    }
    sendto(send_sock, buf, re+sizeof(package_number), 0, 
           (struct sockaddr *)&mul_addr, sizeof(mul_addr));
    //buf[0] = 0;
    sleep(1); 
  }
  //检查所有的丢包情况， 并重发
  auto reactor = Reactor::GetInstance();
  /* TODO: 应该从文件路径提取文件名 <19-07-19, sky> */
  std::string file_name = file_path;
  auto packages_vec_ptr = reactor->GetFileLostedPackage(file_name);
  //有需要重发的包
  while (packages_vec_ptr) {
    for (auto i : (*packages_vec_ptr)) {
      if (i) {
        SendFileDataAtPackNum(send_sock, &mul_addr, file_path, (*packages_vec_ptr).back());
      }
    }
    packages_vec_ptr = reactor->GetFileLostedPackage(file_name);
  }
  close(file_fd);
  close(send_sock);
  return true;
}


//计算文件的长度， 想多播组发送包号为0的数据包
//数据包格式： 包号（2Bytes)文件长度(2Bytes)文件名(小于100Bytes)
int SendFileMessage(int sockfd, sockaddr_in *addr, 
                    std::string file_path) {
  //计算需要总的数据包个数
  int max_pack_num = CalculateMaxPages(file_path);
  //发送文件信息
  char buf[kMaxLength+20];
  *(buf) = 0;
  *(buf+2) = max_pack_num;
  char file_name[100];
  strcpy(file_name, file_path.c_str());
  ::strncpy(buf+4, file_name, kMaxLength);
  int send_len = sendto(sockfd, buf, 4+strlen(file_name), 
                        0, (struct sockaddr *)addr, sizeof(*addr));
  if (-1 == send_len) {
    std::cerr << strerror(errno) << std::endl;
  }
  return max_pack_num;
}

/*
 * 发送一个数据包给对应的组播地址
 * 若为0号包， 则为文件信息
 * 若包号大于0， 则计算数据在文件的相应位置，并发送
 */
void SendFileDataAtPackNum(int sockfd, sockaddr_in *addr, std::string filename, int package_numbuer) {
  if (package_numbuer == 0) {
    SendFileMessage(sockfd, addr, filename); 
  } else {
    int file_fd = -1;
    file_fd = open(filename.c_str(), O_RDONLY);
    //计算文件最大的包序号， 防止非法操作
    //CalculateMaxPages(filename);
    lseek(file_fd, package_numbuer*kMaxLength, SEEK_SET);
    char buf[kMaxLength+20];
    *(int*)buf = package_numbuer;
    int re = read(file_fd, buf+2, kMaxLength);
    if (-1 == re) {
      std::cerr << "read error " << "request package_num is " << package_numbuer  << 
       " file length is " << lseek(file_fd, 0, SEEK_END) << " kMaxLength is " << kMaxLength << std::endl;
    } else {
      sendto(sockfd, buf, re+2, 0, 
          (struct sockaddr *)&addr, sizeof(*addr));
    }
  }
}

