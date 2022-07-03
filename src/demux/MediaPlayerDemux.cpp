#include "MediaPlayerDemux.h"
#include "os_log.h"

static int avio_interupt_cb(void * opaque)
{
    if (nullptr == opaque)
        return 0;

    auto * demux = static_cast<CMediaPlayerDemux *>(opaque);
    int64_t ts = av_gettime();
    //if (ts > 10000)
    //{
    //    return AVERROR_EXIT;
    //}

    return 0;
}

bool CMediaPlayerDemux::open(const std::string & url, const std::string & fmt_name, const std::string & extra)
{
    if (url.empty())
    {
        log_msg_warn("Input param is empty!");
        return false;
    }

    _fmt_ctx = avformat_alloc_context();
    if (nullptr == _fmt_ctx)
    {
        log_msg_warn("avformat_alloc_context failed!");
        return false;
    }

    AVInputFormat * ifmt = nullptr;
    if (!fmt_name.empty())
    {
        log_msg_info("Input format name is %s.", fmt_name.c_str());
        ifmt = av_find_input_format(fmt_name.c_str());
    }

    _fmt_ctx->interrupt_callback.opaque = this;
    _fmt_ctx->interrupt_callback.callback = avio_interupt_cb;

    AVDictionary * dict = nullptr;

    _latest_ms = av_gettime();
    int ret = avformat_open_input(&_fmt_ctx, url.c_str(), ifmt, &dict);
    if (0 != ret)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        log_msg_warn("avformat_open_input %s failed for %s!", url.c_str(), buff);
        avformat_free_context(_fmt_ctx);
        _fmt_ctx = nullptr;
        return false;
    }

    _latest_ms = av_gettime();
    ret = avformat_find_stream_info(_fmt_ctx, &dict);
    if (0 != ret)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        log_msg_warn("avformat_find_stream_info %s failed for %s!", url.c_str(), buff);
        close();
        return false;
    }

    return true;
}

void CMediaPlayerDemux::close()
{
    if (nullptr != _fmt_ctx)
    {
        avformat_close_input(&_fmt_ctx);
        _fmt_ctx = nullptr;
    }
    _streams.clear();
    _url.clear();
}

int CMediaPlayerDemux::getSreamsCount()
{
    if (nullptr == _fmt_ctx)
    {
        log_msg_warn("No opened ctx!");
        return 0;
    }
    return static_cast<int>(_streams.size());
}

bool CMediaPlayerDemux::flush()
{
    if (nullptr == _fmt_ctx)
    {
        log_msg_warn("No opened ctx!");
        return false;
    }

    int ret = avformat_flush(_fmt_ctx);
    if (ret < 0)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        log_msg_warn("avformat_flush failed for %s!", buff);
        return false;
    }

    return true;
}

void CMediaPlayerDemux::setStreamIndex(const int stream_index)
{
    if (stream_index < 0 || stream_index >= _streams.size())
    {
        log_msg_warn("Stream index %d is out of range!", stream_index);
        return;
    }

    if (_stream_targets.end() == _stream_targets.find(stream_index))
    {
        std::queue<AVPacket> pkts_queue;
        _stream_targets.emplace(stream_index, pkts_queue);
    }
}

AVStream * CMediaPlayerDemux::getStreamInfo(int stream_index)
{
    if (nullptr == _fmt_ctx)
    {
        log_msg_warn("No opened ctx!");
        return nullptr;
    }

    const auto streams_count = static_cast<int>(_streams.size());
    if (stream_index < 0 || stream_index >= streams_count)
    {
        log_msg_warn("Invalid stream index %d in url '%s'!", stream_index, _url.c_str());
        return nullptr;
    }
    return _streams[stream_index];
}

bool CMediaPlayerDemux::readPacket(int stream_index, AVPacket * pkt)
{
    if (nullptr == _fmt_ctx || nullptr == pkt)
    {
        log_msg_warn("No opened ctx or input param is nullptr!");
        return false;
    }

    int ret = av_read_frame(_fmt_ctx, pkt);
    if (0 != ret)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        log_msg_warn("av_read_frame failed for %s in url '%s'!", buff, _url.c_str());
        return true;
    }





    return true;
}

std::string CMediaPlayerDemux::processStreamUrl(const std::string & url)
{
    if (0 == url.compare(0, 7, "rtsp://"))
    {
    }
    else if (0 == url.compare(0, 7, "rtmp://"))
    {
    }
    else if (0 == url.compare(0, 6, "udp://"))
    {
    }
    else if (0 == url.compare(0, 6, "srt://"))
    {
    }

    return _url;
}

std::string CMediaPlayerDemux::makeSpecsParam(const std::string & url)
{
    std::string specs;

    return specs;
}

bool CMediaPlayerDemux::parseExtraParams(const std::string & extra)
{
    if (extra.empty())
    {
        log_msg_warn("Input param is empty!");
        return false;
    }

    char key[64] = { 0 };
    char value[64] = { 0 };
    auto res = sscanf(extra.c_str(), "%s=%s", key, value);
    return (2 == res);
}

void CMediaPlayerDemux::readPackeThr()
{
    while (true)
    {

    }
}
