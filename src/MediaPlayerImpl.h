#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <atomic>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "MediaMessageQueue.h"

#include "SDL.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#ifdef __cplusplus
}
#endif

// ���ű���
typedef enum _MEDIA_PLAYER_SPEED
{
	MEDIA_PLAYER_SPEED_NONE = 0,  // ��֧�ֱ���
	MEDIA_PLAYER_SPEED_QUARTER,   // 0.25����
	MEDIA_PLAYER_SPEED_HALF,      // 0.5����
	MEDIA_PLAYER_SPEED_NORMAL,    // 1����
	MEDIA_PLAYER_SPEED_DOUBLE,    // 2����
	MEDIA_PLAYER_SPEED_QUADRUPLE, // 4����
	MEDIA_PLAYER_SPEED_MAX        // �ⶥ
} MEDIA_PLAYER_SPEED;

typedef struct _audio_info_t
{
    int volume;
    int len;
	int index;        // ����λ��
    uint8_t * chunk;
    uint8_t * pos;
} audio_info_t;

class CMediaPlayerRescaler;

class CMediaPlayerImpl
{
public:
	CMediaPlayerImpl() = default;
	~CMediaPlayerImpl() = default;

public:
	// ���ļ�
	bool open(const char* url);

	// �ر��ļ�
	void close();

	// ��ȡ�ļ�ʱ��
	int64_t getVideoDuration();

	// ��ȡ��ǰ����λ��
	int64_t getVideoPos();

	// ��ʼ����
	bool start(const void* wnd, int width, int height);

	// ��ͣ
	bool pause();

	// ֹͣ
	bool stop();

	// ���
	bool forward();

	// ����
	bool backward();

protected:
	// ����������
	AVCodecContext * createDecoder(int stream_index);

	// ���ٽ�����
	void destoryDecoder(AVCodecContext ** decoder);

	// ��Ƶ֡����
	bool decodeVideoPacket(const AVPacket * pkt, AVFrame * frm, bool * got);

	// ��Ƶ֡����
	bool decodeAudioPacket(const AVPacket * pkt, AVFrame * frm, bool * got);

	// ������Ƶ������
	bool createVideoPlayer(const void * wnd, int width, int height);

	// ������Ƶ������
	void destoryVideoPlayer();

	// ������Ƶ������
	bool createAudioPlayer();

	// ������Ƶ������
	void destoryAudioPlayer();

	// ������Ƶת����
	bool createAudioRescaler();

	//������Ƶת����
	void destoryAudioRescaler();

	// �������ݰ��߳�
	void recvPacketsThr();

	// ������Ƶ�����߳�
	void dealVideoPacketsThr();

	// ������Ƶ�����߳�
	void dealAudioPacketsThr();

	// ��ȡ��Ƶʱ��
	double getAudioClock();

private:
	// �Ƿ��ʼ��
	std::atomic_bool _is_init = { false };

	// �Ƿ��ļ�����
	std::atomic_bool _is_over = { false };
	// �Ƿ񲥷���
	std::atomic_bool _is_playing = { false };
	// �Ƿ���ͣ��
	std::atomic_bool _is_pause = { false };

	// ���ݰ��߳���
	std::mutex _packets_mtx;
	// ��ȡ���ݰ��߳�
	std::thread _packets_thr;
	// ��ȡ���ݰ�����
	std::condition_variable _packets_cond;
	
	// ��Ƶ�����߳���
	std::mutex _video_mtx;
	// ������Ƶ�����߳�
	std::thread _video_thr;
	// ��Ƶ�̱߳���
	std::condition_variable _video_cond;

	// ��Ƶ�����߳���
	std::mutex _audio_mtx;
	// ������Ƶ�����߳�
	std::thread _audio_thr;
	// ��Ƶ�����̱߳���
	std::condition_variable _audio_cond;

	// ��Ƶ������
	std::atomic_int _video_index = { -1 };
	// ��Ƶ������
	AVCodecContext * _video_decoder = nullptr;

	// ��ǰʹ����Ƶ������
	std::atomic_int _audio_cur_index = { -1 };
	// ��Ƶ����������
    std::vector<int> _audio_index;
	// ��Ƶ������
	AVCodecContext * _audio_decoder = nullptr;
	// ��Ƶʱ��
	double _audio_clock = 0.0;

	// ��Ƶ���ظ�ʽ
	AVPixelFormat _pix_fmt = AVPixelFormat::AV_PIX_FMT_NONE;
	// ������������
	AVFormatContext * _fmt_ctx = nullptr;
	// ��Ƶ��
	AVStream * _video_stream = nullptr;
	// ��Ƶ��
	AVStream * _audio_stream = nullptr;

	// ��Ļ���
	int _wnd_width = 0;
	// ��Ļ�߶�
	int _wnd_height = 0;
	// ��Ƶ֡���
	int _frm_width = 0;
	// ��Ƶ֡�߶�
	int _frm_height = 0;

	audio_info_t * _audio_info = nullptr;
	// ��Ƶת����
	CMediaPlayerRescaler * _rescaler = nullptr;
	// ��Ƶת����
	SwrContext * _audio_rescaler = nullptr;

	// λ��
	SDL_Rect _sdl_rect = { 0 };
	// ���������
	SDL_Window * _player_wnd = nullptr;
	// ��Ⱦ��
	SDL_Renderer * _sdl_render = nullptr;
	// ɫ������
	SDL_Texture * _sdl_texture = nullptr;

	// ��Ƶ������
	CMediaMessageQueue<AVPacket> _video_queue;
	// ��Ƶ������
	CMediaMessageQueue<AVPacket> _audio_queue;

	// ��Ƶ����
	std::atomic_int _volume = { 64 };
	// ���ű��� Ĭ����������
	MEDIA_PLAYER_SPEED _speed = MEDIA_PLAYER_SPEED_NORMAL;
};

