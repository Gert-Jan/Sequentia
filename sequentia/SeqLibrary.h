#pragma once

#include <SDL_atomic.h>

class SeqProject;
template<class T>
class SeqList;
class SeqSerializer;
class SeqLibrary;
struct SeqStreamInfo;

struct SeqLibraryLink
{
	char *fullPath;
	SDL_atomic_t useCount;
	bool metaDataLoaded;

	int64_t duration;
	int videoStreamCount;
	int audioStreamCount;
	int defaultVideoStreamIndex = -1;
	int defaultAudioStreamIndex = -1;
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