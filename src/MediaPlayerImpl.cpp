#include "MediaPlayerImpl.h"
#include "os_log.h"
#include "demux/MediaPlayerDemuxFfmpeg.h"

#include "decode/MediaPlayerDecoderImpl.h"
#include "rescaler/VideoRescalerImpl.h"
#include "rescaler/AudioRescalerImpl.h"
#include "render/VideoRenderSDL.h"
#include "render/AudioRenderSDL.h"

bool CMediaPlayerImpl::opened() const
{
    return _is_init;
}

bool CMediaPlayerImpl::open(const char * url)
{
    if (nullptr == url || '\0' == url[0])
    {
        log_msg_warn("Input param is nullptr or empty!");
        return false;
    }

    if (_is_init)
    {
        log_msg_warn("Another file has been opened!");
        return false;
    }

    _demux_ctx = new(std::nothrow) CMediaPlayerDemux();
    if (nullptr == _demux_ctx)
    {
        log_msg_error("Create CMediaPlayerDemux instance failed");
        return false;
    }

    if (!_demux_ctx->open(url, ""))
    {
        log_msg_warn("Open url %s failed", url);
        return false;
    }

    const uint32_t streams_cnt = _demux_ctx->getSreamsCount();
    for (uint32_t i = 0; i < streams_cnt; ++i)
    {
        const auto * av_stream = _demux_ctx->getStreamInfo(i);
        if (AVMEDIA_TYPE_VIDEO == av_stream->codecpar->codec_type)
        {
            _video_index.emplace_back(i);
        }
        else if (AVMEDIA_TYPE_AUDIO == av_stream->codecpar->codec_type)
        {
            _audio_index.emplace_back(i);
        }
    }

    if (!_video_index.empty())
    {
        _video_cur_index = _video_index[0];
        _video_stream = _demux_ctx->getStreamInfo(_video_cur_index.load());
    }

    if (!_audio_index.empty())
    {
        _audio_cur_index = _audio_index[0];
        _audio_stream = _demux_ctx->getStreamInfo(_audio_cur_index.load());
    }

    _duration = _demux_ctx->getDuration();
    _is_init = true;

    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO);

    _packets_thr = std::thread{ &CMediaPlayerImpl::recvPacketsThr,this };
    if (!_video_index.empty())
    {
        _video_thr = std::thread{ &CMediaPlayerImpl::dealVideoPacketsThr,this };
    }
    if (!_audio_index.empty())
    {
        _audio_thr = std::thread{ &CMediaPlayerImpl::dealAudioPacketsThr,this };
    }

    return true;
}

void CMediaPlayerImpl::close()
{
    if (!_is_init)
    {
        log_msg_warn("No opened file!");
        return;
    }

    _is_init = false;

    if (_packets_thr.joinable())
    {
        _packets_cond.notify_one();
        _packets_thr.join();
    }
    if (_video_thr.joinable())
    {
        _video_cond.notify_one();
        _video_thr.join();
    }
    if (_audio_thr.joinable())
    {
        _audio_cond.notify_one();
        _audio_thr.join();
    }

    uninitVideoStream();
    uninitAudioStream();
    destoryVideoPlayer();
    destoryAudioPlayer();

    if (nullptr != _demux_ctx)
    {
        _demux_ctx->close();
        delete _demux_ctx;
        _demux_ctx = nullptr;
    }

    SDL_Quit();

    _audio_index.clear();
    _video_index.clear();
    _is_playing = false;
    _cur_pos = 0;
    _audio_clock = 0.0;
}

int64_t CMediaPlayerImpl::getDuration()
{
    if (nullptr == _demux_ctx)
    {
        log_msg_warn("file is not open!");
        return -1;
    }

    return _duration;
}

int64_t CMediaPlayerImpl::getPosition()
{
    int64_t position = static_cast<int64_t>(ceil(_audio_clock));
    if (position > _duration)
        position = _duration;
    return position;
}

bool CMediaPlayerImpl::setPosition(int pos)
{
    if (nullptr == _demux_ctx)
    {
        log_msg_warn("No opened faile!");
        return false;
    }

    _cur_pos = pos;
    _is_skip = true;

    return true;
}

