#pragma once

#include <cstdint>
#include <cstdbool>
#include <string>
#include <vector>

extern "C" {
#include "libavformat/avformat.h"
}

typedef struct _MediaPlayerStream
{
    bool got_first_pkt;
    bool wraped;
    int stream_index;
    int64_t ts_offset;
    int64_t dts;
    int64_t pts;
    int64_t next_dts;
    int64_t next_pts;
    int64_t start_time;
    int64_t wraped_ts;
    AVStream * si;
} MediaPlayerStream;

class CMediaPlayerDemux final
{
public:
    CMediaPlayerDemux() = default;
    ~CMediaPlayerDemux() = default;

    // 打开
    bool open(const std::string & url, const std::string & fmt_name, const std::string & extra = "");
    // 关闭
    void close();
    // 获取文件时长
    int64_t getDuration();
    // 获取流个数
    uint32_t getStreamsCount();
    // 丢掉缓冲
    bool flush();
    // 获取流信息
    AVStream * getStreamInfo(int stream_index);
    // 读包
    bool readPacket(AVPacket * pkt);
    // seek
    bool seek(int stream_index, int64_t timestamp);

protected:
    // 处理网络流
    std::string processStreamUrl(const std::string & url);
    // 为网络流添加demux特有参数
    std::string makeSpecsParam(const std::string & url);

    // 获取扩展参数key和value
    bool parseExtraParams(const std::string & extra);

private:
    bool processPacket(const AVPacket & pkt, MediaPlayerStream & st);

private:
    // 
    bool _ts_discont = false;
    // 输入url
    std::string _url;

    // 最近一次时间
    int64_t _latest_ms = 0;
    // 超时 默认5000 单位：毫秒(ms)
    int64_t _timeout_ms = 5000;
    // 最大ts offset
    int64_t _ts_offset = 0;

    // 流信息集合
    std::vector<MediaPlayerStream> _streams;

    // 上下文
    AVFormatContext * _fmt_ctx = nullptr;
};
