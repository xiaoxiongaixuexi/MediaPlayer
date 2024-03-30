#include "MediaPlayerDecoderImpl.h"
#include "log.h"

bool CAVDecoderImpl::create(const AVCodecParameters * codecpar)
{
    if (nullptr == codecpar)
    {
        log_msg_warn("Input param is nullptr.");
        return false;
    }

    if (nullptr != _codec_ctx)
    {
        log_msg_warn("Already create decoder");
        return false;
    }

    auto * codec = avcodec_find_decoder(codecpar->codec_id);
    if (nullptr == codec)
    {
        log_msg_warn("avcodec_find_decoder by %d failed!", codecpar->codec_id);
        return false;
    }

    _codec_ctx = avcodec_alloc_context3(codec);
    if (nullptr == _codec_ctx)
    {
        log_msg_warn("avcodec_alloc_context3 failed!");
        return false;
    }

    int ret = 0;
    do
    {
        ret = avcodec_parameters_to_context(_codec_ctx, codecpar);
        if (ret < 0)
        {
            char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
            av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
            log_msg_warn("avcodec_parameters_to_context failed, err: %s", buff);
            break;
        }

        ret = avcodec_open2(_codec_ctx, codec, nullptr);
        if (0 != ret)
        {
            char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
            av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
            log_msg_warn("avcodec_open2 failed, err: %s", buff);
            break;
        }
    } while (false);

    if (0 != ret)
    {
        avcodec_free_context(&_codec_ctx);
        _codec_ctx = nullptr;
        return false;
    }

    _codec_par = avcodec_parameters_alloc();
    if (nullptr == _codec_par)
    {
        log_msg_warn("avcodec_parameters_alloc failed.");
        return true;
    }

    ret = avcodec_parameters_copy(_codec_par, codecpar);
    if (ret < 0)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        log_msg_warn("avcodec_parameters_copy failed, err: %s", buff);
        _codec_par = nullptr;
    }

    return true;
}

void CAVDecoderImpl::destroy(const bool flag)
{
    if (nullptr != _codec_ctx)
    {
        avcodec_close(_codec_ctx);
        avcodec_free_context(&_codec_ctx);
        _codec_ctx = nullptr;
    }

    if (nullptr != _codec_par && flag)
    {
        avcodec_parameters_free(&_codec_par);
        _codec_par = nullptr;
    }
}

bool CAVDecoderImpl::send(const AVPacket * pkt)
{
    if (nullptr == _codec_ctx)
    {
        log_msg_warn("Decoder is not open yet.");
        return false;
    }

    int ret = avcodec_send_packet(_codec_ctx, pkt);
    if (0 != ret)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        log_msg_warn("avcodec_send_packet failed, err code: %d, msg: %s", ret, buff);
        return false;
    }

    return true;
}

bool CAVDecoderImpl::recv(bool & got, AVFrame * frm)
{
    if (nullptr == _codec_ctx)
    {
        log_msg_warn("Decoder is not open yet.");
        return false;
    }

    if (nullptr == frm)
    {
        log_msg_warn("Input param is nullptr.");
        return false;
    }

    int ret = avcodec_receive_frame(_codec_ctx, frm);
    if (0 != ret)
    {
        if (ret == AVERROR(EAGAIN))
        {
            got = false;
            return true;
        }

        char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        log_msg_warn("avcodec_receive_frame failed for %s", buff);
        return false;
    }

    got = true;

    return true;
}

bool CAVDecoderImpl::reopen()
{
    if (nullptr == _codec_par)
    {
        log_msg_warn("Codec params is nullptr.");
        return false;
    }

    destroy(false);

    return create(_codec_par);
}
