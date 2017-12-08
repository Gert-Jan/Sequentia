#pragma once

#include <SDL_atomic.h>

class SeqProject;
template<class T>
class SeqList;
class SeqSerializer;
class SeqLibrary;
class SeqVideoInfo;

struct SeqLibraryLink
{
	char *fullPath;
	SDL_atomic_t useCount;
	bool metaDataLoaded;
	int width, height;
	uint64_t duration;
};

class SeqLibrary
{
public:
	SeqLibrary();
	~SeqLibrary();

	void Clear();
	
	void AddLink(char *fullPath);
	void RemoveLink(const int index);
	void RemoveLink(char *fullPath);
	int LinkCount();
	SeqLibraryLink* GetLink(const int index);
	SeqLibraryLink* GetLink(const char *fullPath);
	int GetLinkIndex(const char *fullPath);
	void SetLastLinkFocus(SeqLibraryLink *link);
	SeqLibraryLink* GetLastLinkFocus();
	void UpdatePaths(char *oldProjectFullPath, char *newProjectFullPath);
	void Update();

	void Serialize(SeqSerializer *serializer);
	void Deserialize(SeqSerializer *serializer);

private:
	SeqList<SeqLibraryLink*> *links;
	SeqList<SeqLibraryLink*> *disposeLinks;
	SeqLibraryLink *lastLinkFocus;
};