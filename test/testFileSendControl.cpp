#include "send/FileSendControl.h"


int main(int argc, char *argv[])
{
  auto conse = FileSendControl::GetInstances();
  conse->Run();
  while (true) {
    std::cout << "发送文件 1 <FilePath>" << std::endl;
    std::cout << "继续 2 , 退出 3" << std::endl;
    int cmd = 0;
    std::cin >> cmd;
    if (cmd == 1) {
      std::string filepath;
      std::cin >> filepath;
      conse->SendFile(filepath);
    }
    while (true) {
      auto filename = conse->GetEndFileName();
      if (filename.size()) {
        std::cout << filename << " 传输完毕" << std::endl;
      } else {
        break;
      }
    }
    if (cmd == 3) {
      return 0;
    }
  }


  return 0;
}
