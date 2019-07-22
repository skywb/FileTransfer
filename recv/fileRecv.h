#ifndef FILERECV_H_9Y46TCUG
#define FILERECV_H_9Y46TCUG

#include "send/fileSend.h"

#include <memory>

void FileRecv (const int sockfd, const sockaddr_in* addr, std::string save_path = std::string());
void FileRecv (const int sockfd, const sockaddr_in* addr, std::unique_ptr<File> file_uptr);

#endif /* end of include guard: FILERECV_H_9Y46TCUG */
