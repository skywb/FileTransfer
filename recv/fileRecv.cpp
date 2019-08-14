#include "fileRecv.h"
#include "send/fileSend.h"
#include "util/multicastUtil.h"
#include "util/Connecter.h"
#include "util/testutil.h"
#include "send/FileSendControl.h"

#include <cstring>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include <iostream>
#include <vector>
#include <thread>


static void RequeseResendPackage(int package_num, Connecter& con) {
  Proto request;
  request.set_type(Proto::kReSend);
  request.set_package_number(package_num);
  con.Send(request.buf(), request.get_send_len());
}

/* 加入指定的组播地址和端口， 接收一个文件，file_uptr必须为已经打开的状态
 * 创建一个连接到该组播地址的Connecter
 * 接收到数据包之后进行检查数据包，如果该包已经接收过则丢弃，否则写入文件
 * 检查：
 *    当500ms没有数据到来时，可能发送端已经发送完或者断开连接的状态， 则检查
 *    本地文件的接收状态，发送确实的包序号，等待发送端重发
 *TODO: 判断断开连接的状态，重连
 */
bool FileRecv(std::string group_ip, int port, std::unique_ptr<File>& file_uptr) {
  Connecter con(group_ip, port);
  //char buf[kBufSize]; //接收缓冲区
  int recv_len = 0;  //接收的长度
  //上次更新心跳包的时间
  auto time_alive_pre = std::chrono::system_clock::now();
  auto time_pack_pre = time_alive_pre;
  //检查的包序号
  int recv_max_pack_num = 1, check_package_num = 1;
  Proto proto;
  for (int i = 0; ; ++i) {
    recv_len = con.Recv(proto.buf(), kBufSize, 500);
    if (Abandon(30)) {    //模拟丢包
      std::cout << "主动丢包" << std::endl;
      continue;
    }
    if (recv_len > 0) {  //有数据到来
      Proto::Type type;
      type = proto.type();
      //非数据包
      if (type == Proto::kAlive || type == Proto::kReSend) {
        time_alive_pre = std::chrono::system_clock::now();
        //超时没有数据包到达， 可能发送端已经断开连接
        if (time_pack_pre + std::chrono::seconds(3) <= std::chrono::system_clock::now()) {
          break;
        }
        continue;
      } else if (type != Proto::kData) {
        std::cout << "非法type value is " << type  << std::endl;
        //超时没有数据包到达， 可能发送端已经断开连接
        if (time_pack_pre + std::chrono::seconds(3) <= std::chrono::system_clock::now()) {
          break;
        }
        continue;
      }
      //数据包
      int pack_num = proto.package_numbuer();
      if (pack_num == 0) {
        continue;
      }
      recv_max_pack_num = std::max(pack_num, recv_max_pack_num);
      /* TODO: 检查文件长度， 防止非法长度造成错误 <22-07-19, 王彬> */
      file_uptr->Write(pack_num, proto.get_file_data_buf_ptr(), proto.file_data_len());
      if(recv_max_pack_num - check_package_num > 5) { //请求重发
        recv_max_pack_num = std::min(recv_max_pack_num, file_uptr->File_max_packages());
        for (int i=check_package_num; i<= recv_max_pack_num; ++i) {
          if (!file_uptr->Check_at_package_number(check_package_num)) {
            RequeseResendPackage(check_package_num, con);
            time_alive_pre = std::chrono::system_clock::now();
          }
        }
      }
    } else {    //没有数据， 可能已经发送完成 
      if (time_pack_pre + std::chrono::seconds(3) <= std::chrono::system_clock::now()) {
        std::cout << "发送端已断开连接" << std::endl;
        break;
      }
      for (int i=check_package_num; i<= file_uptr->File_max_packages(); ++i) {
        if (!file_uptr->Check_at_package_number(check_package_num)) {
          RequeseResendPackage(check_package_num, con);
          time_alive_pre = std::chrono::system_clock::now();
        }
      }
    }
    if (check_package_num > file_uptr->File_max_packages()) {   //数据可能已经全部到达， 检查是否已经全部到达
      check_package_num = 0;
      while (check_package_num <= file_uptr->File_max_packages() 
          && file_uptr->Check_at_package_number(check_package_num))
        ++check_package_num;
      if (check_package_num > file_uptr->File_max_packages()) {
        std::cout << "check end" << std::endl;
        break;
      } else {
        for (int i=check_package_num; i<= file_uptr->File_max_packages(); ++i) {
          if (!file_uptr->Check_at_package_number(check_package_num)) {
            RequeseResendPackage(check_package_num, con);
            time_alive_pre = std::chrono::system_clock::now();
          }
        }
      }
    }
    //超过500毫秒没有心跳包， 发送一次心跳包
    if (time_alive_pre + std::chrono::milliseconds(500) <= std::chrono::system_clock::now()) {
      Proto alive;
      alive.set_type(Proto::kAlive);
      con.Send(alive.buf(), alive.get_send_len());
      time_alive_pre = std::chrono::system_clock::now();
    }
  }
  //断开连接， 检查是否数据全部到达
  check_package_num = 0;
  while (check_package_num <= file_uptr->File_max_packages() 
      && file_uptr->Check_at_package_number(check_package_num))
    ++check_package_num;
  return check_package_num > file_uptr->File_max_packages();
  //return true;
}

