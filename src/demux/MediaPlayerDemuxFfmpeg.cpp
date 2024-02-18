#include "MediaPlayerDemuxFfmpeg.h"
#include "os_log.h"

#include <algorithm>

extern "C" {
#include "libavutil/avutil.h"
#include "libavutil/time.h"
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
    _fmt_ctx->interrupt_callback.callback = [](void * opaque) ->int
    {
        //auto * demux = static_cast<CMediaPlayerDemux *>(opaque);
        //if (av_gettime() - demux->_latest_ms > demux->_timeout_ms * 1000)
        //    return AVERROR_EXIT;
        return 0;
    };

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
    ret = avformat_find_stream_info(_fmt_ctx, nullptr);
    if (0 != ret)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        log_msg_warn("avformat_find_stream_info %s failed for %s!", url.c_str(), buff);
        close();
        return false;
    }

    av_dump_format(_fmt_ctx, 0, url.c_str(), 0);

    _url = url;
    _ts_discont = _fmt_ctx->flags & AVFMT_TS_DISCONT;

    for (uint32_t i = 0; i < _fmt_ctx->nb_streams; i++)
    {
        MediaPlayerStream st = {};
        st.stream_index = i;
        st.dts = st.pts = AV_NOPTS_VALUE;
        st.next_dts = st.next_pts = AV_NOPTS_VALUE;
        st.si = _fmt_ctx->streams[i];
        _streams.emplace_back(st);
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
    _url.clear();
}

int64_t CMediaPlayerDemux::getDuration()
{
    if (nullptr != _fmt_ctx)
    {
        return static_cast<int64_t>(_fmt_ctx->duration * av_q2d(av_make_q(1, AV_TIME_BASE)));
    }
    return 0;
}

uint32_t CMediaPlayerDemux::getStreamsCount()
{
    if (nullptr == _fmt_ctx)
    {
        log_msg_warn("No opened ctx!");
        return 0;
    }
    return static_cast<int>(_fmt_ctx->nb_streams);
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

AVStream * CMediaPlayerDemux::getStreamInfo(const int stream_index)
{
    if (nullptr == _fmt_ctx)
    {
        log_msg_warn("No opened ctx!");
        return nullptr;
    }

    if (stream_index < 0 || stream_index >= static_cast<int>(_fmt_ctx->nb_streams))
    {
        log_msg_warn("Invalid stream index %d in url '%s'!", stream_index, _url.c_str());
        return nullptr;
    }

    return _fmt_ctx->streams[stream_index];
}

bool CMediaPlayerDemux::readPacket(AVPacket * pkt)
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
        return false;
    }

    auto & st = _streams[pkt->stream_index];
    const auto codec_type = st.si->codecpar->codec_type;
    if (_ts_discont)
    {
        if ((AVMEDIA_TYPE_VIDEO == codec_type || AVMEDIA_TYPE_AUDIO == codec_type) &&
            AV_NOPTS_VALUE != pkt->dts && AV_NOPTS_VALUE != st.next_dts)
        {
            if (std::abs(pkt->dts + st.ts_offset - st.next_dts) > AV_TIME_BASE * 10 ||
                (pkt->dts + st.ts_offset + st.si->time_base.num / 10 < st.next_dts && pkt->dts <= st.start_time))
            {
                st.ts_offset = st.next_dts - pkt->dts;
                st.wraped = true;
                st.wraped_ts = av_gettime();
                _ts_offset = std::max(_ts_offset, st.ts_offset);
                log_msg_info("SI: %d ts wraped, ts offset is %ld", pkt->stream_index, st.ts_offset);
            }

            bool all_wraped = std::all_of(_streams.begin(), _streams.end(),
                [](const MediaPlayerStream & stream)
                {
                    return stream.wraped;
                });
            if (all_wraped)
            {
                std::for_each(_streams.begin(), _streams.end(),
                    [this](MediaPlayerStream & stream)
                    {
                        stream.wraped = false;
                        stream.ts_offset = 0;
                        stream.ts_offset = _ts_offset;
                    });
            }

            pkt->dts += st.ts_offset;
            if (AV_NOPTS_VALUE != pkt->pts)
                pkt->pts += st.ts_offset;
        }
    }

    processPacket(*pkt, st);

    return true;
}

bool CMediaPlayerDemux::seek(int stream_index, int64_t timestamp)
{
    int ret = av_seek_frame(_fmt_ctx, stream_index, timestamp, AVSEEK_FLAG_BACKWARD);
    if (ret < 0)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        log_msg_warn("av_seek_frame failed for %s in url %s", buff, _url.c_str());
        return false;
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

bool CMediaPlayerDemux::processPacket(const AVPacket & pkt, MediaPlayerStream & st)
{
    const auto * codec = st.si->codec;
    if (!st.got_first_pkt)
    {
        st.dts = st.si->avg_frame_rate.num ? codec->has_b_frames * AV_TIME_BASE / av_q2d(st.si->avg_frame_rate) : 0;
        st.pts = 0;
        if (AV_NOPTS_VALUE != pkt.pts)
        {
            st.dts += av_rescale_q(pkt.pts, st.si->time_base, av_make_q(1, AV_TIME_BASE));
            st.pts = st.dts;
        }
        st.got_first_pkt = true;
    }

    if (AV_NOPTS_VALUE == st.next_dts)
        st.next_dts = st.dts;
    if (AV_NOPTS_VALUE == st.next_pts)
        st.next_pts = st.pts;

    if (AV_NOPTS_VALUE != pkt.dts)
    {
        st.next_dts = st.dts = av_rescale_q(pkt.dts, st.si->time_base, av_make_q(1, AV_TIME_BASE));
        if (AVMEDIA_TYPE_VIDEO != codec->codec_type)
            st.next_pts = st.pts = st.dts;
    }

    switch (codec->codec_type)
    {
    case AVMEDIA_TYPE_VIDEO:
        if (pkt.duration)
            st.next_dts += av_rescale_q(pkt.duration, st.si->time_base, av_make_q(1, AV_TIME_BASE));
        else if (0 != codec->framerate.num * codec->framerate.den)
        {
            const auto * codec_parser = av_stream_get_parser(st.si);
            int ticks = codec_parser ? codec_parser->repeat_pict + 1 : codec->ticks_per_frame;
            st.next_dts = AV_TIME_BASE * codec->framerate.den * ticks / codec->framerate.num / codec->ticks_per_frame;
        }
        break;
    case AVMEDIA_TYPE_AUDIO:
        if (codec->sample_rate)
            st.next_dts += AV_TIME_BASE * codec->frame_size / codec->sample_rate;
        else
            st.next_dts += av_rescale_q(pkt.duration, st.si->time_base, av_make_q(1, AV_TIME_BASE));
        break;
    default:
        break;
    }

    return true;
}
