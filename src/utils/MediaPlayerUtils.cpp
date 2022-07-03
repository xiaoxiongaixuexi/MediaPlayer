#include "MediaPlayerUtils.h"
#include "os_log.h"
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
