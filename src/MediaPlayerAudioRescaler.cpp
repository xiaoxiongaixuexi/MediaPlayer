#include "MediaPlayerAudioRescaler.h"
#include "libos.h"

#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio 48000 * (32/8)

bool CMediaPlayerAudioRescaler::create(int64_t out_ch_layout, AVSampleFormat out_sample_fmt, int out_sample_rate,
                                       int64_t in_ch_layout, AVSampleFormat in_sample_fmt, int in_sample_rate, int frame_size)
{
    if (out_ch_layout == in_ch_layout && out_sample_fmt == in_sample_fmt && out_sample_rate == in_sample_rate)
    {
        _need_rescale = false;
        return true;
    }

    _rescaler = swr_alloc_set_opts(nullptr, out_ch_layout, out_sample_fmt, out_sample_rate,
                                   in_ch_layout, in_sample_fmt, in_sample_rate, 0, nullptr);
    if (nullptr == _rescaler)
    {
        log_msg_warn("swr_alloc_set_opts failed!");
        return false;
    }

    int ret = swr_init(_rescaler);
    if (ret < 0)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        log_msg_warn("swr_init failed, err:%s", buff);
        swr_free(&_rescaler);
        _rescaler = nullptr;

        return false;
    }

    int out_channels = av_get_channel_layout_nb_channels(_out_ch_layout);
    _out_len = av_samples_get_buffer_size(nullptr, out_channels, frame_size, out_sample_fmt, 64);
    if (_out_len <= 0)
    {
        log_msg_warn("av_samples_get_buffer_size failed!");
        destory();
        return false;
    }
    _out_data = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);
    if (nullptr == _out_data)
    {
        log_msg_error("av_malloc failed!");
        destory();
        return false;
    }

    _out_ch_layout = out_ch_layout;
    _out_sample_fmt = out_sample_fmt;
    _out_sample_rate = out_sample_rate;

    return true;
}

void CMediaPlayerAudioRescaler::destory()
{
    if (_out_data)
    {
        av_free(_out_data);
        _out_data = nullptr;
    }

    if (_rescaler)
    {
        swr_close(_rescaler);
        swr_free(&_rescaler);
        _rescaler = nullptr;
    }

    _need_rescale = false;
}

bool CMediaPlayerAudioRescaler::rescale(const AVFrame * in_frm, uint8_t ** out_data, int * out_len)
{
    if (nullptr == in_frm)
    {
        log_msg_warn("Input param is nullptr!");
        return false;
    }

    if (!_need_rescale)
    {
        *out_data = in_frm->data[0];
        *out_len = _out_len;
        return true;
    }

    if (nullptr == _rescaler)
    {
        log_msg_warn("rescaler is not open!");
        return false;
    }

    int ret = swr_convert(_rescaler, &_out_data, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)in_frm->data, in_frm->nb_samples);
    if (ret <= 0)
    {
        log_msg_warn("swr_convert failed!");
        return false;
    }

    *out_data = _out_data;
    *out_len = _out_len;

    return true;
}

void CMediaPlayerAudioRescaler::copyFrame(AVFrame * dst_frm, const AVFrame * src_frm)
{
    if (nullptr == src_frm || nullptr == dst_frm)
    {
        log_msg_warn("Input param is nullptr!");
        return;
    }

    // 拷贝数据
    //memcpy(dst_frm->data, _out_data, sizeof(_out_data[0]) * AV_NUM_DATA_POINTERS);
    //memcpy(dst_frm->linesize, _out_linesize, sizeof(_out_linesize[0]) * AV_NUM_DATA_POINTERS);

    //// 拷贝参数
    //dst_frm->format = static_cast<int>(_out_fmt);
    //dst_frm->pts = dst_frm->pts;
    //dst_frm->pkt_dts = src_frm->pkt_dts;
    //dst_frm->pkt_duration = src_frm->pkt_duration;
    //dst_frm->width = _out_width;
    //dst_frm->height = _out_height;
    dst_frm->color_range = src_frm->color_range;
    dst_frm->color_primaries = src_frm->color_primaries;
    dst_frm->color_trc = src_frm->color_trc;
    dst_frm->color_range = src_frm->color_range;
    dst_frm->colorspace = src_frm->colorspace;
}