bool CMediaPlayerImpl::start(const void * wnd, const int width, const int height)
{
    if (nullptr == wnd)
    {
        log_msg_warn("Input param is nullptr!");
        return false;
    }

    if (!_is_init)
    {
        log_msg_warn("The file isn't opened!");
        return false;
    }

    if (_is_pause)
    {
        _is_pause = false;
        _is_playing = true;
        SDL_PauseAudio(0);
        _packets_cond.notify_one();
        return true;
    }

    _wnd_width = width;
    _wnd_height = height;

    if (!initVideoStream())
    {
        log_msg_warn("initVideoStream failed.");
        return false;
    }

    if (!initAudioStream())
    {
        log_msg_warn("initAudioStream failed.");
        goto ERR;
    }

    // 创建视频播放器
    if (!createVideoPlayer(wnd, width, height))
    {
        log_msg_warn("createVideoPlayer");
        goto ERR;
    }

    if (!createAudioPlayer())
    {
        log_msg_warn("create audio player failed!");
        goto ERR;
    }

    _is_playing = true;
    // 唤醒读包线程
    _packets_cond.notify_one();

    return true;

ERR:
    uninitVideoStream();
    uninitAudioStream();
    destoryVideoPlayer();
    destoryAudioPlayer();

    return false;
}

bool CMediaPlayerImpl::pause()
{
    _is_pause = true;
    _is_playing = false;
    return true;
}

bool CMediaPlayerImpl::forward()
{
    if (_speed < MEDIA_PLAYER_SPEED_QUADRUPLE)
    {
        int speed = static_cast<int>(_speed) + 1;
        _speed = static_cast<MEDIA_PLAYER_SPEED>(speed);
    }

    return true;
}

bool CMediaPlayerImpl::backward()
{
    if (_speed > MEDIA_PLAYER_SPEED_QUARTER)
    {
        int speed = static_cast<int>(_speed) - 1;
        _speed = static_cast<MEDIA_PLAYER_SPEED>(speed);
    }

    return true;
}

bool CMediaPlayerImpl::setVolume(const int volume)
{
    if (nullptr == _demux_ctx || nullptr == _audio_stream)
    {
        log_msg_warn("There is no opened file ...");
        return false;
    }

    if (nullptr == _audio_render)
        return false;

    if (volume <= 0)
        _audio_render->setVolume(0);
    else if (volume >= 128)
        _audio_render->setVolume(128);
    else
        _audio_render->setVolume(volume);

    return true;
}

bool CMediaPlayerImpl::initVideoStream()
{
    if (_video_index.empty())
    {
        log_msg_info("No video stream");
        return true;
    }

    if (nullptr == _video_stream)
    {
        log_msg_warn("Video stream is nullptr.");
        return false;
    }

    _video_decoder = new(std::nothrow) CAVDecoderImpl();
    if (nullptr == _video_decoder)
    {
        log_msg_error("new(std::nothrow) CAVDecoderImpl failed.");
        return false;
    }

    // 创建视频解码器
    const auto * codecpar = _video_stream->codecpar;
    if (!_video_decoder->create(codecpar))
    {
        log_msg_warn("Create video decoder failed.");
        uninitVideoStream();
        return false;
    }

    _pix_fmt = static_cast<AVPixelFormat>(codecpar->format);
    _frm_width = codecpar->width;
    _frm_height = codecpar->height;

    // 创建视频转换器
    _video_rescaler = new(std::nothrow) CVideoRescalerImpl();
    if (nullptr == _video_rescaler)
    {
        log_msg_error("new (std::nothrow)CMediaPlayerRescaler failed!");
        uninitVideoStream();
        return false;
    }
    if (!_video_rescaler->create(_pix_fmt, _frm_width, _frm_height, AV_PIX_FMT_YUV420P, _wnd_width, _wnd_height))
    {
        log_msg_warn("create rescaler failed!");
        uninitVideoStream();
        return false;
    }

    return true;
}

void CMediaPlayerImpl::uninitVideoStream()
{
    if (nullptr != _video_rescaler)
    {
        _video_rescaler->destroy();
        delete _video_rescaler;
        _video_rescaler = nullptr;
    }

    if (nullptr != _video_decoder)
    {
        _video_decoder->destroy();
        delete _video_decoder;
        _video_decoder = nullptr;
    }

    _video_stream = nullptr;
    _video_cur_index = -1;
}

