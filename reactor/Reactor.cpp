#include "Reactor.h"
#include "CMD.h"

#include "send/fileSend.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <cstring>
#include <iostream>

Reactor::Reactor() {
	sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
	addr_.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_.sin_family = AF_INET;
	addr_.sin_port = htons(8888);
	std::cout << "ip : " << inet_ntoa(addr_.sin_addr) << " port : " << 8888 << std::endl;
	if (-1 == bind(sockfd_, (sockaddr*)&addr_, sizeof(addr_))) {
		std::cerr << "bind error" << std::endl;
	}
//弃用    ----  TCP
//	epoll_root_ = epoll_create(10);{{{
//	epoll_event event;
//	event.data.fd = sockfd_;
//	event.events = EPOLLIN;
//	if (-1 == epoll_ctl(epoll_root_, EPOLL_CTL_ADD, sockfd_, &event)) {
//		std::cerr << "epoll_ctl error" << std::endl;	
//	}}}}
}

//弃用    ----  TCP
/*{{{
 * 不适用tcp
void Reactor::loop() {
	const int kMaxEvent = 10000;
	epoll_event events[kMaxEvent];
	while(true) {
		std::cout << "wait" << std::endl;
		int n = ::epoll_wait(epoll_root_, events, kMaxEvent, -1);
		for (int i = 0; i < n; ++i) {
			auto& cur = events[i];
			if (cur.data.fd == sockfd_) {
				std::cout << "new connect" << std::endl;
				sockaddr_in addr;
				socklen_t len = sizeof(addr);
				int fd = accept(sockfd_, (sockaddr*)&addr, &len);
				epoll_event new_event;
				new_event.data.fd = fd;
				new_event.events = EPOLLIN;
				if (-1 == epoll_ctl(epoll_root_, EPOLL_CTL_ADD, fd, &new_event)) {
					std::cerr << "epoll_ctl error" << std::endl;	
				}
			} else {
				int fd = cur.data.fd;
				sockaddr_in addr;
				socklen_t len = sizeof(addr);
				const int kBufSize = 10000;
				char buf[kBufSize];
				//封装函数
				recvfrom(fd, buf, kBufSize, 0, (sockaddr*)&addr, &len);
				int cmd = *(int*)buf;
				std::cout << "recv from ip : " << inet_ntoa(addr.sin_addr) << " : " << ntohs(addr.sin_port) << std::endl;
				if (cmd == 1) {
					strcpy(buf, "223.0.0.11");
					sendto(fd, buf, strlen("223.0.0.11"), 0, (sockaddr*)&addr, sizeof(addr));
				} else {
					std::cout << "丢包 序号为 " << *(int*)(buf+2) << std::endl;	
				}
			}
		}
	}
}
*//*}}}*/

Reactor::~Reactor() {
}

void Reactor::Listen() {
	sockaddr_in addr;
	socklen_t len = sizeof(addr);
	const int kBufSize = 1000;
	char buf[kBufSize];
	while (true) {
		int re = recvfrom(sockfd_, buf, kBufSize, 0, (sockaddr*)&addr, &len);
		if (-1 == re) {
			std::cout << strerror(errno) << std::endl;
			exit(1);
		}
		CMD cmd = *(CMD*)buf;
#if DEBUG
    std::cout << "recv from ip : " << inet_ntoa(addr.sin_addr)
      << " : " << ntohs(addr.sin_port) << std::endl;
#endif
		if (cmd == kRequest) {
			strcpy(buf, "223.0.0.11");
			sendto(sockfd_, buf, strlen("223.0.0.11"), 0, (sockaddr*)&addr, sizeof(addr));
		} else if (cmd == kResend) {
      int package_num = *(int*)(buf+4);
      char file_name[kFileNameMaxLen];
      strncpy(file_name, buf+sizeof(CMD)+sizeof(int), kFileNameMaxLen);
      //防止非法文件超过长度导致数组越界错误
      file_name[kFileNameMaxLen-1] = 0;
      //添加一个丢包记录
      AddFileLostedRecord(file_name, package_num); 
#if DEBUG
			std::cout << "丢包 序号为 " << *(int*)(buf+sizeof(CMD)) << std::endl;	
#endif
		} else {
      std::cerr << "cmd 异常" << std::endl;
    }
	}
}

//若还有丢失的包，则返回所有丢失的包序号
//否则返回空
std::unique_ptr<std::vector<bool>> 
Reactor::GetFileLostedPackage(std::string file_name) {
  std::lock_guard<std::mutex> lock(lock_file_losted_package_);
  if (file_losted_package_[file_name]) {
    return std::move(file_losted_package_[file_name]);
  } else {
    file_losted_package_.erase(file_name);
    return nullptr;
  }
}


void Reactor::AddFileLostedRecord(std::string file_name, int package_num) {
  std::lock_guard<std::mutex> lock(lock_file_losted_package_);
  if (file_losted_package_[file_name]) {
    (*file_losted_package_[file_name])[package_num] = true;
  } else {
    //计算需要多少个包 
    int max_pack_num = CalculateMaxPages(file_name);
    file_losted_package_[file_name] = 
      std::make_unique<std::vector<bool>> (max_pack_num);
    (*file_losted_package_[file_name])[package_num] = true;
  }
}
