#pragma once
extern "C"
{
	#include <libavutil/pixfmt.h>
}
#include <SDL_audio.h>

struct SeqVideoStreamInfo
{
	int width, height;
	AVPixelFormat pixelFormat;
};

struct SeqAudioStreamInfo
{
	int sampleRate;
	SDL_AudioFormat format;
	int channelCount;
	bool isPlanar;
};

enum class SeqStreamInfoType
{
	Video,
	Audio,
	Other
};

struct SeqStreamInfo
{
	int streamIndex = -1;
	double timeBase = 1;
	SeqStreamInfoType type;
	union
	{
		SeqVideoStreamInfo videoInfo;
		SeqAudioStreamInfo audioInfo;
	};
};