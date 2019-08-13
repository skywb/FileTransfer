#include "FileSendControl.h"
#include "fileSend.h"
#include "recv/fileRecv.h"
#include "util/zip.h"
#include "File.h"

#include <utility>
#include <sys/epoll.h>
#include <iomanip>
#include <thread>
#include <arpa/inet.h>
#include <boost/uuid/uuid_generators.hpp>
#include <algorithm>


//读取消息内容
Proto::Proto (const char *buf, const int len) {
  type_ = *(Type*)(buf + kTypeBeg);
  switch (type_) {
    case kAlive:
      break;
    case kReSend:
      package_num_ = *(int*)(buf+kPackNumberBeg);
      break;
    case kNewFile: 
      group_ip_ = *(uint32_t*)(buf+kGroupIPBeg);
      port_ = *(int*)(buf+kPortBeg);
      file_len_ = *(int*)(buf+kFileLenBeg);
      uuid_ = *(boost::uuids::uuid*)(buf+kFileUUIDBeg);
      file_name_ = std::string(buf+kFileNameBeg, *(int*)(buf+kFileNameLenBeg));
      break;
    case kData:
      package_num_ = *(int*)(buf+kPackNumberBeg);
      file_name_ = std::string(buf+kFileNameBeg, *(int*)(buf+kFileNameLenBeg));
      file_data_len_ = *(int*)(buf+kFileDataLenBeg);
      //memcpy(file_data_, buf+kFileDataBeg, file_data_len_);
      //strncpy(file_data_, buf+kFileDataBeg, file_data_len_);
      break;
  }
}
const int Proto::get_send_len() const {
    Type type = *(Type*)(buf_ + kTypeBeg);
    switch (type) {
      case kAlive:
        return sizeof(Type);
      case kReSend:
        return kPackNumberBeg + sizeof(int);
      case kNewFile:
        return kFileNameBeg + *(int*)(buf_+kFileNameLenBeg);
      case kData:
        return kFileDataBeg + file_data_len();
    }
    return 0;
}
bool Proto::Analysis() {
    type_ = *(Type*)(buf_ + kTypeBeg);
    switch (type_) {
      case kAlive:
        break;
      case kReSend:
        package_num_ = *(int*)(buf_+kPackNumberBeg);
        break;
      case kNewFile:
        group_ip_ = *(uint32_t*)(buf_+kGroupIPBeg);
        port_ = *(int*)(buf_+kPortBeg);
        file_len_ = *(int*)(buf_+kFileLenBeg);
        uuid_ = *(boost::uuids::uuid*)(buf_+kFileUUIDBeg);
        file_name_ = std::string(buf_+kFileNameBeg, *(int*)(buf_+kFileNameLenBeg));
        break;
      case kData:
        package_num_ = *(int*)(buf_+kPackNumberBeg);
        file_name_ = std::string(buf_+kFileNameBeg, *(int*)(buf_+kFileNameLenBeg));
        file_data_len_ = *(int*)(buf_+kFileDataLenBeg);
        //strncpy(file_data_, buf+kFileDataBeg, file_data_len_);
        break;
    }
    return true;
}
////发送文件信息
//Proto::Proto (Type type, uint32_t group_ip_local, int port,
//    int file_len, boost::uuids::uuid uuid,
//    std::string file_name) {
//
//}
////文件数据包
//Proto::Proto (Type type, int package_num,
//    std::string file_name, std::string file_data)  {
//}

//const int Proto::buf(Type type, char* buf, int& len) {
//  if (type != type_) return -1;
//  //buf_ = buf;
//  *(Type*)(buf_+kTypeBeg) = type_;
//  len = -1;
//  switch (type_) {
//    case kAlive:
//      len = sizeof(Type);
//      break;
//    case kReSend:
//      *(int*)(buf_+ kPackNumberBeg) = package_num_;
//      len = kPackNumberBeg + sizeof(int);
//      break;
//    case kNewFile:
//      *(uint32_t*)(buf_+kGroupIPBeg) = group_ip_;
//      *(int*)(buf_+kPortBeg) = port_;
//      *(int*)(buf_+kFileLenBeg) = file_len_;
//      *(boost::uuids::uuid*)(buf_+kFileUUIDBeg) = uuid_;
//      *(int*)(buf_+kFileNameLenBeg) = (int)(file_name_.length());
//      strncpy(buf_+kFileNameBeg, file_name_.c_str(), File::kFileNameMaxLen);
//      len = kFileNameBeg + std::min((int)file_name_.size(), 100);
//      break;
//    case kData:
//      *(int*)(buf_+kPackNumberBeg) = package_num_;
//      *(int*)(buf_+kFileNameLenBeg) = (int)file_name_.length();
//      strncpy(buf_+kFileNameBeg, file_name_.c_str(), File::kFileNameMaxLen);
//      *(int*)(buf_+kFileDataLenBeg) = file_data_len_;
//      //memcpy(file_data_, buf+kFileDataBeg, file_data_len_);
//      //strncpy(buf_+kFileDataBeg, file_data_, file_data_len_);
//      len = file_data_len_ + kFileDataBeg;
//      break;
//  }
//  return  len;
//}


