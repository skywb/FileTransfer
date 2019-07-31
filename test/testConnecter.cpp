#include "util/Connecter.h"
#include <unistd.h>
#include <iostream>

int main(int argc, char *argv[])
{
  Connecter con("224.0.2.11", 8888);
  while (true) {
    std::cout << "发送数据" << std::endl;
    con.Send("hello", 5);
    sleep(1);
  }
  return 0;
}
