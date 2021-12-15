#include "AudioRescalerImpl.h"
#include "libos.h"

#define MAX_AUDIO_FRAME_SIZE (44100 * 32 / 8) // 1 second of 48khz 32bit audio 48000 * (32/8)

bool CAudioRescalerImpl::create(const int64_t out_layout, const AVSampleFormat out_fmt, const int out_rate,
                                const int64_t in_layout, const AVSampleFormat in_fmt, const int in_rate, const int frame_size)
{
    if (out_layout == in_layout && out_fmt == in_fmt && out_rate == in_rate)
    {
        _need_rescale = false;
        return true;
    }

    _rescaler = swr_alloc_set_opts(nullptr, out_layout, out_fmt, out_rate, in_layout, in_fmt, in_rate, 0, nullptr);
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

    int out_channels = av_get_channel_layout_nb_channels(out_layout);
    _out_len = av_samples_get_buffer_size(nullptr, out_channels, frame_size, out_fmt, 1);
    if (_out_len <= 0)
    {
        log_msg_warn("av_samples_get_buffer_size failed!");
        destroy();
        return false;
    }
    _out_data = static_cast<uint8_t *>(av_malloc(MAX_AUDIO_FRAME_SIZE * 2));
    if (nullptr == _out_data)
    {
        log_msg_error("av_malloc failed!");
        destroy();
        return false;
    }

    _out_ch_layout = out_layout;
    _out_sample_fmt = out_fmt;
    _out_sample_rate = out_rate;

    return true;
}

void CAudioRescalerImpl::destroy()
{

}

bool CAudioRescalerImpl::rescale(const AVFrame * in_frm, uint8_t ** out_data, int * out_len)
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
    *out_len = ret * in_frm->channels * av_get_bytes_per_sample(_out_sample_fmt);

    return true;
}

void CAudioRescalerImpl::copyFrame(AVFrame * dst_frm, const AVFrame * src_frm)
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
