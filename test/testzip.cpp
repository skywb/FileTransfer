#include "util/zip.h"
#include <cstring>


int main(int argc, char *argv[])
{
  if (strcmp("zip", argv[1]) == 0) {
    Zip(argv[2]);
  } else if(strcmp("zipdir", argv[1]) == 0) {
    std::string filename = std::string(argv[2]) + ".zip";
    Zipdir(argv[2], filename);
  } else {
    Unzip(argv[2], "./");
  }
  return 0;
}
