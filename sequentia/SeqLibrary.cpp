#include <dirent.h>
#include "SeqProject.h";

SeqLibrary::SeqLibrary(SeqProject *project) :
	project(project)
{
	links = new SeqList<SeqLibraryLink>();
}

SeqLibrary::~SeqLibrary()
{
	for (int i = 0; i < links->Count(); i++)
	{
		delete[] links->Get(i).fullPath;
		delete[] links->Get(i).relativePath;
	}
	delete links;
}

void SeqLibrary::AddLink(char *fullPath)
{
	SeqLibraryLink link;
	link.fullPath = fullPath;
}

void SeqLibrary::UpdateRelativePaths(char *projectFullPath)
{

}