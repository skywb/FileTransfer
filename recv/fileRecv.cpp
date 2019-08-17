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

static bool RequeseResendPackage(int package_num, Connecter& con) {
  Proto request;
  request.set_type(Proto::kReSend);
  request.set_package_number(package_num);
  //std::cout << "request " << package_num << std::endl;
  return (-1 != con.Send(request.buf(), request.get_send_len()));
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
  //上次更新心跳包的时间
  auto time_alive_pre = std::chrono::system_clock::now();
  auto time_pack_pre = std::chrono::system_clock::now();
  //检查的包序号
  int request_pack_num = 1, check_package_num = 1, max_ack = 0;
  Proto proto;
  while (true) {
    if (-1 != con.Recv(proto.buf(), proto.BufSize())) {
      /*{{{*/
      Proto::Type type = proto.type();
      //非数据包
      if (type == Proto::kAlive || type == Proto::kReSend) {
        time_alive_pre = std::chrono::system_clock::now();
      } else if (type == Proto::kData) {
        int pack_num = proto.package_numbuer();/*{{{*/
        if (pack_num == 0) {
          std::cout << "pack_num = 0" << std::endl;
          continue;
        }
        //数据包到来， 更新上次到来时间
        time_pack_pre = std::chrono::system_clock::now();
        max_ack = std::max(max_ack, proto.ack_package_number());
        //std::cout << "recv package " << pack_num << " len is " << proto.file_data_len() << std::endl;
        file_uptr->Write(pack_num, proto.get_file_data_buf_ptr(), proto.file_data_len());
        while (check_package_num <= file_uptr->File_max_packages() 
            && file_uptr->Check_at_package_number(check_package_num)) {
          ++check_package_num;
        }/*}}}*/
      } else {
        //std::cout << "非法type value is " << type  << std::endl;
      }
      /*}}}*/
    } else {
      do {/*{{{*/
        bool requested = false;
        for (int i=0; i < 300; ) {
          if (request_pack_num > file_uptr->File_max_packages())
            request_pack_num = check_package_num;
          if (!file_uptr->Check_at_package_number(request_pack_num)) {
            if(!RequeseResendPackage(request_pack_num, con)) break;
            requested = true;
            ++i;
          }
        }
        if (requested) {
          time_alive_pre = std::chrono::system_clock::now();
          break;
        } else {
          auto con_type = con.Wait(Connecter::kAll, 500);
          if (con_type & Connecter::kRead) break;
          else if (con_type == Connecter::kOutTime 
              && time_pack_pre + std::chrono::seconds(3) <= std::chrono::system_clock::now()) {
              return false;
          }
        }
      } while (true);/*}}}*/
    }
    auto now = std::chrono::system_clock::now();
    //超时没有心跳包到达
    if (time_pack_pre + std::chrono::milliseconds(500) <= now) {
      break;
    }
    //超时没有数据包到达， 可能发送端已经断开连接
    if (time_pack_pre + std::chrono::seconds(3) <= now) {
      break;
    }
  }
  //std::cout << check_package_num << " " << file_uptr->File_max_packages() << std::endl;
  return check_package_num > file_uptr->File_max_packages();
}

