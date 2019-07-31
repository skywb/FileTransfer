#ifndef FILESENDCONTROL_H_HNGFS6UN
#define FILESENDCONTROL_H_HNGFS6UN

#include "File.h"
#include "util/multicastUtil.h"
#include "send/fileSend.h"
#include "recv/fileRecv.h"
#include "util/Connecter.h"

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
  static const uint32_t kMulticastIpMin = 3758096897;
  //224.0.0.255
  static const uint32_t kMulticastIpMax = 4009754623;
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
    static FileSendControl* project = new FileSendControl("224.0.2.10", 8888);
    return project;
  }
  void SetSavePath() = delete;   //留接口，暂不实现
  bool Quit() = delete ;
private:
  FileSendControl (std::string group_ip, int port);
  static void ListenFileRecvCallback(Connecter& con);
  static void FileSendCallback(uint32_t group_ip_net, int port_local, std::unique_ptr<File> file);
  static void RecvFile(std::string group_ip, int port, std::unique_ptr<File> file_uptr);
  std::queue<std::unique_ptr<File>> task_que_;
  std::mutex mutex_;
  std::string group_ip_;
  int port_;
  std::vector<bool> ip_used_;
  std::thread listen_file_recv_;
  bool running_;
  std::queue<std::pair<std::unique_ptr<File>, uint32_t>> end_que_;
  //std::string save_path_;
  //sockaddr_in group_addr;
  //int group_sockfd;
  //std::vector<std::pair<int, sockaddr_in*>> group_addrs;
  Connecter con;
};




#endif /* end of include guard: FILESENDCONTROL_H_HNGFS6UN */
