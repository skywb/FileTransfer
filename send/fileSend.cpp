#include "fileSend.h"
#include "send/File.h"
#include "reactor/Reactor.h"
#include "util/multicastUtil.h"
#include "util/Connecter.h"

#include <string.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>

#include <iostream>

/*
 * 文件的发送
 * 根据传入的组播地址和端口， 绑定之后发送文件
 * 会启动一个线程去监听客户端丢失的udp包， 最后检查所有的丢失的包，再次发送
 * 等待一定的时间，防止无效的报文到达
 * 获取所有网卡绑定的IP，并创建套接字
 * 想每个网卡发送相同的信息
 */
bool FileSend(std::string group_ip, 
              int port, std::unique_ptr<File>& file_uptr) {
  //auto ip_vec = GetAllNetIP();
  //std::vector<std::pair<int, sockaddr_in*>> addrs(ip_vec.size());
  //for (int i = 0; i < ip_vec.size(); ++i) {
  //  addrs[i].second = new sockaddr_in();
  //  JoinGroup(addrs[i].second, &addrs[i].first, group_ip, port, group_ip);
  //}
  Connecter con(group_ip, port);
  LostPackageVec losts(file_uptr->File_max_packages());
  //启动一个线程，监听丢失的包
  std::thread listen(ListenLostPackage, port+1, std::ref(losts), std::ref(con)); //修改端口， 参数： socket， addr, losts
  //设置多播地址
  //发送文件内容
  for (int i = 0; i <= file_uptr->File_max_packages(); ++i) {
    //for (auto addr : addrs) {
    //  //int time_live = kTTL;
    //  //setsockopt(addr.first, IPPROTO_IP, IP_MULTICAST_TTL, 
    //  //    (void *)&time_live, sizeof(time_live));
    //  SendFileDataAtPackNum(addr.first, addr.second, file_uptr, i); 
    //}
    SendFileDataAtPackNum(con, file_uptr, i); 
#if DEBUG
    std::cout << "发送成功" << std::endl;
#endif
  }
#if DEBUG
  std::cout << "发送完毕, 开始校验" << std::endl;
#endif
  //检查所有的丢包情况， 并重发
  while (true) {
    auto lose = losts.GetFileLostedPackage();
    if (lose.empty()) {
      /* TODO:  <22-07-19, 王彬> */
      //等待并检查
      //告知子线程结束， 并回收子线程
      //结束
      //简单代替， 应修改
      const int kSleepTime = 3;
      sleep(kSleepTime);
      lose = losts.GetFileLostedPackage();
      if (lose.empty()) { 
        losts.ExitListen();
        break; 
      }
    } else {
      //重发 
      for (auto i : lose) {
        //for (auto addr : addrs) {
        //  //int time_live = kTTL;
        //  //setsockopt(addr.first, IPPROTO_IP, IP_MULTICAST_TTL, 
        //  //    (void *)&time_live, sizeof(time_live));
        //  SendFileDataAtPackNum(addr.first, addr.second, file_uptr, i); 
        //}
        SendFileDataAtPackNum(con, file_uptr, i); 
      }
    }
  }
#if DEBUG
  std::cout << "结束" << std::endl;
#endif
  listen.join();
  //for (auto i : addrs) {
  //  close(i.first);
  //  delete i.second;
  //}
  return true;
}

//计算文件的长度， 想多播组发送包号为0的数据包
//数据包格式： 包号（4Bytes)文件名长度(4Bytes)文件名(小于100Bytes)
void SendFileMessage(Connecter& con, const std::unique_ptr<File>& file) {
  //发送文件信息
  char buf[kBufSize];
  *(int*)(buf+kPackNumberBeg) = (int)0;
  *(buf+kFileNameLenBeg) = file->File_name().size();
  char file_name[100];
  strcpy(file_name, file->File_name().c_str());
  ::strncpy(buf+kFileNameBeg, file_name, File::kFileNameMaxLen);
  *(int*)(buf+kFileLenBeg) = file->File_len();
  con.Send(buf, kFileLenBeg+sizeof(kFileDataLenBeg));
  //int send_len = sendto(sockfd, buf, kFileLenBeg+sizeof(kFileDataLenBeg), 0, (struct sockaddr *)addr, sizeof(*addr));
  //if (-1 == send_len) {
  //  std::cerr << strerror(errno) << std::endl;
  //}
}

