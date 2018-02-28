#include "SeqTaskReadVideoInfo.h"
#include "SeqLibrary.h"
#include "SeqDecoder.h"
#include "SeqVideoContext.h"
#include <SDL.h>

SeqTaskReadVideoInfo::SeqTaskReadVideoInfo(SeqLibraryLink *link) :
	link(link),
	progress(0)
{
	SDL_AtomicIncRef(&link->useCount);
	tempLink = new SeqLibraryLink();
	tempLink->fullPath = link->fullPath;
}

SeqTaskReadVideoInfo::~SeqTaskReadVideoInfo()
{
	delete tempLink;
}

void SeqTaskReadVideoInfo::Start()
{
	// read format
	AVFormatContext *format = nullptr;
	int bestVideoStream = -1, bestAudioStream = -1;
	SeqDecoder::OpenFormatContext(tempLink->fullPath, &format);
	tempLink->duration = format->duration;
	SeqDecoder::GetBestStream(format, AVMediaType::AVMEDIA_TYPE_VIDEO, &bestVideoStream);
	SeqDecoder::GetBestStream(format, AVMediaType::AVMEDIA_TYPE_AUDIO, &bestAudioStream);

	// count video/audio streams and create arrays for info for both types
	for (int i = 0; i < format->nb_streams; i++)
	{
		switch (format->streams[i]->codecpar->codec_type)
		{
			case AVMediaType::AVMEDIA_TYPE_VIDEO:
				tempLink->videoStreamCount++;
				break;
			case AVMediaType::AVMEDIA_TYPE_AUDIO:
				tempLink->audioStreamCount++;
				break;
		}
	}
	if (tempLink->videoStreamCount > 0)
		tempLink->videoStreams = new SeqVideoStreamInfo[tempLink->videoStreamCount];
	if (tempLink->audioStreamCount > 0)
		tempLink->audioStreams = new SeqAudioStreamInfo[tempLink->audioStreamCount];

	// read codec info for all relevant streams
	AVCodecContext *codec = nullptr;
	double timeBase;
	int videoIndex = 0;
	int audioIndex = 0;
	for (int i = 0; i < format->nb_streams; i++)
	{
		SeqDecoder::OpenCodecContext(i, format, nullptr, &codec, &timeBase);
		AVMediaType type = format->streams[i]->codecpar->codec_type;
		if (type == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			SeqVideoStreamInfo *videoInfo = &tempLink->videoStreams[videoIndex];
			videoInfo->streamIndex = i;
			videoInfo->timeBase = timeBase;
			videoInfo->width = codec->width;
			videoInfo->height = codec->height;
			videoInfo->pixelFormat = codec->pix_fmt;
			if (i == bestVideoStream)
				tempLink->defaultVideoStream = &tempLink->videoStreams[videoIndex];
			videoIndex++;
		}
		else if (type == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			SeqAudioStreamInfo *audioInfo = &tempLink->audioStreams[audioIndex];
			audioInfo->streamIndex = i;
			audioInfo->timeBase = timeBase;
			audioInfo->sampleRate = codec->sample_rate;
			audioInfo->channelCount = codec->channels;
			audioInfo->format = FromAVSampleFormat(codec->sample_fmt, &audioInfo->isPlanar);
			if (i == bestAudioStream)
				tempLink->defaultAudioStream = &tempLink->audioStreams[audioIndex];
			audioIndex++;
		}
		SeqDecoder::CloseCodecContext(&codec);
	}
	
	// delete format context
	SeqDecoder::CloseFormatContext(&format);

	// we're done here
	progress = 1;
}

void SeqTaskReadVideoInfo::Stop()
{
}

void SeqTaskReadVideoInfo::Finalize()
{
	// we should not read the metadata twice (otherwise we would need to delete video/audio streams).
	SDL_assert(link->videoStreams == nullptr);
	SDL_assert(link->audioStreams == nullptr);
	// copy the data aquired in the thread to SeqLibrary
	int dataOffset = (char*)&link->duration - (char*)&link->fullPath;
	rsize_t size = sizeof(SeqLibraryLink) - dataOffset;
	memcpy_s((&link->duration), size, (&tempLink->duration), size);
	// update the state indicating the metaData has been aquired
	link->metaDataLoaded = true;
	SDL_AtomicDecRef(&link->useCount);
	// we're done here remove self
	delete this;
}

SeqWorkerTaskPriority SeqTaskReadVideoInfo::GetPriority()
{
	return SeqWorkerTaskPriority::Medium;
}

float SeqTaskReadVideoInfo::GetProgress()
{
	return progress;
}

SDL_AudioFormat SeqTaskReadVideoInfo::FromAVSampleFormat(AVSampleFormat format, bool *isPlanar)
{
	*isPlanar = false;
	switch (format)
	{
		case AV_SAMPLE_FMT_U8P:
			*isPlanar = true;
		case AV_SAMPLE_FMT_U8:
			return AUDIO_U8;
		case AV_SAMPLE_FMT_S16P:
			*isPlanar = true;
		case AV_SAMPLE_FMT_S16:
			return AUDIO_S16;
		case AV_SAMPLE_FMT_S32P:
			*isPlanar = true;
		case AV_SAMPLE_FMT_S32:
			return AUDIO_S32;
		case AV_SAMPLE_FMT_S64P:
			*isPlanar = true;
		case AV_SAMPLE_FMT_S64:
			return 0x8040;
		case AV_SAMPLE_FMT_FLTP:
			*isPlanar = true;
		case AV_SAMPLE_FMT_FLT:
			return AUDIO_F32;
		case AV_SAMPLE_FMT_DBLP:
			*isPlanar = true;
		case AV_SAMPLE_FMT_DBL:
			return 0x8140;
	}
}
