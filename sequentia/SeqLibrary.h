#pragma once

extern "C"
{
	#include <libavutil/pixfmt.h>
}
#include <SDL_atomic.h>
#include <SDL_audio.h>

class SeqProject;
template<class T>
class SeqList;
class SeqSerializer;
class SeqLibrary;

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
	Audio
};

struct SeqStreamInfo
{
	int streamIndex = -1;
	double timeBase;
	SeqStreamInfoType type;
	union
	{
		SeqVideoStreamInfo videoInfo;
		SeqAudioStreamInfo audioInfo;
	};
};

struct SeqLibraryLink
{
	char *fullPath;
	SDL_atomic_t useCount;
	bool metaDataLoaded;

	int64_t duration;
	int videoStreamCount;
	int audioStreamCount;
	int defaultVideoStreamInfoIndex;
	int defaultAudioStreamInfoIndex;
	SeqStreamInfo *streamInfos;
};


class SeqLibrary
{
public:
	SeqLibrary();
	~SeqLibrary();

	void Clear();
	
	void AddLink(const char *fullPath);
	void RemoveLink(const int index);
	void RemoveLink(const char *fullPath);
	int LinkCount();
	SeqLibraryLink* GetLink(const int index);
	SeqLibraryLink* GetLink(const char *fullPath);
	int GetLinkIndex(const char *fullPath);
	void SetLastLinkFocus(SeqLibraryLink *link);
	SeqLibraryLink* GetLastLinkFocus();
	void UpdatePaths(const char *oldProjectFullPath, const char *newProjectFullPath);
	void Update();

	void Serialize(SeqSerializer *serializer);
	void Deserialize(SeqSerializer *serializer);

private:
	SeqList<SeqLibraryLink*> *links;
	SeqList<SeqLibraryLink*> *disposeLinks;
	SeqLibraryLink *lastLinkFocus;
};