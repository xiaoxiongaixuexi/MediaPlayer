#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <atomic>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "utils/MediaPlayerQueue.h"

extern "C" {
#include "libavutil/avutil.h"
#include "libavformat/avformat.h"
}

// 播放倍速
typedef enum _MEDIA_PLAYER_SPEED
{
    MEDIA_PLAYER_SPEED_NONE = 0,  // 不支持倍速
    MEDIA_PLAYER_SPEED_QUARTER,   // 0.25倍速
    MEDIA_PLAYER_SPEED_HALF,      // 0.5倍速
    MEDIA_PLAYER_SPEED_NORMAL,    // 1倍速
    MEDIA_PLAYER_SPEED_DOUBLE,    // 2倍速
    MEDIA_PLAYER_SPEED_QUADRUPLE, // 4倍速
    MEDIA_PLAYER_SPEED_MAX        // 封顶
} MEDIA_PLAYER_SPEED;

typedef struct _audio_info_t
{
    int volume;
    int len;
    int pos;         // 播放位置
    uint8_t * data;
} audio_info_t;

class CMediaPlayerDemux;
class CVideoRescalerImpl;
class CAudioRescalerImpl;
class CAVDecoderImpl;
class CVideoRendererSDL;

class CMediaPlayerImpl
{
public:
    static CMediaPlayerImpl & getInstance()
    {
        static CMediaPlayerImpl instance;
        return instance;
    }

    // 是否有开着的文件
    bool opened() const;

    // 打开文件
    bool open(const char * url);

    // 关闭文件
    void close();

    // 获取文件时长
    int64_t getDuration();

    // 获取当前播放位置
    int64_t getPosition();

    // 设置播放位置
    bool setPosition(int pos);

    // 开始播放
    bool start(const void * wnd, int width, int height);

    // 暂停
    bool pause();

    // 快进
    bool forward();

    // 快退
    bool backward();

    // 设置音量
    bool setVolume(int volume);

protected:
    // 初始化视频流
    bool initVideoStream();

    // 销毁视频流
    void uninitVideoStream();

    // 初始化音频流
    bool initAudioStream();

    // 销毁音频流
    void uninitAudioStream();

    // 创建视频播放器
    bool createVideoPlayer(const void * wnd, int width, int height);

    // 销毁视频播放器
    void destoryVideoPlayer();

    // 创建音频播放器
    bool createAudioPlayer();

    // 销毁音频播放器
    void destoryAudioPlayer();

    // 接收数据包线程
    void recvPacketsThr();

    // 处理视频数据线程
    void dealVideoPacketsThr();

    // 处理音频数据线程
    void dealAudioPacketsThr();

private:
    CMediaPlayerImpl() = default;
    ~CMediaPlayerImpl() = default;
    // 删除三大默认成员函数，赋值、复制和地址运算
    CMediaPlayerImpl(const CMediaPlayerImpl &) = delete;
    CMediaPlayerImpl & operator=(const CMediaPlayerImpl &) = delete;
    CMediaPlayerImpl * operator&() = delete;

    // 是否初始化
    std::atomic_bool _is_init = { false };

    // 是否文件结束
    std::atomic_bool _is_over = { false };
    // 是否播放中
    std::atomic_bool _is_playing = { false };
    // 是否暂停中
    std::atomic_bool _is_pause = { false };
    // 是否跳转播放位置
    std::atomic_bool _is_skip = { false };
    // 当前播放位置
    std::atomic_int32_t _cur_pos = { 0 };

    // 数据包线程锁
    std::mutex _packets_mtx;
    // 读取数据包线程
    std::thread _packets_thr;
    // 读取数据包变量
    std::condition_variable _packets_cond;

    // 视频数据线程锁
    std::mutex _video_mtx;
    // 处理视频数据线程
    std::thread _video_thr;
    // 视频线程变量
    std::condition_variable _video_cond;

    // 音频数据线程锁
    std::mutex _audio_mtx;
    // 处理音频数据线程
    std::thread _audio_thr;
    // 音频解码线程变量
    std::condition_variable _audio_cond;

    // 视频流索引数组
    std::vector<int> _video_index;
    // 当前使用视频流索引
    std::atomic_int _video_cur_index = { -1 };
    // 视频流
    AVStream * _video_stream = nullptr;
    // 视频解码器
    CAVDecoderImpl * _video_decoder = nullptr;
    // 视频转换器
    CVideoRescalerImpl * _video_rescaler = nullptr;
    // 视频播放器
    CVideoRendererSDL * _video_player = nullptr;

    // 音频流索引数组
    std::vector<int> _audio_index;
    // 当前使用音频流索引
    std::atomic_int _audio_cur_index = { -1 };
    // 音频流
    AVStream * _audio_stream = nullptr;
    // 音频解码器
    CAVDecoderImpl * _audio_decoder = nullptr;
    // 音频转换器
    CAudioRescalerImpl * _audio_rescaler = nullptr;
    // 音频时钟
    std::atomic<double> _audio_clock = { 0.0 };

    // 视频像素格式
    AVPixelFormat _pix_fmt = AVPixelFormat::AV_PIX_FMT_NONE;
    // 输入流上下文
    //AVFormatContext * _fmt_ctx = nullptr;
    CMediaPlayerDemux * _demux_ctx = nullptr;

    // 屏幕宽度
    int _wnd_width = 0;
    // 屏幕高度
    int _wnd_height = 0;
    // 视频帧宽度
    int _frm_width = 0;
    // 视频帧高度
    int _frm_height = 0;

    audio_info_t * _audio_info = nullptr;

    // 视频包队列
    CMediaPlayerQueue<AVPacket> _video_queue;
    // 音频包队列
    CMediaPlayerQueue<AVPacket> _audio_queue;

    // 音频音量
    std::atomic_int _volume = { 64 };
    // 文件时长
    std::atomic_int64_t _duration = { -1 };
    // 播放倍速 默认正常倍速
    MEDIA_PLAYER_SPEED _speed = MEDIA_PLAYER_SPEED_NORMAL;
};
