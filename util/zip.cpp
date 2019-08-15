#include "zip.h"
#include <sys/stat.h>
#include <unistd.h>
#include <zip.h>
#include <cstring>
#include <fstream>
#include <dirent.h>

static bool is_dir(const std::string& dir)
{
  struct stat st;
  ::stat(dir.c_str(), &st);
  return S_ISDIR(st.st_mode);
}

static void walk_directory(const std::string& startdir, const std::string& inputdir, zip_t *zipper) {
  DIR *dp = ::opendir(inputdir.c_str());
  if (dp == nullptr) {
    throw std::runtime_error("Failed to open input directory: " + inputdir + " : " + std::string(::strerror(errno)));
  }

  struct dirent *dirp;
  while ((dirp = readdir(dp)) != NULL) {
    if (dirp->d_name != std::string(".") && dirp->d_name != std::string("..")) {
      std::string fullname = inputdir + "/" + dirp->d_name;
      if (is_dir(fullname)) {
        if (zip_dir_add(zipper, fullname.substr(startdir.length() + 1).c_str(), ZIP_FL_ENC_UTF_8) < 0) {
          throw std::runtime_error("Failed to add directory to zip: " + std::string(zip_strerror(zipper)));
        }
        walk_directory(startdir, fullname, zipper);
      } else {
        zip_source_t *source = zip_source_file(zipper, fullname.c_str(), 0, 0);
        if (source == nullptr) {
          throw std::runtime_error("Failed to add file to zip: " + std::string(zip_strerror(zipper)));
        }
        if (zip_file_add(zipper, fullname.substr(startdir.length() + 1).c_str(), source, ZIP_FL_ENC_UTF_8) < 0) {
          zip_source_free(source);
          throw std::runtime_error("Failed to add file to zip: " + std::string(zip_strerror(zipper)));
        }
      }
    }
  }
  ::closedir(dp);
}

static std::string Zipdir(const std::string& inputdir, const std::string& output_filename)
{
  int errorp;
  zip_t *zipper = zip_open(output_filename.c_str(), ZIP_CREATE | ZIP_EXCL, &errorp);
  if (zipper == nullptr) {
    zip_error_t ziperror;
    zip_error_init_with_code(&ziperror, errorp);
    throw std::runtime_error("Failed to open output file " + output_filename + ": " + zip_error_strerror(&ziperror));
  }
  try {
    walk_directory(inputdir, inputdir, zipper);
  } catch(...) {
    zip_close(zipper);
    throw;
  }

  zip_close(zipper);
  return output_filename;
}

std::string Zip(const std::string& filePath, const std::string& savePath) {
  std::string obj_path = savePath;
  if (obj_path[obj_path.size()-1] != '/') {
    obj_path += "/";
  }
  auto pos = filePath.find_last_of('/');
  if (pos == -1) pos = 0;
  else pos += 1;
  std::string file_name = filePath.substr(pos);
  std::string zipfile_name = file_name + ".zip";
  if (is_dir(filePath)) {
    return Zipdir(filePath, obj_path+zipfile_name);
  }
  int errorp;
  /* TODO: 应该修改文件名保存， 不可直接删除， 可能其他线程正在占用 <15-08-19, 王彬> */
  std::string cmd = "rm -f ";
  cmd += obj_path + zipfile_name;
  zip_t *zipper = zip_open((obj_path + zipfile_name).c_str(), ZIP_CREATE | ZIP_EXCL, &errorp);
  if (zipper == nullptr) {
    zip_error_t ziperror;
    zip_error_init_with_code(&ziperror, errorp);
    throw std::runtime_error("Failed to open output file " + filePath + ": " + zip_error_strerror(&ziperror));
  }
  zip_source_t *source = zip_source_file(zipper, filePath.c_str(), 0, 0);
  if (source == nullptr) {
    throw std::runtime_error("Failed to add file to zip: " + std::string(zip_strerror(zipper)));
  }
  if (zip_file_add(zipper, file_name.c_str(), source, ZIP_FL_ENC_UTF_8) < 0) {
    zip_source_free(source);
    throw std::runtime_error("Failed to add file to zip: " + std::string(zip_strerror(zipper)));
  }
  zip_close(zipper);
  return zipfile_name;
}

std::string Unzip(const std::string& filePath, const std::string& savePath)
{
	int err = 0;
	struct zip *pZip;
	pZip = zip_open(filePath.c_str(), 0, &err);
    if (!pZip) {
        std::cerr << zip_strerror(pZip) << std::endl;
    }
	int fileCount;
  std::string start_path = savePath;
  if (start_path[start_path.size() - 1] != '/')
      start_path += "/";
  std::string file_path;
	fileCount = zip_get_num_files(pZip);
  if (fileCount > 1) {
    std::string cmd = "rm -rf ";
    std::string dirName = filePath.substr(0, filePath.find_last_of('.'));
    cmd += dirName;
    system(cmd.c_str());
    mkdir(dirName.c_str(), 0777);
    if (!start_path.empty())
      start_path = start_path + dirName;
    else 
      start_path = dirName;
  }
	for (unsigned int i = 0; i < fileCount ; i++) {
		struct zip_stat zipStat = {0};
		zip_stat_init(&zipStat);
		zip_stat_index(pZip, i, 0, &zipStat);
    //std::cout << i << "th file name is " << zipStat.name << std::endl;
    if (zipStat.name[strlen(zipStat.name) -1] == '/') {
      std::string cmd = "rm -rf ";
      if (!start_path.empty())
        file_path = start_path + zipStat.name;
      else 
        file_path = zipStat.name;
      cmd += file_path.c_str();
			system(cmd.c_str());
			mkdir(file_path.c_str(), 0777);
			continue;
		}
		struct zip_file *pzipFile = zip_fopen_index(pZip, i, 0);
		char *buf = new char[zipStat.size];
		zip_fread(pzipFile, buf, zipStat.size);
		std::fstream fs;
    if (!start_path.empty()) {
		if (start_path[start_path.length()-1] != '/') start_path += "/";
		file_path = start_path +  zipStat.name;
    }
    else 
      file_path = zipStat.name;
	//std::cout << file_path << std::endl;
		fs.open(file_path.c_str(), std::fstream::binary | std::fstream::out);
		fs.write(buf,zipStat.size);
		fs.close();
	}
  return filePath.substr(0, filePath.find_last_of('.'));
}
