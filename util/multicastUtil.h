#ifndef MULTICASTUTIL_H_YWTTF8QR
#define MULTICASTUTIL_H_YWTTF8QR

#include <netinet/ip.h>
#include <iostream>
#include <vector>
#include <string>

bool Bind(sockaddr_in* addr, int* sockfd, std::string ip, int port);

bool JoinGroup (int* sockfd, std::string group_ip, std::string netcard_ip);

std::vector<std::string> GetAllNetIP();

#endif /* end of include guard: MULTICASTUTIL_H_YWTTF8QR */
