#include "reactor/Reactor.h"
#include "send/fileSend.h"


#include <thread>

int main(int argc, char *argv[])
{
  std::thread send(FileSend, "223.0.0.11", 8888, "testFile");
	auto reactor = Reactor::GetInstance();
	reactor->Listen();
	return 0;
}
