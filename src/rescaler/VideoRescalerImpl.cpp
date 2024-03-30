#include "VideoRescalerImpl.h"
#include "log.h"

bool CVideoRescalerImpl::create(const AVPixelFormat in_fmt, const int in_width, const int in_height,
                                const AVPixelFormat out_fmt, const int out_width, const int out_height)
{
    if (_rescaler)
    {
        log_msg_warn("rescaler has already opened!");
        return false;
    }

    // 判断像素格式是否支持
    if (!sws_isSupportedInput(in_fmt))
    {
        log_msg_warn("Input pixel:%d format is not supported", in_fmt);
        return false;
    }
    if (!sws_isSupportedOutput(out_fmt))
    {
        log_msg_warn("Output pixel:%d format is not supported.", out_fmt);
        return false;
    }

    // 完全相同，不需要转换
    if (in_width == out_width && in_height == out_height && in_fmt == out_fmt)
    {
        log_msg_info("No need swscale!");
        _need_rescale = false;
        return true;
    }

    _rescaler = sws_getContext(in_width, in_height, in_fmt, out_width, out_height, out_fmt, 0, nullptr, nullptr, nullptr);
    if (nullptr == _rescaler)
    {
        log_msg_warn("sws_alloc_context failed!");
        return false;
    }

    int ret = av_image_alloc(_out_data, _out_linesize, out_width, out_height, out_fmt, 64);
    if (ret <= 0)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        log_msg_warn("av_image_alloc failed, err:%s", buff);
        sws_freeContext(_rescaler);
        _rescaler = nullptr;
        return false;
    }

    _in_width = in_width;
    _in_height = in_height;
    _in_fmt = in_fmt;
    _out_width = out_width;
    _out_height = out_height;
    _out_fmt = out_fmt;
    _need_rescale = true;

    return true;
}

void CVideoRescalerImpl::destroy()
{
    if (nullptr != _rescaler)
    {
        sws_freeContext(_rescaler);
        _rescaler = nullptr;
    }

    if (nullptr != _out_data)
    {
        av_freep(&_out_data[0]);
        memset(_out_data, 0, sizeof(_out_data));
        memset(_out_linesize, 0, sizeof(_out_linesize));
    }

    _in_width = 0;
    _in_height = 0;
    _out_width = 0;
    _out_height = 0;
    _need_rescale = true;
}

bool CVideoRescalerImpl::rescale(const AVFrame * in_frm, AVFrame * out_frm)
{
    if (nullptr == in_frm || nullptr == in_frm->data[0] || 0 >= in_frm->linesize[0] || nullptr == out_frm)
    {
        log_msg_warn("Input param is invalid!");
        return false;
    }

    if (!_need_rescale)
    {
        memcpy(out_frm->data, in_frm->data, sizeof(in_frm->data[0]) * AV_NUM_DATA_POINTERS);
        memcpy(out_frm->linesize, in_frm->linesize, sizeof(in_frm->linesize[0]) * AV_NUM_DATA_POINTERS);
        return true;
    }

    if (nullptr == _rescaler)
    {
        log_msg_warn("rescaler is not open!");
        return false;
    }

    int ret = sws_scale(_rescaler, in_frm->data, in_frm->linesize, 0, _in_height, _out_data, _out_linesize);
    if (ret <= 0)
    {
        log_msg_warn("sws_scale failed!");
        return false;
    }

    copyFrame(out_frm, in_frm);

    return true;
}

void CVideoRescalerImpl::copyFrame(AVFrame * dst_frm, const AVFrame * src_frm)
{
    if (nullptr == src_frm || nullptr == dst_frm)
    {
        log_msg_warn("Input param is nullptr!");
        return;
    }

    // 拷贝数据
    memcpy(dst_frm->data, _out_data, sizeof(_out_data[0]) * AV_NUM_DATA_POINTERS);
    memcpy(dst_frm->linesize, _out_linesize, sizeof(_out_linesize[0]) * AV_NUM_DATA_POINTERS);

    // 拷贝参数
    dst_frm->format = static_cast<int>(_out_fmt);
    dst_frm->pts = dst_frm->pts;
    dst_frm->pkt_dts = src_frm->pkt_dts;
    dst_frm->pkt_duration = src_frm->pkt_duration;
    dst_frm->width = _out_width;
    dst_frm->height = _out_height;
    dst_frm->color_range = src_frm->color_range;
    dst_frm->color_primaries = src_frm->color_primaries;
    dst_frm->color_trc = src_frm->color_trc;
    dst_frm->color_range = src_frm->color_range;
    dst_frm->colorspace = src_frm->colorspace;
}
