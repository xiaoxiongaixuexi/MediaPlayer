#pragma once

#include <cstdio>
#include <cstdint>
#include <cstdbool>
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>

extern "C"{
#include "libavutil/avutil.h"
#include "libavutil/time.h"
#include "libavformat/avformat.h"
}

class CMediaPlayerDemux
{
public:
    CMediaPlayerDemux() = default;
    ~CMediaPlayerDemux() = default;

    // 打开
    bool open(const std::string & url, const std::string & fmt_name, const std::string & extra = "");
    // 关闭
    void close();
    // 获取流个数
    int getSreamsCount();
    // 丢掉缓冲
    bool flush();
    // 设置有效的流索引
    void setStreamIndex(int stream_index);
    // 获取流信息
    AVStream * getStreamInfo(int stream_index);
    // 读包
    bool readPacket(int stream_index, AVPacket * pkt);

protected:
    // 处理网络流
    std::string processStreamUrl(const std::string & url);
    // 为网络流添加demux特有参数
    std::string makeSpecsParam(const std::string & url);

    // 获取扩展参数key和value
    bool parseExtraParams(const std::string & extra);

    // 读包线程
    void readPackeThr();

private:
    // 输入url
    std::string _url;

    // 最近一次时间
    int64_t _latest_ms = 0;
    // 超时 默认10000 单位：毫秒(ms)
    int64_t _timeout_ms = 10000;

    // 上下文
    AVFormatContext * _fmt_ctx = nullptr;
    // 流信息
    std::vector<AVStream *> _streams;
    // 包信息
    std::unordered_map<int, std::queue<AVPacket>> _stream_targets;
};