FileSendControl::FileSendControl (std::string group_ip, int port) :
                                  group_ip_(group_ip), port_(port),
                                  ip_used_(1000, false), running_(false),
                                  con(group_ip, port) {
  //*(FileSendControl::Type*)alive_msg_ = FileSendControl::kAlive;
  auto ip_net = inet_addr(group_ip_.c_str()); 
  auto ip_local = ntohl(ip_net);
  if (ip_local < kMulticastIpMin || ip_local > kMulticastIpMax) {
    std::cout << "通知多播ip : " << ip_local << std::endl;
    std::cout << "限制ip范围在224.0.2.10 ~ 224.0.2.255之间" << std::endl;
    exit(1);
  }
  ip_used_[ip_local-kMulticastIpMin] = true;
}

FileSendControl::~FileSendControl () { }


/* 通知线程， 每500ms发送一次所有正在发送的文件的信息 */
static void NoticeCallback() {
  auto time_clock = std::chrono::system_clock::now();
  auto ctl = FileSendControl::GetInstances();
  while (true) {
    time_clock += std::chrono::milliseconds(500);
    ctl->SendNoticeToClient();
    std::this_thread::sleep_until(time_clock);
  }
}

/* 开始监听和发送文件, 只能调用一次, 用于运行伴随对象启动的线程 */
void FileSendControl::Run() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (running_) return;
  std::thread local_t(ListenFileRecvCallback, std::ref(con));
  listen_file_recv_.swap(local_t);
  running_ = true;
  //定时500毫秒发送一次所有正在发送的文件的信息
  std::thread send_notices_to_client_thread(NoticeCallback);
  send_notices_to_client_thread.detach();
}

/* 发送文件， 传入的参数是一个文件地址， 该文件必须时存在的
 * 会在压缩到当前文件夹下， 然后再启动一个线程去发送该文件
 */
void FileSendControl::SendFile(std::string file_path) {
  //压缩后的文件名， 带.zip, 路径为该程序运行时路径
  std::string file_name = Zip(file_path);
  //生成一个uuid
  auto file_uuid = boost::uuids::random_generator()();
  auto file = std::make_unique<File>(file_name, file_uuid);
  auto file_notice = std::make_unique<FileNotce>();
  //char *const buf = file_notice->buf_;
  //找到一个可用的ip
  uint32_t ip_local = kMulticastIpMin;
  { std::lock_guard<std::mutex> lock(mutex_);
    while(ip_used_[ip_local-kMulticastIpMin] && ip_local <= kMulticastIpMax) ++ip_local;
    if (ip_local >= kMulticastIpMax) {
#if DEBUG
      std::cout << "文件过多" << std::endl;
#endif
      /* TODO: 文件发送过快时将任务加入队列 <12-08-19, 王彬> */
      return ;
    }
    std::cout << ip_local-kMulticastIpMin << std::endl;
    ip_used_[ip_local-kMulticastIpMin] = true;
  }
  //通知前端时附带的信息
  std::vector<std::string> msg;
  //获取文件名, 带.zip后缀
  file_name = file->File_name();
  //获取到的文件名带.zip, 去掉该后缀名
  msg.push_back(file_name.substr(0, file_name.find_last_of('.')));
  NoticeFront(file_uuid, FileSendControl::kNewSendFile, msg);
  int port = ip_local - kMulticastIpMin;
  if (port % 2) {
    port += 10000;
  } else {
    port += 20000;
  }
  //将要发送的数据按照协议格式写入缓冲区中
  file_name = file->File_name();
  int file_len = file->File_len();
  Proto proto;
  //*(FileSendControl::Type*)(buf+FileSendControl::kTypeBeg) = FileSendControl::kNewFile;
  proto.set_type(Proto::kNewFile);
  //*(uint32_t*)(buf+FileSendControl::kGroupIPBeg) = ip_local;
  proto.set_group_ip(ip_local);
  //*(int*)(buf+FileSendControl::kPortBeg) = port;
  proto.set_port(port);
  //*(int*)(buf+FileSendControl::kFileLenBeg) = file_len;
  proto.set_file_len(file_len);
  //*(boost::uuids::uuid*)(buf+FileSendControl::kFileUUIDBeg) = file->UUID();
  proto.set_uuid(file->UUID());
  //*(int*)(buf+FileSendControl::kFileNameLenBeg) = (int)(file_name.size());
  //strncpy(buf+FileSendControl::kFileNameBeg, file_name.c_str(), File::kFileNameMaxLen);
  proto.set_file_name(file_name);
  //设置正在发送文件的信息
  //file_notice->len_ = FileSendControl::kFileNameBeg+file_name.size();
  //proto.buf(Proto::kNewFile, file_notice->buf_, file_notice->len_);
  memcpy(file_notice->buf_, proto.buf(), proto.get_send_len());
  file_notice->len_ = proto.get_send_len();
  file_notice->uuid_ = proto.uuid();
  std::cout << "filenoto " <<  (*(Type*)(file_notice->buf_) == Proto::kNewFile) << std::endl;
  { std::lock_guard<std::mutex> lock(mutex_);
    file_is_sending_.push_back(std::move(file_notice));
  }
  //发送之前主动通知一次, 用于使接收端第一时间可以加入发送的组播地址，
  //避免前面几个数据包需要重传
  std::cout << "new file ip is " << proto.group_ip() << std::endl;

  SendNoticeToClient();
  //启动文件发送的线程，开始发送文件
  std::thread th(FileSendCallback, ip_local, port, std::move(file));
  th.detach();
}

