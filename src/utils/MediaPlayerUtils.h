#ifndef _MEDIA_PLAYER_UTILS_H_
#define _MEDIA_PLAYER_UTILS_H_

#include <cstdio>
#include <cstdbool>
#include <string>

// 获取工作路径名称
std::string media_utils_get_work_path();

// 获取文件名
std::string media_utils_get_file_name(const std::string & path);

#endif