bool CMediaPlayerImpl::initAudioStream()
{
    if (_audio_index.empty())
    {
        log_msg_info("No audio stream.");
        return true;
    }

    if (nullptr == _audio_stream)
    {
        log_msg_warn("Audio stream is nullptr.");
        return false;
    }

    // 创建音频解码器
    _audio_decoder = new(std::nothrow) CAVDecoderImpl();
    if (nullptr == _audio_decoder)
    {
        log_msg_warn("create audio deocder failed!");
        return false;
    }
    const auto * codecpar = _audio_stream->codecpar;
    if (!_audio_decoder->create(codecpar))
    {
        log_msg_warn("Create audio decoder failed.");
        uninitAudioStream();
        return false;
    }

    // 创建音频转换器
    _audio_rescaler = new(std::nothrow) CAudioRescalerImpl();
    if (nullptr == _audio_rescaler)
    {
        log_msg_warn("new(std::nothrow) CAudioRescalerImpl failed!");
        uninitAudioStream();
        return false;
    }

    if (!_audio_rescaler->create(codecpar->channel_layout, AV_SAMPLE_FMT_S16, codecpar->sample_rate, codecpar->channel_layout,
        static_cast<AVSampleFormat>(codecpar->format), codecpar->sample_rate, codecpar->frame_size))
    {
        log_msg_warn("Create audio rescaler failed.");
        uninitAudioStream();
        return false;
    }

    return true;
}

void CMediaPlayerImpl::uninitAudioStream()
{
    if (nullptr != _audio_rescaler)
    {
        _audio_rescaler->destroy();
        delete _audio_rescaler;
        _audio_rescaler = nullptr;
    }

    if (nullptr != _audio_decoder)
    {
        _audio_decoder->destroy();
        delete _audio_decoder;
        _audio_decoder = nullptr;
    }

    _audio_stream = nullptr;
    _audio_cur_index = -1;
}

bool CMediaPlayerImpl::createVideoPlayer(const void * wnd, const int width, const int height)
{
    if (_video_index.empty())
    {
        return true;
    }

    _video_render = new(std::nothrow) CVideoRendererSDL();
    if (nullptr == _video_render)
    {
        log_msg_error("new(std::nothrow) CVideoRendererSDL() failed");
        return false;
    }
    if (!_video_render->create(wnd, width, height))
    {
        log_msg_warn("Create video player failed!");
        delete _video_render;
        _video_render = nullptr;
        return false;
    }

    return true;
}

void CMediaPlayerImpl::destoryVideoPlayer()
{
    if (nullptr != _video_render)
    {
        _video_render->destroy();
        delete _video_render;
        _video_render = nullptr;
    }
}

bool CMediaPlayerImpl::createAudioPlayer()
{
    if (_audio_index.empty())
    {
        return true;
    }

    _audio_render = new(std::nothrow) CAudioRendererSDL();
    if (nullptr == _audio_render)
    {
        log_msg_error("Create audio render instance failed!");
        return false;
    }

    const auto & cp = _audio_stream->codecpar;
    return _audio_render->create(cp->sample_rate, cp->channels, cp->frame_size);

    return true;
}

void CMediaPlayerImpl::destoryAudioPlayer()
{
    if (nullptr != _audio_render)
    {
        _audio_render->destroy();
        delete _audio_render;
        _audio_render = nullptr;
    }
}

