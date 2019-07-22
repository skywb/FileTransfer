#ifndef REACTOR_H_SPYJ5BVL
#define REACTOR_H_SPYJ5BVL

#include <sys/epoll.h>
#include <netinet/ip.h>

#include "send/File.h"

#include <memory>
#include <map>
#include <vector>
#include <mutex>

class Reactor
{
public:
	virtual ~Reactor ();
	//void loop();
	void Listen();
	static Reactor* GetInstance() {
		static Reactor* projcet = nullptr;
		if (projcet == nullptr) {
			projcet = new Reactor();
		}
		return projcet;
	}
  std::unique_ptr<std::vector<bool>>
    GetFileLostedPackage(std::string file_name);
  void AddFileLostedRecord(std::string file_name, int package_num);
private:
	Reactor ();
	int epoll_root_;		
	int sockfd_;
	sockaddr_in addr_;
  std::map<std::string, 
    std::unique_ptr<std::vector<bool>>> file_losted_package_;
  std::map<std::string, 
    std::weak_ptr<File>> fileName_map_File_;
  std::mutex lock_file_losted_package_; 
};

#endif /* end of include guard: REACTOR_H_SPYJ5BVL */
