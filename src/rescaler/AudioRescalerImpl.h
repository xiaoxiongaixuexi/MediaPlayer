#pragma once

#include <cstdio>
#include <cstdint>
#include <cstdbool>

extern "C"{
#include "libswresample/swresample.h"
}

class CAudioRescalerImpl
{
public:
    CAudioRescalerImpl() = default;
    ~CAudioRescalerImpl() = default;

    // 创建转换器
    bool create(int64_t out_layout, AVSampleFormat out_fmt, int out_rate,
                int64_t in_layout, AVSampleFormat in_fmt, int in_rate, int frame_size);

    // 销毁
    void destroy();

    // 转换
    bool rescale(const AVFrame * in_frm, uint8_t ** out_data, int * out_len);

protected:
    // 拷贝数据
    void copyFrame(AVFrame * dst_frm, const AVFrame * src_frm);

private:
    // 是否需要转换
    bool _need_rescale = true;

    // 输出采样率
    int _out_sample_rate = 0;
    // 输出通道排列
    int64_t _out_ch_layout = -1;
    // 输出采样格式
    AVSampleFormat _out_sample_fmt = AV_SAMPLE_FMT_NONE;

    // 输出内容长度
    int _out_len = -1;
    // 输出内容
    uint8_t * _out_data = nullptr;

    // 转换器
    SwrContext * _rescaler = nullptr;
};
