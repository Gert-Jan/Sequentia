#pragma once

extern "C"
{
	#include "libavformat/avformat.h"
	#include "libavcodec/avcodec.h"
	#include "libavutil/imgutils.h"
	#include "libavutil/samplefmt.h"
	#include "libavutil/timestamp.h"
}

struct SDL_mutex;

enum class SeqEncoderStatus
{
	Inactive,
	Initializing,
	Ready,
	Stopping,
	Disposing
};

class SeqEncoder
{
public:
	SeqEncoder();
	~SeqEncoder();

	SeqEncoderStatus GetStatus();

	void Dispose();
	static int CreateFormatContext(const char *fullPath, AVFormatContext **formatContext);
	static void CloseFormatContext(AVFormatContext **formatContext);
	int Loop();

private:
	void SetStatusInactive();
	void SetStatusInitializing();
	void SetStatusReady();
	void SetStatusStopping();
	void SetStatusDisposing();

	void PrintAVError(const char *message, int error);

private:
	static const int defaultFrameBufferSize = 60;
	static const int defaultPacketBufferSize = 500;

	AVFormatContext *formatContext = nullptr;
	SeqEncoderStatus status = SeqEncoderStatus::Initializing;
	SDL_mutex *statusMutex;
};
