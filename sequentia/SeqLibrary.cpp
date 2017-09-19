#include <dirent.h>
#include "SeqLibrary.h";
#include "SeqProject.h";
#include "SeqList.h";
#include "SeqString.h";
#include "SeqSerializer.h";

SeqLibrary::SeqLibrary(SeqProject *project) :
	project(project)
{
	links = new SeqList<SeqLibraryLink>();
}

SeqLibrary::~SeqLibrary()
{
	Clear();
	delete links;
}

void SeqLibrary::Clear()
{
	links->Clear();
}

void SeqLibrary::AddLink(char *fullPath)
{
	SeqLibraryLink link;
	link.fullPath = fullPath;
	link.relativePath = fullPath;
	links->Add(link);
}

void SeqLibrary::RemoveLink(const int index)
{
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

SeqLibraryLink SeqLibrary::GetLink(const int index)
{
	return links->Get(index);
}

int SeqLibrary::GetLinkIndex(char *fullPath)
{
	for (int i = 0; i < LinkCount(); i++)
	{
		if (SeqString::Equals(GetLink(i).fullPath, fullPath))
		{
			return i;
		}
	}
	return -1;
}

void SeqLibrary::UpdateRelativePaths(char *projectFullPath)
{
	// TODO: update relative paths based on the new project path
}

void SeqLibrary::Serialize(SeqSerializer *serializer)
{
	serializer->Write(links->Count());
	for (int i = 0; i < links->Count(); i++)
	{
		SeqLibraryLink link = links->Get(i);
		serializer->Write(link.fullPath);
		serializer->Write(link.relativePath);
	}
}

void SeqLibrary::Deserialize(SeqSerializer *serializer)
{
	int count = serializer->ReadInt();
	for (int i = 0; i < count; i++)
	{
		SeqLibraryLink link;
		link.fullPath = serializer->ReadString();
		link.relativePath = serializer->ReadString();
		links->Add(link);
	}
}