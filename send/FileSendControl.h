#ifndef FILESENDCONTROL_H_HNGFS6UN
#define FILESENDCONTROL_H_HNGFS6UN

#include "File.h"
#include "util/Connecter.h"

#include <list>
#include <thread>
#include <functional>
#include <mutex>
#include <boost/uuid/uuid.hpp>


class Proto
{
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
  static const int kGroupIPBeg = kTypeBeg + sizeof(Type);
  static const int kPortBeg = kGroupIPBeg + sizeof(uint32_t);
  static const int kFileLenBeg = kPortBeg + sizeof(int);
  static const int kFileUUIDBeg = kFileLenBeg + sizeof(int);
  static const int kFileNameLenBeg = kFileUUIDBeg + sizeof(boost::uuids::uuid);
  static const int kFileNameBeg = kFileNameLenBeg + sizeof(int);

  static const int kPackNumberBeg = kTypeBeg + sizeof(Type);
  static const int kFileDataLenBeg = kFileNameBeg + File::kFileNameMaxLen;
  static const int kFileDataBeg = kFileDataLenBeg  + sizeof(int);
public:
  static const int kPackHadeMaxLen = kFileDataLenBeg;
  //读取消息内容
  Proto (const char *buf, const int len);
  Proto () { type_ = kUnavailable; }
  ////发送文件信息
  //Proto (Type type, uint32_t group_ip_local, int port,
  //        int file_len, boost::uuids::uuid uuid,
  //        std::string file_name);
  ////文件数据包
  //Proto (Type type, int package_num,
  //        std::string file_name, std::string file_data);

  void set_type(Type type) { type_ = type; }
  void set_group_ip(uint32_t group_ip) { group_ip_ = group_ip; }
  void set_port(int port) { port_ = port; }
  void set_file_len(int len) { file_len_ = len; }
  void set_uuid(boost::uuids::uuid uuid) { uuid_ = uuid; }
  void set_file_name(std::string filename) { file_name_ = filename; }
  void set_package_number(int package_num) { package_num_ = package_num; }
  void set_file_data(std::string data) { file_data_ = data; }

  const Type type() const { return type_; }
  const uint32_t group_ip() const { return  group_ip_; }
  const int port() const { return port_; }
  const boost::uuids::uuid& uuid() const { return uuid_; }
  const int file_len() const { return file_len_; }
  const std::string& file_name() const  { return file_name_; }
  const int package_numbuer() const  { return package_num_; }
  const int file_data_len() const { return file_data_.size(); }
  const std::string& file_data() const  { return file_data_; }
  const int buf(Type type, char* buf, int& len);
  virtual ~Proto ();
private:
  Type type_;
  uint32_t group_ip_;
  int port_;
  int file_len_;
  boost::uuids::uuid uuid_;
  std::string file_name_;
  int package_num_;
  std::string file_data_;
  char* buf_;
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
  //enum Type {
  //  kNewFile = 1,    //接收的新的文件
  //  kAlive = 2,      // 心跳包， 保活
  //  kReSend = 3,     // 重发请求
  //  kSendend = 5,    // 发送完毕
  //  kNewSendFile = 6, // 新发送一个文件
  //  kData = 7         // 数据包
  //};
  enum Type {
    kNewFile = 1,    //接收的新的文件
    kRecvend = 2,    // 接收完毕
    kSendend = 3,    // 发送完毕
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
