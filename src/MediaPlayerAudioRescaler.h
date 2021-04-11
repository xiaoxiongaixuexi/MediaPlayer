#pragma once
#include <stdio.h>
#include <stdbool.h>
extern "C"{
#include "libswresample/swresample.h"
}

class CMediaPlayerAudioRescaler
{
public:
	CMediaPlayerAudioRescaler() = default;
	~CMediaPlayerAudioRescaler() = default;

	// ����
    bool create(int64_t out_ch_layout, AVSampleFormat out_sample_fmt, int out_sample_rate,
                int64_t in_ch_layout, AVSampleFormat in_sample_fmt, int in_sample_rate, int frame_size);

	// ����ת����
	void destory();

	// ת��
	bool rescale(const AVFrame * in_frm, uint8_t ** out_data, int * out_len);

protected:
	// ��������
	void copyFrame(AVFrame * dst_frm, const AVFrame * src_frm);

private:
	// �Ƿ���Ҫת��
	bool _need_rescale = false;

	// ���ͨ������
	int64_t _out_ch_layout = -1;
	// ���������ʽ
	AVSampleFormat _out_sample_fmt = AV_SAMPLE_FMT_NONE;
	// ���������
	int _out_sample_rate = 0;
	
	// ת����
	SwrContext * _rescaler = nullptr;

	// �������
	uint8_t * _out_data = nullptr;
	// ������ݳ���
	int _out_len = -1;
};
