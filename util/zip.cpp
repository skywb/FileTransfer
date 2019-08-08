#include "zip.h"
#include <sys/stat.h>
#include <unistd.h>

std::string zip(std::string filePath) {
  struct stat st;
  stat(filePath.c_str(), &st);
  auto pos = filePath.find_last_of('/');
  std::string file_name = filePath.substr(pos+1);
  std::string file_path = filePath.substr(0, pos+1);
  //if (st.st_mode & __S_IFDIR) { //is a dir
    std::string cmd = "tar -C ";
    cmd += file_path + " -czf ";
    cmd += file_name + ".tar.gz ";
    cmd += file_name;
    system(cmd.c_str());
    std::cout << cmd << std::endl;
  //

    return file_name+".tar.gz";
}

std::string unzip(std::string filePath, std::string objPath) {
    std::string cmd = "tar -xzf ";
    cmd += filePath;
    cmd += " -C ";
    cmd += objPath;
    system(cmd.c_str());
    std::cout << cmd << std::endl;
  //}
    std::string file_name = filePath;
    for (int i = 0; i < 2; ++i) {
        auto pos = file_name.find_last_of('.');
        if (pos == -1) break;
        file_name.erase(pos);
    }
  return file_name;
}
