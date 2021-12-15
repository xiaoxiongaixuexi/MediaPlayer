#pragma once

#include <cstdio>
#include <cstdint>
#include <cstdbool>

extern "C" {
#include "libavutil/avutil.h"
#include "libavcodec/avcodec.h"
}

class CAVDecoderImpl
{
public:
    CAVDecoderImpl() = default;
    ~CAVDecoderImpl() = default;

    // 创建解码器
    bool create(const AVCodecParameters * codecpar);

    // 销毁解码器
    void destroy();

    // 发送到解码器
    bool send(const AVPacket * pkt);

    // 从解码器接收
    bool recv(bool * got, AVFrame * frm);

private:
    // 解码器上下文
    AVCodecContext * _codec_ctx = nullptr;
};
