#pragma once

#include <cstdio>
#include <cstdint>
#include <cstdbool>

extern "C"{
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}

class CVideoRescalerImpl
{
public:
    CVideoRescalerImpl() = default;
    ~CVideoRescalerImpl() = default;

    // 创建转换器
    bool create(AVPixelFormat in_fmt, int in_width, int in_height, AVPixelFormat out_fmt, int out_width, int out_height);

    // 销毁
    void destroy();

    // 转换
    bool rescale(const AVFrame * in_frm, AVFrame * out_frm);

protected:
    // 拷贝数据帧
    void copyFrame(AVFrame * dst_frm, const AVFrame * src_frm);

private:
    // 是否需要转换
    bool _need_rescale = true;

    // 输入宽度
    int _in_width = 0;
    // 输入高度
    int _in_height = 0;

    // 输出高度
    int _out_width = 0;
    // 输出宽度
    int _out_height = 0;

    // 输入像素格式
    AVPixelFormat _in_fmt = AVPixelFormat::AV_PIX_FMT_NONE;
    // 输出像素格式
    AVPixelFormat _out_fmt = AVPixelFormat::AV_PIX_FMT_NONE;

    // 输出数据
    uint8_t * _out_data[AV_NUM_DATA_POINTERS] = { nullptr };
    // 输出行
    int _out_linesize[AV_NUM_DATA_POINTERS] = { 0 };

    // 视频转换器
    SwsContext * _rescaler = nullptr;
};
