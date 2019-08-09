#include "util/zip.h"
#include <cstring>


int main(int argc, char *argv[])
{
  if (strcmp("zip", argv[1]) == 0) {
    Zip(argv[2]);
  } else {
    Unzip(argv[2], "./");
  }
  return 0;
}
