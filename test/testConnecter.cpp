#include "util/Connecter.h"
#include <send/File.h>
#include <send/fileSend.h>
#include <unistd.h>
#include <iostream>
#include <thread>

void ListenMsg(Connecter& con) {
  char buf[kBufSize];
  while (true) {
    if (-1 == con.Recv(buf, kBufSize, 3000)) continue;
    std::cout << buf << std::endl;
  }
}

int main(int argc, char *argv[])
{
  Connecter con("224.0.2.11", 8888);
  std::thread listen(ListenMsg, std::ref(con));
  while (true) {
    std::cout << "发送数据" << std::endl;
    con.Send("hello", 5);
    sleep(1);
  }
  return 0;
}
