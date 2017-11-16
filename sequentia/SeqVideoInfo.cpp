#include "SeqVideoInfo.h"
#include "SeqString.h"
#include "SeqUtils.h"

SeqVideoInfo::SeqVideoInfo() :
	formatContext(nullptr),
	videoCodec(nullptr),
	audioCodec(nullptr),
	videoStreamIndex(-1),
	audioStreamIndex(-1),
	videoFrameCount(0),
	audioFrameCount(0)
{
};

SeqVideoInfo::~SeqVideoInfo()
{
	avcodec_free_context(&videoCodec);
	avcodec_free_context(&audioCodec);
	avformat_close_input(&formatContext);
};

void SeqVideoInfo::GetTimeString(char *buffer, int bufferLen, uint64_t time)
{
	if (time != AV_NOPTS_VALUE)
	{
		int hours, mins, secs, us;
		int64_t duration = time + (time <= INT64_MAX - 5000 ? 5000 : 0);
		secs = duration / AV_TIME_BASE;
		us = duration % AV_TIME_BASE;
		mins = secs / 60;
		secs %= 60;
		hours = mins / 60;
		mins %= 60;
		snprintf(buffer, bufferLen, "%02d:%02d:%02d.%02d", hours, mins, secs,
			(100 * us) / AV_TIME_BASE);
	}
	else
	{
		strcpy_s(buffer, bufferLen, "N/A");
	}
};
