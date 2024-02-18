#ifndef _MEDIA_PLAYER_UTILS_H_
#define _MEDIA_PLAYER_UTILS_H_

#include <cstdio>
#include <cstdbool>
#include <string>

// 获取工作路径名称
std::string media_utils_get_work_path();

// 获取文件名
std::string media_utils_get_file_name(const std::string & path);

// 宽字符转换为char
std::string media_utils_wchar_to_utf8(const wchar_t * str);

#endif
