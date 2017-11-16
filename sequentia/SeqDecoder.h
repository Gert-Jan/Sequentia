#pragma once

extern "C"
{
	#include "libavformat/avformat.h"
	#include "libavcodec/avcodec.h"
	#include "libavutil/imgutils.h"
	#include "libavutil/samplefmt.h"
	#include "libavutil/timestamp.h"
}

class SeqVideoInfo;
struct SDL_mutex;

enum class SeqDecoderStatus
{
	Inactive,
	Opening,
	Loading,
	Ready,
	Stopping,
	Disposing
};

class SeqDecoder
{
public:
	SeqDecoder();
	~SeqDecoder();

	SeqDecoderStatus GetStatus();
	int64_t GetDuration();
	int64_t GetPlaybackTime();
	int64_t GetBufferTime();

	void Dispose();
	static int ReadVideoInfo(char *fullPath, SeqVideoInfo *videoInfo);
	int Preload(SeqVideoInfo *info);
	int Loop();
	void Stop();
	void Seek(int64_t time);
	AVFrame* NextFrame(int64_t);
	AVFrame* NextFrame();

private:
	void FillPacketBuffer();
	bool IsSlowAndShouldSkip();
	bool NextKeyFramePts(int64_t *result);
	int DecodePacket(AVPacket packet, AVFrame *target, int *frameIndex, int cached);
	static int OpenCodecContext(int *streamIndex, AVCodecContext **codec, AVFormatContext *format, enum AVMediaType type);
	void PrintAVError(char *message, int error);
	
private:
	static const int ffmpegRefcount = 1;
	static const int defaultFrameBufferSize = 60;
	static const int defaultPacketBufferSize = 500;

	SeqDecoderStatus status = SeqDecoderStatus::Inactive;
	SeqVideoInfo *videoInfo;
	int frameBufferSize = defaultFrameBufferSize;
	int packetBufferSize = defaultPacketBufferSize;
	uint8_t *videoDestData[4] = { NULL };
	bool skipFramesIfSlow = false;
	int64_t lastRequestedFrameTime = 0;
	int64_t bufferPunctuality = 0;
	int64_t lowestKeyFrameDecodeTime = 0;
	bool shouldSeek = false;
	int64_t seekTime = 0;
	SDL_mutex *seekMutex;
	AVFrame *audioFrame = NULL;
	int displayFrameCursor;
	int frameBufferCursor = 0;
	AVFrame **frameBuffer;
	int displayPacketCursor = 0;
	int packetBufferCursor = 0;
	AVPacket *packetBuffer;
};
