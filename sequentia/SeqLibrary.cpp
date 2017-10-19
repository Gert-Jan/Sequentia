#include <dirent.h>
#include <SDL.h>
#include "SeqLibrary.h";
#include "SeqList.h";
#include "SeqString.h";
#include "SeqPath.h";
#include "SeqSerializer.h";
#include "SeqWorkerManager.h";
#include "SeqTaskReadVideoInfo.h";

SeqLibrary::SeqLibrary()
{
	links = new SeqList<SeqLibraryLink*>();
	disposeLinks = new SeqList<SeqLibraryLink*>();
}

SeqLibrary::~SeqLibrary()
{
	Clear();
	delete links;
	delete disposeLinks;
}

void SeqLibrary::Clear()
{
	links->Clear();
}

void SeqLibrary::AddLink(char *fullPath)
{
	SeqLibraryLink *link = new SeqLibraryLink();
	link->fullPath = SeqPath::Normalize(fullPath);
	links->Add(link);
	SeqTaskReadVideoInfo *task = new SeqTaskReadVideoInfo(link);
	SeqWorkerManager::Instance()->PerformTask(task);
}

void SeqLibrary::RemoveLink(const int index)
{
	SeqLibraryLink *link = links->Get(index);
	disposeLinks->Add(link);
	links->RemoveAt(index);
}

void SeqLibrary::RemoveLink(char *fullPath)
{
	int index = GetLinkIndex(fullPath);
	if (index > -1)
		RemoveLink(index);
}

int SeqLibrary::LinkCount()
{
	return links->Count();
}

SeqLibraryLink* SeqLibrary::GetLink(const int index)
{
	return links->Get(index);
}

int SeqLibrary::GetLinkIndex(char *fullPath)
{
	for (int i = 0; i < LinkCount(); i++)
	{
		if (SeqString::Equals(GetLink(i)->fullPath, fullPath))
		{
			return i;
		}
	}
	return -1;
}

void SeqLibrary::UpdatePaths(char *oldProjectFullPath, char *newProjectFullPath)
{
	// TODO: update paths based on the new project path
}

void SeqLibrary::Update()
{
	for (int i = disposeLinks->Count() - 1; i >= 0; i--)
	{
		SeqLibraryLink *link = disposeLinks->Get(i);
		if (SDL_AtomicGet(&link->useCount) == 0)
		{
			disposeLinks->RemoveAt(i);
			delete[] link->fullPath;
			delete link->info;
			delete link;
		}
	}
}

void SeqLibrary::Serialize(SeqSerializer *serializer)
{
	serializer->Write(links->Count());
	for (int i = 0; i < links->Count(); i++)
	{
		SeqLibraryLink *link = links->Get(i);
		serializer->Write(link->fullPath);
	}
}

void SeqLibrary::Deserialize(SeqSerializer *serializer)
{
	int count = serializer->ReadInt();
	for (int i = 0; i < count; i++)
		AddLink(serializer->ReadString());
}