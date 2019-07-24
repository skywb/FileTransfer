#ifndef MULTICASTUTIL_H_YWTTF8QR
#define MULTICASTUTIL_H_YWTTF8QR

#include <netinet/ip.h>
#include <iostream>

bool JoinGroup(sockaddr_in* addr, int* sockfd, std::string group_ip, int port);

#endif /* end of include guard: MULTICASTUTIL_H_YWTTF8QR */
