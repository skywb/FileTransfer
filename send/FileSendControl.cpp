#include "FileSendControl.h"
#include <utility>
#include <sys/epoll.h>


FileSendControl::FileSendControl (std::string group_ip, int port) :
                                  group_ip_(group_ip), port_(port),
                                  ip_used_(1000, false), running_(false) {
  auto ip_net = inet_addr(group_ip_.c_str()); 
  auto ip_local = ntohl(ip_net);
  if (ip_local < kMulticastIpMin || ip_local > kMulticastIpMax) {
    std::cout << "通知多播ip : " << ip_local << std::endl;
    std::cout << "限制ip范围在224.0.2.10 ~ 224.0.2.255之间" << std::endl;
    exit(1);
  }
  ip_used_[ip_local-kMulticastIpMin] = true;
  auto ip_vec = GetAllNetIP();
  for (auto ip : ip_vec) {
    auto addr = new sockaddr_in();
    int sockfd;
    if(JoinGroup(addr, &sockfd, group_ip, port)) {
      group_addrs.push_back(std::make_pair(sockfd, addr));
    } else {
      std::cout << ip << " 绑定失败" << std::endl;
    }
  }
}
FileSendControl::~FileSendControl () {
  /* TODO:  <24-07-19, 王彬> */
  for (auto i : group_addrs) {
    delete i.second;
  }
}
void FileSendControl::Run() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (running_) return;
  std::thread local_t(ListenFileRecvCallback, group_addrs);
  listen_file_recv_.swap(local_t);
  running_ = true;
}

void FileSendControl::SendFile(std::string file_path) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto file = std::make_unique<File>(file_path);
  char buf[kBufSize];
  //找到一个可用的ip
  uint32_t ip_local = kMulticastIpMin;
  while(ip_used_[ip_local-kMulticastIpMin] && ip_local <= kMulticastIpMax+1) ++ip_local;
  if (ip_local <= kMulticastIpMax) {
    //待改进
    int port = ip_local - kMulticastIpMin;
    if (port % 2) {
      port += 10000;
    } else {
      port += 20000;
    }
    std::string file_name = file->File_name();
    int file_len = file->File_len();
    *(FileSendControl::Type*)(buf+FileSendControl::kTypeBeg) = FileSendControl::kNewFile;
    *(uint32_t*)(buf+FileSendControl::kGroupIPBeg) = ip_local;
    *(int*)(buf+FileSendControl::kPortBeg) = port;
    *(int*)(buf+FileSendControl::kFileLenBeg) = file_len;
    *(int*)(buf+FileSendControl::kFileNameLenBeg) = (int)file_name.size();
    strncpy(buf+FileSendControl::kFileNameBeg, file_name.c_str(), File::kFileNameMaxLen);
    for (auto i : group_addrs) {
      sendto(i.first, buf, FileSendControl::kFileNameBeg+file_name.size(), 0, (sockaddr*)i.second, sizeof(sockaddr_in));
    }
    std::thread th(FileSendCallback, ip_local, port, std::move(file));
    th.detach();
  } else {
#if DEBUG
    std::cout << "文件过多" << std::endl;
#endif
    //当文件数量太多时加入队列
    //task_que_.push(std::make_unique<File>(file_path));
  }
}

//文件发送结束
void FileSendControl::Sendend(std::unique_ptr<File> file, uint32_t group_ip_local) {
  std::lock_guard<std::mutex> lock(mutex_);
  ip_used_[group_ip_local-kMulticastIpMin] = false;
  end_que_.push(std::make_pair(std::move(file), group_ip_local));
}

std::string FileSendControl::GetEndFileName() {
  std::lock_guard<std::mutex> lock_guard(mutex_);
  if (end_que_.empty()) return std::string();
  auto endfile = std::move(end_que_.front());
  end_que_.pop();
  std::string filename = endfile.first->File_name();
  endfile.first.release();
  return filename;
}

