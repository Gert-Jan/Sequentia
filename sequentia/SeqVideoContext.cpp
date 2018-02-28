#include "SeqVideoContext.h"
#include "SeqString.h"
#include "SeqTime.h"

SeqVideoContext::SeqVideoContext() :
	formatContext(nullptr),
	videoCodec(nullptr),
	audioCodec(nullptr),
	videoStreamIndex(-1),
	audioStreamIndex(-1),
	videoFrameCount(0),
	audioFrameCount(0),
	timeBase(1)
{
}

SeqVideoContext::~SeqVideoContext()
{
	DisposeVideoCodec();
	DisposeAudioCodec();
	avformat_close_input(&formatContext);
}

void SeqVideoContext::DisposeVideoCodec()
{
	avcodec_free_context(&videoCodec);
}

void SeqVideoContext::DisposeAudioCodec()
{
	avcodec_free_context(&audioCodec);
}

void SeqVideoContext::GetTimeString(char *buffer, int bufferLen, int64_t time)
{
	if (time != AV_NOPTS_VALUE)
	{
		int hours, mins, secs, us;
		int64_t duration = time;
		secs = (int)(duration / AV_TIME_BASE);
		us = duration % AV_TIME_BASE;
		mins = secs / 60;
		secs %= 60;
		hours = mins / 60;
		mins %= 60;
		snprintf(buffer, bufferLen, "%02d:%02d:%02d.%02d", hours, mins, secs, (100 * us) / AV_TIME_BASE);
	}
	else
	{
		strcpy_s(buffer, bufferLen, "N/A");
	}
}

void SeqVideoContext::GetTimeString(char *buffer, int bufferLen, uint32_t time)
{
	int hours, mins, secs, us;
	secs = time / 1000;
	us = time % 1000;
	mins = secs / 60;
	secs %= 60;
	hours = mins / 60;
	mins %= 60;
	snprintf(buffer, bufferLen, "%02d:%02d:%02d.%02d", hours, mins, secs, (100 * us) / 1000);
}
