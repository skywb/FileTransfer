#include "zip.h"
#include <sys/stat.h>
#include <unistd.h>

std::string zip(std::string filePath) {
  //auto const pos = filePath.find_last_of('/');
  //auto file_name = filePath;
  //if (pos != -1) {
  //  file_name = filePath.substr(pos);
  //}
  //struct stat st;
  //stat(filePath.c_str(), &st);
  //if (st.st_mode & __S_IFDIR) { //is a dir
    std::string cmd = "tar -czf ";
    cmd += filePath + ".tar.gz";
    cmd += " ";
    cmd += filePath;
    system(cmd.c_str());
  //}

    return filePath+".tar.gz";
}

bool unzip(std::string filePath, std::string objPath) {
    std::string cmd = "tar -xzf ";
    cmd += filePath;
    cmd += " -C ";
    cmd += objPath;
    system(cmd.c_str());
  //}
  return true;
}
