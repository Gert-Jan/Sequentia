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
	int64_t ToStreamTime(int64_t time);
	int64_t FromStreamTime(int64_t streamTime);
	static void GetTimeString(char *buffer, int bufferLen, int64_t time);
	static void GetTimeString(char *buffer, int bufferLen, uint32_t time);

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
	double timeBase = 1;
};
