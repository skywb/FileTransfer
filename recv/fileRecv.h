#ifndef FILERECV_H_9Y46TCUG
#define FILERECV_H_9Y46TCUG

#include "send/fileSend.h"

#include <memory>

/* 加入指定的组播地址和端口，并接收一个文件， 其中file_uptr必须为打开的可用的状态
 * 数据会自动写入文件， 但是不会关闭file_uptr
 * 默认返回true， 出错返回false
 */
bool FileRecv(std::string group_ip, int port, std::unique_ptr<File>& file_uptr);

#endif /* end of include guard: FILERECV_H_9Y46TCUG */
