#include "MediaPlayerRecord.h"
#include "log.h"

#include <io.h>
#include <windows.h>
#include <algorithm>

CMediaPlayerRecord::CMediaPlayerRecord(const std::string & path) :
    _record_path(path)
{
    if (!loadRecord())
    {
        log_msg_warn("Load record file %s failed!", path.c_str());
    }
}

CMediaPlayerRecord::~CMediaPlayerRecord()
{
    if (!saveRecordToFile())
    {
        log_msg_warn("Save record to file %s failed!", _record_path.c_str());
    }
    unloadRecord();
    _record_path.clear();
}

bool CMediaPlayerRecord::addRecord(const std::string & record)
{
    if (record.empty())
    {
        log_msg_warn("Input record is empty in file %s.", _record_path.c_str());
        return false;
    }

    if (_lst.end() == std::find(_lst.begin(), _lst.end(), record))
    {
        _lst.emplace_back(record);
    }

    return true;
}

bool CMediaPlayerRecord::delRecord(const std::string & record)
{
    if (record.empty())
    {
        log_msg_warn("Input record is empty in file %s.", _record_path.c_str());
        return false;
    }

    auto iter = std::find(_lst.begin(), _lst.end(), record);
    if (_lst.end() != iter)
    {
        _lst.erase(std::remove(_lst.begin(), _lst.end(), record), _lst.end());
    }

    return true;
}

bool CMediaPlayerRecord::modRecord(const std::string & src, const std::string & dst)
{
    if (src.empty() || dst.empty())
    {
        log_msg_warn("Input record is empty in file %s.", _record_path.c_str());
        return false;
    }

    auto iter = std::find(_lst.begin(), _lst.end(), src);
    if (_lst.end() != iter)
    {
        *iter = dst;
    }

    return true;
}

bool CMediaPlayerRecord::getRecord(std::list<std::string> & records)
{
    records.clear();

    for (const auto & record : _lst)
    {
        records.emplace_back(record);
    }

    return true;
}

bool CMediaPlayerRecord::loadRecord()
{
    if (_record_path.empty())
    {
        log_msg_warn("Record file is empty!");
        return false;
    }

    if (!_access(_record_path.c_str(), 0))
    {
        return true;
    }

    //_cnt = GetPrivateProfileInt("TotalRecords", "cnt", 0, _record_path.c_str());
    //for (int i = 0; i < _cnt; ++i)
    //{
    //    char buff[16] = { 0 };
    //    char path[MAX_PATH] = { 0 };
    //    snprintf(buff, 16, "Record%d", i + 1);
    //    GetPrivateProfileString(buff, "path", "", path, MAX_PATH, _record_path.c_str());
    //    _lst.emplace_back(path);
    //}

    return true;
}

void CMediaPlayerRecord::unloadRecord()
{
    _lst.clear();
    _cnt = 0;
}

bool CMediaPlayerRecord::saveRecordToFile()
{
    return true;
}
