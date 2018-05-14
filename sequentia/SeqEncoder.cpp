#include "SeqEncoder.h"
#include <SDL.h>

SeqEncoder::SeqEncoder() :
	formatContext(nullptr)
{
	statusMutex = SDL_CreateMutex();
}

SeqEncoder::~SeqEncoder()
{
	Dispose();
	SDL_DestroyMutex(statusMutex);
}

SeqEncoderStatus SeqEncoder::GetStatus()
{
	return status;
}

void SeqEncoder::SetStatusInactive()
{
	SDL_LockMutex(statusMutex);
	status = SeqEncoderStatus::Inactive;
	SDL_UnlockMutex(statusMutex);
}

void SeqEncoder::SetStatusInitializing()
{
	SDL_LockMutex(statusMutex);
	if (status < SeqEncoderStatus::Stopping)
		status = SeqEncoderStatus::Initializing;
	SDL_UnlockMutex(statusMutex);
}

void SeqEncoder::SetStatusReady()
{
	SDL_LockMutex(statusMutex);
	if (status < SeqEncoderStatus::Stopping)
		status = SeqEncoderStatus::Ready;
	SDL_UnlockMutex(statusMutex);
}

void SeqEncoder::SetStatusStopping()
{
	SDL_LockMutex(statusMutex);
	if (status < SeqEncoderStatus::Disposing)
		status = SeqEncoderStatus::Stopping;
	SDL_UnlockMutex(statusMutex);
}

void SeqEncoder::SetStatusDisposing()
{
	SDL_LockMutex(statusMutex);
	if (status == SeqEncoderStatus::Stopping)
		status = SeqEncoderStatus::Disposing;
	else
		SetStatusStopping();
	SDL_UnlockMutex(statusMutex);
}

void SeqEncoder::Dispose()
{
	if (status != SeqEncoderStatus::Disposing && status != SeqEncoderStatus::Inactive)
	{
		SetStatusDisposing();
		avformat_close_input(&formatContext);
		SetStatusInactive();
	}
}

int SeqEncoder::Loop()
{
	// allocate temp frame
	AVFrame* tempFrame = nullptr;
	tempFrame = av_frame_alloc();
	if (!tempFrame)
	{
		fprintf(stderr, "Could not allocate temp frame\n");
		Dispose();
		return AVERROR(ENOMEM);
	}
	// allocate temp packet
	AVPacket pkt;
	av_init_packet(&pkt);
	// read frames from the file
	while (
		status == SeqEncoderStatus::Initializing || 
		status == SeqEncoderStatus::Ready)
	{
		if (status == SeqEncoderStatus::Stopping)
			break;
	}

	// disposing...
	av_frame_free(&tempFrame);
	Dispose();

	return 0;
}

void SeqEncoder::PrintAVError(const char *message, int error)
{
	char buff[256];
	av_strerror(error, buff, 256);
	fprintf(stderr, message, buff);
}