/* 文件发送结束， 处理相关的资源和通知
 */
void FileSendControl::Sendend(std::unique_ptr<File> file, uint32_t group_ip_local) {
  std::lock_guard<std::mutex> lock(mutex_);
  //释放占用的组播地址
  ip_used_[group_ip_local-kMulticastIpMin] = false;
  auto ctl = GetInstances();
  std::string file_name = file->File_name();
  file_name = file_name.substr(0, file_name.rfind('.'));
  //file_name = file_name.substr(0, file_name.rfind('.'));
  //修改该文件的发送状态， 通知线程会停止发送该文件的发送通知
  for (auto it = file_is_sending_.begin(); it != file_is_sending_.end(); ++it) {
    if ((*it)->uuid_ == file->UUID()) {
      it->release();
      file_is_sending_.erase(it);
      break;
    }
  }
  ctl->NoticeFront(file->UUID(), Type::kSendend);
  //删除产生的临时压缩文件
  std::string cmd = "rm -f ";
  cmd += file->File_name();
  system(cmd.c_str());
  //std::cout << "delete " << file->File_name() << std::endl;
  file.release();
}

void FileSendControl::Recvend(std::unique_ptr<File> file) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::string file_name = Unzip(file->File_name(), "./");
  //删除已经存在的文件
  std::string cmd = "rm -f ";
  cmd += file->File_name();
  system(cmd.c_str());
  if (file) {
    auto ctl = FileSendControl::GetInstances();
    ctl->NoticeFront(file->UUID(), FileSendControl::kRecvend);
  } else {
    std::cout << "file_uptr 不可用" << std::endl;
  }
  for (auto it = file_is_recving_.begin(); it != file_is_recving_.end(); ++it) {
    if ((*it)->uuid_ == file->UUID()) {
      (*it)->end_ = true;
      break;
    }
  }
}

/* 检查文件的是否时接收状态
 * 同时会把该文件的维护时间状态刷新
 * 文件超过10秒中没有刷新会删除该记录
 * 警告： 不可频繁刷新或不停获取状态， 否则会导致该记录无法删除造成资源泄露
 * 目前只在接收到新文件通知时调用该方法
 */
bool FileSendControl::FileIsRecving(boost::uuids::uuid file_uuid) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto it = file_is_recving_.begin(); it != file_is_recving_.end(); ++it) {
    if ((*it)->uuid_ == file_uuid) {
      (*it)->clock_ = std::chrono::system_clock::now();
      return true;
    }
  }
  return false;
}

/* 发送新的文件， file必须时已经打开的File */
void FileSendControl::FileSendCallback(uint32_t group_ip_local, int port_local, std::unique_ptr<File> file) {
  ulong ip_net_u = htonl(group_ip_local);
  in_addr ip_net;
  memcpy(&ip_net, &ip_net_u, 4);
  std::string ip(inet_ntoa(ip_net));
  std::cout << "发送文件 ： ip is " << ip << " port : " << port_local << std::endl;
  FileSend(ip, port_local, file);
  auto ctl = FileSendControl::GetInstances();
  ctl->Sendend(std::move(file), group_ip_local);
}

void FileSendControl::RecvFile(std::string group_ip, int port, std::unique_ptr<File> file_uptr) {
  FileRecv(group_ip, port, file_uptr);
  auto ctl = GetInstances();
  ctl->Recvend(std::move(file_uptr));

}

