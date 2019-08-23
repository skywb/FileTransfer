#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <util/Connecter.h>
#include <chrono>
#include <cstring> 

int main(int argc, char *argv[]) {
  Connecter con("224.0.2.10", 9999);
  char buf[10] = "hello";
  int i = 0;
  while (-1 != con.Send(buf, 5)) {
    ++i;
    if (i % 1000 == 0) 
      std::cout << i << std::endl;
  }
  std::cout << strerror(errno) << std::endl;
  std::cout << i << std::endl;
  return 0;
}

