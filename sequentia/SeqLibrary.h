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
	int streamIndex;
	double timeBase;
	int width, height;
	AVPixelFormat pixelFormat;
};

struct SeqAudioStreamInfo
{
	int streamIndex;
	double timeBase;
	int sampleRate;
	SDL_AudioFormat format;
	int channelCount;
	bool isPlanar;
};

struct SeqLibraryLink
{
	char *fullPath;
	SDL_atomic_t useCount;
	bool metaDataLoaded;

	int64_t duration;
	int videoStreamCount;
	int audioStreamCount;
	SeqVideoStreamInfo *defaultVideoStream;
	SeqAudioStreamInfo *defaultAudioStream;
	SeqVideoStreamInfo *videoStreams;
	SeqAudioStreamInfo *audioStreams;
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