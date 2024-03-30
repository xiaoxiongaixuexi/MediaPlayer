#include "MediaPlayerUtils.h"
#include "log.h"
#include <windows.h>

std::string media_utils_get_work_path()
{
    char buff[MAX_PATH] = { 0 };

    DWORD num = ::GetModuleFileNameA(NULL, buff, MAX_PATH);
    if (0 == num)
    {
        log_msg_error("GetModuleFileNameA failed!");
        return "";
    }
    char drive[_MAX_DRIVE] = { 0 };
    char dir[_MAX_DIR] = { 0 };
    char fname[_MAX_FNAME] = { 0 };
    char ext[_MAX_EXT] = { 0 };
    _splitpath(buff, drive, dir, fname, ext);

    std::string path;
    path.append(drive).append(dir);
    return path;
}

std::string media_utils_get_file_name(const std::string & path)
{
    char drive[_MAX_DRIVE] = { 0 };
    char dir[_MAX_DIR] = { 0 };
    char fname[_MAX_FNAME] = { 0 };
    char ext[_MAX_EXT] = { 0 };
    _splitpath(path.c_str(), drive, dir, fname, ext);

    std::string name;
    name.append(fname).append(ext);
    return name;
}

std::string media_utils_wchar_to_utf8(const wchar_t * str)
{
    std::string res;
    if (nullptr == str)
    {
        log_msg_warn("Input param is nullptr!");
        return res;
    }
    int len = WideCharToMultiByte(CP_UTF8, 0, str, -1, nullptr, 0, nullptr, nullptr);
    char * bytes = new(std::nothrow) char[len + 1];
    if (nullptr == bytes)
    {
        log_msg_error("No enough memory!");
        return res;
    }
    memset(bytes, 0, len + 1);
    WideCharToMultiByte(CP_UTF8, 0, str, -1, bytes, len, nullptr, nullptr);
    res.assign(bytes, len);
    delete[] bytes;
    return res;
}
