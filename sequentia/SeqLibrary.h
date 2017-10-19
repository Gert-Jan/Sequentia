#pragma once

#include <SDL_atomic.h>

class SeqProject;
template<class T>
class SeqList;
class SeqSerializer;
class SeqLibrary;
struct SeqVideoInfo;

struct SeqLibraryLink
{
	char *fullPath;
	SeqVideoInfo *info;
	SDL_atomic_t useCount;
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
	void UpdatePaths(char *oldProjectFullPath, char *newProjectFullPath);
	void Update();

	void Serialize(SeqSerializer *serializer);
	void Deserialize(SeqSerializer *serializer);

private:
	int GetLinkIndex(char *fullPath);

private:
	SeqList<SeqLibraryLink*> *links;
	SeqList<SeqLibraryLink*> *disposeLinks;
};