/*
 * 发送一个数据包给对应的组播地址
 * 若为0号包， 则为文件信息
 * 若包号大于0， 则计算数据在文件的相应位置，并发送
 */
void SendFileDataAtPackNum(Connecter& con, const std::unique_ptr<File>& file, int package_numbuer) {
  if (package_numbuer == 0) {
    SendFileMessage(con, file); 
  } else {
    char buf[kBufSize];
    *(int*)(buf+kPackNumberBeg) = package_numbuer;
    *(int*)(buf+kFileNameLenBeg) = file->File_name().size();
    strncpy(buf+kFileNameBeg, file->File_name().c_str(), File::kFileNameMaxLen);
    std::cout << "package_numbuer is " << package_numbuer << std::endl;
    int re = file->Read(package_numbuer, buf+kFileDataBeg);
    if (re < 0) {
      //read error
    }
    *(int*)(buf+kFileDataLenBeg) = re;
    int len = re+kFileDataBeg;
    re = con.Send(buf, len);
    //re = sendto(sockfd, buf, len, 0, 
    //    (sockaddr *)addr, sizeof(*addr));
    if (re < len) {
     //error 
      std::cerr << strerror(errno) << std::endl;
      std::cout << "re is " << re << std::endl;
#if DEBUG
      std::cout << "re < len  in sendto " << __FILE__  << std::endl;
#endif
    }
  }
}

/* 监听丢失的包
 * 保存到LostPackageVec中
 * 线程任务
 */
void ListenLostPackage(int port, LostPackageVec& losts, Connecter& con) {
  char buf[kBufSize];
  //int epoll_root = epoll_create(10);
  //epoll_event events[addrs.size()];
  //for (auto i : addrs) {
  //  events[0].data.fd =i.first;
  //  events[0].events = EPOLLIN;
  //  epoll_ctl(epoll_root, EPOLL_CTL_ADD,i.first, &events[0]);
  //}
  //timeval listen_time;
  //fd_set rd_fd;
  //FD_ZERO(&rd_fd);
  //FD_SET(sockfd, &rd_fd);
	while (losts.isRunning()) {
    //listen_time.tv_sec = 1;
    //listen_time.tv_usec = 0;
    //setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&listen_time, sizeof(timeval));
    //int re = recvfrom(sockfd, buf, kBufSize, 0, (sockaddr*)&addr, &len);
    //int cnt = epoll_wait(epoll_root, events, addrs.size(), 1);
    //if (cnt > 0) {
    //  for (int i = 0; i < cnt; ++i) {
    //    int re = recvfrom(events[i].data.fd, buf, kBufSize, 0, nullptr, nullptr);
    //    if (re > 0) {
    //      int cmd = *(int*)buf;
    //      int package_num = *(int*)(buf+sizeof(cmd));
    //      losts.AddFileLostedRecord(package_num);
    //    }
    //  }
    //}
    int re = con.Recv(buf, kBufSize, 1000);
    if (re > 0) {
      int cmd = *(int*)buf;
      int package_num = *(int*)(buf+sizeof(cmd));
      losts.AddFileLostedRecord(package_num);
    }
	}
}


LostPackageVec::LostPackageVec (int package_count) : 
  package_count_(package_count),
  lost_(package_count_+1), running(true) {
}

LostPackageVec::~LostPackageVec () { }

/*
 * 获取丢失的包的集合， 返回一个vector
 */
std::vector<int> LostPackageVec::GetFileLostedPackage() {
  std::lock_guard<std::mutex> lock(lock_);
  std::vector<int> res;
  for (int i = 0; i <= package_count_; ++i) {
    if (lost_[i]) {
      res.push_back(i);
      lost_[i] = false;
    } 
  }
  return res;
}

//添加一个丢失记录
void LostPackageVec::AddFileLostedRecord(int package_num) {
  std::lock_guard<std::mutex> lock(lock_);
  lost_[package_num] = true;
}
  //尝试退出子线程, 若没有数据包
  //若没有数据包，则退出子线程，并返回true
  //否则返回false
bool LostPackageVec::ExitListen() {
  std::lock_guard<std::mutex> lock(lock_);
  running = false;
  return true; 
}
bool LostPackageVec::isRunning() {
  std::lock_guard<std::mutex> lock(lock_);
  return  running;
}
