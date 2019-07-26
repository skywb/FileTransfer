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
  static const int kPortBeg = kGroupIPBeg + sizeof(uint32_t);
  static const int kFileLenBeg = kPortBeg + sizeof(int);
  static const int kFileNameLenBeg = kFileLenBeg + sizeof(int);
  static const int kFileNameBeg = kFileNameLenBeg + sizeof(int);
public:
  virtual ~FileSendControl ();
  void SendFile(std::string file_path);
  void Run();
  void Sendend(std::unique_ptr<File> file, uint32_t group_ip_local);
  std::string GetEndFileName();
  static FileSendControl* GetInstances() {
    static FileSendControl* project = new FileSendControl("224.0.0.10", 8888);
    return project;
  }
  void SetSavePath() = delete;   //留接口，暂不实现
  bool Quit() = delete ;
private:
  FileSendControl (std::string group_ip, int port);
  static void ListenFileRecvCallback(int sockfd);
  static void FileSendCallback(uint32_t group_ip_net, int port_local, std::unique_ptr<File> file);
  static void RecvFile(std::string group_ip, int port, std::unique_ptr<File>& file_uptr);
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

void FileSendControl::ListenFileRecvCallback(int sockfd) {
  char buf[kBufSize];
  sockaddr_in sender_addr;
  socklen_t sender_addr_len = sizeof(sender_addr);
  #if DEBUG
  std::cout << "正在监听..." << std::endl;
  #endif
  int re = recvfrom(sockfd, buf, kBufSize, 0, (sockaddr*)&sender_addr, &sender_addr_len);
  #if DEBUG
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
  std::cout << file_uptr->File_name() << " 接收完毕" << std::endl;
}

#endif /* end of include guard: FILESENDCONTROL_H_HNGFS6UN */
