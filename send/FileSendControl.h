#ifndef FILESENDCONTROL_H_HNGFS6UN
#define FILESENDCONTROL_H_HNGFS6UN

#include "File.h"
#include "util/Connecter.h"

#include <list>
#include <thread>
#include <functional>
#include <mutex>
#include <cstring>
#include <boost/uuid/uuid.hpp>


class Proto {
public:
  enum Type {
    kUnavailable = 0,    //新的文件
    kNewFile = 1,    //新的文件
    kAlive = 2,      // 心跳包， 保活
    kReSend = 3,     // 重发请求
    kData = 4         // 数据包
  };
private:
  static const int kTypeBeg = 0;
  static const int kPackNumberBeg = kTypeBeg + sizeof(Type);
  static const int kAckPackNumberBeg = kPackNumberBeg + sizeof(int);
  static const int kGroupIPBeg = kAckPackNumberBeg + sizeof(int);
  static const int kPortBeg = kGroupIPBeg + sizeof(uint32_t);
  static const int kFileLenBeg = kPortBeg + sizeof(int);
  static const int kFileUUIDBeg = kFileLenBeg + sizeof(int);
  static const int kFileNameLenBeg = kFileUUIDBeg + sizeof(boost::uuids::uuid);
  static const int kFileNameBeg = kFileNameLenBeg + sizeof(int);
  static const int kFileDataLenBeg = kFileNameBeg + File::kFileNameMaxLen;
  static const int kFileDataBeg = kFileDataLenBeg  + sizeof(int);
public:
  static const int kPackHadeMaxLen = kFileDataLenBeg;
  //读取消息内容
  //Proto (const char *buf, const int len);
  Proto () { memset(buf_, 0, BUFSIZ);}
  //bool Analysis();
  void set_type(Type type) { *(Type*)(buf_+kTypeBeg) = type; }
  void set_group_ip(uint32_t group_ip) { *(uint32_t*)(buf_+kGroupIPBeg) = group_ip; }
  void set_port(int port) { *(int*)(buf_+kPortBeg) = port; }
  void set_file_len(int len) { *(int*)(buf_+kFileLenBeg) = len; }
  void set_uuid(boost::uuids::uuid uuid) { *(boost::uuids::uuid*)(buf_+kFileUUIDBeg) = uuid; }
  void set_file_name(std::string filename) {
    strncpy(buf_+kFileNameBeg, filename.c_str(), 100);
    *(int*)(buf_+kFileNameLenBeg) = filename.length();
  }
  void set_ack_package_number(int ack_num) { *(int*)(buf_+kAckPackNumberBeg) = ack_num; }
  void set_package_number(int package_num) { *(int*)(buf_+kPackNumberBeg) = package_num; }
  void set_file_data_len(int len) { *(int*)(buf_+kFileDataLenBeg) = len; }
  //void set_file_data(const char* data) { memcpy(buf_+kFileDataBeg, data, file_data_len_); }
  char* get_file_data_buf_ptr() { return buf_+kFileDataBeg;}
  const Type type() const { return *(Type*)(buf_+kTypeBeg); }
  const uint32_t group_ip() const { return  *(uint32_t*)(buf_+kGroupIPBeg); }
  const int port() const { return *(int*)(buf_+kPortBeg); }
  const boost::uuids::uuid& uuid() const { return *(boost::uuids::uuid*)(buf_+kFileUUIDBeg); }
  const int file_len() const { return *(int*)(buf_+kFileLenBeg); }
  const std::string file_name() const  {
    return std::string(buf_+kFileNameBeg, *(int*)(buf_+kFileNameLenBeg)); 
  }
  const int package_numbuer() const  { return *(int*)(buf_+kPackNumberBeg); }
  const int ack_package_number() {  return *(int*)(buf_+kAckPackNumberBeg); }
  const int file_data_len() const { return *(int*)(buf_+kFileDataLenBeg); }
  const int get_send_len() const;
  char* buf() {return buf_;}
  virtual ~Proto () {}
private:
  char buf_[BUFSIZ];
};


const int kBufSize = Proto::kPackHadeMaxLen+File::kFileDataMaxLength+10;

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
    kRecvend = 2,    // 接收完毕
    kSendend = 3,    // 发送完毕
    kNewSendFile = 4,
    kClientExec = 5,
    kNetError = 6
  };

  static const uint32_t kMulticastIpMin = 3758096897; //224.0.0.10
  static const uint32_t kMulticastIpMax = 4009754623; //224.0.0.255
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
   * 同时检查所有失效的uuid */
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
  char alive_msg_[100];
};

#endif /* end of include guard: FILESENDCONTROL_H_HNGFS6UN */
