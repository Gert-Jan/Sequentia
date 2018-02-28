#pragma once

extern "C"
{
	#include "libavformat/avformat.h"
	#include "libavcodec/avcodec.h"
	#include "libavutil/imgutils.h"
	#include "libavutil/samplefmt.h"
	#include "libavutil/timestamp.h"
}

class SeqVideoContext;
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
	int64_t GetBufferLeft();
	int64_t GetBufferRight();

	void Dispose();
	static int OpenFormatContext(const char *fullPath, AVFormatContext **formatContext);
	static void CloseFormatContext(AVFormatContext **formatContext);
	static int OpenCodecContext(int streamIndex, AVFormatContext *format, AVCodec **codec, AVCodecContext **context, double* timeBase);
	static void CloseCodecContext(AVCodecContext **codecContext);
	static int GetBestStream(AVFormatContext *format, enum AVMediaType type, int *streamIndex);
	int Preload(SeqVideoContext *videoContext);
	int Loop();
	void Stop();
	void Seek(int64_t time);
	AVFrame* NextFrame(int64_t);
	AVFrame* NextFrame();
	void SetVideoStreamIndex(int streamIndex);
	void SetAudioStreamIndex(int streamIndex);
	static bool IsValidFrame(AVFrame *frame);

private:
	void SetStatusInactive();
	void SetStatusOpening();
	void SetStatusLoading();
	void SetStatusReady();
	void SetStatusStopping();
	void SetStatusDisposing();

	void FillPacketBuffer();
	bool IsSlowAndShouldSkip();
	bool NextKeyFrameDts(int64_t *result);
	int DecodePacket(AVPacket packet, AVFrame *target, int *frameIndex, int cached);
	void RefreshCodecContexts();
	void PrintAVError(const char *message, int error);
	
private:
	static const int defaultFrameBufferSize = 60;
	static const int defaultPacketBufferSize = 500;

	SeqDecoderStatus status = SeqDecoderStatus::Inactive;
	SeqVideoContext *context;
	int frameBufferSize = defaultFrameBufferSize;
	int packetBufferSize = defaultPacketBufferSize;
	uint8_t *videoDestData[4] = { NULL };
	bool skipFramesIfSlow = false;
	int64_t lastRequestedFrameTime = 0;
	int64_t bufferPunctuality = 0;
	int64_t lowestKeyFrameDecodeTime = 0;
	bool shouldSeek = false;
	int64_t seekTime = 0;
	bool shouldRefreshCodex = false;
	int newVideoStreamIndex = -1;
	int newAudioStreamIndex = -1;
	SDL_mutex *seekMutex;
	SDL_mutex *statusMutex;
	AVFrame *audioFrame = NULL;
	int displayFrameCursor;
	int frameBufferCursor = 0;
	AVFrame **frameBuffer;
	int displayPacketCursor = 0;
	int packetBufferCursor = 0;
	AVPacket *packetBuffer;
};