/* 接听接收文件的线程回调函数，干线程伴随程序一直运行
 * 传入一个连接器， 接收到新文件的通知时， 自动生成一个uuid标识该文件
 * 启动一个线程接收该文件， 并通知前端
 * 当接收到同一个文件请求时，忽略该文件请求
 * 一个见的记录超过10秒中没有刷新， 则认为该发送端已停止发送， 删除该记录
 */
void FileSendControl::ListenFileRecvCallback(Connecter& con) {
  char buf[kBufSize];
  boost::uuids::uuid file_uuid;
  Proto proto;
  while (true) {
    memset(buf, 0, kBufSize);
    //3秒中超时等待， 如果没有数据什么也不做
    //可设置为-1 阻塞等待， 为了留下退出运行的接口,设为可超时
    int cnt = con.Recv(proto.buf(), BUFSIZ, 3000);
    if (cnt == -1) {
      continue;
    } else {
      /*: 解析组播地址和文件名 <24-07-19, 王彬> */
      //std::cout << "Recv " << cnt << " 字节" << std::endl;
      Proto::Type type = proto.type();
      if (type != Proto::kNewFile) {
        std::cout << "type != kNewFile" << std::endl;
      }
      //按照协议格式读取数据
      //uint32_t ip_local = *(uint32_t*)(buf+kGroupIPBeg);
      uint32_t ip_local = proto.group_ip();
      //int port_recv = *(int*)(buf+kPortBeg);
      int port_recv = proto.port();
      //int filename_len = *(int*)(buf+FileSendControl::kFileNameLenBeg);
      //int filename_len = *(int*)(buf+FileSendControl::kFileNameLenBeg);
      //file_uuid = *(boost::uuids::uuid*)(buf+FileSendControl::kFileUUIDBeg);
      file_uuid = proto.uuid();
      //int file_len = *(int*)(buf+FileSendControl::kFileLenBeg);
      int file_len = proto.file_len();
      //char file_name[File::kFileNameMaxLen];
      //strncpy(file_name, buf+FileSendControl::kFileNameBeg, File::kFileNameMaxLen);
      //file_name[filename_len] = 0;

      std::string file_name = proto.file_name();
      /*: 判断是否已经传输 <30-07-19, 王彬> */
      auto conse = FileSendControl::GetInstances();
      if (conse->FileIsRecving(file_uuid)) {   //文件已经接收
        continue;
      }
      //通知前端
      std::string file_name_front(file_name);
      file_name_front = file_name_front.substr(0, file_name_front.rfind('.'));
      file_name_front = file_name_front.substr(0, file_name_front.rfind('.'));
      auto ctl = FileSendControl::GetInstances();
      std::vector<std::string> msg;
      msg.push_back(file_name_front);
      ctl->NoticeFront(file_uuid, FileSendControl::kNewFile, msg);
      auto file = std::make_unique<File> (file_name, file_uuid, file_len, true);
      //保存文件信息
      auto file_notice = std::make_unique<FileNotce>();
      file_notice->clock_ = std::chrono::system_clock::now();
      file_notice->uuid_ = file_uuid;
      file_notice->file_name_ = file->File_name();
      file_notice->end_ = false;
      ctl->AddRecvingFile(std::move(file_notice));
      //转地址
      uint32_t ip_net = htonl(ip_local);
      std::cout << "new file group ip is" << ip_local << std::endl;
      in_addr ip_addr;
      memcpy(&ip_addr, &ip_net, sizeof(in_addr));
      std::string ip(inet_ntoa(ip_addr));
      //开启一个新线程接收文件
      std::thread file_recv_thread(RecvFile, ip, port_recv, std::move(file));
      file_recv_thread.detach();
    }
  }
}

/* 向通知组发送自己所有正在发送的文件的信息
 * 检查所有已失效的uuid， 并删除
 */
void FileSendControl::SendNoticeToClient() {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto& i : file_is_sending_) {
    //std::cout << "notice client " << *(Type*)(i->buf_) << std::endl;
    //std::cout << i->len_ << std::endl;
    con.Send(i->buf_, i->len_);
  }
  for (auto it = file_is_recving_.begin(); it != file_is_recving_.end(); ++it) {
    if (std::chrono::system_clock::now() - (*it)->clock_ > std::chrono::seconds(10)) {
      it->release();
      it = file_is_recving_.erase(it);
    }
  }
}

//调用通知前端的回调函数
void FileSendControl::NoticeFront(const boost::uuids::uuid file_uuid,
                                  const FileSendControl::Type type,
                                  std::vector<std::string> msg) {
    NoticeFront_(file_uuid, type, msg);
}
