#ifndef MULTICASTUTIL_H_YWTTF8QR
#define MULTICASTUTIL_H_YWTTF8QR

#include <netinet/ip.h>
#include <iostream>
#include <vector>
#include <string>

bool JoinGroup(sockaddr_in* addr, int* sockfd, std::string group_ip, int port);

std::vector<std::string> GetAllNetIP();

#endif /* end of include guard: MULTICASTUTIL_H_YWTTF8QR */
