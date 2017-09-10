#include <dirent.h>
#include "SeqLibrary.h";
#include "SeqProject.h";
#include "SeqList.h";
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
	for (int i = 0; i < links->Count(); i++)
	{
		delete[] links->Get(i).fullPath;
		delete[] links->Get(i).relativePath;
	}
	links->Clear();
}

void SeqLibrary::AddLink(char *fullPath)
{
	SeqLibraryLink link;
	link.fullPath = fullPath;
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