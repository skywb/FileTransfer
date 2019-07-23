#include "send/fileSend.h"

int main(int argc, char *argv[])
{
  auto file = std::make_unique<File> ("testFile");
  FileSend("224.0.0.11", 8888, file);
	return 0;
}

