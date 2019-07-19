#ifndef CONNECT_H_K6BGX7EA
#define CONNECT_H_K6BGX7EA

#include <netinet/ip.h>

#include <iostream>
#include <string>

int connect(std::string& ip, int& port, sockaddr_in* addr);

#endif /* end of include guard: CONNECT_H_K6BGX7EA */
