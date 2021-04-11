#pragma once
#include <stdio.h>
#include <stdbool.h>
extern "C" {
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}

class CMediaPlayerRescaler
{
public:
	CMediaPlayerRescaler() = default;
	~CMediaPlayerRescaler() = default;

	// ����ת����
	bool create(AVPixelFormat in_fmt, int in_width, int in_height, AVPixelFormat out_fmt, int out_width, int out_height);

	// ����ת����
	void destory();

	// ת��
	bool rescale(const AVFrame * in_frm, AVFrame * out_frm);

protected:
	// ��������֡
	void copyFrame(AVFrame * dst_frm, const AVFrame * src_frm);

private:
	// �Ƿ���Ҫת��
	bool _need_rescale = true;

	// ������
	int _in_width = 0;
	// ����߶�
	int _in_height = 0;

	// ����߶�
	int _out_width = 0;
	// ������
	int _out_height = 0;

	// �������ظ�ʽ
	AVPixelFormat _in_fmt = AVPixelFormat::AV_PIX_FMT_NONE;
	// ������ظ�ʽ
	AVPixelFormat _out_fmt = AVPixelFormat::AV_PIX_FMT_NONE;

	// �������
	uint8_t * _out_data[AV_NUM_DATA_POINTERS] = { nullptr };
	// �����
	int _out_linesize[AV_NUM_DATA_POINTERS] = { 0 };

	// ת����
	SwsContext * _rescaler = nullptr;
};
