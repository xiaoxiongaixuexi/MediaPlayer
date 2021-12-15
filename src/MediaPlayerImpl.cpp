#include "MediaPlayerImpl.h"
#include "decoder/MediaPlayerDecoderImpl.h"
#include "rescaler/VideoRescalerImpl.h"
#include "rescaler/AudioRescalerImpl.h"
#include "libos.h"

static void readAudioDataCb(void * udata, uint8_t * data, int len)
{
    auto * audio_info = reinterpret_cast<audio_info_t *>(udata);
    if (nullptr == audio_info || 0 == audio_info->len)
    {
        return;
    }

    SDL_memset(data, 0, len);

    len = (len > audio_info->len) ? audio_info->len : len;
    if (len <= 0)
        return;

    SDL_MixAudio(data, audio_info->pos, len, audio_info->volume);
    audio_info->pos += len;
    audio_info->len -= len;
}

bool CMediaPlayerImpl::open(const char * url)
{
    if (nullptr == url)
    {
        log_msg_warn("Input param is nullptr!");
        return false;
    }

    if (_is_init)
    {
        log_msg_warn("Another file has been opened!");
        return false;
    }

    _fmt_ctx = avformat_alloc_context();
    if (nullptr == _fmt_ctx)
    {
        log_msg_warn("avformat_alloc_context failed!");
        return false;
    }

    int ret = -1;
    do
    {
        ret = avformat_open_input(&_fmt_ctx, url, nullptr, nullptr);
        if (0 != ret)
        {
            char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
            av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
            log_msg_warn("avformat_open_input %s failed, err:%s", url, buff);
            break;
        }

        ret = avformat_find_stream_info(_fmt_ctx, nullptr);
        if (ret < 0)
        {
            char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
            av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
            log_msg_warn("avformat_find_stream_info %s failed, err:%s", url, buff);
            break;
        }
    } while (false);

    if (ret < 0)
    {
        avformat_close_input(&_fmt_ctx);
        avformat_free_context(_fmt_ctx);
        _fmt_ctx = nullptr;
        return false;
    }

    for (uint32_t i = 0; i < _fmt_ctx->nb_streams; ++i)
    {
        const auto * av_stream = _fmt_ctx->streams[i];
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
        _video_stream = _fmt_ctx->streams[_video_cur_index.load()];
    }

    if (!_audio_index.empty())
    {
        _audio_cur_index = _audio_index[0];
        _audio_stream = _fmt_ctx->streams[_audio_cur_index.load()];
    }

    _is_init = true;

    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO);

    _packets_thr = std::thread{ &CMediaPlayerImpl::recvPacketsThr,this };
    _video_thr = std::thread{ &CMediaPlayerImpl::dealVideoPacketsThr,this };
    _audio_thr = std::thread{ &CMediaPlayerImpl::dealAudioPacketsThr,this };

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

    if (_fmt_ctx)
    {
        avformat_close_input(&_fmt_ctx);
        avformat_free_context(_fmt_ctx);
        _fmt_ctx = nullptr;
    }

    SDL_Quit();

    _audio_index.clear();
    _is_playing = false;
    _cur_pos = 0;
    _audio_clock = 0.0;
}

int64_t CMediaPlayerImpl::getDuration()
{
    if (nullptr == _fmt_ctx)
    {
        log_msg_warn("file is not open!");
        return -1;
    }

    if (nullptr == _video_stream)
    {
        log_msg_warn("get video stream info failed!");
        return -1;
    }

    return static_cast<int64_t>(_fmt_ctx->duration * av_q2d(av_make_q(1, AV_TIME_BASE)));
}

int64_t CMediaPlayerImpl::getPosition()
{
    return static_cast<int64_t>(ceil(_audio_clock));
}

bool CMediaPlayerImpl::setPosition(int pos)
{
    if (nullptr == _fmt_ctx)
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
    return true;
}

bool CMediaPlayerImpl::backward()
{
    return true;
}

