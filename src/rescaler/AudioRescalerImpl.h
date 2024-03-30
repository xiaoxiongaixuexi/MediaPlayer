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
    bool create(int64_t out_layout, int out_fmt, int out_rate,
                int64_t in_layout, int in_fmt, int in_rate, int frame_size);

    // 销毁
    void destroy();

    // 转换
    bool rescale(const AVFrame * in_frm, uint8_t ** out_data, int * out_len);

protected:
    // 调整缓冲区大小
    bool resizeCache(int nb_samples);

private:
    // 是否需要转换
    bool _need_rescale = true;

    // 输入声道排列
    int64_t _in_ch_layout = -1;
    // 输入采样率
    int _in_sample_rate = 0;
    // 输入帧大小
    int _in_nb_samples = 0;
    // 输入采样格式
    enum AVSampleFormat _in_sample_fmt = AV_SAMPLE_FMT_NONE;

    // 输出采样率
    int _out_sample_rate = 0;
    // 输出通道数
    int _out_channels = 0;
    // 输出通道排列
    int64_t _out_ch_layout = -1;
    // 输出采样格式
    enum AVSampleFormat _out_sample_fmt = AV_SAMPLE_FMT_NONE;

    int _out_max_nb_samples = 0;

    // 输出内容长度
    int _out_size[AV_NUM_DATA_POINTERS] = { 0 };
    // 输出内容
    uint8_t * _out_data[AV_NUM_DATA_POINTERS] = { nullptr };

    // 转换器
    SwrContext * _swr_ctx = nullptr;
};
