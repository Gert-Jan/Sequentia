#pragma once

extern "C"
{
	#include "libavformat/avformat.h"
	#include "libavutil/timestamp.h"
}

class SeqVideoInfo
{
public:
	SeqVideoInfo();
	~SeqVideoInfo();
	static void GetTimeString(char *buffer,int bufferLen, uint64_t time);
public:
	AVFormatContext *formatContext = nullptr;
	AVCodecContext *videoCodec = nullptr;
	AVCodecContext *audioCodec = nullptr;
	int videoDestLinesize[4];
	int videoDestBufferSize;
	int videoStreamIndex = -1;
	int audioStreamIndex = -1;
	int videoFrameCount = 0;
	int audioFrameCount = 0;
};
