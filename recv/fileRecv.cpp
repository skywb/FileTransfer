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


/* 请求重发包号为package_num的数据包的 请求信息写入缓冲buf
 * buf为缓冲区，该方法只提供写入缓冲，不提供发送功能
 * 将要发送的信息写入缓冲中
 */
//static int RequestResendSetbuf(int package_num, char* buf) {
//    std::cout << "request " << package_num << std::endl;
//    *(FileSendControl::Type*)buf = FileSendControl::Type::kReSend;
//    *(int*)(buf+sizeof(FileSendControl::Type)) = package_num;
//    return sizeof (FileSendControl::Type) + sizeof (package_num);
//}

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
  //检查的包序号
  int recv_max_pack_num = 1, check_package_num = 1;
  Proto proto;
  for (int i = 0; ; ++i) {
    recv_len = con.Recv(proto.buf(), kBufSize, 500);
    //模拟丢包
    //if (Abandon(30)) {
    //    std::cout << "主动丢包" << std::endl;
    //    continue;
    //}
    if (recv_len > 0) {  //数据到来
      //buf[recv_len] = 0;
      //int pack_num = *(int*)(buf+kPackNumberBeg);
      //proto.Analysis();
      int pack_num = proto.package_numbuer();
      if (pack_num == 0) {
        continue;
      }
      recv_max_pack_num = std::max(pack_num, recv_max_pack_num);
      //int file_name_len = *(int*)(buf+kFileNameLenBeg);
      //char file_name[File::kFileNameMaxLen+10];
      /* TODO: 检查文件长度， 防止非法长度造成错误 <22-07-19, 王彬> */
      //strncpy(file_name, buf+kFileNameBeg, file_name_len);
      //TODO:校验文件名
      //int data_len = *(int*)(buf+kFileDataLenBeg);
      //file_uptr->Write(pack_num, buf+kFileDataBeg, data_len);
      int data_len = proto.file_data_len();
      file_uptr->Write(pack_num, proto.get_file_data_buf_ptr(), proto.file_data_len());
      /*: 检查之前的包是否到达 <22-07-19, 王彬> */
      while (check_package_num <= file_uptr->File_max_packages()
          && file_uptr->Check_at_package_number(check_package_num)) {
        ++check_package_num;
      }
      if(recv_max_pack_num - check_package_num > 5) { //请求重发
          recv_max_pack_num = std::min(recv_max_pack_num, file_uptr->File_max_packages());
          for (int i=check_package_num; i<= recv_max_pack_num; ++i) {
              if (!file_uptr->Check_at_package_number(check_package_num)) {
                Proto request;
                request.set_type(Proto::kReSend);
                request.set_package_number(check_package_num);
                std::cout << "request " << request.package_numbuer() << std::endl;
                con.Send(request.buf(), request.get_send_len());
              }
          }
      }
    } else {    //没有数据， 可能已经发送完成
        for (int i=check_package_num; i<= file_uptr->File_max_packages(); ++i) {
            if (!file_uptr->Check_at_package_number(check_package_num)) {
                Proto request;
                request.set_type(Proto::kReSend);
                request.set_package_number(check_package_num);
                con.Send(request.buf(), request.get_send_len());
            }
        }
    }
    if (check_package_num > file_uptr->File_max_packages()) {   //数据可能已经全部到达， 检查是否已经全部到达
      check_package_num = 0;
      while (check_package_num <= file_uptr->File_max_packages() && file_uptr->Check_at_package_number(check_package_num))
        ++check_package_num;
      if (check_package_num > file_uptr->File_max_packages()) {
          std::cout << "check end" << std::endl;
          break;
      } else {
          for (int i=check_package_num; i<= file_uptr->File_max_packages(); ++i) {
              if (!file_uptr->Check_at_package_number(check_package_num)) {
                //int len = RequestResendSetbuf(check_package_num, buf);
                Proto request;
                request.set_type(Proto::kReSend);
                request.set_package_number(check_package_num);
                con.Send(request.buf(), request.get_send_len());
              }
          }
      }
    }
  }
  return true;
}

