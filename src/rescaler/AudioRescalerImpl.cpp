#include "AudioRescalerImpl.h"
#include "os_log.h"

bool CAudioRescalerImpl::create(const int64_t out_layout, const int out_fmt, const int out_sample_rate,
                                const int64_t in_layout, const int in_fmt, const int in_sample_rate, int frame_size)
{
    if (out_layout == in_layout && out_fmt == in_fmt && out_sample_rate == in_sample_rate)
    {
        _need_rescale = false;
        return true;
    }

    _in_sample_fmt = static_cast<AVSampleFormat>(in_fmt);
    _out_sample_fmt = static_cast<AVSampleFormat>(out_fmt);

    _swr_ctx = swr_alloc_set_opts(nullptr, out_layout, _out_sample_fmt, out_sample_rate,
                                  in_layout, _in_sample_fmt, in_sample_rate, 0, nullptr);
    if (nullptr == _swr_ctx)
    {
        log_msg_warn("swr_alloc_set_opts failed!");
        return false;
    }

    int ret = swr_init(_swr_ctx);
    if (ret < 0)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        log_msg_warn("swr_init failed, err:%s", buff);
        swr_free(&_swr_ctx);
        _swr_ctx = nullptr;
        return false;
    }

    _out_channels = av_get_channel_layout_nb_channels(out_layout);
    if (_out_channels <= 0)
    {
        log_msg_warn("Output channels %d is invalid.", _out_channels);
        destroy();
        return false;
    }

    _in_ch_layout = in_layout;
    _in_sample_rate = in_sample_rate;
    _in_nb_samples = frame_size;

    _out_ch_layout = out_layout;
    _out_sample_rate = out_sample_rate;

    return true;
}

void CAudioRescalerImpl::destroy()
{
    if (nullptr != _swr_ctx)
    {
        swr_close(_swr_ctx);
        swr_free(&_swr_ctx);
        _swr_ctx = nullptr;
    }

    if (nullptr != _out_data[0])
    {
        av_freep(&_out_data[0]);
        memset(_out_data, 0, AV_NUM_DATA_POINTERS);
        memset(_out_size, 0, AV_NUM_DATA_POINTERS);
    }
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
        *out_len = av_samples_get_buffer_size(_out_size, in_frm->channels, in_frm->nb_samples, _in_sample_fmt, 0);
        return true;
    }

    if (nullptr == _swr_ctx)
    {
        log_msg_warn("rescaler is not open!");
        return false;
    }

    int in_samples_per_channel = 0;
    int out_samples_per_channel = 0;

    in_samples_per_channel = in_frm->linesize[0] / av_get_bytes_per_sample(_in_sample_fmt);
    if (!av_sample_fmt_is_planar(_in_sample_fmt))
        in_samples_per_channel /= in_frm->channels;
    out_samples_per_channel = swr_get_out_samples(_swr_ctx, in_samples_per_channel);
    if (!resizeCache(out_samples_per_channel))
    {
        log_msg_warn("resizeCache failed.");
        return false;
    }

    int ret = swr_convert(_swr_ctx, _out_data, out_samples_per_channel, (const uint8_t **)in_frm->data, _in_nb_samples);
    if (ret <= 0)
    {
        log_msg_warn("swr_convert failed!");
        return false;
    }

    int buf_size = av_samples_get_buffer_size(_out_size, _out_channels, ret, _out_sample_fmt, 0);
    if (buf_size < 0)
    {
        log_msg_warn("Could not get sample buffer size!");
        return false;
    }
    *out_data = _out_data[0];
    *out_len = buf_size;

    return true;
}

bool CAudioRescalerImpl::resizeCache(const int nb_samples)
{
    if (nb_samples <= _out_max_nb_samples)
        return true;

    if (nullptr != _out_data[0])
    {
        av_freep(&_out_data[0]);
        memset(_out_data, 0, AV_NUM_DATA_POINTERS);
        memset(_out_size, 0, AV_NUM_DATA_POINTERS);
    }

    int ret = av_samples_alloc(_out_data, _out_size, _out_channels, nb_samples, _out_sample_fmt, 1);
    if (ret < 0)
    {
        log_msg_warn("av_samples_alloc failed.");
        return false;
    }

    _out_max_nb_samples = nb_samples;

    return true;
}
