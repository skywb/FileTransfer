#ifndef FILESENDCONTROL_H_HNGFS6UN
#define FILESENDCONTROL_H_HNGFS6UN

#include "recv/fileRecv.h"

#include <list>
#include <thread>
#include <functional>
#include <boost/uuid/uuid.hpp>

/*文件通知信息
 * 封装成类便于操作
 */
struct FileNotce {
  std::string file_name_;   //文件名  *.zip
  boost::uuids::uuid uuid_;  //文件唯一标识
  int len_ = 0;
  char buf_[kBufSize];
  bool end_;                //true标识已经传输/发送完毕
  std::chrono::time_point<std::chrono::system_clock> clock_;   //最后一次更新时间
};

/* 文件发送和接收的总控制  单例的
 * 随对象创建时启动两个线程， 运行知道程序结束
 * 1. 监听线程， 监听是否有其他端要发送文件
 * 2. 在发送文件期间不停通知其他端接收自己在发送的文件
 */
class FileSendControl
{
public:
  enum Type {
    kNewFile = 1,    //接收的新的文件
    kAlive = 2,      // 心跳包， 保活
    kReSend = 3,     // 重发请求
    kRecvend = 4,    // 接收完毕
    kSendend = 5,    // 发送完毕
    kNewSendFile = 6 // 新发送一个文件
  };
  static const uint32_t kMulticastIpMin = 3758096897; //224.0.0.10
  static const uint32_t kMulticastIpMax = 4009754623; //224.0.0.255
  static const int kTypeBeg = 0;
  static const int kGroupIPBeg = kTypeBeg + sizeof(Type);
  static const int kPortBeg = kGroupIPBeg + sizeof(uint32_t);
  static const int kFileLenBeg = kPortBeg + sizeof(int);
  static const int kFileUUIDBeg = kFileLenBeg + sizeof(int);
  static const int kFileNameLenBeg = kFileUUIDBeg + sizeof(boost::uuids::uuid);
  static const int kFileNameBeg = kFileNameLenBeg + sizeof(int);
public:
  virtual ~FileSendControl ();
  void SendFile(std::string file_path);
  void RegistNoticeFrontCallBack(std::function<void(const boost::uuids::uuid,
                                 const FileSendControl::Type,
                                 const std::vector<std::string>)> func) { NoticeFront_ = func; }
  void NoticeFront(const boost::uuids::uuid file_uuid,
                   const FileSendControl::Type type,
                   const std::vector<std::string> msg = std::vector<std::string>());
  /* 通知客户端接收该文件
   * 通知发送一次， 发送所有正在发送的文件头信息
   * 同时检查所有失效的uuid
  */
  void SendNoticeToClient();
  /* 开始运行， 创建该对象后需要调用run才会接收和发送文件*/
  void Run();
  /* 发送文件结束， 处理相关的资源和通知 */
  void Sendend(std::unique_ptr<File> file, uint32_t group_ip_local);
  /* 接收文件结束， 处理相关的资源和通知 */
  void Recvend(std::unique_ptr<File> file);
  /* 单例接口 */
  static FileSendControl* GetInstances() {
    static FileSendControl* project = new FileSendControl("224.0.2.10", 8888);
    return project;
  }
  /* 检查该文件是否正在接收 */
  bool FileIsRecving(boost::uuids::uuid file_uuid);
  /* 添加一个正在接收的文件状态 */
  void AddRecvingFile(std::unique_ptr<FileNotce> file_notice) {
    std::lock_guard<std::mutex> lock(mutex_);
    file_is_recving_.push_back(std::move(file_notice));
  }
  void SetSavePath() = delete;   //留接口，暂不实现
  bool Quit() = delete ;
private:
  FileSendControl (std::string group_ip, int port);
  /* 监听是否有其他端要发送文件 的线程函数 */
  static void ListenFileRecvCallback(Connecter& con);
  /* 通知所有接收端的线程函数 */
  static void SendNoticeToClientCallback();
  /* 发送文件回调函数 */
  static void FileSendCallback(uint32_t group_ip_net, int port_local, std::unique_ptr<File> file);
  /* 接收文件的线程函数 */
  static void RecvFile(std::string group_ip, int port, std::unique_ptr<File> file_uptr);
private:
  //std::queue<std::unique_ptr<File>> task_que_;
  std::mutex mutex_;
  std::string group_ip_;   //用于接收和发送通知的组播地址
  int port_;               // 对应端口
  std::vector<bool> ip_used_;
  std::thread listen_file_recv_;
  bool running_;   //状态机
  std::list<std::unique_ptr<FileNotce>> file_is_recving_;
  std::list<std::unique_ptr<FileNotce>> file_is_sending_;
  Connecter con;    //组播连接器
  //注册的 通知前端的回调函数
  std::function<void(const boost::uuids::uuid file_uuid,
                     const FileSendControl::Type type,
                     const std::vector<std::string>)> NoticeFront_;
};

#endif /* end of include guard: FILESENDCONTROL_H_HNGFS6UN */
