#include "FileSendControl.h"


FileSendControl::FileSendControl (std::string group_ip, int port) :
  group_ip_(group_ip), port_(port),
  ip_used_(1000, false), running_(false) {
  auto ip_net = inet_addr(group_ip_.c_str()); 
  if (ip_net < kMulticastIpMin || ip_net > kMulticastIpMax) {
    std::cout << "限制ip范围在224.0.0.10 ~ 224.0.0.255之间" << std::endl;
    exit(1);
  }
  ip_used_[ip_net-kMulticastIpMin] = true;
  if (false == JoinGroup(&group_addr, &group_sockfd, group_ip, port)) {
    //发生错误
    std::cerr << "JoinGroup faile" << std::endl;
    std::cerr << strerror(errno) << std::endl;
    exit(1);
  }
}
FileSendControl::~FileSendControl () {
  /* TODO:  <24-07-19, 王彬> */
}
void FileSendControl::run() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (running_) return;
  std::thread local_t(ListenFileRecvCallback, group_ip_, port_);
  listen_file_recv_.swap(local_t);
  running_ = true;
}

void FileSendControl::SendFile(std::string file_path) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto file = std::make_unique<File>(file_path);
  char buf[kBufSize];
  *(buf+kTypeBeg) = FileSendControl::kNewFile;
  //找到一个可用的ip
  uint32_t ip_net = kMulticastIpMin;
  while(ip_used_[ip_net] && ip_net <= kMulticastIpMax+1) ++ip_net;
  if (ip_net <= kMulticastIpMax) {
    //待改进
    int port = ip_net - kMulticastIpMin;
    if (ip_net % 2) {
      port += 10000;
    } else {
      port += 20000;
    }
    *(buf+kGroupIPBeg) = ip_net;
    *(buf+kPortBeg) = port;
    std::string file_name = file->File_name();
    int file_len = file->File_len();
    *(buf+kFileLenBeg) = file_len;
    *(buf+FileSendControl::kFileNameLenBeg) = file_name.size();
    strncpy(buf+FileSendControl::kFileNameBeg, file_name.c_str(), File::kFileNameMaxLen);
    sendto(group_sockfd, buf, FileSendControl::kFileNameBeg+file_name.size(), 0, (sockaddr*)&group_addr, sizeof(group_addr));
    std::thread th(FileSendCallback, ip_net, port, std::move(file));
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
