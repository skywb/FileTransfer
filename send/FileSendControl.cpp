#include "FileSendControl.h"
#include "util/zip.h"
#include <utility>
#include <sys/epoll.h>
#include <iomanip>


FileSendControl::FileSendControl (std::string group_ip, int port) :
                                  group_ip_(group_ip), port_(port),
                                  ip_used_(1000, false), running_(false),
                                  con(group_ip, port) {
  auto ip_net = inet_addr(group_ip_.c_str()); 
  auto ip_local = ntohl(ip_net);
  if (ip_local < kMulticastIpMin || ip_local > kMulticastIpMax) {
    std::cout << "通知多播ip : " << ip_local << std::endl;
    std::cout << "限制ip范围在224.0.2.10 ~ 224.0.2.255之间" << std::endl;
    exit(1);
  }
  ip_used_[ip_local-kMulticastIpMin] = true;
}

FileSendControl::~FileSendControl () {
  //for (auto i : group_addrs) {
  //  delete i.second;
  //}
}
void FileSendControl::Run() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (running_) return;
  std::thread local_t(ListenFileRecvCallback, std::ref(con));
  listen_file_recv_.swap(local_t);
  running_ = true;
}

void FileSendControl::SendFile(std::string file_path) {
  std::lock_guard<std::mutex> lock(mutex_);
  file_path = zip(file_path);
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
    *(int*)(buf+FileSendControl::kFileNameLenBeg) = (int)(file_name.size());
    strncpy(buf+FileSendControl::kFileNameBeg, file_name.c_str(), File::kFileNameMaxLen);
    con.Send(buf, FileSendControl::kFileNameBeg+file_name.size());
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
  auto ctl = GetInstances();
  std::string file_name = file->File_name();
  file_name = file_name.substr(0, file_name.rfind('.'));
  file_name = file_name.substr(0, file_name.rfind('.'));
  ctl->NoticeFront(file_name, Type::kSendend);
  std::string cmd = "rm -f ";
  cmd += file->File_path() + file->File_name();
  system(cmd.c_str());
  std::cout << cmd << std::endl;
  std::cout << std::flush;
  //std::cout << "delete " << file->File_name() << std::endl;
  file.release();
  //end_que_.push(std::make_pair(std::move(file), group_ip_local));
}
void FileSendControl::Recvend(std::unique_ptr<File> file, uint32_t group_ip_local) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = file_is_recving_.find(file->File_name());
  if (it != file_is_recving_.cend()) {
    file_is_recving_.erase(it);
  }
}

//std::string FileSendControl::GetEndFileName() {
//  std::lock_guard<std::mutex> lock_guard(mutex_);
//  if (end_que_.empty()) return std::string();
//  auto endfile = std::move(end_que_.front());
//  end_que_.pop();
//  std::string filename = endfile.first->File_name();
//  endfile.first.release();
//  return filename;
//}

bool FileSendControl::FileIsRecving(std::string file_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = file_is_recving_.find(file_name);
  if (it == file_is_recving_.cend() || it->second == false) {
    file_is_recving_[file_name] = true;
    return false;
  }
  return true;
}

void FileSendControl::FileSendCallback(uint32_t group_ip_local, int port_local, std::unique_ptr<File> file) {
  ulong ip_net_u = htonl(group_ip_local);
  in_addr ip_net;
  memcpy(&ip_net, &ip_net_u, 4);
  std::string ip(inet_ntoa(ip_net));
  //std::cout << "发送文件 ： ip is " << ip << " port : " << port_local << std::endl;
  FileSend(ip, port_local, file);
  //通知已经传输完毕
  auto ctl = FileSendControl::GetInstances();
  ctl->Sendend(std::move(file), group_ip_local);
}

void FileSendControl::RecvFile(std::string group_ip, int port, std::unique_ptr<File> file_uptr) {
  FileRecv(group_ip, port, file_uptr);
  std::string file_name = file_uptr->File_name();
  for (int i = 0; i < 2; ++i) {
    auto pos = file_name.find_last_of('.');
    if (pos == -1) break;
    file_name.erase(pos);
  }
  unzip(file_uptr->File_name(), "./");
  if (file_uptr) {
    auto ctl = FileSendControl::GetInstances();
    ctl->NoticeFront(file_name, Type::kRecvend);
  } else {
    std::cout << "file_uptr 不可用" << std::endl;
  }
}

void FileSendControl::ListenFileRecvCallback(Connecter& con) {
  char buf[kBufSize];
  while (true) {
    memset(buf, 0, kBufSize);
    int cnt = con.Recv(buf, kBufSize, 3000);
    if (cnt == -1) {
      continue;
    } else {
      /*: 解析组播地址和文件名 <24-07-19, 王彬> */
      std::cout << "Recv " << cnt << " 字节" << std::endl;
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
      file_name[filename_len] = 0;
      /*: 判断是否已经传输 <30-07-19, 王彬> */
      auto conse = FileSendControl::GetInstances();
      if (conse->FileIsRecving(file_name)) {
        continue;
      }
      std::string file_name_front(file_name);
      file_name_front = file_name_front.substr(0, file_name_front.rfind('.'));
      file_name_front = file_name_front.substr(0, file_name_front.rfind('.'));
      auto ctl = FileSendControl::GetInstances();
      ctl->NoticeFront(file_name_front, Type::kNewFile);
      auto file = std::make_unique<File> (file_name, file_len, true);
      //转地址
      uint32_t ip_net = htonl(ip_local);
      in_addr ip_addr;
      memcpy(&ip_addr, &ip_net, sizeof(in_addr));
      std::string ip(inet_ntoa(ip_addr));
      std::thread file_recv_thread(RecvFile, ip, port_recv, std::move(file));
      file_recv_thread.detach();
    }
  }
}

void FileSendControl::NoticeFront(std::string file_name, Type type) {
    NoticeFront_(file_name, type);
}