void FileSendControl::FileSendCallback(uint32_t group_ip_local, int port_local, std::unique_ptr<File> file) {
  ulong ip_net_u = htonl(group_ip_local);
  in_addr ip_net;
  memcpy(&ip_net, &ip_net_u, 4);
  std::string ip(inet_ntoa(ip_net));
  std::cout << "发送文件 ： ip is " << ip << " port : " << port_local << std::endl;
  FileSend(ip, port_local, file);
  //通知已经传输完毕
  auto ctl = FileSendControl::GetInstances();
  ctl->Sendend(std::move(file), group_ip_local);
}
void FileSendControl::RecvFile(std::string group_ip, int port, std::unique_ptr<File>& file_uptr) {
  std::cout << __FILE__ << " : " << __LINE__ << " : ip is " << group_ip << " port " << port << std::endl;
  FileRecv(group_ip, port, file_uptr);
  if (file_uptr) {
    std::cout << file_uptr->File_name() << " 接收完毕" << std::endl;
  } else {
    std::cout << "file_uptr 不可用" << std::endl;
  }
  //std::cout << file_uptr->File_name() << " 接收完毕" << std::endl;
}

void FileSendControl::ListenFileRecvCallback(std::vector<std::pair<int, sockaddr_in*>> sock_vec) {
  char buf[kBufSize];
  int epoll_root = epoll_create(10);
  epoll_event events[sock_vec.size()];
  int sock_cnt = sock_vec.size();
  for (int i = 0; i < sock_cnt; ++i) {
    events[i].data.fd = sock_vec[i].first;
    events[i].events = EPOLLIN;
    epoll_ctl(epoll_root, EPOLL_CTL_ADD, sock_vec[i].first, &events[i]);
  }
  int cnt = epoll_wait(epoll_root, events, sock_cnt, 3);
  if (cnt == 0) {
    std::cout << "超时没消息" << std::endl;
  } else {
    for (int i = 0; i < cnt; ++i) {
      int re = recvfrom(events[i].data.fd, buf, kBufSize, 0, nullptr, nullptr);
#if DEBUG
      std::cout << "ip " << *(uint32_t*)(buf+FileSendControl::kGroupIPBeg) << std::endl;
      std::cout << "port " << *(int*)(buf+FileSendControl::kPortBeg) << std::endl;
      std::cout << "filelen " << *(int*)(buf+FileSendControl::kFileLenBeg) << std::endl;
      std::cout << "filenamelen " << *(int*)(buf+FileSendControl::kFileNameLenBeg) << std::endl;
      std::cout << "new file  " << __FILE__ << std::endl;
#endif
      /*: 解析组播地址和文件名 <24-07-19, 王彬> */
      FileSendControl::Type type = (FileSendControl::Type)*(buf+kTypeBeg);
      if (type != FileSendControl::kNewFile) {
        std::cout << "type != kNewFile" << std::endl;
      }
      uint32_t ip_local = *(uint32_t*)(buf+kGroupIPBeg);
      int port_recv = *(int*)(buf+kPortBeg);
      int filename_len = *(int*)(buf+FileSendControl::kFileNameLenBeg);
      int file_len = *(int*)(buf+FileSendControl::kFileLenBeg);
      char file_name[File::kFileNameMaxLen];
      strncpy(file_name, buf+FileSendControl::kFileNameBeg, File::kFileNameMaxLen);
#if DEBUG
      std::cout << "port " << port_recv << std::endl;
      std::cout << filename_len << std::endl;
      file_name[filename_len] = '1';
      file_name[filename_len+1] = 0;
#endif
      /* TODO: 判断是否已经传输 <30-07-19, 王彬> */
      auto file = std::make_unique<File> (file_name, file_len, true);
      //转地址
      uint32_t ip_net = htonl(ip_local);
      in_addr ip_addr;
      memcpy(&ip_addr, &ip_net, sizeof(in_addr));
      std::string ip(inet_ntoa(ip_addr));
      std::cout << __FILE__ << ":line " << __LINE__ << "ip is " << ip << std::endl;
      std::thread file_recv_thread(RecvFile, ip, port_recv, std::ref(file));
      file_recv_thread.detach();
    }
  }
}
