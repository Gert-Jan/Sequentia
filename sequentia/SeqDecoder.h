#pragma once

extern "C"
{
	#include "libavformat/avformat.h"
	#include "libavcodec/avcodec.h"
	#include "libavutil/imgutils.h"
	#include "libavutil/samplefmt.h"
	#include "libavutil/timestamp.h"
}

class SeqLibraryLink;
class SeqVideoInfo;
struct SDL_mutex;

enum SeqDecoderStatus
{
	Inactive,
	Opening,
	Loading,
	Ready,
	Disposing
};

class SeqDecoder
{
public:
	SeqDecoder(SeqLibraryLink *link);
	~SeqDecoder();
	void Dispose();
	static int ReadVideoInfo(char *fullPath, SeqVideoInfo *videoInfo);
	int Preload();
	int Loop();
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

	SeqDecoderStatus status = Inactive;

	SeqLibraryLink *videoRef;
	
	int frameBufferSize = defaultFrameBufferSize;
	int packetBufferSize = defaultPacketBufferSize;
	uint8_t* videoDestData[4] = { NULL };
	bool skipFramesIfSlow = false;
	int64_t lastRequestedFrameTime = 0;
	int64_t bufferPunctuality = 0;
	int64_t lowestKeyFrameDecodeTime = 0;
	bool shouldSeek = false;
	int64_t seekTime = 0;
	SDL_mutex* seekMutex;
	AVFrame* audioFrame = NULL;
	int displayFrameCursor;
	int frameBufferCursor = 0;
	AVFrame** frameBuffer;
	int displayPacketCursor = 0;
	int packetBufferCursor = 0;
	AVPacket* packetBuffer;
};