bool CMediaPlayerImpl::setVolume(const int volume)
{
    if (nullptr == _fmt_ctx || nullptr == _audio_stream)
    {
        log_msg_warn("There is no opened file ...");
        return false;
    }

    if (volume <= 0)
        _volume = 0;
    else if (volume >= 128)
        _volume = 128;
    else
        _volume = volume;

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

    if (!_audio_rescaler->create(codecpar->channel_layout, AV_SAMPLE_FMT_S16, 44100, codecpar->channel_layout,
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
}

bool CMediaPlayerImpl::createVideoPlayer(const void * wnd, const int width, const int height)
{
    if (nullptr == wnd)
    {
        log_msg_warn("Input param is nullptr.");
        return false;
    }

    // 创建窗口
    _player_wnd = SDL_CreateWindowFrom(wnd);
    if (nullptr == _player_wnd)
    {
        log_msg_warn("SDL_CreateWindowFrom failed.");
        return false;
    }
    // 创建渲染器
    _sdl_render = SDL_CreateRenderer(_player_wnd, -1, SDL_RENDERER_TARGETTEXTURE);
    if (nullptr == _sdl_render)
    {
        log_msg_warn("SDL_CreateRenderer failed.");
        destoryVideoPlayer();
        return false;
    }
    // 创建纹理器
    _sdl_texture = SDL_CreateTexture(_sdl_render, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (nullptr == _sdl_render)
    {
        log_msg_warn("SDL_CreateTexture failed!");
        destoryVideoPlayer();
        return false;
    }

    return true;
}

void CMediaPlayerImpl::destoryVideoPlayer()
{
    if (nullptr != _sdl_texture)
    {
        SDL_DestroyTexture(_sdl_texture);
        _sdl_texture = nullptr;
    }
    if (nullptr != _sdl_render)
    {
        SDL_DestroyRenderer(_sdl_render);
        _sdl_render = nullptr;
    }
    if (nullptr != _player_wnd)
    {
        SDL_DestroyWindow(_player_wnd);
        _player_wnd = nullptr;
    }
}

bool CMediaPlayerImpl::createAudioPlayer()
{
    _audio_info = static_cast<audio_info_t *>(malloc(sizeof(audio_info_t)));
    if (nullptr == _audio_info)
    {
        const int code = errno;
        log_msg_error("malloc failed for %s", strerror(code));
        return false;
    }
    _audio_info->chunk = _audio_info->pos = nullptr;
    _audio_info->volume = _volume.load();
    _audio_info->len = 0;

    SDL_AudioSpec audio_player;
    audio_player.freq = 44100; //根据你录制的PCM采样率决定
    audio_player.format = AUDIO_S16SYS;
    audio_player.channels = _audio_stream->codecpar->channels;
    audio_player.silence = 0;
    audio_player.samples = _audio_stream->codecpar->frame_size;
    audio_player.callback = readAudioDataCb;
    audio_player.userdata = _audio_info;
    if (SDL_OpenAudio(&audio_player, nullptr) < 0)
    {
        log_msg_warn("SDL_OpenAudio failed!");
        return false;
    }
    SDL_PauseAudio(0);

    return true;
}

void CMediaPlayerImpl::destoryAudioPlayer()
{
    SDL_CloseAudio();

    if (_audio_info)
    {
        free(_audio_info);
        _audio_info = nullptr;
    }
}

void CMediaPlayerImpl::recvPacketsThr()
{
    std::unique_lock<std::mutex> lck(_packets_mtx);
    _packets_cond.wait(lck);

    //_video_cond.notify_one();
    //_audio_cond.notify_one();

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
            int ret = av_seek_frame(_fmt_ctx, -1, ts, AVSEEK_FLAG_ANY);
            if (ret < 0)
            {
                char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
                av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
                log_msg_warn("%s", buff);
                break;
            }
            _video_queue.clear();
            _audio_queue.clear();
            _is_skip = false;
        }

        if (_video_queue.size() > 500 && _audio_queue.size() > 500)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        int ret = av_read_frame(_fmt_ctx, &pkt);
        if (ret < 0)
        {
            char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
            av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
            log_msg_warn("%s", buff);
            break;
        }

        if (_video_cur_index == pkt.stream_index)
        {
            _video_queue.push(pkt);
            _video_cond.notify_one();
        }
        else if (_audio_cur_index = pkt.stream_index)
        {
            _audio_queue.push(pkt);
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
        log_msg_warn("no available rescaler!");
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
            double pts = frm.best_effort_timestamp == AV_NOPTS_VALUE ? 0.0 : frm.best_effort_timestamp;
            pts = static_cast<double>(pts) * av_q2d(_video_stream->time_base);
            double delay = pts - _audio_clock;
            if (delay > 0.0)
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(delay * 1000)));

            AVFrame yuv_frm = { 0 };
            if (!_video_rescaler->rescale(&frm, &yuv_frm))
            {
                log_msg_warn("rescale failed!");
                continue;
            }

            SDL_UpdateYUVTexture(_sdl_texture, &_sdl_rect,
                                 yuv_frm.data[0],
                                 yuv_frm.linesize[0],
                                 yuv_frm.data[1],
                                 yuv_frm.linesize[1],
                                 yuv_frm.data[2],
                                 yuv_frm.linesize[2]);

            _sdl_rect = { 0, 0, _wnd_width, _wnd_height };

            SDL_RenderClear(_sdl_render);
            SDL_RenderCopy(_sdl_render, _sdl_texture, nullptr, &_sdl_rect);
            SDL_RenderPresent(_sdl_render);
        }
    }
}

void CMediaPlayerImpl::dealAudioPacketsThr()
{
    std::unique_lock<std::mutex> lck(_audio_mtx);
    _audio_cond.wait(lck);

    int out_buff_size = av_samples_get_buffer_size(nullptr, av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO),
                                                   1024, AV_SAMPLE_FMT_S16, 64);
    uint8_t * out_buff = (uint8_t *)av_malloc(44100 * 32 / 8 * 2);

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

        AVFrame frm = {};
        bool got = false;
        while (_audio_decoder->recv(&got, &frm) && got)
        {
            if (frm.best_effort_timestamp != AV_NOPTS_VALUE)
                _audio_clock = static_cast<double>(frm.best_effort_timestamp) * av_q2d(_audio_stream->time_base);
            _audio_rescaler->rescale(&frm, &out_buff, &out_buff_size);
            _audio_info->chunk = out_buff;
            _audio_info->len = out_buff_size;
            _audio_info->pos = _audio_info->;
            _audio_info->volume = _volume.load();

            while (_audio_info->len > 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

double CMediaPlayerImpl::getAudioClock()
{
    return 0.0;
}
