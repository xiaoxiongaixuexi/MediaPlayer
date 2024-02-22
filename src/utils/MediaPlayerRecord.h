#pragma once

#include <cstdio>
#include <cstdbool>
#include <string>
#include <list>

class CMediaPlayerRecord final
{
public:
    CMediaPlayerRecord(const std::string& path);
    ~CMediaPlayerRecord();

    // 添加记录
    bool addRecord(const std::string& record);
    // 删除记录
    bool delRecord(const std::string& record);
    // 修改记录
    bool modRecord(const std::string& src, const std::string& dst);
    // 获取记录
    bool getRecord(std::list<std::string>& records);

private:
    // 加载配置文件
    bool loadRecord();
    // 释放
    void unloadRecord();
    // 保存到文件中
    bool saveRecordToFile();

private:
    // 文件路径
    std::string _record_path;

    // 记录个数
    int _cnt = 0;
    // 记录列表
    std::list<std::string> _lst;
};