void CMediaPlayerImpl::recvPacketsThr()
{
    std::unique_lock<std::mutex> lck(_packets_mtx);
    _packets_cond.wait(lck);

    int64_t audio_last_dts = AV_NOPTS_VALUE;
    int64_t video_last_dts = AV_NOPTS_VALUE;

    while (_is_init)
    {
        AVPacket pkt;
        av_init_packet(&pkt);

        if (!_is_playing)
        {
            _packets_cond.wait(lck);
            _video_cond.notify_one();
            _audio_cond.notify_one();
            continue;
        }

        if (_is_skip)
        {
            int64_t ts = static_cast<int64_t>(_cur_pos.load()) * AV_TIME_BASE;
            if (!_demux_ctx->seek(-1, ts, AVSEEK_FLAG_ANY))
            {
                log_msg_warn("seek failed");
                break;
            }
            _video_queue.clear();
            _audio_queue.clear();
            _is_skip = false;
        }

        if (_video_queue.size() > 300 && _audio_queue.size() > 300)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        if (!_demux_ctx->readPacket(&pkt))
        {
            log_msg_warn("read packet failed");
            break;
        }

        if (_video_cur_index == pkt.stream_index && pkt.dts > video_last_dts)
        {
            _video_queue.push(pkt);
            video_last_dts = pkt.dts;
            _video_cond.notify_one();
        }
        else if (_audio_cur_index == pkt.stream_index && pkt.dts > audio_last_dts)
        {
            _audio_queue.push(pkt);
            audio_last_dts = pkt.dts;
            _audio_cond.notify_one();
        }
        else
            av_packet_unref(&pkt);
    }
    _is_over = true;
}

void CMediaPlayerImpl::dealVideoPacketsThr()
{
    std::unique_lock<std::mutex> lck(_video_mtx);
    _video_cond.wait(lck);

    if (nullptr == _video_rescaler)
    {
        log_msg_warn("No available video rescaler!");
        return;
    }

    if (nullptr == _video_render)
    {
        log_msg_warn("No available video player!");
        return;
    }

    while (_is_init)
    {
        // 没在播放中
        if (!_is_playing)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        // 播放视频
        bool is_succ = false;
        if (_is_over && _video_queue.empty())
        {
            is_succ = _video_decoder->send(nullptr);
        }
        else
        {
            AVPacket pkt;
            av_init_packet(&pkt);
            if (!_video_queue.pop(pkt))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            is_succ = _video_decoder->send(&pkt);
            av_packet_unref(&pkt);
        }

        if (!is_succ)
        {
            log_msg_warn("decode video packet failed!");
            break;
        }

        bool got = false;
        AVFrame frm = { };
        while (_video_decoder->recv(&got, &frm) && got)
        {
            got = false;
            AVFrame yuv_frm = { 0 };
            if (!_video_rescaler->rescale(&frm, &yuv_frm))
            {
                log_msg_warn("rescale failed!");
                continue;
            }

            _video_render->setData(yuv_frm.data, yuv_frm.linesize);

            double pts = frm.best_effort_timestamp == AV_NOPTS_VALUE ? 0.0 : frm.best_effort_timestamp;
            pts = static_cast<double>(pts) * av_q2d(_video_stream->time_base);
            double delay = pts - _audio_clock;
            if (delay > 0.0)
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(delay * 1000)));
        }
    }
}

void CMediaPlayerImpl::dealAudioPacketsThr()
{
    std::unique_lock<std::mutex> lck(_audio_mtx);
    _audio_cond.wait(lck);

    while (_is_init)
    {
        // 没在播放中
        if (!_is_playing)
        {
            SDL_PauseAudio(1);
            continue;
        }

        // 播放音频
        bool is_succ = false;
        if (_is_over && _audio_queue.empty())
        {
            is_succ = _audio_decoder->send(nullptr);
        }
        else
        {
            AVPacket pkt;
            av_init_packet(&pkt);
            if (!_audio_queue.pop(pkt) && _is_over)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            is_succ = _audio_decoder->send(&pkt);
            av_packet_unref(&pkt);
        }

        if (!is_succ)
        {
            log_msg_warn("decode audio packet failed!");
            break;
        }

        int out_buff_size = 0;
        uint8_t * out_buff = nullptr;
        AVFrame frm = {};
        bool got = false;
        while (_audio_decoder->recv(&got, &frm) && got)
        {
            if (frm.best_effort_timestamp != AV_NOPTS_VALUE)
                _audio_clock = static_cast<double>(frm.best_effort_timestamp) * av_q2d(_audio_stream->time_base);
            if (_audio_rescaler->rescale(&frm, &out_buff, &out_buff_size))
            {
                _audio_render->fillData(out_buff, out_buff_size);
            }

            while (!_audio_render->finished())
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}
