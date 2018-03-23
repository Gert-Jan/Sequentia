#pragma once

extern "C"
{
	#include "libavformat/avformat.h"
	#include "libavcodec/avcodec.h"
	#include "libavutil/imgutils.h"
	#include "libavutil/samplefmt.h"
	#include "libavutil/timestamp.h"
}

template<class T>
class SeqList;
struct SeqStreamContext;
struct SDL_mutex;

enum class SeqDecoderStatus
{
	Inactive,
	Opening,
	Loading,
	Seeking,
	Ready,
	Stopping,
	Disposing
};

struct SeqFrameBuffer
{
	int size;
	AVFrame **buffer;
	int inUseCursor = 0;
	int insertCursor = 0;
	SeqStreamContext *usedBy = nullptr;
};

struct SeqStreamContext
{
	AVCodecContext *codecContext = nullptr;
	SeqFrameBuffer *frameBuffer = nullptr;
	int frameCount = 0;
	double timeBase = 1;
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
	SeqFrameBuffer* CreateFrameBuffer(int size);
	void DisposeFrameBuffer(SeqFrameBuffer *buffer);
	void DisposeStreamContext(SeqStreamContext *context);
	static int OpenFormatContext(const char *fullPath, AVFormatContext **formatContext);
	static void CloseFormatContext(AVFormatContext **formatContext);
	static int OpenCodecContext(int streamIndex, AVFormatContext *format, AVCodec **codec, AVCodecContext **context, double* timeBase);
	static void CloseCodecContext(AVCodecContext **codecContext);
	static int GetBestStream(AVFormatContext *format, enum AVMediaType type, int *streamIndex);
	int Preload();
	int Loop();
	void Stop();
	void Seek(int64_t time);
	AVFrame* NextFrame(int streamIndex, int64_t time);
	AVFrame* NextFrame(int streamIndex);
	void StartDecodingStream(int streamIndex);
	void StopDecodingStream(int streamIndex);
	static bool IsValidFrame(AVFrame *frame);

private:
	void SetStatusInactive();
	void SetStatusOpening();
	void SetStatusLoading();
	void SetStatusSeeking();
	void SetStatusReady();
	void SetStatusStopping();
	void SetStatusDisposing();

	void FillPacketBuffer();
	bool IsSlowAndShouldSkip();
	bool NextKeyFrameDts(int64_t *result);
	int DecodePacket(AVPacket packet, AVFrame *target, int *frameIndex, int cached);
	void RefreshStreamContexts();
	void RefreshPrimaryStreamContext();
	int NextFrameBuffer();
	void ResetFrameBuffer(SeqFrameBuffer *buffer);
	void PrintAVError(const char *message, int error);

public:
	AVFormatContext *formatContext = nullptr;

private:
	static const int defaultFrameBufferSize = 60;
	static const int defaultPacketBufferSize = 500;

	SeqDecoderStatus status = SeqDecoderStatus::Inactive;
	SDL_mutex *statusMutex;

	SeqStreamContext *streamContexts;
	int primaryStreamIndex = -1;

	bool skipFramesIfSlow = false;
	int64_t lastRequestedFrameTime;
	int64_t lastReturnedFrameTime;
	int64_t bufferPunctuality = 0;
	int64_t lowestKeyFrameDecodeTime = 0;

	bool shouldRefreshStreamContexts = false;
	SeqList<int> *startStreams;
	SeqList<int> *stopStreams;
	SDL_mutex *refreshStreamContextsMutex;
	
	bool shouldSeek = false;
	int64_t seekTime = 0;
	SDL_mutex *seekMutex;
	
	SeqList<SeqFrameBuffer*> *frameBuffers;

	int packetBufferSize = defaultPacketBufferSize;
	AVPacket *packetBuffer;
	int displayPacketCursor = 0;
	int packetBufferCursor = 0;
};
