#ifndef MULTICASTUTIL_H_YWTTF8QR
#define MULTICASTUTIL_H_YWTTF8QR

#include <netinet/ip.h>
#include <iostream>
#include <vector>
#include <string>

/*
 * 将IP绑定到addr地址结构
 * 新创建一个socket套接字， 绑定addr
 */
bool Bind(sockaddr_in* addr, int* sockfd, std::string ip, int port);

/* 加入组播地址， 可发可读 */
bool JoinGroup (int* sockfd, std::string group_ip, std::string netcard_ip);

//获取所有网卡的上绑定的IP
std::vector<std::string> GetAllNetIP();

#endif /* end of include guard: MULTICASTUTIL_H_YWTTF8QR */
