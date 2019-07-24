#ifndef FILESENDCONTROL_H_HNGFS6UN
#define FILESENDCONTROL_H_HNGFS6UN

#include "File.h"
#include "util/multicastUtil.h"
#include "send/fileSend.h"
#include "recv/fileRecv.h"

#include <cstring>
#include <arpa/inet.h>
#include <queue>
#include <memory>
#include <mutex>
#include <thread>

/*
 * 启动一个线程监听是否有文件的发送
 * 一个任务队列，用于处理要发送的文件
 * 启动一个线程发送文件
 */
class FileSendControl
{
public:
  enum Type {
    kNewFile = 1,
    kAlive = 2
  };
  //224.0.0.10
  static const uint32_t kMulticastIpMin = 3758096394;
  //224.0.0.255
  static const uint32_t kMulticastIpMax = 3758096639;
  static const int kTypeBeg = 0;
  static const int kGroupIPBeg = kTypeBeg + sizeof(Type);
  static const int kPortBeg = kGroupIPBeg + sizeof(in_addr_t);
  static const int kFileLenBeg = kPortBeg + sizeof(int);
  static const int kFileNameLenBeg = kPortBeg + sizeof(int);
  static const int kFileNameBeg = kFileNameLenBeg + sizeof(int);
public:
  virtual ~FileSendControl ();
  void SendFile(std::string file_path);
  void run();
  void Sendend(std::unique_ptr<File> file, uint32_t group_ip_local);
  static FileSendControl* GetInstances() {
    static FileSendControl* project = new FileSendControl("224.0.0.10", 8888);
    return project;
  }
  void SetSavePath() = delete;   //留接口，暂不实现
  bool Quit() = delete ;

private:
  FileSendControl (std::string group_ip, int port);
  static void ListenFileRecvCallback(std::string group_ip, int port);
  static void FileSendCallback(uint32_t group_ip_net, int port_local, std::unique_ptr<File> file);
  std::queue<std::unique_ptr<File>> task_que_;
  std::mutex mutex_;
  std::string group_ip_;
  int port_;
  std::vector<bool> ip_used_;
  std::thread listen_file_recv_;
  bool running_;
  std::queue<std::pair<std::unique_ptr<File>, uint32_t>> end_que_;
  //std::string save_path_;
  sockaddr_in group_addr;
  int group_sockfd;
};

void FileSendControl::ListenFileRecvCallback(std::string group_ip, int port) {
  sockaddr_in addr;
  int sockfd;
  if (false == JoinGroup(&addr, &sockfd, group_ip, port)) {
    std::cerr << "JoinGroup faile" << std::endl;
    std::cerr << strerror(errno) << std::endl;
    exit(1);
  }
  char buf[kBufSize];
  sockaddr_in sender_addr;
  socklen_t sender_addr_len = sizeof(sender_addr);
  int re = recvfrom(sockfd, buf, kBufSize, 0, (sockaddr*)&sender_addr, &sender_addr_len);
  /* TODO: 解析组播地址和文件名 <24-07-19, 王彬> */
  FileSendControl::Type type = (FileSendControl::Type)*(buf+kTypeBeg);
  if (type != FileSendControl::kNewFile) {
    std::cout << "type != kNewFile" << std::endl;
  }
  uint32_t ip_net = *(buf+kGroupIPBeg);
  int port_recv = *(buf+kPortBeg);
  int filename_len = *(buf+FileSendControl::kFileNameLenBeg);
  int file_len = *(buf+FileSendControl::kFileLenBeg);
  char file_name[File::kFileNameMaxLen];
  strncpy(file_name, buf+FileSendControl::kFileNameBeg, File::kFileNameMaxLen);
  auto file = std::make_unique<File> (file_name, file_len, true);
  //转地址
  in_addr ip_addr;
  memcpy(&ip_addr, &ip_net, 4);
  std::string ip(inet_ntoa(ip_addr));
  FileRecv(ip, port_recv, file);
}

void FileSendControl::FileSendCallback(uint32_t group_ip_local, int port_local, std::unique_ptr<File> file) {
  ulong ip_net_u = htonl(group_ip_local);
  in_addr ip_net;
  memcpy(&ip_net, &ip_net_u, 4);
  std::string ip(inet_ntoa(ip_net));
  std::cout << "ip is " << ip << std::endl;
  FileSend(ip, port_local, file);
  //通知已经传输完毕
  auto ctl = FileSendControl::GetInstances();
  ctl->Sendend(std::move(file), group_ip_local);
}

#endif /* end of include guard: FILESENDCONTROL_H_HNGFS6UN */
