#include "FileSendControl.h"


FileSendControl::FileSendControl (std::string group_ip, int port) :
  group_ip_(group_ip), port_(port),
  ip_used_(1000, false), running_(false) {
  auto ip_net = inet_addr(group_ip_.c_str()); 
  auto ip_local = ntohl(ip_net);
  if (ip_local < kMulticastIpMin || ip_local > kMulticastIpMax) {
    std::cout << "事件ip : " << ip_local << std::endl;
    std::cout << "限制ip范围在224.0.0.10 ~ 224.0.0.255之间" << std::endl;
    exit(1);
  }
  ip_used_[ip_local-kMulticastIpMin] = true;
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
void FileSendControl::Run() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (running_) return;
  std::thread local_t(ListenFileRecvCallback, group_sockfd);
  listen_file_recv_.swap(local_t);
  running_ = true;
}

void FileSendControl::SendFile(std::string file_path) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto file = std::make_unique<File>(file_path);
  char buf[kBufSize];
  *(buf+kTypeBeg) = FileSendControl::kNewFile;
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
    *(uint32_t*)(buf+kGroupIPBeg) = ip_local;
    *(buf+kPortBeg) = port;
    std::string file_name = file->File_name();
    int file_len = file->File_len();
    *(buf+kFileLenBeg) = file_len;
    *(buf+FileSendControl::kFileNameLenBeg) = file_name.size();
    strncpy(buf+FileSendControl::kFileNameBeg, file_name.c_str(), File::kFileNameMaxLen);
    sendto(group_sockfd, buf, FileSendControl::kFileNameBeg+file_name.size(), 0, (sockaddr*)&group_addr, sizeof(group_addr));
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
