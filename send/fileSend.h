#ifndef FILESEND_H_HMY0LJA6
#define FILESEND_H_HMY0LJA6

#include <unistd.h>
#include <netinet/ip.h>
#include <string>

const int kBufSize = 30;
const int kTTL = 64;
const int kMaxLength = 1000;
const int kFileNameMaxLen = 100;
bool FileSend(std::string group_id, int port, std::string file_path);
//需要提供一个已经绑定过的地址结构和socket套接字， 以及一个文件
int SendFileMessage(int sockfd, sockaddr_in *addr, std::string file_path);
//需要提供一个已经绑定过的地址结构和socket套接字， 以及一个文件
void SendFileDataAtPackNum(int sockfd, sockaddr_in *addr, std::string filename, int package_numbuer);
//计算需要多少个数据包
int  CalculateMaxPages(std::string file_path);

#endif /* end of include guard: FILESEND_H_HMY0LJA6 */
