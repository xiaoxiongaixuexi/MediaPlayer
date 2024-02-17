#pragma once

#include <cstdint>
#include <cstdbool>

typedef struct _audio_info_t
{
    int volume;
    int len;
    int pos;         // 播放位置
    uint8_t * data;
} audio_info_t;

class CAudioRendererSDL
{
public:
    CAudioRendererSDL() = default;
    ~CAudioRendererSDL() = default;

    // 创建
    bool create(int sample_rate, int channels, int frame_size, int volume = 64);

    // 销毁
    void destroy();

    // 设置音量
    void setVolume(int volume);

    // 填充数据
    void fillData(uint8_t * data, int len);

    // 数据是否消耗完成
    bool finished();

protected:


private:
    audio_info_t * _cache = nullptr;
};
