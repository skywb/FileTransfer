#include "util/Connecter.h"
#include <unistd.h>
#include <iostream>

int main(int argc, char *argv[])
{
  Connecter con("224.0.2.11", 8888);
  char buf[1000];
  while (true) {
    int re = con.Recv(buf, 1000, 3000);
    if (re == -1) {
      std::cout << "没有数据" << std::endl;
    } else {
      std::cout << buf << std::endl;
    }
  }
  return 0;
}
